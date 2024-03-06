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

struct fx_write_context;

struct fx_write_context_ops
{
    uint32_t (*write_string)(const char *string, struct fx_write_context *fx);
    uint32_t (*write_type)(const struct hlsl_type *type, struct fx_write_context *fx);
    void (*write_technique)(struct hlsl_ir_var *var, struct fx_write_context *fx);
    void (*write_pass)(struct hlsl_ir_var *var, struct fx_write_context *fx);
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
    uint32_t numeric_variable_count;
    uint32_t object_variable_count;
    int status;

    const struct fx_write_context_ops *ops;
};

static void set_status(struct fx_write_context *fx, int status)
{
    if (fx->status < 0)
        return;
    if (status < 0)
        fx->status = status;
}

static uint32_t write_string(const char *string, struct fx_write_context *fx)
{
    return fx->ops->write_string(string, fx);
}

static void write_pass(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    fx->ops->write_pass(var, fx);
}

static uint32_t write_type(const struct hlsl_type *type, struct fx_write_context *fx)
{
    struct type_entry *type_entry;
    unsigned int elements_count;
    const char *name;

    if (type->class == HLSL_CLASS_ARRAY)
    {
        name = hlsl_get_multiarray_element_type(type)->name;
        elements_count = hlsl_get_multiarray_size(type);
    }
    else
    {
        name = type->name;
        elements_count = 0;
    }

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

    type_entry->offset = fx->ops->write_type(type, fx);
    type_entry->name = name;
    type_entry->elements_count = elements_count;

    list_add_tail(&fx->types, &type_entry->entry);

    return type_entry->offset;
}

static void fx_write_context_init(struct hlsl_ctx *ctx, const struct fx_write_context_ops *ops,
        struct fx_write_context *fx)
{
    unsigned int version = ctx->profile->major_version;

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
}

static int fx_write_context_cleanup(struct fx_write_context *fx)
{
    struct type_entry *type, *next_type;
    int status = fx->status;

    rb_destroy(&fx->strings, string_storage_destroy, NULL);

    LIST_FOR_EACH_ENTRY_SAFE(type, next_type, &fx->types, struct type_entry, entry)
    {
        list_remove(&type->entry);
        vkd3d_free(type);
    }

    return status;
}

static bool technique_matches_version(const struct hlsl_ir_var *var, const struct fx_write_context *fx)
{
    const struct hlsl_type *type = var->data_type;

    if (type->base_type != HLSL_TYPE_TECHNIQUE)
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
    uint32_t name_offset;

    name_offset = write_string(var->name, fx);
    put_u32(buffer, name_offset);
    put_u32(buffer, 0); /* Assignment count. */
    put_u32(buffer, 0); /* Annotation count. */

    /* TODO: annotations */
    /* TODO: assignments */
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
}

static uint32_t get_fx_4_type_size(const struct hlsl_type *type)
{
    uint32_t elements_count;

    elements_count = hlsl_get_multiarray_size(type);
    type = hlsl_get_multiarray_element_type(type);

    return type->reg_size[HLSL_REGSET_NUMERIC] * sizeof(float) * elements_count;
}

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
    static const uint32_t numeric_base_type[] =
    {
        [HLSL_TYPE_FLOAT] = 1,
        [HLSL_TYPE_INT  ] = 2,
        [HLSL_TYPE_UINT ] = 3,
        [HLSL_TYPE_BOOL ] = 4,
    };
    uint32_t value = 0;

    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            value |= numeric_type_class[type->class];
            break;
        default:
            FIXME("Unexpected type class %u.\n", type->class);
            set_status(fx, VKD3D_ERROR_NOT_IMPLEMENTED);
            return 0;
    }

    switch (type->base_type)
    {
        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
        case HLSL_TYPE_BOOL:
            value |= (numeric_base_type[type->base_type] << NUMERIC_BASE_TYPE_SHIFT);
            break;
        default:
            FIXME("Unexpected base type %u.\n", type->base_type);
            set_status(fx, VKD3D_ERROR_NOT_IMPLEMENTED);
            return 0;
    }

    value |= (type->dimy & 0x7) << NUMERIC_ROWS_SHIFT;
    value |= (type->dimx & 0x7) << NUMERIC_COLUMNS_SHIFT;
    if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
        value |= NUMERIC_COLUMN_MAJOR_MASK;

    return value;
}

