/*
 * Copyright 2000 Alexandre Julliard
 * Copyright 2019 Zhiyi Zhang for CodeWeavers
 * Copyright 2026 Henri Verbeet
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

#include "vkd3d_memory.h"
#include "vkd3d_string.h"

#include <inttypes.h>
#include <math.h>

static size_t vkd3d_utf8_len(uint32_t c)
{
    /* 0x00-0x7f: 1 byte */
    if (c < 0x80)
        return 1;
    /* 0x80-0x7ff: 2 bytes */
    if (c < 0x800)
        return 2;
    /* 0x800-0xffff: 3 bytes */
    if (c < 0x10000)
        return 3;
    /* 0x10000-0x10ffff: 4 bytes */
    return 4;
}

static void vkd3d_utf8_append(char **dst, uint32_t c)
{
    char *d = *dst;

    /* 0x00-0x7f: 1 byte */
    if (c < 0x80)
    {
        d[0] = c;
        *dst += 1;
        return;
    }

    /* 0x80-0x7ff: 2 bytes */
    if (c < 0x800)
    {
        d[1] = 0x80 | (c & 0x3f);
        c >>= 6;
        d[0] = 0xc0 | c;
        *dst += 2;
        return;
    }

    /* 0x800-0xffff: 3 bytes */
    if (c < 0x10000)  /* 0x800-0xffff: 3 bytes */
    {
        d[2] = 0x80 | (c & 0x3f);
        c >>= 6;
        d[1] = 0x80 | (c & 0x3f);
        c >>= 6;
        d[0] = 0xe0 | c;
        *dst += 3;
        return;
    }

    /* 0x10000-0x10ffff: 4 bytes */
    d[3] = 0x80 | (c & 0x3f);
    c >>= 6;
    d[2] = 0x80 | (c & 0x3f);
    c >>= 6;
    d[1] = 0x80 | (c & 0x3f);
    c >>= 6;
    d[0] = 0xf0 | c;
    *dst += 4;
}

static uint32_t vkd3d_utf16_read(const uint16_t **src)
{
    const uint16_t *s = *src;

    if (s[0] < 0xd800 || s[0] > 0xdfff) /* Not a surrogate pair. */
    {
        *src += 1;
        return s[0];
    }

    if (s[0] > 0xdbff /* Invalid high surrogate. */
            || s[1] < 0xdc00 || s[1] > 0xdfff) /* Invalid low surrogate. */
    {
        *src += 1;
        return 0;
    }

    *src += 2;
    return 0x10000 + ((s[0] & 0x3ff) << 10) + (s[1] & 0x3ff);
}

static char *vkd3d_strdup_w16_utf8(const uint16_t *wstr)
{
    const uint16_t *src = wstr;
    size_t dst_size = 0;
    char *dst, *utf8;
    uint32_t c;

    while (*src)
    {
        if (!(c = vkd3d_utf16_read(&src)))
            continue;
        dst_size += vkd3d_utf8_len(c);
    }
    ++dst_size;

    if (!(dst = vkd3d_malloc(dst_size)))
        return NULL;

    utf8 = dst;
    src = wstr;
    while (*src)
    {
        if (!(c = vkd3d_utf16_read(&src)))
            continue;
        vkd3d_utf8_append(&utf8, c);
    }
    *utf8 = 0;

    return dst;
}

static char *vkd3d_strdup_w32_utf8(const uint32_t *wstr)
{
    const uint32_t *src = wstr;
    size_t dst_size = 0;
    char *dst, *utf8;

    while (*src)
        dst_size += vkd3d_utf8_len(*src++);
    ++dst_size;

    if (!(dst = vkd3d_malloc(dst_size)))
        return NULL;

    utf8 = dst;
    src = wstr;
    while (*src)
        vkd3d_utf8_append(&utf8, *src++);
    *utf8 = 0;

    return dst;
}

char *vkd3d_strdup_w_utf8(const WCHAR *wstr, size_t wchar_size)
{
    if (wchar_size == 2)
        return vkd3d_strdup_w16_utf8((const uint16_t *)wstr);
    return vkd3d_strdup_w32_utf8((const uint32_t *)wstr);
}

void vkd3d_string_buffer_cleanup(struct vkd3d_string_buffer *buffer)
{
    vkd3d_free(buffer->buffer);
}

void vkd3d_string_buffer_init(struct vkd3d_string_buffer *buffer)
{
    buffer->buffer_size = 16;
    buffer->content_size = 0;
    buffer->buffer = vkd3d_malloc(buffer->buffer_size);
    VKD3D_ASSERT(buffer->buffer);
    memset(buffer->buffer, 0, buffer->buffer_size);
}

void vkd3d_string_buffer_truncate(struct vkd3d_string_buffer *buffer, size_t size)
{
    if (size < buffer->content_size)
    {
        buffer->buffer[size] = '\0';
        buffer->content_size = size;
    }
}

void vkd3d_string_buffer_clear(struct vkd3d_string_buffer *buffer)
{
    vkd3d_string_buffer_truncate(buffer, 0);
}

static bool vkd3d_string_buffer_resize(struct vkd3d_string_buffer *buffer, int rc)
{
    size_t new_buffer_size = rc >= 0 ? buffer->content_size + rc + 1 : buffer->buffer_size * 2;

    if (!vkd3d_array_reserve((void **)&buffer->buffer, &buffer->buffer_size, new_buffer_size, 1))
    {
        ERR("Failed to grow buffer.\n");
        buffer->buffer[buffer->content_size] = '\0';
        return false;
    }
    return true;
}

