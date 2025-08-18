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

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dlfcn.h>
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
#include "hidusage.h"

#include "wine/debug.h"
#include "wine/hid.h"
#include "wine/unixlib.h"

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

#ifdef SONAME_LIBSDL2

static pthread_mutex_t sdl_cs = PTHREAD_MUTEX_INITIALIZER;
static const struct bus_options *options;

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
MAKE_FUNCPTR(SDL_WaitEventTimeout);
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
MAKE_FUNCPTR(SDL_HapticGetEffectStatus);
MAKE_FUNCPTR(SDL_HapticNewEffect);
MAKE_FUNCPTR(SDL_HapticNumAxes);
MAKE_FUNCPTR(SDL_HapticOpenFromJoystick);
MAKE_FUNCPTR(SDL_HapticPause);
MAKE_FUNCPTR(SDL_HapticQuery);
MAKE_FUNCPTR(SDL_HapticRumbleInit);
MAKE_FUNCPTR(SDL_HapticRumblePlay);
MAKE_FUNCPTR(SDL_HapticRumbleStop);
MAKE_FUNCPTR(SDL_HapticRumbleSupported);
MAKE_FUNCPTR(SDL_HapticRunEffect);
MAKE_FUNCPTR(SDL_HapticSetGain);
MAKE_FUNCPTR(SDL_HapticSetAutocenter);
MAKE_FUNCPTR(SDL_HapticStopAll);
MAKE_FUNCPTR(SDL_HapticStopEffect);
MAKE_FUNCPTR(SDL_HapticUnpause);
MAKE_FUNCPTR(SDL_HapticUpdateEffect);
MAKE_FUNCPTR(SDL_JoystickIsHaptic);
MAKE_FUNCPTR(SDL_GameControllerAddMapping);
MAKE_FUNCPTR(SDL_RegisterEvents);
MAKE_FUNCPTR(SDL_PushEvent);
MAKE_FUNCPTR(SDL_GetTicks);
static int (*pSDL_JoystickRumble)(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms);
static int (*pSDL_JoystickRumbleTriggers)(SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble, Uint32 duration_ms);
static Uint16 (*pSDL_JoystickGetProduct)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetProductVersion)(SDL_Joystick * joystick);
static Uint16 (*pSDL_JoystickGetVendor)(SDL_Joystick * joystick);
static SDL_JoystickType (*pSDL_JoystickGetType)(SDL_Joystick * joystick);
static const char *(*pSDL_JoystickGetSerial)(SDL_Joystick * joystick);

/* internal bits for extended rumble support, SDL_Haptic types are 16-bits */
#define WINE_SDL_JOYSTICK_RUMBLE  0x40000000 /* using SDL_JoystickRumble API */
#define WINE_SDL_HAPTIC_RUMBLE    0x80000000 /* using SDL_HapticRumble API */

#define EFFECT_SUPPORT_HAPTICS  (SDL_HAPTIC_LEFTRIGHT|WINE_SDL_HAPTIC_RUMBLE|WINE_SDL_JOYSTICK_RUMBLE)
#define EFFECT_SUPPORT_PHYSICAL (SDL_HAPTIC_CONSTANT|SDL_HAPTIC_RAMP|SDL_HAPTIC_SINE|SDL_HAPTIC_TRIANGLE| \
                                 SDL_HAPTIC_SAWTOOTHUP|SDL_HAPTIC_SAWTOOTHDOWN|SDL_HAPTIC_SPRING|SDL_HAPTIC_DAMPER|SDL_HAPTIC_INERTIA| \
                                 SDL_HAPTIC_FRICTION|SDL_HAPTIC_CUSTOM)

struct sdl_device
{
    struct unix_device unix_device;

    SDL_Joystick *sdl_joystick;
    SDL_GameController *sdl_controller;
    SDL_JoystickID id;
    BOOL started;

    DWORD effect_support;
    SDL_Haptic *sdl_haptic;
    int haptic_effect_id;
    int effect_ids[256];
    int effect_state[256];
    LONG effect_flags;
    int axis_offset;
};

static inline struct sdl_device *impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(iface, struct sdl_device, unix_device);
}

static struct sdl_device *find_device_from_id(SDL_JoystickID id)
{
    struct sdl_device *impl;

    LIST_FOR_EACH_ENTRY(impl, &device_list, struct sdl_device, unix_device.entry)
        if (impl->id == id && impl->axis_offset == 0) return impl;

    return NULL;
}

static struct sdl_device *find_device_from_id_and_axis(SDL_JoystickID id, int axis)
{
    struct sdl_device *impl;

    LIST_FOR_EACH_ENTRY(impl, &device_list, struct sdl_device, unix_device.entry)
    {
        USHORT count = impl->unix_device.hid_device_state.abs_axis_count;
        if (impl->id == id && impl->axis_offset <= axis && impl->axis_offset + count > axis)
            return impl;
    }

    return NULL;
}

static void set_hat_value(struct unix_device *iface, int index, int value)
{
    LONG x = 0, y = 0;
    switch (value)
    {
    case SDL_HAT_CENTERED: break;
    case SDL_HAT_DOWN: y = 1; break;
    case SDL_HAT_RIGHTDOWN: y = x = 1; break;
    case SDL_HAT_RIGHT: x = 1; break;
    case SDL_HAT_RIGHTUP: x = 1; y = -1; break;
    case SDL_HAT_UP: y = -1; break;
    case SDL_HAT_LEFTUP: x = y = -1; break;
    case SDL_HAT_LEFT: x = -1; break;
    case SDL_HAT_LEFTDOWN: x = -1; y = 1; break;
    }
    hid_device_set_hatswitch_x(iface, index, x);
    hid_device_set_hatswitch_y(iface, index, y);
}

