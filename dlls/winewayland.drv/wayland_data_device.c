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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "waylanddrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

/* A special MIME type we mark our data offers with, so we can detect that
 * they are coming from us. */
#define WINEWAYLAND_TAG_MIME_TYPE "application/x.winewayland.tag"

struct data_device_format
{
    const char *mime_type;
    UINT clipboard_format;
    const WCHAR *register_name;
    void *(*export)(void *data, size_t size, size_t *ret_size);
    void *(*import)(void *data, size_t size, size_t *ret_size);
};

struct wayland_data_offer
{
    union
    {
        struct zwlr_data_control_offer_v1 *zwlr_data_control_offer_v1;
        struct wl_data_offer *wl_data_offer;
    };
    struct wl_array types;
};

static HWND clipboard_hwnd;
static const WCHAR rich_text_formatW[] = {'R','i','c','h',' ','T','e','x','t',' ','F','o','r','m','a','t',0};
static const WCHAR pngW[] = {'P','N','G',0};
static const WCHAR jfifW[] = {'J','F','I','F',0};
static const WCHAR gifW[] = {'G','I','F',0};
static const WCHAR image_svg_xmlW[] = {'i','m','a','g','e','/','s','v','g','+','x','m','l',0};

/* Normalize the MIME type string by skipping inconsequential characters,
 * such as spaces and double quotes, and convert to lower case. */
static const char *normalize_mime_type(const char *mime_type)
{
    char *new_mime_type;
    const char *cur_read;
    char *cur_write;
    size_t new_mime_len = 0;

    for (cur_read = mime_type; *cur_read != '\0'; ++cur_read)
    {
        if (*cur_read != ' ' && *cur_read != '"')
            new_mime_len++;
    }

    new_mime_type = malloc(new_mime_len + 1);
    if (!new_mime_type) return NULL;

    for (cur_read = mime_type, cur_write = new_mime_type; *cur_read != '\0'; ++cur_read)
    {
        if (*cur_read != ' ' && *cur_read != '"')
            *cur_write++ = tolower(*cur_read);
    }

    *cur_write = '\0';

    return new_mime_type;
}

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

