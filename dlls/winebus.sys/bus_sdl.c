/*
 * Plug and Play support for hid devices found through SDL2
 *
 * Copyright 2017 CodeWeavers, Aric Stewart
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
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SDL2_SDL_H
# include <SDL2/SDL.h>
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
#include "wine/library.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "hidusage.h"
#include "controller.h"

#ifdef WORDS_BIGENDIAN
# define LE_WORD(x) RtlUshortByteSwap(x)
#else
# define LE_WORD(x) (x)
#endif

#include "bus.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef SONAME_LIBSDL2

WINE_DECLARE_DEBUG_CHANNEL(hid_report);

static DRIVER_OBJECT *sdl_driver_obj = NULL;

static const WCHAR sdl_busidW[] = {'S','D','L','J','O','Y',0};

#include "initguid.h"
DEFINE_GUID(GUID_DEVCLASS_SDL, 0x463d60b5,0x802b,0x4bb2,0x8f,0xdb,0x7d,0xa9,0xb9,0x96,0x04,0xd8);

static void *sdl_handle = NULL;

#ifdef SONAME_LIBSDL2
#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(SDL_GetError);
MAKE_FUNCPTR(SDL_Init);
MAKE_FUNCPTR(SDL_JoystickEventState);
MAKE_FUNCPTR(SDL_JoystickGetGUID);
MAKE_FUNCPTR(SDL_JoystickGetGUIDString);
MAKE_FUNCPTR(SDL_JoystickInstanceID);
MAKE_FUNCPTR(SDL_JoystickName);
MAKE_FUNCPTR(SDL_JoystickNumAxes);
MAKE_FUNCPTR(SDL_JoystickOpen);
MAKE_FUNCPTR(SDL_WaitEvent);
MAKE_FUNCPTR(SDL_JoystickNumButtons);
MAKE_FUNCPTR(SDL_JoystickNumBalls);
MAKE_FUNCPTR(SDL_JoystickNumHats);
MAKE_FUNCPTR(SDL_JoystickGetAxis);
MAKE_FUNCPTR(SDL_JoystickGetHat);
#endif
static Uint16 (*pSDL_JoystickGetProduct)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetProductVersion)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetVendor)(SDL_Joystick * joystick);

struct platform_private
{
    SDL_Joystick *sdl_joystick;
    SDL_JoystickID id;

    int axis_start;
    int ball_start;
    int hat_start;

    int report_descriptor_size;
    BYTE *report_descriptor;

    int buffer_length;
    BYTE *report_buffer;
};

static inline struct platform_private *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return (struct platform_private *)get_platform_private(device);
}

static const BYTE REPORT_AXIS_TAIL[] = {
    0x16, 0x00, 0x80,   /* LOGICAL_MINIMUM (-32768) */
    0x26, 0xff, 0x7f,   /* LOGICAL_MAXIMUM (32767) */
    0x36, 0x00, 0x80,   /* PHYSICAL_MINIMUM (-32768) */
    0x46, 0xff, 0x7f,   /* PHYSICAL_MAXIMUM (32767) */
    0x75, 0x10,         /* REPORT_SIZE (16) */
    0x95, 0x00,         /* REPORT_COUNT (?) */
    0x81, 0x02,         /* INPUT (Data,Var,Abs) */
};
#define IDX_ABS_AXIS_COUNT 15

static BYTE *add_axis_block(BYTE *report_ptr, BYTE count, BYTE page, const BYTE *usages, BOOL absolute)
{
    int i;
    memcpy(report_ptr, REPORT_AXIS_HEADER, sizeof(REPORT_AXIS_HEADER));
    report_ptr[IDX_AXIS_PAGE] = page;
    report_ptr += sizeof(REPORT_AXIS_HEADER);
    for (i = 0; i < count; i++)
    {
        memcpy(report_ptr, REPORT_AXIS_USAGE, sizeof(REPORT_AXIS_USAGE));
        report_ptr[IDX_AXIS_USAGE] = usages[i];
        report_ptr += sizeof(REPORT_AXIS_USAGE);
    }
    if (absolute)
    {
        memcpy(report_ptr, REPORT_AXIS_TAIL, sizeof(REPORT_AXIS_TAIL));
        report_ptr[IDX_ABS_AXIS_COUNT] = count;
        report_ptr += sizeof(REPORT_AXIS_TAIL);
    }
    else
    {
        memcpy(report_ptr, REPORT_REL_AXIS_TAIL, sizeof(REPORT_REL_AXIS_TAIL));
        report_ptr[IDX_REL_AXIS_COUNT] = count;
        report_ptr += sizeof(REPORT_REL_AXIS_TAIL);
    }
    return report_ptr;
}

