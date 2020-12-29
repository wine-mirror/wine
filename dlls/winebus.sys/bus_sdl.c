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
#ifdef HAVE_SDL_H
# include <SDL.h>
#endif

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/hidtypes.h"
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

static const WCHAR sdl_busidW[] = {'S','D','L','J','O','Y',0};

static DWORD map_controllers = 0;

static void *sdl_handle = NULL;
static HANDLE deviceloop_handle;
static UINT quit_event = -1;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(SDL_GetError);
MAKE_FUNCPTR(SDL_Init);
MAKE_FUNCPTR(SDL_JoystickClose);
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
MAKE_FUNCPTR(SDL_IsGameController);
MAKE_FUNCPTR(SDL_GameControllerClose);
MAKE_FUNCPTR(SDL_GameControllerGetAxis);
MAKE_FUNCPTR(SDL_GameControllerGetButton);
MAKE_FUNCPTR(SDL_GameControllerName);
MAKE_FUNCPTR(SDL_GameControllerOpen);
MAKE_FUNCPTR(SDL_GameControllerEventState);
MAKE_FUNCPTR(SDL_HapticClose);
MAKE_FUNCPTR(SDL_HapticDestroyEffect);
MAKE_FUNCPTR(SDL_HapticNewEffect);
MAKE_FUNCPTR(SDL_HapticOpenFromJoystick);
MAKE_FUNCPTR(SDL_HapticQuery);
MAKE_FUNCPTR(SDL_HapticRumbleInit);
MAKE_FUNCPTR(SDL_HapticRumblePlay);
MAKE_FUNCPTR(SDL_HapticRumbleSupported);
MAKE_FUNCPTR(SDL_HapticRunEffect);
MAKE_FUNCPTR(SDL_HapticStopAll);
MAKE_FUNCPTR(SDL_JoystickIsHaptic);
MAKE_FUNCPTR(SDL_memset);
MAKE_FUNCPTR(SDL_GameControllerAddMapping);
MAKE_FUNCPTR(SDL_RegisterEvents);
MAKE_FUNCPTR(SDL_PushEvent);
static Uint16 (*pSDL_JoystickGetProduct)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetProductVersion)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetVendor)(SDL_Joystick * joystick);

struct platform_private
{
    SDL_Joystick *sdl_joystick;
    SDL_GameController *sdl_controller;
    SDL_JoystickID id;

    int button_start;
    int axis_start;
    int ball_start;
    int hat_bit_offs; /* hatswitches are reported in the same bytes as buttons */

    int report_descriptor_size;
    BYTE *report_descriptor;

    int buffer_length;
    BYTE *report_buffer;

    SDL_Haptic *sdl_haptic;
    int haptic_effect_id;
};

static inline struct platform_private *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return (struct platform_private *)get_platform_private(device);
}

static const BYTE REPORT_AXIS_TAIL[] = {
    0x17, 0x00, 0x00, 0x00, 0x00,   /* LOGICAL_MINIMUM (0) */
    0x27, 0xff, 0xff, 0x00, 0x00,   /* LOGICAL_MAXIMUM (65535) */
    0x37, 0x00, 0x00, 0x00, 0x00,   /* PHYSICAL_MINIMUM (0) */
    0x47, 0xff, 0xff, 0x00, 0x00,   /* PHYSICAL_MAXIMUM (65535) */
    0x75, 0x10,         /* REPORT_SIZE (16) */
    0x95, 0x00,         /* REPORT_COUNT (?) */
    0x81, 0x02,         /* INPUT (Data,Var,Abs) */
};
#define IDX_ABS_AXIS_COUNT 23

#define CONTROLLER_NUM_BUTTONS 11

static const BYTE CONTROLLER_BUTTONS[] = {
    0x05, 0x09, /* USAGE_PAGE (Button) */
    0x19, 0x01, /* USAGE_MINIMUM (Button 1) */
    0x29, CONTROLLER_NUM_BUTTONS, /* USAGE_MAXIMUM (Button 11) */
    0x15, 0x00, /* LOGICAL_MINIMUM (0) */
    0x25, 0x01, /* LOGICAL_MAXIMUM (1) */
    0x35, 0x00, /* LOGICAL_MINIMUM (0) */
    0x45, 0x01, /* LOGICAL_MAXIMUM (1) */
    0x95, CONTROLLER_NUM_BUTTONS, /* REPORT_COUNT (11) */
    0x75, 0x01, /* REPORT_SIZE (1) */
    0x81, 0x02, /* INPUT (Data,Var,Abs) */
};