int vkd3d_string_buffer_vprintf(struct vkd3d_string_buffer *buffer, const char *format, va_list args)
{
    size_t rem;
    va_list a;
    int rc;

    for (;;)
    {
        rem = buffer->buffer_size - buffer->content_size;
        va_copy(a, args);
        rc = vsnprintf(&buffer->buffer[buffer->content_size], rem, format, a);
        va_end(a);
        if (rc >= 0 && (unsigned int)rc < rem)
        {
            buffer->content_size += rc;
            return 0;
        }

        if (!vkd3d_string_buffer_resize(buffer, rc))
            return -1;
    }
}

int vkd3d_string_buffer_printf(struct vkd3d_string_buffer *buffer, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vkd3d_string_buffer_vprintf(buffer, format, args);
    va_end(args);

    return ret;
}

int vkd3d_string_buffer_print_f16(struct vkd3d_string_buffer *buffer, uint16_t f)
{
    size_t idx = buffer->content_size + 1;
    union
    {
        uint32_t u32;
        float f32;
    } v;
    int ret;

    v.u32 = vkd3d_f32_from_f16(f);
    if (!(ret = vkd3d_string_buffer_printf(buffer, "%.4e", v.f32)) && isfinite(v.f32))
    {
        if (signbit(v.f32))
            ++idx;
        buffer->buffer[idx] = '.';
    }

    return ret;
}

int vkd3d_string_buffer_print_f32(struct vkd3d_string_buffer *buffer, float f)
{
    size_t idx = buffer->content_size + 1;
    int ret;

    if (!(ret = vkd3d_string_buffer_printf(buffer, "%.8e", f)) && isfinite(f))
    {
        if (signbit(f))
            ++idx;
        buffer->buffer[idx] = '.';
    }

    return ret;
}

int vkd3d_string_buffer_print_f64(struct vkd3d_string_buffer *buffer, double d)
{
    size_t idx = buffer->content_size + 1;
    int ret;

    if (!(ret = vkd3d_string_buffer_printf(buffer, "%.16e", d)) && isfinite(d))
    {
        if (signbit(d))
            ++idx;
        buffer->buffer[idx] = '.';
    }

    return ret;
}

static char get_escape_char(char c)
{
    switch (c)
    {
        case '"':
        case '\\':
            return c;
        case '\t':
            return 't';
        case '\n':
            return 'n';
        case '\v':
            return 'v';
        case '\f':
            return 'f';
        case '\r':
            return 'r';
        default:
            return 0;
    }
}

int vkd3d_string_buffer_print_string_escaped(struct vkd3d_string_buffer *buffer, const char *s, size_t len)
{
    size_t content_size, start, i;
    int ret;
    char c;

    content_size = buffer->content_size;
    for (i = 0, start = 0; i < len; ++i)
    {
        if ((c = get_escape_char(s[i])))
        {
            if ((ret = vkd3d_string_buffer_printf(buffer, "%.*s\\%c", (int)(i - start), &s[start], c)) < 0)
                goto fail;
            start = i + 1;
        }
        else if (!isprint(s[i]))
        {
            if ((ret = vkd3d_string_buffer_printf(buffer, "%.*s\\%03o",
                    (int)(i - start), &s[start], (uint8_t)s[i])) < 0)
                goto fail;
            start = i + 1;
        }
    }
    if ((ret = vkd3d_string_buffer_printf(buffer, "%.*s", (int)(len - start), &s[start])) < 0)
        goto fail;
    return ret;

fail:
    buffer->content_size = content_size;
    return ret;
}

void vkd3d_string_buffer_trace_(const struct vkd3d_string_buffer *buffer,
        const char *vkd3d_dbg_env_name, const char *function)
{
    vkd3d_debug_channel_print_text(vkd3d_debug_channel_default, vkd3d_dbg_env_name,
            VKD3D_DEBUG_CLASS_TRACE, function, buffer->buffer, buffer->content_size);
}

void vkd3d_string_buffer_cache_cleanup(struct vkd3d_string_buffer_cache *cache)
{
    unsigned int i;

    for (i = 0; i < cache->count; ++i)
    {
        vkd3d_string_buffer_cleanup(cache->buffers[i]);
        vkd3d_free(cache->buffers[i]);
    }
    vkd3d_free(cache->buffers);
    vkd3d_string_buffer_cache_init(cache);
}

void vkd3d_string_buffer_cache_init(struct vkd3d_string_buffer_cache *cache)
{
    memset(cache, 0, sizeof(*cache));
}

struct vkd3d_string_buffer *vkd3d_string_buffer_get(struct vkd3d_string_buffer_cache *cache)
{
    struct vkd3d_string_buffer *buffer;

    if (!cache->count)
    {
        if (!vkd3d_array_reserve((void **)&cache->buffers, &cache->capacity,
                cache->max_count + 1, sizeof(*cache->buffers)))
            return NULL;
        ++cache->max_count;

        if (!(buffer = vkd3d_malloc(sizeof(*buffer))))
            return NULL;
        vkd3d_string_buffer_init(buffer);
    }
    else
    {
        buffer = cache->buffers[--cache->count];
    }
    vkd3d_string_buffer_clear(buffer);
    return buffer;
}

void vkd3d_string_buffer_release(struct vkd3d_string_buffer_cache *cache, struct vkd3d_string_buffer *buffer)
{
    if (!buffer)
        return;
    VKD3D_ASSERT(cache->count + 1 <= cache->max_count);
    cache->buffers[cache->count++] = buffer;
}
