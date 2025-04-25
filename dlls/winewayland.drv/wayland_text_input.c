/*
 * Wayland text input handling
 *
 * Copyright 2025 Attila Fidan
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "waylanddrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

static void post_ime_update(HWND hwnd, UINT cursor_pos, WCHAR *comp_str, WCHAR *result_str)
{
    /* Windows uses an empty string to clear the composition string. */
    if (!comp_str && !result_str) comp_str = (WCHAR *)L"";

    NtUserMessageCall(hwnd, WINE_IME_POST_UPDATE, cursor_pos, (LPARAM)comp_str, result_str,
            NtUserImeDriverCall, FALSE);
}

static WCHAR *strdupUtoW(const char *str)
{
    WCHAR *ret = NULL;
    size_t len;
    DWORD reslen;

    if (!str) return ret;
    len = strlen(str);
    ret = malloc((len + 1) * sizeof(WCHAR));
    if (ret)
    {
        RtlUTF8ToUnicodeN(ret, len * sizeof(WCHAR), &reslen, str, len);
        reslen /= sizeof(WCHAR);
        ret[reslen] = 0;
    }
    return ret;
}

static void text_input_enter(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        struct wl_surface *surface)
{
    struct wayland_text_input *text_input = data;
    HWND hwnd;

    if (!surface) return;

    hwnd = wl_surface_get_user_data(surface);
    TRACE("data %p, text_input %p, hwnd %p.\n", data, zwp_text_input_v3, hwnd);

    pthread_mutex_lock(&text_input->mutex);
    text_input->focused_hwnd = hwnd;
    zwp_text_input_v3_enable(text_input->zwp_text_input_v3);
    zwp_text_input_v3_set_content_type(text_input->zwp_text_input_v3,
            ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE,
            ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL);
    zwp_text_input_v3_set_cursor_rectangle(text_input->zwp_text_input_v3, 0, 0, 0, 0);
    zwp_text_input_v3_commit(text_input->zwp_text_input_v3);
    pthread_mutex_unlock(&text_input->mutex);
}

static void text_input_leave(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        struct wl_surface *surface)
{
    struct wayland_text_input *text_input = data;
    TRACE("data %p, text_input %p.\n", data, zwp_text_input_v3);

    pthread_mutex_lock(&text_input->mutex);
    zwp_text_input_v3_disable(text_input->zwp_text_input_v3);
    zwp_text_input_v3_commit(text_input->zwp_text_input_v3);
    if (text_input->focused_hwnd)
    {
        post_ime_update(text_input->focused_hwnd, 0, NULL, NULL);
        text_input->focused_hwnd = NULL;
    }
    pthread_mutex_unlock(&text_input->mutex);
}

static void text_input_preedit_string(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        const char *text, int32_t cursor_begin, int32_t cursor_end)
{
    struct wayland_text_input *text_input = data;
    TRACE("data %p, text_input %p, text %s, cursor %d - %d.\n", data, zwp_text_input_v3,
            debugstr_a(text), cursor_begin, cursor_end);

    pthread_mutex_lock(&text_input->mutex);
    if ((text_input->preedit_string = strdupUtoW(text)))
    {
        DWORD begin = 0, end = 0;

        if (cursor_begin > 0) RtlUTF8ToUnicodeN(NULL, 0, &begin, text, cursor_begin);
        if (cursor_end > 0) RtlUTF8ToUnicodeN(NULL, 0, &end, text, cursor_end);
        text_input->preedit_cursor_pos = MAKELONG(begin / sizeof(WCHAR), end / sizeof(WCHAR));
    }
    pthread_mutex_unlock(&text_input->mutex);
}

static void text_input_commit_string(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        const char *text)
{
    struct wayland_text_input *text_input = data;
    TRACE("data %p, text_input %p, text %s.\n", data, zwp_text_input_v3, debugstr_a(text));

    pthread_mutex_lock(&text_input->mutex);
    text_input->commit_string = strdupUtoW(text);
    pthread_mutex_unlock(&text_input->mutex);
}

