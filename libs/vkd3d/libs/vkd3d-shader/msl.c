/*
 * Copyright 2024 Feifan He for CodeWeavers
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

enum msl_data_type
{
    MSL_DATA_FLOAT,
    MSL_DATA_UINT,
    MSL_DATA_UNION,
};

struct msl_src
{
    struct vkd3d_string_buffer *str;
};

struct msl_dst
{
    const struct vkd3d_shader_dst_param *vsir;
    struct vkd3d_string_buffer *register_name;
    struct vkd3d_string_buffer *mask;
};

struct msl_generator
{
    struct vsir_program *program;
    struct vkd3d_string_buffer_cache string_buffers;
    struct vkd3d_string_buffer *buffer;
    struct vkd3d_shader_location location;
    struct vkd3d_shader_message_context *message_context;
    unsigned int indent;
    const char *prefix;
    bool failed;

    bool write_depth;

    const struct vkd3d_shader_interface_info *interface_info;
};

struct msl_resource_type_info
{
    size_t read_coord_size;
    bool array;
    bool lod;
    const char *type_suffix;
};

static void VKD3D_PRINTF_FUNC(3, 4) msl_compiler_error(struct msl_generator *gen,
        enum vkd3d_shader_error error, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vkd3d_shader_verror(gen->message_context, &gen->location, error, fmt, args);
    va_end(args);
    gen->failed = true;
}

static const struct msl_resource_type_info *msl_get_resource_type_info(enum vkd3d_shader_resource_type t)
{
    static const struct msl_resource_type_info info[] =
    {
        [VKD3D_SHADER_RESOURCE_NONE]              = {0, false, false, "none"},
        [VKD3D_SHADER_RESOURCE_BUFFER]            = {1, false, false, "_buffer"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_1D]        = {1, false, false, "1d"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_2D]        = {2, false, true,  "2d"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_2DMS]      = {2, false, false, "2d_ms"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_3D]        = {3, false, true,  "3d"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_CUBE]      = {2, false, true,  "cube"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_1DARRAY]   = {1, true,  false, "1d_array"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY]   = {2, true,  true,  "2d_array"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY] = {2, true,  false, "2d_ms_array"},
        [VKD3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY] = {2, true,  true,  "cube_array"},
    };

    if (!t || t >= ARRAY_SIZE(info))
        return NULL;

    return &info[t];
}


static const char *msl_get_prefix(enum vkd3d_shader_type type)
{
    switch (type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            return "vs";
        case VKD3D_SHADER_TYPE_HULL:
            return "hs";
        case VKD3D_SHADER_TYPE_DOMAIN:
            return "ds";
        case VKD3D_SHADER_TYPE_GEOMETRY:
            return "gs";
        case VKD3D_SHADER_TYPE_PIXEL:
            return "ps";
        case VKD3D_SHADER_TYPE_COMPUTE:
            return "cs";
        default:
            return NULL;
    }
}

static void msl_print_indent(struct vkd3d_string_buffer *buffer, unsigned int indent)
{
    vkd3d_string_buffer_printf(buffer, "%*s", 4 * indent, "");
}

static void msl_print_resource_datatype(struct msl_generator *gen,
        struct vkd3d_string_buffer *buffer, enum vkd3d_data_type data_type)
{
    switch (data_type)
    {
        case VKD3D_DATA_FLOAT:
        case VKD3D_DATA_UNORM:
        case VKD3D_DATA_SNORM:
            vkd3d_string_buffer_printf(buffer, "float");
            break;
        case VKD3D_DATA_INT:
            vkd3d_string_buffer_printf(buffer, "int");
            break;
        case VKD3D_DATA_UINT:
            vkd3d_string_buffer_printf(buffer, "uint");
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled resource datatype %#x.", data_type);
            vkd3d_string_buffer_printf(buffer, "<unrecognised resource datatype %#x>", data_type);
            break;
    }
}

static void msl_print_register_datatype(struct vkd3d_string_buffer *buffer,
        struct msl_generator *gen, enum vkd3d_data_type data_type)
{
    vkd3d_string_buffer_printf(buffer, ".");
    switch (data_type)
    {
        case VKD3D_DATA_FLOAT:
            vkd3d_string_buffer_printf(buffer, "f");
            break;
        case VKD3D_DATA_INT:
            vkd3d_string_buffer_printf(buffer, "i");
            break;
        case VKD3D_DATA_UINT:
            vkd3d_string_buffer_printf(buffer, "u");
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled register datatype %#x.", data_type);
            vkd3d_string_buffer_printf(buffer, "<unrecognised register datatype %#x>", data_type);
            break;
    }
}

static bool msl_check_shader_visibility(const struct msl_generator *gen,
        enum vkd3d_shader_visibility visibility)
{
    enum vkd3d_shader_type t = gen->program->shader_version.type;

    switch (visibility)
    {
        case VKD3D_SHADER_VISIBILITY_ALL:
            return true;
        case VKD3D_SHADER_VISIBILITY_VERTEX:
            return t == VKD3D_SHADER_TYPE_VERTEX;
        case VKD3D_SHADER_VISIBILITY_HULL:
            return t == VKD3D_SHADER_TYPE_HULL;
        case VKD3D_SHADER_VISIBILITY_DOMAIN:
            return t == VKD3D_SHADER_TYPE_DOMAIN;
        case VKD3D_SHADER_VISIBILITY_GEOMETRY:
            return t == VKD3D_SHADER_TYPE_GEOMETRY;
        case VKD3D_SHADER_VISIBILITY_PIXEL:
            return t == VKD3D_SHADER_TYPE_PIXEL;
        case VKD3D_SHADER_VISIBILITY_COMPUTE:
            return t == VKD3D_SHADER_TYPE_COMPUTE;
        default:
            WARN("Invalid shader visibility %#x.\n", visibility);
            return false;
    }
}

static const struct vkd3d_shader_descriptor_binding *msl_get_cbv_binding(const struct msl_generator *gen,
        unsigned int register_space, unsigned int register_idx)
{
    const struct vkd3d_shader_interface_info *interface_info = gen->interface_info;
    unsigned int i;

    if (!interface_info)
        return NULL;

    for (i = 0; i < interface_info->binding_count; ++i)
    {
        const struct vkd3d_shader_resource_binding *binding = &interface_info->bindings[i];

        if (binding->type != VKD3D_SHADER_DESCRIPTOR_TYPE_CBV)
            continue;
        if (binding->register_space != register_space)
            continue;
        if (binding->register_index != register_idx)
            continue;
        if (!msl_check_shader_visibility(gen, binding->shader_visibility))
            continue;
        if (!(binding->flags & VKD3D_SHADER_BINDING_FLAG_BUFFER))
            continue;

        return &binding->binding;
    }

    return NULL;
}

static const struct vkd3d_shader_descriptor_binding *msl_get_srv_binding(const struct msl_generator *gen,
        unsigned int register_space, unsigned int register_idx, enum vkd3d_shader_resource_type resource_type)
{
    const struct vkd3d_shader_interface_info *interface_info = gen->interface_info;
    enum vkd3d_shader_binding_flag resource_type_flag;
    unsigned int i;

    if (!interface_info)
        return NULL;

    resource_type_flag = resource_type == VKD3D_SHADER_RESOURCE_BUFFER
            ? VKD3D_SHADER_BINDING_FLAG_BUFFER : VKD3D_SHADER_BINDING_FLAG_IMAGE;

    for (i = 0; i < interface_info->binding_count; ++i)
    {
        const struct vkd3d_shader_resource_binding *binding = &interface_info->bindings[i];

        if (binding->type != VKD3D_SHADER_DESCRIPTOR_TYPE_SRV)
            continue;
        if (binding->register_space != register_space)
            continue;
        if (binding->register_index != register_idx)
            continue;
        if (!msl_check_shader_visibility(gen, binding->shader_visibility))
            continue;
        if (!(binding->flags & resource_type_flag))
            continue;

        return &binding->binding;
    }

    return NULL;
}

static void msl_print_cbv_name(struct vkd3d_string_buffer *buffer, unsigned int binding)
{
    vkd3d_string_buffer_printf(buffer, "descriptors[%u].buf<vkd3d_vec4>()", binding);
}

static void msl_print_srv_name(struct vkd3d_string_buffer *buffer, struct msl_generator *gen, unsigned int binding,
        const struct msl_resource_type_info *resource_type_info, enum vkd3d_data_type resource_data_type)
{
    vkd3d_string_buffer_printf(buffer, "descriptors[%u].tex<texture%s<",
            binding, resource_type_info->type_suffix);
    msl_print_resource_datatype(gen, buffer, resource_data_type);
    vkd3d_string_buffer_printf(buffer, ">>()");
}

static enum msl_data_type msl_print_register_name(struct vkd3d_string_buffer *buffer,
        struct msl_generator *gen, const struct vkd3d_shader_register *reg)
{
    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            vkd3d_string_buffer_printf(buffer, "r[%u]", reg->idx[0].offset);
            return MSL_DATA_UNION;

        case VKD3DSPR_INPUT:
            if (reg->idx_count != 1)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled input register index count %u.", reg->idx_count);
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                return MSL_DATA_UNION;
            }
            if (reg->idx[0].rel_addr)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled input register indirect addressing.");
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                return MSL_DATA_UNION;
            }
            vkd3d_string_buffer_printf(buffer, "v[%u]", reg->idx[0].offset);
            return MSL_DATA_UNION;

        case VKD3DSPR_OUTPUT:
            if (reg->idx_count != 1)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled output register index count %u.", reg->idx_count);
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                return MSL_DATA_UNION;
            }
            if (reg->idx[0].rel_addr)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled output register indirect addressing.");
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                return MSL_DATA_UNION;
            }
            vkd3d_string_buffer_printf(buffer, "o[%u]", reg->idx[0].offset);
            return MSL_DATA_UNION;

        case VKD3DSPR_DEPTHOUT:
            if (gen->program->shader_version.type != VKD3D_SHADER_TYPE_PIXEL)
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled depth output in shader type #%x.",
                        gen->program->shader_version.type);
            vkd3d_string_buffer_printf(buffer, "o_depth");
            return MSL_DATA_FLOAT;

        case VKD3DSPR_IMMCONST:
            switch (reg->dimension)
            {
                case VSIR_DIMENSION_SCALAR:
                    vkd3d_string_buffer_printf(buffer, "%#xu", reg->u.immconst_u32[0]);
                    return MSL_DATA_UINT;

                case VSIR_DIMENSION_VEC4:
                    vkd3d_string_buffer_printf(buffer, "uint4(%#xu, %#xu, %#xu, %#xu)",
                            reg->u.immconst_u32[0], reg->u.immconst_u32[1],
                            reg->u.immconst_u32[2], reg->u.immconst_u32[3]);
                    return MSL_DATA_UINT;

                default:
                    vkd3d_string_buffer_printf(buffer, "<unhandled_dimension %#x>", reg->dimension);
                    msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                            "Internal compiler error: Unhandled dimension %#x.", reg->dimension);
                    return MSL_DATA_UINT;
            }

        case VKD3DSPR_CONSTBUFFER:
            {
                const struct vkd3d_shader_descriptor_binding *binding;

                if (reg->idx_count != 3)
                {
                    msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                            "Internal compiler error: Unhandled constant buffer register index count %u.",
                            reg->idx_count);
                    vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                    return MSL_DATA_UNION;
                }
                if (reg->idx[0].rel_addr || reg->idx[1].rel_addr || reg->idx[2].rel_addr)
                {
                    msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                            "Internal compiler error: Unhandled constant buffer register indirect addressing.");
                    vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                    return MSL_DATA_UNION;
                }
                if (!(binding = msl_get_cbv_binding(gen, 0, reg->idx[1].offset)))
                {
                    msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_BINDING_NOT_FOUND,
                            "Cannot finding binding for CBV register %u.", reg->idx[0].offset);
                    vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                    return MSL_DATA_UNION;
                }
                msl_print_cbv_name(buffer, binding->binding);
                vkd3d_string_buffer_printf(buffer, "[%u]", reg->idx[2].offset);
                return MSL_DATA_UNION;
            }

        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled register type %#x.", reg->type);
            vkd3d_string_buffer_printf(buffer, "<unrecognised register %#x>", reg->type);
            return MSL_DATA_UINT;
    }
}

static void msl_print_swizzle(struct vkd3d_string_buffer *buffer, uint32_t swizzle, uint32_t mask)
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

static void msl_print_write_mask(struct vkd3d_string_buffer *buffer, uint32_t write_mask)
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

static void msl_src_cleanup(struct msl_src *src, struct vkd3d_string_buffer_cache *cache)
{
    vkd3d_string_buffer_release(cache, src->str);
}

static void msl_print_bitcast(struct vkd3d_string_buffer *dst, struct msl_generator *gen, const char *src,
        enum vkd3d_data_type dst_data_type, enum msl_data_type src_data_type, enum vsir_dimension dimension)
{
    bool write_cast = false;

    if (dst_data_type == VKD3D_DATA_UNORM || dst_data_type == VKD3D_DATA_SNORM)
        dst_data_type = VKD3D_DATA_FLOAT;

    switch (src_data_type)
    {
        case MSL_DATA_FLOAT:
            write_cast = dst_data_type != VKD3D_DATA_FLOAT;
            break;

        case MSL_DATA_UINT:
            write_cast = dst_data_type != VKD3D_DATA_UINT;
            break;

        case MSL_DATA_UNION:
            break;
    }

    if (write_cast)
    {
        vkd3d_string_buffer_printf(dst, "as_type<");
        msl_print_resource_datatype(gen, dst, dst_data_type);
        vkd3d_string_buffer_printf(dst, "%s>(", dimension == VSIR_DIMENSION_VEC4 ? "4" : "");
    }

    vkd3d_string_buffer_printf(dst, "%s", src);

    if (write_cast)
        vkd3d_string_buffer_printf(dst, ")");

    if (src_data_type == MSL_DATA_UNION)
        msl_print_register_datatype(dst, gen, dst_data_type);
}

static void msl_print_src_with_type(struct vkd3d_string_buffer *buffer, struct msl_generator *gen,
    const struct vkd3d_shader_src_param *vsir_src, uint32_t mask, enum vkd3d_data_type data_type)
{
    const struct vkd3d_shader_register *reg = &vsir_src->reg;
    struct vkd3d_string_buffer *register_name, *str;
    enum msl_data_type src_data_type;

    register_name = vkd3d_string_buffer_get(&gen->string_buffers);

    if (reg->non_uniform)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled 'non-uniform' modifier.");

    if (!vsir_src->modifiers)
        str = buffer;
    else
        str = vkd3d_string_buffer_get(&gen->string_buffers);

    src_data_type = msl_print_register_name(register_name, gen, reg);
    msl_print_bitcast(str, gen, register_name->buffer, data_type, src_data_type, reg->dimension);
    if (reg->dimension == VSIR_DIMENSION_VEC4)
        msl_print_swizzle(str, vsir_src->swizzle, mask);

    switch (vsir_src->modifiers)
    {
        case VKD3DSPSM_NONE:
            break;
        case VKD3DSPSM_NEG:
            vkd3d_string_buffer_printf(buffer, "-%s", str->buffer);
            break;
        case VKD3DSPSM_ABS:
            vkd3d_string_buffer_printf(buffer, "abs(%s)", str->buffer);
            break;
        default:
            vkd3d_string_buffer_printf(buffer, "<unhandled modifier %#x>(%s)",
                    vsir_src->modifiers, str->buffer);
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled source modifier(s) %#x.", vsir_src->modifiers);
            break;
    }

    if (str != buffer)
        vkd3d_string_buffer_release(&gen->string_buffers, str);
}

static void msl_src_init(struct msl_src *msl_src, struct msl_generator *gen,
        const struct vkd3d_shader_src_param *vsir_src, uint32_t mask)
{
    msl_src->str = vkd3d_string_buffer_get(&gen->string_buffers);
    msl_print_src_with_type(msl_src->str, gen, vsir_src, mask, vsir_src->reg.data_type);
}

static void msl_dst_cleanup(struct msl_dst *dst, struct vkd3d_string_buffer_cache *cache)
{
    vkd3d_string_buffer_release(cache, dst->mask);
    vkd3d_string_buffer_release(cache, dst->register_name);
}

static uint32_t msl_dst_init(struct msl_dst *msl_dst, struct msl_generator *gen,
        const struct vkd3d_shader_instruction *ins, const struct vkd3d_shader_dst_param *vsir_dst)
{
    uint32_t write_mask = vsir_dst->write_mask;
    enum msl_data_type dst_data_type;

    if (ins->flags & VKD3DSI_PRECISE_XYZW)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled 'precise' modifier.");
    if (vsir_dst->reg.non_uniform)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled 'non-uniform' modifier.");

    msl_dst->vsir = vsir_dst;
    msl_dst->register_name = vkd3d_string_buffer_get(&gen->string_buffers);
    msl_dst->mask = vkd3d_string_buffer_get(&gen->string_buffers);

    dst_data_type = msl_print_register_name(msl_dst->register_name, gen, &vsir_dst->reg);
    if (dst_data_type == MSL_DATA_UNION)
        msl_print_register_datatype(msl_dst->mask, gen, vsir_dst->reg.data_type);
    if (vsir_dst->reg.dimension == VSIR_DIMENSION_VEC4)
        msl_print_write_mask(msl_dst->mask, write_mask);

    return write_mask;
}

static void VKD3D_PRINTF_FUNC(3, 4) msl_print_assignment(
        struct msl_generator *gen, struct msl_dst *dst, const char *format, ...)
{
    uint32_t modifiers = dst->vsir->modifiers;
    va_list args;

    if (dst->vsir->shift)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled destination shift %#x.", dst->vsir->shift);
    if (modifiers & ~VKD3DSPDM_SATURATE)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled destination modifier(s) %#x.", modifiers);

    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "%s%s = ", dst->register_name->buffer, dst->mask->buffer);

    if (modifiers & VKD3DSPDM_SATURATE)
        vkd3d_string_buffer_printf(gen->buffer, "saturate(");

    va_start(args, format);
    vkd3d_string_buffer_vprintf(gen->buffer, format, args);
    va_end(args);

    if (modifiers & VKD3DSPDM_SATURATE)
        vkd3d_string_buffer_printf(gen->buffer, ")");

    vkd3d_string_buffer_printf(gen->buffer, ";\n");
}

static void msl_unhandled(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "/* <unhandled instruction %#x> */\n", ins->opcode);
    msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
            "Internal compiler error: Unhandled instruction %#x.", ins->opcode);
}

