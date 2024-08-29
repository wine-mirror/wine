/*
 * FX (Direct3D 9/10/11 effect) support
 *
 * Copyright 2023 Nikolay Sivov for CodeWeavers
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

static inline size_t put_u32_unaligned(struct vkd3d_bytecode_buffer *buffer, uint32_t value)
{
    return bytecode_put_bytes_unaligned(buffer, &value, sizeof(value));
}

struct string_entry
{
    struct rb_entry entry;
    /* String points to original data, should not be freed. */
    const char *string;
    uint32_t offset;
};

struct type_entry
{
    struct list entry;
    const char *name;
    uint32_t elements_count;
    uint32_t offset;
};

static int string_storage_compare(const void *key, const struct rb_entry *entry)
{
    struct string_entry *string_entry = RB_ENTRY_VALUE(entry, struct string_entry, entry);
    const char *string = key;

    return strcmp(string, string_entry->string);
}

static void string_storage_destroy(struct rb_entry *entry, void *context)
{
    struct string_entry *string_entry = RB_ENTRY_VALUE(entry, struct string_entry, entry);

    vkd3d_free(string_entry);
}

struct function_component
{
    const char *name;
    bool lhs_has_index;
    unsigned int lhs_index;
};

static const struct state_block_function_info
{
    const char *name;
    unsigned int min_args, max_args;
    const struct function_component components[3];
    unsigned int min_profile;
}
function_info[] =
{
    {"SetBlendState",        3, 3, { { "AB_BlendFactor" }, { "AB_SampleMask" }, { "BlendState" } }, 4 },
    {"SetDepthStencilState", 2, 2, { { "DS_StencilRef" }, { "DepthStencilState" } }, 4 },
    {"SetRasterizerState",   1, 1, { { "RasterizerState" } }, 4 },
    {"SetVertexShader",      1, 1, { { "VertexShader" } }, 4 },
    {"SetDomainShader",      1, 1, { { "DomainShader" } }, 5 },
    {"SetHullShader",        1, 1, { { "HullShader" } }, 5 },
    {"SetGeometryShader",    1, 1, { { "GeometryShader" } }, 4 },
    {"SetPixelShader",       1, 1, { { "PixelShader" } }, 4 },
    {"SetComputeShader",     1, 1, { { "ComputeShader" } }, 4 },
    {"OMSetRenderTargets",   2, 9, { {0} }, 4 },
};

static const struct state_block_function_info *get_state_block_function_info(const char *name)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(function_info); ++i)
    {
        if (!strcmp(name, function_info[i].name))
            return &function_info[i];
    }
    return NULL;
}

static void add_function_component(struct function_component **components, const char *name,
        bool lhs_has_index, unsigned int lhs_index)
{
    struct function_component *comp = *components;

    comp->name = name;
    comp->lhs_has_index = lhs_has_index;
    comp->lhs_index = lhs_index;

    *components = *components + 1;
}

static void get_state_block_function_components(const struct state_block_function_info *info,
        struct function_component *components, unsigned int comp_count)
{
    unsigned int i;

    VKD3D_ASSERT(comp_count <= info->max_args);

    if (info->min_args == info->max_args)
    {
        const struct function_component *c = info->components;
        for (i = 0; i < comp_count; ++i, ++c)
            add_function_component(&components, c->name, c->lhs_has_index, c->lhs_index);
        return;
    }

    if (!strcmp(info->name, "OMSetRenderTargets"))
    {
        for (i = 0; i < comp_count - 2; ++i)
            add_function_component(&components, "RenderTargetView", true, i + 1);
        add_function_component(&components, "DepthStencilView", false, 0);
        add_function_component(&components, "RenderTargetView", true, 0);
    }
}

bool hlsl_validate_state_block_entry(struct hlsl_ctx *ctx, struct hlsl_state_block_entry *entry,
        const struct vkd3d_shader_location *loc)
{
    if (entry->is_function_call)
    {
        const struct state_block_function_info *info = get_state_block_function_info(entry->name);

        if (!info)
        {
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_STATE_BLOCK_ENTRY,
                    "Invalid state block function '%s'.", entry->name);
            return false;
        }
        if (entry->args_count < info->min_args || entry->args_count > info->max_args)
        {
            if (info->min_args == info->max_args)
            {
                hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_STATE_BLOCK_ENTRY,
                        "Invalid argument count for state block function '%s' (expected %u).",
                        entry->name, info->min_args);
            }
            else
            {
                hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_STATE_BLOCK_ENTRY,
                        "Invalid argument count for state block function '%s' (expected from %u to %u).",
                        entry->name, info->min_args, info->max_args);
            }
            return false;
        }
    }

    return true;
}

struct fx_write_context;

struct fx_write_context_ops
{
    uint32_t (*write_string)(const char *string, struct fx_write_context *fx);
    void (*write_technique)(struct hlsl_ir_var *var, struct fx_write_context *fx);
    void (*write_pass)(struct hlsl_ir_var *var, struct fx_write_context *fx);
    void (*write_annotation)(struct hlsl_ir_var *var, struct fx_write_context *fx);
    bool are_child_effects_supported;
};

struct fx_write_context
{
    struct hlsl_ctx *ctx;

    struct vkd3d_bytecode_buffer unstructured;
    struct vkd3d_bytecode_buffer structured;

    struct rb_tree strings;
    struct list types;

    unsigned int min_technique_version;
    unsigned int max_technique_version;

    uint32_t technique_count;
    uint32_t group_count;
    uint32_t buffer_count;
    uint32_t shared_buffer_count;
    uint32_t numeric_variable_count;
    uint32_t shared_numeric_variable_count;
    uint32_t object_variable_count;
    uint32_t shared_object_count;
    uint32_t shader_count;
    uint32_t parameter_count;
    uint32_t dsv_count;
    uint32_t rtv_count;
    uint32_t texture_count;
    uint32_t uav_count;
    uint32_t sampler_state_count;
    uint32_t depth_stencil_state_count;
    uint32_t rasterizer_state_count;
    uint32_t blend_state_count;
    uint32_t string_count;
    int status;

    bool child_effect;
    bool include_empty_buffers;

    const struct fx_write_context_ops *ops;
};

static void set_status(struct fx_write_context *fx, int status)
{
    if (fx->status < 0)
        return;
    if (status < 0)
        fx->status = status;
}

static bool has_annotations(const struct hlsl_ir_var *var)
{
    return var->annotations && !list_empty(&var->annotations->vars);
}

static uint32_t write_string(const char *string, struct fx_write_context *fx)
{
    return fx->ops->write_string(string, fx);
}

static void write_pass(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    fx->ops->write_pass(var, fx);
}

static uint32_t write_annotations(struct hlsl_scope *scope, struct fx_write_context *fx)
{
    struct hlsl_ctx *ctx = fx->ctx;
    struct hlsl_ir_var *v;
    uint32_t count = 0;

    if (!scope)
        return 0;

    LIST_FOR_EACH_ENTRY(v, &scope->vars, struct hlsl_ir_var, scope_entry)
    {
        if (!v->default_values)
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                    "Annotation variable is missing default value.");

        fx->ops->write_annotation(v, fx);
        ++count;
    }

    return count;
}

static void write_fx_4_annotations(struct hlsl_scope *scope, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t count_offset, count;

    count_offset = put_u32(buffer, 0);
    count = write_annotations(scope, fx);
    set_u32(buffer, count_offset, count);
}

static uint32_t write_fx_4_type(const struct hlsl_type *type, struct fx_write_context *fx);
static const char * get_fx_4_type_name(const struct hlsl_type *type);
static void write_fx_4_annotation(struct hlsl_ir_var *var, struct fx_write_context *fx);
static void write_fx_4_state_block(struct hlsl_ir_var *var, unsigned int block_index,
        uint32_t count_offset, struct fx_write_context *fx);

static uint32_t write_type(const struct hlsl_type *type, struct fx_write_context *fx)
{
    const struct hlsl_type *element_type;
    struct type_entry *type_entry;
    unsigned int elements_count;
    const char *name;

    VKD3D_ASSERT(fx->ctx->profile->major_version >= 4);

    if (type->class == HLSL_CLASS_ARRAY)
    {
        elements_count = hlsl_get_multiarray_size(type);
        element_type = hlsl_get_multiarray_element_type(type);
    }
    else
    {
        elements_count = 0;
        element_type = type;
    }

    name = get_fx_4_type_name(element_type);

    LIST_FOR_EACH_ENTRY(type_entry, &fx->types, struct type_entry, entry)
    {
        if (strcmp(type_entry->name, name))
            continue;

        if (type_entry->elements_count != elements_count)
            continue;

        return type_entry->offset;
    }

    if (!(type_entry = hlsl_alloc(fx->ctx, sizeof(*type_entry))))
        return 0;

    type_entry->offset = write_fx_4_type(type, fx);
    type_entry->name = name;
    type_entry->elements_count = elements_count;

    list_add_tail(&fx->types, &type_entry->entry);

    return type_entry->offset;
}

static void fx_write_context_init(struct hlsl_ctx *ctx, const struct fx_write_context_ops *ops,
        struct fx_write_context *fx)
{
    unsigned int version = ctx->profile->major_version;
    struct hlsl_ir_var *var;

    memset(fx, 0, sizeof(*fx));

    fx->ctx = ctx;
    fx->ops = ops;
    if (version == 2)
    {
        fx->min_technique_version = 9;
        fx->max_technique_version = 9;
    }
    else if (version == 4)
    {
        fx->min_technique_version = 10;
        fx->max_technique_version = 10;
    }
    else if (version == 5)
    {
        fx->min_technique_version = 10;
        fx->max_technique_version = 11;
    }

    rb_init(&fx->strings, string_storage_compare);
    list_init(&fx->types);

    fx->child_effect = fx->ops->are_child_effects_supported && ctx->child_effect;
    fx->include_empty_buffers = version == 4 && ctx->include_empty_buffers;