static void text_input_delete_surrounding_text(void *data,
        struct zwp_text_input_v3 *zwp_text_input_v3, uint32_t before_length, uint32_t after_length)
{
}

static void text_input_done(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        uint32_t serial)
{
    struct wayland_text_input *text_input = data;
    TRACE("data %p, text_input %p, serial %u.\n", data, zwp_text_input_v3, serial);

    pthread_mutex_lock(&text_input->mutex);
    /* Some compositors will send a done event for every commit, regardless of
     * the focus state of the text input. This behavior is arguably out of spec,
     * but otherwise harmless, so just ignore the new state in such cases. */
    if (text_input->focused_hwnd)
    {
        post_ime_update(text_input->focused_hwnd, text_input->preedit_cursor_pos,
                text_input->preedit_string, text_input->commit_string);
    }

    free(text_input->preedit_string);
    text_input->preedit_string = NULL;
    text_input->preedit_cursor_pos = 0;
    free(text_input->commit_string);
    text_input->commit_string = NULL;
    pthread_mutex_unlock(&text_input->mutex);
}

static const struct zwp_text_input_v3_listener text_input_listener =
{
    text_input_enter,
    text_input_leave,
    text_input_preedit_string,
    text_input_commit_string,
    text_input_delete_surrounding_text,
    text_input_done,
};

void wayland_text_input_init(void)
{
    struct wayland_text_input *text_input = &process_wayland.text_input;

    pthread_mutex_lock(&text_input->mutex);
    text_input->zwp_text_input_v3 = zwp_text_input_manager_v3_get_text_input(
            process_wayland.zwp_text_input_manager_v3, process_wayland.seat.wl_seat);
    zwp_text_input_v3_add_listener(text_input->zwp_text_input_v3, &text_input_listener, text_input);
    pthread_mutex_unlock(&text_input->mutex);
};

void wayland_text_input_deinit(void)
{
    struct wayland_text_input *text_input = &process_wayland.text_input;

    pthread_mutex_lock(&text_input->mutex);
    zwp_text_input_v3_destroy(text_input->zwp_text_input_v3);
    text_input->zwp_text_input_v3 = NULL;
    text_input->focused_hwnd = NULL;
    pthread_mutex_unlock(&text_input->mutex);
};

/***********************************************************************
 *      SetIMECompositionRect (WAYLANDDRV.@)
 */
BOOL WAYLAND_SetIMECompositionRect(HWND hwnd, RECT rect)
{
    struct wayland_text_input *text_input = &process_wayland.text_input;
    struct wayland_win_data *data;
    struct wayland_surface *surface;
    int cursor_x, cursor_y, cursor_width, cursor_height;
    TRACE("hwnd %p, rect %s.\n", hwnd, wine_dbgstr_rect(&rect));

    pthread_mutex_lock(&text_input->mutex);

    if (!text_input->zwp_text_input_v3 || hwnd != text_input->focused_hwnd)
        goto err;

    if (!(data = wayland_win_data_get(hwnd)))
        goto err;

    if (!(surface = data->wayland_surface))
    {
        wayland_win_data_release(data);
        goto err;
    }

    wayland_surface_coords_from_window(surface,
            rect.left - surface->window.rect.left,
            rect.top - surface->window.rect.top,
            &cursor_x, &cursor_y);
    wayland_surface_coords_from_window(surface,
            rect.right - rect.left,
            rect.bottom - rect.top,
            &cursor_width, &cursor_height);
    wayland_win_data_release(data);

    zwp_text_input_v3_set_cursor_rectangle(text_input->zwp_text_input_v3,
            cursor_x, cursor_y, cursor_width, cursor_height);
    zwp_text_input_v3_commit(text_input->zwp_text_input_v3);

    pthread_mutex_unlock(&text_input->mutex);
    return TRUE;

err:
    pthread_mutex_unlock(&text_input->mutex);
    return FALSE;
}