static void msl_binop(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins, const char *op)
{
    struct msl_src src[2];
    struct msl_dst dst;
    uint32_t mask;

    mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    msl_src_init(&src[0], gen, &ins->src[0], mask);
    msl_src_init(&src[1], gen, &ins->src[1], mask);

    msl_print_assignment(gen, &dst, "%s %s %s", src[0].str->buffer, op, src[1].str->buffer);

    msl_src_cleanup(&src[1], &gen->string_buffers);
    msl_src_cleanup(&src[0], &gen->string_buffers);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_dot(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins, uint32_t src_mask)
{
    unsigned int component_count;
    struct msl_src src[2];
    struct msl_dst dst;
    uint32_t dst_mask;

    dst_mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    msl_src_init(&src[0], gen, &ins->src[0], src_mask);
    msl_src_init(&src[1], gen, &ins->src[1], src_mask);

    if ((component_count = vsir_write_mask_component_count(dst_mask)) > 1)
        msl_print_assignment(gen, &dst, "float%u(dot(%s, %s))",
                component_count, src[0].str->buffer, src[1].str->buffer);
    else
        msl_print_assignment(gen, &dst, "dot(%s, %s)", src[0].str->buffer, src[1].str->buffer);

    msl_src_cleanup(&src[1], &gen->string_buffers);
    msl_src_cleanup(&src[0], &gen->string_buffers);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_intrinsic(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins, const char *op)
{
    struct vkd3d_string_buffer *args;
    struct msl_src src;
    struct msl_dst dst;
    unsigned int i;
    uint32_t mask;

    mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    args = vkd3d_string_buffer_get(&gen->string_buffers);

    for (i = 0; i < ins->src_count; ++i)
    {
        msl_src_init(&src, gen, &ins->src[i], mask);
        vkd3d_string_buffer_printf(args, "%s%s", i ? ", " : "", src.str->buffer);
        msl_src_cleanup(&src, &gen->string_buffers);
    }

    msl_print_assignment(gen, &dst, "%s(%s)", op, args->buffer);

    vkd3d_string_buffer_release(&gen->string_buffers, args);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_relop(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins, const char *op)
{
    unsigned int mask_size;
    struct msl_src src[2];
    struct msl_dst dst;
    uint32_t mask;

    mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    msl_src_init(&src[0], gen, &ins->src[0], mask);
    msl_src_init(&src[1], gen, &ins->src[1], mask);

    if ((mask_size = vsir_write_mask_component_count(mask)) > 1)
        msl_print_assignment(gen, &dst, "select(uint%u(0u), uint%u(0xffffffffu), bool%u(%s %s %s))",
                mask_size, mask_size, mask_size, src[0].str->buffer, op, src[1].str->buffer);
    else
        msl_print_assignment(gen, &dst, "%s %s %s ? 0xffffffffu : 0u",
                src[0].str->buffer, op, src[1].str->buffer);

    msl_src_cleanup(&src[1], &gen->string_buffers);
    msl_src_cleanup(&src[0], &gen->string_buffers);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_cast(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins, const char *constructor)
{
    unsigned int component_count;
    struct msl_src src;
    struct msl_dst dst;
    uint32_t mask;

    mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    msl_src_init(&src, gen, &ins->src[0], mask);

    if ((component_count = vsir_write_mask_component_count(mask)) > 1)
        msl_print_assignment(gen, &dst, "%s%u(%s)", constructor, component_count, src.str->buffer);
    else
        msl_print_assignment(gen, &dst, "%s(%s)", constructor, src.str->buffer);

    msl_src_cleanup(&src, &gen->string_buffers);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_end_block(struct msl_generator *gen)
{
    --gen->indent;
    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "}\n");
}

static void msl_begin_block(struct msl_generator *gen)
{
    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "{\n");
    ++gen->indent;
}

static void msl_if(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    const char *condition;
    struct msl_src src;

    msl_src_init(&src, gen, &ins->src[0], VKD3DSP_WRITEMASK_0);

    msl_print_indent(gen->buffer, gen->indent);
    condition = ins->flags == VKD3D_SHADER_CONDITIONAL_OP_NZ ? "bool" : "!bool";
    vkd3d_string_buffer_printf(gen->buffer, "if (%s(%s))\n", condition, src.str->buffer);

    msl_src_cleanup(&src, &gen->string_buffers);

    msl_begin_block(gen);
}

static void msl_else(struct msl_generator *gen)
{
    msl_end_block(gen);
    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "else\n");
    msl_begin_block(gen);
}

static void msl_ld(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    const struct msl_resource_type_info *resource_type_info;
    unsigned int resource_id, resource_idx, resource_space;
    const struct vkd3d_shader_descriptor_info1 *descriptor;
    const struct vkd3d_shader_descriptor_binding *binding;
    enum vkd3d_shader_resource_type resource_type;
    struct vkd3d_string_buffer *read;
    enum vkd3d_data_type data_type;
    uint32_t coord_mask;
    struct msl_dst dst;

    if (vkd3d_shader_instruction_has_texel_offset(ins))
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled texel fetch offset.");

    if (ins->src[1].reg.idx[0].rel_addr || ins->src[1].reg.idx[1].rel_addr)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_UNSUPPORTED,
                "Descriptor indexing is not supported.");

    resource_id = ins->src[1].reg.idx[0].offset;
    resource_idx = ins->src[1].reg.idx[1].offset;
    if ((descriptor = vkd3d_shader_find_descriptor(&gen->program->descriptors,
            VKD3D_SHADER_DESCRIPTOR_TYPE_SRV, resource_id)))
    {
        resource_type = descriptor->resource_type;
        resource_space = descriptor->register_space;
        data_type = descriptor->resource_data_type;
    }
    else
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Undeclared resource descriptor %u.", resource_id);
        resource_space = 0;
        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
        data_type = VKD3D_DATA_FLOAT;
    }

    if ((resource_type_info = msl_get_resource_type_info(resource_type)))
    {
        coord_mask = vkd3d_write_mask_from_component_count(resource_type_info->read_coord_size);
    }
    else
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled resource type %#x.", resource_type);
        coord_mask = vkd3d_write_mask_from_component_count(2);
    }

    if (!(binding = msl_get_srv_binding(gen, resource_space, resource_idx, resource_type)))
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_BINDING_NOT_FOUND,
                "Cannot finding binding for SRV register %u index %u space %u.",
                resource_id, resource_idx, resource_space);
        return;
    }

    msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    read = vkd3d_string_buffer_get(&gen->string_buffers);

    vkd3d_string_buffer_printf(read, "as_type<uint4>(");
    msl_print_srv_name(read, gen, binding->binding, resource_type_info, data_type);
    vkd3d_string_buffer_printf(read, ".read(");
    msl_print_src_with_type(read, gen, &ins->src[0], coord_mask, VKD3D_DATA_UINT);
    if (resource_type_info->array)
    {
        vkd3d_string_buffer_printf(read, ", ");
        msl_print_src_with_type(read, gen, &ins->src[0], coord_mask + 1, VKD3D_DATA_UINT);
    }
    if (resource_type_info->lod)
    {
        vkd3d_string_buffer_printf(read, ", ");
        msl_print_src_with_type(read, gen, &ins->src[0], VKD3DSP_WRITEMASK_3, VKD3D_DATA_UINT);
    }
    vkd3d_string_buffer_printf(read, "))");
    msl_print_swizzle(read, ins->src[1].swizzle, ins->dst[0].write_mask);

    msl_print_assignment(gen, &dst, "%s", read->buffer);

    vkd3d_string_buffer_release(&gen->string_buffers, read);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_unary_op(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins, const char *op)
{
    struct msl_src src;
    struct msl_dst dst;
    uint32_t mask;

    mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    msl_src_init(&src, gen, &ins->src[0], mask);

    msl_print_assignment(gen, &dst, "%s%s", op, src.str->buffer);

    msl_src_cleanup(&src, &gen->string_buffers);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_mov(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    struct msl_src src;
    struct msl_dst dst;
    uint32_t mask;

    mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    msl_src_init(&src, gen, &ins->src[0], mask);

    msl_print_assignment(gen, &dst, "%s", src.str->buffer);

    msl_src_cleanup(&src, &gen->string_buffers);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_movc(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    unsigned int component_count;
    struct msl_src src[3];
    struct msl_dst dst;
    uint32_t mask;

    mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    msl_src_init(&src[0], gen, &ins->src[0], mask);
    msl_src_init(&src[1], gen, &ins->src[1], mask);
    msl_src_init(&src[2], gen, &ins->src[2], mask);

    if ((component_count = vsir_write_mask_component_count(mask)) > 1)
        msl_print_assignment(gen, &dst, "select(%s, %s, bool%u(%s))",
                src[2].str->buffer, src[1].str->buffer, component_count, src[0].str->buffer);
    else
        msl_print_assignment(gen, &dst, "select(%s, %s, bool(%s))",
                src[2].str->buffer, src[1].str->buffer, src[0].str->buffer);

    msl_src_cleanup(&src[2], &gen->string_buffers);
    msl_src_cleanup(&src[1], &gen->string_buffers);
    msl_src_cleanup(&src[0], &gen->string_buffers);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_ret(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "return;\n");
}

static void msl_handle_instruction(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    gen->location = ins->location;

    switch (ins->opcode)
    {
        case VKD3DSIH_ADD:
            msl_binop(gen, ins, "+");
            break;
        case VKD3DSIH_AND:
            msl_binop(gen, ins, "&");
            break;
        case VKD3DSIH_NOP:
            break;
        case VKD3DSIH_DIV:
            msl_binop(gen, ins, "/");
            break;
        case VKD3DSIH_DP2:
            msl_dot(gen, ins, vkd3d_write_mask_from_component_count(2));
            break;
        case VKD3DSIH_DP3:
            msl_dot(gen, ins, vkd3d_write_mask_from_component_count(3));
            break;
        case VKD3DSIH_DP4:
            msl_dot(gen, ins, VKD3DSP_WRITEMASK_ALL);
            break;
        case VKD3DSIH_ELSE:
            msl_else(gen);
            break;
        case VKD3DSIH_ENDIF:
            msl_end_block(gen);
            break;
        case VKD3DSIH_IEQ:
            msl_relop(gen, ins, "==");
            break;
        case VKD3DSIH_EXP:
            msl_intrinsic(gen, ins, "exp2");
            break;
        case VKD3DSIH_FRC:
            msl_intrinsic(gen, ins, "fract");
            break;
        case VKD3DSIH_FTOI:
            msl_cast(gen, ins, "int");
            break;
        case VKD3DSIH_FTOU:
            msl_cast(gen, ins, "uint");
            break;
        case VKD3DSIH_GEO:
            msl_relop(gen, ins, ">=");
            break;
        case VKD3DSIH_IF:
            msl_if(gen, ins);
            break;
        case VKD3DSIH_ISHL:
            msl_binop(gen, ins, "<<");
            break;
        case VKD3DSIH_ISHR:
        case VKD3DSIH_USHR:
            msl_binop(gen, ins, ">>");
            break;
        case VKD3DSIH_LTO:
            msl_relop(gen, ins, "<");
            break;
        case VKD3DSIH_MAD:
            msl_intrinsic(gen, ins, "fma");
            break;
        case VKD3DSIH_MAX:
            msl_intrinsic(gen, ins, "max");
            break;
        case VKD3DSIH_MIN:
            msl_intrinsic(gen, ins, "min");
            break;
        case VKD3DSIH_INE:
        case VKD3DSIH_NEU:
            msl_relop(gen, ins, "!=");
            break;
        case VKD3DSIH_ITOF:
        case VKD3DSIH_UTOF:
            msl_cast(gen, ins, "float");
            break;
        case VKD3DSIH_LD:
            msl_ld(gen, ins);
            break;
        case VKD3DSIH_LOG:
            msl_intrinsic(gen, ins, "log2");
            break;
        case VKD3DSIH_MOV:
            msl_mov(gen, ins);
            break;
        case VKD3DSIH_MOVC:
            msl_movc(gen, ins);
            break;
        case VKD3DSIH_MUL:
            msl_binop(gen, ins, "*");
            break;
        case VKD3DSIH_NOT:
            msl_unary_op(gen, ins, "~");
            break;
        case VKD3DSIH_OR:
            msl_binop(gen, ins, "|");
            break;
        case VKD3DSIH_RET:
            msl_ret(gen, ins);
            break;
        case VKD3DSIH_ROUND_NE:
            msl_intrinsic(gen, ins, "rint");
            break;
        case VKD3DSIH_ROUND_NI:
            msl_intrinsic(gen, ins, "floor");
            break;
        case VKD3DSIH_ROUND_PI:
            msl_intrinsic(gen, ins, "ceil");
            break;
        case VKD3DSIH_ROUND_Z:
            msl_intrinsic(gen, ins, "trunc");
            break;
        case VKD3DSIH_RSQ:
            msl_intrinsic(gen, ins, "rsqrt");
            break;
        case VKD3DSIH_SQRT:
            msl_intrinsic(gen, ins, "sqrt");
            break;
        default:
            msl_unhandled(gen, ins);
            break;
    }
}

static void msl_generate_input_struct_declarations(struct msl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->input_signature;
    enum vkd3d_shader_type type = gen->program->shader_version.type;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i;

    vkd3d_string_buffer_printf(buffer, "struct vkd3d_%s_in\n{\n", gen->prefix);

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        if (e->sysval_semantic)
        {
            if (e->sysval_semantic == VKD3D_SHADER_SV_IS_FRONT_FACE)
            {
                if (type != VKD3D_SHADER_TYPE_PIXEL)
                    msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                            "Internal compiler error: Unhandled SV_IS_FRONT_FACE in shader type #%x.", type);

                msl_print_indent(gen->buffer, 1);
                vkd3d_string_buffer_printf(buffer, "bool is_front_face [[front_facing]];\n");
                continue;
            }
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled system value %#x.", e->sysval_semantic);
            continue;
        }

        if (e->min_precision != VKD3D_SHADER_MINIMUM_PRECISION_NONE)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled minimum precision %#x.", e->min_precision);
            continue;
        }

        if(e->register_count > 1)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled register count %u.", e->register_count);
            continue;
        }

        msl_print_indent(gen->buffer, 1);

        switch(e->component_type)
        {
            case VKD3D_SHADER_COMPONENT_FLOAT:
                vkd3d_string_buffer_printf(buffer, "float4 ");
                break;
            case VKD3D_SHADER_COMPONENT_INT:
                vkd3d_string_buffer_printf(buffer, "int4 ");
                break;
            case VKD3D_SHADER_COMPONENT_UINT:
                vkd3d_string_buffer_printf(buffer, "uint4 ");
                break;
            default:
                vkd3d_string_buffer_printf(buffer, "<unhandled component type %#x> ", e->component_type);
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled component type %#x.", e->component_type);
                break;
        }

        vkd3d_string_buffer_printf(buffer, "shader_in_%u ", i);

        switch (type)
        {
            case VKD3D_SHADER_TYPE_VERTEX:
                vkd3d_string_buffer_printf(gen->buffer, "[[attribute(%u)]]", e->target_location);
                break;
            case VKD3D_SHADER_TYPE_PIXEL:
                vkd3d_string_buffer_printf(gen->buffer, "[[user(locn%u)]]", e->target_location);
                break;
            default:
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled shader type %#x.", type);
                break;
        }

        switch (e->interpolation_mode)
        {
            /* The default interpolation attribute. */
            case VKD3DSIM_LINEAR:
            case VKD3DSIM_NONE:
                break;
            default:
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled interpolation mode %#x.", e->interpolation_mode);
                break;
        }

        vkd3d_string_buffer_printf(buffer, ";\n");
    }

    vkd3d_string_buffer_printf(buffer, "};\n\n");
}