static BOOL descriptor_add_haptic(struct sdl_device *impl, BOOL force)
{
    USHORT i, count = 0;
    USAGE usages[16];

    if (impl->axis_offset > 0 || !pSDL_JoystickIsHaptic(impl->sdl_joystick) ||
        !(impl->sdl_haptic = pSDL_HapticOpenFromJoystick(impl->sdl_joystick)))
        impl->effect_support = 0;
    else
    {
        impl->effect_support = pSDL_HapticQuery(impl->sdl_haptic);
        if (!(impl->effect_support & EFFECT_SUPPORT_HAPTICS) &&
            pSDL_HapticRumbleSupported(impl->sdl_haptic) &&
            pSDL_HapticRumbleInit(impl->sdl_haptic) == 0)
            impl->effect_support |= WINE_SDL_HAPTIC_RUMBLE;
    }

    if (impl->axis_offset == 0 && pSDL_JoystickRumble && !pSDL_JoystickRumble(impl->sdl_joystick, 0, 0, 0))
        impl->effect_support |= WINE_SDL_JOYSTICK_RUMBLE;

    if (impl->effect_support & EFFECT_SUPPORT_HAPTICS)
    {
        if (!hid_device_add_haptics(&impl->unix_device))
            return FALSE;
    }

    if ((impl->effect_support & EFFECT_SUPPORT_PHYSICAL))
    {
        int axes_count;

        /* SDL_HAPTIC_SQUARE doesn't exist */
        if (force || (impl->effect_support & SDL_HAPTIC_SINE)) usages[count++] = PID_USAGE_ET_SINE;
        if (force || (impl->effect_support & SDL_HAPTIC_TRIANGLE)) usages[count++] = PID_USAGE_ET_TRIANGLE;
        if (force || (impl->effect_support & SDL_HAPTIC_SAWTOOTHUP)) usages[count++] = PID_USAGE_ET_SAWTOOTH_UP;
        if (force || (impl->effect_support & SDL_HAPTIC_SAWTOOTHDOWN)) usages[count++] = PID_USAGE_ET_SAWTOOTH_DOWN;
        if (force || (impl->effect_support & SDL_HAPTIC_SPRING)) usages[count++] = PID_USAGE_ET_SPRING;
        if (force || (impl->effect_support & SDL_HAPTIC_DAMPER)) usages[count++] = PID_USAGE_ET_DAMPER;
        if (force || (impl->effect_support & SDL_HAPTIC_INERTIA)) usages[count++] = PID_USAGE_ET_INERTIA;
        if (force || (impl->effect_support & SDL_HAPTIC_FRICTION)) usages[count++] = PID_USAGE_ET_FRICTION;
        if (force || (impl->effect_support & SDL_HAPTIC_CONSTANT)) usages[count++] = PID_USAGE_ET_CONSTANT_FORCE;
        if (force || (impl->effect_support & SDL_HAPTIC_RAMP)) usages[count++] = PID_USAGE_ET_RAMP;

        if ((axes_count = pSDL_HapticNumAxes(impl->sdl_haptic)) < 0) axes_count = 2;
        if (!hid_device_add_physical(&impl->unix_device, usages, count, axes_count))
            return FALSE;
    }

    impl->haptic_effect_id = -1;
    for (i = 0; i < ARRAY_SIZE(impl->effect_ids); ++i) impl->effect_ids[i] = -1;
    return TRUE;
}

static const USAGE_AND_PAGE g920_absolute_usages[] =
{
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_X},  /* wheel */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_Y},  /* accelerator */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_Z},  /* brake */
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RZ}, /* clutch */
};
static const USAGE_AND_PAGE absolute_axis_usages[] =
{
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_X},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_Y},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_Z},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RX},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RY},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RZ},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_SLIDER},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_DIAL},
};
static const USAGE_AND_PAGE relative_axis_usages[] =
{
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_X},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_Y},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RX},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RY},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_Z},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_RZ},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_SLIDER},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_DIAL},
    {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_WHEEL},
};

static int get_absolute_usages(const struct device_desc *desc, const USAGE_AND_PAGE **absolute_usages)
{
    if (desc->vid == 0x046d && desc->pid == 0xc262)
    {
        *absolute_usages = g920_absolute_usages;
        return ARRAY_SIZE(g920_absolute_usages);
    }

    *absolute_usages = absolute_axis_usages;
    return ARRAY_SIZE(absolute_axis_usages);
}