static void set_axis_value(struct platform_private *ext, int index, short value)
{
    int offset;
    offset = ext->axis_start + index * 2;
    *((WORD*)&ext->report_buffer[offset]) = LE_WORD(value);
}

static void set_ball_value(struct platform_private *ext, int index, int value1, int value2)
{
    int offset;
    offset = ext->ball_start + (index * 2);
    if (value1 > 127) value1 = 127;
    if (value1 < -127) value1 = -127;
    if (value2 > 127) value2 = 127;
    if (value2 < -127) value2 = -127;
    ext->report_buffer[offset] = value1;
    ext->report_buffer[offset + 1] = value2;
}

static void set_hat_value(struct platform_private *ext, int index, int value)
{
    int offset;
    offset = ext->hat_start + index;
    switch (value)
    {
        case SDL_HAT_CENTERED: ext->report_buffer[offset] = 8; break;
        case SDL_HAT_UP: ext->report_buffer[offset] = 0; break;
        case SDL_HAT_RIGHTUP: ext->report_buffer[offset] = 1; break;
        case SDL_HAT_RIGHT: ext->report_buffer[offset] = 2; break;
        case SDL_HAT_RIGHTDOWN: ext->report_buffer[offset] = 3; break;
        case SDL_HAT_DOWN: ext->report_buffer[offset] = 4; break;
        case SDL_HAT_LEFTDOWN: ext->report_buffer[offset] = 5; break;
        case SDL_HAT_LEFT: ext->report_buffer[offset] = 6; break;
        case SDL_HAT_LEFTUP: ext->report_buffer[offset] = 7; break;
    }
}

