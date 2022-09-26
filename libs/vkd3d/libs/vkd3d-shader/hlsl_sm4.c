/*
 * HLSL code generation for DXBC shader models 4-5
 *
 * Copyright 2019-2020 Zebediah Figura for CodeWeavers
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

#include "hlsl.h"
#include <stdio.h>
#include "d3dcommon.h"
#include "sm4.h"

static void write_sm4_block(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, const struct hlsl_block *block);

static bool type_is_integer(const struct hlsl_type *type)
{
    switch (type->base_type)
    {
        case HLSL_TYPE_BOOL:
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
            return true;

        default:
            return false;
    }
}

bool hlsl_sm4_register_from_semantic(struct hlsl_ctx *ctx, const struct hlsl_semantic *semantic,
        bool output, enum vkd3d_sm4_register_type *type, enum vkd3d_sm4_swizzle_type *swizzle_type, bool *has_idx)
{
    unsigned int i;

    static const struct
    {
        const char *semantic;
        bool output;
        enum vkd3d_shader_type shader_type;
        enum vkd3d_sm4_register_type type;
        enum vkd3d_sm4_swizzle_type swizzle_type;
        bool has_idx;
    }
    register_table[] =
    {
        {"sv_primitiveid",  false, VKD3D_SHADER_TYPE_GEOMETRY, VKD3D_SM4_RT_PRIMID, VKD3D_SM4_SWIZZLE_NONE, false},

        /* Put sv_target in this table, instead of letting it fall through to
         * default varying allocation, so that the register index matches the
         * usage index. */
        {"color",           true, VKD3D_SHADER_TYPE_PIXEL, VKD3D_SM4_RT_OUTPUT, VKD3D_SM4_SWIZZLE_VEC4, true},
        {"depth",           true, VKD3D_SHADER_TYPE_PIXEL, VKD3D_SM4_RT_DEPTHOUT, VKD3D_SM4_SWIZZLE_VEC4, false},
        {"sv_depth",        true, VKD3D_SHADER_TYPE_PIXEL, VKD3D_SM4_RT_DEPTHOUT, VKD3D_SM4_SWIZZLE_VEC4, false},
        {"sv_target",       true, VKD3D_SHADER_TYPE_PIXEL, VKD3D_SM4_RT_OUTPUT, VKD3D_SM4_SWIZZLE_VEC4, true},
    };

    for (i = 0; i < ARRAY_SIZE(register_table); ++i)
    {
        if (!ascii_strcasecmp(semantic->name, register_table[i].semantic)
                && output == register_table[i].output
                && ctx->profile->type == register_table[i].shader_type)
        {
            *type = register_table[i].type;
            if (swizzle_type)
                *swizzle_type = register_table[i].swizzle_type;
            *has_idx = register_table[i].has_idx;
            return true;
        }
    }

    return false;
}

bool hlsl_sm4_usage_from_semantic(struct hlsl_ctx *ctx, const struct hlsl_semantic *semantic,
        bool output, D3D_NAME *usage)
{
    unsigned int i;

    static const struct
    {
        const char *name;
        bool output;
        enum vkd3d_shader_type shader_type;
        D3DDECLUSAGE usage;
    }
    semantics[] =
    {
        {"position",                    false, VKD3D_SHADER_TYPE_GEOMETRY,  D3D_NAME_POSITION},
        {"sv_position",                 false, VKD3D_SHADER_TYPE_GEOMETRY,  D3D_NAME_POSITION},
        {"sv_primitiveid",              false, VKD3D_SHADER_TYPE_GEOMETRY,  D3D_NAME_PRIMITIVE_ID},

        {"position",                    true,  VKD3D_SHADER_TYPE_GEOMETRY,  D3D_NAME_POSITION},
        {"sv_position",                 true,  VKD3D_SHADER_TYPE_GEOMETRY,  D3D_NAME_POSITION},
        {"sv_primitiveid",              true,  VKD3D_SHADER_TYPE_GEOMETRY,  D3D_NAME_PRIMITIVE_ID},

        {"position",                    false, VKD3D_SHADER_TYPE_PIXEL,     D3D_NAME_POSITION},
        {"sv_position",                 false, VKD3D_SHADER_TYPE_PIXEL,     D3D_NAME_POSITION},

        {"color",                       true,  VKD3D_SHADER_TYPE_PIXEL,     D3D_NAME_TARGET},
        {"depth",                       true,  VKD3D_SHADER_TYPE_PIXEL,     D3D_NAME_DEPTH},
        {"sv_target",                   true,  VKD3D_SHADER_TYPE_PIXEL,     D3D_NAME_TARGET},
        {"sv_depth",                    true,  VKD3D_SHADER_TYPE_PIXEL,     D3D_NAME_DEPTH},

        {"sv_position",                 false, VKD3D_SHADER_TYPE_VERTEX,    D3D_NAME_UNDEFINED},
        {"sv_vertexid",                 false, VKD3D_SHADER_TYPE_VERTEX,    D3D_NAME_VERTEX_ID},

        {"position",                    true,  VKD3D_SHADER_TYPE_VERTEX,    D3D_NAME_POSITION},
        {"sv_position",                 true,  VKD3D_SHADER_TYPE_VERTEX,    D3D_NAME_POSITION},
    };

    for (i = 0; i < ARRAY_SIZE(semantics); ++i)
    {
        if (!ascii_strcasecmp(semantic->name, semantics[i].name)
                && output == semantics[i].output
                && ctx->profile->type == semantics[i].shader_type
                && !ascii_strncasecmp(semantic->name, "sv_", 3))
        {
            *usage = semantics[i].usage;
            return true;
        }
    }

    if (!ascii_strncasecmp(semantic->name, "sv_", 3))
        return false;

    *usage = D3D_NAME_UNDEFINED;
    return true;
}

static void write_sm4_signature(struct hlsl_ctx *ctx, struct dxbc_writer *dxbc, bool output)
{
    struct vkd3d_bytecode_buffer buffer = {0};
    struct vkd3d_string_buffer *string;
    const struct hlsl_ir_var *var;
    size_t count_position;
    unsigned int i;
    bool ret;

    count_position = put_u32(&buffer, 0);
    put_u32(&buffer, 8); /* unknown */

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int width = (1u << var->data_type->dimx) - 1, use_mask;
        enum vkd3d_sm4_register_type type;
        uint32_t usage_idx, reg_idx;
        D3D_NAME usage;
        bool has_idx;

        if ((output && !var->is_output_semantic) || (!output && !var->is_input_semantic))
            continue;

        ret = hlsl_sm4_usage_from_semantic(ctx, &var->semantic, output, &usage);
        assert(ret);
        usage_idx = var->semantic.index;

        if (hlsl_sm4_register_from_semantic(ctx, &var->semantic, output, &type, NULL, &has_idx))
        {
            reg_idx = has_idx ? var->semantic.index : ~0u;
        }
        else
        {
            assert(var->reg.allocated);
            type = VKD3D_SM4_RT_INPUT;
            reg_idx = var->reg.id;
        }

        use_mask = width; /* FIXME: accurately report use mask */
        if (output)
            use_mask = 0xf ^ use_mask;

        /* Special pixel shader semantics (TARGET, DEPTH, COVERAGE). */
        if (usage >= 64)
            usage = 0;

        put_u32(&buffer, 0); /* name */
        put_u32(&buffer, usage_idx);
        put_u32(&buffer, usage);
        switch (var->data_type->base_type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                put_u32(&buffer, D3D_REGISTER_COMPONENT_FLOAT32);
                break;

            case HLSL_TYPE_INT:
                put_u32(&buffer, D3D_REGISTER_COMPONENT_SINT32);
                break;

            case HLSL_TYPE_BOOL:
            case HLSL_TYPE_UINT:
                put_u32(&buffer, D3D_REGISTER_COMPONENT_UINT32);
                break;

            default:
                if ((string = hlsl_type_to_string(ctx, var->data_type)))
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Invalid data type %s for semantic variable %s.", string->buffer, var->name);
                hlsl_release_string_buffer(ctx, string);
                put_u32(&buffer, D3D_REGISTER_COMPONENT_UNKNOWN);
        }
        put_u32(&buffer, reg_idx);
        put_u32(&buffer, vkd3d_make_u16(width, use_mask));
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        const char *semantic = var->semantic.name;
        size_t string_offset;
        D3D_NAME usage;

        if ((output && !var->is_output_semantic) || (!output && !var->is_input_semantic))
            continue;

        hlsl_sm4_usage_from_semantic(ctx, &var->semantic, output, &usage);

        if (usage == D3D_NAME_TARGET && !ascii_strcasecmp(semantic, "color"))
            string_offset = put_string(&buffer, "SV_Target");
        else if (usage == D3D_NAME_DEPTH && !ascii_strcasecmp(semantic, "depth"))
            string_offset = put_string(&buffer, "SV_Depth");
        else if (usage == D3D_NAME_POSITION && !ascii_strcasecmp(semantic, "position"))
            string_offset = put_string(&buffer, "SV_Position");
        else
            string_offset = put_string(&buffer, semantic);
        set_u32(&buffer, (2 + i++ * 6) * sizeof(uint32_t), string_offset);
    }

    set_u32(&buffer, count_position, i);

    dxbc_writer_add_section(dxbc, output ? TAG_OSGN : TAG_ISGN, buffer.data, buffer.size);
}

static const struct hlsl_type *get_array_type(const struct hlsl_type *type)
{
    if (type->type == HLSL_CLASS_ARRAY)
        return get_array_type(type->e.array.type);
    return type;
}

static unsigned int get_array_size(const struct hlsl_type *type)
{
    if (type->type == HLSL_CLASS_ARRAY)
        return get_array_size(type->e.array.type) * type->e.array.elements_count;
    return 1;
}

