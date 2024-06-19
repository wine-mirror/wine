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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#ifdef HAVE_LIBUDEV_H
# include <libudev.h>
#endif
#ifdef HAVE_LINUX_HIDRAW_H
# include <linux/hidraw.h>
#endif
#ifdef HAVE_SYS_INOTIFY_H
# include <sys/inotify.h>
#endif
#include <limits.h>

#ifdef HAVE_LINUX_INPUT_H
# include <linux/input.h>
# undef SW_MAX
# if defined(EVIOCGBIT) && defined(EV_ABS) && defined(BTN_PINKIE)
#  define HAS_PROPER_INPUT_HEADER
# endif
# ifndef SYN_DROPPED
#  define SYN_DROPPED 3
# endif
#endif

#ifndef BUS_BLUETOOTH
# define BUS_BLUETOOTH 5
#endif

#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/hidtypes.h"
#include "ddk/hidsdi.h"

#include "wine/debug.h"
#include "wine/hid.h"
#include "wine/unixlib.h"

#ifdef HAS_PROPER_INPUT_HEADER
# include "hidusage.h"
#endif

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

#ifdef HAVE_UDEV

static pthread_mutex_t udev_cs = PTHREAD_MUTEX_INITIALIZER;

static struct udev *udev_context = NULL;
static struct udev_monitor *udev_monitor;
static int deviceloop_control[2];
static struct list event_queue = LIST_INIT(event_queue);
static struct list device_list = LIST_INIT(device_list);
static const struct bus_options *options;

struct base_device
{
    struct unix_device unix_device;
    void (*read_report)(struct unix_device *iface);
    BOOL started;

    struct udev_device *udev_device;
    char devnode[MAX_PATH];
    int device_fd;
};

static inline struct base_device *impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(iface, struct base_device, unix_device);
}

struct hidraw_device
{
    struct base_device base;
};

static inline struct hidraw_device *hidraw_impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(impl_from_unix_device(iface), struct hidraw_device, base);
}

#ifdef HAS_PROPER_INPUT_HEADER

static const USAGE_AND_PAGE absolute_usages[ABS_CNT] =
{
    {.UsagePage = HID_USAGE_PAGE_GENERIC,    .Usage = HID_USAGE_GENERIC_X},              /* ABS_X */
    {.UsagePage = HID_USAGE_PAGE_GENERIC,    .Usage = HID_USAGE_GENERIC_Y},              /* ABS_Y */
    {.UsagePage = HID_USAGE_PAGE_GENERIC,    .Usage = HID_USAGE_GENERIC_Z},              /* ABS_Z */
    {.UsagePage = HID_USAGE_PAGE_GENERIC,    .Usage = HID_USAGE_GENERIC_RX},             /* ABS_RX */
    {.UsagePage = HID_USAGE_PAGE_GENERIC,    .Usage = HID_USAGE_GENERIC_RY},             /* ABS_RY */
    {.UsagePage = HID_USAGE_PAGE_GENERIC,    .Usage = HID_USAGE_GENERIC_RZ},             /* ABS_RZ */
    {.UsagePage = HID_USAGE_PAGE_SIMULATION, .Usage = HID_USAGE_SIMULATION_THROTTLE},    /* ABS_THROTTLE */
    {.UsagePage = HID_USAGE_PAGE_SIMULATION, .Usage = HID_USAGE_SIMULATION_RUDDER},      /* ABS_RUDDER */
    {.UsagePage = HID_USAGE_PAGE_GENERIC,    .Usage = HID_USAGE_GENERIC_WHEEL},          /* ABS_WHEEL */
    {.UsagePage = HID_USAGE_PAGE_SIMULATION, .Usage = HID_USAGE_SIMULATION_ACCELERATOR}, /* ABS_GAS */
    {.UsagePage = HID_USAGE_PAGE_SIMULATION, .Usage = HID_USAGE_SIMULATION_BRAKE},       /* ABS_BRAKE */
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},                                                                                 /* ABS_HAT0X */
    {0},                                                                                 /* ABS_HAT0Y */
    {0},                                                                                 /* ABS_HAT1X */
    {0},                                                                                 /* ABS_HAT1Y */
    {0},                                                                                 /* ABS_HAT2X */
    {0},                                                                                 /* ABS_HAT2Y */
    {0},                                                                                 /* ABS_HAT3X */
    {0},                                                                                 /* ABS_HAT3Y */
    {.UsagePage = HID_USAGE_PAGE_DIGITIZER,  .Usage = HID_USAGE_DIGITIZER_TIP_PRESSURE}, /* ABS_PRESSURE */
    {0},                                                                                 /* ABS_DISTANCE */
    {.UsagePage = HID_USAGE_PAGE_DIGITIZER,  .Usage = HID_USAGE_DIGITIZER_X_TILT},       /* ABS_TILT_X */
    {.UsagePage = HID_USAGE_PAGE_DIGITIZER,  .Usage = HID_USAGE_DIGITIZER_Y_TILT},       /* ABS_TILT_Y */
    {0},                                                                                 /* ABS_TOOL_WIDTH */
    {0},
    {0},
    {0},
    {.UsagePage = HID_USAGE_PAGE_CONSUMER,   .Usage = HID_USAGE_CONSUMER_VOLUME},        /* ABS_VOLUME */
};

static const USAGE_AND_PAGE relative_usages[REL_CNT] =
{
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_X},     /* REL_X */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_Y},     /* REL_Y */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_Z},     /* REL_Z */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RX},    /* REL_RX */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RY},    /* REL_RY */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RZ},    /* REL_RZ */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_SLIDER},/* REL_HWHEEL */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_DIAL},  /* REL_DIAL */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_WHEEL}, /* REL_WHEEL */
    {0},                                                                     /* REL_MISC */
};

struct lnxev_info
{
    struct input_id id;
    char name[MAX_PATH];
    char uniq[MAX_PATH];
    BYTE abs[(ABS_CNT + 7) / 8];
    BYTE rel[(REL_CNT + 7) / 8];
    BYTE key[(KEY_CNT + 7) / 8];
    BYTE ff[(FF_CNT + 7) / 8];
};

struct lnxev_device
{
    struct base_device base;

    LONG abs_min[ABS_CNT];
    LONG abs_max[ABS_CNT];
    BYTE abs_map[ABS_CNT];
    BYTE rel_map[REL_CNT];
    BYTE hat_map[8];
    BYTE button_map[KEY_CNT];
    int hat_count;
    int button_count;
    BOOL is_gamepad;

    int haptic_effect_id;
    int effect_ids[256];
    LONG effect_flags;
};

static inline struct lnxev_device *lnxev_impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(impl_from_unix_device(iface), struct lnxev_device, base);
}

#endif /* HAS_PROPER_INPUT_HEADER */

#define MAX_DEVICES 128
static int close_fds[MAX_DEVICES];
static struct pollfd poll_fds[MAX_DEVICES];
static struct base_device *poll_devs[MAX_DEVICES];
static int close_count, poll_count;

static void stop_polling_device(struct unix_device *iface)
{
    struct base_device *impl = impl_from_unix_device(iface);
    int i;

    if (impl->device_fd == -1) return; /* already removed */
    if (!impl->started) return; /* not started */
    impl->started = FALSE;

    for (i = 2; i < poll_count; ++i)
        if (poll_fds[i].fd == impl->device_fd) break;

    if (i == poll_count)
        ERR("could not find poll entry matching device %p fd\n", iface);
    else
    {
        poll_count--;
        poll_fds[i] = poll_fds[poll_count];
        poll_devs[i] = poll_devs[poll_count];
        close_fds[close_count++] = impl->device_fd;
        impl->device_fd = -1;
    }
}

static void start_polling_device(struct unix_device *iface)
{
    struct base_device *impl = impl_from_unix_device(iface);

    if (poll_count >= ARRAY_SIZE(poll_fds))
        ERR("could not start polling device %p, too many fds\n", iface);
    else
    {
        poll_devs[poll_count] = impl;
        poll_fds[poll_count].fd = impl->device_fd;
        poll_fds[poll_count].events = POLLIN;
        poll_fds[poll_count].revents = 0;
        poll_count++;

        write(deviceloop_control[1], "u", 1);
        impl->started = TRUE;
    }
}

