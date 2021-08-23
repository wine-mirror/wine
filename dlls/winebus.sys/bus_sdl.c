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

#ifdef WORDS_BIGENDIAN
# define LE_WORD(x) RtlUshortByteSwap(x)
#else
# define LE_WORD(x) (x)
#endif

#include "bus.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef SONAME_LIBSDL2

WINE_DECLARE_DEBUG_CHANNEL(hid_report);

static const WCHAR sdl_busidW[] = {'S','D','L','J','O','Y',0};
static struct sdl_bus_options options;

static void *sdl_handle = NULL;
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
    struct unix_device unix_device;

    SDL_Joystick *sdl_joystick;
    SDL_GameController *sdl_controller;
    SDL_JoystickID id;

    int button_start;
    int axis_start;
    int ball_start;
    int hat_bit_offs; /* hatswitches are reported in the same bytes as buttons */

    struct hid_descriptor desc;

    int buffer_length;
    BYTE *report_buffer;

    SDL_Haptic *sdl_haptic;
    int haptic_effect_id;
};

static inline struct platform_private *impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(iface, struct platform_private, unix_device);
}

static inline struct platform_private *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return impl_from_unix_device(get_unix_device(device));
}

#define CONTROLLER_NUM_BUTTONS 11
#define CONTROLLER_NUM_AXES 6
#define CONTROLLER_NUM_HATSWITCHES 1

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

static BOOL descriptor_add_haptic(struct platform_private *ext)
{
    if (pSDL_JoystickIsHaptic(ext->sdl_joystick))
    {
        ext->sdl_haptic = pSDL_HapticOpenFromJoystick(ext->sdl_joystick);
        if (ext->sdl_haptic &&
            ((pSDL_HapticQuery(ext->sdl_haptic) & SDL_HAPTIC_LEFTRIGHT) != 0 ||
             pSDL_HapticRumbleSupported(ext->sdl_haptic)))
        {
            pSDL_HapticStopAll(ext->sdl_haptic);
            pSDL_HapticRumbleInit(ext->sdl_haptic);
            if (!hid_descriptor_add_haptics(&ext->desc))
                return FALSE;
            ext->haptic_effect_id = -1;
        }
        else
        {
            pSDL_HapticClose(ext->sdl_haptic);
            ext->sdl_haptic = NULL;
        }
    }

    return TRUE;
}

static NTSTATUS build_report_descriptor(struct platform_private *ext)
{
    INT i;
    INT report_size;
    INT button_count, axis_count, ball_count, hat_count;
    static const USAGE device_usage[2] = {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD};
    static const USAGE controller_usages[] = {
        HID_USAGE_GENERIC_X,
        HID_USAGE_GENERIC_Y,
        HID_USAGE_GENERIC_Z,
        HID_USAGE_GENERIC_RX,
        HID_USAGE_GENERIC_RY,
        HID_USAGE_GENERIC_RZ,
        HID_USAGE_GENERIC_SLIDER,
        HID_USAGE_GENERIC_DIAL,
        HID_USAGE_GENERIC_WHEEL};
    static const USAGE joystick_usages[] = {
        HID_USAGE_GENERIC_X,
        HID_USAGE_GENERIC_Y,
        HID_USAGE_GENERIC_Z,
        HID_USAGE_GENERIC_RZ,
        HID_USAGE_GENERIC_RX,
        HID_USAGE_GENERIC_RY,
        HID_USAGE_GENERIC_SLIDER,
        HID_USAGE_GENERIC_DIAL,
        HID_USAGE_GENERIC_WHEEL};

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
        report_size += (sizeof(WORD) * 2 * ball_count);
    }

    /* For now lump all buttons just into incremental usages, Ignore Keys */
    button_count = pSDL_JoystickNumButtons(ext->sdl_joystick);
    ext->button_start = report_size;

    hat_count = pSDL_JoystickNumHats(ext->sdl_joystick);
    ext->hat_bit_offs = button_count;

    report_size += (button_count + hat_count * 4 + 7) / 8;

    TRACE("Report will be %i bytes\n", report_size);

    if (!hid_descriptor_begin(&ext->desc, device_usage[0], device_usage[1]))
        return STATUS_NO_MEMORY;

    if (axis_count == 6 && button_count >= 14)
    {
        if (!hid_descriptor_add_axes(&ext->desc, axis_count, HID_USAGE_PAGE_GENERIC,
                                     controller_usages, FALSE, 16, 0, 0xffff))
            return STATUS_NO_MEMORY;
    }
    else if (axis_count)
    {
        if (!hid_descriptor_add_axes(&ext->desc, axis_count, HID_USAGE_PAGE_GENERIC,
                                     joystick_usages, FALSE, 16, 0, 0xffff))
            return STATUS_NO_MEMORY;
    }

    if (ball_count && !hid_descriptor_add_axes(&ext->desc, ball_count * 2, HID_USAGE_PAGE_GENERIC,
                                               &joystick_usages[axis_count], TRUE, 8, 0x81, 0x7f))
        return STATUS_NO_MEMORY;

    if (button_count && !hid_descriptor_add_buttons(&ext->desc, HID_USAGE_PAGE_BUTTON, 1, button_count))
        return STATUS_NO_MEMORY;

    if (hat_count && !hid_descriptor_add_hatswitch(&ext->desc, hat_count))
        return STATUS_NO_MEMORY;

    if (!descriptor_add_haptic(ext))
        return STATUS_NO_MEMORY;

    if (!hid_descriptor_end(&ext->desc))
        return STATUS_NO_MEMORY;

    ext->buffer_length = report_size;
    if (!(ext->report_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, report_size)))
        goto failed;

    /* Initialize axis in the report */
    for (i = 0; i < axis_count; i++)
        set_axis_value(ext, i, pSDL_JoystickGetAxis(ext->sdl_joystick, i), FALSE);
    for (i = 0; i < hat_count; i++)
        set_hat_value(ext, i, pSDL_JoystickGetHat(ext->sdl_joystick, i));

    return STATUS_SUCCESS;

