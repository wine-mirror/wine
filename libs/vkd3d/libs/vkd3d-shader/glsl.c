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

struct glsl_src
{
    struct vkd3d_string_buffer *str;
};

struct glsl_dst
{
    const struct vkd3d_shader_dst_param *vsir;
    struct vkd3d_string_buffer *register_name;
    struct vkd3d_string_buffer *mask;
};

struct vkd3d_glsl_generator
{
    struct vsir_program *program;
    struct vkd3d_string_buffer_cache string_buffers;
    struct vkd3d_string_buffer *buffer;
    struct vkd3d_shader_location location;
    struct vkd3d_shader_message_context *message_context;
    unsigned int indent;
    bool failed;
};

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

static void shader_glsl_print_indent(struct vkd3d_string_buffer *buffer, unsigned int indent)
{
    vkd3d_string_buffer_printf(buffer, "%*s", 4 * indent, "");
}

static void shader_glsl_print_register_name(struct vkd3d_string_buffer *buffer,
        struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_register *reg)
{
    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            vkd3d_string_buffer_printf(buffer, "r[%u]", reg->idx[0].offset);
            break;

        default:
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled register type %#x.", reg->type);
            vkd3d_string_buffer_printf(buffer, "<unrecognised register %#x>", reg->type);
            break;
    }
}

static void shader_glsl_print_swizzle(struct vkd3d_string_buffer *buffer, uint32_t swizzle, uint32_t mask)
{
    const char swizzle_chars[] = "xyzw";
    unsigned int i;

    vkd3d_string_buffer_printf(buffer, ".");
    for (i = 0; i < VKD3D_VEC4_SIZE; ++i)
    {
        if (mask & (VKD3DSP_WRITEMASK_0 << i))
            vkd3d_string_buffer_printf(buffer, "%c", swizzle_chars[vsir_swizzle_get_component(swizzle, i)]);
    }
}

static void shader_glsl_print_write_mask(struct vkd3d_string_buffer *buffer, uint32_t write_mask)
{
    vkd3d_string_buffer_printf(buffer, ".");
    if (write_mask & VKD3DSP_WRITEMASK_0)
        vkd3d_string_buffer_printf(buffer, "x");
    if (write_mask & VKD3DSP_WRITEMASK_1)
        vkd3d_string_buffer_printf(buffer, "y");
    if (write_mask & VKD3DSP_WRITEMASK_2)
        vkd3d_string_buffer_printf(buffer, "z");
    if (write_mask & VKD3DSP_WRITEMASK_3)
        vkd3d_string_buffer_printf(buffer, "w");
}

static void glsl_src_cleanup(struct glsl_src *src, struct vkd3d_string_buffer_cache *cache)
{
    vkd3d_string_buffer_release(cache, src->str);
}

static void glsl_src_init(struct glsl_src *glsl_src, struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_src_param *vsir_src, uint32_t mask)
{
    const struct vkd3d_shader_register *reg = &vsir_src->reg;

    glsl_src->str = vkd3d_string_buffer_get(&gen->string_buffers);

    if (reg->non_uniform)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled 'non-uniform' modifier.");
    if (vsir_src->modifiers)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled source modifier(s) %#x.", vsir_src->modifiers);

    shader_glsl_print_register_name(glsl_src->str, gen, reg);
    if (reg->dimension == VSIR_DIMENSION_VEC4)
        shader_glsl_print_swizzle(glsl_src->str, vsir_src->swizzle, mask);
}

static void glsl_dst_cleanup(struct glsl_dst *dst, struct vkd3d_string_buffer_cache *cache)
{
    vkd3d_string_buffer_release(cache, dst->mask);
    vkd3d_string_buffer_release(cache, dst->register_name);
}

static uint32_t glsl_dst_init(struct glsl_dst *glsl_dst, struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins, const struct vkd3d_shader_dst_param *vsir_dst)
{
    uint32_t write_mask = vsir_dst->write_mask;

    if (ins->flags & VKD3DSI_PRECISE_XYZW)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled 'precise' modifier.");
    if (vsir_dst->reg.non_uniform)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled 'non-uniform' modifier.");

    glsl_dst->vsir = vsir_dst;
    glsl_dst->register_name = vkd3d_string_buffer_get(&gen->string_buffers);
    glsl_dst->mask = vkd3d_string_buffer_get(&gen->string_buffers);

    shader_glsl_print_register_name(glsl_dst->register_name, gen, &vsir_dst->reg);
    shader_glsl_print_write_mask(glsl_dst->mask, write_mask);

    return write_mask;
}

static void VKD3D_PRINTF_FUNC(3, 4) shader_glsl_print_assignment(
        struct vkd3d_glsl_generator *gen, struct glsl_dst *dst, const char *format, ...)
{
    va_list args;