static struct base_device *find_device_from_fd(int fd)
{
    int i;

    for (i = 2; i < poll_count; ++i) if (poll_fds[i].fd == fd) break;
    if (i < poll_count) return  poll_devs[i];

    return NULL;
}

static struct base_device *find_device_from_devnode(const char *path)
{
    struct base_device *impl;

    LIST_FOR_EACH_ENTRY(impl, &device_list, struct base_device, unix_device.entry)
        if (!strcmp(impl->devnode, path)) return impl;

    return NULL;
}

static void hidraw_device_destroy(struct unix_device *iface)
{
    struct hidraw_device *impl = hidraw_impl_from_unix_device(iface);

    udev_device_unref(impl->base.udev_device);
}

static NTSTATUS hidraw_device_start(struct unix_device *iface)
{
    pthread_mutex_lock(&udev_cs);
    start_polling_device(iface);
    pthread_mutex_unlock(&udev_cs);
    return STATUS_SUCCESS;
}

static void hidraw_device_stop(struct unix_device *iface)
{
    struct hidraw_device *impl = hidraw_impl_from_unix_device(iface);

    pthread_mutex_lock(&udev_cs);
    stop_polling_device(iface);
    list_remove(&impl->base.unix_device.entry);
    pthread_mutex_unlock(&udev_cs);
}