failed:
    HeapFree(GetProcessHeap(), 0, ext->report_buffer);
    hid_descriptor_free(&ext->desc);
    return STATUS_NO_MEMORY;
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

static NTSTATUS build_mapped_report_descriptor(struct platform_private *ext)
{
    static const USAGE left_axis_usages[] = {HID_USAGE_GENERIC_X, HID_USAGE_GENERIC_Y};
    static const USAGE right_axis_usages[] = {HID_USAGE_GENERIC_RX, HID_USAGE_GENERIC_RY};
    static const USAGE trigger_axis_usages[] = {HID_USAGE_GENERIC_Z, HID_USAGE_GENERIC_RZ};
    INT i;

    static const int BUTTON_BIT_COUNT = CONTROLLER_NUM_BUTTONS + CONTROLLER_NUM_HATSWITCHES * 4;

    ext->axis_start = 0;
    ext->button_start = CONTROLLER_NUM_AXES * sizeof(WORD);
    ext->hat_bit_offs = CONTROLLER_NUM_BUTTONS;

    ext->buffer_length = (BUTTON_BIT_COUNT + 7) / 8
        + CONTROLLER_NUM_AXES * sizeof(WORD)
        + 2/* unknown constant*/;

    TRACE("Report will be %i bytes\n", ext->buffer_length);

    if (!hid_descriptor_begin(&ext->desc, HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD))
        return STATUS_NO_MEMORY;

    if (!hid_descriptor_add_axes(&ext->desc, 2, HID_USAGE_PAGE_GENERIC, left_axis_usages,
                                 FALSE, 16, 0, 0xffff))
        return STATUS_NO_MEMORY;

    if (!hid_descriptor_add_axes(&ext->desc, 2, HID_USAGE_PAGE_GENERIC, right_axis_usages,
                                 FALSE, 16, 0, 0xffff))
        return STATUS_NO_MEMORY;

    if (!hid_descriptor_add_axes(&ext->desc, 2, HID_USAGE_PAGE_GENERIC, trigger_axis_usages,
                                 FALSE, 16, 0, 0x7fff))
        return STATUS_NO_MEMORY;

    if (!hid_descriptor_add_buttons(&ext->desc, HID_USAGE_PAGE_BUTTON, 1, CONTROLLER_NUM_BUTTONS))
        return STATUS_NO_MEMORY;

    if (!hid_descriptor_add_hatswitch(&ext->desc, 1))
        return STATUS_NO_MEMORY;

    if (BUTTON_BIT_COUNT % 8 != 0)
    {
        /* unused bits between hatswitch and following constant */
        if (!hid_descriptor_add_padding(&ext->desc, 8 - (BUTTON_BIT_COUNT % 8)))
            return STATUS_NO_MEMORY;
    }

    /* unknown constant */
    if (!hid_descriptor_add_padding(&ext->desc, 16))
        return STATUS_NO_MEMORY;

    if (!descriptor_add_haptic(ext))
        return STATUS_NO_MEMORY;

    if (!hid_descriptor_end(&ext->desc))
        return STATUS_NO_MEMORY;

    if (!(ext->report_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ext->buffer_length)))
        goto failed;

    /* Initialize axis in the report */
    for (i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++)
        set_axis_value(ext, i, pSDL_GameControllerGetAxis(ext->sdl_controller, i), TRUE);

    set_hat_value(ext, 0, compose_dpad_value(ext->sdl_controller));

    /* unknown constant */
    ext->report_buffer[14] = 0x89;
    ext->report_buffer[15] = 0xc5;

    return STATUS_SUCCESS;