static D3D_SHADER_VARIABLE_CLASS sm4_class(const struct hlsl_type *type)
{
    switch (type->type)
    {
        case HLSL_CLASS_ARRAY:
            return sm4_class(type->e.array.type);
        case HLSL_CLASS_MATRIX:
            assert(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);
            if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
                return D3D_SVC_MATRIX_COLUMNS;
            else
                return D3D_SVC_MATRIX_ROWS;
        case HLSL_CLASS_OBJECT:
            return D3D_SVC_OBJECT;
        case HLSL_CLASS_SCALAR:
            return D3D_SVC_SCALAR;
        case HLSL_CLASS_STRUCT:
            return D3D_SVC_STRUCT;
        case HLSL_CLASS_VECTOR:
            return D3D_SVC_VECTOR;
        default:
            ERR("Invalid class %#x.\n", type->type);
            assert(0);
            return 0;
    }
}

static D3D_SHADER_VARIABLE_TYPE sm4_base_type(const struct hlsl_type *type)
{
    switch (type->base_type)
    {
        case HLSL_TYPE_BOOL:
            return D3D_SVT_BOOL;
        case HLSL_TYPE_DOUBLE:
            return D3D_SVT_DOUBLE;
        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_HALF:
            return D3D_SVT_FLOAT;
        case HLSL_TYPE_INT:
            return D3D_SVT_INT;
        case HLSL_TYPE_PIXELSHADER:
            return D3D_SVT_PIXELSHADER;
        case HLSL_TYPE_SAMPLER:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                    return D3D_SVT_SAMPLER1D;
                case HLSL_SAMPLER_DIM_2D:
                    return D3D_SVT_SAMPLER2D;
                case HLSL_SAMPLER_DIM_3D:
                    return D3D_SVT_SAMPLER3D;
                case HLSL_SAMPLER_DIM_CUBE:
                    return D3D_SVT_SAMPLERCUBE;
                case HLSL_SAMPLER_DIM_GENERIC:
                    return D3D_SVT_SAMPLER;
                default:
                    assert(0);
            }
            break;
        case HLSL_TYPE_STRING:
            return D3D_SVT_STRING;
        case HLSL_TYPE_TEXTURE:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                    return D3D_SVT_TEXTURE1D;
                case HLSL_SAMPLER_DIM_2D:
                    return D3D_SVT_TEXTURE2D;
                case HLSL_SAMPLER_DIM_3D:
                    return D3D_SVT_TEXTURE3D;
                case HLSL_SAMPLER_DIM_CUBE:
                    return D3D_SVT_TEXTURECUBE;
                case HLSL_SAMPLER_DIM_GENERIC:
                    return D3D_SVT_TEXTURE;
                default:
                    assert(0);
            }
            break;
        case HLSL_TYPE_UINT:
            return D3D_SVT_UINT;
        case HLSL_TYPE_VERTEXSHADER:
            return D3D_SVT_VERTEXSHADER;
        case HLSL_TYPE_VOID:
            return D3D_SVT_VOID;
        default:
            assert(0);
    }
    assert(0);
    return 0;
}

static void write_sm4_type(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, struct hlsl_type *type)
{
    const struct hlsl_type *array_type = get_array_type(type);
    const char *name = array_type->name ? array_type->name : "<unnamed>";
    const struct hlsl_profile_info *profile = ctx->profile;
    unsigned int field_count = 0, array_size = 0;
    size_t fields_offset = 0, name_offset = 0;
    size_t i;

    if (type->bytecode_offset)
        return;

    if (profile->major_version >= 5)
        name_offset = put_string(buffer, name);

    if (type->type == HLSL_CLASS_ARRAY)
        array_size = get_array_size(type);

    if (array_type->type == HLSL_CLASS_STRUCT)
    {
        field_count = array_type->e.record.field_count;

        for (i = 0; i < field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            field->name_bytecode_offset = put_string(buffer, field->name);
            write_sm4_type(ctx, buffer, field->type);
        }

        fields_offset = bytecode_get_size(buffer);

        for (i = 0; i < field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            put_u32(buffer, field->name_bytecode_offset);
            put_u32(buffer, field->type->bytecode_offset);
            put_u32(buffer, field->reg_offset);
        }
    }

    type->bytecode_offset = put_u32(buffer, vkd3d_make_u32(sm4_class(type), sm4_base_type(type)));
    put_u32(buffer, vkd3d_make_u32(type->dimy, type->dimx));
    put_u32(buffer, vkd3d_make_u32(array_size, field_count));
    put_u32(buffer, fields_offset);

    if (profile->major_version >= 5)
    {
        put_u32(buffer, 0); /* FIXME: unknown */
        put_u32(buffer, 0); /* FIXME: unknown */
        put_u32(buffer, 0); /* FIXME: unknown */
        put_u32(buffer, 0); /* FIXME: unknown */
        put_u32(buffer, name_offset);
    }
}

static D3D_SHADER_INPUT_TYPE sm4_resource_type(const struct hlsl_type *type)
{
    switch (type->base_type)
    {
        case HLSL_TYPE_SAMPLER:
            return D3D_SIT_SAMPLER;
        case HLSL_TYPE_TEXTURE:
            return D3D_SIT_TEXTURE;
        default:
            assert(0);
            return 0;
    }
}

static D3D_RESOURCE_RETURN_TYPE sm4_resource_format(const struct hlsl_type *type)
{
    switch (type->e.resource_format->base_type)
    {
        case HLSL_TYPE_DOUBLE:
            return D3D_RETURN_TYPE_DOUBLE;

        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_HALF:
            return D3D_RETURN_TYPE_FLOAT;

        case HLSL_TYPE_INT:
            return D3D_RETURN_TYPE_SINT;
            break;

        case HLSL_TYPE_BOOL:
        case HLSL_TYPE_UINT:
            return D3D_RETURN_TYPE_UINT;

        default:
            assert(0);
            return 0;
    }
}

static D3D_SRV_DIMENSION sm4_rdef_resource_dimension(const struct hlsl_type *type)
{
    switch (type->sampler_dim)
    {
        case HLSL_SAMPLER_DIM_1D:
            return D3D_SRV_DIMENSION_TEXTURE1D;
        case HLSL_SAMPLER_DIM_2D:
            return D3D_SRV_DIMENSION_TEXTURE2D;
        case HLSL_SAMPLER_DIM_3D:
            return D3D_SRV_DIMENSION_TEXTURE3D;
        case HLSL_SAMPLER_DIM_CUBE:
            return D3D_SRV_DIMENSION_TEXTURECUBE;
        case HLSL_SAMPLER_DIM_1DARRAY:
            return D3D_SRV_DIMENSION_TEXTURE1DARRAY;
        case HLSL_SAMPLER_DIM_2DARRAY:
            return D3D_SRV_DIMENSION_TEXTURE2DARRAY;
        case HLSL_SAMPLER_DIM_2DMS:
            return D3D_SRV_DIMENSION_TEXTURE2DMS;
        case HLSL_SAMPLER_DIM_2DMSARRAY:
            return D3D_SRV_DIMENSION_TEXTURE2DMSARRAY;
        case HLSL_SAMPLER_DIM_CUBEARRAY:
            return D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
        default:
            assert(0);
            return D3D_SRV_DIMENSION_UNKNOWN;
    }
}

static int sm4_compare_externs(const struct hlsl_ir_var *a, const struct hlsl_ir_var *b)
{
    if (a->data_type->base_type != b->data_type->base_type)
        return a->data_type->base_type - b->data_type->base_type;
    if (a->reg.allocated && b->reg.allocated)
        return a->reg.id - b->reg.id;
    return strcmp(a->name, b->name);
}

static void sm4_sort_extern(struct list *sorted, struct hlsl_ir_var *to_sort)
{
    struct hlsl_ir_var *var;

    list_remove(&to_sort->extern_entry);

    LIST_FOR_EACH_ENTRY(var, sorted, struct hlsl_ir_var, extern_entry)
    {
        if (sm4_compare_externs(to_sort, var) < 0)
        {
            list_add_before(&var->extern_entry, &to_sort->extern_entry);
            return;
        }
    }

    list_add_tail(sorted, &to_sort->extern_entry);
}

static void sm4_sort_externs(struct hlsl_ctx *ctx)
{
    struct list sorted = LIST_INIT(sorted);
    struct hlsl_ir_var *var, *next;

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->data_type->type == HLSL_CLASS_OBJECT)
            sm4_sort_extern(&sorted, var);
    }
    list_move_tail(&ctx->extern_vars, &sorted);
}