static NTSTATUS build_joystick_report_descriptor(struct unix_device *iface, const struct device_desc *desc)
{
    const USAGE_AND_PAGE device_usage = {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_JOYSTICK};
    struct sdl_device *impl = impl_from_unix_device(iface);
    int i, button_count, axis_count, ball_count, hat_count;
    USAGE_AND_PAGE physical_usage;

    const USAGE_AND_PAGE *absolute_usages = NULL;
    size_t absolute_usages_count = get_absolute_usages(desc, &absolute_usages);

    axis_count = pSDL_JoystickNumAxes(impl->sdl_joystick);
    if (options->split_controllers) axis_count = min(6, axis_count - impl->axis_offset);
    if (axis_count > absolute_usages_count)
    {
        FIXME("More than %zu absolute axes found, ignoring.\n", absolute_usages_count);
        axis_count = absolute_usages_count;
    }

    ball_count = pSDL_JoystickNumBalls(impl->sdl_joystick);
    if (ball_count > ARRAY_SIZE(relative_axis_usages) / 2)
    {
        FIXME("More than %zu relative axes found, ignoring.\n", ARRAY_SIZE(relative_axis_usages));
        ball_count = ARRAY_SIZE(relative_axis_usages) / 2;
    }

    if (impl->axis_offset == 0)
    {
        hat_count = pSDL_JoystickNumHats(impl->sdl_joystick);
        button_count = pSDL_JoystickNumButtons(impl->sdl_joystick);
    }
    else
    {
        hat_count = 0;
        button_count = 0;
    }

    if (!pSDL_JoystickGetType) physical_usage = device_usage;
    else switch (pSDL_JoystickGetType(impl->sdl_joystick))
    {
    case SDL_JOYSTICK_TYPE_ARCADE_PAD:
    case SDL_JOYSTICK_TYPE_ARCADE_STICK:
    case SDL_JOYSTICK_TYPE_DANCE_PAD:
    case SDL_JOYSTICK_TYPE_DRUM_KIT:
    case SDL_JOYSTICK_TYPE_GUITAR:
    case SDL_JOYSTICK_TYPE_UNKNOWN:
        physical_usage.UsagePage = HID_USAGE_PAGE_GENERIC;
        physical_usage.Usage = HID_USAGE_GENERIC_JOYSTICK;
        break;
    case SDL_JOYSTICK_TYPE_GAMECONTROLLER:
        physical_usage.UsagePage = HID_USAGE_PAGE_GENERIC;
        physical_usage.Usage = HID_USAGE_GENERIC_GAMEPAD;
        break;
    case SDL_JOYSTICK_TYPE_WHEEL:
        physical_usage.UsagePage = HID_USAGE_PAGE_SIMULATION;
        physical_usage.Usage = HID_USAGE_SIMULATION_AUTOMOBILE_SIMULATION_DEVICE;
        break;
    case SDL_JOYSTICK_TYPE_FLIGHT_STICK:
    case SDL_JOYSTICK_TYPE_THROTTLE:
        physical_usage.UsagePage = HID_USAGE_PAGE_SIMULATION;
        physical_usage.Usage = HID_USAGE_SIMULATION_FLIGHT_SIMULATION_DEVICE;
        break;
    }

    if (!hid_device_begin_report_descriptor(iface, &device_usage))
        return STATUS_NO_MEMORY;

    if (!hid_device_begin_input_report(iface, &physical_usage))
        return STATUS_NO_MEMORY;

    for (i = 0; i < axis_count; i++)
    {
        if (!hid_device_add_axes(iface, 1, absolute_usages[i].UsagePage,
                                 &absolute_usages[i].Usage, FALSE, -32768, 32767))
            return STATUS_NO_MEMORY;
    }

    for (i = 0; i < ball_count; i++)
    {
        if (!hid_device_add_axes(iface, 2, relative_axis_usages[2 * i].UsagePage,
                                 &relative_axis_usages[2 * i].Usage, TRUE, INT32_MIN, INT32_MAX))
            return STATUS_NO_MEMORY;
    }

    if (hat_count && !hid_device_add_hatswitch(iface, hat_count))
        return STATUS_NO_MEMORY;

    if (button_count && !hid_device_add_buttons(iface, HID_USAGE_PAGE_BUTTON, 1, button_count))
        return STATUS_NO_MEMORY;

    if (!hid_device_end_input_report(iface))
        return STATUS_NO_MEMORY;

    if (!descriptor_add_haptic(impl, physical_usage.Usage == HID_USAGE_SIMULATION_AUTOMOBILE_SIMULATION_DEVICE))
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
    const USAGE_AND_PAGE device_usage = {.UsagePage = HID_USAGE_PAGE_GENERIC, .Usage = HID_USAGE_GENERIC_GAMEPAD};
    struct sdl_device *impl = impl_from_unix_device(iface);
    BOOL state;

    C_ASSERT(SDL_CONTROLLER_AXIS_MAX == 6);

    if (!hid_device_begin_report_descriptor(iface, &device_usage)) return STATUS_NO_MEMORY;
    if (!hid_device_add_gamepad(iface)) return STATUS_NO_MEMORY;
    if (!descriptor_add_haptic(impl, FALSE)) return STATUS_NO_MEMORY;
    if (!hid_device_end_report_descriptor(iface)) return STATUS_NO_MEMORY;

    for (int i = SDL_CONTROLLER_AXIS_LEFTX; i < SDL_CONTROLLER_AXIS_MAX; i++)
    {
        int value = pSDL_GameControllerGetAxis(impl->sdl_controller, i);
        if (i == SDL_CONTROLLER_AXIS_LEFTY || i == SDL_CONTROLLER_AXIS_RIGHTY)
            value = -value - 1; /* match XUSB / GIP protocol */
        hid_device_set_abs_axis(iface, i, value);
    }

    state = pSDL_GameControllerGetButton(impl->sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
    hid_device_move_hatswitch(iface, 0, 0, state ? -1 : +1);
    state = pSDL_GameControllerGetButton(impl->sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    hid_device_move_hatswitch(iface, 0, 0, state ? +1 : -1);
    state = pSDL_GameControllerGetButton(impl->sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    hid_device_move_hatswitch(iface, 0, state ? -1 : +1, 0);
    state = pSDL_GameControllerGetButton(impl->sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    hid_device_move_hatswitch(iface, 0, state ? +1 : -1, 0);

    return STATUS_SUCCESS;
}

static void sdl_device_destroy(struct unix_device *iface)
{
}

static NTSTATUS sdl_device_start(struct unix_device *iface)
{
    struct sdl_device *impl = impl_from_unix_device(iface);

    pthread_mutex_lock(&sdl_cs);
    impl->started = TRUE;
    pthread_mutex_unlock(&sdl_cs);

    return STATUS_SUCCESS;
}

static void sdl_device_stop(struct unix_device *iface)
{
    struct sdl_device *impl = impl_from_unix_device(iface);

    pSDL_JoystickClose(impl->sdl_joystick);
    if (impl->sdl_controller) pSDL_GameControllerClose(impl->sdl_controller);
    if (impl->sdl_haptic) pSDL_HapticClose(impl->sdl_haptic);

    pthread_mutex_lock(&sdl_cs);
    impl->started = FALSE;
    list_remove(&impl->unix_device.entry);
    pthread_mutex_unlock(&sdl_cs);
}

static NTSTATUS sdl_device_haptics_start(struct unix_device *iface, UINT duration_ms,
                                         USHORT rumble_intensity, USHORT buzz_intensity,
                                         USHORT left_intensity, USHORT right_intensity)
{
    struct sdl_device *impl = impl_from_unix_device(iface);

    TRACE("iface %p, duration_ms %u, rumble_intensity %u, buzz_intensity %u, left_intensity %u, right_intensity %u.\n",
          iface, duration_ms, rumble_intensity, buzz_intensity, left_intensity, right_intensity);

    if (!(impl->effect_support & EFFECT_SUPPORT_HAPTICS)) return STATUS_NOT_SUPPORTED;

    if (impl->effect_support & WINE_SDL_JOYSTICK_RUMBLE)
    {
        pSDL_JoystickRumble(impl->sdl_joystick, rumble_intensity, buzz_intensity, duration_ms);
        if (pSDL_JoystickRumbleTriggers)
            pSDL_JoystickRumbleTriggers(impl->sdl_joystick, left_intensity, right_intensity, duration_ms);
    }
    else if (impl->effect_support & SDL_HAPTIC_LEFTRIGHT)
    {
        SDL_HapticEffect effect;

        memset(&effect, 0, sizeof(SDL_HapticEffect));
        effect.type = SDL_HAPTIC_LEFTRIGHT;
        effect.leftright.length = duration_ms;
        effect.leftright.large_magnitude = rumble_intensity;
        effect.leftright.small_magnitude = buzz_intensity;

        if (impl->haptic_effect_id >= 0)
            pSDL_HapticDestroyEffect(impl->sdl_haptic, impl->haptic_effect_id);
        impl->haptic_effect_id = pSDL_HapticNewEffect(impl->sdl_haptic, &effect);
        if (impl->haptic_effect_id >= 0)
            pSDL_HapticRunEffect(impl->sdl_haptic, impl->haptic_effect_id, 1);
    }
    else if (impl->effect_support & WINE_SDL_HAPTIC_RUMBLE)
    {
        float magnitude = (rumble_intensity + buzz_intensity) / 2.0 / 32767.0;
        pSDL_HapticRumblePlay(impl->sdl_haptic, magnitude, duration_ms);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS sdl_device_haptics_stop(struct unix_device *iface)
{
    struct sdl_device *impl = impl_from_unix_device(iface);

    TRACE("iface %p.\n", iface);

    if (impl->effect_support & WINE_SDL_JOYSTICK_RUMBLE)
    {
        pSDL_JoystickRumble(impl->sdl_joystick, 0, 0, 0);
        if (pSDL_JoystickRumbleTriggers)
            pSDL_JoystickRumbleTriggers(impl->sdl_joystick, 0, 0, 0);
    }
    else if (impl->effect_support & SDL_HAPTIC_LEFTRIGHT)
        pSDL_HapticStopAll(impl->sdl_haptic);
    else if (impl->effect_support & WINE_SDL_HAPTIC_RUMBLE)
        pSDL_HapticRumbleStop(impl->sdl_haptic);

    return STATUS_SUCCESS;
}

static NTSTATUS sdl_device_physical_device_control(struct unix_device *iface, USAGE control)
{
    struct sdl_device *impl = impl_from_unix_device(iface);
    unsigned int i;

    TRACE("iface %p, control %#04x.\n", iface, control);

    switch (control)
    {
    case PID_USAGE_DC_ENABLE_ACTUATORS:
        pSDL_HapticSetGain(impl->sdl_haptic, 100);
        InterlockedOr(&impl->effect_flags, EFFECT_STATE_ACTUATORS_ENABLED);
        return STATUS_SUCCESS;
    case PID_USAGE_DC_DISABLE_ACTUATORS:
        pSDL_HapticSetGain(impl->sdl_haptic, 0);
        InterlockedAnd(&impl->effect_flags, ~EFFECT_STATE_ACTUATORS_ENABLED);
        return STATUS_SUCCESS;
    case PID_USAGE_DC_STOP_ALL_EFFECTS:
        pSDL_HapticStopAll(impl->sdl_haptic);
        return STATUS_SUCCESS;
    case PID_USAGE_DC_DEVICE_RESET:
        pSDL_HapticStopAll(impl->sdl_haptic);
        for (i = 0; i < ARRAY_SIZE(impl->effect_ids); ++i)
        {
            if (impl->effect_ids[i] < 0) continue;
            pSDL_HapticDestroyEffect(impl->sdl_haptic, impl->effect_ids[i]);
            impl->effect_ids[i] = -1;
        }
        return STATUS_SUCCESS;
    case PID_USAGE_DC_DEVICE_PAUSE:
        pSDL_HapticPause(impl->sdl_haptic);
        InterlockedOr(&impl->effect_flags, EFFECT_STATE_DEVICE_PAUSED);
        return STATUS_SUCCESS;
    case PID_USAGE_DC_DEVICE_CONTINUE:
        pSDL_HapticUnpause(impl->sdl_haptic);
        InterlockedAnd(&impl->effect_flags, ~EFFECT_STATE_DEVICE_PAUSED);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS sdl_device_physical_device_set_gain(struct unix_device *iface, BYTE percent)
{
    struct sdl_device *impl = impl_from_unix_device(iface);

    TRACE("iface %p, percent %#x.\n", iface, percent);

    pSDL_HapticSetGain(impl->sdl_haptic, percent);

    return STATUS_SUCCESS;
}

static NTSTATUS sdl_device_physical_effect_control(struct unix_device *iface, BYTE index,
                                                   USAGE control, BYTE iterations)
{
    struct sdl_device *impl = impl_from_unix_device(iface);
    int id = impl->effect_ids[index];

    TRACE("iface %p, index %u, control %04x, iterations %u.\n", iface, index, control, iterations);

    if (id < 0) return STATUS_SUCCESS;

    switch (control)
    {
    case PID_USAGE_OP_EFFECT_START_SOLO:
        pSDL_HapticStopAll(impl->sdl_haptic);
        /* fallthrough */
    case PID_USAGE_OP_EFFECT_START:
        pSDL_HapticRunEffect(impl->sdl_haptic, id, (iterations == 0xff ? SDL_HAPTIC_INFINITY : iterations));
        break;
    case PID_USAGE_OP_EFFECT_STOP:
        pSDL_HapticStopEffect(impl->sdl_haptic, id);
        break;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS set_effect_type_from_usage(SDL_HapticEffect *effect, USAGE type)
{
    switch (type)
    {
    case PID_USAGE_ET_SINE:
        effect->type = SDL_HAPTIC_SINE;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_TRIANGLE:
        effect->type = SDL_HAPTIC_TRIANGLE;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_SAWTOOTH_UP:
        effect->type = SDL_HAPTIC_SAWTOOTHUP;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_SAWTOOTH_DOWN:
        effect->type = SDL_HAPTIC_SAWTOOTHDOWN;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_SPRING:
        effect->type = SDL_HAPTIC_SPRING;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_DAMPER:
        effect->type = SDL_HAPTIC_DAMPER;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_INERTIA:
        effect->type = SDL_HAPTIC_INERTIA;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_FRICTION:
        effect->type = SDL_HAPTIC_FRICTION;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_CONSTANT_FORCE:
        effect->type = SDL_HAPTIC_CONSTANT;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_RAMP:
        effect->type = SDL_HAPTIC_RAMP;
        return STATUS_SUCCESS;
    case PID_USAGE_ET_CUSTOM_FORCE_DATA:
        effect->type = SDL_HAPTIC_CUSTOM;
        return STATUS_SUCCESS;
    default:
        return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS sdl_device_physical_effect_update(struct unix_device *iface, BYTE index,
                                                  struct effect_params *params)
{
    struct sdl_device *impl = impl_from_unix_device(iface);
    int id = impl->effect_ids[index];
    SDL_HapticEffect effect = {0};
    INT32 direction;
    NTSTATUS status;

    TRACE("iface %p, index %u, params %p.\n", iface, index, params);

    if (params->effect_type == PID_USAGE_UNDEFINED) return STATUS_SUCCESS;
    if ((status = set_effect_type_from_usage(&effect, params->effect_type))) return status;

    /* The first direction we get from PID is in polar coordinate space, so we need to
     * remove 90° to make it match SDL spherical coordinates. */
    direction = (params->direction[0] + 27000) % 36000;

    switch (params->effect_type)
    {
    case PID_USAGE_ET_SINE:
    case PID_USAGE_ET_SQUARE:
    case PID_USAGE_ET_TRIANGLE:
    case PID_USAGE_ET_SAWTOOTH_UP:
    case PID_USAGE_ET_SAWTOOTH_DOWN:
        effect.periodic.length = (params->duration == 0xffff ? SDL_HAPTIC_INFINITY : params->duration);
        effect.periodic.delay = params->start_delay;
        effect.periodic.button = params->trigger_button;
        effect.periodic.interval = params->trigger_repeat_interval;
        effect.periodic.direction.type = SDL_HAPTIC_SPHERICAL;
        effect.periodic.direction.dir[0] = direction;
        effect.periodic.direction.dir[1] = params->direction[1];
        effect.periodic.period = params->periodic.period;
        effect.periodic.magnitude = (params->periodic.magnitude * params->gain_percent) / 100;
        effect.periodic.offset = params->periodic.offset;
        effect.periodic.phase = params->periodic.phase;
        effect.periodic.attack_length = params->envelope.attack_time;
        effect.periodic.attack_level = params->envelope.attack_level;
        effect.periodic.fade_length = params->envelope.fade_time;
        effect.periodic.fade_level = params->envelope.fade_level;
        break;

    case PID_USAGE_ET_SPRING:
    case PID_USAGE_ET_DAMPER:
    case PID_USAGE_ET_INERTIA:
    case PID_USAGE_ET_FRICTION:
        effect.condition.length = (params->duration == 0xffff ? SDL_HAPTIC_INFINITY : params->duration);
        effect.condition.delay = params->start_delay;
        effect.condition.button = params->trigger_button;
        effect.condition.interval = params->trigger_repeat_interval;
        effect.condition.direction.type = SDL_HAPTIC_SPHERICAL;
        effect.condition.direction.dir[0] = direction;
        effect.condition.direction.dir[1] = params->direction[1];
        if (params->condition_count >= 1)
        {
            effect.condition.right_sat[0] = params->condition[0].positive_saturation;
            effect.condition.left_sat[0] = params->condition[0].negative_saturation;
            effect.condition.right_coeff[0] = params->condition[0].positive_coefficient;
            effect.condition.left_coeff[0] = params->condition[0].negative_coefficient;
            effect.condition.deadband[0] = params->condition[0].dead_band;
            effect.condition.center[0] = params->condition[0].center_point_offset;
        }
        if (params->condition_count >= 2)
        {
            effect.condition.right_sat[1] = params->condition[1].positive_saturation;
            effect.condition.left_sat[1] = params->condition[1].negative_saturation;
            effect.condition.right_coeff[1] = params->condition[1].positive_coefficient;
            effect.condition.left_coeff[1] = params->condition[1].negative_coefficient;
            effect.condition.deadband[1] = params->condition[1].dead_band;
            effect.condition.center[1] = params->condition[1].center_point_offset;
        }
        break;

    case PID_USAGE_ET_CONSTANT_FORCE:
        effect.constant.length = (params->duration == 0xffff ? SDL_HAPTIC_INFINITY : params->duration);
        effect.constant.delay = params->start_delay;
        effect.constant.button = params->trigger_button;
        effect.constant.interval = params->trigger_repeat_interval;
        effect.constant.direction.type = SDL_HAPTIC_SPHERICAL;
        effect.constant.direction.dir[0] = direction;
        effect.constant.direction.dir[1] = params->direction[1];
        effect.constant.level = (params->constant_force.magnitude * params->gain_percent) / 100;
        effect.constant.attack_length = params->envelope.attack_time;
        effect.constant.attack_level = params->envelope.attack_level;
        effect.constant.fade_length = params->envelope.fade_time;
        effect.constant.fade_level = params->envelope.fade_level;
        break;

    /* According to the SDL documentation, ramp effect doesn't
     * support SDL_HAPTIC_INFINITY. */
    case PID_USAGE_ET_RAMP:
        effect.ramp.length = params->duration;
        effect.ramp.delay = params->start_delay;
        effect.ramp.button = params->trigger_button;
        effect.ramp.interval = params->trigger_repeat_interval;
        effect.ramp.direction.type = SDL_HAPTIC_SPHERICAL;
        effect.ramp.direction.dir[0] = params->direction[0];
        effect.ramp.direction.dir[1] = params->direction[1];
        effect.ramp.start = (params->ramp_force.ramp_start * params->gain_percent) / 100;
        effect.ramp.end = (params->ramp_force.ramp_end * params->gain_percent) / 100;
        effect.ramp.attack_length = params->envelope.attack_time;
        effect.ramp.attack_level = params->envelope.attack_level;
        effect.ramp.fade_length = params->envelope.fade_time;
        effect.ramp.fade_level = params->envelope.fade_level;
        break;

    case PID_USAGE_ET_CUSTOM_FORCE_DATA:
        FIXME("not implemented!\n");
        break;
    }

    if (id < 0) impl->effect_ids[index] = pSDL_HapticNewEffect(impl->sdl_haptic, &effect);
    else pSDL_HapticUpdateEffect(impl->sdl_haptic, id, &effect);

    return STATUS_SUCCESS;
}

static const struct hid_device_vtbl sdl_device_vtbl =
{
    sdl_device_destroy,
    sdl_device_start,
    sdl_device_stop,
    sdl_device_haptics_start,
    sdl_device_haptics_stop,
    sdl_device_physical_device_control,
    sdl_device_physical_device_set_gain,
    sdl_device_physical_effect_control,
    sdl_device_physical_effect_update,
};

static void check_device_effects_state(struct sdl_device *impl)
{
    struct unix_device *iface = &impl->unix_device;
    struct hid_effect_state *effect_state = &iface->hid_physical.effect_state;
    ULONG effect_flags = InterlockedOr(&impl->effect_flags, 0);
    unsigned int i, ret;

    if (!impl->sdl_haptic) return;
    if (!(impl->effect_support & EFFECT_SUPPORT_PHYSICAL)) return;

    for (i = 0; i < ARRAY_SIZE(impl->effect_ids); ++i)
    {
        if (impl->effect_ids[i] == -1) continue;
        if (!(impl->effect_support & SDL_HAPTIC_STATUS)) ret = 1;
        else ret = pSDL_HapticGetEffectStatus(impl->sdl_haptic, impl->effect_ids[i]);
        if (impl->effect_state[i] == ret) continue;
        impl->effect_state[i] = ret;
        hid_device_set_effect_state(iface, i, effect_flags | (ret == 1 ? EFFECT_STATE_EFFECT_PLAYING : 0));
        bus_event_queue_input_report(&event_queue, iface, effect_state->report_buf, effect_state->report_len);
    }
}

static void check_all_devices_effects_state(void)
{
    static UINT last_ticks = 0;
    UINT ticks = pSDL_GetTicks();
    struct sdl_device *impl;

    if (ticks - last_ticks < 10) return;
    last_ticks = ticks;

    pthread_mutex_lock(&sdl_cs);
    LIST_FOR_EACH_ENTRY(impl, &device_list, struct sdl_device, unix_device.entry)
        check_device_effects_state(impl);
    pthread_mutex_unlock(&sdl_cs);
}

static BOOL set_report_from_joystick_event(struct sdl_device *impl, SDL_Event *event)
{
    struct unix_device *iface = &impl->unix_device;
    struct hid_device_state *state = &iface->hid_device_state;

    if (impl->sdl_controller) return TRUE; /* use controller events instead */

    switch (event->type)
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

            if (!hid_device_set_abs_axis(iface, ie->axis, ie->value)) break;
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        case SDL_JOYBALLMOTION:
        {
            SDL_JoyBallEvent *ie = &event->jball;

            if (!hid_device_set_rel_axis(iface, 2 * ie->ball, ie->xrel)) break;
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

    check_device_effects_state(impl);
    return FALSE;
}

static BOOL set_report_from_controller_event(struct sdl_device *impl, SDL_Event *event)
{
    struct unix_device *iface = &impl->unix_device;
    struct hid_device_state *state = &iface->hid_device_state;

    switch (event->type)
    {
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        {
            SDL_ControllerButtonEvent *ie = &event->cbutton;
            int button;

            switch (ie->button)
            {
            case SDL_CONTROLLER_BUTTON_A: button = 0; break;
            case SDL_CONTROLLER_BUTTON_B: button = 1; break;
            case SDL_CONTROLLER_BUTTON_X: button = 2; break;
            case SDL_CONTROLLER_BUTTON_Y: button = 3; break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: button = 4; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: button = 5; break;
            case SDL_CONTROLLER_BUTTON_BACK: button = 6; break;
            case SDL_CONTROLLER_BUTTON_START: button = 7; break;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: button = 8; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: button = 9; break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP: button = 10; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: button = 11; break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: button = 12; break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: button = 13; break;
            case SDL_CONTROLLER_BUTTON_GUIDE: button = 16; break;
            default: button = -1; break;
            }

            if (button == -1) break;
            if (button == 10) hid_device_move_hatswitch(iface, 0, 0, ie->state ? -1 : +1);
            if (button == 11) hid_device_move_hatswitch(iface, 0, 0, ie->state ? +1 : -1);
            if (button == 12) hid_device_move_hatswitch(iface, 0, ie->state ? -1 : +1, 0);
            if (button == 13) hid_device_move_hatswitch(iface, 0, ie->state ? +1 : -1, 0);
            hid_device_set_button(iface, button, ie->state);
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        case SDL_CONTROLLERAXISMOTION:
        {
            SDL_ControllerAxisEvent *ie = &event->caxis;

            if (ie->axis == SDL_CONTROLLER_AXIS_LEFTY || ie->axis == SDL_CONTROLLER_AXIS_RIGHTY)
                ie->value = -ie->value - 1; /* match XUSB / GIP protocol */

            hid_device_set_abs_axis(iface, ie->axis, ie->value);
            bus_event_queue_input_report(&event_queue, iface, state->report_buf, state->report_len);
            break;
        }
        default:
            ERR("TODO: Process Report (%x)\n",event->type);
    }

    check_device_effects_state(impl);
    return FALSE;
}

static void sdl_add_device(unsigned int index)
{
    struct device_desc desc =
    {
        .input = -1,
        .manufacturer = {'S','D','L',0},
        .serialnumber = {'0','0','0','0',0},
    };
    struct sdl_device *impl;

    SDL_Joystick* joystick;
    SDL_JoystickID id;
    SDL_JoystickType joystick_type;
    SDL_GameController *controller = NULL;
    const char *product, *sdl_serial;
    char guid_str[33], buffer[ARRAY_SIZE(desc.product)];
    int axis_count, axis_offset;

    if ((joystick = pSDL_JoystickOpen(index)) == NULL)
    {
        WARN("Unable to open sdl device %i: %s\n", index, pSDL_GetError());
        return;
    }

    joystick_type = pSDL_JoystickGetType(joystick);
    if (options->map_controllers && pSDL_IsGameController(index)
            && joystick_type != SDL_JOYSTICK_TYPE_WHEEL
            && joystick_type != SDL_JOYSTICK_TYPE_FLIGHT_STICK)
        controller = pSDL_GameControllerOpen(index);

    if (controller) product = pSDL_GameControllerName(controller);
    else product = pSDL_JoystickName(joystick);
    if (!product) product = "Joystick";

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

    if (pSDL_JoystickGetSerial && (sdl_serial = pSDL_JoystickGetSerial(joystick)))
    {
        ntdll_umbstowcs(sdl_serial, strlen(sdl_serial) + 1, desc.serialnumber, ARRAY_SIZE(desc.serialnumber));
    }
    else
    {
        /* Overcooked! All You Can Eat only adds controllers with unique serial numbers
         * Prefer keeping serial numbers unique over keeping them consistent across runs */
        pSDL_JoystickGetGUIDString(pSDL_JoystickGetGUID(joystick), guid_str, sizeof(guid_str));
        snprintf(buffer, sizeof(buffer), "%s.%d", guid_str, index);
        TRACE("Making up serial number for %s: %s\n", product, buffer);
        ntdll_umbstowcs(buffer, strlen(buffer) + 1, desc.serialnumber, ARRAY_SIZE(desc.serialnumber));
    }

    if (controller)
    {
        desc.is_gamepad = TRUE;
        axis_count = 6;
    }
    else
    {
        int button_count = pSDL_JoystickNumButtons(joystick);
        axis_count = pSDL_JoystickNumAxes(joystick);
        desc.is_gamepad = (axis_count == 6  && button_count >= 14);
    }

    axis_offset = 0;
    do
    {
        NTSTATUS status;

        if (!axis_offset) strcpy(buffer, product);
        else snprintf(buffer, ARRAY_SIZE(buffer), "%s %d", product, axis_offset / 6);
        ntdll_umbstowcs(buffer, strlen(buffer) + 1, desc.product, ARRAY_SIZE(desc.product));

        TRACE("%s id %d, axis_offset %u, desc %s.\n", controller ? "controller" : "joystick", id, axis_offset, debugstr_device_desc(&desc));

        if (!(impl = hid_device_create(&sdl_device_vtbl, sizeof(struct sdl_device)))) return;
        list_add_tail(&device_list, &impl->unix_device.entry);
        impl->sdl_joystick = joystick;
        impl->sdl_controller = controller;
        impl->id = id;
        impl->axis_offset = axis_offset;

        if (impl->sdl_controller) status = build_controller_report_descriptor(&impl->unix_device);
        else status = build_joystick_report_descriptor(&impl->unix_device, &desc);
        if (status)
        {
            list_remove(&impl->unix_device.entry);
            impl->unix_device.vtbl->destroy(&impl->unix_device);
            return;
        }

        bus_event_queue_device_created(&event_queue, &impl->unix_device, &desc);
        axis_offset += (options->split_controllers ? 6 : axis_count);
    }
    while (axis_offset < axis_count);
}

static void process_device_event(SDL_Event *event)
{
    struct sdl_device *impl;
    SDL_JoystickID id;

    TRACE("Received action %x\n", event->type);

    pthread_mutex_lock(&sdl_cs);

    if (event->type == SDL_JOYDEVICEADDED)
        sdl_add_device(((SDL_JoyDeviceEvent *)event)->which);
    else if (event->type == SDL_JOYDEVICEREMOVED)
    {
        id = ((SDL_JoyDeviceEvent *)event)->which;
        impl = find_device_from_id(id);
        if (impl) bus_event_queue_device_removed(&event_queue, &impl->unix_device);
        else WARN("Failed to find device with id %d\n", id);
    }
    else if (event->type == SDL_JOYAXISMOTION && options->split_controllers)
    {
        id = event->jaxis.which;
        impl = find_device_from_id_and_axis(id, event->jaxis.axis);
        if (!impl) WARN("Failed to find device with id %d for axis %d\n", id, event->jaxis.axis);
        else if (!impl->started) WARN("Device %p with id %d is stopped, ignoring event %#x\n", impl, id, event->type);
        else
        {
            event->jaxis.axis -= impl->axis_offset;
            set_report_from_joystick_event(impl, event);
        }
    }
    else if (event->type >= SDL_JOYAXISMOTION && event->type <= SDL_JOYBUTTONUP)
    {
        id = ((SDL_JoyButtonEvent *)event)->which;
        impl = find_device_from_id(id);
        if (!impl) WARN("Failed to find device with id %d\n", id);
        else if (!impl->started) WARN("Device %p with id %d is stopped, ignoring event %#x\n", impl, id, event->type);
        else set_report_from_joystick_event(impl, event);
    }
    else if (event->type >= SDL_CONTROLLERAXISMOTION && event->type <= SDL_CONTROLLERBUTTONUP)
    {
        id = ((SDL_ControllerButtonEvent *)event)->which;
        impl = find_device_from_id(id);
        if (!impl) WARN("Failed to find device with id %d\n", id);
        else if (!impl->started) WARN("Device %p with id %d is stopped, ignoring event %#x\n", impl, id, event->type);
        else set_report_from_controller_event(impl, event);
    }

    pthread_mutex_unlock(&sdl_cs);
}

NTSTATUS sdl_bus_init(void *args)
{
    const char *mapping;
    int i;

    TRACE("args %p\n", args);

    options = (struct bus_options *)args;

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
    LOAD_FUNCPTR(SDL_WaitEventTimeout);
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
    LOAD_FUNCPTR(SDL_HapticGetEffectStatus);
    LOAD_FUNCPTR(SDL_HapticNewEffect);
    LOAD_FUNCPTR(SDL_HapticNumAxes);
    LOAD_FUNCPTR(SDL_HapticOpenFromJoystick);
    LOAD_FUNCPTR(SDL_HapticPause);
    LOAD_FUNCPTR(SDL_HapticQuery);
    LOAD_FUNCPTR(SDL_HapticRumbleInit);
    LOAD_FUNCPTR(SDL_HapticRumblePlay);
    LOAD_FUNCPTR(SDL_HapticRumbleStop);
    LOAD_FUNCPTR(SDL_HapticRumbleSupported);
    LOAD_FUNCPTR(SDL_HapticRunEffect);
    LOAD_FUNCPTR(SDL_HapticSetGain);
    LOAD_FUNCPTR(SDL_HapticSetAutocenter);
    LOAD_FUNCPTR(SDL_HapticStopAll);
    LOAD_FUNCPTR(SDL_HapticStopEffect);
    LOAD_FUNCPTR(SDL_HapticUnpause);
    LOAD_FUNCPTR(SDL_HapticUpdateEffect);
    LOAD_FUNCPTR(SDL_JoystickIsHaptic);
    LOAD_FUNCPTR(SDL_GameControllerAddMapping);
    LOAD_FUNCPTR(SDL_RegisterEvents);
    LOAD_FUNCPTR(SDL_PushEvent);
    LOAD_FUNCPTR(SDL_GetTicks);
#undef LOAD_FUNCPTR
    pSDL_JoystickRumble = dlsym(sdl_handle, "SDL_JoystickRumble");
    pSDL_JoystickRumbleTriggers = dlsym(sdl_handle, "SDL_JoystickRumbleTriggers");
    pSDL_JoystickGetProduct = dlsym(sdl_handle, "SDL_JoystickGetProduct");
    pSDL_JoystickGetProductVersion = dlsym(sdl_handle, "SDL_JoystickGetProductVersion");
    pSDL_JoystickGetVendor = dlsym(sdl_handle, "SDL_JoystickGetVendor");
    pSDL_JoystickGetType = dlsym(sdl_handle, "SDL_JoystickGetType");
    pSDL_JoystickGetSerial = dlsym(sdl_handle, "SDL_JoystickGetSerial");

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
        else for (i = 0; i < options->mappings_count; ++i)
        {
            TRACE("Setting registry mapping %s\n", debugstr_a(options->mappings[i]));
            if (pSDL_GameControllerAddMapping(options->mappings[i]) < 0)
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
        if (pSDL_WaitEventTimeout(&event, 10) != 0) process_device_event(&event);
        else check_all_devices_effects_state();
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
