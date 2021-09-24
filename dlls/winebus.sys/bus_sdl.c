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

#if 0
#pragma makedep unix
#endif

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

#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/hidtypes.h"
#include "ddk/hidsdi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "hidusage.h"

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef SONAME_LIBSDL2

WINE_DECLARE_DEBUG_CHANNEL(hid_report);

static pthread_mutex_t sdl_cs = PTHREAD_MUTEX_INITIALIZER;
static struct sdl_bus_options options;

static void *sdl_handle = NULL;
static UINT quit_event = -1;
static struct list event_queue = LIST_INIT(event_queue);
static struct list device_list = LIST_INIT(device_list);

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
MAKE_FUNCPTR(SDL_GameControllerAddMapping);
MAKE_FUNCPTR(SDL_RegisterEvents);
MAKE_FUNCPTR(SDL_PushEvent);
static int (*pSDL_JoystickRumble)(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms);
static Uint16 (*pSDL_JoystickGetProduct)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetProductVersion)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetVendor)(SDL_Joystick * joystick);

/* internal bits for extended rumble support, SDL_Haptic types are 16-bits */
#define WINE_SDL_JOYSTICK_RUMBLE  0x40000000 /* using SDL_JoystickRumble API */
#define WINE_SDL_HAPTIC_RUMBLE    0x80000000 /* using SDL_HapticRumble API */

#define EFFECT_SUPPORT_HAPTICS  (SDL_HAPTIC_LEFTRIGHT|WINE_SDL_HAPTIC_RUMBLE|WINE_SDL_JOYSTICK_RUMBLE)

struct sdl_device
{
    struct unix_device unix_device;

    SDL_Joystick *sdl_joystick;
    SDL_GameController *sdl_controller;
    SDL_JoystickID id;

    DWORD effect_support;
    SDL_Haptic *sdl_haptic;
    int haptic_effect_id;
};

static inline struct sdl_device *impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(iface, struct sdl_device, unix_device);
}

static struct sdl_device *find_device_from_id(SDL_JoystickID id)
{
    struct sdl_device *impl;

    LIST_FOR_EACH_ENTRY(impl, &device_list, struct sdl_device, unix_device.entry)
        if (impl->id == id) return impl;

    return NULL;
}

static void set_hat_value(struct unix_device *iface, int index, int value)
{
    LONG x = 0, y = 0;
    switch (value)
    {
    case SDL_HAT_CENTERED: break;
    case SDL_HAT_UP: y = 1; break;
    case SDL_HAT_RIGHTUP: y = x = 1; break;
    case SDL_HAT_RIGHT: x = 1; break;
    case SDL_HAT_RIGHTDOWN: x = 1; y = -1; break;
    case SDL_HAT_DOWN: y = -1; break;
    case SDL_HAT_LEFTDOWN: x = y = -1; break;
    case SDL_HAT_LEFT: x = -1; break;
    case SDL_HAT_LEFTUP: x = -1; y = 1; break;
    }
    hid_device_set_hatswitch_x(iface, index, x);
    hid_device_set_hatswitch_y(iface, index, y);
}

static BOOL descriptor_add_haptic(struct sdl_device *impl)
{
    unsigned int haptics_mask = SDL_HAPTIC_LEFTRIGHT;

    if (!pSDL_JoystickIsHaptic(impl->sdl_joystick) ||
        !(impl->sdl_haptic = pSDL_HapticOpenFromJoystick(impl->sdl_joystick)))
        impl->effect_support = 0;
    else
    {
        impl->effect_support = pSDL_HapticQuery(impl->sdl_haptic) & haptics_mask;
        if (pSDL_HapticRumbleSupported(impl->sdl_haptic))
            impl->effect_support |= WINE_SDL_HAPTIC_RUMBLE;

        pSDL_HapticStopAll(impl->sdl_haptic);
        pSDL_HapticRumbleInit(impl->sdl_haptic);
    }

    if (pSDL_JoystickRumble && !pSDL_JoystickRumble(impl->sdl_joystick, 0, 0, 0))
        impl->effect_support |= WINE_SDL_JOYSTICK_RUMBLE;

    if (impl->effect_support & EFFECT_SUPPORT_HAPTICS)
    {
        if (!hid_device_add_haptics(&impl->unix_device))
            return FALSE;
    }

    impl->haptic_effect_id = -1;
    return TRUE;
}