static void write_sm4_rdef(struct hlsl_ctx *ctx, struct dxbc_writer *dxbc)
{
    size_t cbuffers_offset, resources_offset, creator_offset, string_offset;
    size_t cbuffer_position, resource_position, creator_position;
    unsigned int cbuffer_count = 0, resource_count = 0, i, j;
    const struct hlsl_profile_info *profile = ctx->profile;
    struct vkd3d_bytecode_buffer buffer = {0};
    const struct hlsl_buffer *cbuffer;
    const struct hlsl_ir_var *var;

    static const uint16_t target_types[] =
    {
        0xffff, /* PIXEL */
        0xfffe, /* VERTEX */
        0x4753, /* GEOMETRY */
        0x4853, /* HULL */
        0x4453, /* DOMAIN */
        0x4353, /* COMPUTE */
    };

    sm4_sort_externs(ctx);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->reg.allocated && var->data_type->type == HLSL_CLASS_OBJECT)
            ++resource_count;
    }

    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (cbuffer->reg.allocated)
        {
            ++cbuffer_count;
            ++resource_count;
        }
    }

    put_u32(&buffer, cbuffer_count);
    cbuffer_position = put_u32(&buffer, 0);
    put_u32(&buffer, resource_count);
    resource_position = put_u32(&buffer, 0);
    put_u32(&buffer, vkd3d_make_u32(vkd3d_make_u16(profile->minor_version, profile->major_version),
            target_types[profile->type]));
    put_u32(&buffer, 0); /* FIXME: compilation flags */
    creator_position = put_u32(&buffer, 0);

    if (profile->major_version >= 5)
    {
        put_u32(&buffer, TAG_RD11);
        put_u32(&buffer, 15 * sizeof(uint32_t)); /* size of RDEF header including this header */
        put_u32(&buffer, 6 * sizeof(uint32_t)); /* size of buffer desc */
        put_u32(&buffer, 8 * sizeof(uint32_t)); /* size of binding desc */
        put_u32(&buffer, 10 * sizeof(uint32_t)); /* size of variable desc */
        put_u32(&buffer, 9 * sizeof(uint32_t)); /* size of type desc */
        put_u32(&buffer, 3 * sizeof(uint32_t)); /* size of member desc */
        put_u32(&buffer, 0); /* unknown; possibly a null terminator */
    }

    /* Bound resources. */

    resources_offset = bytecode_get_size(&buffer);
    set_u32(&buffer, resource_position, resources_offset);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        uint32_t flags = 0;

        if (!var->reg.allocated || var->data_type->type != HLSL_CLASS_OBJECT)
            continue;

        if (var->reg_reservation.type)
            flags |= D3D_SIF_USERPACKED;

        put_u32(&buffer, 0); /* name */
        put_u32(&buffer, sm4_resource_type(var->data_type));
        if (var->data_type->base_type == HLSL_TYPE_SAMPLER)
        {
            put_u32(&buffer, 0);
            put_u32(&buffer, 0);
            put_u32(&buffer, 0);
        }
        else
        {
            put_u32(&buffer, sm4_resource_format(var->data_type));
            put_u32(&buffer, sm4_rdef_resource_dimension(var->data_type));
            put_u32(&buffer, ~0u); /* FIXME: multisample count */
            flags |= (var->data_type->e.resource_format->dimx - 1) << VKD3D_SM4_SIF_TEXTURE_COMPONENTS_SHIFT;
        }
        put_u32(&buffer, var->reg.id);
        put_u32(&buffer, 1); /* bind count */
        put_u32(&buffer, flags);
    }

    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        uint32_t flags = 0;

        if (!cbuffer->reg.allocated)
            continue;

        if (cbuffer->reservation.type)
            flags |= D3D_SIF_USERPACKED;

        put_u32(&buffer, 0); /* name */
        put_u32(&buffer, cbuffer->type == HLSL_BUFFER_CONSTANT ? D3D_SIT_CBUFFER : D3D_SIT_TBUFFER);
        put_u32(&buffer, 0); /* return type */
        put_u32(&buffer, 0); /* dimension */
        put_u32(&buffer, 0); /* multisample count */
        put_u32(&buffer, cbuffer->reg.id); /* bind point */
        put_u32(&buffer, 1); /* bind count */
        put_u32(&buffer, flags); /* flags */
    }

    i = 0;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->reg.allocated || var->data_type->type != HLSL_CLASS_OBJECT)
            continue;

        string_offset = put_string(&buffer, var->name);
        set_u32(&buffer, resources_offset + i++ * 8 * sizeof(uint32_t), string_offset);
    }

    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (!cbuffer->reg.allocated)
            continue;

        string_offset = put_string(&buffer, cbuffer->name);
        set_u32(&buffer, resources_offset + i++ * 8 * sizeof(uint32_t), string_offset);
    }

    assert(i == resource_count);

    /* Buffers. */

    cbuffers_offset = bytecode_get_size(&buffer);
    set_u32(&buffer, cbuffer_position, cbuffers_offset);
    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        unsigned int var_count = 0;

        if (!cbuffer->reg.allocated)
            continue;

        LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            if (var->is_uniform && var->buffer == cbuffer)
                ++var_count;
        }

        put_u32(&buffer, 0); /* name */
        put_u32(&buffer, var_count);
        put_u32(&buffer, 0); /* variable offset */
        put_u32(&buffer, align(cbuffer->size, 4) * sizeof(float));
        put_u32(&buffer, 0); /* FIXME: flags */
        put_u32(&buffer, cbuffer->type == HLSL_BUFFER_CONSTANT ? D3D_CT_CBUFFER : D3D_CT_TBUFFER);
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (!cbuffer->reg.allocated)
            continue;

        string_offset = put_string(&buffer, cbuffer->name);
        set_u32(&buffer, cbuffers_offset + i++ * 6 * sizeof(uint32_t), string_offset);
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        size_t vars_start = bytecode_get_size(&buffer);

        if (!cbuffer->reg.allocated)
            continue;

        set_u32(&buffer, cbuffers_offset + (i++ * 6 + 2) * sizeof(uint32_t), vars_start);

        LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            if (var->is_uniform && var->buffer == cbuffer)
            {
                uint32_t flags = 0;

                if (var->last_read)
                    flags |= D3D_SVF_USED;

                put_u32(&buffer, 0); /* name */
                put_u32(&buffer, var->buffer_offset * sizeof(float));
                put_u32(&buffer, var->data_type->reg_size * sizeof(float));
                put_u32(&buffer, flags);
                put_u32(&buffer, 0); /* type */
                put_u32(&buffer, 0); /* FIXME: default value */

                if (profile->major_version >= 5)
                {
                    put_u32(&buffer, 0); /* texture start */
                    put_u32(&buffer, 0); /* texture count */
                    put_u32(&buffer, 0); /* sampler start */
                    put_u32(&buffer, 0); /* sampler count */
                }
            }
        }

        j = 0;
        LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            if (var->is_uniform && var->buffer == cbuffer)
            {
                const unsigned int var_size = (profile->major_version >= 5 ? 10 : 6);
                size_t var_offset = vars_start + j * var_size * sizeof(uint32_t);
                size_t string_offset = put_string(&buffer, var->name);

                set_u32(&buffer, var_offset, string_offset);
                write_sm4_type(ctx, &buffer, var->data_type);
                set_u32(&buffer, var_offset + 4 * sizeof(uint32_t), var->data_type->bytecode_offset);
                ++j;
            }
        }
    }

    creator_offset = put_string(&buffer, vkd3d_shader_get_version(NULL, NULL));
    set_u32(&buffer, creator_position, creator_offset);

    dxbc_writer_add_section(dxbc, TAG_RDEF, buffer.data, buffer.size);
}

static enum vkd3d_sm4_resource_type sm4_resource_dimension(const struct hlsl_type *type)
{
    switch (type->sampler_dim)
    {
        case HLSL_SAMPLER_DIM_1D:
            return VKD3D_SM4_RESOURCE_TEXTURE_1D;
        case HLSL_SAMPLER_DIM_2D:
            return VKD3D_SM4_RESOURCE_TEXTURE_2D;
        case HLSL_SAMPLER_DIM_3D:
            return VKD3D_SM4_RESOURCE_TEXTURE_3D;
        case HLSL_SAMPLER_DIM_CUBE:
            return VKD3D_SM4_RESOURCE_TEXTURE_CUBE;
        case HLSL_SAMPLER_DIM_1DARRAY:
            return VKD3D_SM4_RESOURCE_TEXTURE_1DARRAY;
        case HLSL_SAMPLER_DIM_2DARRAY:
            return VKD3D_SM4_RESOURCE_TEXTURE_2DARRAY;
        case HLSL_SAMPLER_DIM_2DMS:
            return VKD3D_SM4_RESOURCE_TEXTURE_2DMS;
        case HLSL_SAMPLER_DIM_2DMSARRAY:
            return VKD3D_SM4_RESOURCE_TEXTURE_2DMSARRAY;
        case HLSL_SAMPLER_DIM_CUBEARRAY:
            return VKD3D_SM4_RESOURCE_TEXTURE_CUBEARRAY;
        default:
            assert(0);
            return 0;
    }
}

struct sm4_instruction_modifier
{
    enum vkd3d_sm4_instruction_modifier type;

    union
    {
        struct
        {
            int u, v, w;
        } aoffimmi;
    } u;
};

static uint32_t sm4_encode_instruction_modifier(const struct sm4_instruction_modifier *imod)
{
    uint32_t word = 0;

    word |= VKD3D_SM4_MODIFIER_MASK & imod->type;

    switch (imod->type)
    {
        case VKD3D_SM4_MODIFIER_AOFFIMMI:
            assert(-8 <= imod->u.aoffimmi.u && imod->u.aoffimmi.u <= 7);
            assert(-8 <= imod->u.aoffimmi.v && imod->u.aoffimmi.v <= 7);
            assert(-8 <= imod->u.aoffimmi.w && imod->u.aoffimmi.w <= 7);
            word |= ((uint32_t)imod->u.aoffimmi.u & 0xf) << VKD3D_SM4_AOFFIMMI_U_SHIFT;
            word |= ((uint32_t)imod->u.aoffimmi.v & 0xf) << VKD3D_SM4_AOFFIMMI_V_SHIFT;
            word |= ((uint32_t)imod->u.aoffimmi.w & 0xf) << VKD3D_SM4_AOFFIMMI_W_SHIFT;
            break;

        default:
            assert(0);
            break;
    }

    return word;
}

struct sm4_register
{
    enum vkd3d_sm4_register_type type;
    uint32_t idx[2];
    unsigned int idx_count;
    enum vkd3d_sm4_dimension dim;
    uint32_t immconst_uint[4];
    unsigned int mod;
};

struct sm4_instruction
{
    enum vkd3d_sm4_opcode opcode;