static uint32_t write_fx_4_type(const struct hlsl_type *type, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    uint32_t name_offset, offset, size, stride, numeric_desc;
    uint32_t elements_count = 0;
    const char *name;
    static const uint32_t variable_type[] =
    {
        [HLSL_CLASS_SCALAR] = 1,
        [HLSL_CLASS_VECTOR] = 1,
        [HLSL_CLASS_MATRIX] = 1,
        [HLSL_CLASS_OBJECT] = 2,
        [HLSL_CLASS_STRUCT] = 3,
    };
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

    /* Resolve arrays to element type and number of elements. */
    if (type->class == HLSL_CLASS_ARRAY)
    {
        elements_count = hlsl_get_multiarray_size(type);
        type = hlsl_get_multiarray_element_type(type);
    }

    if (type->base_type == HLSL_TYPE_TEXTURE)
        name = texture_type_names[type->sampler_dim];
    else if (type->base_type == HLSL_TYPE_UAV)
        name = uav_type_names[type->sampler_dim];
    else
        name = type->name;

    name_offset = write_string(name, fx);
    offset = put_u32_unaligned(buffer, name_offset);

    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
        case HLSL_CLASS_OBJECT:
        case HLSL_CLASS_STRUCT:
            put_u32_unaligned(buffer, variable_type[type->class]);
            break;
        default:
            FIXME("Writing type class %u is not implemented.\n", type->class);
            set_status(fx, VKD3D_ERROR_NOT_IMPLEMENTED);
            return 0;
    }

    size = stride = type->reg_size[HLSL_REGSET_NUMERIC] * sizeof(float);
    if (elements_count)
        size *= elements_count;
    stride = align(stride, 4 * sizeof(float));

    put_u32_unaligned(buffer, elements_count);
    put_u32_unaligned(buffer, size); /* Total size. */
    put_u32_unaligned(buffer, stride); /* Stride. */
    put_u32_unaligned(buffer, size);

    if (type->class == HLSL_CLASS_STRUCT)
    {
        size_t i;

        put_u32_unaligned(buffer, type->e.record.field_count);
        for (i = 0; i < type->e.record.field_count; ++i)
        {
            const struct hlsl_struct_field *field = &type->e.record.fields[i];
            uint32_t semantic_offset, field_type_offset;

            name_offset = write_string(field->name, fx);
            semantic_offset = write_string(field->semantic.name, fx);
            field_type_offset = write_type(field->type, fx);

            put_u32_unaligned(buffer, name_offset);
            put_u32_unaligned(buffer, semantic_offset);
            put_u32_unaligned(buffer, field->reg_offset[HLSL_REGSET_NUMERIC]);
            put_u32_unaligned(buffer, field_type_offset);
        }
    }
    else if (type->class == HLSL_CLASS_OBJECT)
    {
        static const uint32_t object_type[] =
        {
            [HLSL_TYPE_RENDERTARGETVIEW] = 19,
            [HLSL_TYPE_DEPTHSTENCILVIEW] = 20,
        };
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

        switch (type->base_type)
        {
            case HLSL_TYPE_DEPTHSTENCILVIEW:
            case HLSL_TYPE_RENDERTARGETVIEW:
                put_u32_unaligned(buffer, object_type[type->base_type]);
                break;
            case HLSL_TYPE_TEXTURE:
                put_u32_unaligned(buffer, texture_type[type->sampler_dim]);
                break;
            case HLSL_TYPE_UAV:
                put_u32_unaligned(buffer, uav_type[type->sampler_dim]);
                break;
            default:
                FIXME("Object type %u is not supported.\n", type->base_type);
                set_status(fx, VKD3D_ERROR_NOT_IMPLEMENTED);
                return 0;
        }
    }
    else /* Numeric type */
    {
        numeric_desc = get_fx_4_numeric_type_description(type, fx);
        put_u32_unaligned(buffer, numeric_desc);
    }

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
    put_u32(buffer, 0); /* Annotation count. */

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
    put_u32(buffer, 0); /* Annotation count */

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

        if (type->base_type == HLSL_TYPE_EFFECT_GROUP)
            write_group(var, fx);
    }
}