    LIST_FOR_EACH_ENTRY(var, &ctx->globals->vars, struct hlsl_ir_var, scope_entry)
    {
        if (var->storage_modifiers & HLSL_STORAGE_UNIFORM)
        {
            list_add_tail(&ctx->extern_vars, &var->extern_entry);
            var->is_uniform = 1;
        }
    }

    hlsl_calculate_buffer_offsets(fx->ctx);
}

static int fx_write_context_cleanup(struct fx_write_context *fx)
{
    struct type_entry *type, *next_type;

    rb_destroy(&fx->strings, string_storage_destroy, NULL);

    LIST_FOR_EACH_ENTRY_SAFE(type, next_type, &fx->types, struct type_entry, entry)
    {
        list_remove(&type->entry);
        vkd3d_free(type);
    }

    return fx->ctx->result;
}

static bool technique_matches_version(const struct hlsl_ir_var *var, const struct fx_write_context *fx)
{
    const struct hlsl_type *type = var->data_type;

    if (type->class != HLSL_CLASS_TECHNIQUE)
        return false;

    return type->e.version >= fx->min_technique_version && type->e.version <= fx->max_technique_version;
}

static uint32_t write_fx_4_string(const char *string, struct fx_write_context *fx)
{
    struct string_entry *string_entry;
    struct rb_entry *entry;

    /* NULLs are emitted as empty strings using the same 4 bytes at the start of the section. */
    if (!string)
        return 0;

    if ((entry = rb_get(&fx->strings, string)))
    {
        string_entry = RB_ENTRY_VALUE(entry, struct string_entry, entry);
        return string_entry->offset;
    }

    if (!(string_entry = hlsl_alloc(fx->ctx, sizeof(*string_entry))))
        return 0;

    string_entry->offset = bytecode_put_bytes_unaligned(&fx->unstructured, string, strlen(string) + 1);
    string_entry->string = string;

    rb_put(&fx->strings, string, &string_entry->entry);

    return string_entry->offset;
}

static void write_fx_4_pass(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t name_offset, count_offset;

    name_offset = write_string(var->name, fx);
    put_u32(buffer, name_offset);
    count_offset = put_u32(buffer, 0);

    write_fx_4_annotations(var->annotations, fx);
    write_fx_4_state_block(var, 0, count_offset, fx);
}

static void write_fx_2_pass(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t name_offset;

    name_offset = write_string(var->name, fx);
    put_u32(buffer, name_offset);
    put_u32(buffer, 0); /* Annotation count. */
    put_u32(buffer, 0); /* Assignment count. */

    /* TODO: annotations */
    /* TODO: assignments */

    if (var->state_block_count && var->state_blocks[0]->count)
        hlsl_fixme(fx->ctx, &var->loc, "Write pass assignments.");

    /* For some reason every pass adds to the total shader object count. */
    fx->shader_count++;
}

static uint32_t get_fx_4_type_size(const struct hlsl_type *type)
{
    uint32_t elements_count;

    elements_count = hlsl_get_multiarray_size(type);
    type = hlsl_get_multiarray_element_type(type);

    return type->reg_size[HLSL_REGSET_NUMERIC] * sizeof(float) * elements_count;
}

static const uint32_t fx_4_numeric_base_type[] =
{
    [HLSL_TYPE_FLOAT] = 1,
    [HLSL_TYPE_INT  ] = 2,
    [HLSL_TYPE_UINT ] = 3,
    [HLSL_TYPE_BOOL ] = 4,
};

static uint32_t get_fx_4_numeric_type_description(const struct hlsl_type *type, struct fx_write_context *fx)
{
    static const unsigned int NUMERIC_BASE_TYPE_SHIFT = 3;
    static const unsigned int NUMERIC_ROWS_SHIFT = 8;
    static const unsigned int NUMERIC_COLUMNS_SHIFT = 11;
    static const unsigned int NUMERIC_COLUMN_MAJOR_MASK = 0x4000;
    static const uint32_t numeric_type_class[] =
    {
        [HLSL_CLASS_SCALAR] = 1,
        [HLSL_CLASS_VECTOR] = 2,
        [HLSL_CLASS_MATRIX] = 3,
    };
    struct hlsl_ctx *ctx = fx->ctx;
    uint32_t value = 0;

    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            value |= numeric_type_class[type->class];
            break;
        default:
            hlsl_fixme(ctx, &ctx->location, "Not implemented for type class %u.", type->class);
            return 0;
    }

    switch (type->e.numeric.type)
    {
        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
        case HLSL_TYPE_BOOL:
            value |= (fx_4_numeric_base_type[type->e.numeric.type] << NUMERIC_BASE_TYPE_SHIFT);
            break;
        default:
            hlsl_fixme(ctx, &ctx->location, "Not implemented for base type %u.", type->e.numeric.type);
            return 0;
    }

    value |= (type->dimy & 0x7) << NUMERIC_ROWS_SHIFT;
    value |= (type->dimx & 0x7) << NUMERIC_COLUMNS_SHIFT;
    if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
        value |= NUMERIC_COLUMN_MAJOR_MASK;

    return value;
}

static const char * get_fx_4_type_name(const struct hlsl_type *type)
{
    static const char * const texture_type_names[] =
    {
        [HLSL_SAMPLER_DIM_GENERIC]   = "texture",
        [HLSL_SAMPLER_DIM_1D]        = "Texture1D",
        [HLSL_SAMPLER_DIM_1DARRAY]   = "Texture1DArray",
        [HLSL_SAMPLER_DIM_2D]        = "Texture2D",
        [HLSL_SAMPLER_DIM_2DARRAY]   = "Texture2DArray",
        [HLSL_SAMPLER_DIM_2DMS]      = "Texture2DMS",
        [HLSL_SAMPLER_DIM_2DMSARRAY] = "Texture2DMSArray",
        [HLSL_SAMPLER_DIM_3D]        = "Texture3D",
        [HLSL_SAMPLER_DIM_CUBE]      = "TextureCube",
        [HLSL_SAMPLER_DIM_CUBEARRAY] = "TextureCubeArray",
    };
    static const char * const uav_type_names[] =
    {
        [HLSL_SAMPLER_DIM_1D]                = "RWTexture1D",
        [HLSL_SAMPLER_DIM_1DARRAY]           = "RWTexture1DArray",
        [HLSL_SAMPLER_DIM_2D]                = "RWTexture2D",
        [HLSL_SAMPLER_DIM_2DARRAY]           = "RWTexture2DArray",
        [HLSL_SAMPLER_DIM_3D]                = "RWTexture3D",
        [HLSL_SAMPLER_DIM_BUFFER]            = "RWBuffer",
        [HLSL_SAMPLER_DIM_STRUCTURED_BUFFER] = "RWStructuredBuffer",
    };

    switch (type->class)
    {
        case HLSL_CLASS_SAMPLER:
            return "SamplerState";

        case HLSL_CLASS_TEXTURE:
            return texture_type_names[type->sampler_dim];

        case HLSL_CLASS_UAV:
            return uav_type_names[type->sampler_dim];

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
            return "DepthStencilState";

        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
            return "DepthStencilView";

        case HLSL_CLASS_RENDER_TARGET_VIEW:
            return "RenderTargetView";

        case HLSL_CLASS_VERTEX_SHADER:
            return "VertexShader";

        case HLSL_CLASS_PIXEL_SHADER:
            return "PixelShader";

        case HLSL_CLASS_STRING:
            return "String";

        default:
            return type->name;
    }
}