static void msl_generate_vertex_output_element_attribute(struct msl_generator *gen, const struct signature_element *e)
{
    switch (e->sysval_semantic)
    {
        case VKD3D_SHADER_SV_POSITION:
            vkd3d_string_buffer_printf(gen->buffer, "[[position]]");
            break;
        case VKD3D_SHADER_SV_NONE:
            vkd3d_string_buffer_printf(gen->buffer, "[[user(locn%u)]]", e->target_location);
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled vertex shader system value %#x.", e->sysval_semantic);
            break;
    }
}

static void msl_generate_pixel_output_element_attribute(struct msl_generator *gen, const struct signature_element *e)
{
    switch (e->sysval_semantic)
    {
        case VKD3D_SHADER_SV_TARGET:
            vkd3d_string_buffer_printf(gen->buffer, "[[color(%u)]]", e->target_location);
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled pixel shader system value %#x.", e->sysval_semantic);
            break;
    }
}

static void msl_generate_output_struct_declarations(struct msl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->output_signature;
    enum vkd3d_shader_type type = gen->program->shader_version.type;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i;

    vkd3d_string_buffer_printf(buffer, "struct vkd3d_%s_out\n{\n", gen->prefix);

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->sysval_semantic == VKD3D_SHADER_SV_DEPTH)
        {
            gen->write_depth = true;
            msl_print_indent(gen->buffer, 1);
            vkd3d_string_buffer_printf(buffer, "float shader_out_depth [[depth(any)]];\n");
            continue;
        }

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        if (e->min_precision != VKD3D_SHADER_MINIMUM_PRECISION_NONE)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled minimum precision %#x.", e->min_precision);
            continue;
        }

        if (e->interpolation_mode != VKD3DSIM_NONE)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled interpolation mode %#x.", e->interpolation_mode);
            continue;
        }

        if(e->register_count > 1)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled register count %u.", e->register_count);
            continue;
        }

        msl_print_indent(gen->buffer, 1);

        switch(e->component_type)
        {
            case VKD3D_SHADER_COMPONENT_FLOAT:
                vkd3d_string_buffer_printf(buffer, "float4 ");
                break;
            case VKD3D_SHADER_COMPONENT_INT:
                vkd3d_string_buffer_printf(buffer, "int4 ");
                break;
            case VKD3D_SHADER_COMPONENT_UINT:
                vkd3d_string_buffer_printf(buffer, "uint4 ");
                break;
            default:
                vkd3d_string_buffer_printf(buffer, "<unhandled component type %#x> ", e->component_type);
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled component type %#x.", e->component_type);
                break;
        }

        vkd3d_string_buffer_printf(buffer, "shader_out_%u ", i);

        switch (type)
        {
            case VKD3D_SHADER_TYPE_VERTEX:
                msl_generate_vertex_output_element_attribute(gen, e);
                break;
            case VKD3D_SHADER_TYPE_PIXEL:
                msl_generate_pixel_output_element_attribute(gen, e);
                break;
            default:
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled shader type %#x.", type);
                break;
        }

        vkd3d_string_buffer_printf(buffer, ";\n");
    }

    vkd3d_string_buffer_printf(buffer, "};\n\n");
}