static uint32_t write_fx_2_string(const char *string, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    const char *s = string ? string : "";
    uint32_t size, offset;

    size = strlen(s) + 1;
    offset = put_u32(buffer, size);
    bytecode_put_bytes(buffer, s, size);
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

static const struct fx_write_context_ops fx_2_ops =
{
    .write_string = write_fx_2_string,
    .write_technique = write_fx_2_technique,
    .write_pass = write_fx_2_pass,
};

static int hlsl_fx_2_write(struct hlsl_ctx *ctx, struct vkd3d_shader_code *out)
{
    struct vkd3d_bytecode_buffer buffer = { 0 };
    struct vkd3d_bytecode_buffer *structured;
    uint32_t offset, size, technique_count;
    struct fx_write_context fx;

    fx_write_context_init(ctx, &fx_2_ops, &fx);
    structured = &fx.structured;

    /* First entry is always zeroed and skipped. */
    put_u32(&fx.unstructured, 0);

    put_u32(&buffer, 0xfeff0901); /* Version. */
    offset = put_u32(&buffer, 0);

    put_u32(structured, 0); /* Parameter count */
    technique_count = put_u32(structured, 0);
    put_u32(structured, 0); /* Unknown */
    put_u32(structured, 0); /* Object count */

    /* TODO: parameters */

    write_techniques(ctx->globals, &fx);
    set_u32(structured, technique_count, fx.technique_count);

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

    if (!fx.status)
    {
        out->code = buffer.data;
        out->size = buffer.size;
    }

    if (fx.status < 0)
        ctx->result = fx.status;

    return fx_write_context_cleanup(&fx);
}

static const struct fx_write_context_ops fx_4_ops =
{
    .write_string = write_fx_4_string,
    .write_type = write_fx_4_type,
    .write_technique = write_fx_4_technique,
    .write_pass = write_fx_4_pass,
};

static void write_fx_4_numeric_variable(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t semantic_offset, flags = 0;
    uint32_t name_offset, type_offset;
    enum fx_4_numeric_variable_flags
    {
        HAS_EXPLICIT_BIND_POINT = 0x4,
    };

    /* Explicit bind point. */
    if (var->reg_reservation.reg_type)
        flags |= HAS_EXPLICIT_BIND_POINT;

    type_offset = write_type(var->data_type, fx);
    name_offset = write_string(var->name, fx);
    semantic_offset = write_string(var->semantic.name, fx);

    put_u32(buffer, name_offset);
    put_u32(buffer, type_offset);

    semantic_offset = put_u32(buffer, semantic_offset); /* Semantic */
    put_u32(buffer, var->buffer_offset); /* Offset in the constant buffer */
    put_u32(buffer, 0); /* FIXME: default value offset */
    put_u32(buffer, flags); /* Flags */

    put_u32(buffer, 0); /* Annotations count */
    /* FIXME: write annotations */
}

static void write_fx_4_object_variable(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t semantic_offset, bind_point = ~0u;
    uint32_t name_offset, type_offset;

    if (var->reg_reservation.reg_type)
        bind_point = var->reg_reservation.reg_index;

    type_offset = write_type(var->data_type, fx);
    name_offset = write_string(var->name, fx);
    semantic_offset = write_string(var->semantic.name, fx);

    put_u32(buffer, name_offset);
    put_u32(buffer, type_offset);

    semantic_offset = put_u32(buffer, semantic_offset); /* Semantic */
    put_u32(buffer, bind_point); /* Explicit bind point */

    put_u32(buffer, 0); /* Annotations count */
    /* FIXME: write annotations */
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

    if (b->reservation.reg_type)
        bind_point = b->reservation.reg_index;
    if (b->type == HLSL_BUFFER_TEXTURE)
        flags |= IS_TBUFFER;
    /* FIXME: set 'single' flag for fx_5_0 */

    name_offset = write_string(b->name, fx);

    put_u32(buffer, name_offset); /* Name */
    size_offset = put_u32(buffer, 0); /* Data size */
    put_u32(buffer, flags); /* Flags */
    count_offset = put_u32(buffer, 0);
    put_u32(buffer, bind_point); /* Bind point */

    put_u32(buffer, 0); /* Annotations count */
    /* FIXME: write annotations */

    count = 0;
    size = 0;
    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->buffer != b)
            continue;

        write_fx_4_numeric_variable(var, fx);
        size += get_fx_4_type_size(var->data_type);
        ++count;
    }

    set_u32(buffer, count_offset, count);
    set_u32(buffer, size_offset, align(size, 16));

    fx->numeric_variable_count += count;
}