    struct sm4_instruction_modifier modifiers[1];
    unsigned int modifier_count;

    struct sm4_dst_register
    {
        struct sm4_register reg;
        unsigned int writemask;
    } dsts[2];
    unsigned int dst_count;

    struct sm4_src_register
    {
        struct sm4_register reg;
        enum vkd3d_sm4_swizzle_type swizzle_type;
        unsigned int swizzle;
    } srcs[4];
    unsigned int src_count;

    uint32_t idx[2];
    unsigned int idx_count;
};

static void sm4_register_from_deref(struct hlsl_ctx *ctx, struct sm4_register *reg,
        unsigned int *writemask, enum vkd3d_sm4_swizzle_type *swizzle_type,
        const struct hlsl_deref *deref, const struct hlsl_type *data_type)
{
    const struct hlsl_ir_var *var = deref->var;

    if (var->is_uniform)
    {
        if (data_type->type == HLSL_CLASS_OBJECT && data_type->base_type == HLSL_TYPE_TEXTURE)
        {
            reg->type = VKD3D_SM4_RT_RESOURCE;
            reg->dim = VKD3D_SM4_DIMENSION_VEC4;
            if (swizzle_type)
                *swizzle_type = VKD3D_SM4_SWIZZLE_VEC4;
            reg->idx[0] = var->reg.id;
            reg->idx_count = 1;
            *writemask = VKD3DSP_WRITEMASK_ALL;
        }
        else if (data_type->type == HLSL_CLASS_OBJECT && data_type->base_type == HLSL_TYPE_SAMPLER)
        {
            reg->type = VKD3D_SM4_RT_SAMPLER;
            reg->dim = VKD3D_SM4_DIMENSION_NONE;
            if (swizzle_type)
                *swizzle_type = VKD3D_SM4_SWIZZLE_NONE;
            reg->idx[0] = var->reg.id;
            reg->idx_count = 1;
            *writemask = VKD3DSP_WRITEMASK_ALL;
        }
        else
        {
            unsigned int offset = hlsl_offset_from_deref_safe(ctx, deref) + var->buffer_offset;

            assert(data_type->type <= HLSL_CLASS_VECTOR);
            reg->type = VKD3D_SM4_RT_CONSTBUFFER;
            reg->dim = VKD3D_SM4_DIMENSION_VEC4;
            if (swizzle_type)
                *swizzle_type = VKD3D_SM4_SWIZZLE_VEC4;
            reg->idx[0] = var->buffer->reg.id;
            reg->idx[1] = offset / 4;
            reg->idx_count = 2;
            *writemask = ((1u << data_type->dimx) - 1) << (offset & 3);
        }
    }
    else if (var->is_input_semantic)
    {
        bool has_idx;

        if (hlsl_sm4_register_from_semantic(ctx, &var->semantic, false, &reg->type, swizzle_type, &has_idx))
        {
            unsigned int offset = hlsl_offset_from_deref_safe(ctx, deref);

            if (has_idx)
            {
                reg->idx[0] = var->semantic.index + offset / 4;
                reg->idx_count = 1;
            }

            reg->dim = VKD3D_SM4_DIMENSION_VEC4;
            *writemask = ((1u << data_type->dimx) - 1) << (offset % 4);
        }
        else
        {
            struct hlsl_reg hlsl_reg = hlsl_reg_from_deref(ctx, deref);

            assert(hlsl_reg.allocated);
            reg->type = VKD3D_SM4_RT_INPUT;
            reg->dim = VKD3D_SM4_DIMENSION_VEC4;
            if (swizzle_type)
                *swizzle_type = VKD3D_SM4_SWIZZLE_VEC4;
            reg->idx[0] = hlsl_reg.id;
            reg->idx_count = 1;
            *writemask = hlsl_reg.writemask;
        }
    }
    else if (var->is_output_semantic)
    {
        bool has_idx;

        if (hlsl_sm4_register_from_semantic(ctx, &var->semantic, true, &reg->type, swizzle_type, &has_idx))
        {
            unsigned int offset = hlsl_offset_from_deref_safe(ctx, deref);

            if (has_idx)
            {
                reg->idx[0] = var->semantic.index + offset / 4;
                reg->idx_count = 1;
            }

            if (reg->type == VKD3D_SM4_RT_DEPTHOUT)
                reg->dim = VKD3D_SM4_DIMENSION_SCALAR;
            else
                reg->dim = VKD3D_SM4_DIMENSION_VEC4;
            *writemask = ((1u << data_type->dimx) - 1) << (offset % 4);
        }
        else
        {
            struct hlsl_reg hlsl_reg = hlsl_reg_from_deref(ctx, deref);

            assert(hlsl_reg.allocated);
            reg->type = VKD3D_SM4_RT_OUTPUT;
            reg->dim = VKD3D_SM4_DIMENSION_VEC4;
            reg->idx[0] = hlsl_reg.id;
            reg->idx_count = 1;
            *writemask = hlsl_reg.writemask;
        }
    }
    else
    {
        struct hlsl_reg hlsl_reg = hlsl_reg_from_deref(ctx, deref);

        assert(hlsl_reg.allocated);
        reg->type = VKD3D_SM4_RT_TEMP;
        reg->dim = VKD3D_SM4_DIMENSION_VEC4;
        if (swizzle_type)
            *swizzle_type = VKD3D_SM4_SWIZZLE_VEC4;
        reg->idx[0] = hlsl_reg.id;
        reg->idx_count = 1;
        *writemask = hlsl_reg.writemask;
    }
}

static void sm4_src_from_deref(struct hlsl_ctx *ctx, struct sm4_src_register *src,
        const struct hlsl_deref *deref, const struct hlsl_type *data_type, unsigned int map_writemask)
{
    unsigned int writemask;

    sm4_register_from_deref(ctx, &src->reg, &writemask, &src->swizzle_type, deref, data_type);
    if (src->swizzle_type == VKD3D_SM4_SWIZZLE_VEC4)
        src->swizzle = hlsl_map_swizzle(hlsl_swizzle_from_writemask(writemask), map_writemask);
}

static void sm4_register_from_node(struct sm4_register *reg, unsigned int *writemask,
        enum vkd3d_sm4_swizzle_type *swizzle_type, const struct hlsl_ir_node *instr)
{
    assert(instr->reg.allocated);
    reg->type = VKD3D_SM4_RT_TEMP;
    reg->dim = VKD3D_SM4_DIMENSION_VEC4;
    *swizzle_type = VKD3D_SM4_SWIZZLE_VEC4;
    reg->idx[0] = instr->reg.id;
    reg->idx_count = 1;
    *writemask = instr->reg.writemask;
}

static void sm4_dst_from_node(struct sm4_dst_register *dst, const struct hlsl_ir_node *instr)
{
    unsigned int swizzle_type;

    sm4_register_from_node(&dst->reg, &dst->writemask, &swizzle_type, instr);
}

static void sm4_src_from_node(struct sm4_src_register *src,
        const struct hlsl_ir_node *instr, unsigned int map_writemask)
{
    unsigned int writemask;

    sm4_register_from_node(&src->reg, &writemask, &src->swizzle_type, instr);
    if (src->swizzle_type == VKD3D_SM4_SWIZZLE_VEC4)
        src->swizzle = hlsl_map_swizzle(hlsl_swizzle_from_writemask(writemask), map_writemask);
}

static uint32_t sm4_encode_register(const struct sm4_register *reg)
{
    return (reg->type << VKD3D_SM4_REGISTER_TYPE_SHIFT)
            | (reg->idx_count << VKD3D_SM4_REGISTER_ORDER_SHIFT)
            | (reg->dim << VKD3D_SM4_DIMENSION_SHIFT);
}

static uint32_t sm4_register_order(const struct sm4_register *reg)
{
    uint32_t order = 1;
    if (reg->type == VKD3D_SM4_RT_IMMCONST)
        order += reg->dim == VKD3D_SM4_DIMENSION_VEC4 ? 4 : 1;
    order += reg->idx_count;
    if (reg->mod)
        ++order;
    return order;
}

static void write_sm4_instruction(struct vkd3d_bytecode_buffer *buffer, const struct sm4_instruction *instr)
{
    uint32_t token = instr->opcode;
    unsigned int size = 1, i, j;

    size += instr->modifier_count;
    for (i = 0; i < instr->dst_count; ++i)
        size += sm4_register_order(&instr->dsts[i].reg);
    for (i = 0; i < instr->src_count; ++i)
        size += sm4_register_order(&instr->srcs[i].reg);
    size += instr->idx_count;

    token |= (size << VKD3D_SM4_INSTRUCTION_LENGTH_SHIFT);

    if (instr->modifier_count > 0)
        token |= VKD3D_SM4_INSTRUCTION_MODIFIER;
    put_u32(buffer, token);

    for (i = 0; i < instr->modifier_count; ++i)
    {
        token = sm4_encode_instruction_modifier(&instr->modifiers[i]);
        if (instr->modifier_count > i + 1)
            token |= VKD3D_SM4_INSTRUCTION_MODIFIER;
        put_u32(buffer, token);
    }

    for (i = 0; i < instr->dst_count; ++i)
    {
        token = sm4_encode_register(&instr->dsts[i].reg);
        if (instr->dsts[i].reg.dim == VKD3D_SM4_DIMENSION_VEC4)
            token |= instr->dsts[i].writemask << VKD3D_SM4_WRITEMASK_SHIFT;
        put_u32(buffer, token);

        for (j = 0; j < instr->dsts[i].reg.idx_count; ++j)
            put_u32(buffer, instr->dsts[i].reg.idx[j]);
    }

    for (i = 0; i < instr->src_count; ++i)
    {
        token = sm4_encode_register(&instr->srcs[i].reg);
        token |= (uint32_t)instr->srcs[i].swizzle_type << VKD3D_SM4_SWIZZLE_TYPE_SHIFT;
        token |= instr->srcs[i].swizzle << VKD3D_SM4_SWIZZLE_SHIFT;
        if (instr->srcs[i].reg.mod)
            token |= VKD3D_SM4_EXTENDED_OPERAND;
        put_u32(buffer, token);

        if (instr->srcs[i].reg.mod)
            put_u32(buffer, (instr->srcs[i].reg.mod << VKD3D_SM4_REGISTER_MODIFIER_SHIFT)
                    | VKD3D_SM4_EXTENDED_OPERAND_MODIFIER);

        for (j = 0; j < instr->srcs[i].reg.idx_count; ++j)
            put_u32(buffer, instr->srcs[i].reg.idx[j]);

        if (instr->srcs[i].reg.type == VKD3D_SM4_RT_IMMCONST)
        {
            put_u32(buffer, instr->srcs[i].reg.immconst_uint[0]);
            if (instr->srcs[i].reg.dim == VKD3D_SM4_DIMENSION_VEC4)
            {
                put_u32(buffer, instr->srcs[i].reg.immconst_uint[1]);
                put_u32(buffer, instr->srcs[i].reg.immconst_uint[2]);
                put_u32(buffer, instr->srcs[i].reg.immconst_uint[3]);
            }
        }
    }

    for (j = 0; j < instr->idx_count; ++j)
        put_u32(buffer, instr->idx[j]);
}