    if (dst->vsir->shift)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled destination shift %#x.", dst->vsir->shift);
    if (dst->vsir->modifiers)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled destination modifier(s) %#x.", dst->vsir->modifiers);

    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "%s%s = ", dst->register_name->buffer, dst->mask->buffer);

    va_start(args, format);
    vkd3d_string_buffer_vprintf(gen->buffer, format, args);
    va_end(args);

    vkd3d_string_buffer_printf(gen->buffer, ";\n");
}

static void shader_glsl_unhandled(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "/* <unhandled instruction %#x> */\n", ins->opcode);
    vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
            "Internal compiler error: Unhandled instruction %#x.", ins->opcode);
}

static void shader_glsl_mov(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    struct glsl_src src;
    struct glsl_dst dst;
    uint32_t mask;

    mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&src, gen, &ins->src[0], mask);

    shader_glsl_print_assignment(gen, &dst, "%s", src.str->buffer);

    glsl_src_cleanup(&src, &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_ret(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    const struct vkd3d_shader_version *version = &gen->program->shader_version;

    /*
    * TODO: Implement in_subroutine
    * TODO: shader_glsl_generate_shader_epilogue(generator);
    */
    if (version->major >= 4)
    {
        shader_glsl_print_indent(gen->buffer, gen->indent);
        vkd3d_string_buffer_printf(gen->buffer, "return;\n");
    }
}

static void vkd3d_glsl_handle_instruction(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins)
{
    gen->location = ins->location;

    switch (ins->opcode)
    {
        case VKD3DSIH_DCL_INPUT:
        case VKD3DSIH_DCL_OUTPUT:
        case VKD3DSIH_DCL_OUTPUT_SIV:
        case VKD3DSIH_NOP:
            break;
        case VKD3DSIH_MOV:
            shader_glsl_mov(gen, ins);
            break;
        case VKD3DSIH_RET:
            shader_glsl_ret(gen, ins);
            break;
        default:
            shader_glsl_unhandled(gen, ins);
            break;
    }
}

static void shader_glsl_generate_declarations(struct vkd3d_glsl_generator *gen)
{
    const struct vsir_program *program = gen->program;
    struct vkd3d_string_buffer *buffer = gen->buffer;

    if (program->temp_count)
        vkd3d_string_buffer_printf(buffer, "vec4 r[%u];\n\n", program->temp_count);
}

static int vkd3d_glsl_generator_generate(struct vkd3d_glsl_generator *gen, struct vkd3d_shader_code *out)
{
    const struct vkd3d_shader_instruction_array *instructions = &gen->program->instructions;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    unsigned int i;
    void *code;

    MESSAGE("Generating a GLSL shader. This is unsupported; you get to keep all the pieces if it breaks.\n");

    vkd3d_string_buffer_printf(buffer, "#version 440\n\n");

    vkd3d_string_buffer_printf(buffer, "/* Generated by %s. */\n\n", vkd3d_shader_get_version(NULL, NULL));

    shader_glsl_generate_declarations(gen);

    vkd3d_string_buffer_printf(buffer, "void main()\n{\n");

    ++gen->indent;
    for (i = 0; i < instructions->count; ++i)
    {
        vkd3d_glsl_handle_instruction(gen, &instructions->elements[i]);
    }

    vkd3d_string_buffer_printf(buffer, "}\n");

    if (TRACE_ON())
        vkd3d_string_buffer_trace(buffer);

    if (gen->failed)
        return VKD3D_ERROR_INVALID_SHADER;

    if ((code = vkd3d_malloc(buffer->buffer_size)))
    {
        memcpy(code, buffer->buffer, buffer->content_size);
        out->size = buffer->content_size;
        out->code = code;
    }
    else return VKD3D_ERROR_OUT_OF_MEMORY;

    return VKD3D_OK;
}

static void vkd3d_glsl_generator_cleanup(struct vkd3d_glsl_generator *gen)
{
    vkd3d_string_buffer_release(&gen->string_buffers, gen->buffer);
    vkd3d_string_buffer_cache_cleanup(&gen->string_buffers);
}

static void vkd3d_glsl_generator_init(struct vkd3d_glsl_generator *gen,
        struct vsir_program *program, struct vkd3d_shader_message_context *message_context)
{
    memset(gen, 0, sizeof(*gen));
    gen->program = program;
    vkd3d_string_buffer_cache_init(&gen->string_buffers);
    gen->buffer = vkd3d_string_buffer_get(&gen->string_buffers);
    gen->message_context = message_context;
}

int glsl_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_code *out,
        struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_glsl_generator generator;
    int ret;

    if ((ret = vsir_program_normalise(program, config_flags, compile_info, message_context)) < 0)
        return ret;

    vkd3d_glsl_generator_init(&generator, program, message_context);
    ret = vkd3d_glsl_generator_generate(&generator, out);
    vkd3d_glsl_generator_cleanup(&generator);

    return ret;
}