static void *read_all(int fd, size_t *size_out)
{
    size_t buffer_size = 4096;
    int total = 0;
    unsigned char *buffer;
    int nread;

    if (!(buffer = malloc(buffer_size)))
    {
        ERR("failed to allocate read buffer\n");
        goto out;
    }

    do
    {
        nread = read(fd, buffer + total, buffer_size - total);
        if (nread == -1 && errno != EINTR)
        {
            TRACE("failed to read from fd (errno: %d)\n", errno);
            total = 0;
            goto out;
        }
        else if (nread > 0)
        {
            total += nread;
            if (total == buffer_size)
            {
                unsigned char *new_buffer;
                buffer_size *= 2;
                new_buffer = realloc(buffer, buffer_size);
                if (!new_buffer)
                {
                    ERR("failed to reallocate read buffer\n");
                    total = 0;
                    goto out;
                }
                buffer = new_buffer;
            }
        }
    } while (nread > 0);

    TRACE("read %d bytes\n", total);

out:
    if (total == 0 && buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }
    *size_out = total;
    return buffer;
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

static void *export_data(void *data, size_t size, size_t *ret_size)
{
    *ret_size = size;
    return data;
}

static void *import_unicode_text(void *data, size_t size, size_t *ret_size)
{
    DWORD wsize;
    WCHAR *ret;

    RtlUTF8ToUnicodeN(NULL, 0, &wsize, data, size);
    if (!(ret = malloc(wsize + sizeof(WCHAR)))) return NULL;
    RtlUTF8ToUnicodeN(ret, wsize, &wsize, data, size);
    ret[wsize / sizeof(WCHAR)] = 0;

    *ret_size = wsize + sizeof(WCHAR);

    return ret;
}

static void *import_data(void *data, size_t size, size_t *ret_size)
{
    *ret_size = size;
    return data;
}

/* Order is important. When selecting a mime-type for a clipboard format we
 * will choose the first entry that matches the specified clipboard format. */
static struct data_device_format supported_formats[] =
{
    {"text/plain;charset=utf-8", CF_UNICODETEXT, NULL, export_unicode_text, import_unicode_text},
    {"text/rtf", 0, rich_text_formatW, export_data, import_data},
    {"image/tiff", CF_TIFF, NULL, export_data, import_data},
    {"image/png", 0, pngW, export_data, import_data},
    {"image/jpeg", 0, jfifW, export_data, import_data},
    {"image/gif", 0, gifW, export_data, import_data},
    {"image/svg+xml", 0, image_svg_xmlW, export_data, import_data},
    {"application/x-riff", CF_RIFF, NULL, export_data, import_data},
    {"audio/wav", CF_WAVE, NULL, export_data, import_data},
    {"audio/x-wav", CF_WAVE, NULL, export_data, import_data},
    {NULL, 0, NULL},
};

static BOOL string_array_contains(struct wl_array *array, const char *str)
{
    char **p;

    wl_array_for_each(p, array)
        if (!strcmp(*p, str)) return TRUE;

    return FALSE;
}

static struct data_device_format *data_device_format_for_clipboard_format(UINT clipboard_format,
                                                                          struct wl_array *types)
{
    struct data_device_format *format;

    for (format = supported_formats; format->mime_type; ++format)
    {
        if (format->clipboard_format == clipboard_format &&
            (!types || string_array_contains(types, format->mime_type)))
        {
            return format;
        }
    }

    return NULL;
}

static struct data_device_format *data_device_format_for_mime_type(const char *mime)
{
    struct data_device_format *format;

    for (format = supported_formats; format->mime_type; ++format)
    {
        if (!strcmp(mime, format->mime_type)) return format;
    }

    return NULL;
}

static ATOM register_clipboard_format(const WCHAR *name)
{
    ATOM atom;
    if (NtAddAtom(name, lstrlenW(name) * sizeof(WCHAR), &atom)) return 0;
    return atom;
}

/**********************************************************************
 *          zwlr_data_control_source_v1 handling
 */

static void wayland_data_source_export(struct data_device_format *format, int fd)
{
    struct get_clipboard_params params = { .data_only = TRUE, .size = 1024 };
    void *exported = NULL;
    size_t exported_size;

    TRACE("format=%u => mime=%s\n", format->clipboard_format, format->mime_type);

    if (!NtUserOpenClipboard(clipboard_hwnd, 0))
    {
        TRACE("failed to open clipboard for export\n");
        return;
    }

    for (;;)
    {
        if (!(params.data = malloc(params.size))) break;
        if (NtUserGetClipboardData(format->clipboard_format, &params))
        {
            exported = format->export(params.data, params.size, &exported_size);
            break;
        }
        if (!params.data_size) break;
        free(params.data);
        params.size = params.data_size;
        params.data_size = 0;
    }

    NtUserCloseClipboard();
    if (exported) write_all(fd, exported, exported_size);

    if (exported != params.data) free(exported);
    free(params.data);
}

static void data_control_source_send(void *data,
                                     struct zwlr_data_control_source_v1 *source,
                                     const char *mime_type, int32_t fd)
{
    struct data_device_format *format;
    const char *normalized;

    if ((normalized = normalize_mime_type(mime_type)) &&
        (format = data_device_format_for_mime_type(normalized)))
    {
        wayland_data_source_export(format, fd);
    }
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

/**********************************************************************
 *          zwlr_data_control_offer_v1 handling
 */

static void data_control_offer_offer(void *data,
                                     struct zwlr_data_control_offer_v1 *zwlr_data_control_offer_v1,
                                     const char *type)
{
    struct wayland_data_offer *data_offer = data;
    const char *normalized;
    const char **p;

    if ((normalized = normalize_mime_type(type)) &&
        (p = wl_array_add(&data_offer->types, sizeof *p)))
    {
        *p = normalized;
    }
}

static const struct zwlr_data_control_offer_v1_listener data_control_offer_listener =
{
    data_control_offer_offer,
};

static void data_offer_offer(void *data, struct wl_data_offer *wl_data_offer, const char *type)
{
    data_control_offer_offer(data, NULL, type);
}

static const struct wl_data_offer_listener data_offer_listener =
{
    data_offer_offer,
};

static void wayland_data_offer_create(void *offer_proxy)
{
    struct wayland_data_offer *data_offer;

    if (!(data_offer = calloc(1, sizeof(*data_offer))))
    {
        ERR("Failed to allocate memory for data offer\n");
        return;
    }

    wl_array_init(&data_offer->types);
    if (process_wayland.zwlr_data_control_manager_v1)
    {
        data_offer->zwlr_data_control_offer_v1 = offer_proxy;
        zwlr_data_control_offer_v1_add_listener(data_offer->zwlr_data_control_offer_v1,
                                                &data_control_offer_listener, data_offer);
    }
    else
    {
        data_offer->wl_data_offer = offer_proxy;
        wl_data_offer_add_listener(data_offer->wl_data_offer, &data_offer_listener,
                                   data_offer);

    }
}

static void wayland_data_offer_destroy(struct wayland_data_offer *data_offer)
{
    char **p;

    if (process_wayland.zwlr_data_control_manager_v1)
        zwlr_data_control_offer_v1_destroy(data_offer->zwlr_data_control_offer_v1);
    else
        wl_data_offer_destroy(data_offer->wl_data_offer);
    wl_array_for_each(p, &data_offer->types)
        free(*p);
    wl_array_release(&data_offer->types);
    free(data_offer);
}

static int wayland_data_offer_get_import_fd(struct wayland_data_offer *data_offer,
                                            const char *mime_type)
{
    int data_pipe[2];

#if HAVE_PIPE2
    if (pipe2(data_pipe, O_CLOEXEC) == -1)
#endif
    {
        if (pipe(data_pipe) == -1)
        {
            ERR("failed to create clipboard data pipe\n");
            return -1;
        }
        fcntl(data_pipe[0], F_SETFD, FD_CLOEXEC);
        fcntl(data_pipe[1], F_SETFD, FD_CLOEXEC);
    }

    if (process_wayland.zwlr_data_control_manager_v1)
    {
        zwlr_data_control_offer_v1_receive(data_offer->zwlr_data_control_offer_v1,
                                           mime_type, data_pipe[1]);
    }
    else
    {
        wl_data_offer_receive(data_offer->wl_data_offer, mime_type, data_pipe[1]);
    }
    close(data_pipe[1]);

    /* Flush to ensure our receive request reaches the server. */
    wl_display_flush(process_wayland.wl_display);

    return data_pipe[0];
}

static void *import_format(int fd, struct data_device_format *format, size_t *ret_size)
{
    size_t size;
    void *data, *ret;

    if (!(data = read_all(fd, &size))) return NULL;
    ret = format->import(data, size, ret_size);
    if (ret != data) free(data);
    return ret;
}

static void wayland_data_device_destroy_clipboard_data_offer(struct wayland_data_device *data_device)
{
    struct wayland_data_offer *data_offer = NULL;

    if (process_wayland.zwlr_data_control_manager_v1 &&
        data_device->clipboard_zwlr_data_control_offer_v1)
    {
        data_offer = zwlr_data_control_offer_v1_get_user_data(
            data_device->clipboard_zwlr_data_control_offer_v1);
        data_device->clipboard_zwlr_data_control_offer_v1 = NULL;
    }
    else if (!process_wayland.zwlr_data_control_manager_v1 &&
             data_device->clipboard_wl_data_offer)
    {
        data_offer = wl_data_offer_get_user_data(data_device->clipboard_wl_data_offer);
        data_device->clipboard_wl_data_offer = NULL;
    }

    if (data_offer) wayland_data_offer_destroy(data_offer);
}

/**********************************************************************
 *          zwlr_data_control_device_v1 handling
 */

static void data_control_device_data_offer(
    void *data,
    struct zwlr_data_control_device_v1 *zwlr_data_control_device_v1,
    struct zwlr_data_control_offer_v1 *zwlr_data_control_offer_v1)
{
    wayland_data_offer_create(zwlr_data_control_offer_v1);
}

static void handle_selection(struct wayland_data_device *data_device,
                             struct wayland_data_offer *data_offer)
{
    char **p;

    if (!data_offer)
    {
        TRACE("empty offer, clearing clipboard\n");
        if (NtUserOpenClipboard(clipboard_hwnd, 0))
        {
            NtUserEmptyClipboard();
            NtUserCloseClipboard();
        }
        goto done;
    }

    TRACE("updating clipboard from wayland offer\n");

    /* If this offer contains the special winewayland tag mime-type, it was sent
     * by a winewayland process to notify external wayland clients about a Wine
     * clipboard update. */
    wl_array_for_each(p, &data_offer->types)
    {
        if (!strcmp(*p, WINEWAYLAND_TAG_MIME_TYPE))
        {
            TRACE("offer sent by winewayland, ignoring\n");
            wayland_data_offer_destroy(data_offer);
            data_offer = NULL;
            goto done;
        }
    }

    if (!NtUserOpenClipboard(clipboard_hwnd, 0))
    {
        TRACE("failed to open clipboard for selection\n");
        wayland_data_offer_destroy(data_offer);
        data_offer = NULL;
        goto done;
    }

    NtUserEmptyClipboard();

    /* For each mime type, mark that we have available clipboard data. */
    wl_array_for_each(p, &data_offer->types)
    {
        struct data_device_format *format = data_device_format_for_mime_type(*p);
        if (format)
        {
            struct set_clipboard_params params = {0};
            TRACE("available clipboard format for %s => %u\n",
                  *p, format->clipboard_format);
            NtUserSetClipboardData(format->clipboard_format, 0, &params);
        }
    }

    NtUserCloseClipboard();

done:
    pthread_mutex_lock(&data_device->mutex);
    wayland_data_device_destroy_clipboard_data_offer(data_device);
    if (data_offer)
    {
        if (process_wayland.zwlr_data_control_manager_v1)
            data_device->clipboard_zwlr_data_control_offer_v1 = data_offer->zwlr_data_control_offer_v1;
        else
            data_device->clipboard_wl_data_offer = data_offer->wl_data_offer;
    }
    pthread_mutex_unlock(&data_device->mutex);

}

static void data_control_device_selection(
    void *data,
    struct zwlr_data_control_device_v1 *zwlr_data_control_device_v1,
    struct zwlr_data_control_offer_v1 *zwlr_data_control_offer_v1)
{
    handle_selection(data,
                     zwlr_data_control_offer_v1 ?
                         zwlr_data_control_offer_v1_get_user_data(zwlr_data_control_offer_v1) :
                         NULL);
}

static void data_control_device_finished(
    void *data, struct zwlr_data_control_device_v1 *zwlr_data_control_device_v1)
{
}

static const struct zwlr_data_control_device_v1_listener data_control_device_listener =
{
    data_control_device_data_offer,
    data_control_device_selection,
    data_control_device_finished,
};

/**********************************************************************
 *          wl_data_source handling
 */

static void data_source_target(void *data, struct wl_data_source *source,
                               const char *mime_type)
{
}

static void data_source_send(void *data, struct wl_data_source *source,
                             const char *mime_type, int32_t fd)
{
    struct data_device_format *format;
    const char *normalized;

    if ((normalized = normalize_mime_type(mime_type)) &&
        (format = data_device_format_for_mime_type(normalized)))
    {
        wayland_data_source_export(format, fd);
    }
    close(fd);
}

static void data_source_cancelled(void *data, struct wl_data_source *source)
{
    struct wayland_data_device *data_device = data;

    pthread_mutex_lock(&data_device->mutex);
    wl_data_source_destroy(source);
    if (source == data_device->wl_data_source)
        data_device->wl_data_source = NULL;
    pthread_mutex_unlock(&data_device->mutex);
}

static void data_source_dnd_drop_performed(void *data,
                                           struct wl_data_source *source)
{
}

static void data_source_dnd_finished(void *data, struct wl_data_source *source)
{
}

static void data_source_action(void *data, struct wl_data_source *source,
                               uint32_t dnd_action)
{
}

static const struct wl_data_source_listener data_source_listener =
{
    data_source_target,
    data_source_send,
    data_source_cancelled,
    data_source_dnd_drop_performed,
    data_source_dnd_finished,
    data_source_action,
};

/**********************************************************************
 *          wl_data_device handling
 */

static void data_device_data_offer(void *data, struct wl_data_device *wl_data_device,
                                   struct wl_data_offer *wl_data_offer)
{
    wayland_data_offer_create(wl_data_offer);
}

static void data_device_enter(void *data, struct wl_data_device *wl_data_device,
                              uint32_t serial, struct wl_surface *wl_surface,
                              wl_fixed_t x_w, wl_fixed_t y_w,
                              struct wl_data_offer *wl_data_offer)
{
}

static void data_device_leave(void *data, struct wl_data_device *wl_data_device)
{
}

static void data_device_motion(void *data, struct wl_data_device *wl_data_device,
                               uint32_t time, wl_fixed_t x_w, wl_fixed_t y_w)
{
}

static void data_device_drop(void *data, struct wl_data_device *wl_data_device)
{
}

static void data_device_selection(void *data, struct wl_data_device *wl_data_device,
                                  struct wl_data_offer *wl_data_offer)
{
    handle_selection(data, wl_data_offer ? wl_data_offer_get_user_data(wl_data_offer) : NULL);
}

static const struct wl_data_device_listener data_device_listener =
{
    data_device_data_offer,
    data_device_enter,
    data_device_leave,
    data_device_motion,
    data_device_drop,
    data_device_selection,
};

void wayland_data_device_init(void)
{
    struct wayland_data_device *data_device = &process_wayland.data_device;
    struct data_device_format *format = supported_formats;

    TRACE("\n");

    pthread_mutex_lock(&data_device->mutex);
    if (process_wayland.zwlr_data_control_manager_v1)
    {
        if (data_device->zwlr_data_control_device_v1)
            zwlr_data_control_device_v1_destroy(data_device->zwlr_data_control_device_v1);
        data_device->zwlr_data_control_device_v1 =
            zwlr_data_control_manager_v1_get_data_device(
                process_wayland.zwlr_data_control_manager_v1,
                process_wayland.seat.wl_seat);
        if (data_device->zwlr_data_control_device_v1)
        {
            zwlr_data_control_device_v1_add_listener(
                data_device->zwlr_data_control_device_v1, &data_control_device_listener,
                data_device);
        }
    }
    else if (process_wayland.wl_data_device_manager)
    {
        if (data_device->wl_data_device)
            wl_data_device_release(data_device->wl_data_device);
        data_device->wl_data_device =
            wl_data_device_manager_get_data_device(
                process_wayland.wl_data_device_manager,
                process_wayland.seat.wl_seat);
        if (data_device->wl_data_device)
        {
            wl_data_device_add_listener(data_device->wl_data_device,
                                        &data_device_listener, data_device);
        }
    }
    pthread_mutex_unlock(&data_device->mutex);

    for (; format->mime_type; ++format)
    {
        if (format->clipboard_format == 0)
            format->clipboard_format = register_clipboard_format(format->register_name);
    }
}

static void clipboard_update(void)
{
    struct wayland_data_device *data_device = &process_wayland.data_device;
    struct zwlr_data_control_source_v1 *zwlr_source = NULL;
    struct wl_data_source *wl_source = NULL;
    UINT *formats, formats_size = 256, i;
    uint32_t serial = 0;

    if (process_wayland.zwlr_data_control_manager_v1)
    {
        zwlr_source = zwlr_data_control_manager_v1_create_data_source(
            process_wayland.zwlr_data_control_manager_v1);
    }
    else
    {
        serial = InterlockedCompareExchange(&process_wayland.input_serial, 0, 0);
        pthread_mutex_lock(&process_wayland.keyboard.mutex);
        if (!process_wayland.keyboard.focused_hwnd) serial = 0;
        pthread_mutex_unlock(&process_wayland.keyboard.mutex);
        if (process_wayland.wl_data_device_manager && serial)
        {
            wl_source = wl_data_device_manager_create_data_source(
                process_wayland.wl_data_device_manager);
        }
        else return;
    }

    TRACE("\n");

    if (!zwlr_source && !wl_source)
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
        if (wl_source) wl_data_source_destroy(wl_source);
        else zwlr_data_control_source_v1_destroy(zwlr_source);
        return;
    }

    for (i = 0; i < formats_size; ++i)
    {
        struct data_device_format *format =
            data_device_format_for_clipboard_format(formats[i], NULL);
        if (format)
        {
            TRACE("offering mime=%s for format=%u\n", format->mime_type, formats[i]);
            if (wl_source) wl_data_source_offer(wl_source, format->mime_type);
            else zwlr_data_control_source_v1_offer(zwlr_source, format->mime_type);
        }
    }

    free(formats);

    if (wl_source)
    {
        wl_data_source_offer(wl_source, WINEWAYLAND_TAG_MIME_TYPE);
        wl_data_source_add_listener(wl_source, &data_source_listener, data_device);
    }
    else
    {
        zwlr_data_control_source_v1_offer(zwlr_source, WINEWAYLAND_TAG_MIME_TYPE);
        zwlr_data_control_source_v1_add_listener(zwlr_source, &data_control_source_listener, data_device);
    }

    pthread_mutex_lock(&data_device->mutex);
    /* Destroy any previous source only after setting the new source, to
     * avoid spurious 'selection(nil)' events. */
    if (wl_source)
    {
        if (data_device->wl_data_device)
            wl_data_device_set_selection(data_device->wl_data_device, wl_source, serial);
        if (data_device->wl_data_source)
            wl_data_source_destroy(data_device->wl_data_source);
        data_device->wl_data_source = wl_source;
    }
    else
    {
        if (data_device->zwlr_data_control_device_v1)
            zwlr_data_control_device_v1_set_selection(data_device->zwlr_data_control_device_v1, zwlr_source);
        if (data_device->zwlr_data_control_source_v1)
            zwlr_data_control_source_v1_destroy(data_device->zwlr_data_control_source_v1);
        data_device->zwlr_data_control_source_v1 = zwlr_source;
    }
    pthread_mutex_unlock(&data_device->mutex);

    wl_display_flush(process_wayland.wl_display);
}

static void render_format(UINT clipboard_format)
{
    struct wayland_data_device *data_device = &process_wayland.data_device;
    struct wayland_data_offer *data_offer = NULL;
    struct data_device_format *format;
    int import_fd = -1;

    TRACE("clipboard_format=%u\n", clipboard_format);

    pthread_mutex_lock(&data_device->mutex);
    if (process_wayland.zwlr_data_control_manager_v1 &&
        data_device->clipboard_zwlr_data_control_offer_v1)
    {
        data_offer = zwlr_data_control_offer_v1_get_user_data(
            data_device->clipboard_zwlr_data_control_offer_v1);
    }
    else if (!process_wayland.zwlr_data_control_manager_v1 &&
             data_device->clipboard_wl_data_offer)
    {
        data_offer = wl_data_offer_get_user_data(data_device->clipboard_wl_data_offer);
    }

    if (data_offer &&
        (format = data_device_format_for_clipboard_format(clipboard_format,
                                                          &data_offer->types)))
    {
        import_fd = wayland_data_offer_get_import_fd(data_offer, format->mime_type);
    }
    pthread_mutex_unlock(&data_device->mutex);

    if (import_fd >= 0)
    {
        struct set_clipboard_params params = {0};
        if ((params.data = import_format(import_fd, format, &params.size)))
        {
            NtUserSetClipboardData(format->clipboard_format, 0, &params);
            free(params.data);
        }
        close(import_fd);
    }
}

static void destroy_clipboard(void)
{
    struct wayland_data_device *data_device = &process_wayland.data_device;

    TRACE("\n");

    pthread_mutex_lock(&data_device->mutex);
    wayland_data_device_destroy_clipboard_data_offer(data_device);
    pthread_mutex_unlock(&data_device->mutex);
}

static BOOL is_winewayland_clipboard_hwnd(HWND hwnd)
{
    static const WCHAR clipboard_classnameW[] = {
        '_','_','w','i','n','e','w','a','y','l','a','n','d','_',
        'c','l','i','p','b','o','a','r','d','_','m','a','n','a','g','e','r'};
    WCHAR buffer[64];
    UNICODE_STRING name = {.Buffer = buffer, .MaximumLength = sizeof(buffer)};

    if (!NtUserGetClassName(hwnd, FALSE, &name)) return FALSE;
    return !wcscmp(buffer, clipboard_classnameW);
}

LRESULT WAYLAND_ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_NCCREATE:
        /* Disable the default clipboard window in the desktop process if we are
         * using the core wl_data_device protocol. */
        if (!process_wayland.zwlr_data_control_manager_v1 &&
            process_wayland.wl_data_device_manager &&
            !is_winewayland_clipboard_hwnd(hwnd))
        {
            return FALSE;
        }
        clipboard_hwnd = hwnd;
        NtUserAddClipboardFormatListener(hwnd);
        pthread_mutex_lock(&process_wayland.seat.mutex);
        if (process_wayland.seat.wl_seat) wayland_data_device_init();
        pthread_mutex_unlock(&process_wayland.seat.mutex);
        return TRUE;
    case WM_CLIPBOARDUPDATE:
        if (NtUserGetClipboardOwner() == clipboard_hwnd) break;
        clipboard_update();
        break;
    case WM_RENDERFORMAT:
        render_format(wparam);
        break;
    case WM_DESTROYCLIPBOARD:
        destroy_clipboard();
        break;
    }

    return NtUserMessageCall(hwnd, msg, wparam, lparam, NULL, NtUserDefWindowProc, FALSE);
}