static const BYTE CONTROLLER_AXIS [] = {
    0x05, 0x01,         /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x30,         /* USAGE (X) */
    0x09, 0x31,         /* USAGE (Y) */
    0x09, 0x33,         /* USAGE (RX) */
    0x09, 0x34,         /* USAGE (RY) */
    0x17, 0x00, 0x00, 0x00, 0x00,   /* LOGICAL_MINIMUM (0) */
    0x27, 0xff, 0xff, 0x00, 0x00,   /* LOGICAL_MAXIMUM (65535) */
    0x37, 0x00, 0x00, 0x00, 0x00,   /* PHYSICAL_MINIMUM (0) */
    0x47, 0xff, 0xff, 0x00, 0x00,   /* PHYSICAL_MAXIMUM (65535) */
    0x75, 0x10,         /* REPORT_SIZE (16) */
    0x95, 0x04,         /* REPORT_COUNT (4) */
    0x81, 0x02,         /* INPUT (Data,Var,Abs) */
};

static const BYTE CONTROLLER_TRIGGERS [] = {
    0x05, 0x01,         /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x32,         /* USAGE (Z) */
    0x09, 0x35,         /* USAGE (RZ) */
    0x16, 0x00, 0x00,   /* LOGICAL_MINIMUM (0) */
    0x26, 0xff, 0x7f,   /* LOGICAL_MAXIMUM (32767) */
    0x36, 0x00, 0x00,   /* PHYSICAL_MINIMUM (0) */
    0x46, 0xff, 0x7f,   /* PHYSICAL_MAXIMUM (32767) */
    0x75, 0x10,         /* REPORT_SIZE (16) */
    0x95, 0x02,         /* REPORT_COUNT (2) */
    0x81, 0x02,         /* INPUT (Data,Var,Abs) */
};

#define CONTROLLER_NUM_AXES 6

#define CONTROLLER_NUM_HATSWITCHES 1

static const BYTE HAPTIC_RUMBLE[] = {
    0x06, 0x00, 0xff,   /* USAGE PAGE (vendor-defined) */
    0x09, 0x01,         /* USAGE (1) */
    0x85, 0x00,         /* REPORT_ID (0) */
    /* padding */
    0x95, 0x02,         /* REPORT_COUNT (2) */
    0x75, 0x08,         /* REPORT_SIZE (8) */
    0x91, 0x02,         /* OUTPUT (Data,Var,Abs) */
    /* actuators */
    0x15, 0x00,         /* LOGICAL MINIMUM (0) */
    0x25, 0xff,         /* LOGICAL MAXIMUM (255) */
    0x35, 0x00,         /* PHYSICAL MINIMUM (0) */
    0x45, 0xff,         /* PHYSICAL MAXIMUM (255) */
    0x75, 0x08,         /* REPORT_SIZE (8) */
    0x95, 0x02,         /* REPORT_COUNT (2) */
    0x91, 0x02,         /* OUTPUT (Data,Var,Abs) */
    /* padding */
    0x95, 0x02,         /* REPORT_COUNT (3) */
    0x75, 0x08,         /* REPORT_SIZE (8) */
    0x91, 0x02,         /* OUTPUT (Data,Var,Abs) */
};

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

static void set_button_value(struct platform_private *ext, int index, int value)
{
    int byte_index = ext->button_start + index / 8;
    int bit_index = index % 8;
    BYTE mask = 1 << bit_index;

    if (value)
    {
        ext->report_buffer[byte_index] = ext->report_buffer[byte_index] | mask;
    }
    else
    {
        mask = ~mask;
        ext->report_buffer[byte_index] = ext->report_buffer[byte_index] & mask;
    }
}

static void set_axis_value(struct platform_private *ext, int index, short value, BOOL controller)
{
    WORD *report = (WORD *)(ext->report_buffer + ext->axis_start);

    if (controller && (index == SDL_CONTROLLER_AXIS_TRIGGERLEFT || index == SDL_CONTROLLER_AXIS_TRIGGERRIGHT))
        report[index] = LE_WORD(value);
    else
        report[index] = LE_WORD(value) + 32768;
}

