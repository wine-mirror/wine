/*
 * Wayland pointer handling
 *
 * Copyright (c) 2020 Alexandros Frantzis for Collabora Ltd
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
#include <math.h>
#include <stdlib.h>

#include "waylanddrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

static HWND wayland_pointer_get_focused_hwnd(void)
{
    struct wayland_pointer *pointer = &process_wayland.pointer;
    HWND hwnd;

    pthread_mutex_lock(&pointer->mutex);
    hwnd = pointer->focused_hwnd;
    pthread_mutex_unlock(&pointer->mutex);

    return hwnd;
}

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    INPUT input = {0};
    RECT window_rect;
    HWND hwnd;
    int screen_x, screen_y;

    if (!(hwnd = wayland_pointer_get_focused_hwnd())) return;
    if (!NtUserGetWindowRect(hwnd, &window_rect)) return;

    screen_x = round(wl_fixed_to_double(sx)) + window_rect.left;
    screen_y = round(wl_fixed_to_double(sy)) + window_rect.top;
    /* Sometimes, due to rounding, we may end up with pointer coordinates
     * slightly outside the target window, so bring them within bounds. */
    if (screen_x >= window_rect.right) screen_x = window_rect.right - 1;
    else if (screen_x < window_rect.left) screen_x = window_rect.left;
    if (screen_y >= window_rect.bottom) screen_y = window_rect.bottom - 1;
    else if (screen_y < window_rect.top) screen_y = window_rect.top;

    input.type = INPUT_MOUSE;
    input.mi.dx = screen_x;
    input.mi.dy = screen_y;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    TRACE("hwnd=%p wayland_xy=%.2f,%.2f screen_xy=%d,%d\n",
          hwnd, wl_fixed_to_double(sx), wl_fixed_to_double(sy),
          screen_x, screen_y);

    __wine_send_input(hwnd, &input, NULL);
}

static void pointer_handle_enter(void *data, struct wl_pointer *wl_pointer,
                                 uint32_t serial, struct wl_surface *wl_surface,
                                 wl_fixed_t sx, wl_fixed_t sy)
{
    struct wayland_pointer *pointer = &process_wayland.pointer;
    HWND hwnd;

    if (!wl_surface) return;
    /* The wl_surface user data remains valid and immutable for the whole
     * lifetime of the object, so it's safe to access without locking. */
    hwnd = wl_surface_get_user_data(wl_surface);

    TRACE("hwnd=%p\n", hwnd);

    pthread_mutex_lock(&pointer->mutex);
    pointer->focused_hwnd = hwnd;
    pointer->enter_serial = serial;
    /* The cursor is undefined at every enter, so we set it again with
     * the latest information we have. */
    wl_pointer_set_cursor(pointer->wl_pointer,
                          pointer->enter_serial,
                          pointer->cursor.wl_surface,
                          pointer->cursor.hotspot_x,
                          pointer->cursor.hotspot_y);
    pthread_mutex_unlock(&pointer->mutex);

    /* Handle the enter as a motion, to account for cases where the
     * window first appears beneath the pointer and won't get a separate
     * motion event. */
    pointer_handle_motion(data, wl_pointer, 0, sx, sy);
}

static void pointer_handle_leave(void *data, struct wl_pointer *wl_pointer,
                                 uint32_t serial, struct wl_surface *wl_surface)
{
    struct wayland_pointer *pointer = &process_wayland.pointer;

    if (!wl_surface) return;

    TRACE("hwnd=%p\n", wl_surface_get_user_data(wl_surface));

    pthread_mutex_lock(&pointer->mutex);
    pointer->focused_hwnd = NULL;
    pointer->enter_serial = 0;
    pthread_mutex_unlock(&pointer->mutex);
}

