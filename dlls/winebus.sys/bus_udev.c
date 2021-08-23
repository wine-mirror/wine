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
#include <stdio.h>
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
# ifndef SYN_DROPPED
#  define SYN_DROPPED 3
# endif
#endif

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
#include "wine/heap.h"
#include "wine/unicode.h"

#ifdef HAS_PROPER_INPUT_HEADER
# include "hidusage.h"
#endif

#ifdef WORDS_BIGENDIAN
#define LE_WORD(x) RtlUshortByteSwap(x)
#define LE_DWORD(x) RtlUlongByteSwap(x)
#else
#define LE_WORD(x) (x)
#define LE_DWORD(x) (x)
#endif

#include "bus.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef HAVE_UDEV

WINE_DECLARE_DEBUG_CHANNEL(hid_report);

static struct udev *udev_context = NULL;
static struct udev_monitor *udev_monitor;
static int deviceloop_control[2];
static int udev_monitor_fd;

static const WCHAR hidraw_busidW[] = {'H','I','D','R','A','W',0};
static const WCHAR lnxev_busidW[] = {'L','N','X','E','V',0};
static struct udev_bus_options options;

struct platform_private
{
    struct unix_device unix_device;

    struct udev_device *udev_device;
    int device_fd;

    HANDLE report_thread;
    int control_pipe[2];
};

static inline struct platform_private *impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(iface, struct platform_private, unix_device);
}

static inline struct platform_private *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return impl_from_unix_device(get_unix_device(device));
}

#ifdef HAS_PROPER_INPUT_HEADER

static const BYTE ABS_TO_HID_MAP[][2] = {
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_X},              /*ABS_X*/
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_Y},              /*ABS_Y*/
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_Z},              /*ABS_Z*/
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_RX},             /*ABS_RX*/
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_RY},             /*ABS_RY*/
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_RZ},             /*ABS_RZ*/
    {HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_THROTTLE}, /*ABS_THROTTLE*/
    {HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_RUDDER},   /*ABS_RUDDER*/
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_WHEEL},          /*ABS_WHEEL*/
    {HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_ACCELERATOR}, /*ABS_GAS*/
    {HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_BRAKE},    /*ABS_BRAKE*/
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},                                                      /*ABS_HAT0X*/
    {0,0},                                                      /*ABS_HAT0Y*/
    {0,0},                                                      /*ABS_HAT1X*/
    {0,0},                                                      /*ABS_HAT1Y*/
    {0,0},                                                      /*ABS_HAT2X*/
    {0,0},                                                      /*ABS_HAT2Y*/
    {0,0},                                                      /*ABS_HAT3X*/
    {0,0},                                                      /*ABS_HAT3Y*/
    {HID_USAGE_PAGE_DIGITIZER, HID_USAGE_DIGITIZER_TIP_PRESSURE}, /*ABS_PRESSURE*/
    {0, 0},                                                     /*ABS_DISTANCE*/
    {HID_USAGE_PAGE_DIGITIZER, HID_USAGE_DIGITIZER_X_TILT},     /*ABS_TILT_X*/
    {HID_USAGE_PAGE_DIGITIZER, HID_USAGE_DIGITIZER_Y_TILT},     /*ABS_TILT_Y*/
    {0, 0},                                                     /*ABS_TOOL_WIDTH*/
    {0, 0},
    {0, 0},
    {0, 0},
    {HID_USAGE_PAGE_CONSUMER, HID_USAGE_CONSUMER_VOLUME}        /*ABS_VOLUME*/
};
#define HID_ABS_MAX (ABS_VOLUME+1)
C_ASSERT(ARRAY_SIZE(ABS_TO_HID_MAP) == HID_ABS_MAX);
#define TOP_ABS_PAGE (HID_USAGE_PAGE_DIGITIZER+1)

static const BYTE REL_TO_HID_MAP[][2] = {
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_X},     /* REL_X */
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_Y},     /* REL_Y */
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_Z},     /* REL_Z */
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_RX},    /* REL_RX */
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_RY},    /* REL_RY */
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_RZ},    /* REL_RZ */
    {0, 0},                                            /* REL_HWHEEL */
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_DIAL},  /* REL_DIAL */
    {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_WHEEL}, /* REL_WHEEL */
    {0, 0}                                             /* REL_MISC */
};

