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

#include "waylanddrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

static void text_input_enter(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        struct wl_surface *surface)
{
    struct wayland_text_input *text_input = data;
    TRACE("data %p, text_input %p, surface %p.\n", data, zwp_text_input_v3, surface);

    pthread_mutex_lock(&text_input->mutex);
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
    TRACE("data %p, text_input %p, surface %p.\n", data, zwp_text_input_v3, surface);

    pthread_mutex_lock(&text_input->mutex);
    zwp_text_input_v3_disable(text_input->zwp_text_input_v3);
    zwp_text_input_v3_commit(text_input->zwp_text_input_v3);
    pthread_mutex_unlock(&text_input->mutex);
}

static void text_input_preedit_string(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        const char *text, int32_t cursor_begin, int32_t cursor_end)
{
}

static void text_input_commit_string(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        const char *text)
{
}

static void text_input_delete_surrounding_text(void *data,
        struct zwp_text_input_v3 *zwp_text_input_v3, uint32_t before_length, uint32_t after_length)
{
}

static void text_input_done(void *data, struct zwp_text_input_v3 *zwp_text_input_v3,
        uint32_t serial)
{
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
    pthread_mutex_unlock(&text_input->mutex);
};