static void set_ball_value(struct platform_private *ext, int index, int value1, int value2)
{
    int offset;
    offset = ext->ball_start + (index * sizeof(WORD));
    if (value1 > 127) value1 = 127;
    if (value1 < -127) value1 = -127;
    if (value2 > 127) value2 = 127;
    if (value2 < -127) value2 = -127;
    *((WORD*)&ext->report_buffer[offset]) = LE_WORD(value1);
    *((WORD*)&ext->report_buffer[offset + sizeof(WORD)]) = LE_WORD(value2);
}

static void set_hat_value(struct platform_private *ext, int index, int value)
{
    int byte = ext->button_start + (ext->hat_bit_offs + 4 * index) / 8;
    int bit_offs = (ext->hat_bit_offs + 4 * index) % 8;
    int num_low_bits, num_high_bits;
    unsigned char val, low_mask, high_mask;

    /* 4-bit hatswitch value is packed into button bytes */
    if (bit_offs <= 4)
    {
        num_low_bits = 4;
        num_high_bits = 0;
        low_mask = 0xf;
        high_mask = 0;
    }
    else
    {
        num_low_bits = 8 - bit_offs;
        num_high_bits = 4 - num_low_bits;
        low_mask = (1 << num_low_bits) - 1;
        high_mask = (1 << num_high_bits) - 1;
    }

    switch (value)
    {
        /* 8 1 2
         * 7 0 3
         * 6 5 4 */
        case SDL_HAT_CENTERED: val = 0; break;
        case SDL_HAT_UP: val = 1; break;
        case SDL_HAT_RIGHTUP: val = 2; break;
        case SDL_HAT_RIGHT: val = 3; break;
        case SDL_HAT_RIGHTDOWN: val = 4; break;
        case SDL_HAT_DOWN: val = 5; break;
        case SDL_HAT_LEFTDOWN: val = 6; break;
        case SDL_HAT_LEFT: val = 7; break;
        case SDL_HAT_LEFTUP: val = 8; break;
        default: return;
    }

    ext->report_buffer[byte] &= ~(low_mask << bit_offs);
    ext->report_buffer[byte] |= (val & low_mask) << bit_offs;
    if (high_mask)
    {
        ext->report_buffer[byte + 1] &= ~high_mask;
        ext->report_buffer[byte + 1] |= val & high_mask;
    }
}

static int test_haptic(struct platform_private *ext)
{
    int rc = 0;
    if (pSDL_JoystickIsHaptic(ext->sdl_joystick))
    {
        ext->sdl_haptic = pSDL_HapticOpenFromJoystick(ext->sdl_joystick);
        if (ext->sdl_haptic &&
            ((pSDL_HapticQuery(ext->sdl_haptic) & SDL_HAPTIC_LEFTRIGHT) != 0 ||
             pSDL_HapticRumbleSupported(ext->sdl_haptic)))
        {
            pSDL_HapticStopAll(ext->sdl_haptic);
            pSDL_HapticRumbleInit(ext->sdl_haptic);
            rc = sizeof(HAPTIC_RUMBLE);
            ext->haptic_effect_id = -1;
        }
        else
        {
            pSDL_HapticClose(ext->sdl_haptic);
            ext->sdl_haptic = NULL;
        }
    }
    return rc;
}

static int build_haptic(struct platform_private *ext, BYTE *report_ptr)
{
    if (ext->sdl_haptic)
    {
        memcpy(report_ptr, HAPTIC_RUMBLE, sizeof(HAPTIC_RUMBLE));
        return (sizeof(HAPTIC_RUMBLE));
    }
    return 0;
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
        report_size += (sizeof(WORD) * axis_count);
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
        report_size += (sizeof(WORD) * 2 * ball_count);
    }

    /* For now lump all buttons just into incremental usages, Ignore Keys */
    button_count = pSDL_JoystickNumButtons(ext->sdl_joystick);
    ext->button_start = report_size;
    if (button_count)
    {
        descript_size += sizeof(REPORT_BUTTONS);
    }

    hat_count = pSDL_JoystickNumHats(ext->sdl_joystick);
    ext->hat_bit_offs = button_count;
    if (hat_count)
    {
        descript_size += sizeof(REPORT_HATSWITCH);
    }

    report_size += (button_count + hat_count * 4 + 7) / 8;

    descript_size += test_haptic(ext);

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
    if (button_count)
    {
        report_ptr = add_button_block(report_ptr, 1, button_count);
    }
    if (hat_count)
        report_ptr = add_hatswitch(report_ptr, hat_count);

    report_ptr += build_haptic(ext, report_ptr);
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
        set_axis_value(ext, i, pSDL_JoystickGetAxis(ext->sdl_joystick, i), FALSE);
    for (i = 0; i < hat_count; i++)
        set_hat_value(ext, i, pSDL_JoystickGetHat(ext->sdl_joystick, i));

    return TRUE;
}

