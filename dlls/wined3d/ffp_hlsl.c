/*
 * Fixed-function pipeline replacement implemented using HLSL shaders
 *
 * Copyright 2006 Jason Green
 * Copyright 2006-2007 Henri Verbeet
 * Copyright 2007-2009,2013 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
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

static bool ffp_hlsl_generate_vertex_shader(const struct wined3d_ffp_vs_settings *settings,
        struct wined3d_string_buffer *buffer)
{
    if (settings->lighting)
        FIXME("Ignoring lighting.\n");

    if (settings->point_size)
        FIXME("Ignoring point size.\n");

    if (settings->transformed)
        FIXME("Ignoring pretransformed vertices.\n");

    if (settings->vertexblends)
        FIXME("Ignoring vertex blend.\n");

    if (settings->normal)
        FIXME("Ignoring normals.\n");

    if (settings->fog_mode != WINED3D_FFP_VS_FOG_OFF)
        FIXME("Ignoring fog.\n");

    /* This must be kept in sync with struct wined3d_ffp_vs_constants. */
    shader_addline(buffer, "uniform struct\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4x4 modelview_matrices[%u];\n", MAX_VERTEX_BLENDS);
    shader_addline(buffer, "    float4x4 projection_matrix;\n");
    shader_addline(buffer, "    float4x4 texture_matrices[%u];\n", WINED3D_MAX_FFP_TEXTURES);
    shader_addline(buffer, "    float4 point_params;\n");
    shader_addline(buffer, "    struct\n");
    shader_addline(buffer, "    {\n");
    shader_addline(buffer, "        float4 diffuse;\n");
    shader_addline(buffer, "        float4 ambient;\n");
    shader_addline(buffer, "        float4 specular;\n");
    shader_addline(buffer, "        float4 emissive;\n");
    shader_addline(buffer, "        float power;\n");
    shader_addline(buffer, "    } material;\n");
    shader_addline(buffer, "    float4 ambient_colour;\n");
    shader_addline(buffer, "    struct\n");
    shader_addline(buffer, "    {\n");
    shader_addline(buffer, "        float4 diffuse, specular, ambient;\n");
    shader_addline(buffer, "        float4 position, direction;\n");
    shader_addline(buffer, "        float4 packed_params;\n");
    shader_addline(buffer, "        float4 attenuation;\n");
    shader_addline(buffer, "    } lights[%u];\n", WINED3D_MAX_ACTIVE_LIGHTS);
    shader_addline(buffer, "} c;\n");

    shader_addline(buffer, "struct input\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4 pos : POSITION;\n");
    shader_addline(buffer, "    float4 blend_weight : BLENDWEIGHT;\n");
    shader_addline(buffer, "    uint blend_indices : BLENDINDICES;\n");
    shader_addline(buffer, "    float3 normal : NORMAL;\n");
    shader_addline(buffer, "    float point_size : PSIZE;\n");
    shader_addline(buffer, "    float4 diffuse : COLOR0;\n");
    shader_addline(buffer, "    float4 specular : COLOR1;\n");
    shader_addline(buffer, "    float4 texcoord[%u] : TEXCOORD;\n", WINED3D_MAX_FFP_TEXTURES);
    shader_addline(buffer, "};\n\n");

    shader_addline(buffer, "struct output\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4 pos : POSITION;\n");
    shader_addline(buffer, "    float4 diffuse : COLOR0;\n");
    shader_addline(buffer, "    float4 specular : COLOR1;\n");
    for (unsigned int i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        if (((settings->texgen[i] & 0xffff0000) != WINED3DTSS_TCI_PASSTHRU) || (settings->texcoords & (1u << i)))
            shader_addline(buffer, "    float4 texcoord%u : TEXCOORD%u;\n", i, i);
    }
    shader_addline(buffer, "};\n\n");

    shader_addline(buffer, "void main(in struct input i, out struct output o)\n");
    shader_addline(buffer, "{\n");
    shader_addline(buffer, "    float4 ec_pos = 0.0;\n\n");

    shader_addline(buffer, "    ec_pos += mul(c.modelview_matrices[0], float4(i.pos.xyz, 1.0));\n\n");

    shader_addline(buffer, "    o.pos = mul(c.projection_matrix, ec_pos);\n");
    shader_addline(buffer, "    ec_pos /= ec_pos.w;\n\n");

    /* No lighting. */
    if (settings->diffuse)
        shader_addline(buffer, "    o.diffuse = i.diffuse;\n");
    else
        shader_addline(buffer, "    o.diffuse = 1.0;\n");
    shader_addline(buffer, "    o.specular = i.specular;\n\n");

    for (unsigned int i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
    {
        switch (settings->texgen[i] & 0xffff0000)
        {
            case WINED3DTSS_TCI_PASSTHRU:
                if (settings->texcoords & (1u << i))
                    shader_addline(buffer, "    o.texcoord%u = mul(c.texture_matrices[%u], i.texcoord[%u]);\n",
                            i, i, settings->texgen[i] & 0x0000ffff);
                else
                    continue;
                break;

            default:
                FIXME("Unhandled texgen %#x.\n", settings->texgen[i]);
                break;
        }
    }

    switch (settings->fog_mode)
    {
        case WINED3D_FFP_VS_FOG_OFF:
            break;

        default:
            FIXME("Unhandled fog mode %#x.\n", settings->fog_mode);
            break;
    }

    shader_addline(buffer, "}\n");

    return true;
}

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

bool ffp_hlsl_compile_vs(const struct wined3d_ffp_vs_settings *settings, struct wined3d_shader_desc *shader_desc)
{
    struct wined3d_string_buffer string;
    struct vkd3d_shader_code sm1;

    if (!string_buffer_init(&string))
        return false;

    if (!ffp_hlsl_generate_vertex_shader(settings, &string))
    {
        string_buffer_free(&string);
        return false;
    }

    if (!compile_hlsl_shader(&string, &sm1, "vs_2_0"))
    {
        string_buffer_free(&string);
        return false;
    }
    string_buffer_free(&string);

    shader_desc->byte_code = sm1.code;
    shader_desc->byte_code_size = ~(size_t)0;
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
