/*
 * Fixed-function pipeline replacement implemented using HLSL shaders
 *
 * Copyright 2022,2024 Elizabeth Figura for CodeWeavers
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

#include "wined3d_private.h"
#include <vkd3d_shader.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

static bool ffp_hlsl_generate_pixel_shader(const struct ffp_frag_settings *settings,
        struct wined3d_string_buffer *string)
{
    FIXME("Not yet implemented.\n");
    return false;
}

static bool compile_hlsl_shader(const struct wined3d_string_buffer *hlsl,
        struct vkd3d_shader_code *sm1, const char *profile)
{
    struct vkd3d_shader_hlsl_source_info hlsl_source_info = {.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO};
    struct vkd3d_shader_compile_info compile_info = {.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO};
    char *messages;
    int ret;

    compile_info.source.code = hlsl->buffer;
    compile_info.source.size = hlsl->content_size;
    compile_info.source_type = VKD3D_SHADER_SOURCE_HLSL;
    compile_info.target_type = VKD3D_SHADER_TARGET_D3D_BYTECODE;
    compile_info.log_level = VKD3D_SHADER_LOG_WARNING;

    compile_info.next = &hlsl_source_info;
    hlsl_source_info.profile = profile;

    ret = vkd3d_shader_compile(&compile_info, sm1, &messages);
    if (messages && *messages && FIXME_ON(d3d_shader))
    {
        const char *ptr, *end, *line;

        FIXME("Shader log:\n");
        ptr = messages;
        end = ptr + strlen(ptr);
        while ((line = wined3d_get_line(&ptr, end)))
            FIXME("    %.*s", (int)(ptr - line), line);
        FIXME("\n");
    }
    vkd3d_shader_free_messages(messages);

    if (ret < 0)
    {
        ERR("Failed to compile HLSL, ret %d.\n", ret);
        return false;
    }

    return true;
}

bool ffp_hlsl_compile_ps(const struct ffp_frag_settings *settings, struct wined3d_shader_desc *shader_desc)
{
    struct wined3d_string_buffer string;
    struct vkd3d_shader_code sm1;

    if (!string_buffer_init(&string))
        return false;

    if (!ffp_hlsl_generate_pixel_shader(settings, &string))
    {
        string_buffer_free(&string);
        return false;
    }

    if (!compile_hlsl_shader(&string, &sm1, "ps_2_0"))
    {
        string_buffer_free(&string);
        return false;
    }
    string_buffer_free(&string);

    shader_desc->byte_code = sm1.code;
    shader_desc->byte_code_size = ~(size_t)0;
    return true;
}
