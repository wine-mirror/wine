/*
 * Copyright 2021 Atharva Nimbalkar
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

#include "vkd3d_shader_private.h"

struct vkd3d_glsl_generator
{
    struct vkd3d_shader_version version;
    struct vkd3d_string_buffer buffer;
    struct vkd3d_shader_location location;
    struct vkd3d_shader_message_context *message_context;
    bool failed;
};

struct vkd3d_glsl_generator *vkd3d_glsl_generator_create(const struct vkd3d_shader_version *version,
        struct vkd3d_shader_message_context *message_context, const struct vkd3d_shader_location *location)
{
    struct vkd3d_glsl_generator *generator;

    if (!(generator = vkd3d_malloc(sizeof(*generator))))
        return NULL;

    memset(generator, 0, sizeof(*generator));
    generator->version = *version;
    vkd3d_string_buffer_init(&generator->buffer);
    generator->location = *location;
    generator->message_context = message_context;
    return generator;
}

static void VKD3D_PRINTF_FUNC(3, 4) vkd3d_glsl_compiler_error(
        struct vkd3d_glsl_generator *generator,
        enum vkd3d_shader_error error, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vkd3d_shader_verror(generator->message_context, &generator->location, error, fmt, args);
    va_end(args);
    generator->failed = true;
}

static void shader_glsl_ret(struct vkd3d_glsl_generator *generator,
        const struct vkd3d_shader_instruction *ins)
{
    const struct vkd3d_shader_version *version = &generator->version;

    /*
    * TODO: Implement in_subroutine
    * TODO: shader_glsl_generate_shader_epilogue(generator);
    */
    if (version->major >= 4)
    {
        vkd3d_string_buffer_printf(&generator->buffer, "return;\n");
    }
}

static void vkd3d_glsl_handle_instruction(struct vkd3d_glsl_generator *generator,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->handler_idx)
    {
        case VKD3DSIH_DCL_INPUT:
        case VKD3DSIH_DCL_OUTPUT:
        case VKD3DSIH_DCL_OUTPUT_SIV:
            break;
        case VKD3DSIH_RET:
            shader_glsl_ret(generator, instruction);
            break;
        default:
            vkd3d_glsl_compiler_error(generator,
                    VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Unhandled instruction %#x", instruction->handler_idx);
            break;
    }
}

int vkd3d_glsl_generator_generate(struct vkd3d_glsl_generator *generator,
        struct vkd3d_shader_parser *parser, struct vkd3d_shader_code *out)
{
    unsigned int i;
    void *code;

    vkd3d_string_buffer_printf(&generator->buffer, "#version 440\n\n");
    vkd3d_string_buffer_printf(&generator->buffer, "void main()\n{\n");

    generator->location.column = 0;
    for (i = 0; i < parser->instructions.count; ++i)
    {
        generator->location.line = i + 1;
        vkd3d_glsl_handle_instruction(generator, &parser->instructions.elements[i]);
    }

    if (generator->failed)
        return VKD3D_ERROR_INVALID_SHADER;

    vkd3d_string_buffer_printf(&generator->buffer, "}\n");

    if ((code = vkd3d_malloc(generator->buffer.buffer_size)))
    {
        memcpy(code, generator->buffer.buffer, generator->buffer.content_size);
        out->size = generator->buffer.content_size;
        out->code = code;
    }
    else return VKD3D_ERROR_OUT_OF_MEMORY;

    return VKD3D_OK;
}

void vkd3d_glsl_generator_destroy(struct vkd3d_glsl_generator *generator)
{
    vkd3d_string_buffer_cleanup(&generator->buffer);
    vkd3d_free(generator);
}