static BOOL build_report_descriptor(struct platform_private *ext)
{
    BYTE *report_ptr;
    INT i, descript_size;
    INT report_size;
    INT button_count, axis_count, ball_count, hat_count;
    static const BYTE device_usage[2] = {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD};
    static const BYTE controller_usages[] = {
        HID_USAGE_GENERIC_X,
        HID_USAGE_GENERIC_Y,
        HID_USAGE_GENERIC_Z,
        HID_USAGE_GENERIC_RX,
        HID_USAGE_GENERIC_RY,
        HID_USAGE_GENERIC_RZ,
        HID_USAGE_GENERIC_SLIDER,
        HID_USAGE_GENERIC_DIAL,
        HID_USAGE_GENERIC_WHEEL};
    static const BYTE joystick_usages[] = {
        HID_USAGE_GENERIC_X,
        HID_USAGE_GENERIC_Y,
        HID_USAGE_GENERIC_Z,
        HID_USAGE_GENERIC_RZ,
        HID_USAGE_GENERIC_RX,
        HID_USAGE_GENERIC_RY,
        HID_USAGE_GENERIC_SLIDER,
        HID_USAGE_GENERIC_DIAL,
        HID_USAGE_GENERIC_WHEEL};

    descript_size = sizeof(REPORT_HEADER) + sizeof(REPORT_TAIL);
    report_size = 0;

    /* For now lump all buttons just into incremental usages, Ignore Keys */
    button_count = pSDL_JoystickNumButtons(ext->sdl_joystick);
    if (button_count)
    {
        descript_size += sizeof(REPORT_BUTTONS);
        if (button_count % 8)
            descript_size += sizeof(REPORT_PADDING);
        report_size = (button_count + 7) / 8;
    }

    axis_count = pSDL_JoystickNumAxes(ext->sdl_joystick);
    if (axis_count > 6)
    {
        FIXME("Clamping joystick to 6 axis\n");
        axis_count = 6;
    }

    ext->axis_start = report_size;
    if (axis_count)
    {
        descript_size += sizeof(REPORT_AXIS_HEADER);
        descript_size += (sizeof(REPORT_AXIS_USAGE) * axis_count);
        descript_size += sizeof(REPORT_AXIS_TAIL);
        report_size += (2 * axis_count);
    }

    ball_count = pSDL_JoystickNumBalls(ext->sdl_joystick);
    ext->ball_start = report_size;
    if (ball_count)
    {
        if ((ball_count*2) + axis_count > 9)
        {
            FIXME("Capping ball + axis at 9\n");
            ball_count = (9-axis_count)/2;
        }
        descript_size += sizeof(REPORT_AXIS_HEADER);
        descript_size += (sizeof(REPORT_AXIS_USAGE) * ball_count * 2);
        descript_size += sizeof(REPORT_REL_AXIS_TAIL);
        report_size += (2*ball_count);
    }

    hat_count = pSDL_JoystickNumHats(ext->sdl_joystick);
    ext->hat_start = report_size;
    if (hat_count)
    {
        descript_size += sizeof(REPORT_HATSWITCH);
        for (i = 0; i < hat_count; i++)
            report_size++;
    }

    TRACE("Report Descriptor will be %i bytes\n", descript_size);
    TRACE("Report will be %i bytes\n", report_size);

    ext->report_descriptor = HeapAlloc(GetProcessHeap(), 0, descript_size);
    if (!ext->report_descriptor)
    {
        ERR("Failed to alloc report descriptor\n");
        return FALSE;
    }
    report_ptr = ext->report_descriptor;

    memcpy(report_ptr, REPORT_HEADER, sizeof(REPORT_HEADER));
    report_ptr[IDX_HEADER_PAGE] = device_usage[0];
    report_ptr[IDX_HEADER_USAGE] = device_usage[1];
    report_ptr += sizeof(REPORT_HEADER);
    if (button_count)
    {
        report_ptr = add_button_block(report_ptr, 1, button_count);
        if (button_count % 8)
        {
            BYTE padding = 8 - (button_count % 8);
            report_ptr = add_padding_block(report_ptr, padding);
        }
    }
    if (axis_count)
    {
        if (axis_count == 6 && button_count >= 14)
            report_ptr = add_axis_block(report_ptr, axis_count, HID_USAGE_PAGE_GENERIC, controller_usages, TRUE);
        else
            report_ptr = add_axis_block(report_ptr, axis_count, HID_USAGE_PAGE_GENERIC, joystick_usages, TRUE);

    }
    if (ball_count)
    {
        report_ptr = add_axis_block(report_ptr, ball_count * 2, HID_USAGE_PAGE_GENERIC, &joystick_usages[axis_count], FALSE);
    }
    if (hat_count)
        report_ptr = add_hatswitch(report_ptr, hat_count);

    memcpy(report_ptr, REPORT_TAIL, sizeof(REPORT_TAIL));

    ext->report_descriptor_size = descript_size;
    ext->buffer_length = report_size;
    ext->report_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, report_size);
    if (ext->report_buffer == NULL)
    {
        ERR("Failed to alloc report buffer\n");
        HeapFree(GetProcessHeap(), 0, ext->report_descriptor);
        return FALSE;
    }

    /* Initialize axis in the report */
    for (i = 0; i < axis_count; i++)
        set_axis_value(ext, i, pSDL_JoystickGetAxis(ext->sdl_joystick, i));
    for (i = 0; i < hat_count; i++)
        set_hat_value(ext, i, pSDL_JoystickGetHat(ext->sdl_joystick, i));

    return TRUE;
}

static int compare_platform_device(DEVICE_OBJECT *device, void *platform_dev)
{
    SDL_JoystickID id1 = impl_from_DEVICE_OBJECT(device)->id;
    SDL_JoystickID id2 = PtrToUlong(platform_dev);
    return (id1 != id2);
}

static NTSTATUS get_reportdescriptor(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length)
{
    struct platform_private *ext = impl_from_DEVICE_OBJECT(device);

    *out_length = ext->report_descriptor_size;

    if (length < ext->report_descriptor_size)
        return STATUS_BUFFER_TOO_SMALL;

    memcpy(buffer, ext->report_descriptor, ext->report_descriptor_size);

    return STATUS_SUCCESS;
}

static NTSTATUS get_string(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD length)
{
    struct platform_private *ext = impl_from_DEVICE_OBJECT(device);
    const char* str = NULL;

    switch (index)
    {
        case HID_STRING_ID_IPRODUCT:
            str = pSDL_JoystickName(ext->sdl_joystick);
            break;
        case HID_STRING_ID_IMANUFACTURER:
            str = "SDL";
            break;
        case HID_STRING_ID_ISERIALNUMBER:
            str = "000000";
            break;
        default:
            ERR("Unhandled string index %i\n", index);
    }

    if (str && str[0])
        MultiByteToWideChar(CP_ACP, 0, str, -1, buffer, length);
    else
        buffer[0] = 0;

    return STATUS_SUCCESS;
}