static NTSTATUS hidraw_device_get_report_descriptor(struct unix_device *iface, BYTE *buffer,
                                                    UINT length, UINT *out_length)
{
#ifdef HAVE_LINUX_HIDRAW_H
    struct hidraw_report_descriptor descriptor;
    struct hidraw_device *impl = hidraw_impl_from_unix_device(iface);

    if (ioctl(impl->base.device_fd, HIDIOCGRDESCSIZE, &descriptor.size) == -1)
    {
        WARN("ioctl(HIDIOCGRDESCSIZE) failed: %d %s\n", errno, strerror(errno));
        return STATUS_UNSUCCESSFUL;
    }

    *out_length = descriptor.size;

    if (length < descriptor.size)
        return STATUS_BUFFER_TOO_SMALL;
    if (!descriptor.size)
        return STATUS_SUCCESS;

    if (ioctl(impl->base.device_fd, HIDIOCGRDESC, &descriptor) == -1)
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

static void hidraw_device_read_report(struct unix_device *iface)
{
    struct hidraw_device *impl = hidraw_impl_from_unix_device(iface);
    BYTE report_buffer[1024], *buff = report_buffer;

    int size = read(impl->base.device_fd, report_buffer, sizeof(report_buffer));
    if (size == -1)
        TRACE("Read failed. Likely an unplugged device %d %s\n", errno, strerror(errno));
    else if (size == 0)
        TRACE("Failed to read report\n");
    else
        bus_event_queue_input_report(&event_queue, iface, buff, size);
}

static void hidraw_device_set_output_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct hidraw_device *impl = hidraw_impl_from_unix_device(iface);
    unsigned int length = packet->reportBufferLen;
    BYTE buffer[8192];
    int count = 0;

    if ((buffer[0] = packet->reportId))
        count = write(impl->base.device_fd, packet->reportBuffer, length);
    else if (length > sizeof(buffer) - 1)
        ERR("id %d length %u >= 8192, cannot write\n", packet->reportId, length);
    else
    {
        memcpy(buffer + 1, packet->reportBuffer, length);
        count = write(impl->base.device_fd, buffer, length + 1);
    }

    if (count > 0)
    {
        io->Information = count;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        ERR("id %d write failed error: %d %s\n", packet->reportId, errno, strerror(errno));
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
}

static void hidraw_device_get_feature_report(struct unix_device *iface, HID_XFER_PACKET *packet,
                                             IO_STATUS_BLOCK *io)
{
#if defined(HAVE_LINUX_HIDRAW_H) && defined(HIDIOCGFEATURE)
    struct hidraw_device *impl = hidraw_impl_from_unix_device(iface);
    unsigned int length = packet->reportBufferLen;
    BYTE buffer[8192];
    int count = 0;

    if ((buffer[0] = packet->reportId) && length <= 0x1fff)
        count = ioctl(impl->base.device_fd, HIDIOCGFEATURE(length), packet->reportBuffer);
    else if (length > sizeof(buffer) - 1)
        ERR("id %d length %u >= 8192, cannot read\n", packet->reportId, length);
    else
    {
        count = ioctl(impl->base.device_fd, HIDIOCGFEATURE(length + 1), buffer);
        memcpy(packet->reportBuffer, buffer + 1, length);
    }

    if (count > 0)
    {
        io->Information = count;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        ERR("id %d read failed, error: %d %s\n", packet->reportId, errno, strerror(errno));
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
#else
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
#endif
}

static void hidraw_device_set_feature_report(struct unix_device *iface, HID_XFER_PACKET *packet,
                                             IO_STATUS_BLOCK *io)
{
#if defined(HAVE_LINUX_HIDRAW_H) && defined(HIDIOCSFEATURE)
    struct hidraw_device *impl = hidraw_impl_from_unix_device(iface);
    unsigned int length = packet->reportBufferLen;
    BYTE buffer[8192];
    int count = 0;

    if ((buffer[0] = packet->reportId) && length <= 0x1fff)
        count = ioctl(impl->base.device_fd, HIDIOCSFEATURE(length), packet->reportBuffer);
    else if (length > sizeof(buffer) - 1)
        ERR("id %d length %u >= 8192, cannot write\n", packet->reportId, length);
    else
    {
        memcpy(buffer + 1, packet->reportBuffer, length);
        count = ioctl(impl->base.device_fd, HIDIOCSFEATURE(length + 1), buffer);
    }

    if (count > 0)
    {
        io->Information = count;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        ERR("id %d write failed, error: %d %s\n", packet->reportId, errno, strerror(errno));
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
#else
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
#endif
}

static const struct raw_device_vtbl hidraw_device_vtbl =
{
    hidraw_device_destroy,
    hidraw_device_start,
    hidraw_device_stop,
    hidraw_device_get_report_descriptor,
    hidraw_device_set_output_report,
    hidraw_device_get_feature_report,
    hidraw_device_set_feature_report,
};

#ifdef HAS_PROPER_INPUT_HEADER

#define test_bit(arr,bit) (((BYTE*)(arr))[(bit)>>3]&(1<<((bit)&7)))

static const USAGE_AND_PAGE *what_am_I(struct udev_device *dev, int fd)
{
    static const USAGE_AND_PAGE Unknown     = {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = 0};
    static const USAGE_AND_PAGE Mouse       = {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_MOUSE};
    static const USAGE_AND_PAGE Keyboard    = {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_KEYBOARD};
    static const USAGE_AND_PAGE Gamepad     = {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_GAMEPAD};
    static const USAGE_AND_PAGE Keypad      = {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_KEYPAD};
    static const USAGE_AND_PAGE Tablet      = {.UsagePage = HID_USAGE_PAGE_DIGITIZER, .Usage = HID_USAGE_DIGITIZER_PEN};
    static const USAGE_AND_PAGE Touchscreen = {.UsagePage = HID_USAGE_PAGE_DIGITIZER, .Usage = HID_USAGE_DIGITIZER_TOUCH_SCREEN};
    static const USAGE_AND_PAGE Touchpad    = {.UsagePage = HID_USAGE_PAGE_DIGITIZER, .Usage = HID_USAGE_DIGITIZER_TOUCH_PAD};

    struct udev_device *parent = dev;

    /* Look to the parents until we get a clue */
    while (parent)
    {
        if (udev_device_get_property_value(parent, "ID_INPUT_MOUSE"))
            return &Mouse;
        else if (udev_device_get_property_value(parent, "ID_INPUT_KEYBOARD"))
            return &Keyboard;
        else if (udev_device_get_property_value(parent, "ID_INPUT_JOYSTICK"))
            return &Gamepad;
        else if (udev_device_get_property_value(parent, "ID_INPUT_KEY"))
            return &Keypad;
        else if (udev_device_get_property_value(parent, "ID_INPUT_TOUCHPAD"))
            return &Touchpad;
        else if (udev_device_get_property_value(parent, "ID_INPUT_TOUCHSCREEN"))
            return &Touchscreen;
        else if (udev_device_get_property_value(parent, "ID_INPUT_TABLET"))
            return &Tablet;

        parent = udev_device_get_parent_with_subsystem_devtype(parent, "input", NULL);
    }

    return &Unknown;
}

static void set_abs_axis_value(struct unix_device *iface, int code, int value)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);

    if (code < ABS_HAT0X || code > ABS_HAT3Y)
    {
        LONG min = impl->abs_min[code], range = impl->abs_max[code] - impl->abs_min[code];
        if (!(code = impl->abs_map[code])) return;

        if (impl->is_gamepad)
        {
            double scale = (code == 5 || code == 6 ? 32767.0 : 65535.0) / range;
            value = (value - min) * scale - (code == 5 || code == 6 ? 0 : 32768);
            if (code == 2 || code == 4) value = -value - 1; /* match XUSB / GIP protocol */
        }

        hid_device_set_abs_axis(iface, code - 1, value);
    }
    else if ((code - ABS_HAT0X) % 2)
    {
        if (!(code = impl->hat_map[code - ABS_HAT0X])) return;
        if (impl->is_gamepad)
        {
            hid_device_set_button(iface, 10, value < 0);
            hid_device_set_button(iface, 11, value > 0);
        }
        hid_device_set_hatswitch_y(iface, code - 1, value);
    }
    else
    {
        if (!(code = impl->hat_map[code - ABS_HAT0X])) return;
        if (impl->is_gamepad)
        {
            hid_device_set_button(iface, 12, value < 0);
            hid_device_set_button(iface, 13, value > 0);
        }
        hid_device_set_hatswitch_x(iface, code - 1, value);
    }
}

static NTSTATUS build_report_descriptor(struct unix_device *iface, struct udev_device *dev, struct lnxev_info *info)
{
    struct input_absinfo abs_info[ABS_CNT];
    USHORT count = 0;
    USAGE usages[16];
    int i;
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    const USAGE_AND_PAGE device_usage = *what_am_I(dev, impl->base.device_fd);

    if (!hid_device_begin_report_descriptor(iface, &device_usage))
        return STATUS_NO_MEMORY;

    if (impl->is_gamepad)
    {
        if (!hid_device_add_gamepad(iface))
            return STATUS_NO_MEMORY;
    }
    else
    {
        if (!hid_device_begin_input_report(iface, &device_usage))
            return STATUS_NO_MEMORY;

        for (i = 0; i < ABS_CNT; i++)
        {
            LONG min = impl->abs_min[i], max = impl->abs_max[i];
            USAGE_AND_PAGE usage = absolute_usages[i];
            if (!impl->abs_map[i]) continue;
            ioctl(impl->base.device_fd, EVIOCGABS(i), abs_info + i);
            if (!hid_device_add_axes(iface, 1, usage.UsagePage, &usage.Usage, FALSE,
                                     min, max))
                return STATUS_NO_MEMORY;
        }

        for (i = 0; i < REL_CNT; i++)
        {
            USAGE_AND_PAGE usage = relative_usages[i];
            if (!impl->rel_map[i]) continue;
            if (!hid_device_add_axes(iface, 1, usage.UsagePage, &usage.Usage, TRUE,
                                     INT32_MIN, INT32_MAX))
                return STATUS_NO_MEMORY;
        }

        if (impl->hat_count && !hid_device_add_hatswitch(iface, impl->hat_count)) return STATUS_NO_MEMORY;
        if (impl->button_count && !hid_device_add_buttons(iface, HID_USAGE_PAGE_BUTTON, 1, impl->button_count)) return STATUS_NO_MEMORY;

        if (!hid_device_end_input_report(iface))
            return STATUS_NO_MEMORY;
    }

    impl->haptic_effect_id = -1;
    for (i = 0; i < ARRAY_SIZE(impl->effect_ids); ++i) impl->effect_ids[i] = -1;

    if (test_bit(info->ff, FF_RUMBLE) && !hid_device_add_haptics(iface))
        return STATUS_NO_MEMORY;

    for (i = 0; i < FF_MAX; ++i) if (test_bit(info->ff, i)) break;
    if (i != FF_MAX)
    {
        if (test_bit(info->ff, FF_SINE)) usages[count++] = PID_USAGE_ET_SINE;
        if (test_bit(info->ff, FF_SQUARE)) usages[count++] = PID_USAGE_ET_SQUARE;
        if (test_bit(info->ff, FF_TRIANGLE)) usages[count++] = PID_USAGE_ET_TRIANGLE;
        if (test_bit(info->ff, FF_SAW_UP)) usages[count++] = PID_USAGE_ET_SAWTOOTH_UP;
        if (test_bit(info->ff, FF_SAW_DOWN)) usages[count++] = PID_USAGE_ET_SAWTOOTH_DOWN;
        if (test_bit(info->ff, FF_SPRING)) usages[count++] = PID_USAGE_ET_SPRING;
        if (test_bit(info->ff, FF_DAMPER)) usages[count++] = PID_USAGE_ET_DAMPER;
        if (test_bit(info->ff, FF_INERTIA)) usages[count++] = PID_USAGE_ET_INERTIA;
        if (test_bit(info->ff, FF_FRICTION)) usages[count++] = PID_USAGE_ET_FRICTION;
        if (test_bit(info->ff, FF_CONSTANT)) usages[count++] = PID_USAGE_ET_CONSTANT_FORCE;
        if (test_bit(info->ff, FF_RAMP)) usages[count++] = PID_USAGE_ET_RAMP;

        if (!hid_device_add_physical(iface, usages, count, 2))
            return STATUS_NO_MEMORY;
    }

    if (!hid_device_end_report_descriptor(iface))
        return STATUS_NO_MEMORY;

    /* Initialize axis in the report */
    for (i = 0; i < ABS_CNT; i++)
    {
        USAGE_AND_PAGE usage = absolute_usages[i];
        if (!usage.UsagePage || !usage.Usage) continue;
        if (!test_bit(info->abs, i)) continue;
        set_abs_axis_value(iface, i, abs_info[i].value);
    }

    return STATUS_SUCCESS;
}

static BOOL set_report_from_event(struct unix_device *iface, struct input_event *ie)
{
    struct hid_effect_state *effect_state = &iface->hid_physical.effect_state;
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    ULONG effect_flags = InterlockedOr(&impl->effect_flags, 0);
    unsigned int i, button;

    switch (ie->type)
    {
#ifdef EV_SYN
    case EV_SYN:
        switch (ie->code)
        {
        case SYN_REPORT: return hid_device_sync_report(iface);
        case SYN_DROPPED: hid_device_drop_report(iface); break;
        }
        return FALSE;
#endif
#ifdef EV_MSC
    case EV_MSC:
        return FALSE;
#endif
    case EV_KEY:
        if (!(button = impl->button_map[ie->code])) return FALSE;
        if (impl->is_gamepad && !impl->hat_count)
        {
            if (button == 11) hid_device_set_hatswitch_y(iface, 0, -1);
            if (button == 12) hid_device_set_hatswitch_y(iface, 0, +1);
            if (button == 13) hid_device_set_hatswitch_x(iface, 0, -1);
            if (button == 14) hid_device_set_hatswitch_x(iface, 0, +1);
        }
        hid_device_set_button(iface, button - 1, ie->value);
        return FALSE;
    case EV_ABS:
        set_abs_axis_value(iface, ie->code, ie->value);
        return FALSE;
    case EV_REL:
        hid_device_set_rel_axis(iface, impl->rel_map[ie->code], ie->value);
        return FALSE;
    case EV_FF_STATUS:
        for (i = 0; i < ARRAY_SIZE(impl->effect_ids); ++i) if (impl->effect_ids[i] == ie->code) break;
        if (i == ARRAY_SIZE(impl->effect_ids)) return FALSE;

        if (ie->value == FF_STATUS_PLAYING) effect_flags |= EFFECT_STATE_EFFECT_PLAYING;
        hid_device_set_effect_state(iface, i, effect_flags);
        bus_event_queue_input_report(&event_queue, iface, effect_state->report_buf, effect_state->report_len);
        return FALSE;
    default:
        ERR("TODO: Process Report (%i, %i)\n",ie->type, ie->code);
        return FALSE;
    }
}

static void lnxev_device_destroy(struct unix_device *iface)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    udev_device_unref(impl->base.udev_device);
}