static uint32_t write_fx_4_type(const struct hlsl_type *type, struct fx_write_context *fx)
{
    struct field_offsets
    {
        uint32_t name;
        uint32_t semantic;
        uint32_t offset;
        uint32_t type;
    };
    uint32_t name_offset, offset, total_size, packed_size, stride, numeric_desc;
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    struct field_offsets *field_offsets = NULL;
    struct hlsl_ctx *ctx = fx->ctx;
    uint32_t elements_count = 0;
    const char *name;
    size_t i;

    /* Resolve arrays to element type and number of elements. */
    if (type->class == HLSL_CLASS_ARRAY)
    {
        elements_count = hlsl_get_multiarray_size(type);
        type = hlsl_get_multiarray_element_type(type);
    }

    name = get_fx_4_type_name(type);

    name_offset = write_string(name, fx);
    if (type->class == HLSL_CLASS_STRUCT)
    {
        if (!(field_offsets = hlsl_calloc(ctx, type->e.record.field_count, sizeof(*field_offsets))))
            return 0;

        for (i = 0; i < type->e.record.field_count; ++i)
        {
            const struct hlsl_struct_field *field = &type->e.record.fields[i];

            field_offsets[i].name = write_string(field->name, fx);
            field_offsets[i].semantic = write_string(field->semantic.raw_name, fx);
            field_offsets[i].offset = field->reg_offset[HLSL_REGSET_NUMERIC];
            field_offsets[i].type = write_type(field->type, fx);
        }
    }

    offset = put_u32_unaligned(buffer, name_offset);

    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            put_u32_unaligned(buffer, 1);
            break;

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_STRING:
            put_u32_unaligned(buffer, 2);
            break;

        case HLSL_CLASS_STRUCT:
            put_u32_unaligned(buffer, 3);
            break;

        case HLSL_CLASS_ARRAY:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_NULL:
            vkd3d_unreachable();

        case HLSL_CLASS_VOID:
            FIXME("Writing type class %u is not implemented.\n", type->class);
            set_status(fx, VKD3D_ERROR_NOT_IMPLEMENTED);
            return 0;
    }

    /* Structures can only contain numeric fields, this is validated during variable declaration. */
    total_size = stride = type->reg_size[HLSL_REGSET_NUMERIC] * sizeof(float);
    packed_size = 0;
    if (type->class == HLSL_CLASS_STRUCT || hlsl_is_numeric_type(type))
        packed_size = hlsl_type_component_count(type) * sizeof(float);
    if (elements_count)
    {
        total_size *= elements_count;
        packed_size *= elements_count;
    }
    stride = align(stride, 4 * sizeof(float));

    put_u32_unaligned(buffer, elements_count);
    put_u32_unaligned(buffer, total_size);
    put_u32_unaligned(buffer, stride);
    put_u32_unaligned(buffer, packed_size);

    if (type->class == HLSL_CLASS_STRUCT)
    {
        put_u32_unaligned(buffer, type->e.record.field_count);
        for (i = 0; i < type->e.record.field_count; ++i)
        {
            const struct field_offsets *field = &field_offsets[i];

            put_u32_unaligned(buffer, field->name);
            put_u32_unaligned(buffer, field->semantic);
            put_u32_unaligned(buffer, field->offset);
            put_u32_unaligned(buffer, field->type);
        }

        if (ctx->profile->major_version == 5)
        {
            put_u32_unaligned(buffer, 0); /* Base class type */
            put_u32_unaligned(buffer, 0); /* Interface count */
        }
    }
    else if (type->class == HLSL_CLASS_TEXTURE)
    {
        static const uint32_t texture_type[] =
        {
            [HLSL_SAMPLER_DIM_GENERIC]   = 9,
            [HLSL_SAMPLER_DIM_1D]        = 10,
            [HLSL_SAMPLER_DIM_1DARRAY]   = 11,
            [HLSL_SAMPLER_DIM_2D]        = 12,
            [HLSL_SAMPLER_DIM_2DARRAY]   = 13,
            [HLSL_SAMPLER_DIM_2DMS]      = 14,
            [HLSL_SAMPLER_DIM_2DMSARRAY] = 15,
            [HLSL_SAMPLER_DIM_3D]        = 16,
            [HLSL_SAMPLER_DIM_CUBE]      = 17,
            [HLSL_SAMPLER_DIM_CUBEARRAY] = 23,
        };

        put_u32_unaligned(buffer, texture_type[type->sampler_dim]);
    }
    else if (type->class == HLSL_CLASS_SAMPLER)
    {
        put_u32_unaligned(buffer, 21);
    }
    else if (type->class == HLSL_CLASS_UAV)
    {
        static const uint32_t uav_type[] =
        {
            [HLSL_SAMPLER_DIM_1D]                = 31,
            [HLSL_SAMPLER_DIM_1DARRAY]           = 32,
            [HLSL_SAMPLER_DIM_2D]                = 33,
            [HLSL_SAMPLER_DIM_2DARRAY]           = 34,
            [HLSL_SAMPLER_DIM_3D]                = 35,
            [HLSL_SAMPLER_DIM_BUFFER]            = 36,
            [HLSL_SAMPLER_DIM_STRUCTURED_BUFFER] = 40,
        };

        put_u32_unaligned(buffer, uav_type[type->sampler_dim]);
    }
    else if (type->class == HLSL_CLASS_DEPTH_STENCIL_VIEW)
    {
        put_u32_unaligned(buffer, 20);
    }
    else if (type->class == HLSL_CLASS_RENDER_TARGET_VIEW)
    {
        put_u32_unaligned(buffer, 19);
    }
    else if (type->class == HLSL_CLASS_PIXEL_SHADER)
    {
        put_u32_unaligned(buffer, 5);
    }
    else if (type->class == HLSL_CLASS_VERTEX_SHADER)
    {
        put_u32_unaligned(buffer, 6);
    }
    else if (type->class == HLSL_CLASS_RASTERIZER_STATE)
    {
        put_u32_unaligned(buffer, 4);
    }
    else if (type->class == HLSL_CLASS_DEPTH_STENCIL_STATE)
    {
        put_u32_unaligned(buffer, 3);
    }
    else if (type->class == HLSL_CLASS_BLEND_STATE)
    {
        put_u32_unaligned(buffer, 2);
    }
    else if (type->class == HLSL_CLASS_STRING)
    {
        put_u32_unaligned(buffer, 1);
    }
    else if (hlsl_is_numeric_type(type))
    {
        numeric_desc = get_fx_4_numeric_type_description(type, fx);
        put_u32_unaligned(buffer, numeric_desc);
    }
    else if (type->class == HLSL_CLASS_COMPUTE_SHADER)
    {
        put_u32_unaligned(buffer, 28);
    }
    else if (type->class == HLSL_CLASS_HULL_SHADER)
    {
        put_u32_unaligned(buffer, 29);
    }
    else if (type->class == HLSL_CLASS_DOMAIN_SHADER)
    {
        put_u32_unaligned(buffer, 30);
    }
    else
    {
        FIXME("Type %u is not supported.\n", type->class);
        set_status(fx, VKD3D_ERROR_NOT_IMPLEMENTED);
    }

    vkd3d_free(field_offsets);
    return offset;
}

static void write_fx_4_technique(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t name_offset, count = 0;
    struct hlsl_ir_var *pass;
    uint32_t count_offset;

    name_offset = write_string(var->name, fx);
    put_u32(buffer, name_offset);
    count_offset = put_u32(buffer, 0);
    write_fx_4_annotations(var->annotations, fx);

    count = 0;
    LIST_FOR_EACH_ENTRY(pass, &var->scope->vars, struct hlsl_ir_var, scope_entry)
    {
        write_pass(pass, fx);
        ++count;
    }

    set_u32(buffer, count_offset, count);
}

static void write_techniques(struct hlsl_scope *scope, struct fx_write_context *fx)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
    {
        if (technique_matches_version(var, fx))
        {
            fx->ops->write_technique(var, fx);
            ++fx->technique_count;
        }
    }

    set_status(fx, fx->unstructured.status);
    set_status(fx, fx->structured.status);
}

static void write_group(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t name_offset = write_string(var ? var->name : NULL, fx);
    uint32_t count_offset, count;

    put_u32(buffer, name_offset);
    count_offset = put_u32(buffer, 0); /* Technique count */
    write_fx_4_annotations(var ? var->annotations : NULL, fx);

    count = fx->technique_count;
    write_techniques(var ? var->scope : fx->ctx->globals, fx);
    set_u32(buffer, count_offset, fx->technique_count - count);

    ++fx->group_count;
}

static void write_groups(struct fx_write_context *fx)
{
    struct hlsl_scope *scope = fx->ctx->globals;
    bool needs_default_group = false;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
    {
        if (technique_matches_version(var, fx))
        {
            needs_default_group = true;
            break;
        }
    }

    if (needs_default_group)
        write_group(NULL, fx);
    LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
    {
        const struct hlsl_type *type = var->data_type;

        if (type->class == HLSL_CLASS_EFFECT_GROUP)
            write_group(var, fx);
    }
}

static uint32_t write_fx_2_string(const char *string, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    const char *s = string ? string : "";
    static const char tail[3];
    uint32_t size, offset;

    size = strlen(s) + 1;
    offset = put_u32(buffer, size);
    bytecode_put_bytes(buffer, s, size);
    size %= 4;
    if (size)
        bytecode_put_bytes_unaligned(buffer, tail, 4 - size);
    return offset;
}

static uint32_t get_fx_2_type_class(const struct hlsl_type *type)
{
    if (type->class == HLSL_CLASS_MATRIX)
        return D3DXPC_MATRIX_ROWS;
    return hlsl_sm1_class(type);
}

static uint32_t write_fx_2_parameter(const struct hlsl_type *type, const char *name, const struct hlsl_semantic *semantic,
        struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    uint32_t semantic_offset, offset, elements_count = 0, name_offset;
    size_t i;

    /* Resolve arrays to element type and number of elements. */
    if (type->class == HLSL_CLASS_ARRAY)
    {
        elements_count = hlsl_get_multiarray_size(type);
        type = hlsl_get_multiarray_element_type(type);
    }

    name_offset = write_string(name, fx);
    semantic_offset = semantic->raw_name ? write_string(semantic->raw_name, fx) : 0;

    offset = put_u32(buffer, hlsl_sm1_base_type(type));
    put_u32(buffer, get_fx_2_type_class(type));
    put_u32(buffer, name_offset);
    put_u32(buffer, semantic_offset);
    put_u32(buffer, elements_count);

    switch (type->class)
    {
        case HLSL_CLASS_VECTOR:
            put_u32(buffer, type->dimx);
            put_u32(buffer, type->dimy);
            break;
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_MATRIX:
            put_u32(buffer, type->dimy);
            put_u32(buffer, type->dimx);
            break;
        case HLSL_CLASS_STRUCT:
            put_u32(buffer, type->e.record.field_count);
            break;
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_PIXEL_SHADER:
            fx->shader_count += elements_count;
            break;
        default:
            ;
    }

    if (type->class == HLSL_CLASS_STRUCT)
    {
        for (i = 0; i < type->e.record.field_count; ++i)
        {
            const struct hlsl_struct_field *field = &type->e.record.fields[i];

            /* Validated in check_invalid_object_fields(). */
            VKD3D_ASSERT(hlsl_is_numeric_type(field->type));
            write_fx_2_parameter(field->type, field->name, &field->semantic, fx);
        }
    }

    return offset;
}

static void write_fx_2_technique(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t name_offset, count_offset, count = 0;
    struct hlsl_ir_var *pass;

    name_offset = write_string(var->name, fx);
    put_u32(buffer, name_offset);
    put_u32(buffer, 0); /* Annotation count. */
    count_offset = put_u32(buffer, 0); /* Pass count. */

    /* FIXME: annotations */

    LIST_FOR_EACH_ENTRY(pass, &var->scope->vars, struct hlsl_ir_var, scope_entry)
    {
        write_pass(pass, fx);
        ++count;
    }

    set_u32(buffer, count_offset, count);
}

static uint32_t get_fx_2_type_size(const struct hlsl_type *type)
{
    uint32_t size = 0, elements_count;
    size_t i;

    if (type->class == HLSL_CLASS_ARRAY)
    {
        elements_count = hlsl_get_multiarray_size(type);
        type = hlsl_get_multiarray_element_type(type);
        return get_fx_2_type_size(type) * elements_count;
    }
    else if (type->class == HLSL_CLASS_STRUCT)
    {
        for (i = 0; i < type->e.record.field_count; ++i)
        {
            const struct hlsl_struct_field *field = &type->e.record.fields[i];
            size += get_fx_2_type_size(field->type);
        }

        return size;
    }

    return type->dimx * type->dimy * sizeof(float);
}