failed:
    HeapFree(GetProcessHeap(), 0, ext->report_buffer);
    hid_descriptor_free(&ext->desc);
    return STATUS_NO_MEMORY;
}

static void free_device(DEVICE_OBJECT *device)
{
    struct platform_private *ext = impl_from_DEVICE_OBJECT(device);

    pSDL_JoystickClose(ext->sdl_joystick);
    if (ext->sdl_controller)
        pSDL_GameControllerClose(ext->sdl_controller);
    if (ext->sdl_haptic)
        pSDL_HapticClose(ext->sdl_haptic);

    HeapFree(GetProcessHeap(), 0, ext);
}

static int compare_platform_device(DEVICE_OBJECT *device, void *context)
{
    return impl_from_DEVICE_OBJECT(device)->id - PtrToUlong(context);
}

static NTSTATUS start_device(DEVICE_OBJECT *device)
{
    struct platform_private *ext = impl_from_DEVICE_OBJECT(device);
    if (ext->sdl_controller) return build_mapped_report_descriptor(ext);
    return build_report_descriptor(ext);
}

static NTSTATUS get_reportdescriptor(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length)
{
    struct platform_private *ext = impl_from_DEVICE_OBJECT(device);

    *out_length = ext->desc.size;
    if (length < ext->desc.size) return STATUS_BUFFER_TOO_SMALL;

    memcpy(buffer, ext->desc.data, ext->desc.size);
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

static void set_output_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct platform_private *ext = impl_from_DEVICE_OBJECT(device);

    if (ext->sdl_haptic && packet->reportId == 0)
    {
        WORD left = packet->reportBuffer[2] * 128;
        WORD right = packet->reportBuffer[3] * 128;

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

        io->Information = packet->reportBufferLen;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        io->Information = 0;
        io->Status = STATUS_NOT_IMPLEMENTED;
    }
}

static void get_feature_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
}

static void set_feature_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    io->Information = 0;
    io->Status = STATUS_NOT_IMPLEMENTED;
}

static const platform_vtbl sdl_vtbl =
{
    free_device,
    compare_platform_device,
    start_device,
    get_reportdescriptor,
    get_string,
    set_output_report,
    get_feature_report,
    set_feature_report,
};

static BOOL set_report_from_event(DEVICE_OBJECT *device, SDL_Event *event)
{
    struct platform_private *private;
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

static BOOL set_mapped_report_from_event(DEVICE_OBJECT *device, SDL_Event *event)
{
    struct platform_private *private;
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

static void try_remove_device(DEVICE_OBJECT *device)
{
    bus_unlink_hid_device(device);
    IoInvalidateDeviceRelations(bus_pdo, BusRelations);
}

static void try_add_device(unsigned int index)
{
    DWORD vid = 0, pid = 0, version = 0;
    struct platform_private *private;
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

    if (options.map_controllers && pSDL_IsGameController(index))
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

    if (!(private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*private)))) return;

    device = bus_create_hid_device(sdl_busidW, vid, pid, input, version, index, serial, is_xbox_gamepad,
                                   &sdl_vtbl, &private->unix_device);
    if (!device) HeapFree(GetProcessHeap(), 0, private);
    else
    {
        private->sdl_joystick = joystick;
        private->sdl_controller = controller;
        private->id = id;
        IoInvalidateDeviceRelations(bus_pdo, BusRelations);
    }
}

