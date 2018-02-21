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
#endif
static Uint16 (*pSDL_JoystickGetProduct)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetProductVersion)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetVendor)(SDL_Joystick * joystick);

struct platform_private
{
    SDL_Joystick *sdl_joystick;
    SDL_JoystickID id;
};

static inline struct platform_private *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return (struct platform_private *)get_platform_private(device);
}

static int compare_platform_device(DEVICE_OBJECT *device, void *platform_dev)
{
    SDL_JoystickID id1 = impl_from_DEVICE_OBJECT(device)->id;
    SDL_JoystickID id2 = PtrToUlong(platform_dev);
    return (id1 != id2);
}

static NTSTATUS get_reportdescriptor(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length)
{
    return STATUS_NOT_IMPLEMENTED;
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