static NTSTATUS build_joystick_report_descriptor(struct unix_device *iface)
{
    static const USAGE joystick_usages[] =
    {
        HID_USAGE_GENERIC_X,
        HID_USAGE_GENERIC_Y,
        HID_USAGE_GENERIC_Z,
        HID_USAGE_GENERIC_RX,
        HID_USAGE_GENERIC_RY,
        HID_USAGE_GENERIC_RZ,
        HID_USAGE_GENERIC_SLIDER,
        HID_USAGE_GENERIC_DIAL,
        HID_USAGE_GENERIC_WHEEL
    };
    struct sdl_device *impl = impl_from_unix_device(iface);
    int i, button_count, axis_count, ball_count, hat_count;

    axis_count = pSDL_JoystickNumAxes(impl->sdl_joystick);
    if (axis_count > 6)
    {
        FIXME("Clamping joystick to 6 axis\n");
        axis_count = 6;
    }

    ball_count = pSDL_JoystickNumBalls(impl->sdl_joystick);
    if (axis_count + ball_count * 2 > ARRAY_SIZE(joystick_usages))
    {
        FIXME("Capping ball + axis at 9\n");
        ball_count = (ARRAY_SIZE(joystick_usages) - axis_count) / 2;
    }

    hat_count = pSDL_JoystickNumHats(impl->sdl_joystick);
    button_count = pSDL_JoystickNumButtons(impl->sdl_joystick);

    if (!hid_device_begin_report_descriptor(iface, HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_JOYSTICK))
        return STATUS_NO_MEMORY;

    if (!hid_device_begin_input_report(iface))
        return STATUS_NO_MEMORY;

    if (axis_count && !hid_device_add_axes(iface, axis_count, HID_USAGE_PAGE_GENERIC,
                                           joystick_usages, FALSE, -32768, 32767))
        return STATUS_NO_MEMORY;

    if (ball_count && !hid_device_add_axes(iface, ball_count * 2, HID_USAGE_PAGE_GENERIC,
                                           &joystick_usages[axis_count], TRUE, INT32_MIN, INT32_MAX))
        return STATUS_NO_MEMORY;

    if (hat_count && !hid_device_add_hatswitch(iface, hat_count))
        return STATUS_NO_MEMORY;

    if (button_count && !hid_device_add_buttons(iface, HID_USAGE_PAGE_BUTTON, 1, button_count))
        return STATUS_NO_MEMORY;

    if (!hid_device_end_input_report(iface))
        return STATUS_NO_MEMORY;

    if (!descriptor_add_haptic(impl))
        return STATUS_NO_MEMORY;

    if (!hid_device_end_report_descriptor(iface))
        return STATUS_NO_MEMORY;

    /* Initialize axis in the report */
    for (i = 0; i < axis_count; i++)
        hid_device_set_abs_axis(iface, i, pSDL_JoystickGetAxis(impl->sdl_joystick, i));
    for (i = 0; i < hat_count; i++)
        set_hat_value(iface, i, pSDL_JoystickGetHat(impl->sdl_joystick, i));

    return STATUS_SUCCESS;
}