static uint32_t write_fx_2_initial_value(const struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    const struct hlsl_type *type = var->data_type;
    uint32_t offset, size, elements_count = 1;

    size = get_fx_2_type_size(type);

    if (type->class == HLSL_CLASS_ARRAY)
    {
        elements_count = hlsl_get_multiarray_size(type);
        type = hlsl_get_multiarray_element_type(type);
    }

    /* Note that struct fields must all be numeric;
     * this was validated in check_invalid_object_fields(). */
    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
        case HLSL_CLASS_STRUCT:
            /* FIXME: write actual initial value */
            if (var->default_values)
                hlsl_fixme(fx->ctx, &var->loc, "Write default values.\n");

            offset = put_u32(buffer, 0);

            for (uint32_t i = 1; i < size / sizeof(uint32_t); ++i)
                put_u32(buffer, 0);
            break;

        default:
            /* Objects are given sequential ids. */
            offset = put_u32(buffer, fx->object_variable_count++);
            for (uint32_t i = 1; i < elements_count; ++i)
                put_u32(buffer, fx->object_variable_count++);
            break;
    }

    return offset;
}

static bool is_type_supported_fx_2(struct hlsl_ctx *ctx, const struct hlsl_type *type,
        const struct vkd3d_shader_location *loc)
{
    switch (type->class)
    {
        case HLSL_CLASS_STRUCT:
            /* Note that the fields must all be numeric; this was validated in
             * check_invalid_object_fields(). */
            return true;

        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            return true;

        case HLSL_CLASS_ARRAY:
            return is_type_supported_fx_2(ctx, type->e.array.type, loc);

        case HLSL_CLASS_TEXTURE:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                case HLSL_SAMPLER_DIM_2D:
                case HLSL_SAMPLER_DIM_3D:
                case HLSL_SAMPLER_DIM_CUBE:
                case HLSL_SAMPLER_DIM_GENERIC:
                    return true;
                default:
                    return false;
            }
            break;

        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_VERTEX_SHADER:
            hlsl_fixme(ctx, loc, "Write fx 2.0 parameter class %#x.", type->class);
            return false;

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
            return false;

        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_NULL:
            /* This cannot appear as an extern variable. */
            break;
    }

    vkd3d_unreachable();
}

static void write_fx_2_parameters(struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t desc_offset, value_offset, flags;
    struct hlsl_ctx *ctx = fx->ctx;
    struct hlsl_ir_var *var;
    enum fx_2_parameter_flags
    {
        IS_SHARED = 0x1,
    };

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!is_type_supported_fx_2(ctx, var->data_type, &var->loc))
            continue;

        desc_offset = write_fx_2_parameter(var->data_type, var->name, &var->semantic, fx);
        value_offset = write_fx_2_initial_value(var, fx);

        flags = 0;
        if (var->storage_modifiers & HLSL_STORAGE_SHARED)
            flags |= IS_SHARED;

        put_u32(buffer, desc_offset); /* Parameter description */
        put_u32(buffer, value_offset); /* Value */
        put_u32(buffer, flags); /* Flags */

        put_u32(buffer, 0); /* Annotations count */
        if (has_annotations(var))
            hlsl_fixme(ctx, &ctx->location, "Writing annotations for parameters is not implemented.");

        ++fx->parameter_count;
    }
}

static const struct fx_write_context_ops fx_2_ops =
{
    .write_string = write_fx_2_string,
    .write_technique = write_fx_2_technique,
    .write_pass = write_fx_2_pass,
};

static int hlsl_fx_2_write(struct hlsl_ctx *ctx, struct vkd3d_shader_code *out)
{
    uint32_t offset, size, technique_count, shader_count, parameter_count, object_count;
    struct vkd3d_bytecode_buffer buffer = { 0 };
    struct vkd3d_bytecode_buffer *structured;
    struct fx_write_context fx;

    fx_write_context_init(ctx, &fx_2_ops, &fx);
    fx.object_variable_count = 1;
    structured = &fx.structured;

    /* First entry is always zeroed and skipped. */
    put_u32(&fx.unstructured, 0);

    put_u32(&buffer, 0xfeff0901); /* Version. */
    offset = put_u32(&buffer, 0);

    parameter_count = put_u32(structured, 0); /* Parameter count */
    technique_count = put_u32(structured, 0);
    shader_count = put_u32(structured, 0);
    object_count = put_u32(structured, 0);

    write_fx_2_parameters(&fx);
    set_u32(structured, parameter_count, fx.parameter_count);
    set_u32(structured, object_count, fx.object_variable_count);

    write_techniques(ctx->globals, &fx);
    set_u32(structured, technique_count, fx.technique_count);
    set_u32(structured, shader_count, fx.shader_count);

    put_u32(structured, 0); /* String count */
    put_u32(structured, 0); /* Resource count */

    /* TODO: strings */
    /* TODO: resources */

    size = align(fx.unstructured.size, 4);
    set_u32(&buffer, offset, size);

    bytecode_put_bytes(&buffer, fx.unstructured.data, fx.unstructured.size);
    bytecode_put_bytes(&buffer, fx.structured.data, fx.structured.size);

    vkd3d_free(fx.unstructured.data);
    vkd3d_free(fx.structured.data);

    if (!fx.technique_count)
        hlsl_error(ctx, &ctx->location, VKD3D_SHADER_ERROR_HLSL_MISSING_TECHNIQUE, "No techniques found.");

    if (fx.status < 0)
        ctx->result = fx.status;

    if (!ctx->result)
    {
        out->code = buffer.data;
        out->size = buffer.size;
    }

    return fx_write_context_cleanup(&fx);
}

static const struct fx_write_context_ops fx_4_ops =
{
    .write_string = write_fx_4_string,
    .write_technique = write_fx_4_technique,
    .write_pass = write_fx_4_pass,
    .write_annotation = write_fx_4_annotation,
    .are_child_effects_supported = true,
};

static uint32_t write_fx_4_default_value(struct hlsl_type *value_type, struct hlsl_default_value *value,
            struct fx_write_context *fx)
{
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(value_type);
    uint32_t elements_count = hlsl_get_multiarray_size(value_type), i, j;
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    struct hlsl_ctx *ctx = fx->ctx;
    uint32_t offset = buffer->size;
    unsigned int comp_count;

    if (!value)
        return 0;

    comp_count = hlsl_type_component_count(type);

    for (i = 0; i < elements_count; ++i)
    {
        switch (type->class)
        {
            case HLSL_CLASS_SCALAR:
            case HLSL_CLASS_VECTOR:
            case HLSL_CLASS_MATRIX:
            {
                switch (type->e.numeric.type)
                {
                    case HLSL_TYPE_FLOAT:
                    case HLSL_TYPE_INT:
                    case HLSL_TYPE_UINT:
                    case HLSL_TYPE_BOOL:

                        for (j = 0; j < comp_count; ++j)
                        {
                            put_u32_unaligned(buffer, value->number.u);
                            value++;
                        }
                        break;
                    default:
                        hlsl_fixme(ctx, &ctx->location, "Writing default values for numeric type %u is not implemented.",
                                type->e.numeric.type);
                }

                break;
            }
            case HLSL_CLASS_STRUCT:
            {
                struct hlsl_struct_field *fields = type->e.record.fields;

                for (j = 0; j < type->e.record.field_count; ++j)
                {
                    write_fx_4_default_value(fields[i].type, value, fx);
                    value += hlsl_type_component_count(fields[i].type);
                }
                break;
            }
            default:
                hlsl_fixme(ctx, &ctx->location, "Writing default values for class %u is not implemented.", type->class);
        }
    }

    return offset;
}

static void write_fx_4_string_initializer(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    uint32_t elements_count = hlsl_get_multiarray_size(var->data_type), i;
    const struct hlsl_default_value *value = var->default_values;
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    struct hlsl_ctx *ctx = fx->ctx;
    uint32_t offset;

    if (!value)
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "String objects have to be initialized.");
        return;
    }

    for (i = 0; i < elements_count; ++i, ++value)
    {
        offset = write_fx_4_string(value->string, fx);
        put_u32(buffer, offset);
    }
}

static void write_fx_4_numeric_variable(struct hlsl_ir_var *var, bool shared, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t name_offset, type_offset, value_offset;
    uint32_t semantic_offset, flags = 0;
    enum fx_4_numeric_variable_flags
    {
        HAS_EXPLICIT_BIND_POINT = 0x4,
    };

    if (var->has_explicit_bind_point)
        flags |= HAS_EXPLICIT_BIND_POINT;

    type_offset = write_type(var->data_type, fx);
    name_offset = write_string(var->name, fx);
    semantic_offset = write_string(var->semantic.raw_name, fx);

    put_u32(buffer, name_offset);
    put_u32(buffer, type_offset);

    semantic_offset = put_u32(buffer, semantic_offset); /* Semantic */
    put_u32(buffer, var->buffer_offset * 4); /* Offset in the constant buffer, in bytes. */
    value_offset = put_u32(buffer, 0);
    put_u32(buffer, flags); /* Flags */

    if (shared)
    {
        fx->shared_numeric_variable_count++;
    }
    else
    {
        uint32_t offset = write_fx_4_default_value(var->data_type, var->default_values, fx);
        set_u32(buffer, value_offset, offset);

        write_fx_4_annotations(var->annotations, fx);

        fx->numeric_variable_count++;
    }
}

static void write_fx_4_annotation(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t name_offset, type_offset, offset;
    struct hlsl_ctx *ctx = fx->ctx;

    name_offset = write_string(var->name, fx);
    type_offset = write_type(var->data_type, fx);

    put_u32(buffer, name_offset);
    put_u32(buffer, type_offset);

    if (hlsl_is_numeric_type(type))
    {
        offset = write_fx_4_default_value(var->data_type, var->default_values, fx);
        put_u32(buffer, offset);
    }
    else if (type->class == HLSL_CLASS_STRING)
    {
        write_fx_4_string_initializer(var, fx);
    }
    else
    {
        hlsl_fixme(ctx, &var->loc, "Writing annotations for type class %u is not implemented.", type->class);
    }
}

