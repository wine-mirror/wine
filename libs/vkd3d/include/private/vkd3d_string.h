/*
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

#ifndef __VKD3D_UTF8_H
#define __VKD3D_UTF8_H

#include "vkd3d_common.h"

struct vkd3d_string_buffer
{
    char *buffer;
    size_t buffer_size, content_size;
};

struct vkd3d_string_buffer_cache
{
    struct vkd3d_string_buffer **buffers;
    size_t count, max_count, capacity;
};

void vkd3d_string_buffer_cleanup(struct vkd3d_string_buffer *buffer);
void vkd3d_string_buffer_clear(struct vkd3d_string_buffer *buffer);
void vkd3d_string_buffer_init(struct vkd3d_string_buffer *buffer);
int vkd3d_string_buffer_print_f16(struct vkd3d_string_buffer *buffer, uint16_t f);
int vkd3d_string_buffer_print_f32(struct vkd3d_string_buffer *buffer, float f);
int vkd3d_string_buffer_print_f64(struct vkd3d_string_buffer *buffer, double d);
int vkd3d_string_buffer_print_string_escaped(struct vkd3d_string_buffer *buffer, const char *s, size_t len);
int vkd3d_string_buffer_printf(struct vkd3d_string_buffer *buffer, const char *format, ...) VKD3D_PRINTF_FUNC(2, 3);
#define vkd3d_string_buffer_trace(buffer) \
        vkd3d_string_buffer_trace_(buffer, VKD3D_DEBUG_ENV_NAME, __FUNCTION__)
void vkd3d_string_buffer_trace_(const struct vkd3d_string_buffer *buffer,
        const char *vkd3d_dbg_env_name, const char *function);
void vkd3d_string_buffer_truncate(struct vkd3d_string_buffer *buffer, size_t size);
int vkd3d_string_buffer_vprintf(struct vkd3d_string_buffer *buffer, const char *format, va_list args);

void vkd3d_string_buffer_cache_cleanup(struct vkd3d_string_buffer_cache *list);
struct vkd3d_string_buffer *vkd3d_string_buffer_get(struct vkd3d_string_buffer_cache *list);
void vkd3d_string_buffer_cache_init(struct vkd3d_string_buffer_cache *list);
void vkd3d_string_buffer_release(struct vkd3d_string_buffer_cache *list, struct vkd3d_string_buffer *buffer);

char *vkd3d_strdup_w_utf8(const WCHAR *wstr, size_t wchar_size);

#endif /* __VKD3D_UTF8_H */
