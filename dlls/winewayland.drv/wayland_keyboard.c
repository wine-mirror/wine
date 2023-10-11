/*
 * Keyboard related functions
 *
 * Copyright 2020 Alexandros Frantzis for Collabora Ltd.
 * Copyright 2023 Rémi Bernon for CodeWeavers
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

#include <linux/input.h>
#undef SW_MAX /* Also defined in winuser.rh */
#include <unistd.h>

#include "waylanddrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(keyboard);
WINE_DECLARE_DEBUG_CHANNEL(key);

static WORD key2scan(UINT key)
{
    /* base keys can be mapped directly */
    if (key <= KEY_KPDOT) return key;

    /* map keys found in KBDTABLES definitions (Txx Xxx Yxx macros) */
    switch (key)
    {
    case 84 /* ISO_Level3_Shift */: return 0x005a; /* T5A / VK_OEM_WSCTRL */
    case KEY_SYSRQ: return 0x0054; /* T54 / VK_SNAPSHOT */
    case KEY_102ND: return 0x0056; /* T56 / VK_OEM_102 */
    case KEY_F11: return 0x0057; /* T57 / VK_F11 */
    case KEY_F12: return 0x0058; /* T58 / VK_F12 */
    case KEY_LINEFEED: return 0x0059; /* T59 / VK_CLEAR */
    case KEY_EXIT: return 0x005b; /* T5B / VK_OEM_FINISH */
    case KEY_OPEN: return 0x005c; /* T5C / VK_OEM_JUMP */
    /* FIXME: map a KEY to T5D / VK_EREOF */
    /* FIXME: map a KEY to T5E / VK_OEM_BACKTAB */
    case KEY_COMPOSE: return 0x005f; /* T5F / VK_OEM_AUTO */
    case KEY_SCALE: return 0x0062; /* T62 / VK_ZOOM */
    case KEY_HELP: return 0x0063; /* T63 / VK_HELP */
    case KEY_F13: return 0x0064; /* T64 / VK_F13 */
    case KEY_F14: return 0x0065; /* T65 / VK_F14 */
    case KEY_F15: return 0x0066; /* T66 / VK_F15 */
    case KEY_F16: return 0x0067; /* T67 / VK_F16 */
    case KEY_F17: return 0x0068; /* T68 / VK_F17 */
    case KEY_F18: return 0x0069; /* T69 / VK_F18 */
    case KEY_F19: return 0x006a; /* T6A / VK_F19 */
    case KEY_F20: return 0x006b; /* T6B / VK_F20 */
    case KEY_F21: return 0x006c; /* T6C / VK_F21 */
    case KEY_F22: return 0x006d; /* T6D / VK_F22 */
    case KEY_F23: return 0x006e; /* T6E / VK_F23 */
    /* FIXME: map a KEY to T6F / VK_OEM_PA3 */
    case KEY_COMPUTER: return 0x0071; /* T71 / VK_OEM_RESET */
    /* FIXME: map a KEY to T73 / VK_ABNT_C1 */
    case KEY_F24: return 0x0076; /* T76 / VK_F24 */
    case KEY_KPPLUSMINUS: return 0x007b; /* T7B / VK_OEM_PA1 */
    /* FIXME: map a KEY to T7C / VK_TAB */
    /* FIXME: map a KEY to T7E / VK_ABNT_C2 */
    /* FIXME: map a KEY to T7F / VK_OEM_PA2 */
    case KEY_PREVIOUSSONG: return 0x0110; /* X10 / VK_MEDIA_PREV_TRACK */
    case KEY_NEXTSONG: return 0x0119; /* X19 / VK_MEDIA_NEXT_TRACK */
    case KEY_KPENTER: return 0x011c; /* X1C / VK_RETURN */
    case KEY_RIGHTCTRL: return 0x011d; /* X1D / VK_RCONTROL */
    case KEY_MUTE: return 0x0120; /* X20 / VK_VOLUME_MUTE */
    case KEY_PROG2: return 0x0121; /* X21 / VK_LAUNCH_APP2 */
    case KEY_PLAYPAUSE: return 0x0122; /* X22 / VK_MEDIA_PLAY_PAUSE */
    case KEY_STOPCD: return 0x0124; /* X24 / VK_MEDIA_STOP */
    case KEY_VOLUMEDOWN: return 0x012e; /* X2E / VK_VOLUME_DOWN */
    case KEY_VOLUMEUP: return 0x0130; /* X30 / VK_VOLUME_UP */
    case KEY_HOMEPAGE: return 0x0132; /* X32 / VK_BROWSER_HOME */
    case KEY_KPSLASH: return 0x0135; /* X35 / VK_DIVIDE */
    case KEY_PRINT: return 0x0137; /* X37 / VK_SNAPSHOT */
    case KEY_RIGHTALT: return 0x0138; /* X38 / VK_RMENU */
    case KEY_CANCEL: return 0x0146; /* X46 / VK_CANCEL */
    case KEY_HOME: return 0x0147; /* X47 / VK_HOME */
    case KEY_UP: return 0x0148; /* X48 / VK_UP */
    case KEY_PAGEUP: return 0x0149; /* X49 / VK_PRIOR */
    case KEY_LEFT: return 0x014b; /* X4B / VK_LEFT */
    case KEY_RIGHT: return 0x014d; /* X4D / VK_RIGHT */
    case KEY_END: return 0x014f; /* X4F / VK_END */
    case KEY_DOWN: return 0x0150; /* X50 / VK_DOWN */
    case KEY_PAGEDOWN: return 0x0151; /* X51 / VK_NEXT */
    case KEY_INSERT: return 0x0152; /* X52 / VK_INSERT */
    case KEY_DELETE: return 0x0153; /* X53 / VK_DELETE */
    case KEY_LEFTMETA: return 0x015b; /* X5B / VK_LWIN */
    case KEY_RIGHTMETA: return 0x015c; /* X5C / VK_RWIN */
    case KEY_MENU: return 0x015d; /* X5D / VK_APPS */
    case KEY_POWER: return 0x015e; /* X5E / VK_POWER */
    case KEY_SLEEP: return 0x015f; /* X5F / VK_SLEEP */
    case KEY_FIND: return 0x0165; /* X65 / VK_BROWSER_SEARCH */
    case KEY_BOOKMARKS: return 0x0166; /* X66 / VK_BROWSER_FAVORITES */
    case KEY_REFRESH: return 0x0167; /* X67 / VK_BROWSER_REFRESH */
    case KEY_STOP: return 0x0168; /* X68 / VK_BROWSER_STOP */
    case KEY_FORWARD: return 0x0169; /* X69 / VK_BROWSER_FORWARD */
    case KEY_BACK: return 0x016a; /* X6A / VK_BROWSER_BACK */
    case KEY_PROG1: return 0x016b; /* X6B / VK_LAUNCH_APP1 */
    case KEY_MAIL: return 0x016c; /* X6C / VK_LAUNCH_MAIL */
    case KEY_MEDIA: return 0x016d; /* X6D / VK_LAUNCH_MEDIA_SELECT */
    case KEY_PAUSE: return 0x021d; /* Y1D / VK_PAUSE */
    }

    /* otherwise just make up some extended scancode */
    return 0x200 | (key & 0x7f);
}