struct rhs_named_value
{
    const char *name;
    unsigned int value;
};

static bool get_fx_4_state_enum_value(const struct rhs_named_value *pairs,
        const char *name, unsigned int *value)
{
    while (pairs->name)
    {
        if (!ascii_strcasecmp(pairs->name, name))
        {
            *value = pairs->value;
            return true;
        }

        pairs++;
    }

    return false;
}

static uint32_t write_fx_4_state_numeric_value(struct hlsl_ir_constant *value, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    struct hlsl_type *data_type = value->node.data_type;
    struct hlsl_ctx *ctx = fx->ctx;
    uint32_t i, type, offset;
    unsigned int count = hlsl_type_component_count(data_type);

    offset = put_u32_unaligned(buffer, count);

    for (i = 0; i < count; ++i)
    {
        if (hlsl_is_numeric_type(data_type))
        {
            switch (data_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                case HLSL_TYPE_BOOL:
                    type = fx_4_numeric_base_type[data_type->e.numeric.type];
                    break;
                default:
                    type = 0;
                    hlsl_fixme(ctx, &ctx->location, "Unsupported numeric state value type %u.", data_type->e.numeric.type);
            }
        }

        put_u32_unaligned(buffer, type);
        put_u32_unaligned(buffer, value->value.u[i].u);
    }

    return offset;
}

static void write_fx_4_state_assignment(const struct hlsl_ir_var *var, struct hlsl_state_block_entry *entry,
        struct fx_write_context *fx)
{
    uint32_t value_offset = 0, assignment_type = 0, rhs_offset;
    uint32_t type_offset;
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    struct hlsl_ctx *ctx = fx->ctx;
    struct hlsl_ir_node *value = entry->args->node;

    put_u32(buffer, entry->name_id);
    put_u32(buffer, entry->lhs_index);
    type_offset = put_u32(buffer, 0);
    rhs_offset = put_u32(buffer, 0);

    switch (value->type)
    {
        case HLSL_IR_CONSTANT:
        {
            struct hlsl_ir_constant *c = hlsl_ir_constant(value);

            value_offset = write_fx_4_state_numeric_value(c, fx);
            assignment_type = 1;
            break;
        }
        case HLSL_IR_LOAD:
        {
            struct hlsl_ir_load *l = hlsl_ir_load(value);

            if (l->src.path_len)
                hlsl_fixme(ctx, &var->loc, "Indexed access in RHS values is not implemented.");

            value_offset = write_fx_4_string(l->src.var->name, fx);
            assignment_type = 2;
            break;
        }
        default:
            hlsl_fixme(ctx, &var->loc, "Unsupported assignment type for state %s.", entry->name);
    }

    set_u32(buffer, type_offset, assignment_type);
    set_u32(buffer, rhs_offset, value_offset);
}

static bool state_block_contains_state(const struct hlsl_state_block_entry *entry, unsigned int start_index,
        struct hlsl_state_block *block)
{
    unsigned int i;

    for (i = start_index; i < block->count; ++i)
    {
        const struct hlsl_state_block_entry *cur = block->entries[i];

        if (cur->is_function_call)
            continue;

        if (ascii_strcasecmp(cur->name, entry->name))
            continue;

        if (cur->lhs_has_index != entry->lhs_has_index)
            continue;

        if (cur->lhs_has_index && cur->lhs_index != entry->lhs_index)
            continue;

        return true;
    }

    return false;
}

struct replace_state_context
{
    const struct rhs_named_value *values;
    struct hlsl_ir_var *var;
};

static bool lower_null_constant(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_node *c;

    if (instr->type != HLSL_IR_CONSTANT)
        return false;
    if (instr->data_type->class != HLSL_CLASS_NULL)
        return false;

    if (!(c = hlsl_new_uint_constant(ctx, 0, &instr->loc)))
        return false;

    list_add_before(&instr->entry, &c->entry);
    hlsl_replace_node(instr, c);

    return true;
}

static bool replace_state_block_constant(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct replace_state_context *replace_context = context;
    struct hlsl_ir_stateblock_constant *state_constant;
    struct hlsl_ir_node *c;
    unsigned int value;

    if (!replace_context->values)
        return false;
    if (instr->type != HLSL_IR_STATEBLOCK_CONSTANT)
        return false;

    state_constant = hlsl_ir_stateblock_constant(instr);
    if (!get_fx_4_state_enum_value(replace_context->values, state_constant->name, &value))
    {
        hlsl_error(ctx, &replace_context->var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                "Unrecognized state constant %s.", state_constant->name);
        return false;
    }

    if (!(c = hlsl_new_uint_constant(ctx, value, &replace_context->var->loc)))
        return false;

    list_add_before(&state_constant->node.entry, &c->entry);
    hlsl_replace_node(&state_constant->node, c);

    return true;
}

enum state_property_component_type
{
    FX_BOOL,
    FX_FLOAT,
    FX_UINT,
    FX_UINT8,
    FX_DEPTHSTENCIL,
    FX_RASTERIZER,
    FX_DOMAINSHADER,
    FX_HULLSHADER,
    FX_COMPUTESHADER,
    FX_TEXTURE,
    FX_DEPTHSTENCILVIEW,
    FX_RENDERTARGETVIEW,
    FX_BLEND,
    FX_VERTEXSHADER,
    FX_PIXELSHADER,
};

static inline bool is_object_fx_type(enum state_property_component_type type)
{
    switch (type)
    {
        case FX_DEPTHSTENCIL:
        case FX_RASTERIZER:
        case FX_DOMAINSHADER:
        case FX_HULLSHADER:
        case FX_COMPUTESHADER:
        case FX_TEXTURE:
        case FX_RENDERTARGETVIEW:
        case FX_DEPTHSTENCILVIEW:
        case FX_BLEND:
        case FX_VERTEXSHADER:
        case FX_PIXELSHADER:
            return true;
        default:
            return false;
    }
}

static inline enum hlsl_type_class hlsl_type_class_from_fx_type(enum state_property_component_type type)
{
    switch (type)
    {
        case FX_DEPTHSTENCIL:
            return HLSL_CLASS_DEPTH_STENCIL_STATE;
        case FX_RASTERIZER:
            return HLSL_CLASS_RASTERIZER_STATE;
        case FX_DOMAINSHADER:
            return HLSL_CLASS_DOMAIN_SHADER;
        case FX_HULLSHADER:
            return HLSL_CLASS_HULL_SHADER;
        case FX_COMPUTESHADER:
            return HLSL_CLASS_COMPUTE_SHADER;
        case FX_TEXTURE:
            return HLSL_CLASS_TEXTURE;
        case FX_RENDERTARGETVIEW:
            return HLSL_CLASS_RENDER_TARGET_VIEW;
        case FX_DEPTHSTENCILVIEW:
            return HLSL_CLASS_DEPTH_STENCIL_VIEW;
        case FX_BLEND:
            return HLSL_CLASS_BLEND_STATE;
        case FX_VERTEXSHADER:
            return HLSL_CLASS_VERTEX_SHADER;
        case FX_PIXELSHADER:
            return HLSL_CLASS_PIXEL_SHADER;
        default:
            vkd3d_unreachable();
    }
}

static inline enum hlsl_base_type hlsl_type_from_fx_type(enum state_property_component_type type)
{
    switch (type)
    {
        case FX_BOOL:
            return HLSL_TYPE_BOOL;
        case FX_FLOAT:
            return HLSL_TYPE_FLOAT;
        case FX_UINT:
        case FX_UINT8:
            return HLSL_TYPE_UINT;
        default:
            vkd3d_unreachable();
     }
}

static void resolve_fx_4_state_block_values(struct hlsl_ir_var *var, struct hlsl_state_block_entry *entry,
        struct fx_write_context *fx)
{
    static const struct rhs_named_value filter_values[] =
    {
        { "MIN_MAG_MIP_POINT", 0x00 },
        { "MIN_MAG_POINT_MIP_LINEAR", 0x01 },
        { "MIN_POINT_MAG_LINEAR_MIP_POINT", 0x04 },
        { "MIN_POINT_MAG_MIP_LINEAR", 0x05 },
        { "MIN_LINEAR_MAG_MIP_POINT", 0x10 },
        { "MIN_LINEAR_MAG_POINT_MIP_LINEAR", 0x11 },
        { "MIN_MAG_LINEAR_MIP_POINT", 0x14 },
        { "MIN_MAG_MIP_LINEAR", 0x15 },
        { "ANISOTROPIC", 0x55 },
        { "COMPARISON_MIN_MAG_MIP_POINT", 0x80 },
        { "COMPARISON_MIN_MAG_POINT_MIP_LINEAR", 0x81 },
        { "COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT", 0x84 },
        { "COMPARISON_MIN_POINT_MAG_MIP_LINEAR", 0x85 },
        { "COMPARISON_MIN_LINEAR_MAG_MIP_POINT", 0x90 },
        { "COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR", 0x91 },
        { "COMPARISON_MIN_MAG_LINEAR_MIP_POINT", 0x94 },
        { "COMPARISON_MIN_MAG_MIP_LINEAR", 0x95 },
        { "COMPARISON_ANISOTROPIC", 0xd5 },
        { NULL },
    };

    static const struct rhs_named_value address_values[] =
    {
        { "WRAP", 1 },
        { "MIRROR", 2 },
        { "CLAMP", 3 },
        { "BORDER", 4 },
        { "MIRROR_ONCE", 5 },
        { NULL },
    };

    static const struct rhs_named_value compare_func_values[] =
    {
        { "NEVER",         1 },
        { "LESS",          2 },
        { "EQUAL",         3 },
        { "LESS_EQUAL",    4 },
        { "GREATER",       5 },
        { "NOT_EQUAL",     6 },
        { "GREATER_EQUAL", 7 },
        { "ALWAYS",        8 },
        { NULL }
    };

    static const struct rhs_named_value depth_write_mask_values[] =
    {
        { "ZERO", 0 },
        { "ALL",  1 },
        { NULL }
    };