#define HID_REL_MAX (REL_MISC+1)
#define TOP_REL_PAGE (HID_USAGE_PAGE_CONSUMER+1)

struct wine_input_private {
    struct platform_private base;

    int buffer_length;
    BYTE *last_report_buffer;
    BYTE *current_report_buffer;
    enum { FIRST, NORMAL, DROPPED } report_state;

    struct hid_descriptor desc;

    int button_start;
    BYTE button_map[KEY_MAX];
    BYTE rel_map[HID_REL_MAX];
    BYTE hat_map[8];
    int hat_values[8];
    int abs_map[HID_ABS_MAX];
};

#define test_bit(arr,bit) (((BYTE*)(arr))[(bit)>>3]&(1<<((bit)&7)))

static const BYTE* what_am_I(struct udev_device *dev)
{
    static const BYTE Unknown[2]     = {HID_USAGE_PAGE_GENERIC, 0};
    static const BYTE Mouse[2]       = {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE};
    static const BYTE Keyboard[2]    = {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD};
    static const BYTE Gamepad[2]     = {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD};
    static const BYTE Keypad[2]      = {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYPAD};
    static const BYTE Tablet[2]      = {HID_USAGE_PAGE_DIGITIZER, HID_USAGE_DIGITIZER_PEN};
    static const BYTE Touchscreen[2] = {HID_USAGE_PAGE_DIGITIZER, HID_USAGE_DIGITIZER_TOUCH_SCREEN};
    static const BYTE Touchpad[2]    = {HID_USAGE_PAGE_DIGITIZER, HID_USAGE_DIGITIZER_TOUCH_PAD};

    struct udev_device *parent = dev;

    /* Look to the parents until we get a clue */
    while (parent)
    {
        if (udev_device_get_property_value(parent, "ID_INPUT_MOUSE"))
            return Mouse;
        else if (udev_device_get_property_value(parent, "ID_INPUT_KEYBOARD"))
            return Keyboard;
        else if (udev_device_get_property_value(parent, "ID_INPUT_JOYSTICK"))
            return Gamepad;
        else if (udev_device_get_property_value(parent, "ID_INPUT_KEY"))
            return Keypad;
        else if (udev_device_get_property_value(parent, "ID_INPUT_TOUCHPAD"))
            return Touchpad;
        else if (udev_device_get_property_value(parent, "ID_INPUT_TOUCHSCREEN"))
            return Touchscreen;
        else if (udev_device_get_property_value(parent, "ID_INPUT_TABLET"))
            return Tablet;

        parent = udev_device_get_parent_with_subsystem_devtype(parent, "input", NULL);
    }
    return Unknown;
}

static void set_button_value(int index, int value, BYTE* buffer)
{
    int bindex = index / 8;
    int b = index % 8;
    BYTE mask;

    mask = 1<<b;
    if (value)
        buffer[bindex] = buffer[bindex] | mask;
    else
    {
        mask = ~mask;
        buffer[bindex] = buffer[bindex] & mask;
    }
}

static void set_abs_axis_value(struct wine_input_private *ext, int code, int value)
{
    int index;
    /* check for hatswitches */
    if (code <= ABS_HAT3Y && code >= ABS_HAT0X)
    {
        index = code - ABS_HAT0X;
        ext->hat_values[index] = value;
        if ((code - ABS_HAT0X) % 2)
            index--;
        /* 8 1 2
         * 7 0 3
         * 6 5 4 */
        if (ext->hat_values[index] == 0)
        {
            if (ext->hat_values[index+1] == 0)
                value = 0;
            else if (ext->hat_values[index+1] < 0)
                value = 1;
            else
                value = 5;
        }
        else if (ext->hat_values[index] > 0)
        {
            if (ext->hat_values[index+1] == 0)
                value = 3;
            else if (ext->hat_values[index+1] < 0)
                value = 2;
            else
                value = 4;
        }
        else
        {
            if (ext->hat_values[index+1] == 0)
                value = 7;
            else if (ext->hat_values[index+1] < 0)
                value = 8;
            else
                value = 6;
        }
        ext->current_report_buffer[ext->hat_map[index]] = value;
    }
    else if (code < HID_ABS_MAX && ABS_TO_HID_MAP[code][0] != 0)
    {
        index = ext->abs_map[code];
        *((DWORD*)&ext->current_report_buffer[index]) = LE_DWORD(value);
    }
}