static void process_device_event(SDL_Event *event)
{
    DEVICE_OBJECT *device;
    SDL_JoystickID id;

    TRACE_(hid_report)("Received action %x\n", event->type);

    if (event->type == SDL_JOYDEVICEADDED)
        try_add_device(((SDL_JoyDeviceEvent*)event)->which);
    else if (event->type == SDL_JOYDEVICEREMOVED)
    {
        id = ((SDL_JoyDeviceEvent *)event)->which;
        device = bus_find_hid_device(sdl_busidW, ULongToPtr(id));
        if (device) try_remove_device(device);
        else WARN("failed to find device with id %d\n", id);
    }
    else if (event->type >= SDL_JOYAXISMOTION && event->type <= SDL_JOYBUTTONUP)
    {
        id = ((SDL_JoyButtonEvent *)event)->which;
        device = bus_find_hid_device(sdl_busidW, ULongToPtr(id));
        if (device) set_report_from_event(device, event);
        else WARN("failed to find device with id %d\n", id);
    }
    else if (event->type >= SDL_CONTROLLERAXISMOTION && event->type <= SDL_CONTROLLERBUTTONUP)
    {
        id = ((SDL_ControllerButtonEvent *)event)->which;
        device = bus_find_hid_device(sdl_busidW, ULongToPtr(id));
        if (device) set_mapped_report_from_event(device, event);
        else WARN("failed to find device with id %d\n", id);
    }
}

static void sdl_load_mappings(void)
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

        do
        {
            CHAR name[255];
            DWORD name_len;
            DWORD type;
            DWORD data_len = buffer_len;

            name_len = sizeof(name);
            rc = RegEnumValueA(key, index, name, &name_len, NULL, &type, (LPBYTE)buffer, &data_len);
            if (rc == ERROR_MORE_DATA || buffer == NULL)
            {
                if (buffer) buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, data_len);
                else buffer = HeapAlloc(GetProcessHeap(), 0, data_len);
                buffer_len = data_len;

                name_len = sizeof(name);
                rc = RegEnumValueA(key, index, name, &name_len, NULL, &type, (LPBYTE)buffer, &data_len);
            }

            if (rc == STATUS_SUCCESS)
            {
                TRACE("Setting registry mapping %s\n", debugstr_a(buffer));
                if (pSDL_GameControllerAddMapping(buffer) < 0)
                    WARN("Failed to add registry mapping %s\n", pSDL_GetError());
                index++;
            }
        } while (rc == STATUS_SUCCESS);
        HeapFree(GetProcessHeap(), 0, buffer);
        NtClose(key);
    }
}

NTSTATUS sdl_bus_init(void *args)
{
    TRACE("args %p\n", args);

    options = *(struct sdl_bus_options *)args;

    if (!(sdl_handle = dlopen(SONAME_LIBSDL2, RTLD_NOW)))
    {
        WARN("could not load %s\n", SONAME_LIBSDL2);
        return STATUS_UNSUCCESSFUL;
    }
#define LOAD_FUNCPTR(f)                          \
    if ((p##f = dlsym(sdl_handle, #f)) == NULL)  \
    {                                            \
        WARN("could not find symbol %s\n", #f);  \
        goto failed;                             \
    }
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

    if (pSDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0)
    {
        ERR("could not init SDL: %s\n", pSDL_GetError());
        goto failed;
    }

    if ((quit_event = pSDL_RegisterEvents(1)) == -1)
    {
        ERR("error registering quit event\n");
        goto failed;
    }

    pSDL_JoystickEventState(SDL_ENABLE);
    pSDL_GameControllerEventState(SDL_ENABLE);

    /* Process mappings */
    if (pSDL_GameControllerAddMapping != NULL) sdl_load_mappings();

    return STATUS_SUCCESS;

failed:
    dlclose(sdl_handle);
    sdl_handle = NULL;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS sdl_bus_wait(void *args)
{
    SDL_Event event;

    do
    {
        if (pSDL_WaitEvent(&event) != 0) process_device_event(&event);
        else WARN("SDL_WaitEvent failed: %s\n", pSDL_GetError());
    } while (event.type != quit_event);

    TRACE("SDL main loop exiting\n");
    dlclose(sdl_handle);
    sdl_handle = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS sdl_bus_stop(void *args)
{
    SDL_Event event;

    if (!sdl_handle) return STATUS_SUCCESS;

    event.type = quit_event;
    if (pSDL_PushEvent(&event) != 1)
    {
        ERR("error pushing quit event\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

#else

NTSTATUS sdl_bus_init(void *args)
{
    WARN("SDL support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS sdl_bus_wait(void *args)
{
    WARN("SDL support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS sdl_bus_stop(void *args)
{
    WARN("SDL support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBSDL2 */