static NTSTATUS lnxev_device_start(struct unix_device *iface)
{
    pthread_mutex_lock(&udev_cs);
    start_polling_device(iface);
    pthread_mutex_unlock(&udev_cs);
    return STATUS_SUCCESS;
}

static void lnxev_device_stop(struct unix_device *iface)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);

    pthread_mutex_lock(&udev_cs);
    stop_polling_device(iface);
    list_remove(&impl->base.unix_device.entry);
    pthread_mutex_unlock(&udev_cs);
}

static void lnxev_device_read_report(struct unix_device *iface)
{
    struct hid_device_state *state = &iface->hid_device_state;
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    struct input_event ie;
    int size;

    size = read(impl->base.device_fd, &ie, sizeof(ie));
    if (size == -1)
        TRACE("Read failed. Likely an unplugged device\n");
    else if (size == 0)
        TRACE("Failed to read report\n");
    else if (set_report_from_event(iface, &ie))
        bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
}

static NTSTATUS lnxev_device_haptics_start(struct unix_device *iface, UINT duration_ms,
                                           USHORT rumble_intensity, USHORT buzz_intensity,
                                           USHORT left_intensity, USHORT right_intensity)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    struct ff_effect effect =
    {
        .id = impl->haptic_effect_id,
        .type = FF_RUMBLE,
    };
    struct input_event event;

    TRACE("iface %p, duration_ms %u, rumble_intensity %u, buzz_intensity %u, left_intensity %u, right_intensity %u.\n",
          iface, duration_ms, rumble_intensity, buzz_intensity, left_intensity, right_intensity);

    effect.replay.length = duration_ms;
    effect.u.rumble.strong_magnitude = rumble_intensity;
    effect.u.rumble.weak_magnitude = buzz_intensity;

    if (effect.id == -1 || ioctl(impl->base.device_fd, EVIOCSFF, &effect) == -1)
    {
        effect.id = -1;
        if (ioctl(impl->base.device_fd, EVIOCSFF, &effect) == 1)
        {
            WARN("couldn't re-allocate rumble effect for haptics: %d %s\n", errno, strerror(errno));
            return STATUS_UNSUCCESSFUL;
        }
        impl->haptic_effect_id = effect.id;
    }

    event.type = EV_FF;
    event.code = effect.id;
    event.value = 1;
    if (write(impl->base.device_fd, &event, sizeof(event)) == -1)
    {
        WARN("couldn't start haptics rumble effect: %d %s\n", errno, strerror(errno));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS lnxev_device_haptics_stop(struct unix_device *iface)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    struct ff_effect effect =
    {
        .id = impl->haptic_effect_id,
        .type = FF_RUMBLE,
    };
    struct input_event event;

    TRACE("iface %p.\n", iface);

    if (effect.id == -1) return STATUS_SUCCESS;

    event.type = EV_FF;
    event.code = effect.id;
    event.value = 0;
    if (write(impl->base.device_fd, &event, sizeof(event)) == -1)
        WARN("couldn't stop haptics rumble effect: %d %s\n", errno, strerror(errno));

    return STATUS_SUCCESS;
}

