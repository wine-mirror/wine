/*
 * Wayland clipboard
 *
 * Copyright 2025 Alexandros Frantzis for Collabora Ltd
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
#include <stdlib.h>
#include <unistd.h>

#include "waylanddrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

static HWND clipboard_hwnd;

static void write_all(int fd, const void *buf, size_t count)
{
    size_t nwritten = 0;
    ssize_t ret;

    while (nwritten < count)
    {
        ret = write(fd, (const char*)buf + nwritten, count - nwritten);
        if (ret == -1 && errno != EINTR) break;
        else if (ret > 0) nwritten += ret;
    }

    if (nwritten < count)
    {
        WARN("Failed to write all clipboard data, had %zu bytes, wrote %zu bytes\n",
             count, nwritten);
    }
}

static void *export_unicode_text(void *data, size_t size, size_t *ret_size)
{
    DWORD byte_count;
    char *bytes;

    /* Wayland apps expect strings to not be zero-terminated, so avoid
     * zero-terminating the resulting converted string. */
    if (size >= sizeof(WCHAR) && ((WCHAR *)data)[size / sizeof(WCHAR) - 1] == 0)
        size -= sizeof(WCHAR);

    RtlUnicodeToUTF8N(NULL, 0, &byte_count, data, size);
    if (!(bytes = malloc(byte_count))) return NULL;
    RtlUnicodeToUTF8N(bytes, byte_count, &byte_count, data, size);

    *ret_size = byte_count;
    return bytes;
}

/**********************************************************************
 *          zwlr_data_control_source_v1 handling
 */

static void wayland_data_source_export(int32_t fd)
{
    struct get_clipboard_params params = { .data_only = TRUE, .size = 1024 };
    void *exported = NULL;
    size_t exported_size;

    TRACE("\n");

    if (!NtUserOpenClipboard(clipboard_hwnd, 0))
    {
        TRACE("failed to open clipboard for export\n");
        return;
    }

    for (;;)
    {
        if (!(params.data = malloc(params.size))) break;
        if (NtUserGetClipboardData(CF_UNICODETEXT, &params))
        {
            exported = export_unicode_text(params.data, params.size, &exported_size);
            break;
        }
        if (!params.data_size) break;
        free(params.data);
        params.size = params.data_size;
        params.data_size = 0;
    }

    NtUserCloseClipboard();
    if (exported) write_all(fd, exported, exported_size);

    free(exported);
    free(params.data);
}

static void data_control_source_send(void *data,
                                     struct zwlr_data_control_source_v1 *source,
                                     const char *mime_type, int32_t fd)
{
    if (!strcmp(mime_type, "text/plain;charset=utf-8"))
        wayland_data_source_export(fd);
    close(fd);
}

static void data_control_source_cancelled(void *data,
                                          struct zwlr_data_control_source_v1 *source)
{
    struct wayland_data_device *data_device = data;

    pthread_mutex_lock(&data_device->mutex);
    zwlr_data_control_source_v1_destroy(source);
    if (source == data_device->zwlr_data_control_source_v1)
        data_device->zwlr_data_control_source_v1 = NULL;
    pthread_mutex_unlock(&data_device->mutex);
}

static const struct zwlr_data_control_source_v1_listener data_control_source_listener =
{
    data_control_source_send,
    data_control_source_cancelled,
};

void wayland_data_device_init(void)
{
    struct wayland_data_device *data_device = &process_wayland.data_device;

    TRACE("\n");

    pthread_mutex_lock(&data_device->mutex);
    if (data_device->zwlr_data_control_device_v1)
        zwlr_data_control_device_v1_destroy(data_device->zwlr_data_control_device_v1);
    data_device->zwlr_data_control_device_v1 =
        zwlr_data_control_manager_v1_get_data_device(
            process_wayland.zwlr_data_control_manager_v1,
            process_wayland.seat.wl_seat);
    pthread_mutex_unlock(&data_device->mutex);
}

static void clipboard_update(void)
{
    struct wayland_data_device *data_device = &process_wayland.data_device;
    struct zwlr_data_control_source_v1 *source;
    UINT *formats, formats_size = 256, i;

    if (!process_wayland.zwlr_data_control_manager_v1) return;

    TRACE("\n");

    source = zwlr_data_control_manager_v1_create_data_source(
        process_wayland.zwlr_data_control_manager_v1);
    if (!source)
    {
        ERR("failed to create data source\n");
        return;
    }

    for (;;)
    {
        if (!(formats = malloc(formats_size * sizeof(*formats)))) break;
        if (NtUserGetUpdatedClipboardFormats(formats, formats_size, &formats_size)) break;
        free(formats);
        formats = NULL;
        if (RtlGetLastWin32Error() != ERROR_INSUFFICIENT_BUFFER) break;
    }

    if (!formats && formats_size)
    {
        ERR("failed to get clipboard formats\n");
        zwlr_data_control_source_v1_destroy(source);
        return;
    }

    for (i = 0; i < formats_size; ++i)
    {
        if (formats[i] == CF_UNICODETEXT)
            zwlr_data_control_source_v1_offer(source, "text/plain;charset=utf-8");
    }

    free(formats);

    zwlr_data_control_source_v1_add_listener(source, &data_control_source_listener, data_device);

    pthread_mutex_lock(&data_device->mutex);
    if (data_device->zwlr_data_control_device_v1)
        zwlr_data_control_device_v1_set_selection(data_device->zwlr_data_control_device_v1, source);
    /* Destroy any previous source only after setting the new source, to
     * avoid spurious 'selection(nil)' events. */
    if (data_device->zwlr_data_control_source_v1)
        zwlr_data_control_source_v1_destroy(data_device->zwlr_data_control_source_v1);
    data_device->zwlr_data_control_source_v1 = source;
    pthread_mutex_unlock(&data_device->mutex);

    wl_display_flush(process_wayland.wl_display);
}

LRESULT WAYLAND_ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_NCCREATE:
        clipboard_hwnd = hwnd;
        NtUserAddClipboardFormatListener(hwnd);
        pthread_mutex_lock(&process_wayland.seat.mutex);
        if (process_wayland.seat.wl_seat && process_wayland.zwlr_data_control_manager_v1)
            wayland_data_device_init();
        pthread_mutex_unlock(&process_wayland.seat.mutex);
        return TRUE;
    case WM_CLIPBOARDUPDATE:
        clipboard_update();
        break;
    }

    return NtUserMessageCall(hwnd, msg, wparam, lparam, NULL, NtUserDefWindowProc, FALSE);
}