static void set_rel_axis_value(struct wine_input_private *ext, int code, int value)
{
    int index;
    if (code < HID_REL_MAX && REL_TO_HID_MAP[code][0] != 0)
    {
        index = ext->rel_map[code];
        if (value > 127) value = 127;
        if (value < -127) value = -127;
        ext->current_report_buffer[index] = value;
    }
}

static INT count_buttons(int device_fd, BYTE *map)
{
    int i;
    int button_count = 0;
    BYTE keybits[(KEY_MAX+7)/8];

    if (ioctl(device_fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) == -1)
    {
        WARN("ioctl(EVIOCGBIT, EV_KEY) failed: %d %s\n", errno, strerror(errno));
        return FALSE;
    }

    for (i = BTN_MISC; i < KEY_MAX; i++)
    {
        if (test_bit(keybits, i))
        {
            if (map) map[i] = button_count;
            button_count++;
        }
    }
    return button_count;
}

static INT count_abs_axis(int device_fd)
{
    BYTE absbits[(ABS_MAX+7)/8];
    int abs_count = 0;
    int i;

    if (ioctl(device_fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) == -1)
    {
        WARN("ioctl(EVIOCGBIT, EV_ABS) failed: %d %s\n", errno, strerror(errno));
        return 0;
    }

    for (i = 0; i < HID_ABS_MAX; i++)
        if (test_bit(absbits, i) &&
            (ABS_TO_HID_MAP[i][1] >= HID_USAGE_GENERIC_X &&
             ABS_TO_HID_MAP[i][1] <= HID_USAGE_GENERIC_WHEEL))
                abs_count++;
    return abs_count;
}

static NTSTATUS build_report_descriptor(struct wine_input_private *ext, struct udev_device *dev)
{
    struct input_absinfo abs_info[HID_ABS_MAX];
    BYTE absbits[(ABS_MAX+7)/8];
    BYTE relbits[(REL_MAX+7)/8];
    USAGE_AND_PAGE usage;
    INT i;
    INT report_size;
    INT button_count, abs_count, rel_count, hat_count;
    const BYTE *device_usage = what_am_I(dev);

    if (ioctl(ext->base.device_fd, EVIOCGBIT(EV_REL, sizeof(relbits)), relbits) == -1)
    {
        WARN("ioctl(EVIOCGBIT, EV_REL) failed: %d %s\n", errno, strerror(errno));
        memset(relbits, 0, sizeof(relbits));
    }
    if (ioctl(ext->base.device_fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) == -1)
    {
        WARN("ioctl(EVIOCGBIT, EV_ABS) failed: %d %s\n", errno, strerror(errno));
        memset(absbits, 0, sizeof(absbits));
    }

    report_size = 0;

    if (!hid_descriptor_begin(&ext->desc, device_usage[0], device_usage[1]))
        return STATUS_NO_MEMORY;

    abs_count = 0;
    for (i = 0; i < HID_ABS_MAX; i++)
    {
        if (!test_bit(absbits, i)) continue;
        ioctl(ext->base.device_fd, EVIOCGABS(i), abs_info + i);

        if (!(usage.UsagePage = ABS_TO_HID_MAP[i][0])) continue;
        if (!(usage.Usage = ABS_TO_HID_MAP[i][1])) continue;

        if (!hid_descriptor_add_axes(&ext->desc, 1, usage.UsagePage, &usage.Usage, FALSE, 32,
                                     LE_DWORD(abs_info[i].minimum), LE_DWORD(abs_info[i].maximum)))
            return STATUS_NO_MEMORY;

        ext->abs_map[i] = report_size;
        report_size += 4;
        abs_count++;
    }

    rel_count = 0;
    for (i = 0; i < HID_REL_MAX; i++)
    {
        if (!test_bit(relbits, i)) continue;
        if (!(usage.UsagePage = REL_TO_HID_MAP[i][0])) continue;
        if (!(usage.Usage = REL_TO_HID_MAP[i][1])) continue;

        if (!hid_descriptor_add_axes(&ext->desc, 1, usage.UsagePage, &usage.Usage, TRUE, 8,
                                     0x81, 0x7f))
            return STATUS_NO_MEMORY;

        ext->rel_map[i] = report_size;
        report_size++;
        rel_count++;
    }

    /* For now lump all buttons just into incremental usages, Ignore Keys */
    ext->button_start = report_size;
    button_count = count_buttons(ext->base.device_fd, ext->button_map);
    if (button_count)
    {
        if (!hid_descriptor_add_buttons(&ext->desc, HID_USAGE_PAGE_BUTTON, 1, button_count))
            return STATUS_NO_MEMORY;

        if (button_count % 8)
        {
            BYTE padding = 8 - (button_count % 8);
            if (!hid_descriptor_add_padding(&ext->desc, padding))
                return STATUS_NO_MEMORY;
        }

        report_size += (button_count + 7) / 8;
    }

    hat_count = 0;
    for (i = ABS_HAT0X; i <=ABS_HAT3X; i+=2)
    {
        if (!test_bit(absbits, i)) continue;
        ext->hat_map[i - ABS_HAT0X] = report_size;
        ext->hat_values[i - ABS_HAT0X] = 0;
        ext->hat_values[i - ABS_HAT0X + 1] = 0;
        report_size++;
        hat_count++;
    }

    if (hat_count)
    {
        if (!hid_descriptor_add_hatswitch(&ext->desc, hat_count))
            return STATUS_NO_MEMORY;
    }

    if (!hid_descriptor_end(&ext->desc))
        return STATUS_NO_MEMORY;

    TRACE("Report will be %i bytes\n", report_size);

    ext->buffer_length = report_size;
    if (!(ext->current_report_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, report_size)))
        goto failed;
    if (!(ext->last_report_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, report_size)))
        goto failed;
    ext->report_state = FIRST;

    /* Initialize axis in the report */
    for (i = 0; i < HID_ABS_MAX; i++)
        if (test_bit(absbits, i))
            set_abs_axis_value(ext, i, abs_info[i].value);

    return STATUS_SUCCESS;