static void msl_generate_entrypoint_prologue(struct msl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->input_signature;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        vkd3d_string_buffer_printf(buffer, "    %s_in[%u]", gen->prefix, e->register_index);
        if (e->sysval_semantic == VKD3D_SHADER_SV_NONE)
        {
            msl_print_register_datatype(buffer, gen, vkd3d_data_type_from_component_type(e->component_type));
            msl_print_write_mask(buffer, e->mask);
            vkd3d_string_buffer_printf(buffer, " = input.shader_in_%u", i);
            msl_print_write_mask(buffer, e->mask);
        }
        else if (e->sysval_semantic == VKD3D_SHADER_SV_IS_FRONT_FACE)
        {
            vkd3d_string_buffer_printf(buffer, ".u = uint4(input.is_front_face ? 0xffffffffu : 0u, 0, 0, 0)");
        }
        else
        {
            vkd3d_string_buffer_printf(buffer, " = <unhandled sysval %#x>", e->sysval_semantic);
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled system value %#x input.", e->sysval_semantic);
        }
        vkd3d_string_buffer_printf(buffer, ";\n");
    }
}

static void msl_generate_entrypoint_epilogue(struct msl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->output_signature;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->sysval_semantic == VKD3D_SHADER_SV_DEPTH)
        {
            vkd3d_string_buffer_printf(buffer, "    output.shader_out_depth = shader_out_depth;\n");
            continue;
        }

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        switch (e->sysval_semantic)
        {
            case VKD3D_SHADER_SV_NONE:
            case VKD3D_SHADER_SV_TARGET:
            case VKD3D_SHADER_SV_POSITION:
                vkd3d_string_buffer_printf(buffer, "    output.shader_out_%u", i);
                msl_print_write_mask(buffer, e->mask);
                vkd3d_string_buffer_printf(buffer, " = %s_out[%u]", gen->prefix, e->register_index);
                msl_print_register_datatype(buffer, gen, vkd3d_data_type_from_component_type(e->component_type));
                msl_print_write_mask(buffer, e->mask);
                break;
            default:
                vkd3d_string_buffer_printf(buffer, "    <unhandled sysval %#x>", e->sysval_semantic);
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled system value %#x input.", e->sysval_semantic);
        }
        vkd3d_string_buffer_printf(buffer, ";\n");
    }
}