static NTSTATUS begin_report_processing(DEVICE_OBJECT *device)
{
    return STATUS_SUCCESS;
}

static NTSTATUS set_output_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written)
{
    *written = 0;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *read)
{
    *read = 0;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS set_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written)
{
    *written = 0;
    return STATUS_NOT_IMPLEMENTED;
}

static const platform_vtbl sdl_vtbl =
{
    compare_platform_device,
    get_reportdescriptor,
    get_string,
    begin_report_processing,
    set_output_report,
    get_feature_report,
    set_feature_report,
};

static BOOL set_report_from_event(SDL_Event *event)
{
    DEVICE_OBJECT *device;
    struct platform_private *private;
    /* All the events coming in will have 'which' as a 3rd field */
    SDL_JoystickID index = ((SDL_JoyButtonEvent*)event)->which;

    device = bus_find_hid_device(&sdl_vtbl, ULongToPtr(index));
    if (!device)
    {
        ERR("Failed to find device at index %i\n",index);
        return FALSE;
    }
    private = impl_from_DEVICE_OBJECT(device);

    switch(event->type)
    {
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        {
            SDL_JoyButtonEvent *ie = &event->jbutton;

            set_button_value(ie->button, ie->state, private->report_buffer);

            process_hid_report(device, private->report_buffer, private->buffer_length);
            break;
        }
        case SDL_JOYAXISMOTION:
        {
            SDL_JoyAxisEvent *ie = &event->jaxis;

            if (ie->axis < 6)
            {
                set_axis_value(private, ie->axis, ie->value);
                process_hid_report(device, private->report_buffer, private->buffer_length);
            }
            break;
        }
        case SDL_JOYBALLMOTION:
        {
            SDL_JoyBallEvent *ie = &event->jball;

            set_ball_value(private, ie->ball, ie->xrel, ie->yrel);
            process_hid_report(device, private->report_buffer, private->buffer_length);
            break;
        }
        case SDL_JOYHATMOTION:
        {
            SDL_JoyHatEvent *ie = &event->jhat;

            set_hat_value(private, ie->hat, ie->value);
            process_hid_report(device, private->report_buffer, private->buffer_length);
            break;
        }
        default:
            ERR("TODO: Process Report (%x)\n",event->type);
    }
    return FALSE;
}

static void try_remove_device(SDL_JoystickID index)
{
    DEVICE_OBJECT *device = NULL;

    device = bus_find_hid_device(&sdl_vtbl, ULongToPtr(index));
    if (!device) return;

    IoInvalidateDeviceRelations(device, RemovalRelations);

    bus_remove_hid_device(device);
}

static void try_add_device(SDL_JoystickID index)
{
    DWORD vid = 0, pid = 0, version = 0;
    DEVICE_OBJECT *device = NULL;
    WCHAR serial[34] = {0};
    char guid_str[34];
    BOOL is_xbox_gamepad;
    int button_count, axis_count;

    SDL_Joystick* joystick;
    SDL_JoystickID id;
    SDL_JoystickGUID guid;

    if ((joystick = pSDL_JoystickOpen(index)) == NULL)
    {
        WARN("Unable to open sdl device %i: %s\n", index, pSDL_GetError());
        return;
    }

    id = index;
    if (pSDL_JoystickGetProductVersion != NULL) {
        vid = pSDL_JoystickGetVendor(joystick);
        pid = pSDL_JoystickGetProduct(joystick);
        version = pSDL_JoystickGetProductVersion(joystick);
    }
    else
    {
        vid = 0x01;
        pid = pSDL_JoystickInstanceID(joystick) + 1;
        version = 0;
    }

    guid = pSDL_JoystickGetGUID(joystick);
    pSDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));
    MultiByteToWideChar(CP_ACP, 0, guid_str, -1, serial, sizeof(guid_str));

    TRACE("Found sdl device %i (vid %04x, pid %04x, version %u, serial %s)\n",
          index, vid, pid, version, debugstr_w(serial));

    axis_count = pSDL_JoystickNumAxes(joystick);
    button_count = pSDL_JoystickNumAxes(joystick);
    is_xbox_gamepad = (axis_count == 6  && button_count >= 14);

    device = bus_create_hid_device(sdl_driver_obj, sdl_busidW, vid, pid, version, 0, serial, is_xbox_gamepad, &GUID_DEVCLASS_SDL, &sdl_vtbl, sizeof(struct platform_private));

    if (device)
    {
        struct platform_private *private = impl_from_DEVICE_OBJECT(device);
        private->sdl_joystick = joystick;
        private->id = id;
        if (!build_report_descriptor(private))
        {
            ERR("Building report descriptor failed, removing device\n");
            bus_remove_hid_device(device);
            HeapFree(GetProcessHeap(), 0, serial);
            return;
        }
        IoInvalidateDeviceRelations(device, BusRelations);
    }
    else
    {
        WARN("Ignoring device %i\n", index);
    }
}