failed:
    HeapFree(GetProcessHeap(), 0, ext->current_report_buffer);
    HeapFree(GetProcessHeap(), 0, ext->last_report_buffer);
    hid_descriptor_free(&ext->desc);
    return STATUS_NO_MEMORY;
}

static BOOL set_report_from_event(struct wine_input_private *ext, struct input_event *ie)
{
    switch(ie->type)
    {
#ifdef EV_SYN
        case EV_SYN:
            switch (ie->code)
            {
                case SYN_REPORT:
                    if (ext->report_state == NORMAL)
                    {
                        memcpy(ext->last_report_buffer, ext->current_report_buffer, ext->buffer_length);
                        return TRUE;
                    }
                    else
                    {
                        if (ext->report_state == DROPPED)
                            memcpy(ext->current_report_buffer, ext->last_report_buffer, ext->buffer_length);
                        ext->report_state = NORMAL;
                    }
                    break;
                case SYN_DROPPED:
                    TRACE_(hid_report)("received SY_DROPPED\n");
                    ext->report_state = DROPPED;
            }
            return FALSE;
#endif
#ifdef EV_MSC
        case EV_MSC:
            return FALSE;
#endif
        case EV_KEY:
            set_button_value(ext->button_start * 8 + ext->button_map[ie->code], ie->value, ext->current_report_buffer);
            return FALSE;
        case EV_ABS:
            set_abs_axis_value(ext, ie->code, ie->value);
            return FALSE;
        case EV_REL:
            set_rel_axis_value(ext, ie->code, ie->value);
            return FALSE;
        default:
            ERR("TODO: Process Report (%i, %i)\n",ie->type, ie->code);
            return FALSE;
    }
}
#endif

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

static void hidraw_free_device(DEVICE_OBJECT *device)
{
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);

    if (private->report_thread)
    {
        write(private->control_pipe[1], "q", 1);
        WaitForSingleObject(private->report_thread, INFINITE);
        close(private->control_pipe[0]);
        close(private->control_pipe[1]);
        CloseHandle(private->report_thread);
    }

    close(private->device_fd);
    udev_device_unref(private->udev_device);

    HeapFree(GetProcessHeap(), 0, private);
}