static void write_buffers(struct fx_write_context *fx)
{
    struct hlsl_buffer *buffer;
    struct hlsl_block block;

    hlsl_block_init(&block);
    hlsl_prepend_global_uniform_copy(fx->ctx, &block);
    hlsl_block_init(&block);
    hlsl_calculate_buffer_offsets(fx->ctx);

    LIST_FOR_EACH_ENTRY(buffer, &fx->ctx->buffers, struct hlsl_buffer, entry)
    {
        if (!buffer->size)
            continue;

        write_fx_4_buffer(buffer, fx);
        ++fx->buffer_count;
    }
}

static bool is_object_variable(const struct hlsl_ir_var *var)
{
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);

    if (type->class != HLSL_CLASS_OBJECT)
        return false;

    switch (type->base_type)
    {
        case HLSL_TYPE_SAMPLER:
        case HLSL_TYPE_TEXTURE:
        case HLSL_TYPE_UAV:
        case HLSL_TYPE_PIXELSHADER:
        case HLSL_TYPE_VERTEXSHADER:
        case HLSL_TYPE_RENDERTARGETVIEW:
            return true;
        default:
            return false;
    }
}

static void write_objects(struct fx_write_context *fx)
{
    struct hlsl_ir_var *var;
    uint32_t count = 0;

    LIST_FOR_EACH_ENTRY(var, &fx->ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!is_object_variable(var))
            continue;

        write_fx_4_object_variable(var, fx);
        ++count;
    }

    fx->object_variable_count += count;
}