    static const struct rhs_named_value comparison_values[] =
    {
        { "NEVER", 1 },
        { "LESS",  2 },
        { "EQUAL", 3 },
        { "LESS_EQUAL", 4 },
        { "GREATER", 5 },
        { "NOT_EQUAL", 6 },
        { "GREATER_EQUAL", 7 },
        { "ALWAYS", 8 },
        { NULL }
    };

    static const struct rhs_named_value stencil_op_values[] =
    {
        { "KEEP", 1 },
        { "ZERO", 2 },
        { "REPLACE", 3 },
        { "INCR_SAT", 4 },
        { "DECR_SAT", 5 },
        { "INVERT", 6 },
        { "INCR", 7 },
        { "DECR", 8 },
        { NULL }
    };

    static const struct rhs_named_value fill_values[] =
    {
        { "WIREFRAME", 2 },
        { "SOLID", 3 },
        { NULL }
    };

    static const struct rhs_named_value cull_values[] =
    {
        { "NONE", 1 },
        { "FRONT", 2 },
        { "BACK", 3 },
        { NULL }
    };

    static const struct rhs_named_value blend_values[] =
    {
        { "ZERO", 1 },
        { "ONE", 2 },
        { "SRC_COLOR", 3 },
        { "INV_SRC_COLOR", 4 },
        { "SRC_ALPHA", 5 },
        { "INV_SRC_ALPHA", 6 },
        { "DEST_ALPHA", 7 },
        { "INV_DEST_ALPHA", 8 },
        { "DEST_COLOR", 9 },
        { "INV_DEST_COLOR", 10 },
        { "SRC_ALPHA_SAT", 11 },
        { "BLEND_FACTOR", 14 },
        { "INV_BLEND_FACTOR", 15 },
        { "SRC1_COLOR", 16 },
        { "INV_SRC1_COLOR", 17 },
        { "SRC1_ALPHA", 18 },
        { "INV_SRC1_ALPHA", 19 },
        { NULL }
    };

    static const struct rhs_named_value blendop_values[] =
    {
        { "ADD", 1 },
        { "SUBTRACT", 2 },
        { "REV_SUBTRACT", 3 },
        { "MIN", 4 },
        { "MAX", 5 },
        { NULL }
    };

    static const struct rhs_named_value bool_values[] =
    {
        { "FALSE", 0 },
        { "TRUE", 1 },
        { NULL }
    };

    static const struct rhs_named_value null_values[] =
    {
        { "NULL", 0 },
        { NULL }
    };

    static const struct state
    {
        const char *name;
        enum hlsl_type_class container;
        enum hlsl_type_class class;
        enum state_property_component_type type;
        unsigned int dimx;
        unsigned int array_size;
        uint32_t id;
        const struct rhs_named_value *values;
    }
    states[] =
    {
        { "RasterizerState",       HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_RASTERIZER,       1, 1, 0 },
        { "DepthStencilState",     HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_DEPTHSTENCIL,     1, 1, 1 },
        { "BlendState",            HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_BLEND,            1, 1, 2 },
        { "RenderTargetView",      HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_RENDERTARGETVIEW, 1, 8, 3 },
        { "DepthStencilView",      HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_DEPTHSTENCILVIEW, 1, 1, 4 },

        { "VertexShader",          HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_VERTEXSHADER,     1, 1, 6 },
        { "PixelShader",           HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_PIXELSHADER,      1, 1, 7 },
        { "DS_StencilRef",         HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 9 },
        { "AB_BlendFactor",        HLSL_CLASS_PASS, HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 10 },
        { "AB_SampleMask",         HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 11 },

        { "FillMode",              HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 12, fill_values },
        { "CullMode",              HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 13, cull_values },
        { "FrontCounterClockwise", HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 14, bool_values },
        { "DepthBias",             HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 15 },
        { "DepthBiasClamp",        HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 16 },
        { "SlopeScaledDepthBias",  HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 17 },
        { "DepthClipEnable",       HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 18, bool_values },
        { "ScissorEnable",         HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 19, bool_values },
        { "MultisampleEnable",     HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 20, bool_values },
        { "AntializedLineEnable",  HLSL_CLASS_RASTERIZER_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 21, bool_values },

        { "DepthEnable",               HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 22, bool_values },
        { "DepthWriteMask",            HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 23, depth_write_mask_values },
        { "DepthFunc",                 HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 24, comparison_values },
        { "StencilEnable",             HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 25, bool_values },
        { "StencilReadMask",           HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT8, 1, 1, 26 },
        { "StencilWriteMask",          HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT8, 1, 1, 27 },
        { "FrontFaceStencilFail",      HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 28, stencil_op_values },
        { "FrontFaceStencilDepthFail", HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 29, stencil_op_values },
        { "FrontFaceStencilPass",      HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 30, stencil_op_values },
        { "FrontFaceStencilFunc",      HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 31, comparison_values },
        { "BackFaceStencilFail",       HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 32, stencil_op_values },
        { "BackFaceStencilDepthFail",  HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 33, stencil_op_values },
        { "BackFaceStencilPass",       HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 34, stencil_op_values },
        { "BackFaceStencilFunc",       HLSL_CLASS_DEPTH_STENCIL_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 35, comparison_values },

        { "Filter",         HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_UINT,    1, 1, 45, filter_values },
        { "AddressU",       HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_UINT,    1, 1, 46, address_values },
        { "AddressV",       HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_UINT,    1, 1, 47, address_values },
        { "AddressW",       HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_UINT,    1, 1, 48, address_values },
        { "MipLODBias",     HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_FLOAT,   1, 1, 49 },
        { "MaxAnisotropy",  HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_UINT,    1, 1, 50 },
        { "ComparisonFunc", HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_UINT,    1, 1, 51, compare_func_values },
        { "BorderColor",    HLSL_CLASS_SAMPLER, HLSL_CLASS_VECTOR,  FX_FLOAT,   4, 1, 52 },
        { "MinLOD",         HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_FLOAT,   1, 1, 53 },
        { "MaxLOD",         HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_FLOAT,   1, 1, 54 },
        { "Texture",        HLSL_CLASS_SAMPLER, HLSL_CLASS_SCALAR,  FX_TEXTURE, 1, 1, 55, null_values },

        { "HullShader",     HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_HULLSHADER,    1, 1, 56 },
        { "DomainShader",   HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_DOMAINSHADER,  1, 1, 57 },
        { "ComputeShader",  HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_COMPUTESHADER, 1, 1, 58 },
    };

    static const struct state fx_4_blend_states[] =
    {
        { "AlphaToCoverageEnable", HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 36, bool_values },
        { "BlendEnable",           HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 8, 37, bool_values },
        { "SrcBlend",              HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 38, blend_values },
        { "DestBlend",             HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 39, blend_values },
        { "BlendOp",               HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 40, blendop_values },
        { "SrcBlendAlpha",         HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 41, blend_values },
        { "DestBlendAlpha",        HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 42, blend_values },
        { "BlendOpAlpha",          HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 43, blendop_values },
        { "RenderTargetWriteMask", HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT8, 1, 8, 44 },
    };

    static const struct state fx_5_blend_states[] =
    {
        { "AlphaToCoverageEnable", HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 36, bool_values },
        { "BlendEnable",           HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 8, 37, bool_values },
        { "SrcBlend",              HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 8, 38, blend_values },
        { "DestBlend",             HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 8, 39, blend_values },
        { "BlendOp",               HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 8, 40, blendop_values },
        { "SrcBlendAlpha",         HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 8, 41, blend_values },
        { "DestBlendAlpha",        HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 8, 42, blend_values },
        { "BlendOpAlpha",          HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 8, 43, blendop_values },
        { "RenderTargetWriteMask", HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT8, 1, 8, 44 },
    };