static int compare_platform_device(DEVICE_OBJECT *device, void *platform_dev)
{
    struct udev_device *dev1 = impl_from_DEVICE_OBJECT(device)->udev_device;
    struct udev_device *dev2 = platform_dev;
    return strcmp(udev_device_get_syspath(dev1), udev_device_get_syspath(dev2));
}

static DWORD CALLBACK device_report_thread(void *args);

static NTSTATUS hidraw_start_device(DEVICE_OBJECT *device)
{
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);

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

    return STATUS_SUCCESS;
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
            TRACE_(hid_report)("Read failed. Likely an unplugged device %d %s\n", errno, strerror(errno));
        else if (size == 0)
            TRACE_(hid_report)("Failed to read report\n");
        else
            process_hid_report(device, report_buffer, size);
    }
    return 0;
}

static void hidraw_set_output_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct platform_private* ext = impl_from_DEVICE_OBJECT(device);
    ULONG length = packet->reportBufferLen;
    BYTE buffer[8192];
    int count = 0;

    if ((buffer[0] = packet->reportId))
        count = write(ext->device_fd, packet->reportBuffer, length);
    else if (length > sizeof(buffer) - 1)
        ERR_(hid_report)("id %d length %u >= 8192, cannot write\n", packet->reportId, length);
    else
    {
        memcpy(buffer + 1, packet->reportBuffer, length);
        count = write(ext->device_fd, buffer, length + 1);
    }

    if (count > 0)
    {
        io->Information = count;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        ERR_(hid_report)("id %d write failed error: %d %s\n", packet->reportId, errno, strerror(errno));
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
}

static void hidraw_get_feature_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
#if defined(HAVE_LINUX_HIDRAW_H) && defined(HIDIOCGFEATURE)
    struct platform_private* ext = impl_from_DEVICE_OBJECT(device);
    ULONG length = packet->reportBufferLen;
    BYTE buffer[8192];
    int count = 0;

    if ((buffer[0] = packet->reportId) && length <= 0x1fff)
        count = ioctl(ext->device_fd, HIDIOCGFEATURE(length), packet->reportBuffer);
    else if (length > sizeof(buffer) - 1)
        ERR_(hid_report)("id %d length %u >= 8192, cannot read\n", packet->reportId, length);
    else
    {
        count = ioctl(ext->device_fd, HIDIOCGFEATURE(length + 1), buffer);
        memcpy(packet->reportBuffer, buffer + 1, length);
    }

    if (count > 0)
    {
        io->Information = count;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        ERR_(hid_report)("id %d read failed, error: %d %s\n", packet->reportId, errno, strerror(errno));
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
#else
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
#endif
}

static void hidraw_set_feature_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
#if defined(HAVE_LINUX_HIDRAW_H) && defined(HIDIOCSFEATURE)
    struct platform_private* ext = impl_from_DEVICE_OBJECT(device);
    ULONG length = packet->reportBufferLen;
    BYTE buffer[8192];
    int count = 0;

    if ((buffer[0] = packet->reportId) && length <= 0x1fff)
        count = ioctl(ext->device_fd, HIDIOCSFEATURE(length), packet->reportBuffer);
    else if (length > sizeof(buffer) - 1)
        ERR_(hid_report)("id %d length %u >= 8192, cannot write\n", packet->reportId, length);
    else
    {
        memcpy(buffer + 1, packet->reportBuffer, length);
        count = ioctl(ext->device_fd, HIDIOCSFEATURE(length + 1), buffer);
    }

    if (count > 0)
    {
        io->Information = count;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        ERR_(hid_report)("id %d write failed, error: %d %s\n", packet->reportId, errno, strerror(errno));
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
#else
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
#endif
}

static const platform_vtbl hidraw_vtbl =
{
    hidraw_free_device,
    compare_platform_device,
    hidraw_start_device,
    hidraw_get_reportdescriptor,
    hidraw_get_string,
    hidraw_set_output_report,
    hidraw_get_feature_report,
    hidraw_set_feature_report,
};

#ifdef HAS_PROPER_INPUT_HEADER

static inline struct wine_input_private *input_impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return CONTAINING_RECORD(impl_from_DEVICE_OBJECT(device), struct wine_input_private, base);
}