static int hlsl_fx_4_write(struct hlsl_ctx *ctx, struct vkd3d_shader_code *out)
{
    struct vkd3d_bytecode_buffer buffer = { 0 };
    struct fx_write_context fx;
    uint32_t size_offset;

    fx_write_context_init(ctx, &fx_4_ops, &fx);

    put_u32(&fx.unstructured, 0); /* Empty string placeholder. */

    write_buffers(&fx);
    write_objects(&fx);
    /* TODO: shared buffers */
    /* TODO: shared objects */

    write_techniques(ctx->globals, &fx);

    put_u32(&buffer, ctx->profile->minor_version == 0 ? 0xfeff1001 : 0xfeff1011); /* Version. */
    put_u32(&buffer, fx.buffer_count); /* Buffer count. */
    put_u32(&buffer, fx.numeric_variable_count); /* Numeric variable count. */
    put_u32(&buffer, fx.object_variable_count); /* Object variable count. */
    put_u32(&buffer, 0); /* Pool buffer count. */
    put_u32(&buffer, 0); /* Pool variable count. */
    put_u32(&buffer, 0); /* Pool object count. */
    put_u32(&buffer, fx.technique_count);
    size_offset = put_u32(&buffer, 0); /* Unstructured size. */
    put_u32(&buffer, 0); /* String count. */
    put_u32(&buffer, 0); /* Texture object count. */
    put_u32(&buffer, 0); /* Depth stencil state count. */
    put_u32(&buffer, 0); /* Blend state count. */
    put_u32(&buffer, 0); /* Rasterizer state count. */
    put_u32(&buffer, 0); /* Sampler state count. */
    put_u32(&buffer, 0); /* Rendertarget view count. */
    put_u32(&buffer, 0); /* Depth stencil view count. */
    put_u32(&buffer, 0); /* Shader count. */
    put_u32(&buffer, 0); /* Inline shader count. */

    set_u32(&buffer, size_offset, fx.unstructured.size);

    bytecode_put_bytes(&buffer, fx.unstructured.data, fx.unstructured.size);
    bytecode_put_bytes_unaligned(&buffer, fx.structured.data, fx.structured.size);

    vkd3d_free(fx.unstructured.data);
    vkd3d_free(fx.structured.data);

    set_status(&fx, buffer.status);

    if (!fx.status)
    {
        out->code = buffer.data;
        out->size = buffer.size;
    }

    if (fx.status < 0)
        ctx->result = fx.status;

    return fx_write_context_cleanup(&fx);
}

static int hlsl_fx_5_write(struct hlsl_ctx *ctx, struct vkd3d_shader_code *out)
{
    struct vkd3d_bytecode_buffer buffer = { 0 };
    struct fx_write_context fx;
    uint32_t size_offset;

    fx_write_context_init(ctx, &fx_4_ops, &fx);

    put_u32(&fx.unstructured, 0); /* Empty string placeholder. */

    write_buffers(&fx);
    write_objects(&fx);
    /* TODO: interface variables */

    write_groups(&fx);

    put_u32(&buffer, 0xfeff2001); /* Version. */
    put_u32(&buffer, fx.buffer_count); /* Buffer count. */
    put_u32(&buffer, fx.numeric_variable_count); /* Numeric variable count. */
    put_u32(&buffer, fx.object_variable_count); /* Object variable count. */
    put_u32(&buffer, 0); /* Pool buffer count. */
    put_u32(&buffer, 0); /* Pool variable count. */
    put_u32(&buffer, 0); /* Pool object count. */
    put_u32(&buffer, fx.technique_count);
    size_offset = put_u32(&buffer, 0); /* Unstructured size. */
    put_u32(&buffer, 0); /* String count. */
    put_u32(&buffer, 0); /* Texture object count. */
    put_u32(&buffer, 0); /* Depth stencil state count. */
    put_u32(&buffer, 0); /* Blend state count. */
    put_u32(&buffer, 0); /* Rasterizer state count. */
    put_u32(&buffer, 0); /* Sampler state count. */
    put_u32(&buffer, 0); /* Rendertarget view count. */
    put_u32(&buffer, 0); /* Depth stencil view count. */
    put_u32(&buffer, 0); /* Shader count. */
    put_u32(&buffer, 0); /* Inline shader count. */
    put_u32(&buffer, fx.group_count); /* Group count. */
    put_u32(&buffer, 0); /* UAV count. */
    put_u32(&buffer, 0); /* Interface variables count. */
    put_u32(&buffer, 0); /* Interface variable element count. */
    put_u32(&buffer, 0); /* Class instance elements count. */

    set_u32(&buffer, size_offset, fx.unstructured.size);

    bytecode_put_bytes(&buffer, fx.unstructured.data, fx.unstructured.size);
    bytecode_put_bytes_unaligned(&buffer, fx.structured.data, fx.structured.size);

    vkd3d_free(fx.unstructured.data);
    vkd3d_free(fx.structured.data);

    set_status(&fx, buffer.status);

    if (!fx.status)
    {
        out->code = buffer.data;
        out->size = buffer.size;
    }

    if (fx.status < 0)
        ctx->result = fx.status;

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