static SHORT compose_dpad_value(SDL_GameController *joystick)
{
    if (pSDL_GameControllerGetButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_UP))
    {
        if (pSDL_GameControllerGetButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
            return SDL_HAT_RIGHTUP;
        if (pSDL_GameControllerGetButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
            return SDL_HAT_LEFTUP;
        return SDL_HAT_UP;
    }

    if (pSDL_GameControllerGetButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
    {
        if (pSDL_GameControllerGetButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
            return SDL_HAT_RIGHTDOWN;
        if (pSDL_GameControllerGetButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
            return SDL_HAT_LEFTDOWN;
        return SDL_HAT_DOWN;
    }

    if (pSDL_GameControllerGetButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
        return SDL_HAT_RIGHT;
    if (pSDL_GameControllerGetButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
        return SDL_HAT_LEFT;
    return SDL_HAT_CENTERED;
}

static BOOL build_mapped_report_descriptor(struct platform_private *ext)
{
    BYTE *report_ptr;
    INT i, descript_size;

    static const int BUTTON_BIT_COUNT = CONTROLLER_NUM_BUTTONS + CONTROLLER_NUM_HATSWITCHES * 4;

    descript_size = sizeof(REPORT_HEADER) + sizeof(REPORT_TAIL);
    descript_size += sizeof(CONTROLLER_AXIS);
    descript_size += sizeof(CONTROLLER_TRIGGERS);
    descript_size += sizeof(CONTROLLER_BUTTONS);
    descript_size += sizeof(REPORT_HATSWITCH);
    descript_size += sizeof(REPORT_PADDING);
    if (BUTTON_BIT_COUNT % 8 != 0)
        descript_size += sizeof(REPORT_PADDING);
    descript_size += test_haptic(ext);

    ext->axis_start = 0;
    ext->button_start = CONTROLLER_NUM_AXES * sizeof(WORD);
    ext->hat_bit_offs = CONTROLLER_NUM_BUTTONS;

    ext->buffer_length = (BUTTON_BIT_COUNT + 7) / 8
        + CONTROLLER_NUM_AXES * sizeof(WORD)
        + 2/* unknown constant*/;

    TRACE("Report Descriptor will be %i bytes\n", descript_size);
    TRACE("Report will be %i bytes\n", ext->buffer_length);

    ext->report_descriptor = HeapAlloc(GetProcessHeap(), 0, descript_size);
    if (!ext->report_descriptor)
    {
        ERR("Failed to alloc report descriptor\n");
        return FALSE;
    }
    report_ptr = ext->report_descriptor;

    memcpy(report_ptr, REPORT_HEADER, sizeof(REPORT_HEADER));
    report_ptr[IDX_HEADER_PAGE] = HID_USAGE_PAGE_GENERIC;
    report_ptr[IDX_HEADER_USAGE] = HID_USAGE_GENERIC_GAMEPAD;
    report_ptr += sizeof(REPORT_HEADER);
    memcpy(report_ptr, CONTROLLER_AXIS, sizeof(CONTROLLER_AXIS));
    report_ptr += sizeof(CONTROLLER_AXIS);
    memcpy(report_ptr, CONTROLLER_TRIGGERS, sizeof(CONTROLLER_TRIGGERS));
    report_ptr += sizeof(CONTROLLER_TRIGGERS);
    memcpy(report_ptr, CONTROLLER_BUTTONS, sizeof(CONTROLLER_BUTTONS));
    report_ptr += sizeof(CONTROLLER_BUTTONS);
    report_ptr = add_hatswitch(report_ptr, 1);
    if (BUTTON_BIT_COUNT % 8 != 0)
        report_ptr = add_padding_block(report_ptr, 8 - (BUTTON_BIT_COUNT % 8));/* unused bits between hatswitch and following constant */
    report_ptr = add_padding_block(report_ptr, 16);/* unknown constant */
    report_ptr += build_haptic(ext, report_ptr);
    memcpy(report_ptr, REPORT_TAIL, sizeof(REPORT_TAIL));

    ext->report_descriptor_size = descript_size;
    ext->report_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ext->buffer_length);
    if (ext->report_buffer == NULL)
    {
        ERR("Failed to alloc report buffer\n");
        HeapFree(GetProcessHeap(), 0, ext->report_descriptor);
        return FALSE;
    }

    /* Initialize axis in the report */
    for (i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++)
        set_axis_value(ext, i, pSDL_GameControllerGetAxis(ext->sdl_controller, i), TRUE);

    set_hat_value(ext, 0, compose_dpad_value(ext->sdl_controller));

    /* unknown constant */
    ext->report_buffer[14] = 0x89;
    ext->report_buffer[15] = 0xc5;

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
            if (ext->sdl_controller)
                str = pSDL_GameControllerName(ext->sdl_controller);
            else
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
    struct platform_private *ext = impl_from_DEVICE_OBJECT(device);

    if (ext->sdl_haptic && id == 0)
    {
        WORD left = report[2] * 128;
        WORD right = report[3] * 128;

        if (ext->haptic_effect_id >= 0)
        {
            pSDL_HapticDestroyEffect(ext->sdl_haptic, ext->haptic_effect_id);
            ext->haptic_effect_id = -1;
        }
        pSDL_HapticStopAll(ext->sdl_haptic);
        if (left != 0 || right != 0)
        {
            SDL_HapticEffect effect;

            pSDL_memset( &effect, 0, sizeof(SDL_HapticEffect) );
            effect.type = SDL_HAPTIC_LEFTRIGHT;
            effect.leftright.length = -1;
            effect.leftright.large_magnitude = left;
            effect.leftright.small_magnitude = right;

            ext->haptic_effect_id = pSDL_HapticNewEffect(ext->sdl_haptic, &effect);
            if (ext->haptic_effect_id >= 0)
            {
                pSDL_HapticRunEffect(ext->sdl_haptic, ext->haptic_effect_id, 1);
            }
            else
            {
                float i = (float)((left + right)/2.0) / 32767.0;
                pSDL_HapticRumblePlay(ext->sdl_haptic, i, -1);
            }
        }
        *written = length;
        return STATUS_SUCCESS;
    }
    else
    {
        *written = 0;
        return STATUS_NOT_IMPLEMENTED;
    }
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

static int compare_joystick_id(DEVICE_OBJECT *device, void* context)
{
    return impl_from_DEVICE_OBJECT(device)->id - PtrToUlong(context);
}

static BOOL set_report_from_event(SDL_Event *event)
{
    DEVICE_OBJECT *device;
    struct platform_private *private;
    /* All the events coming in will have 'which' as a 3rd field */
    SDL_JoystickID id = ((SDL_JoyButtonEvent*)event)->which;
    device = bus_enumerate_hid_devices(&sdl_vtbl, compare_joystick_id, ULongToPtr(id));
    if (!device)
    {
        ERR("Failed to find device at index %i\n",id);
        return FALSE;
    }
    private = impl_from_DEVICE_OBJECT(device);
    if (private->sdl_controller)
    {
        /* We want mapped events */
        return TRUE;
    }

    switch(event->type)
    {
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        {
            SDL_JoyButtonEvent *ie = &event->jbutton;

            set_button_value(private, ie->button, ie->state);

            process_hid_report(device, private->report_buffer, private->buffer_length);
            break;
        }
        case SDL_JOYAXISMOTION:
        {
            SDL_JoyAxisEvent *ie = &event->jaxis;

            if (ie->axis < 6)
            {
                set_axis_value(private, ie->axis, ie->value, FALSE);
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
            ERR("TODO: Process Report (0x%x)\n",event->type);
    }
    return FALSE;
}

static BOOL set_mapped_report_from_event(SDL_Event *event)
{
    DEVICE_OBJECT *device;
    struct platform_private *private;
    /* All the events coming in will have 'which' as a 3rd field */
    SDL_JoystickID id = ((SDL_ControllerButtonEvent*)event)->which;
    device = bus_enumerate_hid_devices(&sdl_vtbl, compare_joystick_id, ULongToPtr(id));
    if (!device)
    {
        ERR("Failed to find device at index %i\n",id);
        return FALSE;
    }
    private = impl_from_DEVICE_OBJECT(device);

    switch(event->type)
    {
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        {
            int usage = -1;
            SDL_ControllerButtonEvent *ie = &event->cbutton;

            switch (ie->button)
            {
                case SDL_CONTROLLER_BUTTON_A: usage = 0; break;
                case SDL_CONTROLLER_BUTTON_B: usage = 1; break;
                case SDL_CONTROLLER_BUTTON_X: usage = 2; break;
                case SDL_CONTROLLER_BUTTON_Y: usage = 3; break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: usage = 4; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: usage = 5; break;
                case SDL_CONTROLLER_BUTTON_BACK: usage = 6; break;
                case SDL_CONTROLLER_BUTTON_START: usage = 7; break;
                case SDL_CONTROLLER_BUTTON_LEFTSTICK: usage = 8; break;
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK: usage = 9; break;
                case SDL_CONTROLLER_BUTTON_GUIDE: usage = 10; break;

                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    set_hat_value(private, 0, compose_dpad_value(private->sdl_controller));
                    process_hid_report(device, private->report_buffer, private->buffer_length);
                    break;

                default:
                    ERR("Unknown Button %i\n",ie->button);
            }

            if (usage >= 0)
            {
                set_button_value(private, usage, ie->state);
                process_hid_report(device, private->report_buffer, private->buffer_length);
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION:
        {
            SDL_ControllerAxisEvent *ie = &event->caxis;

            set_axis_value(private, ie->axis, ie->value, TRUE);
            process_hid_report(device, private->report_buffer, private->buffer_length);
            break;
        }
        default:
            ERR("TODO: Process Report (%x)\n",event->type);
    }
    return FALSE;
}

static void try_remove_device(SDL_JoystickID id)
{
    DEVICE_OBJECT *device = NULL;
    struct platform_private *private;
    SDL_Joystick *sdl_joystick;
    SDL_GameController *sdl_controller;
    SDL_Haptic *sdl_haptic;

    device = bus_enumerate_hid_devices(&sdl_vtbl, compare_joystick_id, ULongToPtr(id));
    if (!device) return;

    private = impl_from_DEVICE_OBJECT(device);
    sdl_joystick = private->sdl_joystick;
    sdl_controller = private->sdl_controller;
    sdl_haptic = private->sdl_haptic;

    bus_unlink_hid_device(device);
    IoInvalidateDeviceRelations(bus_pdo, BusRelations);

    bus_remove_hid_device(device);

    pSDL_JoystickClose(sdl_joystick);
    if (sdl_controller)
        pSDL_GameControllerClose(sdl_controller);
    if (sdl_haptic)
        pSDL_HapticClose(sdl_haptic);
}

static void try_add_device(unsigned int index)
{
    DWORD vid = 0, pid = 0, version = 0;
    DEVICE_OBJECT *device = NULL;
    WCHAR serial[34] = {0};
    char guid_str[34];
    BOOL is_xbox_gamepad;
    WORD input = -1;

    SDL_Joystick* joystick;
    SDL_JoystickID id;
    SDL_JoystickGUID guid;
    SDL_GameController *controller = NULL;

    if ((joystick = pSDL_JoystickOpen(index)) == NULL)
    {
        WARN("Unable to open sdl device %i: %s\n", index, pSDL_GetError());
        return;
    }

    if (map_controllers && pSDL_IsGameController(index))
        controller = pSDL_GameControllerOpen(index);

    id = pSDL_JoystickInstanceID(joystick);

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

    if (controller)
    {
        TRACE("Found sdl game controller %i (vid %04x, pid %04x, version %u, serial %s)\n",
              id, vid, pid, version, debugstr_w(serial));
        is_xbox_gamepad = TRUE;
    }
    else
    {
        int button_count, axis_count;

        TRACE("Found sdl device %i (vid %04x, pid %04x, version %u, serial %s)\n",
              id, vid, pid, version, debugstr_w(serial));

        axis_count = pSDL_JoystickNumAxes(joystick);
        button_count = pSDL_JoystickNumButtons(joystick);
        is_xbox_gamepad = (axis_count == 6  && button_count >= 14);
    }
    if (is_xbox_gamepad)
        input = 0;

    device = bus_create_hid_device(sdl_busidW, vid, pid, input, version, index,
            serial, is_xbox_gamepad, &sdl_vtbl, sizeof(struct platform_private));

    if (device)
    {
        BOOL rc;
        struct platform_private *private = impl_from_DEVICE_OBJECT(device);
        private->sdl_joystick = joystick;
        private->sdl_controller = controller;
        private->id = id;
        if (controller)
            rc = build_mapped_report_descriptor(private);
        else
            rc = build_report_descriptor(private);
        if (!rc)
        {
            ERR("Building report descriptor failed, removing device\n");
            bus_unlink_hid_device(device);
            bus_remove_hid_device(device);
            HeapFree(GetProcessHeap(), 0, serial);
            return;
        }
        IoInvalidateDeviceRelations(bus_pdo, BusRelations);
    }
    else
    {
        WARN("Ignoring device %i\n", id);
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
    else if (event->type >= SDL_CONTROLLERAXISMOTION && event->type <= SDL_CONTROLLERBUTTONUP)
        set_mapped_report_from_event(event);
}

static DWORD CALLBACK deviceloop_thread(void *args)
{
    HANDLE init_done = args;
    SDL_Event event;

    if (pSDL_Init(SDL_INIT_GAMECONTROLLER|SDL_INIT_HAPTIC) < 0)
    {
        ERR("Can't init SDL: %s\n", pSDL_GetError());
        return STATUS_UNSUCCESSFUL;
    }

    pSDL_JoystickEventState(SDL_ENABLE);
    pSDL_GameControllerEventState(SDL_ENABLE);

    /* Process mappings */
    if (pSDL_GameControllerAddMapping != NULL)
    {
        HKEY key;
        static const WCHAR szPath[] = {'m','a','p',0};
        const char *mapping;

        if ((mapping = getenv("SDL_GAMECONTROLLERCONFIG")))
        {
            TRACE("Setting environment mapping %s\n", debugstr_a(mapping));
            if (pSDL_GameControllerAddMapping(mapping) < 0)
                WARN("Failed to add environment mapping %s\n", pSDL_GetError());
        }
        else if (!RegOpenKeyExW(driver_key, szPath, 0, KEY_QUERY_VALUE, &key))
        {
            DWORD index = 0;
            CHAR *buffer = NULL;
            DWORD buffer_len = 0;
            LSTATUS rc;

            do {
                CHAR name[255];
                DWORD name_len;
                DWORD type;
                DWORD data_len = buffer_len;

                name_len = sizeof(name);
                rc = RegEnumValueA(key, index, name, &name_len, NULL, &type, (LPBYTE)buffer, &data_len);
                if (rc == ERROR_MORE_DATA || buffer == NULL)
                {
                    if (buffer)
                        buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, data_len);
                    else
                        buffer = HeapAlloc(GetProcessHeap(), 0, data_len);
                    buffer_len = data_len;

                    name_len = sizeof(name);
                    rc = RegEnumValueA(key, index, name, &name_len, NULL, &type, (LPBYTE)buffer, &data_len);
                }

                if (rc == STATUS_SUCCESS)
                {
                    TRACE("Setting registry mapping %s\n", debugstr_a(buffer));
                    if (pSDL_GameControllerAddMapping(buffer) < 0)
                        WARN("Failed to add registry mapping %s\n", pSDL_GetError());
                    index ++;
                }
            } while (rc == STATUS_SUCCESS);
            HeapFree(GetProcessHeap(), 0, buffer);
            NtClose(key);
        }
    }

    SetEvent(init_done);

    while (1) {
        while (pSDL_WaitEvent(&event) != 0) {
            if (event.type == quit_event) {
                TRACE("Device thread exiting\n");
                return 0;
            }
            process_device_event(&event);
        }
    }
}

void sdl_driver_unload( void )
{
    SDL_Event event;

    TRACE("Unload Driver\n");

    if (!deviceloop_handle)
        return;

    quit_event = pSDL_RegisterEvents(1);
    if (quit_event == -1) {
        ERR("error registering quit event\n");
        return;
    }

    event.type = quit_event;
    if (pSDL_PushEvent(&event) != 1) {
        ERR("error pushing quit event\n");
        return;
    }

    WaitForSingleObject(deviceloop_handle, INFINITE);
    CloseHandle(deviceloop_handle);
    dlclose(sdl_handle);
}

NTSTATUS sdl_driver_init(void)
{
    static const WCHAR controller_modeW[] = {'M','a','p',' ','C','o','n','t','r','o','l','l','e','r','s',0};
    static const UNICODE_STRING controller_mode = {sizeof(controller_modeW) - sizeof(WCHAR), sizeof(controller_modeW), (WCHAR*)controller_modeW};

    HANDLE events[2];
    DWORD result;

    if (sdl_handle == NULL)
    {
        sdl_handle = dlopen(SONAME_LIBSDL2, RTLD_NOW);
        if (!sdl_handle) {
            WARN("could not load %s\n", SONAME_LIBSDL2);
            return STATUS_UNSUCCESSFUL;
        }
#define LOAD_FUNCPTR(f) if((p##f = dlsym(sdl_handle, #f)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}
        LOAD_FUNCPTR(SDL_GetError);
        LOAD_FUNCPTR(SDL_Init);
        LOAD_FUNCPTR(SDL_JoystickClose);
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
        LOAD_FUNCPTR(SDL_IsGameController);
        LOAD_FUNCPTR(SDL_GameControllerClose);
        LOAD_FUNCPTR(SDL_GameControllerGetAxis);
        LOAD_FUNCPTR(SDL_GameControllerGetButton);
        LOAD_FUNCPTR(SDL_GameControllerName);
        LOAD_FUNCPTR(SDL_GameControllerOpen);
        LOAD_FUNCPTR(SDL_GameControllerEventState);
        LOAD_FUNCPTR(SDL_HapticClose);
        LOAD_FUNCPTR(SDL_HapticDestroyEffect);
        LOAD_FUNCPTR(SDL_HapticNewEffect);
        LOAD_FUNCPTR(SDL_HapticOpenFromJoystick);
        LOAD_FUNCPTR(SDL_HapticQuery);
        LOAD_FUNCPTR(SDL_HapticRumbleInit);
        LOAD_FUNCPTR(SDL_HapticRumblePlay);
        LOAD_FUNCPTR(SDL_HapticRumbleSupported);
        LOAD_FUNCPTR(SDL_HapticRunEffect);
        LOAD_FUNCPTR(SDL_HapticStopAll);
        LOAD_FUNCPTR(SDL_JoystickIsHaptic);
        LOAD_FUNCPTR(SDL_memset);
        LOAD_FUNCPTR(SDL_GameControllerAddMapping);
        LOAD_FUNCPTR(SDL_RegisterEvents);
        LOAD_FUNCPTR(SDL_PushEvent);
#undef LOAD_FUNCPTR
        pSDL_JoystickGetProduct = dlsym(sdl_handle, "SDL_JoystickGetProduct");
        pSDL_JoystickGetProductVersion = dlsym(sdl_handle, "SDL_JoystickGetProductVersion");
        pSDL_JoystickGetVendor = dlsym(sdl_handle, "SDL_JoystickGetVendor");
    }

    map_controllers = check_bus_option(&controller_mode, 1);

    if (!(events[0] = CreateEventW(NULL, TRUE, FALSE, NULL)))
    {
        WARN("CreateEvent failed\n");
        return STATUS_UNSUCCESSFUL;
    }
    if (!(events[1] = CreateThread(NULL, 0, deviceloop_thread, events[0], 0, NULL)))
    {
        WARN("CreateThread failed\n");
        CloseHandle(events[0]);
        return STATUS_UNSUCCESSFUL;
    }

    result = WaitForMultipleObjects(2, events, FALSE, INFINITE);
    CloseHandle(events[0]);
    if (result == WAIT_OBJECT_0)
    {
        TRACE("Initialization successful\n");
        deviceloop_handle = events[1];
        return STATUS_SUCCESS;
    }
    CloseHandle(events[1]);

sym_not_found:
    dlclose(sdl_handle);
    sdl_handle = NULL;
    return STATUS_UNSUCCESSFUL;
}

#else

NTSTATUS sdl_driver_init(void)
{
    return STATUS_NOT_IMPLEMENTED;
}

void sdl_driver_unload( void )
{
    TRACE("Stub: Unload Driver\n");
}

#endif /* SONAME_LIBSDL2 */