static void process_device_event(SDL_Event *event)
{
    TRACE_(hid_report)("Received action %x\n", event->type);

    if (event->type == SDL_JOYDEVICEADDED)
        try_add_device(((SDL_JoyDeviceEvent*)event)->which);
    else if (event->type == SDL_JOYDEVICEREMOVED)
        try_remove_device(((SDL_JoyDeviceEvent*)event)->which);
    else if (event->type >= SDL_JOYAXISMOTION && event->type <= SDL_JOYBUTTONUP)
        set_report_from_event(event);
}

static DWORD CALLBACK deviceloop_thread(void *args)
{
    HANDLE init_done = args;
    SDL_Event event;

    if (pSDL_Init(SDL_INIT_JOYSTICK) < 0)
    {
        ERR("Can't Init SDL\n");
        return STATUS_UNSUCCESSFUL;
    }

    pSDL_JoystickEventState(SDL_ENABLE);

    SetEvent(init_done);

    while (1)
        while (pSDL_WaitEvent(&event) != 0)
            process_device_event(&event);

    TRACE("Device thread exiting\n");
    return 0;
}

NTSTATUS WINAPI sdl_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path)
{
    HANDLE events[2];
    DWORD result;

    TRACE("(%p, %s)\n", driver, debugstr_w(registry_path->Buffer));
    if (sdl_handle == NULL)
    {
        sdl_handle = wine_dlopen(SONAME_LIBSDL2, RTLD_NOW, NULL, 0);
        if (!sdl_handle) {
            WARN("could not load %s\n", SONAME_LIBSDL2);
            sdl_driver_obj = NULL;
            return STATUS_UNSUCCESSFUL;
        }
#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(sdl_handle, #f, NULL, 0)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}
        LOAD_FUNCPTR(SDL_GetError);
        LOAD_FUNCPTR(SDL_Init);
        LOAD_FUNCPTR(SDL_JoystickEventState);
        LOAD_FUNCPTR(SDL_JoystickGetGUID);
        LOAD_FUNCPTR(SDL_JoystickGetGUIDString);
        LOAD_FUNCPTR(SDL_JoystickInstanceID);
        LOAD_FUNCPTR(SDL_JoystickName);
        LOAD_FUNCPTR(SDL_JoystickNumAxes);
        LOAD_FUNCPTR(SDL_JoystickOpen);
        LOAD_FUNCPTR(SDL_WaitEvent);
        LOAD_FUNCPTR(SDL_JoystickNumButtons);
        LOAD_FUNCPTR(SDL_JoystickNumBalls);
        LOAD_FUNCPTR(SDL_JoystickNumHats);
        LOAD_FUNCPTR(SDL_JoystickGetAxis);
        LOAD_FUNCPTR(SDL_JoystickGetHat);
#undef LOAD_FUNCPTR
        pSDL_JoystickGetProduct = wine_dlsym(sdl_handle, "SDL_JoystickGetProduct", NULL, 0);
        pSDL_JoystickGetProductVersion = wine_dlsym(sdl_handle, "SDL_JoystickGetProductVersion", NULL, 0);
        pSDL_JoystickGetVendor = wine_dlsym(sdl_handle, "SDL_JoystickGetVendor", NULL, 0);
    }

    sdl_driver_obj = driver;
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
    sdl_driver_obj = NULL;
    return STATUS_UNSUCCESSFUL;
sym_not_found:
    wine_dlclose(sdl_handle, NULL, 0);
    sdl_handle = NULL;
    return STATUS_UNSUCCESSFUL;
}

#else

NTSTATUS WINAPI sdl_driver_init(DRIVER_OBJECT *driver, UNICODE_STRING *registry_path)
{
    WARN("compiled without SDL support\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBSDL2 */