static void pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t serial, uint32_t time, uint32_t button,
                                  uint32_t state)
{
    struct wayland_pointer *pointer = &process_wayland.pointer;
    INPUT input = {0};
    HWND hwnd;

    if (!(hwnd = wayland_pointer_get_focused_hwnd())) return;

    input.type = INPUT_MOUSE;

    switch (button)
    {
    case BTN_LEFT: input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; break;
    case BTN_RIGHT: input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; break;
    case BTN_MIDDLE: input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
    default: break;
    }

    if (state == WL_POINTER_BUTTON_STATE_RELEASED) input.mi.dwFlags <<= 1;

    pthread_mutex_lock(&pointer->mutex);
    pointer->button_serial = state == WL_POINTER_BUTTON_STATE_PRESSED ?
                             serial : 0;
    pthread_mutex_unlock(&pointer->mutex);

    TRACE("hwnd=%p button=%#x state=%u\n", hwnd, button, state);

    __wine_send_input(hwnd, &input, NULL);
}

static void pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
                                uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static void pointer_handle_frame(void *data, struct wl_pointer *wl_pointer)
{
}

static void pointer_handle_axis_source(void *data, struct wl_pointer *wl_pointer,
                                       uint32_t axis_source)
{
}

static void pointer_handle_axis_stop(void *data, struct wl_pointer *wl_pointer,
                                     uint32_t time, uint32_t axis)
{
}

static void pointer_handle_axis_discrete(void *data, struct wl_pointer *wl_pointer,
                                         uint32_t axis, int32_t discrete)
{
    INPUT input = {0};
    HWND hwnd;

    if (!(hwnd = wayland_pointer_get_focused_hwnd())) return;

    input.type = INPUT_MOUSE;

    switch (axis)
    {
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = -WHEEL_DELTA * discrete;
        break;
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
        input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
        input.mi.mouseData = WHEEL_DELTA * discrete;
        break;
    default: break;
    }

    TRACE("hwnd=%p axis=%u discrete=%d\n", hwnd, axis, discrete);

    __wine_send_input(hwnd, &input, NULL);
}

static const struct wl_pointer_listener pointer_listener =
{
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
    pointer_handle_frame,
    pointer_handle_axis_source,
    pointer_handle_axis_stop,
    pointer_handle_axis_discrete
};

void wayland_pointer_init(struct wl_pointer *wl_pointer)
{
    struct wayland_pointer *pointer = &process_wayland.pointer;

    pthread_mutex_lock(&pointer->mutex);
    pointer->wl_pointer = wl_pointer;
    pointer->focused_hwnd = NULL;
    pointer->enter_serial = 0;
    pthread_mutex_unlock(&pointer->mutex);
    wl_pointer_add_listener(pointer->wl_pointer, &pointer_listener, NULL);
}

void wayland_pointer_deinit(void)
{
    struct wayland_pointer *pointer = &process_wayland.pointer;

    pthread_mutex_lock(&pointer->mutex);
    wl_pointer_release(pointer->wl_pointer);
    pointer->wl_pointer = NULL;
    pointer->focused_hwnd = NULL;
    pointer->enter_serial = 0;
    pthread_mutex_unlock(&pointer->mutex);
}

/***********************************************************************
 *           create_mono_cursor_buffer
 *
 * Create a wayland_shm_buffer for a monochrome cursor bitmap.
 *
 * Adapted from wineandroid.drv code.
 */
static struct wayland_shm_buffer *create_mono_cursor_buffer(HBITMAP bmp)
{
    struct wayland_shm_buffer *shm_buffer = NULL;
    BITMAP bm;
    char *mask = NULL;
    unsigned int i, j, stride, mask_size, *ptr;

    if (!NtGdiExtGetObjectW(bmp, sizeof(bm), &bm)) goto done;
    stride = ((bm.bmWidth + 15) >> 3) & ~1;
    mask_size = stride * bm.bmHeight;
    if (!(mask = malloc(mask_size))) goto done;
    if (!NtGdiGetBitmapBits(bmp, mask_size, mask)) goto done;

    bm.bmHeight /= 2;
    shm_buffer = wayland_shm_buffer_create(bm.bmWidth, bm.bmHeight,
                                           WL_SHM_FORMAT_ARGB8888);
    if (!shm_buffer) goto done;