static void msl_generate_entrypoint(struct msl_generator *gen)
{
    enum vkd3d_shader_type type = gen->program->shader_version.type;

    switch (type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            vkd3d_string_buffer_printf(gen->buffer, "vertex ");
            break;
        case VKD3D_SHADER_TYPE_PIXEL:
            vkd3d_string_buffer_printf(gen->buffer, "fragment ");
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled shader type %#x.", type);
            return;
    }

    vkd3d_string_buffer_printf(gen->buffer, "vkd3d_%s_out shader_entry(\n", gen->prefix);

    if (gen->program->descriptors.descriptor_count)
    {
        msl_print_indent(gen->buffer, 2);
        /* TODO: Configurable argument buffer binding location. */
        vkd3d_string_buffer_printf(gen->buffer,
                "constant descriptor *descriptors [[buffer(0)]],\n");
    }

    msl_print_indent(gen->buffer, 2);
    vkd3d_string_buffer_printf(gen->buffer, "vkd3d_%s_in input [[stage_in]])\n{\n", gen->prefix);

    /* TODO: declare #maximum_register + 1 */
    vkd3d_string_buffer_printf(gen->buffer, "    vkd3d_vec4 %s_in[%u];\n", gen->prefix, 32);
    vkd3d_string_buffer_printf(gen->buffer, "    vkd3d_vec4 %s_out[%u];\n", gen->prefix, 32);
    vkd3d_string_buffer_printf(gen->buffer, "    vkd3d_%s_out output;\n", gen->prefix);

    if (gen->write_depth)
        vkd3d_string_buffer_printf(gen->buffer, "    float shader_out_depth;\n");

    msl_generate_entrypoint_prologue(gen);

    vkd3d_string_buffer_printf(gen->buffer, "    %s_main(%s_in, %s_out", gen->prefix, gen->prefix, gen->prefix);
    if (gen->write_depth)
        vkd3d_string_buffer_printf(gen->buffer, ", shader_out_depth");
    if (gen->program->descriptors.descriptor_count)
        vkd3d_string_buffer_printf(gen->buffer, ", descriptors");
    vkd3d_string_buffer_printf(gen->buffer, ");\n");

    msl_generate_entrypoint_epilogue(gen);

    vkd3d_string_buffer_printf(gen->buffer, "    return output;\n}\n");
}

