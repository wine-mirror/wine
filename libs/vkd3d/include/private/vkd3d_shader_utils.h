/*
 * Copyright 2023 Conor McCarthy for CodeWeavers
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

#ifndef __VKD3D_SHADER_UTILS_H
#define __VKD3D_SHADER_UTILS_H

#include "vkd3d_shader.h"
#include <sys/stat.h>

/* S_ISREG may not be defined when building for Windows. MinGW provides a more
 * POSIX-like environment that does define S_ISREG, but the Wine/msvcrt
 * headers do not. */
#ifndef S_ISREG
# define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

static inline enum vkd3d_result vkd3d_shader_parse_dxbc_source_type(const struct vkd3d_shader_code *dxbc,
        enum vkd3d_shader_source_type *type, char **messages)
{
    struct vkd3d_shader_dxbc_desc desc;
    enum vkd3d_result ret;
    unsigned int i;

    *type = VKD3D_SHADER_SOURCE_NONE;

    if ((ret = vkd3d_shader_parse_dxbc(dxbc, 0, &desc, messages)) < 0)
        return ret;

    for (i = 0; i < desc.section_count; ++i)
    {
        uint32_t tag = desc.sections[i].tag;
        if (tag == TAG_SHDR || tag == TAG_SHEX)
        {
            *type = VKD3D_SHADER_SOURCE_DXBC_TPF;
        }
        else if (tag == TAG_DXIL)
        {
            *type = VKD3D_SHADER_SOURCE_DXBC_DXIL;
            /* Default to DXIL if both are present. */
            break;
        }
    }

    vkd3d_shader_free_dxbc(&desc);

    if (*type == VKD3D_SHADER_SOURCE_NONE)
        return VKD3D_ERROR_INVALID_SHADER;

    return VKD3D_OK;
}

static inline enum vkd3d_result vkd3d_shader_code_from_file(struct vkd3d_shader_code *shader, FILE *f)
{
    size_t size = 4096;
    struct stat st;
    size_t pos = 0;
    uint8_t *data;
    size_t ret;

    memset(shader, 0, sizeof(*shader));

    if (fstat(fileno(f), &st) == -1)
        return VKD3D_ERROR;

    if (S_ISREG(st.st_mode))
        size = st.st_size;

    if (!(data = malloc(size)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    for (;;)
    {
        if (pos >= size)
        {
            if (size > SIZE_MAX / 2 || !(data = realloc(data, size * 2)))
            {
                free(data);
                return VKD3D_ERROR_OUT_OF_MEMORY;
            }
            size *= 2;
        }

        if (!(ret = fread(&data[pos], 1, size - pos, f)))
            break;
        pos += ret;
    }

    if (!feof(f))
    {
        free(data);
        return VKD3D_ERROR;
    }

    shader->code = data;
    shader->size = pos;

    return VKD3D_OK;
}

#endif  /* __VKD3D_SHADER_UTILS_H */