    ptr = shm_buffer->map_data;
    for (i = 0; i < bm.bmHeight; i++)
    {
        for (j = 0; j < bm.bmWidth; j++, ptr++)
        {
            int and = ((mask[i * stride + j / 8] << (j % 8)) & 0x80);
            int xor = ((mask[(i + bm.bmHeight) * stride + j / 8] << (j % 8)) & 0x80);
            if (!xor && and)
                *ptr = 0;
            else if (xor && !and)
                *ptr = 0xffffffff;
            else
                /* we can't draw "invert" pixels, so render them as black instead */
                *ptr = 0xff000000;
        }
    }

done:
    free(mask);
    return shm_buffer;
}

/***********************************************************************
 *           create_color_cursor_buffer
 *
 * Create a wayland_shm_buffer for a color cursor bitmap.
 *
 * Adapted from wineandroid.drv code.
 */
static struct wayland_shm_buffer *create_color_cursor_buffer(HDC hdc, HBITMAP color,
                                                             HBITMAP mask)
{
    struct wayland_shm_buffer *shm_buffer = NULL;
    char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    BITMAP bm;
    unsigned int *ptr, *bits = NULL;
    unsigned char *mask_bits = NULL;
    int i, j;
    BOOL has_alpha = FALSE;

    if (!NtGdiExtGetObjectW(color, sizeof(bm), &bm)) goto failed;

    shm_buffer = wayland_shm_buffer_create(bm.bmWidth, bm.bmHeight,
                                           WL_SHM_FORMAT_ARGB8888);
    if (!shm_buffer) goto failed;
    bits = shm_buffer->map_data;

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = bm.bmWidth;
    info->bmiHeader.biHeight = -bm.bmHeight;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 4;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;

    if (!NtGdiGetDIBitsInternal(hdc, color, 0, bm.bmHeight, bits, info,
                                DIB_RGB_COLORS, 0, 0))
        goto failed;

    for (i = 0; i < bm.bmWidth * bm.bmHeight; i++)
        if ((has_alpha = (bits[i] & 0xff000000) != 0)) break;

    if (!has_alpha)
    {
        unsigned int width_bytes = (bm.bmWidth + 31) / 32 * 4;
        /* generate alpha channel from the mask */
        info->bmiHeader.biBitCount = 1;
        info->bmiHeader.biSizeImage = width_bytes * bm.bmHeight;
        if (!(mask_bits = malloc(info->bmiHeader.biSizeImage))) goto failed;
        if (!NtGdiGetDIBitsInternal(hdc, mask, 0, bm.bmHeight, mask_bits,
                                    info, DIB_RGB_COLORS, 0, 0))
            goto failed;
        ptr = bits;
        for (i = 0; i < bm.bmHeight; i++)
        {
            for (j = 0; j < bm.bmWidth; j++, ptr++)
            {
                if (!((mask_bits[i * width_bytes + j / 8] << (j % 8)) & 0x80))
                    *ptr |= 0xff000000;
            }
        }
        free(mask_bits);
    }

    /* Wayland requires pre-multiplied alpha values */
    for (ptr = bits, i = 0; i < bm.bmWidth * bm.bmHeight; ptr++, i++)
    {
        unsigned char alpha = *ptr >> 24;
        if (alpha == 0)
        {
            *ptr = 0;
        }
        else if (alpha != 255)
        {
            *ptr = (alpha << 24) |
                   (((BYTE)(*ptr >> 16) * alpha / 255) << 16) |
                   (((BYTE)(*ptr >> 8) * alpha / 255) << 8) |
                   (((BYTE)*ptr * alpha / 255));
        }
    }

    return shm_buffer;

failed:
    if (shm_buffer) wayland_shm_buffer_unref(shm_buffer);
    free(mask_bits);
    return NULL;
}

/***********************************************************************
 *           get_icon_info
 *
 * Local GetIconInfoExW helper implementation.
 */
static BOOL get_icon_info(HICON handle, ICONINFOEXW *ret)
{
    UNICODE_STRING module, res_name;
    ICONINFO info;

    module.Buffer = ret->szModName;
    module.MaximumLength = sizeof(ret->szModName) - sizeof(WCHAR);
    res_name.Buffer = ret->szResName;
    res_name.MaximumLength = sizeof(ret->szResName) - sizeof(WCHAR);
    if (!NtUserGetIconInfo(handle, &info, &module, &res_name, NULL, 0)) return FALSE;
    ret->fIcon = info.fIcon;
    ret->xHotspot = info.xHotspot;
    ret->yHotspot = info.yHotspot;
    ret->hbmColor = info.hbmColor;
    ret->hbmMask = info.hbmMask;
    ret->wResID = res_name.Length ? 0 : LOWORD(res_name.Buffer);
    ret->szModName[module.Length] = 0;
    ret->szResName[res_name.Length] = 0;
    return TRUE;
}