/**********************************************************************
 *          Keyboard handling
 */

static HWND wayland_keyboard_get_focused_hwnd(void)
{
    struct wayland_keyboard *keyboard = &process_wayland.keyboard;
    HWND hwnd;

    pthread_mutex_lock(&keyboard->mutex);
    hwnd = keyboard->focused_hwnd;
    pthread_mutex_unlock(&keyboard->mutex);

    return hwnd;
}

static void keyboard_handle_keymap(void *data, struct wl_keyboard *wl_keyboard,
                                   uint32_t format, int fd, uint32_t size)
{
    FIXME("stub!\n");
    close(fd);
}

static void keyboard_handle_enter(void *data, struct wl_keyboard *wl_keyboard,
                                  uint32_t serial, struct wl_surface *wl_surface,
                                  struct wl_array *keys)
{
    struct wayland_keyboard *keyboard = &process_wayland.keyboard;
    HWND hwnd;

    if (!wl_surface) return;

    /* The wl_surface user data remains valid and immutable for the whole
     * lifetime of the object, so it's safe to access without locking. */
    hwnd = wl_surface_get_user_data(wl_surface);
    TRACE("serial=%u hwnd=%p\n", serial, hwnd);

    pthread_mutex_lock(&keyboard->mutex);
    keyboard->focused_hwnd = hwnd;
    pthread_mutex_unlock(&keyboard->mutex);

    /* FIXME: update foreground window as well */
}