static NTSTATUS lnxev_device_physical_effect_run(struct lnxev_device *impl, BYTE index,
                                                 int iterations)
{
    struct input_event ie =
    {
        .type = EV_FF,
        .value = iterations,
    };

    if (impl->effect_ids[index] < 0) return STATUS_UNSUCCESSFUL;
    ie.code = impl->effect_ids[index];

    if (write(impl->base.device_fd, &ie, sizeof(ie)) == -1)
    {
        WARN("couldn't stop effect, write failed %d %s\n", errno, strerror(errno));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS lnxev_device_physical_device_control(struct unix_device *iface, USAGE control)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    unsigned int i;

    TRACE("iface %p, control %#04x.\n", iface, control);

    switch (control)
    {
    case PID_USAGE_DC_ENABLE_ACTUATORS:
    {
        struct input_event ie =
        {
            .type = EV_FF,
            .code = FF_GAIN,
            .value = 0xffff,
        };
        if (write(impl->base.device_fd, &ie, sizeof(ie)) == -1)
            WARN("write failed %d %s\n", errno, strerror(errno));
        else
            InterlockedOr(&impl->effect_flags, EFFECT_STATE_ACTUATORS_ENABLED);
        return STATUS_SUCCESS;
    }
    case PID_USAGE_DC_DISABLE_ACTUATORS:
    {
        struct input_event ie =
        {
            .type = EV_FF,
            .code = FF_GAIN,
            .value = 0,
        };
        if (write(impl->base.device_fd, &ie, sizeof(ie)) == -1)
            WARN("write failed %d %s\n", errno, strerror(errno));
        else
            InterlockedAnd(&impl->effect_flags, ~EFFECT_STATE_ACTUATORS_ENABLED);
        return STATUS_SUCCESS;
    }
    case PID_USAGE_DC_STOP_ALL_EFFECTS:
        for (i = 0; i < ARRAY_SIZE(impl->effect_ids); ++i)
        {
            if (impl->effect_ids[i] < 0) continue;
            lnxev_device_physical_effect_run(impl, i, 0);
        }
        return STATUS_SUCCESS;
    case PID_USAGE_DC_DEVICE_RESET:
        for (i = 0; i < ARRAY_SIZE(impl->effect_ids); ++i)
        {
            if (impl->effect_ids[i] < 0) continue;
            if (ioctl(impl->base.device_fd, EVIOCRMFF, impl->effect_ids[i]) == -1)
                WARN("couldn't free effect, EVIOCRMFF ioctl failed: %d %s\n", errno, strerror(errno));
            impl->effect_ids[i] = -1;
        }
        return STATUS_SUCCESS;
    case PID_USAGE_DC_DEVICE_PAUSE:
        WARN("device pause not supported\n");
        InterlockedOr(&impl->effect_flags, EFFECT_STATE_DEVICE_PAUSED);
        return STATUS_NOT_SUPPORTED;
    case PID_USAGE_DC_DEVICE_CONTINUE:
        WARN("device continue not supported\n");
        InterlockedAnd(&impl->effect_flags, ~EFFECT_STATE_DEVICE_PAUSED);
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS lnxev_device_physical_device_set_gain(struct unix_device *iface, BYTE percent)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    struct input_event ie =
    {
        .type = EV_FF,
        .code = FF_GAIN,
        .value = 0xffff * percent / 100,
    };

    TRACE("iface %p, percent %#x.\n", iface, percent);

    if (write(impl->base.device_fd, &ie, sizeof(ie)) == -1)
        WARN("write failed %d %s\n", errno, strerror(errno));

    return STATUS_SUCCESS;
}

static NTSTATUS lnxev_device_physical_effect_control(struct unix_device *iface, BYTE index,
                                                     USAGE control, BYTE iterations)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    NTSTATUS status;

    TRACE("iface %p, index %u, control %04x, iterations %u.\n", iface, index, control, iterations);

    switch (control)
    {
    case PID_USAGE_OP_EFFECT_START_SOLO:
        if ((status = lnxev_device_physical_device_control(iface, PID_USAGE_DC_STOP_ALL_EFFECTS)))
            return status;
        /* fallthrough */
    case PID_USAGE_OP_EFFECT_START:
        return lnxev_device_physical_effect_run(impl, index, iterations);
    case PID_USAGE_OP_EFFECT_STOP:
        return lnxev_device_physical_effect_run(impl, index, 0);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS set_effect_type_from_usage(struct ff_effect *effect, USAGE type)
{
    switch (type)
    {
    case PID_USAGE_ET_SINE:
        effect->type = FF_PERIODIC;
        effect->u.periodic.waveform = FF_SINE;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_SQUARE:
        effect->type = FF_PERIODIC;
        effect->u.periodic.waveform = FF_SQUARE;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_TRIANGLE:
        effect->type = FF_PERIODIC;
        effect->u.periodic.waveform = FF_TRIANGLE;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_SAWTOOTH_UP:
        effect->type = FF_PERIODIC;
        effect->u.periodic.waveform = FF_SAW_UP;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_SAWTOOTH_DOWN:
        effect->type = FF_PERIODIC;
        effect->u.periodic.waveform = FF_SAW_DOWN;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_SPRING:
        effect->type = FF_SPRING;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_DAMPER:
        effect->type = FF_DAMPER;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_INERTIA:
        effect->type = FF_INERTIA;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_FRICTION:
        effect->type = FF_FRICTION;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_CONSTANT_FORCE:
        effect->type = FF_CONSTANT;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_RAMP:
        effect->type = FF_RAMP;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_CUSTOM_FORCE_DATA:
        effect->type = FF_CUSTOM;
        return STATUS_SUCCESS;
    default:
        return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS lnxev_device_physical_effect_update(struct unix_device *iface, BYTE index,
                                                    struct effect_params *params)
{
    struct lnxev_device *impl = lnxev_impl_from_unix_device(iface);
    struct ff_effect effect = {.id = impl->effect_ids[index]};
    NTSTATUS status;

    TRACE("iface %p, index %u, params %p.\n", iface, index, params);

    if (params->effect_type == PID_USAGE_UNDEFINED) return STATUS_SUCCESS;
    if ((status = set_effect_type_from_usage(&effect, params->effect_type))) return status;

    effect.replay.length = (params->duration == 0xffff ? 0 : params->duration);
    effect.replay.delay = params->start_delay;
    effect.trigger.button = params->trigger_button;
    effect.trigger.interval = params->trigger_repeat_interval;

    /* Linux FF only supports polar direction, and the first direction we get from PID
     * is in polar coordinate space already. */
    effect.direction = params->direction[0] * 0x800 / 1125;

    switch (params->effect_type)
    {
    case PID_USAGE_ET_SINE:
    case PID_USAGE_ET_SQUARE:
    case PID_USAGE_ET_TRIANGLE:
    case PID_USAGE_ET_SAWTOOTH_UP:
    case PID_USAGE_ET_SAWTOOTH_DOWN:
        effect.u.periodic.period = params->periodic.period;
        effect.u.periodic.magnitude = (params->periodic.magnitude * params->gain_percent) / 100;
        effect.u.periodic.offset = params->periodic.offset;
        effect.u.periodic.phase = params->periodic.phase * 0x800 / 1125;
        effect.u.periodic.envelope.attack_length = params->envelope.attack_time;
        effect.u.periodic.envelope.attack_level = params->envelope.attack_level;
        effect.u.periodic.envelope.fade_length = params->envelope.fade_time;
        effect.u.periodic.envelope.fade_level = params->envelope.fade_level;
        break;

    case PID_USAGE_ET_SPRING:
    case PID_USAGE_ET_DAMPER:
    case PID_USAGE_ET_INERTIA:
    case PID_USAGE_ET_FRICTION:
        if (params->condition_count >= 1)
        {
            effect.u.condition[0].right_saturation = params->condition[0].positive_saturation;
            effect.u.condition[0].left_saturation = params->condition[0].negative_saturation;
            effect.u.condition[0].right_coeff = params->condition[0].positive_coefficient;
            effect.u.condition[0].left_coeff = params->condition[0].negative_coefficient;
            effect.u.condition[0].deadband = params->condition[0].dead_band;
            effect.u.condition[0].center = params->condition[0].center_point_offset;
        }
        if (params->condition_count >= 2)
        {
            effect.u.condition[1].right_saturation = params->condition[1].positive_saturation;
            effect.u.condition[1].left_saturation = params->condition[1].negative_saturation;
            effect.u.condition[1].right_coeff = params->condition[1].positive_coefficient;
            effect.u.condition[1].left_coeff = params->condition[1].negative_coefficient;
            effect.u.condition[1].deadband = params->condition[1].dead_band;
            effect.u.condition[1].center = params->condition[1].center_point_offset;
        }
        break;

    case PID_USAGE_ET_CONSTANT_FORCE:
        effect.u.constant.level = (params->constant_force.magnitude * params->gain_percent) / 100;
        effect.u.constant.envelope.attack_length = params->envelope.attack_time;
        effect.u.constant.envelope.attack_level = params->envelope.attack_level;
        effect.u.constant.envelope.fade_length = params->envelope.fade_time;
        effect.u.constant.envelope.fade_level = params->envelope.fade_level;
        break;

    case PID_USAGE_ET_RAMP:
        effect.u.ramp.start_level = (params->ramp_force.ramp_start * params->gain_percent) / 100;
        effect.u.ramp.end_level = (params->ramp_force.ramp_end * params->gain_percent) / 100;
        effect.u.ramp.envelope.attack_length = params->envelope.attack_time;
        effect.u.ramp.envelope.attack_level = params->envelope.attack_level;
        effect.u.ramp.envelope.fade_length = params->envelope.fade_time;
        effect.u.ramp.envelope.fade_level = params->envelope.fade_level;
        break;

    case PID_USAGE_ET_CUSTOM_FORCE_DATA:
        FIXME("not implemented!\n");
        break;
    }

    if (ioctl(impl->base.device_fd, EVIOCSFF, &effect) != -1)
        impl->effect_ids[index] = effect.id;
    else
    {
        WARN("couldn't create effect, EVIOCSFF ioctl failed: %d %s\n", errno, strerror(errno));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static const struct hid_device_vtbl lnxev_device_vtbl =
{
    lnxev_device_destroy,
    lnxev_device_start,
    lnxev_device_stop,
    lnxev_device_haptics_start,
    lnxev_device_haptics_stop,
    lnxev_device_physical_device_control,
    lnxev_device_physical_device_set_gain,
    lnxev_device_physical_effect_control,
    lnxev_device_physical_effect_update,
};
#endif /* HAS_PROPER_INPUT_HEADER */

static void get_device_subsystem_info(struct udev_device *dev, const char *subsystem, const char *devtype,
                                      struct device_desc *desc, int *bus)
{
    struct udev_device *parent = NULL;
    const char *ptr, *next, *tmp;
    char buffer[MAX_PATH];

    if (!(parent = udev_device_get_parent_with_subsystem_devtype(dev, subsystem, devtype))) return;

    if ((next = udev_device_get_sysattr_value(parent, "uevent")))
    {
        while ((ptr = next) && *ptr)
        {
            if ((next = strchr(next, '\n'))) next += 1;
            else next = ptr + strlen(ptr);
            TRACE("%s uevent %s\n", subsystem, debugstr_an(ptr, next - ptr - 1));

            if (!strncmp(ptr, "HID_UNIQ=", 9))
            {
                if (desc->serialnumber[0]) continue;
                if (sscanf(ptr, "HID_UNIQ=%256[^\n]", buffer) == 1)
                    ntdll_umbstowcs(buffer, strlen(buffer) + 1, desc->serialnumber, ARRAY_SIZE(desc->serialnumber));
            }
            if (!strncmp(ptr, "HID_NAME=", 9))
            {
                if (desc->product[0]) continue;
                if (sscanf(ptr, "HID_NAME=%256[^\n]", buffer) == 1)
                    ntdll_umbstowcs(buffer, strlen(buffer) + 1, desc->product, ARRAY_SIZE(desc->product));
            }
            if (!strncmp(ptr, "HID_PHYS=", 9) || !strncmp(ptr, "PHYS=\"", 6))
            {
                if (!(tmp = strstr(ptr, "/input")) || tmp >= next) continue;
                if (desc->input == -1) sscanf(tmp, "/input%d\n", &desc->input);
            }
            if (!strncmp(ptr, "HID_ID=", 7))
            {
                if (*bus || desc->vid || desc->pid) continue;
                sscanf(ptr, "HID_ID=%x:%x:%x\n", bus, &desc->vid, &desc->pid);
            }

            if (!strcmp(subsystem, "input"))
            {
                if (!strncmp(ptr, "PRODUCT=", 8))
                    sscanf(ptr, "PRODUCT=%x/%x/%x/%x\n", bus, &desc->vid, &desc->pid, &desc->version);
            }

            if (!strcmp(subsystem, "usb") && *bus != BUS_BLUETOOTH)
            {
                if (!strncmp(ptr, "PRODUCT=", 8))
                    sscanf(ptr, "PRODUCT=%x/%x/%x\n", &desc->vid, &desc->pid, &desc->version);
            }
        }
    }

    if (!strcmp(subsystem, "usb") && *bus != BUS_BLUETOOTH)
    {
        if ((tmp = udev_device_get_sysattr_value(parent, "manufacturer")))
            ntdll_umbstowcs(tmp, strlen(tmp) + 1, desc->manufacturer, ARRAY_SIZE(desc->manufacturer));

        if ((tmp = udev_device_get_sysattr_value(parent, "product")))
            ntdll_umbstowcs(tmp, strlen(tmp) + 1, desc->product, ARRAY_SIZE(desc->product));

        if ((tmp = udev_device_get_sysattr_value(parent, "serial")))
            ntdll_umbstowcs(tmp, strlen(tmp) + 1, desc->serialnumber, ARRAY_SIZE(desc->serialnumber));
    }
}

static NTSTATUS hidraw_device_create(struct udev_device *dev, int fd, const char *devnode, struct device_desc desc)
{
#ifdef HAVE_LINUX_HIDRAW_H
    static const WCHAR hidraw[] = {'h','i','d','r','a','w',0};
    static const WCHAR zeros[] = {'0','0','0','0',0};
    struct base_device *impl;
    char buffer[MAX_PATH];

    desc.is_hidraw = TRUE;
    if (!desc.product[0] && ioctl(fd, HIDIOCGRAWNAME(sizeof(buffer) - 1), buffer) >= 0)
        ntdll_umbstowcs(buffer, strlen(buffer) + 1, desc.product, ARRAY_SIZE(desc.product));

    if (!desc.manufacturer[0]) memcpy(desc.manufacturer, hidraw, sizeof(hidraw));
    if (!desc.serialnumber[0]) memcpy(desc.serialnumber, zeros, sizeof(zeros));

    if (!(impl = raw_device_create(&hidraw_device_vtbl, sizeof(struct hidraw_device))))
        return STATUS_NO_MEMORY;
    list_add_tail(&device_list, &impl->unix_device.entry);
    impl->read_report = hidraw_device_read_report;
    impl->udev_device = udev_device_ref(dev);
    strcpy(impl->devnode, devnode);
    impl->device_fd = fd;

    TRACE("dev %p, node %s, desc %s.\n", dev, debugstr_a(devnode), debugstr_device_desc(&desc));
    bus_event_queue_device_created(&event_queue, &impl->unix_device, &desc);
    return STATUS_SUCCESS;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

static NTSTATUS lnxev_device_create(struct udev_device *dev, int fd, const char *devnode, struct device_desc desc)
{
#ifdef HAS_PROPER_INPUT_HEADER
    static const WCHAR evdev[] = {'e','v','d','e','v',0};
    static const WCHAR zeros[] = {'0','0','0','0',0};
    int axis_count = 0, button_count = 0;
    struct lnxev_info info = {0};
    struct lnxev_device *impl;

    if (ioctl(fd, EVIOCGID, &info.id) == -1) memset(&info.id, 0, sizeof(info.id));
    if (ioctl(fd, EVIOCGNAME(sizeof(info.name) - 1), info.name) == -1) memset(info.name, 0, sizeof(info.name));
    if (ioctl(fd, EVIOCGUNIQ(sizeof(info.uniq) - 1), info.uniq) == -1) memset(info.uniq, 0, sizeof(info.uniq));
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(info.abs)), info.abs) == -1) memset(info.abs, 0, sizeof(info.abs));
    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(info.rel)), info.rel) == -1) memset(info.rel, 0, sizeof(info.rel));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(info.key)), info.key) == -1) memset(info.key, 0, sizeof(info.key));
    if (ioctl(fd, EVIOCGBIT(EV_FF, sizeof(info.ff)), info.ff) == -1) memset(info.ff, 0, sizeof(info.ff));

    if (!desc.vid) desc.vid = info.id.vendor;
    if (!desc.pid) desc.pid = info.id.product;
    if (!desc.version) desc.version = info.id.version;
    if (!desc.manufacturer[0]) memcpy(desc.manufacturer, evdev, sizeof(evdev));
    if (!desc.product[0]) ntdll_umbstowcs(info.name, strlen(info.name) + 1, desc.product, ARRAY_SIZE(desc.product));
    if (!desc.serialnumber[0]) ntdll_umbstowcs(info.uniq, strlen(info.uniq) + 1, desc.serialnumber, ARRAY_SIZE(desc.serialnumber));
    if (!desc.serialnumber[0]) memcpy(desc.serialnumber, zeros, sizeof(zeros));

    if (!(impl = hid_device_create(&lnxev_device_vtbl, sizeof(struct lnxev_device))))
        return STATUS_NO_MEMORY;
    list_add_tail(&device_list, &impl->base.unix_device.entry);
    impl->base.read_report = lnxev_device_read_report;
    impl->base.udev_device = udev_device_ref(dev);
    strcpy(impl->base.devnode, devnode);
    impl->base.device_fd = fd;

    for (int i = 0; i < ABS_CNT; i++)
    {
        USAGE_AND_PAGE usage = absolute_usages[i];
        struct input_absinfo abs;
        if (!usage.UsagePage || !usage.Usage) continue;
        if (!test_bit(info.abs, i)) continue;
        ioctl(fd, EVIOCGABS(i), &abs);
        impl->abs_map[i] = ++axis_count;
        impl->abs_min[i] = abs.minimum;
        impl->abs_max[i] = abs.maximum;
    }

    for (int i = 0, count = 0; i < REL_CNT; i++)
    {
        USAGE_AND_PAGE usage = relative_usages[i];
        if (!usage.UsagePage || !usage.Usage) continue;
        if (!test_bit(info.rel, i)) continue;
        impl->rel_map[i] = ++count;
    }

    for (int i = ABS_HAT0X; i <= ABS_HAT3X; i += 2)
    {
        if (!test_bit(info.abs, i)) continue;
        impl->hat_map[i - ABS_HAT0X] = ++impl->hat_count;
        impl->hat_map[i - ABS_HAT0X + 1] = impl->hat_count;
    }

    for (int i = BTN_MISC; i < KEY_MAX; i++)
    {
        if (!test_bit(info.key, i)) continue;
        impl->button_map[i] = ++impl->button_count;
    }

    if (is_xbox_gamepad(desc.vid, desc.pid)) desc.is_gamepad = TRUE;
    else if (axis_count == 6 && button_count >= (impl->hat_count ? 10 : 14)) desc.is_gamepad = TRUE;

    if ((impl->is_gamepad = desc.is_gamepad))
    {
        static const int gamepad_axes[] = {1, 2, 5, 3, 4, 6};
        static const UINT gamepad_buttons[] =
        {
            BTN_A,
            BTN_B,
            BTN_X,
            BTN_Y,
            BTN_TL,
            BTN_TR,
            BTN_SELECT,
            BTN_START,
            BTN_THUMBL,
            BTN_THUMBR,
            BTN_MODE,
            BTN_C,
            BTN_Z,
            BTN_TL2,
            BTN_TR2,
        };

        memset(impl->abs_map, 0, sizeof(impl->abs_map));
        memset(impl->button_map, 0, sizeof(impl->button_map));
        impl->button_count = 0;

        for (int i = 0, count = 0; i < ABS_CNT; i++)
        {
            USAGE_AND_PAGE usage = absolute_usages[i];
            if (!usage.UsagePage || !usage.Usage) continue;
            if (!test_bit(info.abs, i)) continue;
            impl->abs_map[i] = gamepad_axes[count++];
        }

        for (int i = 0; i < ARRAY_SIZE(gamepad_buttons); i++)
        {
            int button = gamepad_buttons[i];
            if (!test_bit(info.key, button)) continue;
            if (impl->button_count > (impl->hat_count ? 10 : 14)) break;
            impl->button_map[button] = ++impl->button_count;
            if (impl->hat_count && impl->button_count == 11) impl->button_map[button] = 17;
        }

        for (int i = BTN_MISC; i < KEY_MAX; i++)
        {
            if (i >= BTN_GAMEPAD && i < BTN_DIGI) continue;
            if (impl->button_count > (impl->hat_count ? 10 : 14)) break;
            if (!test_bit(info.key, i)) continue;
            impl->button_map[i] = ++impl->button_count;
            if (impl->hat_count && impl->button_count == 11) impl->button_map[i] = 17;
        }
    }

    if (build_report_descriptor(&impl->base.unix_device, impl->base.udev_device, &info))
    {
        list_remove(&impl->base.unix_device.entry);
        impl->base.unix_device.vtbl->destroy(&impl->base.unix_device);
    }
    else
    {
        TRACE("dev %p, node %s, desc %s.\n", dev, debugstr_a(devnode), debugstr_device_desc(&desc));
        bus_event_queue_device_created(&event_queue, &impl->base.unix_device, &desc);
    }

    return STATUS_SUCCESS;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