    struct state_table
    {
        const struct state *ptr;
        unsigned int count;
    } table;

    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);
    struct replace_state_context replace_context;
    struct hlsl_type *state_type = NULL;
    struct hlsl_ir_node *node, *cast;
    const struct state *state = NULL;
    struct hlsl_ctx *ctx = fx->ctx;
    enum hlsl_base_type base_type;
    unsigned int i;

    if (type->class == HLSL_CLASS_BLEND_STATE)
    {
        if (ctx->profile->major_version == 4)
        {
            table.ptr = fx_4_blend_states;
            table.count = ARRAY_SIZE(fx_4_blend_states);
        }
        else
        {
            table.ptr = fx_5_blend_states;
            table.count = ARRAY_SIZE(fx_5_blend_states);
        }
    }
    else
    {
        table.ptr = states;
        table.count = ARRAY_SIZE(states);
    }

    for (i = 0; i < table.count; ++i)
    {
        if (type->class == table.ptr[i].container
                && !ascii_strcasecmp(entry->name, table.ptr[i].name))
        {
            state = &table.ptr[i];
            break;
        }
    }

    if (!state)
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Unrecognized state name %s.", entry->name);
        return;
    }

    if (entry->args_count != 1)
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Unrecognized initializer for the state %s.",
                entry->name);
        return;
    }

    if (entry->lhs_has_index && state->array_size == 1)
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Can't use array-style access for non-array state %s.",
                entry->name);
        return;
    }

    if (!entry->lhs_has_index && state->array_size > 1)
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Expected array index for array state %s.",
                entry->name);
        return;
    }

    if (entry->lhs_has_index && (state->array_size <= entry->lhs_index))
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Invalid element index %u for the state %s[%u].",
                entry->lhs_index, state->name, state->array_size);
        return;
    }

    entry->name_id = state->id;

    replace_context.values = state->values;
    replace_context.var = var;

    /* Turn named constants to actual constants. */
    hlsl_transform_ir(ctx, lower_null_constant, entry->instrs, NULL);
    hlsl_transform_ir(ctx, replace_state_block_constant, entry->instrs, &replace_context);
    hlsl_run_const_passes(ctx, entry->instrs);

    /* Now cast and run folding again. */

    if (is_object_fx_type(state->type))
    {
        node = entry->args->node;

        switch (node->type)
        {
            case HLSL_IR_LOAD:
            {
                struct hlsl_ir_load *load = hlsl_ir_load(node);

                if (load->src.path_len)
                    hlsl_fixme(ctx, &ctx->location, "Arrays are not supported for RHS.");

                if (load->src.var->data_type->class != hlsl_type_class_from_fx_type(state->type))
                {
                    hlsl_error(ctx, &ctx->location, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Type mismatch for the %s state value",
                            entry->name);
                }

                break;
            }
            case HLSL_IR_CONSTANT:
            {
                struct hlsl_ir_constant *c = hlsl_ir_constant(node);
                struct hlsl_type *data_type = c->node.data_type;

                if (data_type->class == HLSL_CLASS_SCALAR && data_type->e.numeric.type == HLSL_TYPE_UINT)
                {
                    if (c->value.u[0].u != 0)
                        hlsl_error(ctx, &ctx->location, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                                "Only 0 integer constants are allowed for object-typed fields.");
                }
                else
                {
                    hlsl_error(ctx, &ctx->location, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                            "Unexpected constant used for object-typed field.");
                }

                break;
            }
            default:
                hlsl_fixme(ctx, &ctx->location, "Unhandled node type for object-typed field.");
        }

        return;
    }

    base_type = hlsl_type_from_fx_type(state->type);
    switch (state->class)
    {
        case HLSL_CLASS_VECTOR:
            state_type = hlsl_get_vector_type(ctx, base_type, state->dimx);
            break;
        case HLSL_CLASS_SCALAR:
            state_type = hlsl_get_scalar_type(ctx, base_type);
            break;
        case HLSL_CLASS_TEXTURE:
            hlsl_fixme(ctx, &ctx->location, "Object type fields are not supported.");
            break;
        default:
            ;
    }

    if (state_type)
    {
        node = entry->args->node;
        if (!(cast = hlsl_new_cast(ctx, node, state_type, &var->loc)))
            return;
        list_add_after(&node->entry, &cast->entry);

        /* FX_UINT8 values are using 32-bits in the binary. Mask higher 24 bits for those. */
        if (state->type == FX_UINT8)
        {
            struct hlsl_ir_node *mask;

            if (!(mask = hlsl_new_uint_constant(ctx, 0xff, &var->loc)))
                return;
            list_add_after(&cast->entry, &mask->entry);

            if (!(cast = hlsl_new_binary_expr(ctx, HLSL_OP2_BIT_AND, cast, mask)))
                return;
            list_add_after(&mask->entry, &cast->entry);
        }

        hlsl_src_remove(entry->args);
        hlsl_src_from_node(entry->args, cast);

        hlsl_run_const_passes(ctx, entry->instrs);
    }
}

static bool decompose_fx_4_state_add_entries(struct hlsl_state_block *block, unsigned int entry_index,
        unsigned int count)
{
    if (!vkd3d_array_reserve((void **)&block->entries, &block->capacity, block->count + count, sizeof(*block->entries)))
        return false;

    if (entry_index != block->count - 1)
    {
        memmove(&block->entries[entry_index + count + 1], &block->entries[entry_index + 1],
                (block->count - entry_index - 1) * sizeof(*block->entries));
    }
    block->count += count;

    return true;
}

static unsigned int decompose_fx_4_state_function_call(struct hlsl_ir_var *var, struct hlsl_state_block *block,
        unsigned int entry_index, struct fx_write_context *fx)
{
    struct hlsl_state_block_entry *entry = block->entries[entry_index];
    const struct state_block_function_info *info;
    struct function_component components[9];
    struct hlsl_ctx *ctx = fx->ctx;
    unsigned int i;

    if (!entry->is_function_call)
        return 1;

    if (!(info = get_state_block_function_info(entry->name)))
        return 1;

    if (info->min_profile > ctx->profile->major_version)
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_STATE_BLOCK_ENTRY,
                "State %s is not supported for this profile.", entry->name);
        return 1;
    }

    /* For single argument case simply replace the name. */
    if (info->min_args == info->max_args && info->min_args == 1)
    {
        vkd3d_free(entry->name);
        entry->name = hlsl_strdup(ctx, info->components[0].name);
        return 1;
    }

    if (!decompose_fx_4_state_add_entries(block, entry_index, entry->args_count - 1))
        return 1;

    get_state_block_function_components(info, components, entry->args_count);

    for (i = 0; i < entry->args_count; ++i)
    {
        const struct function_component *comp = &components[i];
        unsigned int arg_index = (i + 1) % entry->args_count;
        block->entries[entry_index + i] = clone_stateblock_entry(ctx, entry, comp->name,
                comp->lhs_has_index, comp->lhs_index, arg_index);
    }
    hlsl_free_state_block_entry(entry);

    return entry->args_count;
}

/* For some states assignment sets all of the elements. This behaviour is limited to certain states of BlendState
   object, and only when fx_5_0 profile is used. */
static unsigned int decompose_fx_4_state_block_expand_array(struct hlsl_ir_var *var, struct hlsl_state_block *block,
        unsigned int entry_index, struct fx_write_context *fx)
{
    static const char *states[] = { "SrcBlend", "DestBlend", "BlendOp", "SrcBlendAlpha", "DestBlendAlpha", "BlendOpAlpha" };
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);
    struct hlsl_state_block_entry *entry = block->entries[entry_index];
    static const unsigned int array_size = 8;
    struct hlsl_ctx *ctx = fx->ctx;
    bool found = false;
    unsigned int i;

    if (type->class != HLSL_CLASS_BLEND_STATE)
        return 1;
    if (ctx->profile->major_version != 5)
        return 1;
    if (entry->lhs_has_index)
        return 1;

    for (i = 0; i < ARRAY_SIZE(states); ++i)
    {
        if (!ascii_strcasecmp(entry->name, states[i]))
        {
            found = true;
            break;
        }
    }

    if (!found)
        return 1;

    if (!decompose_fx_4_state_add_entries(block, entry_index, array_size - 1))
        return 1;

    block->entries[entry_index]->lhs_has_index = true;
    for (i = 1; i < array_size; ++i)
    {
        block->entries[entry_index + i] = clone_stateblock_entry(ctx, entry,
                entry->name, true, i, 0);
    }

    return array_size;
}

static unsigned int decompose_fx_4_state_block(struct hlsl_ir_var *var, struct hlsl_state_block *block,
        unsigned int entry_index, struct fx_write_context *fx)
{
    struct hlsl_state_block_entry *entry = block->entries[entry_index];

    if (entry->is_function_call)
        return decompose_fx_4_state_function_call(var, block, entry_index, fx);

    return decompose_fx_4_state_block_expand_array(var, block, entry_index, fx);
}

static void write_fx_4_state_block(struct hlsl_ir_var *var, unsigned int block_index,
        uint32_t count_offset, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    struct hlsl_state_block *block;
    uint32_t i, count = 0;

    if (var->state_blocks)
    {
        block = var->state_blocks[block_index];

        for (i = 0; i < block->count;)
        {
            i += decompose_fx_4_state_block(var, block, i, fx);
        }

        for (i = 0; i < block->count; ++i)
        {
            struct hlsl_state_block_entry *entry = block->entries[i];

            /* Skip if property is reassigned later. This will use the last assignment. */
            if (state_block_contains_state(entry, i + 1, block))
                continue;

            /* Resolve special constant names and property names. */
            resolve_fx_4_state_block_values(var, entry, fx);

            write_fx_4_state_assignment(var, entry, fx);
            ++count;
        }
    }

    set_u32(buffer, count_offset, count);
}

static void write_fx_4_state_object_initializer(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    uint32_t elements_count = hlsl_get_multiarray_size(var->data_type), i;
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t count_offset;

    for (i = 0; i < elements_count; ++i)
    {
        count_offset = put_u32(buffer, 0);

        write_fx_4_state_block(var, i, count_offset, fx);
    }
}

static void write_fx_4_shader_initializer(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t elements_count = hlsl_get_multiarray_size(var->data_type);
    unsigned int i;

    /* FIXME: write shader blobs, once parser support works. */
    for (i = 0; i < elements_count; ++i)
        put_u32(buffer, 0);
}

static void write_fx_5_shader_initializer(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t elements_count = hlsl_get_multiarray_size(var->data_type);
    unsigned int i;

    /* FIXME: write shader blobs, once parser support works. */
    for (i = 0; i < elements_count; ++i)
    {
        put_u32(buffer, 0); /* Blob offset */
        put_u32(buffer, 0); /* SODecl[0] offset */
        put_u32(buffer, 0); /* SODecl[1] offset */
        put_u32(buffer, 0); /* SODecl[2] offset */
        put_u32(buffer, 0); /* SODecl[3] offset */
        put_u32(buffer, 0); /* SODecl count */
        put_u32(buffer, 0); /* Rasterizer stream */
        put_u32(buffer, 0); /* Interface bindings count */
        put_u32(buffer, 0); /* Interface initializer offset */
    }
}