static void lnxev_free_device(DEVICE_OBJECT *device)
{
    struct wine_input_private *ext = input_impl_from_DEVICE_OBJECT(device);

    if (ext->base.report_thread)
    {
        write(ext->base.control_pipe[1], "q", 1);
        WaitForSingleObject(ext->base.report_thread, INFINITE);
        close(ext->base.control_pipe[0]);
        close(ext->base.control_pipe[1]);
        CloseHandle(ext->base.report_thread);
    }

    HeapFree(GetProcessHeap(), 0, ext->current_report_buffer);
    HeapFree(GetProcessHeap(), 0, ext->last_report_buffer);
    hid_descriptor_free(&ext->desc);

    close(ext->base.device_fd);
    udev_device_unref(ext->base.udev_device);

    HeapFree(GetProcessHeap(), 0, ext);
}

static DWORD CALLBACK lnxev_device_report_thread(void *args);

static NTSTATUS lnxev_start_device(DEVICE_OBJECT *device)
{
    struct wine_input_private *ext = input_impl_from_DEVICE_OBJECT(device);
    NTSTATUS status;

    if ((status = build_report_descriptor(ext, ext->base.udev_device)))
        return status;

    if (pipe(ext->base.control_pipe) != 0)
    {
        ERR("Control pipe creation failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    ext->base.report_thread = CreateThread(NULL, 0, lnxev_device_report_thread, device, 0, NULL);
    if (!ext->base.report_thread)
    {
        ERR("Unable to create device report thread\n");
        close(ext->base.control_pipe[0]);
        close(ext->base.control_pipe[1]);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS lnxev_get_reportdescriptor(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length)
{
    struct wine_input_private *ext = input_impl_from_DEVICE_OBJECT(device);

    *out_length = ext->desc.size;
    if (length < ext->desc.size) return STATUS_BUFFER_TOO_SMALL;

    memcpy(buffer, ext->desc.data, ext->desc.size);
    return STATUS_SUCCESS;
}

static NTSTATUS lnxev_get_string(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD length)
{
    struct wine_input_private *ext = input_impl_from_DEVICE_OBJECT(device);
    char str[255];

    str[0] = 0;
    switch (index)
    {
        case HID_STRING_ID_IPRODUCT:
            ioctl(ext->base.device_fd, EVIOCGNAME(sizeof(str)), str);
            break;
        case HID_STRING_ID_IMANUFACTURER:
            strcpy(str,"evdev");
            break;
        case HID_STRING_ID_ISERIALNUMBER:
            ioctl(ext->base.device_fd, EVIOCGUNIQ(sizeof(str)), str);
            break;
        default:
            ERR("Unhandled string index %i\n", index);
    }

    MultiByteToWideChar(CP_ACP, 0, str, -1, buffer, length);
    return STATUS_SUCCESS;
}

static DWORD CALLBACK lnxev_device_report_thread(void *args)
{
    DEVICE_OBJECT *device = (DEVICE_OBJECT*)args;
    struct wine_input_private *private = input_impl_from_DEVICE_OBJECT(device);
    struct pollfd plfds[2];

    plfds[0].fd = private->base.device_fd;
    plfds[0].events = POLLIN;
    plfds[0].revents = 0;
    plfds[1].fd = private->base.control_pipe[0];
    plfds[1].events = POLLIN;
    plfds[1].revents = 0;

    while (1)
    {
        int size;
        struct input_event ie;

        if (poll(plfds, 2, -1) <= 0) continue;
        if (plfds[1].revents || !private->current_report_buffer || private->buffer_length == 0)
            break;
        size = read(plfds[0].fd, &ie, sizeof(ie));
        if (size == -1)
            TRACE_(hid_report)("Read failed. Likely an unplugged device\n");
        else if (size == 0)
            TRACE_(hid_report)("Failed to read report\n");
        else if (set_report_from_event(private, &ie))
            process_hid_report(device, private->current_report_buffer, private->buffer_length);
    }
    return 0;
}

static void lnxev_set_output_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
}

static void lnxev_get_feature_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
}

static void lnxev_set_feature_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
}

static const platform_vtbl lnxev_vtbl = {
    lnxev_free_device,
    compare_platform_device,
    lnxev_start_device,
    lnxev_get_reportdescriptor,
    lnxev_get_string,
    lnxev_set_output_report,
    lnxev_get_feature_report,
    lnxev_set_feature_report,
};
#endif

static const char *get_device_syspath(struct udev_device *dev)
{
    struct udev_device *parent;

    if ((parent = udev_device_get_parent_with_subsystem_devtype(dev, "hid", NULL)))
        return udev_device_get_syspath(parent);

    if ((parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device")))
        return udev_device_get_syspath(parent);

    return "";
}

static int check_device_syspath(DEVICE_OBJECT *device, void* context)
{
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);
    return strcmp(get_device_syspath(private->udev_device), context);
}

static void get_device_subsystem_info(struct udev_device *dev, char const *subsystem, DWORD *vendor_id,
                                      DWORD *product_id, DWORD *input, DWORD *version, WCHAR **serial_number)
{
    struct udev_device *parent = NULL;
    const char *ptr, *next, *tmp;
    char buffer[256];
    DWORD bus = 0;

    if (!(parent = udev_device_get_parent_with_subsystem_devtype(dev, subsystem, NULL))) return;

    if ((next = udev_device_get_sysattr_value(parent, "uevent")))
    {
        while ((ptr = next) && *ptr)
        {
            if ((next = strchr(next, '\n'))) next += 1;
            else next = ptr + strlen(ptr);
            TRACE("%s uevent %s\n", subsystem, debugstr_an(ptr, next - ptr - 1));

            if (!strncmp(ptr, "HID_UNIQ=", 9))
            {
                if (sscanf(ptr, "HID_UNIQ=%256s\n", buffer) != 1 || !*buffer) continue;
                if (!*serial_number) *serial_number = strdupAtoW(buffer);
            }
            if (!strncmp(ptr, "HID_PHYS=", 9) || !strncmp(ptr, "PHYS=\"", 6))
            {
                if (!(tmp = strstr(ptr, "/input")) || tmp >= next) continue;
                if (*input == -1) sscanf(tmp, "/input%d\n", input);
            }
            if (!strncmp(ptr, "HID_ID=", 7))
            {
                if (bus || *vendor_id || *product_id) continue;
                sscanf(ptr, "HID_ID=%x:%x:%x\n", &bus, vendor_id, product_id);
            }
            if (!strncmp(ptr, "PRODUCT=", 8))
            {
                if (*version) continue;
                if (!strcmp(subsystem, "usb"))
                    sscanf(ptr, "PRODUCT=%x/%x/%x\n", vendor_id, product_id, version);
                else
                    sscanf(ptr, "PRODUCT=%x/%x/%x/%x\n", &bus, vendor_id, product_id, version);
            }
        }
    }
}

static void try_add_device(struct udev_device *dev)
{
    DWORD vid = 0, pid = 0, version = 0, input = -1;
    struct platform_private *private;
    DEVICE_OBJECT *device = NULL;
    const char *subsystem;
    const char *devnode;
    WCHAR *serial = NULL;
    BOOL is_gamepad = FALSE;
    int fd;
    static const CHAR *base_serial = "0000";

    if (!(devnode = udev_device_get_devnode(dev)))
        return;

    if ((fd = open(devnode, O_RDWR)) == -1)
    {
        WARN("Unable to open udev device %s: %s\n", debugstr_a(devnode), strerror(errno));
        return;
    }

    TRACE("udev %s syspath %s\n", debugstr_a(devnode), udev_device_get_syspath(dev));

#ifdef HAS_PROPER_INPUT_HEADER
    device = bus_enumerate_hid_devices(lnxev_busidW, check_device_syspath, (void *)get_device_syspath(dev));
    if (!device) device = bus_enumerate_hid_devices(hidraw_busidW, check_device_syspath, (void *)get_device_syspath(dev));
    if (device)
    {
        TRACE("duplicate device found, not adding the new one\n");
        close(fd);
        return;
    }
#endif

    get_device_subsystem_info(dev, "hid", &vid, &pid, &input, &version, &serial);
    get_device_subsystem_info(dev, "input", &vid, &pid, &input, &version, &serial);
    get_device_subsystem_info(dev, "usb", &vid, &pid, &input, &version, &serial);

    subsystem = udev_device_get_subsystem(dev);
#ifdef HAS_PROPER_INPUT_HEADER
    if (!strcmp(subsystem, "input"))
    {
        struct input_id device_id = {0};
        char device_uid[255];

        if (ioctl(fd, EVIOCGID, &device_id) < 0)
            WARN("ioctl(EVIOCGID) failed: %d %s\n", errno, strerror(errno));
        else
        {
            vid = device_id.vendor;
            pid = device_id.product;
            version = device_id.version;
        }

        device_uid[0] = 0;
        if (ioctl(fd, EVIOCGUNIQ(254), device_uid) >= 0 && device_uid[0])
            serial = strdupAtoW(device_uid);
    }
#endif

    if (serial == NULL) serial = strdupAtoW(base_serial);

    if (is_xbox_gamepad(vid, pid))
        is_gamepad = TRUE;
#ifdef HAS_PROPER_INPUT_HEADER
    else
    {
        int axes=0, buttons=0;
        axes = count_abs_axis(fd);
        buttons = count_buttons(fd, NULL);
        is_gamepad = (axes == 6  && buttons >= 14);
    }
#endif
    if (input == (WORD)-1 && is_gamepad)
        input = 0;

    TRACE("Found udev device %s (vid %04x, pid %04x, version %04x, input %d, serial %s)\n",
          debugstr_a(devnode), vid, pid, version, input, debugstr_w(serial));

    if (strcmp(subsystem, "hidraw") == 0)
    {
        if (!(private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct platform_private))))
            return;
        device = bus_create_hid_device(hidraw_busidW, vid, pid, input, version, 0, serial,
                                       is_gamepad, &hidraw_vtbl, &private->unix_device);
        if (!device) HeapFree(GetProcessHeap(), 0, private);
    }
#ifdef HAS_PROPER_INPUT_HEADER
    else if (strcmp(subsystem, "input") == 0)
    {
        if (!(private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct wine_input_private))))
            return;
        device = bus_create_hid_device(lnxev_busidW, vid, pid, input, version, 0, serial,
                                       is_gamepad, &lnxev_vtbl, &private->unix_device);
        if (!device) HeapFree(GetProcessHeap(), 0, private);
    }