static void udev_add_device(struct udev_device *dev, int fd)
{
    struct device_desc desc = { .input = -1 };
    const char *subsystem, *devnode;
    int bus = 0;

    if (!(devnode = udev_device_get_devnode(dev)))
    {
        if (fd >= 0) close(fd);
        return;
    }

    if (fd < 0 && (fd = open(devnode, O_RDWR)) == -1)
    {
        WARN("Unable to open udev device %s: %s\n", debugstr_a(devnode), strerror(errno));
        return;
    }

    TRACE("udev %s syspath %s\n", debugstr_a(devnode), udev_device_get_syspath(dev));

    get_device_subsystem_info(dev, "hid", NULL, &desc, &bus);
    get_device_subsystem_info(dev, "input", NULL, &desc, &bus);
    get_device_subsystem_info(dev, "usb", "usb_device", &desc, &bus);
    if (bus == BUS_BLUETOOTH) desc.is_bluetooth = TRUE;

    if (!(subsystem = udev_device_get_subsystem(dev)))
    {
        WARN("udev_device_get_subsystem failed for %s.\n", debugstr_a(devnode));
        close(fd);
        return;
    }

    if ((desc.is_hidraw = !strcmp(subsystem, "hidraw")) && !hidraw_device_create(dev, fd, devnode, desc)) return;
    if (!strcmp(subsystem, "input") && !lnxev_device_create(dev, fd, devnode, desc)) return;
    close(fd);
}