static void keyboard_handle_leave(void *data, struct wl_keyboard *wl_keyboard,
                                  uint32_t serial, struct wl_surface *wl_surface)
{
    struct wayland_keyboard *keyboard = &process_wayland.keyboard;
    HWND hwnd;

    if (!wl_surface) return;

    /* The wl_surface user data remains valid and immutable for the whole
     * lifetime of the object, so it's safe to access without locking. */
    hwnd = wl_surface_get_user_data(wl_surface);
    TRACE("serial=%u hwnd=%p\n", serial, hwnd);

    pthread_mutex_lock(&keyboard->mutex);
    if (keyboard->focused_hwnd == hwnd)
        keyboard->focused_hwnd = NULL;
    pthread_mutex_unlock(&keyboard->mutex);

    /* FIXME: update foreground window as well */
}

static void keyboard_handle_key(void *data, struct wl_keyboard *wl_keyboard,
                                uint32_t serial, uint32_t time, uint32_t key,
                                uint32_t state)
{
    UINT scan = key2scan(key);
    INPUT input = {0};
    HWND hwnd;

    if (!(hwnd = wayland_keyboard_get_focused_hwnd())) return;

    TRACE_(key)("serial=%u hwnd=%p key=%d scan=%#x state=%#x\n", serial, hwnd, key, scan, state);

    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scan & 0xff;
    input.ki.wVk = NtUserMapVirtualKeyEx(scan, MAPVK_VSC_TO_VK_EX, NtUserGetKeyboardLayout(0));
    if (scan & ~0xff) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED) input.ki.dwFlags |= KEYEVENTF_KEYUP;
    __wine_send_input(hwnd, &input, NULL);
}

static void keyboard_handle_modifiers(void *data, struct wl_keyboard *wl_keyboard,
                                      uint32_t serial, uint32_t mods_depressed,
                                      uint32_t mods_latched, uint32_t mods_locked,
                                      uint32_t xkb_group)
{
    FIXME("serial=%u mods_depressed=%#x mods_latched=%#x mods_locked=%#x xkb_group=%d stub!\n",
          serial, mods_depressed, mods_latched, mods_locked, xkb_group);
}

static void keyboard_handle_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                                        int rate, int delay)
{
    UINT speed;

    TRACE("rate=%d delay=%d\n", rate, delay);

    /* Handle non-negative rate values, ignore invalid (negative) values.  A
     * rate of 0 disables repeat. */
    if (rate >= 80) speed = 31;
    else if (rate >= 5) speed = rate * 400 / 1000 - 1;
    else speed = 0;

    delay = max(0, min(3, round(delay / 250.0) - 1));
    NtUserSystemParametersInfo(SPI_SETKEYBOARDSPEED, speed, NULL, 0);
    NtUserSystemParametersInfo(SPI_SETKEYBOARDDELAY, delay, NULL, 0);
    NtUserCallOneParam(rate > 0, NtUserCallOneParam_SetKeyboardAutoRepeat);
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap,
    keyboard_handle_enter,
    keyboard_handle_leave,
    keyboard_handle_key,
    keyboard_handle_modifiers,
    keyboard_handle_repeat_info,
};

/***********************************************************************
 *           wayland_keyboard_init
 */
void wayland_keyboard_init(struct wl_keyboard *wl_keyboard)
{
    struct wayland_keyboard *keyboard = &process_wayland.keyboard;
    struct xkb_context *xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (!xkb_context)
    {
        ERR("Failed to create XKB context\n");
        return;
    }

    NtUserCallOneParam(TRUE, NtUserCallOneParam_SetKeyboardAutoRepeat);
    pthread_mutex_lock(&keyboard->mutex);
    keyboard->wl_keyboard = wl_keyboard;
    keyboard->xkb_context = xkb_context;
    pthread_mutex_unlock(&keyboard->mutex);
    wl_keyboard_add_listener(keyboard->wl_keyboard, &keyboard_listener, NULL);
}

/***********************************************************************
 *           wayland_keyboard_deinit
 */
void wayland_keyboard_deinit(void)
{
    struct wayland_keyboard *keyboard = &process_wayland.keyboard;

    pthread_mutex_lock(&keyboard->mutex);
    if (keyboard->wl_keyboard)
    {
        wl_keyboard_destroy(keyboard->wl_keyboard);
        keyboard->wl_keyboard = NULL;
    }
    if (keyboard->xkb_context)
    {
        xkb_context_unref(keyboard->xkb_context);
        keyboard->xkb_context = NULL;
    }
    pthread_mutex_unlock(&keyboard->mutex);
}