static void wayland_pointer_update_cursor(HCURSOR hcursor)
{
    struct wayland_cursor *cursor = &process_wayland.pointer.cursor;
    ICONINFOEXW info = {0};

    if (!hcursor) goto clear_cursor;

    /* Create a new buffer for the specified cursor. */
    if (cursor->shm_buffer)
    {
        wayland_shm_buffer_unref(cursor->shm_buffer);
        cursor->shm_buffer = NULL;
    }

    if (!get_icon_info(hcursor, &info))
    {
        ERR("Failed to get icon info for cursor=%p\n", hcursor);
        goto clear_cursor;
    }

    if (info.hbmColor)
    {
        HDC hdc = NtGdiCreateCompatibleDC(0);
        cursor->shm_buffer =
            create_color_cursor_buffer(hdc, info.hbmColor, info.hbmMask);
        NtGdiDeleteObjectApp(hdc);
    }
    else
    {
        cursor->shm_buffer = create_mono_cursor_buffer(info.hbmMask);
    }

    if (info.hbmColor) NtGdiDeleteObjectApp(info.hbmColor);
    if (info.hbmMask) NtGdiDeleteObjectApp(info.hbmMask);

    cursor->hotspot_x = info.xHotspot;
    cursor->hotspot_y = info.yHotspot;

    if (!cursor->shm_buffer)
    {
        ERR("Failed to create shm_buffer for cursor=%p\n", hcursor);
        goto clear_cursor;
    }

    /* Make sure the hotspot is valid. */
    if (cursor->hotspot_x >= cursor->shm_buffer->width ||
        cursor->hotspot_y >= cursor->shm_buffer->height)
    {
        cursor->hotspot_x = cursor->shm_buffer->width / 2;
        cursor->hotspot_y = cursor->shm_buffer->height / 2;
    }

    if (!cursor->wl_surface)
    {
        cursor->wl_surface =
            wl_compositor_create_surface(process_wayland.wl_compositor);
        if (!cursor->wl_surface)
        {
            ERR("Failed to create wl_surface for cursor\n");
            goto clear_cursor;
        }
    }

    /* Commit the cursor buffer to the cursor surface. */
    wl_surface_attach(cursor->wl_surface,
                      cursor->shm_buffer->wl_buffer, 0, 0);
    wl_surface_damage_buffer(cursor->wl_surface, 0, 0,
                             cursor->shm_buffer->width,
                             cursor->shm_buffer->height);
    wl_surface_commit(cursor->wl_surface);

    return;

clear_cursor:
    if (cursor->shm_buffer)
    {
        wayland_shm_buffer_unref(cursor->shm_buffer);
        cursor->shm_buffer = NULL;
    }
    if (cursor->wl_surface)
    {
        wl_surface_destroy(cursor->wl_surface);
        cursor->wl_surface = NULL;
    }
}

/***********************************************************************
 *           WAYLAND_SetCursor
 */
void WAYLAND_SetCursor(HWND hwnd, HCURSOR hcursor)
{
    struct wayland_pointer *pointer = &process_wayland.pointer;

    TRACE("hwnd=%p hcursor=%p\n", hwnd, hcursor);

    pthread_mutex_lock(&pointer->mutex);
    if (pointer->focused_hwnd == hwnd)
    {
        wayland_pointer_update_cursor(hcursor);
        wl_pointer_set_cursor(pointer->wl_pointer,
                              pointer->enter_serial,
                              pointer->cursor.wl_surface,
                              pointer->cursor.hotspot_x,
                              pointer->cursor.hotspot_y);
        wl_display_flush(process_wayland.wl_display);
    }
    pthread_mutex_unlock(&pointer->mutex);
}