static int msl_generator_generate(struct msl_generator *gen, struct vkd3d_shader_code *out)
{
    const struct vkd3d_shader_instruction_array *instructions = &gen->program->instructions;
    unsigned int i;

    MESSAGE("Generating a MSL shader. This is unsupported; you get to keep all the pieces if it breaks.\n");

    vkd3d_string_buffer_printf(gen->buffer, "/* Generated by %s. */\n\n", vkd3d_shader_get_version(NULL, NULL));
    vkd3d_string_buffer_printf(gen->buffer, "#include <metal_common>\n");
    vkd3d_string_buffer_printf(gen->buffer, "#include <metal_texture>\n\n");
    vkd3d_string_buffer_printf(gen->buffer, "using namespace metal;\n\n");

    if (gen->program->global_flags)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled global flags %#"PRIx64".", (uint64_t)gen->program->global_flags);

    vkd3d_string_buffer_printf(gen->buffer, "union vkd3d_vec4\n{\n");
    vkd3d_string_buffer_printf(gen->buffer, "    uint4 u;\n");
    vkd3d_string_buffer_printf(gen->buffer, "    int4 i;\n");
    vkd3d_string_buffer_printf(gen->buffer, "    float4 f;\n};\n\n");

    if (gen->program->descriptors.descriptor_count > 0)
    {
        vkd3d_string_buffer_printf(gen->buffer,
                "struct descriptor\n"
                "{\n"
                "    const device void *ptr;\n"
                "\n"
                "    template<typename T>\n"
                "    constant T &tex() constant\n"
                "    {\n"
                "        return reinterpret_cast<constant T &>(this->ptr);\n"
                "    }\n"
                "\n"
                "    template<typename T>\n"
                "    const device T * constant &buf() constant\n"
                "    {\n"
                "        return reinterpret_cast<const device T * constant &>(this->ptr);\n"
                "    }\n"
                "};\n"
                "\n");
    }

    msl_generate_input_struct_declarations(gen);
    msl_generate_output_struct_declarations(gen);

    vkd3d_string_buffer_printf(gen->buffer,
            "void %s_main(thread vkd3d_vec4 *v, "
            "thread vkd3d_vec4 *o",
            gen->prefix);
    if (gen->write_depth)
        vkd3d_string_buffer_printf(gen->buffer, ", thread float& o_depth");
    if (gen->program->descriptors.descriptor_count)
        vkd3d_string_buffer_printf(gen->buffer, ", constant descriptor *descriptors");
    vkd3d_string_buffer_printf(gen->buffer, ")\n{\n");

    ++gen->indent;

    if (gen->program->temp_count)
    {
        msl_print_indent(gen->buffer, gen->indent);
        vkd3d_string_buffer_printf(gen->buffer, "vkd3d_vec4 r[%u];\n\n", gen->program->temp_count);
    }

    for (i = 0; i < instructions->count; ++i)
    {
        msl_handle_instruction(gen, &instructions->elements[i]);
    }

    --gen->indent;

    vkd3d_string_buffer_printf(gen->buffer, "}\n\n");

    msl_generate_entrypoint(gen);

    if (TRACE_ON())
        vkd3d_string_buffer_trace(gen->buffer);

    if (gen->failed)
        return VKD3D_ERROR_INVALID_SHADER;

    vkd3d_shader_code_from_string_buffer(out, gen->buffer);

    return VKD3D_OK;
}