static void write_fx_4_object_variable(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);
    uint32_t elements_count = hlsl_get_multiarray_size(var->data_type);
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t semantic_offset, bind_point = ~0u;
    uint32_t name_offset, type_offset;
    struct hlsl_ctx *ctx = fx->ctx;

    if (var->reg_reservation.reg_type)
        bind_point = var->reg_reservation.reg_index;

    type_offset = write_type(var->data_type, fx);
    name_offset = write_string(var->name, fx);
    semantic_offset = write_string(var->semantic.raw_name, fx);

    put_u32(buffer, name_offset);
    put_u32(buffer, type_offset);

    semantic_offset = put_u32(buffer, semantic_offset); /* Semantic */
    put_u32(buffer, bind_point); /* Explicit bind point */

    if (fx->child_effect && var->storage_modifiers & HLSL_STORAGE_SHARED)
    {
        ++fx->shared_object_count;
        return;
    }

    /* Initializer */
    switch (type->class)
    {
        case HLSL_CLASS_RENDER_TARGET_VIEW:
            fx->rtv_count += elements_count;
            break;
        case HLSL_CLASS_TEXTURE:
            fx->texture_count += elements_count;
            break;
        case HLSL_CLASS_UAV:
            fx->uav_count += elements_count;
            break;

        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_VERTEX_SHADER:
            write_fx_4_shader_initializer(var, fx);
            fx->shader_count += elements_count;
            break;

        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
            write_fx_5_shader_initializer(var, fx);
            fx->shader_count += elements_count;
            break;

        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
            fx->dsv_count += elements_count;
            break;

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
            write_fx_4_state_object_initializer(var, fx);
            fx->depth_stencil_state_count += elements_count;
            break;

        case HLSL_CLASS_SAMPLER:
            write_fx_4_state_object_initializer(var, fx);
            fx->sampler_state_count += elements_count;
            break;

        case HLSL_CLASS_RASTERIZER_STATE:
            write_fx_4_state_object_initializer(var, fx);
            fx->rasterizer_state_count += elements_count;
            break;

        case HLSL_CLASS_BLEND_STATE:
            write_fx_4_state_object_initializer(var, fx);
            fx->blend_state_count += elements_count;
            break;

        case HLSL_CLASS_STRING:
            write_fx_4_string_initializer(var, fx);
            fx->string_count += elements_count;
            break;

        default:
            hlsl_fixme(ctx, &ctx->location, "Writing initializer for object class %u is not implemented.",
                    type->class);
    }

    write_fx_4_annotations(var->annotations, fx);

    ++fx->object_variable_count;
}

static void write_fx_4_buffer(struct hlsl_buffer *b, struct fx_write_context *fx)
{
    enum fx_4_buffer_flags
    {
        IS_TBUFFER = 0x1,
        IS_SINGLE  = 0x2,
    };
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t count = 0, bind_point = ~0u, flags = 0, size;
    uint32_t name_offset, size_offset;
    struct hlsl_ctx *ctx = fx->ctx;
    struct hlsl_ir_var *var;
    uint32_t count_offset;
    bool shared;

    shared = fx->child_effect && b->modifiers & HLSL_STORAGE_SHARED;

    if (b->reservation.reg_type)
        bind_point = b->reservation.reg_index;
    if (b->type == HLSL_BUFFER_TEXTURE)
        flags |= IS_TBUFFER;
    if (ctx->profile->major_version == 5 && b->modifiers & HLSL_MODIFIER_SINGLE)
        flags |= IS_SINGLE;

    name_offset = write_string(b->name, fx);

    put_u32(buffer, name_offset); /* Name */
    size_offset = put_u32(buffer, 0); /* Data size */
    put_u32(buffer, flags); /* Flags */
    count_offset = put_u32(buffer, 0);
    put_u32(buffer, bind_point); /* Bind point */

    if (shared)
    {
        ++fx->shared_buffer_count;
    }
    else
    {
        write_fx_4_annotations(b->annotations, fx);
        ++fx->buffer_count;
    }

    count = 0;
    size = 0;
    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->buffer != b)
            continue;

        write_fx_4_numeric_variable(var, shared, fx);
        size += get_fx_4_type_size(var->data_type);
        ++count;
    }

    set_u32(buffer, count_offset, count);
    set_u32(buffer, size_offset, align(size, 16));
}

static void write_buffers(struct fx_write_context *fx, bool shared)
{
    struct hlsl_buffer *buffer;

    if (shared && !fx->child_effect)
        return;

    LIST_FOR_EACH_ENTRY(buffer, &fx->ctx->buffers, struct hlsl_buffer, entry)
    {
        if (!buffer->size && !fx->include_empty_buffers)
            continue;
        if (!strcmp(buffer->name, "$Params"))
            continue;
        if (fx->child_effect && (shared != !!(buffer->modifiers & HLSL_STORAGE_SHARED)))
            continue;

        write_fx_4_buffer(buffer, fx);
    }
}

static bool is_supported_object_variable(const struct hlsl_ctx *ctx, const struct hlsl_ir_var *var)
{
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);

    switch (type->class)
    {
        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_STRING:
            return true;
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
            if (ctx->profile->major_version < 5)
                return false;
            return true;
        case HLSL_CLASS_UAV:
            if (ctx->profile->major_version < 5)
                return false;
            if (type->e.resource.rasteriser_ordered)
                return false;
            return true;

        default:
            return false;
    }
}

static void write_objects(struct fx_write_context *fx, bool shared)
{
    struct hlsl_ctx *ctx = fx->ctx;
    struct hlsl_ir_var *var;

    if (shared && !fx->child_effect)
        return;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!is_supported_object_variable(ctx, var))
            continue;

        if (fx->child_effect && (shared != !!(var->storage_modifiers & HLSL_STORAGE_SHARED)))
            continue;

        write_fx_4_object_variable(var, fx);
    }
}

static int hlsl_fx_4_write(struct hlsl_ctx *ctx, struct vkd3d_shader_code *out)
{
    struct vkd3d_bytecode_buffer buffer = { 0 };
    struct fx_write_context fx;
    uint32_t size_offset;

    fx_write_context_init(ctx, &fx_4_ops, &fx);

    put_u32(&fx.unstructured, 0); /* Empty string placeholder. */

    write_buffers(&fx, false);
    write_objects(&fx, false);
    write_buffers(&fx, true);
    write_objects(&fx, true);

    write_techniques(ctx->globals, &fx);

    put_u32(&buffer, ctx->profile->minor_version == 0 ? 0xfeff1001 : 0xfeff1011); /* Version. */
    put_u32(&buffer, fx.buffer_count); /* Buffer count. */
    put_u32(&buffer, fx.numeric_variable_count); /* Numeric variable count. */
    put_u32(&buffer, fx.object_variable_count); /* Object variable count. */
    put_u32(&buffer, fx.shared_buffer_count);
    put_u32(&buffer, fx.shared_numeric_variable_count);
    put_u32(&buffer, fx.shared_object_count);
    put_u32(&buffer, fx.technique_count);
    size_offset = put_u32(&buffer, 0); /* Unstructured size. */
    put_u32(&buffer, fx.string_count);
    put_u32(&buffer, fx.texture_count);
    put_u32(&buffer, fx.depth_stencil_state_count);
    put_u32(&buffer, fx.blend_state_count);
    put_u32(&buffer, fx.rasterizer_state_count);
    put_u32(&buffer, fx.sampler_state_count);
    put_u32(&buffer, fx.rtv_count);
    put_u32(&buffer, fx.dsv_count);
    put_u32(&buffer, fx.shader_count);
    put_u32(&buffer, 0); /* Inline shader count. */

    set_u32(&buffer, size_offset, fx.unstructured.size);

    bytecode_put_bytes(&buffer, fx.unstructured.data, fx.unstructured.size);
    bytecode_put_bytes_unaligned(&buffer, fx.structured.data, fx.structured.size);

    vkd3d_free(fx.unstructured.data);
    vkd3d_free(fx.structured.data);

    set_status(&fx, buffer.status);

    if (fx.status < 0)
        ctx->result = fx.status;

    if (!ctx->result)
    {
        out->code = buffer.data;
        out->size = buffer.size;
    }

    return fx_write_context_cleanup(&fx);
}

static int hlsl_fx_5_write(struct hlsl_ctx *ctx, struct vkd3d_shader_code *out)
{
    struct vkd3d_bytecode_buffer buffer = { 0 };
    struct fx_write_context fx;
    uint32_t size_offset;

    fx_write_context_init(ctx, &fx_4_ops, &fx);

    put_u32(&fx.unstructured, 0); /* Empty string placeholder. */

    write_buffers(&fx, false);
    write_objects(&fx, false);
    /* TODO: interface variables */

    write_groups(&fx);

    put_u32(&buffer, 0xfeff2001); /* Version. */
    put_u32(&buffer, fx.buffer_count); /* Buffer count. */
    put_u32(&buffer, fx.numeric_variable_count); /* Numeric variable count. */
    put_u32(&buffer, fx.object_variable_count); /* Object variable count. */
    put_u32(&buffer, fx.shared_buffer_count);
    put_u32(&buffer, fx.shared_numeric_variable_count);
    put_u32(&buffer, fx.shared_object_count);
    put_u32(&buffer, fx.technique_count);
    size_offset = put_u32(&buffer, 0); /* Unstructured size. */
    put_u32(&buffer, fx.string_count);
    put_u32(&buffer, fx.texture_count);
    put_u32(&buffer, fx.depth_stencil_state_count);
    put_u32(&buffer, fx.blend_state_count);
    put_u32(&buffer, fx.rasterizer_state_count);
    put_u32(&buffer, fx.sampler_state_count);
    put_u32(&buffer, fx.rtv_count);
    put_u32(&buffer, fx.dsv_count);
    put_u32(&buffer, fx.shader_count);
    put_u32(&buffer, 0); /* Inline shader count. */
    put_u32(&buffer, fx.group_count); /* Group count. */
    put_u32(&buffer, fx.uav_count);
    put_u32(&buffer, 0); /* Interface variables count. */
    put_u32(&buffer, 0); /* Interface variable element count. */
    put_u32(&buffer, 0); /* Class instance elements count. */

    set_u32(&buffer, size_offset, fx.unstructured.size);

    bytecode_put_bytes(&buffer, fx.unstructured.data, fx.unstructured.size);
    bytecode_put_bytes_unaligned(&buffer, fx.structured.data, fx.structured.size);

    vkd3d_free(fx.unstructured.data);
    vkd3d_free(fx.structured.data);

    set_status(&fx, buffer.status);

    if (fx.status < 0)
        ctx->result = fx.status;

    if (!ctx->result)
    {
        out->code = buffer.data;
        out->size = buffer.size;
    }

    return fx_write_context_cleanup(&fx);
}

int hlsl_emit_effect_binary(struct hlsl_ctx *ctx, struct vkd3d_shader_code *out)
{
    if (ctx->profile->major_version == 2)
    {
        return hlsl_fx_2_write(ctx, out);
    }
    else if (ctx->profile->major_version == 4)
    {
        return hlsl_fx_4_write(ctx, out);
    }
    else if (ctx->profile->major_version == 5)
    {
        return hlsl_fx_5_write(ctx, out);
    }
    else
    {
        vkd3d_unreachable();
    }
}