static bool encode_texel_offset_as_aoffimmi(struct sm4_instruction *instr,
        const struct hlsl_ir_node *texel_offset)
{
    struct sm4_instruction_modifier modif;
    struct hlsl_ir_constant *offset;

    if (!texel_offset || texel_offset->type != HLSL_IR_CONSTANT)
        return false;
    offset = hlsl_ir_constant(texel_offset);

    modif.type = VKD3D_SM4_MODIFIER_AOFFIMMI;
    modif.u.aoffimmi.u = offset->value[0].i;
    modif.u.aoffimmi.v = offset->value[1].i;
    modif.u.aoffimmi.w = offset->value[2].i;
    if (modif.u.aoffimmi.u < -8 || modif.u.aoffimmi.u > 7
            || modif.u.aoffimmi.v < -8 || modif.u.aoffimmi.v > 7
            || modif.u.aoffimmi.w < -8 || modif.u.aoffimmi.w > 7)
        return false;

    instr->modifiers[instr->modifier_count++] = modif;
    return true;
}

static void write_sm4_dcl_constant_buffer(struct vkd3d_bytecode_buffer *buffer, const struct hlsl_buffer *cbuffer)
{
    const struct sm4_instruction instr =
    {
        .opcode = VKD3D_SM4_OP_DCL_CONSTANT_BUFFER,

        .srcs[0].reg.dim = VKD3D_SM4_DIMENSION_VEC4,
        .srcs[0].reg.type = VKD3D_SM4_RT_CONSTBUFFER,
        .srcs[0].reg.idx = {cbuffer->reg.id, (cbuffer->used_size + 3) / 4},
        .srcs[0].reg.idx_count = 2,
        .srcs[0].swizzle_type = VKD3D_SM4_SWIZZLE_VEC4,
        .srcs[0].swizzle = HLSL_SWIZZLE(X, Y, Z, W),
        .src_count = 1,
    };
    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_dcl_sampler(struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_var *var)
{
    const struct sm4_instruction instr =
    {
        .opcode = VKD3D_SM4_OP_DCL_SAMPLER,

        .dsts[0].reg.type = VKD3D_SM4_RT_SAMPLER,
        .dsts[0].reg.idx = {var->reg.id},
        .dsts[0].reg.idx_count = 1,
        .dst_count = 1,
    };
    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_dcl_texture(struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_var *var)
{
    const struct sm4_instruction instr =
    {
        .opcode = VKD3D_SM4_OP_DCL_RESOURCE
                | (sm4_resource_dimension(var->data_type) << VKD3D_SM4_RESOURCE_TYPE_SHIFT),

        .dsts[0].reg.type = VKD3D_SM4_RT_RESOURCE,
        .dsts[0].reg.idx = {var->reg.id},
        .dsts[0].reg.idx_count = 1,
        .dst_count = 1,

        .idx[0] = sm4_resource_format(var->data_type) * 0x1111,
        .idx_count = 1,
    };
    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_dcl_semantic(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_var *var)
{
    const struct hlsl_profile_info *profile = ctx->profile;
    const bool output = var->is_output_semantic;
    D3D_NAME usage;
    bool has_idx;

    struct sm4_instruction instr =
    {
        .dsts[0].reg.dim = VKD3D_SM4_DIMENSION_VEC4,
        .dst_count = 1,
    };

    if (hlsl_sm4_register_from_semantic(ctx, &var->semantic, output, &instr.dsts[0].reg.type, NULL, &has_idx))
    {
        if (has_idx)
        {
            instr.dsts[0].reg.idx[0] = var->semantic.index;
            instr.dsts[0].reg.idx_count = 1;
        }
        else
        {
            instr.dsts[0].reg.idx_count = 0;
        }
        instr.dsts[0].writemask = (1 << var->data_type->dimx) - 1;
    }
    else
    {
        instr.dsts[0].reg.type = output ? VKD3D_SM4_RT_OUTPUT : VKD3D_SM4_RT_INPUT;
        instr.dsts[0].reg.idx[0] = var->reg.id;
        instr.dsts[0].reg.idx_count = 1;
        instr.dsts[0].writemask = var->reg.writemask;
    }

    if (instr.dsts[0].reg.type == VKD3D_SM4_RT_DEPTHOUT)
        instr.dsts[0].reg.dim = VKD3D_SM4_DIMENSION_SCALAR;

    hlsl_sm4_usage_from_semantic(ctx, &var->semantic, output, &usage);

    if (var->is_input_semantic)
    {
        switch (usage)
        {
            case D3D_NAME_UNDEFINED:
                instr.opcode = (profile->type == VKD3D_SHADER_TYPE_PIXEL)
                        ? VKD3D_SM4_OP_DCL_INPUT_PS : VKD3D_SM4_OP_DCL_INPUT;
                break;

            case D3D_NAME_INSTANCE_ID:
            case D3D_NAME_PRIMITIVE_ID:
            case D3D_NAME_VERTEX_ID:
                instr.opcode = (profile->type == VKD3D_SHADER_TYPE_PIXEL)
                        ? VKD3D_SM4_OP_DCL_INPUT_PS_SGV : VKD3D_SM4_OP_DCL_INPUT_SGV;
                break;

            default:
                instr.opcode = (profile->type == VKD3D_SHADER_TYPE_PIXEL)
                        ? VKD3D_SM4_OP_DCL_INPUT_PS_SIV : VKD3D_SM4_OP_DCL_INPUT_SIV;
                break;
        }

        if (profile->type == VKD3D_SHADER_TYPE_PIXEL)
        {
            enum vkd3d_shader_interpolation_mode mode = VKD3DSIM_LINEAR;

            if ((var->modifiers & HLSL_STORAGE_NOINTERPOLATION) || type_is_integer(var->data_type))
                mode = VKD3DSIM_CONSTANT;

            instr.opcode |= mode << VKD3D_SM4_INTERPOLATION_MODE_SHIFT;
        }
    }
    else
    {
        if (usage == D3D_NAME_UNDEFINED || profile->type == VKD3D_SHADER_TYPE_PIXEL)
            instr.opcode = VKD3D_SM4_OP_DCL_OUTPUT;
        else
            instr.opcode = VKD3D_SM4_OP_DCL_OUTPUT_SIV;
    }

    switch (usage)
    {
        case D3D_NAME_COVERAGE:
        case D3D_NAME_DEPTH:
        case D3D_NAME_DEPTH_GREATER_EQUAL:
        case D3D_NAME_DEPTH_LESS_EQUAL:
        case D3D_NAME_TARGET:
        case D3D_NAME_UNDEFINED:
            break;

        default:
            instr.idx_count = 1;
            instr.idx[0] = usage;
            break;
    }

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_dcl_temps(struct vkd3d_bytecode_buffer *buffer, uint32_t temp_count)
{
    struct sm4_instruction instr =
    {
        .opcode = VKD3D_SM4_OP_DCL_TEMPS,

        .idx = {temp_count},
        .idx_count = 1,
    };

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_ret(struct vkd3d_bytecode_buffer *buffer)
{
    struct sm4_instruction instr =
    {
        .opcode = VKD3D_SM4_OP_RET,
    };

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_unary_op(struct vkd3d_bytecode_buffer *buffer, enum vkd3d_sm4_opcode opcode,
        const struct hlsl_ir_node *dst, const struct hlsl_ir_node *src, unsigned int src_mod)
{
    struct sm4_instruction instr;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = opcode;

    sm4_dst_from_node(&instr.dsts[0], dst);
    instr.dst_count = 1;

    sm4_src_from_node(&instr.srcs[0], src, instr.dsts[0].writemask);
    instr.srcs[0].reg.mod = src_mod;
    instr.src_count = 1;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_binary_op(struct vkd3d_bytecode_buffer *buffer, enum vkd3d_sm4_opcode opcode,
        const struct hlsl_ir_node *dst, const struct hlsl_ir_node *src1, const struct hlsl_ir_node *src2)
{
    struct sm4_instruction instr;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = opcode;

    sm4_dst_from_node(&instr.dsts[0], dst);
    instr.dst_count = 1;

    sm4_src_from_node(&instr.srcs[0], src1, instr.dsts[0].writemask);
    sm4_src_from_node(&instr.srcs[1], src2, instr.dsts[0].writemask);
    instr.src_count = 2;

    write_sm4_instruction(buffer, &instr);
}

/* dp# instructions don't map the swizzle. */
static void write_sm4_binary_op_dot(struct vkd3d_bytecode_buffer *buffer, enum vkd3d_sm4_opcode opcode,
        const struct hlsl_ir_node *dst, const struct hlsl_ir_node *src1, const struct hlsl_ir_node *src2)
{
    struct sm4_instruction instr;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = opcode;

    sm4_dst_from_node(&instr.dsts[0], dst);
    instr.dst_count = 1;

    sm4_src_from_node(&instr.srcs[0], src1, VKD3DSP_WRITEMASK_ALL);
    sm4_src_from_node(&instr.srcs[1], src2, VKD3DSP_WRITEMASK_ALL);
    instr.src_count = 2;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_binary_op_with_two_destinations(struct vkd3d_bytecode_buffer *buffer,
        enum vkd3d_sm4_opcode opcode, const struct hlsl_ir_node *dst, unsigned dst_idx,
        const struct hlsl_ir_node *src1, const struct hlsl_ir_node *src2)
{
    struct sm4_instruction instr;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = opcode;

    assert(dst_idx < ARRAY_SIZE(instr.dsts));
    sm4_dst_from_node(&instr.dsts[dst_idx], dst);
    assert(1 - dst_idx >= 0);
    instr.dsts[1 - dst_idx].reg.type = VKD3D_SM4_RT_NULL;
    instr.dsts[1 - dst_idx].reg.dim = VKD3D_SM4_DIMENSION_NONE;
    instr.dsts[1 - dst_idx].reg.idx_count = 0;
    instr.dst_count = 2;

    sm4_src_from_node(&instr.srcs[0], src1, instr.dsts[dst_idx].writemask);
    sm4_src_from_node(&instr.srcs[1], src2, instr.dsts[dst_idx].writemask);
    instr.src_count = 2;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_constant(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_constant *constant)
{
    const unsigned int dimx = constant->node.data_type->dimx;
    struct sm4_instruction instr;
    struct sm4_register *reg = &instr.srcs[0].reg;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = VKD3D_SM4_OP_MOV;

    sm4_dst_from_node(&instr.dsts[0], &constant->node);
    instr.dst_count = 1;

    instr.srcs[0].swizzle_type = VKD3D_SM4_SWIZZLE_NONE;
    reg->type = VKD3D_SM4_RT_IMMCONST;
    if (dimx == 1)
    {
        reg->dim = VKD3D_SM4_DIMENSION_SCALAR;
        reg->immconst_uint[0] = constant->value[0].u;
    }
    else
    {
        unsigned int i, j = 0;

        reg->dim = VKD3D_SM4_DIMENSION_VEC4;
        for (i = 0; i < 4; ++i)
        {
            if (instr.dsts[0].writemask & (1u << i))
                reg->immconst_uint[i] = constant->value[j++].u;
        }
    }
    instr.src_count = 1,

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_ld(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_type *resource_type, const struct hlsl_ir_node *dst,
        const struct hlsl_deref *resource, const struct hlsl_ir_node *coords)
{
    struct sm4_instruction instr;
    unsigned int dim_count;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = VKD3D_SM4_OP_LD;

    sm4_dst_from_node(&instr.dsts[0], dst);
    instr.dst_count = 1;

    sm4_src_from_node(&instr.srcs[0], coords, VKD3DSP_WRITEMASK_ALL);

    /* Mipmap level is in the last component in the IR, but needs to be in the W
     * component in the instruction. */
    dim_count = hlsl_sampler_dim_count(resource_type->sampler_dim);
    if (dim_count == 1)
        instr.srcs[0].swizzle = hlsl_combine_swizzles(instr.srcs[0].swizzle, HLSL_SWIZZLE(X, X, X, Y), 4);
    if (dim_count == 2)
        instr.srcs[0].swizzle = hlsl_combine_swizzles(instr.srcs[0].swizzle, HLSL_SWIZZLE(X, Y, X, Z), 4);

    sm4_src_from_deref(ctx, &instr.srcs[1], resource, resource_type, instr.dsts[0].writemask);

    instr.src_count = 2;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_sample(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_type *resource_type, const struct hlsl_ir_node *dst,
        const struct hlsl_deref *resource, const struct hlsl_deref *sampler,
        const struct hlsl_ir_node *coords, const struct hlsl_ir_node *texel_offset)
{
    struct sm4_instruction instr;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = VKD3D_SM4_OP_SAMPLE;

    if (texel_offset)
    {
        if (!encode_texel_offset_as_aoffimmi(&instr, texel_offset))
        {
            hlsl_error(ctx, &texel_offset->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TEXEL_OFFSET,
                    "Offset must resolve to integer literal in the range -8 to 7.");
            return;
        }
    }

    sm4_dst_from_node(&instr.dsts[0], dst);
    instr.dst_count = 1;

    sm4_src_from_node(&instr.srcs[0], coords, VKD3DSP_WRITEMASK_ALL);
    sm4_src_from_deref(ctx, &instr.srcs[1], resource, resource_type, instr.dsts[0].writemask);
    sm4_src_from_deref(ctx, &instr.srcs[2], sampler, sampler->var->data_type, VKD3DSP_WRITEMASK_ALL);
    instr.src_count = 3;

    write_sm4_instruction(buffer, &instr);
}

static bool type_is_float(const struct hlsl_type *type)
{
    return type->base_type == HLSL_TYPE_FLOAT || type->base_type == HLSL_TYPE_HALF;
}

static void write_sm4_cast_from_bool(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_expr *expr,
        const struct hlsl_ir_node *arg, uint32_t mask)
{
    struct sm4_instruction instr;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = VKD3D_SM4_OP_AND;

    sm4_dst_from_node(&instr.dsts[0], &expr->node);
    instr.dst_count = 1;

    sm4_src_from_node(&instr.srcs[0], arg, instr.dsts[0].writemask);
    instr.srcs[1].swizzle_type = VKD3D_SM4_SWIZZLE_NONE;
    instr.srcs[1].reg.type = VKD3D_SM4_RT_IMMCONST;
    instr.srcs[1].reg.dim = VKD3D_SM4_DIMENSION_SCALAR;
    instr.srcs[1].reg.immconst_uint[0] = mask;
    instr.src_count = 2;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_cast(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_expr *expr)
{
    static const union
    {
        uint32_t u;
        float f;
    } one = { .f = 1.0 };
    const struct hlsl_ir_node *arg1 = expr->operands[0].node;
    const struct hlsl_type *dst_type = expr->node.data_type;
    const struct hlsl_type *src_type = arg1->data_type;

    /* Narrowing casts were already lowered. */
    assert(src_type->dimx == dst_type->dimx);

    switch (dst_type->base_type)
    {
        case HLSL_TYPE_FLOAT:
            switch (src_type->base_type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_MOV, &expr->node, arg1, 0);
                    break;

                case HLSL_TYPE_INT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_ITOF, &expr->node, arg1, 0);
                    break;

                case HLSL_TYPE_UINT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_UTOF, &expr->node, arg1, 0);
                    break;

                case HLSL_TYPE_BOOL:
                    write_sm4_cast_from_bool(ctx, buffer, expr, arg1, one.u);
                    break;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to float.");
                    break;

                default:
                    assert(0);
            }
            break;

        case HLSL_TYPE_INT:
            switch (src_type->base_type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_FTOI, &expr->node, arg1, 0);
                    break;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_MOV, &expr->node, arg1, 0);
                    break;

                case HLSL_TYPE_BOOL:
                    write_sm4_cast_from_bool(ctx, buffer, expr, arg1, 1);
                    break;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to int.");
                    break;

                default:
                    assert(0);
            }
            break;

        case HLSL_TYPE_UINT:
            switch (src_type->base_type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_FTOU, &expr->node, arg1, 0);
                    break;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_MOV, &expr->node, arg1, 0);
                    break;

                case HLSL_TYPE_BOOL:
                    write_sm4_cast_from_bool(ctx, buffer, expr, arg1, 1);
                    break;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to uint.");
                    break;

                default:
                    assert(0);
            }
            break;

        case HLSL_TYPE_HALF:
            hlsl_fixme(ctx, &expr->node.loc, "SM4 cast to half.");
            break;

        case HLSL_TYPE_DOUBLE:
            hlsl_fixme(ctx, &expr->node.loc, "SM4 cast to double.");
            break;

        case HLSL_TYPE_BOOL:
            /* Casts to bool should have already been lowered. */
            assert(0);
            break;

        default:
            assert(0);
    }
}

static void write_sm4_expr(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_expr *expr)
{
    const struct hlsl_ir_node *arg1 = expr->operands[0].node;
    const struct hlsl_ir_node *arg2 = expr->operands[1].node;
    const struct hlsl_type *dst_type = expr->node.data_type;
    struct vkd3d_string_buffer *dst_type_string;

    assert(expr->node.reg.allocated);

    if (!(dst_type_string = hlsl_type_to_string(ctx, dst_type)))
        return;

    switch (expr->op)
    {
        case HLSL_OP1_ABS:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_MOV, &expr->node, arg1, VKD3D_SM4_REGISTER_MODIFIER_ABS);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s absolute value expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP1_BIT_NOT:
            assert(type_is_integer(dst_type));
            write_sm4_unary_op(buffer, VKD3D_SM4_OP_NOT, &expr->node, arg1, 0);
            break;

        case HLSL_OP1_CAST:
            write_sm4_cast(ctx, buffer, expr);
            break;

        case HLSL_OP1_EXP2:
            assert(type_is_float(dst_type));
            write_sm4_unary_op(buffer, VKD3D_SM4_OP_EXP, &expr->node, arg1, 0);
            break;

        case HLSL_OP1_FLOOR:
            assert(type_is_float(dst_type));
            write_sm4_unary_op(buffer, VKD3D_SM4_OP_ROUND_NI, &expr->node, arg1, 0);
            break;

        case HLSL_OP1_LOG2:
            assert(type_is_float(dst_type));
            write_sm4_unary_op(buffer, VKD3D_SM4_OP_LOG, &expr->node, arg1, 0);
            break;

        case HLSL_OP1_LOGIC_NOT:
            assert(dst_type->base_type == HLSL_TYPE_BOOL);
            write_sm4_unary_op(buffer, VKD3D_SM4_OP_NOT, &expr->node, arg1, 0);
            break;

        case HLSL_OP1_NEG:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_MOV, &expr->node, arg1, VKD3D_SM4_REGISTER_MODIFIER_NEGATE);
                    break;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    write_sm4_unary_op(buffer, VKD3D_SM4_OP_INEG, &expr->node, arg1, 0);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s negation expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP1_ROUND:
            assert(type_is_float(dst_type));
            write_sm4_unary_op(buffer, VKD3D_SM4_OP_ROUND_NE, &expr->node, arg1, 0);
            break;

        case HLSL_OP1_SAT:
            assert(type_is_float(dst_type));
            write_sm4_unary_op(buffer, VKD3D_SM4_OP_MOV
                    | (VKD3D_SM4_INSTRUCTION_FLAG_SATURATE << VKD3D_SM4_INSTRUCTION_FLAGS_SHIFT),
                    &expr->node, arg1, 0);
            break;

        case HLSL_OP2_ADD:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_ADD, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_IADD, &expr->node, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s addition expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP2_BIT_AND:
            assert(type_is_integer(dst_type));
            write_sm4_binary_op(buffer, VKD3D_SM4_OP_AND, &expr->node, arg1, arg2);
            break;

        case HLSL_OP2_BIT_OR:
            assert(type_is_integer(dst_type));
            write_sm4_binary_op(buffer, VKD3D_SM4_OP_OR, &expr->node, arg1, arg2);
            break;

        case HLSL_OP2_BIT_XOR:
            assert(type_is_integer(dst_type));
            write_sm4_binary_op(buffer, VKD3D_SM4_OP_XOR, &expr->node, arg1, arg2);
            break;

        case HLSL_OP2_DIV:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_DIV, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_UINT:
                    write_sm4_binary_op_with_two_destinations(buffer, VKD3D_SM4_OP_UDIV, &expr->node, 0, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s division expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP2_DOT:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    switch (arg1->data_type->dimx)
                    {
                        case 4:
                            write_sm4_binary_op_dot(buffer, VKD3D_SM4_OP_DP4, &expr->node, arg1, arg2);
                            break;

                        case 3:
                            write_sm4_binary_op_dot(buffer, VKD3D_SM4_OP_DP3, &expr->node, arg1, arg2);
                            break;

                        case 2:
                            write_sm4_binary_op_dot(buffer, VKD3D_SM4_OP_DP2, &expr->node, arg1, arg2);
                            break;

                        case 1:
                            assert(0);
                            break;

                        default:
                            assert(0);
                    }
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s dot expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP2_EQUAL:
        {
            const struct hlsl_type *src_type = arg1->data_type;

            assert(dst_type->base_type == HLSL_TYPE_BOOL);

            switch (src_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_EQ, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_IEQ, &expr->node, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 equality between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    break;
            }
            break;
        }

        case HLSL_OP2_GEQUAL:
        {
            const struct hlsl_type *src_type = arg1->data_type;

            assert(dst_type->base_type == HLSL_TYPE_BOOL);

            switch (src_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_GE, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_INT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_IGE, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_UINT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_UGE, &expr->node, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 greater-than-or-equal between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    break;
            }
            break;
        }

        case HLSL_OP2_LESS:
        {
            const struct hlsl_type *src_type = arg1->data_type;

            assert(dst_type->base_type == HLSL_TYPE_BOOL);

            switch (src_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_LT, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_INT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_ILT, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_UINT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_ULT, &expr->node, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 less-than between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    break;
            }
            break;
        }

        case HLSL_OP2_LOGIC_AND:
            assert(dst_type->base_type == HLSL_TYPE_BOOL);
            write_sm4_binary_op(buffer, VKD3D_SM4_OP_AND, &expr->node, arg1, arg2);
            break;

        case HLSL_OP2_LOGIC_OR:
            assert(dst_type->base_type == HLSL_TYPE_BOOL);
            write_sm4_binary_op(buffer, VKD3D_SM4_OP_OR, &expr->node, arg1, arg2);
            break;

        case HLSL_OP2_LSHIFT:
            assert(type_is_integer(dst_type));
            assert(dst_type->base_type != HLSL_TYPE_BOOL);
            write_sm4_binary_op(buffer, VKD3D_SM4_OP_ISHL, &expr->node, arg1, arg2);
            break;

        case HLSL_OP2_MAX:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_MAX, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_INT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_IMAX, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_UINT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_UMAX, &expr->node, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s maximum expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP2_MIN:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_MIN, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_INT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_IMIN, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_UINT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_UMIN, &expr->node, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s minimum expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP2_MOD:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_UINT:
                    write_sm4_binary_op_with_two_destinations(buffer, VKD3D_SM4_OP_UDIV, &expr->node, 1, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s modulus expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP2_MUL:
            switch (dst_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_MUL, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    /* Using IMUL instead of UMUL because we're taking the low
                     * bits, and the native compiler generates IMUL. */
                    write_sm4_binary_op_with_two_destinations(buffer, VKD3D_SM4_OP_IMUL, &expr->node, 1, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s multiplication expression.", dst_type_string->buffer);
            }
            break;

        case HLSL_OP2_NEQUAL:
        {
            const struct hlsl_type *src_type = arg1->data_type;

            assert(dst_type->base_type == HLSL_TYPE_BOOL);

            switch (src_type->base_type)
            {
                case HLSL_TYPE_FLOAT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_NE, &expr->node, arg1, arg2);
                    break;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    write_sm4_binary_op(buffer, VKD3D_SM4_OP_INE, &expr->node, arg1, arg2);
                    break;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 inequality between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    break;
            }
            break;
        }

        case HLSL_OP2_RSHIFT:
            assert(type_is_integer(dst_type));
            assert(dst_type->base_type != HLSL_TYPE_BOOL);
            write_sm4_binary_op(buffer, dst_type->base_type == HLSL_TYPE_INT ? VKD3D_SM4_OP_ISHR : VKD3D_SM4_OP_USHR,
                    &expr->node, arg1, arg2);
            break;

        default:
            hlsl_fixme(ctx, &expr->node.loc, "SM4 %s expression.", debug_hlsl_expr_op(expr->op));
    }

    hlsl_release_string_buffer(ctx, dst_type_string);
}

static void write_sm4_if(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_if *iff)
{
    struct sm4_instruction instr =
    {
        .opcode = VKD3D_SM4_OP_IF | VKD3D_SM4_CONDITIONAL_NZ,
        .src_count = 1,
    };

    assert(iff->condition.node->data_type->dimx == 1);

    sm4_src_from_node(&instr.srcs[0], iff->condition.node, VKD3DSP_WRITEMASK_ALL);
    write_sm4_instruction(buffer, &instr);

    write_sm4_block(ctx, buffer, &iff->then_instrs);

    if (!list_empty(&iff->else_instrs.instrs))
    {
        instr.opcode = VKD3D_SM4_OP_ELSE;
        instr.src_count = 0;
        write_sm4_instruction(buffer, &instr);

        write_sm4_block(ctx, buffer, &iff->else_instrs);
    }

    instr.opcode = VKD3D_SM4_OP_ENDIF;
    instr.src_count = 0;
    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_load(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_load *load)
{
    struct sm4_instruction instr;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = VKD3D_SM4_OP_MOV;

    sm4_dst_from_node(&instr.dsts[0], &load->node);
    instr.dst_count = 1;

    sm4_src_from_deref(ctx, &instr.srcs[0], &load->src, load->node.data_type, instr.dsts[0].writemask);
    instr.src_count = 1;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_loop(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_loop *loop)
{
    struct sm4_instruction instr =
    {
        .opcode = VKD3D_SM4_OP_LOOP,
    };

    write_sm4_instruction(buffer, &instr);

    write_sm4_block(ctx, buffer, &loop->body);

    instr.opcode = VKD3D_SM4_OP_ENDLOOP;
    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_gather(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_type *resource_type, const struct hlsl_ir_node *dst,
        const struct hlsl_deref *resource, const struct hlsl_deref *sampler,
        const struct hlsl_ir_node *coords, unsigned int swizzle, const struct hlsl_ir_node *texel_offset)
{
    struct sm4_src_register *src;
    struct sm4_instruction instr;

    memset(&instr, 0, sizeof(instr));

    instr.opcode = VKD3D_SM4_OP_GATHER4;

    sm4_dst_from_node(&instr.dsts[0], dst);
    instr.dst_count = 1;

    sm4_src_from_node(&instr.srcs[instr.src_count++], coords, VKD3DSP_WRITEMASK_ALL);

    /* FIXME: Use an aoffimmi modifier if possible. */
    if (texel_offset)
    {
        instr.opcode = VKD3D_SM5_OP_GATHER4_PO;
        sm4_src_from_node(&instr.srcs[instr.src_count++], texel_offset, VKD3DSP_WRITEMASK_ALL);
    }

    sm4_src_from_deref(ctx, &instr.srcs[instr.src_count++], resource, resource_type, instr.dsts[0].writemask);

    src = &instr.srcs[instr.src_count++];
    sm4_src_from_deref(ctx, src, sampler, sampler->var->data_type, VKD3DSP_WRITEMASK_ALL);
    src->reg.dim = VKD3D_SM4_DIMENSION_VEC4;
    src->swizzle_type = VKD3D_SM4_SWIZZLE_SCALAR;
    src->swizzle = swizzle;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_resource_load(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_resource_load *load)
{
    const struct hlsl_type *resource_type = load->resource.var->data_type;
    const struct hlsl_ir_node *texel_offset = load->texel_offset.node;
    const struct hlsl_ir_node *coords = load->coords.node;

    if (resource_type->type != HLSL_CLASS_OBJECT)
    {
        assert(resource_type->type == HLSL_CLASS_ARRAY || resource_type->type == HLSL_CLASS_STRUCT);
        hlsl_fixme(ctx, &load->node.loc, "Resource being a component of another variable.");
        return;
    }

    if (load->sampler.var)
    {
        const struct hlsl_type *sampler_type = load->sampler.var->data_type;

        if (sampler_type->type != HLSL_CLASS_OBJECT)
        {
            assert(sampler_type->type == HLSL_CLASS_ARRAY || sampler_type->type == HLSL_CLASS_STRUCT);
            hlsl_fixme(ctx, &load->node.loc, "Sampler being a component of another variable.");
            return;
        }
        assert(sampler_type->base_type == HLSL_TYPE_SAMPLER);
        assert(sampler_type->sampler_dim == HLSL_SAMPLER_DIM_GENERIC);

        if (!load->sampler.var->is_uniform)
        {
            hlsl_fixme(ctx, &load->node.loc, "Sample using non-uniform sampler variable.");
            return;
        }
    }

    if (!load->resource.var->is_uniform)
    {
        hlsl_fixme(ctx, &load->node.loc, "Load from non-uniform resource variable.");
        return;
    }

    switch (load->load_type)
    {
        case HLSL_RESOURCE_LOAD:
            write_sm4_ld(ctx, buffer, resource_type, &load->node, &load->resource, coords);
            break;

        case HLSL_RESOURCE_SAMPLE:
            if (!load->sampler.var)
                hlsl_fixme(ctx, &load->node.loc, "SM4 combined sample expression.");
            write_sm4_sample(ctx, buffer, resource_type, &load->node,
                    &load->resource, &load->sampler, coords, texel_offset);
            break;

        case HLSL_RESOURCE_GATHER_RED:
            write_sm4_gather(ctx, buffer, resource_type, &load->node, &load->resource,
                    &load->sampler, coords, HLSL_SWIZZLE(X, X, X, X), texel_offset);
            break;

        case HLSL_RESOURCE_GATHER_GREEN:
            write_sm4_gather(ctx, buffer, resource_type, &load->node, &load->resource,
                    &load->sampler, coords, HLSL_SWIZZLE(Y, Y, Y, Y), texel_offset);
            break;

        case HLSL_RESOURCE_GATHER_BLUE:
            write_sm4_gather(ctx, buffer, resource_type, &load->node, &load->resource,
                    &load->sampler, coords, HLSL_SWIZZLE(Z, Z, Z, Z), texel_offset);
            break;

        case HLSL_RESOURCE_GATHER_ALPHA:
            write_sm4_gather(ctx, buffer, resource_type, &load->node, &load->resource,
                    &load->sampler, coords, HLSL_SWIZZLE(W, W, W, W), texel_offset);
            break;

        case HLSL_RESOURCE_SAMPLE_LOD:
            hlsl_fixme(ctx, &load->node.loc, "SM4 sample-LOD expression.");
            break;
    }
}

static void write_sm4_store(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_store *store)
{
    const struct hlsl_ir_node *rhs = store->rhs.node;
    struct sm4_instruction instr;
    unsigned int writemask;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = VKD3D_SM4_OP_MOV;

    sm4_register_from_deref(ctx, &instr.dsts[0].reg, &writemask, NULL, &store->lhs, rhs->data_type);
    instr.dsts[0].writemask = hlsl_combine_writemasks(writemask, store->writemask);
    instr.dst_count = 1;

    sm4_src_from_node(&instr.srcs[0], rhs, instr.dsts[0].writemask);
    instr.src_count = 1;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_swizzle(struct hlsl_ctx *ctx,
        struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_swizzle *swizzle)
{
    struct sm4_instruction instr;
    unsigned int writemask;

    memset(&instr, 0, sizeof(instr));
    instr.opcode = VKD3D_SM4_OP_MOV;

    sm4_dst_from_node(&instr.dsts[0], &swizzle->node);
    instr.dst_count = 1;

    sm4_register_from_node(&instr.srcs[0].reg, &writemask, &instr.srcs[0].swizzle_type, swizzle->val.node);
    instr.srcs[0].swizzle = hlsl_map_swizzle(hlsl_combine_swizzles(hlsl_swizzle_from_writemask(writemask),
            swizzle->swizzle, swizzle->node.data_type->dimx), instr.dsts[0].writemask);
    instr.src_count = 1;

    write_sm4_instruction(buffer, &instr);
}

static void write_sm4_block(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_block *block)
{
    const struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->data_type)
        {
            if (instr->data_type->type == HLSL_CLASS_MATRIX)
            {
                hlsl_fixme(ctx, &instr->loc, "Matrix operations need to be lowered.");
                break;
            }
            else if (instr->data_type->type == HLSL_CLASS_OBJECT)
            {
                hlsl_fixme(ctx, &instr->loc, "Object copy.");
                break;
            }

            assert(instr->data_type->type == HLSL_CLASS_SCALAR || instr->data_type->type == HLSL_CLASS_VECTOR);
        }

        switch (instr->type)
        {
            case HLSL_IR_CONSTANT:
                write_sm4_constant(ctx, buffer, hlsl_ir_constant(instr));
                break;

            case HLSL_IR_EXPR:
                write_sm4_expr(ctx, buffer, hlsl_ir_expr(instr));
                break;

            case HLSL_IR_IF:
                write_sm4_if(ctx, buffer, hlsl_ir_if(instr));
                break;

            case HLSL_IR_LOAD:
                write_sm4_load(ctx, buffer, hlsl_ir_load(instr));
                break;

            case HLSL_IR_RESOURCE_LOAD:
                write_sm4_resource_load(ctx, buffer, hlsl_ir_resource_load(instr));
                break;

            case HLSL_IR_LOOP:
                write_sm4_loop(ctx, buffer, hlsl_ir_loop(instr));
                break;

            case HLSL_IR_STORE:
                write_sm4_store(ctx, buffer, hlsl_ir_store(instr));
                break;

            case HLSL_IR_SWIZZLE:
                write_sm4_swizzle(ctx, buffer, hlsl_ir_swizzle(instr));
                break;

            default:
                FIXME("Unhandled instruction type %s.\n", hlsl_node_type_to_string(instr->type));
        }
    }
}

static void write_sm4_shdr(struct hlsl_ctx *ctx,
        const struct hlsl_ir_function_decl *entry_func, struct dxbc_writer *dxbc)
{
    const struct hlsl_profile_info *profile = ctx->profile;
    struct vkd3d_bytecode_buffer buffer = {0};
    const struct hlsl_buffer *cbuffer;
    const struct hlsl_ir_var *var;
    size_t token_count_position;

    static const uint16_t shader_types[VKD3D_SHADER_TYPE_COUNT] =
    {
        VKD3D_SM4_PS,
        VKD3D_SM4_VS,
        VKD3D_SM4_GS,
        VKD3D_SM5_HS,
        VKD3D_SM5_DS,
        VKD3D_SM5_CS,
        0, /* EFFECT */
        0, /* TEXTURE */
        VKD3D_SM4_LIB,
    };

    put_u32(&buffer, vkd3d_make_u32((profile->major_version << 4) | profile->minor_version, shader_types[profile->type]));
    token_count_position = put_u32(&buffer, 0);

    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (cbuffer->reg.allocated)
            write_sm4_dcl_constant_buffer(&buffer, cbuffer);
    }

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, const struct hlsl_ir_var, extern_entry)
    {
        if (!var->reg.allocated || var->data_type->type != HLSL_CLASS_OBJECT)
            continue;

        if (var->data_type->base_type == HLSL_TYPE_SAMPLER)
            write_sm4_dcl_sampler(&buffer, var);
        else if (var->data_type->base_type == HLSL_TYPE_TEXTURE)
            write_sm4_dcl_texture(&buffer, var);
    }

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if ((var->is_input_semantic && var->last_read) || (var->is_output_semantic && var->first_write))
            write_sm4_dcl_semantic(ctx, &buffer, var);
    }

    if (ctx->temp_count)
        write_sm4_dcl_temps(&buffer, ctx->temp_count);

    write_sm4_block(ctx, &buffer, &entry_func->body);

    write_sm4_ret(&buffer);

    set_u32(&buffer, token_count_position, bytecode_get_size(&buffer) / sizeof(uint32_t));

    dxbc_writer_add_section(dxbc, TAG_SHDR, buffer.data, buffer.size);
}

int hlsl_sm4_write(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func, struct vkd3d_shader_code *out)
{
    struct dxbc_writer dxbc;
    size_t i;
    int ret;

    dxbc_writer_init(&dxbc);

    write_sm4_signature(ctx, &dxbc, false);
    write_sm4_signature(ctx, &dxbc, true);
    write_sm4_rdef(ctx, &dxbc);
    write_sm4_shdr(ctx, entry_func, &dxbc);

    if (!(ret = ctx->result))
        ret = dxbc_writer_write(&dxbc, out);
    for (i = 0; i < dxbc.section_count; ++i)
        vkd3d_free((void *)dxbc.sections[i].data);
    return ret;
}