static void msl_generator_cleanup(struct msl_generator *gen)
{
    vkd3d_string_buffer_release(&gen->string_buffers, gen->buffer);
    vkd3d_string_buffer_cache_cleanup(&gen->string_buffers);
}

static int msl_generator_init(struct msl_generator *gen, struct vsir_program *program,
        const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context)
{
    enum vkd3d_shader_type type = program->shader_version.type;

    memset(gen, 0, sizeof(*gen));
    gen->program = program;
    vkd3d_string_buffer_cache_init(&gen->string_buffers);
    if (!(gen->buffer = vkd3d_string_buffer_get(&gen->string_buffers)))
    {
        vkd3d_string_buffer_cache_cleanup(&gen->string_buffers);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    gen->message_context = message_context;
    if (!(gen->prefix = msl_get_prefix(type)))
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled shader type %#x.", type);
        gen->prefix = "unknown";
    }
    gen->interface_info = vkd3d_find_struct(compile_info->next, INTERFACE_INFO);

    return VKD3D_OK;
}

int msl_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_code *out,
        struct vkd3d_shader_message_context *message_context)
{
    struct msl_generator generator;
    int ret;

    if ((ret = vsir_program_transform(program, config_flags, compile_info, message_context)) < 0)
        return ret;

    VKD3D_ASSERT(program->normalisation_level == VSIR_NORMALISED_SM6);
    VKD3D_ASSERT(program->has_descriptor_info);

    if ((ret = msl_generator_init(&generator, program, compile_info, message_context)) < 0)
        return ret;
    ret = msl_generator_generate(&generator, out);
    msl_generator_cleanup(&generator);

    return ret;
}