#ifdef HAVE_SYS_INOTIFY_H
static int dev_watch = -1;
#ifdef HAS_PROPER_INPUT_HEADER
static int devinput_watch = -1;
#endif

static void maybe_add_devnode(const char *base, const char *dir, const char *subsystem)
{
    char *syspath = NULL, devnode[MAX_PATH], syslink[MAX_PATH];
    struct udev_device *dev = NULL;
    const char *udev_devnode;
    int fd = -1;

    TRACE("Considering %s/%s...\n", dir, base);

    snprintf(devnode, sizeof(devnode), "%s/%s", dir, base);
    if ((fd = open(devnode, O_RDWR)) < 0)
    {
        /* When using inotify monitoring, quietly ignore device nodes that we cannot read,
         * without emitting a warning.
         *
         * We can expect that a significant number of device nodes will be permanently
         * unreadable, such as the device nodes for keyboards and mice. We can also expect
         * that joysticks and game controllers will be temporarily unreadable until udevd
         * chmods them; we'll get another chance to open them when their attributes change. */
        TRACE("Unable to open %s, ignoring: %s\n", debugstr_a(devnode), strerror(errno));
        return;
    }

    snprintf(syslink, sizeof(syslink), "/sys/class/%s/%s", subsystem, base);
    TRACE("Resolving real path to %s\n", debugstr_a(syslink));

    if (!(syspath = realpath(syslink, NULL)))
    {
        WARN("Unable to resolve path \"%s\" for \"%s/%s\": %s\n",
             debugstr_a(syslink), dir, base, strerror(errno));
        goto error;
    }

    TRACE("Creating udev_device for %s\n", syspath);
    if (!(dev = udev_device_new_from_syspath(udev_context, syspath)))
    {
        WARN("failed to create udev device from syspath %s\n", syspath);
        goto error;
    }

    if (!(udev_devnode = udev_device_get_devnode(dev)) || strcmp(devnode, udev_devnode) != 0)
    {
        WARN("Tried to get udev device for \"%s\" but device node of \"%s\" -> \"%s\" is \"%s\"\n",
             debugstr_a(devnode), debugstr_a(syslink), debugstr_a(syspath), debugstr_a(udev_devnode));
        goto error;
    }

    TRACE("Adding device for %s\n", syspath);
    udev_add_device(dev, fd);
    udev_device_unref(dev);
    return;

error:
    if (dev) udev_device_unref(dev);
    free(syspath);
    close(fd);
}

static void build_initial_deviceset_direct(void)
{
    struct dirent *dent;
    int n, len;
    DIR *dir;

    if (!options->disable_hidraw)
    {
        TRACE("Initial enumeration of /dev/hidraw*\n");
        if (!(dir = opendir("/dev"))) WARN("Unable to open /dev: %s\n", strerror(errno));
        else
        {
            for (dent = readdir(dir); dent; dent = readdir(dir))
            {
                if (sscanf(dent->d_name, "hidraw%u%n", &n, &len) != 1 || len != strlen(dent->d_name))
                    WARN("ignoring %s, name doesn't match hidraw%%u\n", debugstr_a(dent->d_name));
                else
                    maybe_add_devnode(dent->d_name, "/dev", "hidraw");
            }
            closedir(dir);
        }
    }
#ifdef HAS_PROPER_INPUT_HEADER
    if (!options->disable_input)
    {
        TRACE("Initial enumeration of /dev/input/event*\n");
        if (!(dir = opendir("/dev/input"))) WARN("Unable to open /dev/input: %s\n", strerror(errno));
        else
        {
            for (dent = readdir(dir); dent; dent = readdir(dir))
            {
                if (sscanf(dent->d_name, "event%u%n", &n, &len) != 1 || len != strlen(dent->d_name))
                    WARN("ignoring %s, name doesn't match event%%u\n", debugstr_a(dent->d_name));
                else
                    maybe_add_devnode(dent->d_name, "/dev/input", "input");
            }
            closedir(dir);
        }
    }
#endif
}

static int create_inotify(void)
{
    int systems = 0, fd, flags = IN_CREATE | IN_DELETE | IN_MOVE | IN_ATTRIB;

    if ((fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC)) < 0)
    {
        WARN("Unable to get inotify fd\n");
        return fd;
    }

    if (!options->disable_hidraw)
    {
        /* We need to watch for attribute changes in addition to
         * creation, because when a device is first created, it has
         * permissions that we can't read. When udev chmods it to
         * something that we maybe *can* read, we'll get an
         * IN_ATTRIB event to tell us. */
        dev_watch = inotify_add_watch(fd, "/dev", flags);
        if (dev_watch < 0) WARN("Unable to initialize inotify for /dev: %s\n", strerror(errno));
        else systems++;
    }
#ifdef HAS_PROPER_INPUT_HEADER
    if (!options->disable_input)
    {
        devinput_watch = inotify_add_watch(fd, "/dev/input", flags);
        if (devinput_watch < 0) WARN("Unable to initialize inotify for /dev/input: %s\n", strerror(errno));
        else systems++;
    }
#endif
    if (systems == 0)
    {
        WARN("No subsystems added to monitor\n");
        close(fd);
        return -1;
    }

    return fd;
}

static void maybe_remove_devnode(const char *base, const char *dir)
{
    struct base_device *impl;
    char devnode[MAX_PATH];

    snprintf(devnode, sizeof(devnode), "%s/%s", dir, base);
    impl = find_device_from_devnode(devnode);
    if (impl) bus_event_queue_device_removed(&event_queue, &impl->unix_device);
    else WARN("failed to find device for path %s\n", devnode);
}