#endif

    if (device)
    {
        private->udev_device = udev_device_ref(dev);
        private->device_fd = fd;
        IoInvalidateDeviceRelations(bus_pdo, BusRelations);
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

    device = bus_find_hid_device(hidraw_busidW, dev);
#ifdef HAS_PROPER_INPUT_HEADER
    if (device == NULL) device = bus_find_hid_device(lnxev_busidW, dev);
#endif
    if (!device) return;

    bus_unlink_hid_device(device);
    IoInvalidateDeviceRelations(bus_pdo, BusRelations);
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

    if (!options.disable_hidraw)
        if (udev_enumerate_add_match_subsystem(enumerate, "hidraw") < 0)
            WARN("Failed to add subsystem 'hidraw' to enumeration\n");
#ifdef HAS_PROPER_INPUT_HEADER
    if (!options.disable_input)
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

    if (!options.disable_hidraw)
    {
        if (udev_monitor_filter_add_match_subsystem_devtype(monitor, "hidraw", NULL) < 0)
            WARN("Failed to add 'hidraw' subsystem to monitor\n");
        else
            systems++;
    }
#ifdef HAS_PROPER_INPUT_HEADER
    if (!options.disable_input)
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

NTSTATUS udev_bus_init(void *args)
{
    TRACE("args %p\n", args);

    options = *(struct udev_bus_options *)args;

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

    if (!(udev_monitor = create_monitor(&udev_monitor_fd)))
    {
        ERR("UDEV monitor creation failed\n");
        goto error;
    }

    build_initial_deviceset();
    return STATUS_SUCCESS;

error:
    if (udev_context) udev_unref(udev_context);
    udev_context = NULL;
    close(deviceloop_control[0]);
    close(deviceloop_control[1]);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS udev_bus_wait(void *args)
{
    struct pollfd pfd[2];

    pfd[0].fd = udev_monitor_fd;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;
    pfd[1].fd = deviceloop_control[0];
    pfd[1].events = POLLIN;
    pfd[1].revents = 0;

    while (1)
    {
        if (poll(pfd, 2, -1) <= 0) continue;
        if (pfd[1].revents) break;
        process_monitor_event(udev_monitor);
    }

    TRACE("UDEV main loop exiting\n");
    udev_monitor_unref(udev_monitor);
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