static NTSTATUS build_controller_report_descriptor(struct unix_device *iface)
{
    static const USAGE left_axis_usages[] = {HID_USAGE_GENERIC_X, HID_USAGE_GENERIC_Y};
    static const USAGE right_axis_usages[] = {HID_USAGE_GENERIC_RX, HID_USAGE_GENERIC_RY};
    static const USAGE trigger_axis_usages[] = {HID_USAGE_GENERIC_Z, HID_USAGE_GENERIC_RZ};
    struct sdl_device *impl = impl_from_unix_device(iface);
    ULONG i, button_count = SDL_CONTROLLER_BUTTON_MAX - 1;
    C_ASSERT(SDL_CONTROLLER_AXIS_MAX == 6);

    if (!hid_device_begin_report_descriptor(iface, HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD))
        return STATUS_NO_MEMORY;

    if (!hid_device_begin_input_report(iface))
        return STATUS_NO_MEMORY;

    if (!hid_device_add_axes(iface, 2, HID_USAGE_PAGE_GENERIC, left_axis_usages,
                             FALSE, -32768, 32767))
        return STATUS_NO_MEMORY;

    if (!hid_device_add_axes(iface, 2, HID_USAGE_PAGE_GENERIC, right_axis_usages,
                             FALSE, -32768, 32767))
        return STATUS_NO_MEMORY;

    if (!hid_device_add_axes(iface, 2, HID_USAGE_PAGE_GENERIC, trigger_axis_usages,
                             FALSE, 0, 32767))
        return STATUS_NO_MEMORY;

    if (!hid_device_add_hatswitch(iface, 1))
        return STATUS_NO_MEMORY;

    if (!hid_device_add_buttons(iface, HID_USAGE_PAGE_BUTTON, 1, button_count))
        return STATUS_NO_MEMORY;

    if (!hid_device_end_input_report(iface))
        return STATUS_NO_MEMORY;

    if (!descriptor_add_haptic(impl))
        return STATUS_NO_MEMORY;

    if (!hid_device_end_report_descriptor(iface))
        return STATUS_NO_MEMORY;

    /* Initialize axis in the report */
    for (i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++)
        hid_device_set_abs_axis(iface, i, pSDL_GameControllerGetAxis(impl->sdl_controller, i));
    if (pSDL_GameControllerGetButton(impl->sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
        hid_device_set_hatswitch_y(iface, 0, -1);
    if (pSDL_GameControllerGetButton(impl->sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
        hid_device_set_hatswitch_y(iface, 0, +1);
    if (pSDL_GameControllerGetButton(impl->sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
        hid_device_set_hatswitch_x(iface, 0, -1);
    if (pSDL_GameControllerGetButton(impl->sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
        hid_device_set_hatswitch_x(iface, 0, +1);

    return STATUS_SUCCESS;
}

static void sdl_device_destroy(struct unix_device *iface)
{
}

static NTSTATUS sdl_device_start(struct unix_device *iface)
{
    struct sdl_device *impl = impl_from_unix_device(iface);
    if (impl->sdl_controller) return build_controller_report_descriptor(iface);
    return build_joystick_report_descriptor(iface);
}

static void sdl_device_stop(struct unix_device *iface)
{
    struct sdl_device *impl = impl_from_unix_device(iface);

    pSDL_JoystickClose(impl->sdl_joystick);
    if (impl->sdl_controller) pSDL_GameControllerClose(impl->sdl_controller);
    if (impl->sdl_haptic) pSDL_HapticClose(impl->sdl_haptic);

    pthread_mutex_lock(&sdl_cs);
    list_remove(&impl->unix_device.entry);
    pthread_mutex_unlock(&sdl_cs);
}

NTSTATUS sdl_device_haptics_start(struct unix_device *iface, DWORD duration_ms,
                                  USHORT rumble_intensity, USHORT buzz_intensity)
{
    struct sdl_device *impl = impl_from_unix_device(iface);
    SDL_HapticEffect effect;

    TRACE("iface %p, duration_ms %u, rumble_intensity %u, buzz_intensity %u.\n", iface, duration_ms,
          rumble_intensity, buzz_intensity);

    if (!(impl->effect_support & EFFECT_SUPPORT_HAPTICS)) return STATUS_NOT_SUPPORTED;

    memset(&effect, 0, sizeof(SDL_HapticEffect));
    effect.type = SDL_HAPTIC_LEFTRIGHT;
    effect.leftright.length = duration_ms;
    effect.leftright.large_magnitude = rumble_intensity;
    effect.leftright.small_magnitude = buzz_intensity;

    if (impl->sdl_haptic) pSDL_HapticStopAll(impl->sdl_haptic);
    if (impl->effect_support & WINE_SDL_JOYSTICK_RUMBLE)
        pSDL_JoystickRumble(impl->sdl_joystick, 0, 0, 0);
    if (!effect.leftright.large_magnitude && !effect.leftright.small_magnitude)
        return STATUS_SUCCESS;

    if (impl->effect_support & SDL_HAPTIC_LEFTRIGHT)
    {
        if (impl->haptic_effect_id >= 0)
            pSDL_HapticDestroyEffect(impl->sdl_haptic, impl->haptic_effect_id);
        impl->haptic_effect_id = pSDL_HapticNewEffect(impl->sdl_haptic, &effect);
        if (impl->haptic_effect_id >= 0)
            pSDL_HapticRunEffect(impl->sdl_haptic, impl->haptic_effect_id, 1);
    }
    else if (impl->effect_support & WINE_SDL_HAPTIC_RUMBLE)
    {
        float magnitude = (effect.leftright.large_magnitude + effect.leftright.small_magnitude) / 2.0 / 32767.0;
        pSDL_HapticRumblePlay(impl->sdl_haptic, magnitude, effect.leftright.length);
    }
    else if (impl->effect_support & WINE_SDL_JOYSTICK_RUMBLE)
    {
        pSDL_JoystickRumble(impl->sdl_joystick, effect.leftright.large_magnitude,
                            effect.leftright.small_magnitude, -1);
    }

    return STATUS_SUCCESS;
}

static const struct hid_device_vtbl sdl_device_vtbl =
{
    sdl_device_destroy,
    sdl_device_start,
    sdl_device_stop,
    sdl_device_haptics_start,
};

static BOOL set_report_from_joystick_event(struct sdl_device *impl, SDL_Event *event)
{
    struct unix_device *iface = &impl->unix_device;
    struct hid_device_state *state = &iface->hid_device_state;

    if (impl->sdl_controller) return TRUE; /* use controller events instead */

    switch(event->type)
    {
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        {
            SDL_JoyButtonEvent *ie = &event->jbutton;

            hid_device_set_button(iface, ie->button, ie->state);
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        case SDL_JOYAXISMOTION:
        {
            SDL_JoyAxisEvent *ie = &event->jaxis;

            hid_device_set_abs_axis(iface, ie->axis, ie->value);
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        case SDL_JOYBALLMOTION:
        {
            SDL_JoyBallEvent *ie = &event->jball;

            hid_device_set_rel_axis(iface, 2 * ie->ball, ie->xrel);
            hid_device_set_rel_axis(iface, 2 * ie->ball + 1, ie->yrel);
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        case SDL_JOYHATMOTION:
        {
            SDL_JoyHatEvent *ie = &event->jhat;

            set_hat_value(iface, ie->hat, ie->value);
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        default:
            ERR("TODO: Process Report (0x%x)\n",event->type);
    }
    return FALSE;
}

static BOOL set_report_from_controller_event(struct sdl_device *impl, SDL_Event *event)
{
    struct unix_device *iface = &impl->unix_device;
    struct hid_device_state *state = &iface->hid_device_state;

    switch(event->type)
    {
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        {
            SDL_ControllerButtonEvent *ie = &event->cbutton;
            int button;

            switch ((button = ie->button))
            {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                hid_device_set_hatswitch_y(iface, 0, ie->state ? -1 : 0);
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                hid_device_set_hatswitch_y(iface, 0, ie->state ? +1 : 0);
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                hid_device_set_hatswitch_x(iface, 0, ie->state ? -1 : 0);
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                hid_device_set_hatswitch_x(iface, 0, ie->state ? +1 : 0);
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: button = 4; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: button = 5; break;
            case SDL_CONTROLLER_BUTTON_BACK: button = 6; break;
            case SDL_CONTROLLER_BUTTON_START: button = 7; break;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: button = 8; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: button = 9; break;
            case SDL_CONTROLLER_BUTTON_GUIDE: button = 10; break;
            }

            hid_device_set_button(iface, button, ie->state);
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        case SDL_CONTROLLERAXISMOTION:
        {
            SDL_ControllerAxisEvent *ie = &event->caxis;

            hid_device_set_abs_axis(iface, ie->axis, ie->value);
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        default:
            ERR("TODO: Process Report (%x)\n",event->type);
    }
    return FALSE;
}

static void sdl_add_device(unsigned int index)
{
    struct device_desc desc =
    {
        .input = -1,
        .manufacturer = {"SDL"},
        .serialnumber = {"0000"},
    };
    struct sdl_device *impl;

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

    if (controller) lstrcpynA(desc.product, pSDL_GameControllerName(controller), sizeof(desc.product));
    else lstrcpynA(desc.product, pSDL_JoystickName(joystick), sizeof(desc.product));

    id = pSDL_JoystickInstanceID(joystick);

    if (pSDL_JoystickGetProductVersion != NULL) {
        desc.vid = pSDL_JoystickGetVendor(joystick);
        desc.pid = pSDL_JoystickGetProduct(joystick);
        desc.version = pSDL_JoystickGetProductVersion(joystick);
    }
    else
    {
        desc.vid = 0x01;
        desc.pid = pSDL_JoystickInstanceID(joystick) + 1;
        desc.version = 0;
    }

    guid = pSDL_JoystickGetGUID(joystick);
    pSDL_JoystickGetGUIDString(guid, desc.serialnumber, sizeof(desc.serialnumber));

    if (controller) desc.is_gamepad = TRUE;
    else
    {
        int button_count, axis_count;

        axis_count = pSDL_JoystickNumAxes(joystick);
        button_count = pSDL_JoystickNumButtons(joystick);
        desc.is_gamepad = (axis_count == 6  && button_count >= 14);
    }

    TRACE("%s id %d, desc %s.\n", controller ? "controller" : "joystick", id, debugstr_device_desc(&desc));

    if (!(impl = hid_device_create(&sdl_device_vtbl, sizeof(struct sdl_device)))) return;
    list_add_tail(&device_list, &impl->unix_device.entry);
    impl->sdl_joystick = joystick;
    impl->sdl_controller = controller;
    impl->id = id;

    bus_event_queue_device_created(&event_queue, &impl->unix_device, &desc);
}

static void process_device_event(SDL_Event *event)
{
    struct sdl_device *impl;
    SDL_JoystickID id;

    TRACE_(hid_report)("Received action %x\n", event->type);

    pthread_mutex_lock(&sdl_cs);

    if (event->type == SDL_JOYDEVICEADDED)
        sdl_add_device(((SDL_JoyDeviceEvent *)event)->which);
    else if (event->type == SDL_JOYDEVICEREMOVED)
    {
        id = ((SDL_JoyDeviceEvent *)event)->which;
        impl = find_device_from_id(id);
        if (impl) bus_event_queue_device_removed(&event_queue, &impl->unix_device);
        else WARN("failed to find device with id %d\n", id);
    }
    else if (event->type >= SDL_JOYAXISMOTION && event->type <= SDL_JOYBUTTONUP)
    {
        id = ((SDL_JoyButtonEvent *)event)->which;
        impl = find_device_from_id(id);
        if (impl) set_report_from_joystick_event(impl, event);
        else WARN("failed to find device with id %d\n", id);
    }
    else if (event->type >= SDL_CONTROLLERAXISMOTION && event->type <= SDL_CONTROLLERBUTTONUP)
    {
        id = ((SDL_ControllerButtonEvent *)event)->which;
        impl = find_device_from_id(id);
        if (impl) set_report_from_controller_event(impl, event);
        else WARN("failed to find device with id %d\n", id);
    }

    pthread_mutex_unlock(&sdl_cs);
}

NTSTATUS sdl_bus_init(void *args)
{
    const char *mapping;
    int i;

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
    LOAD_FUNCPTR(SDL_GameControllerAddMapping);
    LOAD_FUNCPTR(SDL_RegisterEvents);
    LOAD_FUNCPTR(SDL_PushEvent);
#undef LOAD_FUNCPTR
    pSDL_JoystickRumble = dlsym(sdl_handle, "SDL_JoystickRumble");
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
    if (pSDL_GameControllerAddMapping)
    {
        if ((mapping = getenv("SDL_GAMECONTROLLERCONFIG")))
        {
            TRACE("Setting environment mapping %s\n", debugstr_a(mapping));
            if (pSDL_GameControllerAddMapping(mapping) < 0)
                WARN("Failed to add environment mapping %s\n", pSDL_GetError());
        }
        else for (i = 0; i < options.mappings_count; ++i)
        {
            TRACE("Setting registry mapping %s\n", debugstr_a(options.mappings[i]));
            if (pSDL_GameControllerAddMapping(options.mappings[i]) < 0)
                WARN("Failed to add registry mapping %s\n", pSDL_GetError());
        }
    }

    return STATUS_SUCCESS;

failed:
    dlclose(sdl_handle);
    sdl_handle = NULL;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS sdl_bus_wait(void *args)
{
    struct bus_event *result = args;
    SDL_Event event;

    /* cleanup previously returned event */
    bus_event_cleanup(result);

    do
    {
        if (bus_event_queue_pop(&event_queue, result)) return STATUS_PENDING;
        if (pSDL_WaitEvent(&event) != 0) process_device_event(&event);
        else WARN("SDL_WaitEvent failed: %s\n", pSDL_GetError());
    } while (event.type != quit_event);

    TRACE("SDL main loop exiting\n");
    bus_event_queue_destroy(&event_queue);
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