static void process_inotify_event(int fd)
{
    union
    {
        struct inotify_event event;
        char storage[4096];
        char enough_for_inotify[sizeof(struct inotify_event) + NAME_MAX + 1];
    } buf;
    ssize_t bytes;
    int n, len;

    if ((bytes = read(fd, &buf, sizeof(buf))) < 0)
        WARN("read failed: %u %s\n", errno, strerror(errno));
    else while (bytes > 0)
    {
        if (buf.event.len > 0)
        {
            if (buf.event.wd == dev_watch)
            {
                if (sscanf(buf.event.name, "hidraw%u%n", &n, &len) != 1 || len != strlen(buf.event.name))
                    WARN("ignoring %s, name doesn't match hidraw%%u\n", debugstr_a(buf.event.name));
                else if (buf.event.mask & (IN_DELETE | IN_MOVED_FROM))
                    maybe_remove_devnode(buf.event.name, "/dev");
                else if (buf.event.mask & (IN_CREATE | IN_MOVED_TO))
                    maybe_add_devnode(buf.event.name, "/dev", "hidraw");
                else if (buf.event.mask & IN_ATTRIB)
                {
                    maybe_remove_devnode(buf.event.name, "/dev");
                    maybe_add_devnode(buf.event.name, "/dev", "hidraw");
                }
            }
#ifdef HAS_PROPER_INPUT_HEADER
            else if (buf.event.wd == devinput_watch)
            {
                if (sscanf(buf.event.name, "event%u%n", &n, &len) != 1 || len != strlen(buf.event.name))
                    WARN("ignoring %s, name doesn't match event%%u\n", debugstr_a(buf.event.name));
                else if (buf.event.mask & (IN_DELETE | IN_MOVED_FROM))
                    maybe_remove_devnode(buf.event.name, "/dev/input");
                else if (buf.event.mask & (IN_CREATE | IN_MOVED_TO))
                    maybe_add_devnode(buf.event.name, "/dev/input", "input");
                else if (buf.event.mask & IN_ATTRIB)
                {
                    maybe_remove_devnode(buf.event.name, "/dev/input");
                    maybe_add_devnode(buf.event.name, "/dev/input", "input");
                }
            }
#endif
        }

        len = sizeof(struct inotify_event) + buf.event.len;
        bytes -= len;
        if (bytes > 0) memmove(&buf.storage[0], &buf.storage[len], bytes);
    }
}
#endif /* HAVE_SYS_INOTIFY_H */

static void build_initial_deviceset_udevd(void)
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;

    enumerate = udev_enumerate_new(udev_context);
    if (!enumerate)
    {
        WARN("Unable to create udev enumeration object\n");
        return;
    }

    if (!options->disable_hidraw)
        if (udev_enumerate_add_match_subsystem(enumerate, "hidraw") < 0)
            WARN("Failed to add subsystem 'hidraw' to enumeration\n");
#ifdef HAS_PROPER_INPUT_HEADER
    if (!options->disable_input)
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
            udev_add_device(dev, -1);
            udev_device_unref(dev);
        }
    }

    udev_enumerate_unref(enumerate);
}

static struct udev_monitor *create_monitor(int *fd)
{
    struct udev_monitor *monitor;
    int systems = 0;

    monitor = udev_monitor_new_from_netlink(udev_context, "udev");
    if (!monitor)
    {
        WARN("Unable to get udev monitor object\n");
        return NULL;
    }

    if (!options->disable_hidraw)
    {
        if (udev_monitor_filter_add_match_subsystem_devtype(monitor, "hidraw", NULL) < 0)
            WARN("Failed to add 'hidraw' subsystem to monitor\n");
        else
            systems++;
    }
#ifdef HAS_PROPER_INPUT_HEADER
    if (!options->disable_input)
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

    if ((*fd = udev_monitor_get_fd(monitor)) >= 0)
        return monitor;

error:
    WARN("Failed to start monitoring\n");
    udev_monitor_unref(monitor);
    return NULL;
}

static void process_monitor_event(struct udev_monitor *monitor)
{
    struct base_device *impl;
    struct udev_device *dev;
    const char *action, *devnode, *syspath;

    dev = udev_monitor_receive_device(monitor);
    if (!dev)
    {
        ERR("Failed to get device that has changed\n");
        return;
    }

    action = udev_device_get_action(dev);
    syspath = udev_device_get_syspath(dev);
    devnode = udev_device_get_devnode(dev);
    TRACE("Received action %s for udev device %s (%p) devnode %s\n",
          debugstr_a(action), debugstr_a(syspath), dev, debugstr_a(devnode));

    if (!syspath)
        ERR("udev device %p does not have syspath!\n", dev);
    else if (!action)
        ERR("event for udev device %s does not have any action!\n", syspath);
    else if (!devnode)
    {
        /* Pretty normal case, not all devices have associated
         * devnodes. For example root input devices do not, but
         * related/child mouse and event devices do.
         */
        TRACE("udev device %s does not have devnode, ignoring\n", syspath);
    }
    else if (strcmp(action, "remove"))
        udev_add_device(dev, -1);
    else
    {
        impl = find_device_from_devnode(devnode);
        if (impl) bus_event_queue_device_removed(&event_queue, &impl->unix_device);
        else WARN("failed to find device for udev device %p\n", dev);
    }

    udev_device_unref(dev);
}

NTSTATUS udev_bus_init(void *args)
{
    int monitor_fd = -1;
    BOOL disable_udevd;

    TRACE("args %p\n", args);

    options = (struct bus_options *)args;
    disable_udevd = options->disable_udevd;

    if (pipe(deviceloop_control) != 0)
    {
        ERR("UDEV control pipe creation failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!(udev_context = udev_new()))
    {
        ERR("UDEV object creation failed\n");
        goto error;
    }

#ifdef HAVE_SYS_INOTIFY_H
    if (disable_udevd) monitor_fd = create_inotify();
    if (monitor_fd < 0) disable_udevd = FALSE;
#else
    if (disable_udevd) ERR("inotify support not compiled in!\n");
    disable_udevd = FALSE;
#endif

    if (monitor_fd < 0 && !(udev_monitor = create_monitor(&monitor_fd)))
    {
        ERR("UDEV monitor creation failed\n");
        goto error;
    }

    if (monitor_fd < 0) goto error;

    poll_fds[0].fd = monitor_fd;
    poll_fds[0].events = POLLIN;
    poll_fds[0].revents = 0;
    poll_fds[1].fd = deviceloop_control[0];
    poll_fds[1].events = POLLIN;
    poll_fds[1].revents = 0;
    poll_count = 2;

    if (!disable_udevd) build_initial_deviceset_udevd();
#ifdef HAVE_SYS_INOTIFY_H
    else build_initial_deviceset_direct();
#endif

    return STATUS_SUCCESS;

error:
    if (udev_monitor) udev_monitor_unref(udev_monitor);
    if (udev_context) udev_unref(udev_context);
    udev_context = NULL;
    close(deviceloop_control[0]);
    close(deviceloop_control[1]);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS udev_bus_wait(void *args)
{
    struct bus_event *result = args;
    struct pollfd pfd[MAX_DEVICES];
    struct base_device *impl;
    char ctrl = 0;
    int i, count;

    /* cleanup previously returned event */
    bus_event_cleanup(result);

    while (ctrl != 'q')
    {
        if (bus_event_queue_pop(&event_queue, result)) return STATUS_PENDING;

        pthread_mutex_lock(&udev_cs);
        while (close_count--) close(close_fds[close_count]);
        memcpy(pfd, poll_fds, poll_count * sizeof(*pfd));
        count = poll_count;
        close_count = 0;
        pthread_mutex_unlock(&udev_cs);

        while (poll(pfd, count, -1) <= 0) {}

        pthread_mutex_lock(&udev_cs);
        if (pfd[0].revents)
        {
            if (udev_monitor) process_monitor_event(udev_monitor);
#ifdef HAVE_SYS_INOTIFY_H
            else process_inotify_event(pfd[0].fd);
#endif
        }
        if (pfd[1].revents) read(deviceloop_control[0], &ctrl, 1);
        for (i = 2; i < count; ++i)
        {
            if (!pfd[i].revents) continue;
            impl = find_device_from_fd(pfd[i].fd);
            if (impl) impl->read_report(&impl->unix_device);
        }
        pthread_mutex_unlock(&udev_cs);
    }

    TRACE("UDEV main loop exiting\n");
    bus_event_queue_destroy(&event_queue);
    if (udev_monitor) udev_monitor_unref(udev_monitor);
    udev_unref(udev_context);
    udev_context = NULL;
    close(deviceloop_control[0]);
    close(deviceloop_control[1]);
    return STATUS_SUCCESS;
}

NTSTATUS udev_bus_stop(void *args)
{
    if (!udev_context) return STATUS_SUCCESS;
    write(deviceloop_control[1], "q", 1);
    return STATUS_SUCCESS;
}

#else

NTSTATUS udev_bus_init(void *args)
{
    WARN("UDEV support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS udev_bus_wait(void *args)
{
    WARN("UDEV support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS udev_bus_stop(void *args)
{
    WARN("UDEV support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* HAVE_UDEV */
