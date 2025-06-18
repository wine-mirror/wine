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

enum fx_2_type_constants
{
    /* Assignment types */
    FX_2_ASSIGNMENT_CODE_BLOB = 0x0,
    FX_2_ASSIGNMENT_PARAMETER = 0x1,
    FX_2_ASSIGNMENT_ARRAY_SELECTOR = 0x2,
};

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
    FX_GEOMETRYSHADER,
    FX_COMPONENT_TYPE_COUNT,
};

struct rhs_named_value
{
    const char *name;
    unsigned int value;
};

struct fx_assignment
{
    uint32_t id;
    uint32_t lhs_index;
    uint32_t type;
    uint32_t value;
};

struct fx_4_binary_type
{
    uint32_t name;
    uint32_t class;
    uint32_t element_count;
    uint32_t unpacked_size;
    uint32_t stride;
    uint32_t packed_size;
    uint32_t typeinfo;
};

struct fx_5_shader
{
    uint32_t offset;
    uint32_t sodecl[4];
    uint32_t sodecl_count;
    uint32_t rast_stream;
    uint32_t iface_bindings_count;
    uint32_t iface_bindings;
};

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
    uint32_t modifiers;
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
    struct vkd3d_bytecode_buffer objects;

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

static void fx_print_string(struct vkd3d_string_buffer *buffer, const char *prefix,
        const char *s, size_t len)
{
    if (len)
        --len; /* Trim terminating null. */
    vkd3d_string_buffer_printf(buffer, "%s", prefix);
    vkd3d_string_buffer_print_string_escaped(buffer, s, len);
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
    unsigned int elements_count, modifiers;
    const struct hlsl_type *element_type;
    struct type_entry *type_entry;
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
    modifiers = element_type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK;

    LIST_FOR_EACH_ENTRY(type_entry, &fx->types, struct type_entry, entry)
    {
        if (strcmp(type_entry->name, name))
            continue;

        if (type_entry->elements_count != elements_count)
            continue;

        if (type_entry->modifiers != modifiers)
            continue;

        return type_entry->offset;
    }

    if (!(type_entry = hlsl_alloc(fx->ctx, sizeof(*type_entry))))
        return 0;

    type_entry->offset = write_fx_4_type(type, fx);
    type_entry->name = name;
    type_entry->elements_count = elements_count;
    type_entry->modifiers = modifiers;

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

static void write_fx_2_annotations(struct hlsl_ir_var *var, uint32_t count_offset, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t count;

    count = write_annotations(var->annotations, fx);
    set_u32(buffer, count_offset, count);
}

static const struct rhs_named_value fx_2_zenable_values[] =
{
    { "USEW", 2 },
    { NULL }
};

static const struct rhs_named_value fx_2_fillmode_values[] =
{
    { "POINT", 1 },
    { "WIREFRAME", 2 },
    { "SOLID", 3 },
    { NULL },
};

static const struct rhs_named_value fx_2_shademode_values[] =
{
    { "FLAT", 1 },
    { "GOURAUD", 2 },
    { "PHONG", 3 },
    { NULL }
};

static const struct rhs_named_value fx_2_blendmode_values[] =
{
    { "ZERO", 1 },
    { "ONE", 2 },
    { "SRCCOLOR", 3 },
    { "INVSRCCOLOR", 4 },
    { "SRCALPHA", 5 },
    { "INVSRCALPHA", 6 },
    { "DESTALPHA", 7 },
    { "INVDESTALPHA", 8 },
    { "DESTCOLOR", 9 },
    { "INVDESTCOLOR", 10 },
    { "SRCALPHASAT", 11 },
    { "BOTHSRCALPHA", 12 },
    { "BOTHINVSRCALPHA", 13 },
    { "BLENDFACTOR", 14 },
    { "INVBLENDFACTOR", 15 },
    { "SRCCOLOR2", 16 },
    { "INVSRCCOLOR2", 17 },
    { NULL }
};

static const struct rhs_named_value fx_2_cullmode_values[] =
{
    { "NONE", 1 },
    { "CW", 2 },
    { "CCW", 3 },
    { NULL }
};

static const struct rhs_named_value fx_2_cmpfunc_values[] =
{
    { "NEVER", 1 },
    { "LESS", 2 },
    { "EQUAL", 3 },
    { "LESSEQUAL", 4 },
    { "GREATER", 5 },
    { "NOTEQUAL", 6 },
    { "GREATEREQUAL", 7 },
    { "ALWAYS", 8 },
    { NULL }
};

static const struct rhs_named_value fx_2_fogmode_values[] =
{
    { "NONE", 0 },
    { "EXP", 1 },
    { "EXP2", 2 },
    { "LINEAR", 3 },
    { NULL }
};

static const struct rhs_named_value fx_2_stencilcaps_values[] =
{
    { "KEEP", 0x1 },
    { "ZERO", 0x2 },
    { "REPLACE", 0x4 },
    { "INCRSAT", 0x8 },
    { "DECRSAT", 0x10 },
    { "INVERT", 0x20 },
    { "INCR", 0x40 },
    { "DECR", 0x80 },
    { "TWOSIDED", 0x100 },
    { NULL }
};

static const struct rhs_named_value fx_2_wrap_values[] =
{
    { "COORD_0", 0x1 },
    { "COORD_1", 0x2 },
    { "COORD_2", 0x4 },
    { "COORD_3", 0x8 },
    { "U", 0x1 },
    { "V", 0x2 },
    { "W", 0x4 },
    { NULL }
};

static const struct rhs_named_value fx_2_materialcolorsource_values[] =
{
    { "MATERIAL", 0 },
    { "COORD1", 1 },
    { "COORD2", 2 },
    { NULL }
};

static const struct rhs_named_value fx_2_vertexblend_values[] =
{
    { "DISABLE", 0 },
    { "1WEIGHTS", 1 },
    { "2WEIGHTS", 2 },
    { "3WEIGHTS", 3 },
    { "TWEENING", 255 },
    { "0WEIGHTS", 256 },
    { NULL }
};

static const struct rhs_named_value fx_2_clipplane_values[] =
{
    { "CLIPPLANE0", 0x1 },
    { "CLIPPLANE1", 0x2 },
    { "CLIPPLANE2", 0x4 },
    { "CLIPPLANE3", 0x8 },
    { "CLIPPLANE4", 0x10 },
    { "CLIPPLANE5", 0x20 },
    { NULL }
};

static const struct rhs_named_value fx_2_patchedgestyle_values[] =
{
    { "DISCRETE", 0 },
    { "CONTINUOUS", 1 },
    { NULL }
};

static const struct rhs_named_value fx_2_colorwriteenable_values[] =
{
    { "RED", 0x1 },
    { "GREEN", 0x2 },
    { "BLUE", 0x4 },
    { "ALPHA", 0x8 },
    { NULL }
};

static const struct rhs_named_value fx_2_blendop_values[] =
{
    { "ADD", 1 },
    { "SUBTRACT", 2 },
    { "REVSUBTRACT", 3 },
    { "MIN", 4 },
    { "MAX", 5 },
    { NULL }
};

static const struct rhs_named_value fx_2_degree_values[] =
{
    { "LINEAR", 1 },
    { "QUADRATIC", 2 },
    { "CUBIC", 3 },
    { "QUINTIC", 4 },
    { NULL }
};

static const struct rhs_named_value fx_2_textureop_values[] =
{
    { "DISABLE", 1 },
    { "SELECTARG1", 2 },
    { "SELECTARG2", 3 },
    { "MODULATE", 4 },
    { "MODULATE2X", 5 },
    { "MODULATE4X", 6 },
    { "ADD", 7 },
    { "ADDSIGNED", 8 },
    { "ADDSIGNED2X", 9 },
    { "SUBTRACT", 10 },
    { "ADDSMOOTH", 11 },
    { "BLENDDIFFUSEALPHA", 12 },
    { "BLENDTEXTUREALPHA", 13 },
    { "BLENDFACTORALPHA", 14 },
    { "BLENDTEXTUREALPHAPM", 15 },
    { "BLENDCURRENTALPHA", 16 },
    { "PREMODULATE", 17 },
    { "MODULATEALPHA_ADDCOLOR", 18 },
    { "MODULATECOLOR_ADDALPHA", 19 },
    { "MODULATEINVALPHA_ADDCOLOR", 20 },
    { "MODULATEINVCOLOR_ADDALPHA", 21 },
    { "BUMPENVMAP", 22 },
    { "BUMPENVMAPLUMINANCE", 23 },
    { "DOTPRODUCT3", 24 },
    { "MULTIPLYADD", 25 },
    { "LERP", 26 },
    { NULL }
};

static const struct rhs_named_value fx_2_colorarg_values[] =
{
    { "DIFFUSE", 0x0 },
    { "CURRENT", 0x1 },
    { "TEXTURE", 0x2 },
    { "TFACTOR", 0x3 },
    { "SPECULAR", 0x4 },
    { "TEMP", 0x5 },
    { "CONSTANT", 0x6 },
    { "COMPLEMENT", 0x10 },
    { "ALPHAREPLICATE", 0x20 },
    { NULL }
};

static const struct rhs_named_value fx_2_texturetransform_values[] =
{
    { "DISABLE", 0 },
    { "COUNT1", 1 },
    { "COUNT2", 2 },
    { "COUNT3", 3 },
    { "COUNT4", 4 },
    { "PROJECTED", 256 },
    { NULL }
};

static const struct rhs_named_value fx_2_lighttype_values[] =
{
    { "POINT", 1 },
    { "SPOT", 2 },
    { "DIRECTIONAL", 3 },
    { NULL }
};

static const struct rhs_named_value fx_2_address_values[] =
{
    { "WRAP", 1 },
    { "MIRROR", 2 },
    { "CLAMP", 3 },
    { "BORDER", 4 },
    { "MIRROR_ONCE", 5 },
    { NULL }
};

static const struct rhs_named_value fx_2_filter_values[] =
{
    { "NONE", 0 },
    { "POINT", 1 },
    { "LINEAR", 2 },
    { "ANISOTROPIC", 3 },
    { "PYRAMIDALQUAD", 6 },
    { "GAUSSIANQUAD", 7 },
    { "CONVOLUTIONMONO", 8 },
    { NULL }
};

static const struct fx_2_state
{
    const char *name;
    enum hlsl_type_class class;
    enum state_property_component_type type;
    unsigned int dimx;
    uint32_t array_size;
    uint32_t id;
    const struct rhs_named_value *values;
}
fx_2_states[] =
{
    { "ZEnable",          HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 0, fx_2_zenable_values },
    { "FillMode",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 1, fx_2_fillmode_values },
    { "ShadeMode",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 2, fx_2_shademode_values },
    { "ZWriteEnable",     HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 3 },
    { "AlphaTestEnable",  HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 4 },
    { "LastPixel",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 5 },
    { "SrcBlend",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 6, fx_2_blendmode_values },
    { "DestBlend",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 7, fx_2_blendmode_values },
    { "CullMode",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 8, fx_2_cullmode_values },
    { "ZFunc",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 9, fx_2_cmpfunc_values },
    { "AlphaRef",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 10 },
    { "AlphaFunc",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 11, fx_2_cmpfunc_values },
    { "DitherEnable",     HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 12 },
    { "AlphaBlendEnable", HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 13 },
    { "FogEnable",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 14 },
    { "SpecularEnable",   HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 15 },
    { "FogColor",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 16 },
    { "FogTableMode",     HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 17, fx_2_fogmode_values },
    { "FogStart",         HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 18 },
    { "FogEnd",           HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 19 },
    { "FogDensity",       HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 20 },
    { "RangeFogEnable",   HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 21 },
    { "StencilEnable",    HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 22 },
    { "StencilFail",      HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 23, fx_2_stencilcaps_values },
    { "StencilZFail",     HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 24, fx_2_stencilcaps_values },
    { "StencilPass",      HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 25, fx_2_stencilcaps_values },
    { "StencilFunc",      HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 26, fx_2_cmpfunc_values },
    { "StencilRef",       HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 27 },
    { "StencilMask",      HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 28 },
    { "StencilWriteMask", HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 29 },
    { "TextureFactor",    HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 30 },
    { "Wrap0",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 31, fx_2_wrap_values },
    { "Wrap1",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 32, fx_2_wrap_values },
    { "Wrap2",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 33, fx_2_wrap_values },
    { "Wrap3",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 34, fx_2_wrap_values },
    { "Wrap4",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 35, fx_2_wrap_values },
    { "Wrap5",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 36, fx_2_wrap_values },
    { "Wrap6",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 37, fx_2_wrap_values },
    { "Wrap7",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 38, fx_2_wrap_values },
    { "Wrap8",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 39, fx_2_wrap_values },
    { "Wrap9",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 40, fx_2_wrap_values },
    { "Wrap10",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 41, fx_2_wrap_values },
    { "Wrap11",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 42, fx_2_wrap_values },
    { "Wrap12",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 43, fx_2_wrap_values },
    { "Wrap13",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 44, fx_2_wrap_values },
    { "Wrap14",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 45, fx_2_wrap_values },
    { "Wrap15",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 46, fx_2_wrap_values },
    { "Clipping",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 47 },
    { "Lighting",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 48 },
    { "Ambient",          HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 49 },
    { "FogVertexMode",    HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 50, fx_2_fogmode_values },
    { "ColorVertex",      HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 51 },
    { "LocalViewer",      HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 52 },
    { "NormalizeNormals", HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 53 },

    { "DiffuseMaterialSource",  HLSL_CLASS_SCALAR, FX_UINT, 1, 1, 54, fx_2_materialcolorsource_values },
    { "SpecularMaterialSource", HLSL_CLASS_SCALAR, FX_UINT, 1, 1, 55, fx_2_materialcolorsource_values },
    { "AmbientMaterialSource",  HLSL_CLASS_SCALAR, FX_UINT, 1, 1, 56, fx_2_materialcolorsource_values },
    { "EmissiveMaterialSource", HLSL_CLASS_SCALAR, FX_UINT, 1, 1, 57, fx_2_materialcolorsource_values },

    { "VertexBlend",       HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 58, fx_2_vertexblend_values },
    { "ClipPlaneEnable",   HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 59, fx_2_clipplane_values },
    { "PointSize",         HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 60 },
    { "PointSize_Min",     HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 61 },
    { "PointSize_Max",     HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 62 },
    { "PointSpriteEnable", HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 63 },
    { "PointScaleEnable",  HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 64 },
    { "PointScale_A",      HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 65 },
    { "PointScale_B",      HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 66 },
    { "PointScale_C",      HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 67 },

    { "MultiSampleAntialias",     HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 68 },
    { "MultiSampleMask",          HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 69 },
    { "PatchEdgeStyle",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 70, fx_2_patchedgestyle_values },
    { "DebugMonitorToken",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 71 },
    { "IndexedVertexBlendEnable", HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 72 },
    { "ColorWriteEnable",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 73, fx_2_colorwriteenable_values },
    { "TweenFactor",              HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 74 },
    { "BlendOp",                  HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 75, fx_2_blendop_values },
    { "PositionDegree",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 76, fx_2_degree_values },
    { "NormalDegree",             HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 77, fx_2_degree_values },
    { "ScissorTestEnable",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 78 },
    { "SlopeScaleDepthBias",      HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 79 },

    { "AntialiasedLineEnable",     HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 80 },
    { "MinTessellationLevel",      HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 81 },
    { "MaxTessellationLevel",      HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 82 },
    { "AdaptiveTess_X",            HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 83 },
    { "AdaptiveTess_Y",            HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 84 },
    { "AdaptiveTess_Z",            HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 85 },
    { "AdaptiveTess_W",            HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 86 },
    { "EnableAdaptiveTesselation", HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 87 },
    { "TwoSidedStencilMode",       HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 88 },
    { "StencilFail",               HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 89, fx_2_stencilcaps_values },
    { "StencilZFail",              HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 90, fx_2_stencilcaps_values },
    { "StencilPass",               HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 91, fx_2_stencilcaps_values },
    { "StencilFunc",               HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 92, fx_2_cmpfunc_values },

    { "ColorWriteEnable1",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 93, fx_2_colorwriteenable_values },
    { "ColorWriteEnable2",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 94, fx_2_colorwriteenable_values },
    { "ColorWriteEnable3",        HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 95, fx_2_colorwriteenable_values },
    { "BlendFactor",              HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 96 },
    { "SRGBWriteEnable",          HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 97 },
    { "DepthBias",                HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 98 },
    { "SeparateAlphaBlendEnable", HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 99 },
    { "SrcBlendAlpha",            HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 100, fx_2_blendmode_values },
    { "DestBlendAlpha",           HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 101, fx_2_blendmode_values },
    { "BlendOpAlpha",             HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 102, fx_2_blendmode_values },

    { "ColorOp",               HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 103, fx_2_textureop_values },
    { "ColorArg0",             HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 104, fx_2_colorarg_values },
    { "ColorArg1",             HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 105, fx_2_colorarg_values },
    { "ColorArg2",             HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 106, fx_2_colorarg_values },
    { "AlphaOp",               HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 107, fx_2_textureop_values },
    { "AlphaArg0",             HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 108, fx_2_colorarg_values },
    { "AlphaArg1",             HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 109, fx_2_colorarg_values },
    { "AlphaArg2",             HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 110, fx_2_colorarg_values },
    { "ResultArg",             HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 111, fx_2_colorarg_values },
    { "BumpEnvMat00",          HLSL_CLASS_SCALAR, FX_FLOAT, 1, 8, 112 },
    { "BumpEnvMat01",          HLSL_CLASS_SCALAR, FX_FLOAT, 1, 8, 113 },
    { "BumpEnvMat10",          HLSL_CLASS_SCALAR, FX_FLOAT, 1, 8, 114 },
    { "BumpEnvMat11",          HLSL_CLASS_SCALAR, FX_FLOAT, 1, 8, 115 },
    { "TextCoordIndex",        HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 116 },
    { "BumpEnvLScale",         HLSL_CLASS_SCALAR, FX_FLOAT, 1, 8, 117 },
    { "BumpEnvLOffset",        HLSL_CLASS_SCALAR, FX_FLOAT, 1, 8, 118 },
    { "TextureTransformFlags", HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 119, fx_2_texturetransform_values },
    { "Constant",              HLSL_CLASS_SCALAR, FX_UINT, 1, 8, 120 },
    { "NPatchMode",            HLSL_CLASS_SCALAR, FX_UINT, 1, 1, 121 },
    { "FVF",                   HLSL_CLASS_SCALAR, FX_UINT, 1, 1, 122 },

    { "ProjectionTransform", HLSL_CLASS_MATRIX, FX_FLOAT, 4, 1, 123 },
    { "ViewTransform",       HLSL_CLASS_MATRIX, FX_FLOAT, 4, 1, 124 },
    { "WorldTransform",      HLSL_CLASS_MATRIX, FX_FLOAT, 4, 1, 125 },
    { "TextureTransform",    HLSL_CLASS_MATRIX, FX_FLOAT, 4, 8, 126 },

    { "MaterialAmbient",   HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 127 },
    { "MaterialDiffuse",   HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 128 },
    { "MaterialSpecular",  HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 129 },
    { "MaterialEmissive",  HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 130 },
    { "MaterialPower",     HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 131 },

    { "LightType",         HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 132, fx_2_lighttype_values },
    { "LightDiffuse",      HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 133 },
    { "LightSpecular",     HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 134 },
    { "LightAmbient",      HLSL_CLASS_VECTOR, FX_FLOAT, 4, 1, 135 },
    { "LightPosition",     HLSL_CLASS_VECTOR, FX_FLOAT, 3, 1, 136 },
    { "LightDirection",    HLSL_CLASS_VECTOR, FX_FLOAT, 3, 1, 137 },
    { "LightRange",        HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 138 },
    { "LightFalloff",      HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 139 },
    { "LightAttenuation0", HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 140 },
    { "LightAttenuation1", HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 141 },
    { "LightAttenuation2", HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 142 },
    { "LightTheta",        HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 143 },
    { "LightPhi",          HLSL_CLASS_SCALAR, FX_FLOAT, 1, 1, 144 },
    { "LightEnable",       HLSL_CLASS_SCALAR, FX_FLOAT, 1, 8, 145 },

    { "VertexShader",      HLSL_CLASS_SCALAR, FX_VERTEXSHADER, 1, 1, 146 },
    { "PixelShader",       HLSL_CLASS_SCALAR, FX_PIXELSHADER,  1, 1, 147 },

    { "VertexShaderConstantF", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 148 },
    { "VertexShaderConstantB", HLSL_CLASS_SCALAR, FX_BOOL,  1, ~0u-1, 149 },
    { "VertexShaderConstantI", HLSL_CLASS_SCALAR, FX_UINT,  1, ~0u-1, 150 },
    { "VertexShaderConstant",  HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 151 },
    { "VertexShaderConstant1", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 152 },
    { "VertexShaderConstant2", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 153 },
    { "VertexShaderConstant3", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 154 },
    { "VertexShaderConstant4", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 155 },

    { "PixelShaderConstantF", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 156 },
    { "PixelShaderConstantB", HLSL_CLASS_SCALAR, FX_BOOL,  1, ~0u-1, 157 },
    { "PixelShaderConstantI", HLSL_CLASS_SCALAR, FX_UINT,  1, ~0u-1, 158 },
    { "PixelShaderConstant",  HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 159 },
    { "PixelShaderConstant1", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 160 },
    { "PixelShaderConstant2", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 161 },
    { "PixelShaderConstant3", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 162 },
    { "PixelShaderConstant4", HLSL_CLASS_SCALAR, FX_FLOAT, 1, ~0u-1, 163 },

    { "Texture",           HLSL_CLASS_SCALAR, FX_TEXTURE, 1, 1, 164 },
    { "AddressU",          HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 165, fx_2_address_values },
    { "AddressV",          HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 166, fx_2_address_values },
    { "AddressW",          HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 167, fx_2_address_values },
    { "BorderColor",       HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 168 },
    { "MagFilter",         HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 169, fx_2_filter_values },
    { "MinFilter",         HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 170, fx_2_filter_values },
    { "MipFilter",         HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 171, fx_2_filter_values },
    { "MipMapLodBias",     HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 172 },
    { "MaxMipLevel",       HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 173 },
    { "MaxAnisotropy",     HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 174 },
    { "SRBTexture",        HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 175 },
    { "ElementIndex",      HLSL_CLASS_SCALAR, FX_UINT,    1, 1, 176 },
};

static void write_fx_2_pass(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t name_offset, annotation_count_offset;

    name_offset = write_string(var->name, fx);
    put_u32(buffer, name_offset);
    annotation_count_offset = put_u32(buffer, 0);
    put_u32(buffer, 0); /* Assignment count. */

    write_fx_2_annotations(var, annotation_count_offset, fx);
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

enum fx_4_type_constants
{
    /* Numeric types encoding */
    FX_4_NUMERIC_TYPE_FLOAT = 1,
    FX_4_NUMERIC_TYPE_INT   = 2,
    FX_4_NUMERIC_TYPE_UINT  = 3,
    FX_4_NUMERIC_TYPE_BOOL  = 4,

    FX_4_NUMERIC_CLASS_SCALAR = 1,
    FX_4_NUMERIC_CLASS_VECTOR = 2,
    FX_4_NUMERIC_CLASS_MATRIX = 3,

    FX_4_NUMERIC_BASE_TYPE_SHIFT = 3,
    FX_4_NUMERIC_ROWS_SHIFT = 8,
    FX_4_NUMERIC_COLUMNS_SHIFT = 11,
    FX_4_NUMERIC_COLUMN_MAJOR_MASK = 0x4000,

    /* Object types */
    FX_4_OBJECT_TYPE_STRING = 0x1,
    FX_4_OBJECT_TYPE_BLEND_STATE = 0x2,
    FX_4_OBJECT_TYPE_DEPTH_STENCIL_STATE = 0x3,
    FX_4_OBJECT_TYPE_RASTERIZER_STATE = 0x4,
    FX_4_OBJECT_TYPE_PIXEL_SHADER = 0x5,
    FX_4_OBJECT_TYPE_VERTEX_SHADER = 0x6,
    FX_4_OBJECT_TYPE_GEOMETRY_SHADER = 0x7,
    FX_4_OBJECT_TYPE_GEOMETRY_SHADER_SO = 0x8,

    FX_4_OBJECT_TYPE_TEXTURE = 0x9,
    FX_4_OBJECT_TYPE_TEXTURE_1D = 0xa,
    FX_4_OBJECT_TYPE_TEXTURE_1DARRAY = 0xb,
    FX_4_OBJECT_TYPE_TEXTURE_2D = 0xc,
    FX_4_OBJECT_TYPE_TEXTURE_2DARRAY = 0xd,
    FX_4_OBJECT_TYPE_TEXTURE_2DMS = 0xe,
    FX_4_OBJECT_TYPE_TEXTURE_2DMSARRAY = 0xf,
    FX_4_OBJECT_TYPE_TEXTURE_3D = 0x10,
    FX_4_OBJECT_TYPE_TEXTURE_CUBE = 0x11,
    FX_4_OBJECT_TYPE_RTV = 0x13,
    FX_4_OBJECT_TYPE_DSV = 0x14,
    FX_4_OBJECT_TYPE_SAMPLER_STATE = 0x15,
    FX_4_OBJECT_TYPE_TEXTURE_CUBEARRAY = 0x17,

    FX_5_OBJECT_TYPE_GEOMETRY_SHADER = 0x1b,
    FX_5_OBJECT_TYPE_COMPUTE_SHADER = 0x1c,
    FX_5_OBJECT_TYPE_HULL_SHADER = 0x1d,
    FX_5_OBJECT_TYPE_DOMAIN_SHADER = 0x1e,

    FX_5_OBJECT_TYPE_UAV_1D = 0x1f,
    FX_5_OBJECT_TYPE_UAV_1DARRAY = 0x20,
    FX_5_OBJECT_TYPE_UAV_2D = 0x21,
    FX_5_OBJECT_TYPE_UAV_2DARRAY = 0x22,
    FX_5_OBJECT_TYPE_UAV_3D = 0x23,
    FX_5_OBJECT_TYPE_UAV_BUFFER = 0x24,
    FX_5_OBJECT_TYPE_SRV_RAW_BUFFER = 0x25,
    FX_5_OBJECT_TYPE_UAV_RAW_BUFFER = 0x26,
    FX_5_OBJECT_TYPE_SRV_STRUCTURED_BUFFER = 0x27,
    FX_5_OBJECT_TYPE_UAV_STRUCTURED_BUFFER = 0x28,
    FX_5_OBJECT_TYPE_SRV_APPEND_STRUCTURED_BUFFER = 0x2b,
    FX_5_OBJECT_TYPE_SRV_CONSUME_STRUCTURED_BUFFER = 0x2c,

    /* Types */
    FX_4_TYPE_CLASS_NUMERIC = 1,
    FX_4_TYPE_CLASS_OBJECT = 2,
    FX_4_TYPE_CLASS_STRUCT = 3,

    /* Assignment types */
    FX_4_ASSIGNMENT_CONSTANT = 0x1,
    FX_4_ASSIGNMENT_VARIABLE = 0x2,
    FX_4_ASSIGNMENT_ARRAY_CONSTANT_INDEX = 0x3,
    FX_4_ASSIGNMENT_ARRAY_VARIABLE_INDEX = 0x4,
    FX_4_ASSIGNMENT_INDEX_EXPRESSION = 0x5,
    FX_4_ASSIGNMENT_VALUE_EXPRESSION = 0x6,
    FX_4_ASSIGNMENT_INLINE_SHADER = 0x7,
    FX_5_ASSIGNMENT_INLINE_SHADER = 0x8,

    /* FXLVM constants */
    FX_4_FXLC_COMP_COUNT_MASK = 0xffff,
    FX_4_FXLC_OPCODE_MASK = 0x7ff,
    FX_4_FXLC_OPCODE_SHIFT = 20,
    FX_4_FXLC_IS_SCALAR_MASK = 0x80000000,

    FX_4_FXLC_REG_LITERAL = 1,
    FX_4_FXLC_REG_CB = 2,
    FX_4_FXLC_REG_OUTPUT = 4,
    FX_4_FXLC_REG_TEMP = 7,
};

static const uint32_t fx_4_numeric_base_types[] =
{
    [HLSL_TYPE_HALF ] = FX_4_NUMERIC_TYPE_FLOAT,
    [HLSL_TYPE_FLOAT] = FX_4_NUMERIC_TYPE_FLOAT,
    [HLSL_TYPE_INT  ] = FX_4_NUMERIC_TYPE_INT,
    [HLSL_TYPE_UINT ] = FX_4_NUMERIC_TYPE_UINT,
    [HLSL_TYPE_BOOL ] = FX_4_NUMERIC_TYPE_BOOL,
};

static uint32_t get_fx_4_numeric_type_description(const struct hlsl_type *type, struct fx_write_context *fx)
{
    static const uint32_t numeric_type_class[] =
    {
        [HLSL_CLASS_SCALAR] = FX_4_NUMERIC_CLASS_SCALAR,
        [HLSL_CLASS_VECTOR] = FX_4_NUMERIC_CLASS_VECTOR,
        [HLSL_CLASS_MATRIX] = FX_4_NUMERIC_CLASS_MATRIX,
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
        case HLSL_TYPE_HALF:
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
        case HLSL_TYPE_BOOL:
            value |= (fx_4_numeric_base_types[type->e.numeric.type] << FX_4_NUMERIC_BASE_TYPE_SHIFT);
            break;
        default:
            hlsl_fixme(ctx, &ctx->location, "Not implemented for base type %u.", type->e.numeric.type);
            return 0;
    }

    value |= (type->e.numeric.dimy & 0x7) << FX_4_NUMERIC_ROWS_SHIFT;
    value |= (type->e.numeric.dimx & 0x7) << FX_4_NUMERIC_COLUMNS_SHIFT;
    if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
        value |= FX_4_NUMERIC_COLUMN_MAJOR_MASK;

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
        [HLSL_SAMPLER_DIM_RAW_BUFFER]        = "RWByteAddressBuffer",
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

        case HLSL_CLASS_GEOMETRY_SHADER:
            return "GeometryShader";

        case HLSL_CLASS_PIXEL_SHADER:
            return "PixelShader";

        case HLSL_CLASS_STRING:
            return "String";

        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            if (type->e.numeric.type == HLSL_TYPE_HALF)
                return "float";
            /* fall-through */
        default:
            return type->name;
    }
}

static bool is_numeric_fx_4_type(const struct hlsl_type *type)
{
    type = hlsl_get_multiarray_element_type(type);
    return type->class == HLSL_CLASS_STRUCT || hlsl_is_numeric_type(type);
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
    uint32_t name_offset, offset, unpacked_size, packed_size, stride, numeric_desc;
    struct vkd3d_bytecode_buffer *buffer = &fx->unstructured;
    struct field_offsets *field_offsets = NULL;
    const struct hlsl_type *element_type;
    struct hlsl_ctx *ctx = fx->ctx;
    uint32_t elements_count = 0;
    const char *name;
    size_t i;

    if (type->class == HLSL_CLASS_ARRAY)
        elements_count = hlsl_get_multiarray_size(type);
    element_type = hlsl_get_multiarray_element_type(type);

    name = get_fx_4_type_name(element_type);

    name_offset = write_string(name, fx);
    if (element_type->class == HLSL_CLASS_STRUCT)
    {
        if (!(field_offsets = hlsl_calloc(ctx, element_type->e.record.field_count, sizeof(*field_offsets))))
            return 0;

        for (i = 0; i < element_type->e.record.field_count; ++i)
        {
            const struct hlsl_struct_field *field = &element_type->e.record.fields[i];

            field_offsets[i].name = write_string(field->name, fx);
            field_offsets[i].semantic = write_string(field->semantic.raw_name, fx);
            field_offsets[i].offset = field->reg_offset[HLSL_REGSET_NUMERIC] * sizeof(float);
            field_offsets[i].type = write_type(field->type, fx);
        }
    }

    offset = put_u32_unaligned(buffer, name_offset);

    switch (element_type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            put_u32_unaligned(buffer, FX_4_TYPE_CLASS_NUMERIC);
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
            put_u32_unaligned(buffer, FX_4_TYPE_CLASS_OBJECT);
            break;

        case HLSL_CLASS_STRUCT:
            put_u32_unaligned(buffer, FX_4_TYPE_CLASS_STRUCT);
            break;

        case HLSL_CLASS_ARRAY:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_NULL:
        case HLSL_CLASS_STREAM_OUTPUT:
            vkd3d_unreachable();

        case HLSL_CLASS_VOID:
            FIXME("Writing type class %u is not implemented.\n", element_type->class);
            set_status(fx, VKD3D_ERROR_NOT_IMPLEMENTED);
            return 0;
    }

    /* Structures can only contain numeric fields, this is validated during variable declaration. */
    unpacked_size = type->reg_size[HLSL_REGSET_NUMERIC] * sizeof(float);

    packed_size = 0;
    if (is_numeric_fx_4_type(element_type))
        packed_size = hlsl_type_component_count(element_type) * sizeof(float);
    if (elements_count)
        packed_size *= elements_count;

    stride = element_type->reg_size[HLSL_REGSET_NUMERIC] * sizeof(float);
    stride = align(stride, 4 * sizeof(float));

    put_u32_unaligned(buffer, elements_count);
    put_u32_unaligned(buffer, unpacked_size);
    put_u32_unaligned(buffer, stride);
    put_u32_unaligned(buffer, packed_size);

    if (element_type->class == HLSL_CLASS_STRUCT)
    {
        put_u32_unaligned(buffer, element_type->e.record.field_count);
        for (i = 0; i < element_type->e.record.field_count; ++i)
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
    else if (element_type->class == HLSL_CLASS_TEXTURE)
    {
        static const uint32_t texture_type[] =
        {
            [HLSL_SAMPLER_DIM_GENERIC]   = FX_4_OBJECT_TYPE_TEXTURE,
            [HLSL_SAMPLER_DIM_1D]        = FX_4_OBJECT_TYPE_TEXTURE_1D,
            [HLSL_SAMPLER_DIM_1DARRAY]   = FX_4_OBJECT_TYPE_TEXTURE_1DARRAY,
            [HLSL_SAMPLER_DIM_2D]        = FX_4_OBJECT_TYPE_TEXTURE_2D,
            [HLSL_SAMPLER_DIM_2DARRAY]   = FX_4_OBJECT_TYPE_TEXTURE_2DARRAY,
            [HLSL_SAMPLER_DIM_2DMS]      = FX_4_OBJECT_TYPE_TEXTURE_2DMS,
            [HLSL_SAMPLER_DIM_2DMSARRAY] = FX_4_OBJECT_TYPE_TEXTURE_2DMSARRAY,
            [HLSL_SAMPLER_DIM_3D]        = FX_4_OBJECT_TYPE_TEXTURE_3D,
            [HLSL_SAMPLER_DIM_CUBE]      = FX_4_OBJECT_TYPE_TEXTURE_CUBE,
            [HLSL_SAMPLER_DIM_CUBEARRAY] = FX_4_OBJECT_TYPE_TEXTURE_CUBEARRAY,
        };

        put_u32_unaligned(buffer, texture_type[element_type->sampler_dim]);
    }
    else if (element_type->class == HLSL_CLASS_SAMPLER)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_SAMPLER_STATE);
    }
    else if (element_type->class == HLSL_CLASS_UAV)
    {
        static const uint32_t uav_type[] =
        {
            [HLSL_SAMPLER_DIM_1D]                = FX_5_OBJECT_TYPE_UAV_1D,
            [HLSL_SAMPLER_DIM_1DARRAY]           = FX_5_OBJECT_TYPE_UAV_1DARRAY,
            [HLSL_SAMPLER_DIM_2D]                = FX_5_OBJECT_TYPE_UAV_2D,
            [HLSL_SAMPLER_DIM_2DARRAY]           = FX_5_OBJECT_TYPE_UAV_2DARRAY,
            [HLSL_SAMPLER_DIM_3D]                = FX_5_OBJECT_TYPE_UAV_3D,
            [HLSL_SAMPLER_DIM_BUFFER]            = FX_5_OBJECT_TYPE_UAV_BUFFER,
            [HLSL_SAMPLER_DIM_STRUCTURED_BUFFER] = FX_5_OBJECT_TYPE_UAV_STRUCTURED_BUFFER,
            [HLSL_SAMPLER_DIM_RAW_BUFFER]        = FX_5_OBJECT_TYPE_UAV_RAW_BUFFER,
        };

        put_u32_unaligned(buffer, uav_type[element_type->sampler_dim]);
    }
    else if (element_type->class == HLSL_CLASS_DEPTH_STENCIL_VIEW)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_DSV);
    }
    else if (element_type->class == HLSL_CLASS_RENDER_TARGET_VIEW)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_RTV);
    }
    else if (element_type->class == HLSL_CLASS_PIXEL_SHADER)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_PIXEL_SHADER);
    }
    else if (element_type->class == HLSL_CLASS_VERTEX_SHADER)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_VERTEX_SHADER);
    }
    else if (element_type->class == HLSL_CLASS_RASTERIZER_STATE)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_RASTERIZER_STATE);
    }
    else if (element_type->class == HLSL_CLASS_DEPTH_STENCIL_STATE)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_DEPTH_STENCIL_STATE);
    }
    else if (element_type->class == HLSL_CLASS_BLEND_STATE)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_BLEND_STATE);
    }
    else if (element_type->class == HLSL_CLASS_STRING)
    {
        put_u32_unaligned(buffer, FX_4_OBJECT_TYPE_STRING);
    }
    else if (hlsl_is_numeric_type(element_type))
    {
        numeric_desc = get_fx_4_numeric_type_description(element_type, fx);
        put_u32_unaligned(buffer, numeric_desc);
    }
    else if (element_type->class == HLSL_CLASS_COMPUTE_SHADER)
    {
        put_u32_unaligned(buffer, FX_5_OBJECT_TYPE_COMPUTE_SHADER);
    }
    else if (element_type->class == HLSL_CLASS_HULL_SHADER)
    {
        put_u32_unaligned(buffer, FX_5_OBJECT_TYPE_HULL_SHADER);
    }
    else if (element_type->class == HLSL_CLASS_DOMAIN_SHADER)
    {
        put_u32_unaligned(buffer, FX_5_OBJECT_TYPE_DOMAIN_SHADER);
    }
    else
    {
        FIXME("Type %u is not supported.\n", element_type->class);
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

static uint32_t write_fx_2_parameter(const struct hlsl_type *type, const char *name,
        const struct hlsl_semantic *semantic, bool is_combined_sampler, struct fx_write_context *fx)
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

    offset = put_u32(buffer, hlsl_sm1_base_type(type, is_combined_sampler));
    put_u32(buffer, get_fx_2_type_class(type));
    put_u32(buffer, name_offset);
    put_u32(buffer, semantic_offset);
    put_u32(buffer, elements_count);

    switch (type->class)
    {
        case HLSL_CLASS_VECTOR:
            put_u32(buffer, type->e.numeric.dimx);
            put_u32(buffer, type->e.numeric.dimy);
            break;
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_MATRIX:
            put_u32(buffer, type->e.numeric.dimy);
            put_u32(buffer, type->e.numeric.dimx);
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
            write_fx_2_parameter(field->type, field->name, &field->semantic, false, fx);
        }
    }

    return offset;
}

static void write_fx_2_technique(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    uint32_t name_offset, pass_count_offset, annotation_count_offset, count = 0;
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    struct hlsl_ir_var *pass;

    name_offset = write_string(var->name, fx);
    put_u32(buffer, name_offset);
    annotation_count_offset = put_u32(buffer, 0);
    pass_count_offset = put_u32(buffer, 0);

    write_fx_2_annotations(var, annotation_count_offset, fx);

    LIST_FOR_EACH_ENTRY(pass, &var->scope->vars, struct hlsl_ir_var, scope_entry)
    {
        write_pass(pass, fx);
        ++count;
    }

    set_u32(buffer, pass_count_offset, count);
}

static uint32_t write_fx_2_default_value(struct hlsl_type *value_type, struct hlsl_default_value *value,
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
                    case HLSL_TYPE_HALF:
                    case HLSL_TYPE_INT:
                    case HLSL_TYPE_UINT:
                    case HLSL_TYPE_BOOL:

                        for (j = 0; j < comp_count; ++j)
                        {
                            put_u32(buffer, value->number.u);
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
                    write_fx_2_default_value(fields[i].type, value, fx);
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

static uint32_t write_fx_2_object_initializer(const struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);
    unsigned int i, elements_count = hlsl_get_multiarray_size(var->data_type);
    struct vkd3d_bytecode_buffer *buffer = &fx->objects;
    uint32_t offset = fx->unstructured.size, id, size;
    struct hlsl_ctx *ctx = fx->ctx;
    const void *data;

    for (i = 0; i < elements_count; ++i)
    {
        if (type->class == HLSL_CLASS_SAMPLER)
        {
            hlsl_fixme(ctx, &var->loc, "Writing fx_2_0 sampler objects initializers is not implemented.");
        }
        else
        {
            switch (type->class)
            {
                case HLSL_CLASS_STRING:
                {
                    const char *string = var->default_values[i].string ? var->default_values[i].string : "";
                    size = strlen(string) + 1;
                    data = string;
                    break;
                }
                case HLSL_CLASS_TEXTURE:
                    size = 0;
                    break;
                case HLSL_CLASS_PIXEL_SHADER:
                case HLSL_CLASS_VERTEX_SHADER:
                    size = 0;
                    hlsl_fixme(ctx, &var->loc, "Writing fx_2_0 shader objects initializers is not implemented.");
                    break;
                default:
                    vkd3d_unreachable();
            }
            id = fx->object_variable_count++;

            put_u32(&fx->unstructured, id);

            put_u32(buffer, id);
            put_u32(buffer, size);
            if (size)
            {
                static const uint32_t pad;

                bytecode_put_bytes(buffer, data, size);
                if (size % 4)
                    bytecode_put_bytes_unaligned(buffer, &pad, 4 - (size % 4));
            }
        }
    }

    return offset;
}

static uint32_t write_fx_2_initial_value(const struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);
    struct hlsl_ctx *ctx = fx->ctx;
    uint32_t offset;

    /* Note that struct fields must all be numeric;
     * this was validated in check_invalid_object_fields(). */
    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
        case HLSL_CLASS_STRUCT:
            offset = write_fx_2_default_value(var->data_type, var->default_values, fx);
            break;

        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_VERTEX_SHADER:
            offset = write_fx_2_object_initializer(var, fx);
            break;

        default:
            offset = 0;
            hlsl_fixme(ctx, &var->loc, "Writing initializer not implemented for parameter class %#x.", type->class);
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
        case HLSL_CLASS_SAMPLER:
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

        case HLSL_CLASS_STRING:
            return true;

        case HLSL_CLASS_PIXEL_SHADER:
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
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_NULL:
        case HLSL_CLASS_STREAM_OUTPUT:
            /* This cannot appear as an extern variable. */
            break;
    }

    vkd3d_unreachable();
}

static void write_fx_2_parameters(struct fx_write_context *fx)
{
    uint32_t desc_offset, value_offset, flags, annotation_count_offset;
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
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

        desc_offset = write_fx_2_parameter(var->data_type, var->name, &var->semantic, var->is_combined_sampler, fx);
        value_offset = write_fx_2_initial_value(var, fx);

        flags = 0;
        if (var->storage_modifiers & HLSL_STORAGE_SHARED)
            flags |= IS_SHARED;

        put_u32(buffer, desc_offset);
        put_u32(buffer, value_offset);
        put_u32(buffer, flags);

        annotation_count_offset = put_u32(buffer, 0);
        write_fx_2_annotations(var, annotation_count_offset, fx);

        ++fx->parameter_count;
    }
}

static void write_fx_2_annotation(struct hlsl_ir_var *var, struct fx_write_context *fx)
{
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    uint32_t desc_offset, value_offset;

    desc_offset = write_fx_2_parameter(var->data_type, var->name, &var->semantic, var->is_combined_sampler, fx);
    value_offset = write_fx_2_initial_value(var, fx);

    put_u32(buffer, desc_offset);
    put_u32(buffer, value_offset);
}

static const struct fx_write_context_ops fx_2_ops =
{
    .write_string = write_fx_2_string,
    .write_technique = write_fx_2_technique,
    .write_pass = write_fx_2_pass,
    .write_annotation = write_fx_2_annotation,
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
    write_techniques(ctx->globals, &fx);
    put_u32(structured, fx.object_variable_count - 1);
    put_u32(structured, 0); /* Resource count */

    bytecode_put_bytes(structured, fx.objects.data, fx.objects.size);
    /* TODO: resources */

    set_u32(structured, parameter_count, fx.parameter_count);
    set_u32(structured, object_count, fx.object_variable_count);
    set_u32(structured, technique_count, fx.technique_count);
    set_u32(structured, shader_count, fx.shader_count);

    size = align(fx.unstructured.size, 4);
    set_u32(&buffer, offset, size);

    bytecode_put_bytes(&buffer, fx.unstructured.data, fx.unstructured.size);
    bytecode_put_bytes(&buffer, fx.structured.data, fx.structured.size);

    vkd3d_free(fx.unstructured.data);
    vkd3d_free(fx.structured.data);
    vkd3d_free(fx.objects.data);

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
                    case HLSL_TYPE_HALF:
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
        switch (data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_INT:
            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                type = fx_4_numeric_base_types[data_type->e.numeric.type];
                break;
            default:
                type = 0;
                hlsl_fixme(ctx, &ctx->location, "Unsupported numeric state value type %u.", data_type->e.numeric.type);
        }

        put_u32_unaligned(buffer, type);
        put_u32_unaligned(buffer, value->value.u[i].u);
    }

    return offset;
}

static void write_fx_4_state_assignment(const struct hlsl_ir_var *var, struct hlsl_state_block_entry *entry,
        struct fx_write_context *fx)
{
    uint32_t value_offset = 0, assignment_type = 0, rhs_offset, type_offset, offset;
    struct vkd3d_bytecode_buffer *unstructured = &fx->unstructured;
    struct vkd3d_bytecode_buffer *buffer = &fx->structured;
    struct hlsl_ir_node *value = entry->args->node;
    struct hlsl_ctx *ctx = fx->ctx;
    struct hlsl_ir_var *index_var;
    struct hlsl_ir_constant *c;
    struct hlsl_ir_load *load;

    put_u32(buffer, entry->name_id);
    put_u32(buffer, entry->lhs_index);
    type_offset = put_u32(buffer, 0);
    rhs_offset = put_u32(buffer, 0);

    switch (value->type)
    {
        case HLSL_IR_CONSTANT:
        {
            c = hlsl_ir_constant(value);

            value_offset = write_fx_4_state_numeric_value(c, fx);
            assignment_type = FX_4_ASSIGNMENT_CONSTANT;
            break;
        }
        case HLSL_IR_LOAD:
        {
            load = hlsl_ir_load(value);

            if (load->src.path_len)
                hlsl_fixme(ctx, &var->loc, "Indexed access in RHS values is not implemented.");

            value_offset = write_fx_4_string(load->src.var->name, fx);
            assignment_type = FX_4_ASSIGNMENT_VARIABLE;
            break;
        }
        case HLSL_IR_INDEX:
        {
            struct hlsl_ir_index *index = hlsl_ir_index(value);
            struct hlsl_ir_node *val = index->val.node;
            struct hlsl_ir_node *idx = index->idx.node;
            struct hlsl_type *type;

            if (val->type != HLSL_IR_LOAD)
            {
                hlsl_fixme(ctx, &var->loc, "Unexpected indexed RHS value type.");
                break;
            }

            load = hlsl_ir_load(val);
            value_offset = write_fx_4_string(load->src.var->name, fx);
            type = load->src.var->data_type;

            switch (idx->type)
            {
                case HLSL_IR_CONSTANT:
                {
                    c = hlsl_ir_constant(idx);
                    value_offset = put_u32(unstructured, value_offset);
                    put_u32(unstructured, c->value.u[0].u);
                    assignment_type = FX_4_ASSIGNMENT_ARRAY_CONSTANT_INDEX;

                    if (c->value.u[0].u >= type->e.array.elements_count)
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                                "Array index %u exceeds array size %u.", c->value.u[0].u, type->e.array.elements_count);
                    break;
                }

                case HLSL_IR_LOAD:
                {
                    load = hlsl_ir_load(idx);
                    index_var = load->src.var;

                    /* Special case for uint index variables, for anything more complex use an expression. */
                    if (hlsl_types_are_equal(index_var->data_type, hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT))
                            && !load->src.path_len)
                    {
                        offset = write_fx_4_string(index_var->name, fx);

                        value_offset = put_u32(unstructured, value_offset);
                        put_u32(unstructured, offset);
                        assignment_type = FX_4_ASSIGNMENT_ARRAY_VARIABLE_INDEX;
                        break;
                    }
                }
                /* fall through */

                default:
                    hlsl_fixme(ctx, &var->loc, "Complex array index expressions in RHS values are not implemented.");
            }
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
        case FX_GEOMETRYSHADER:
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

static inline bool hlsl_type_state_compatible(struct hlsl_type *lhs, enum hlsl_base_type rhs)
{
    if (!hlsl_is_numeric_type(lhs))
        return false;
    switch (lhs->e.numeric.type)
    {
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
            return rhs == HLSL_TYPE_INT || rhs == HLSL_TYPE_UINT;

        default:
            return lhs->e.numeric.type == rhs;
    }

    vkd3d_unreachable();
}

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

static const struct fx_4_state
{
    const char *name;
    enum hlsl_type_class container;
    enum hlsl_type_class class;
    enum state_property_component_type type;
    unsigned int dimx;
    unsigned int array_size;
    int id;
    const struct rhs_named_value *values;
}
fx_4_states[] =
{
    { "RasterizerState",       HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_RASTERIZER,       1, 1, 0 },
    { "DepthStencilState",     HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_DEPTHSTENCIL,     1, 1, 1 },
    { "BlendState",            HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_BLEND,            1, 1, 2 },
    { "RenderTargetView",      HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_RENDERTARGETVIEW, 1, 8, 3 },
    { "DepthStencilView",      HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_DEPTHSTENCILVIEW, 1, 1, 4 },

    { "VertexShader",          HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_VERTEXSHADER,     1, 1, 6 },
    { "PixelShader",           HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_PIXELSHADER,      1, 1, 7 },
    { "GeometryShader",        HLSL_CLASS_PASS, HLSL_CLASS_SCALAR, FX_GEOMETRYSHADER,   1, 1, 8 },
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

    { "AlphaToCoverageEnable", HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 1, 36, bool_values },
    { "BlendEnable",           HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_BOOL,  1, 8, 37, bool_values },
    { "SrcBlend",              HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 38, blend_values },
    { "DestBlend",             HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 39, blend_values },
    { "BlendOp",               HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 40, blendop_values },
    { "SrcBlendAlpha",         HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 41, blend_values },
    { "DestBlendAlpha",        HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 42, blend_values },
    { "BlendOpAlpha",          HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT,  1, 1, 43, blendop_values },
    { "RenderTargetWriteMask", HLSL_CLASS_BLEND_STATE, HLSL_CLASS_SCALAR, FX_UINT8, 1, 8, 44 },

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

static const struct fx_4_state fx_5_blend_states[] =
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

struct fx_4_state_table
{
    const struct fx_4_state *ptr;
    unsigned int count;
};

static struct fx_4_state_table fx_4_get_state_table(enum hlsl_type_class type_class,
        unsigned int major, unsigned int minor)
{
    struct fx_4_state_table table;

    if (type_class == HLSL_CLASS_BLEND_STATE && (major == 5 || (major == 4 && minor == 1)))
    {
        table.ptr = fx_5_blend_states;
        table.count = ARRAY_SIZE(fx_5_blend_states);
    }
    else
    {
        table.ptr = fx_4_states;
        table.count = ARRAY_SIZE(fx_4_states);
    }

    return table;
}

static void resolve_fx_4_state_block_values(struct hlsl_ir_var *var,
        struct hlsl_state_block_entry *entry, struct fx_write_context *fx)
{
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);
    struct replace_state_context replace_context;
    const struct fx_4_state *state = NULL;
    struct hlsl_type *state_type = NULL;
    struct hlsl_ctx *ctx = fx->ctx;
    enum hlsl_base_type base_type;
    struct fx_4_state_table table;
    struct hlsl_ir_node *node;
    unsigned int i;

    table = fx_4_get_state_table(type->class, ctx->profile->major_version, ctx->profile->minor_version);

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

                if (data_type->class == HLSL_CLASS_SCALAR
                        && (data_type->e.numeric.type == HLSL_TYPE_INT || data_type->e.numeric.type == HLSL_TYPE_UINT))
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
        if (state->type == FX_UINT8 || !hlsl_type_state_compatible(node->data_type, base_type))
        {
            struct hlsl_ir_node *cast;

            if (!(cast = hlsl_new_cast(ctx, node, state_type, &var->loc)))
                return;
            list_add_after(&node->entry, &cast->entry);
            node = cast;
        }

        /* FX_UINT8 values are using 32-bits in the binary. Mask higher 24 bits for those. */
        if (state->type == FX_UINT8)
        {
            struct hlsl_ir_node *mask;

            if (!(mask = hlsl_new_uint_constant(ctx, 0xff, &var->loc)))
                return;
            list_add_after(&node->entry, &mask->entry);

            if (!(node = hlsl_new_binary_expr(ctx, HLSL_OP2_BIT_AND, node, mask)))
                return;
            list_add_after(&mask->entry, &node->entry);
        }

        if (node != entry->args->node)
        {
            hlsl_src_remove(entry->args);
            hlsl_src_from_node(entry->args, node);
        }

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
                comp->lhs_has_index, comp->lhs_index, true, arg_index);
    }
    hlsl_free_state_block_entry(entry);

    return entry->args_count;
}

/* For some states assignment sets all of the elements. This behaviour is limited to certain states of BlendState
   object, and only when fx_4_1 or fx_5_0 profile is used. */
static unsigned int decompose_fx_4_state_block_expand_array(struct hlsl_ir_var *var, struct hlsl_state_block *block,
        unsigned int entry_index, struct fx_write_context *fx)
{
    static const char *const states[] =
    {
        "SrcBlend",
        "DestBlend",
        "BlendOp",
        "SrcBlendAlpha",
        "DestBlendAlpha",
        "BlendOpAlpha",
    };
    const struct hlsl_type *type = hlsl_get_multiarray_element_type(var->data_type);
    struct hlsl_state_block_entry *entry = block->entries[entry_index];
    static const unsigned int array_size = 8;
    struct hlsl_ctx *ctx = fx->ctx;
    bool found = false;
    unsigned int i;

    if (type->class != HLSL_CLASS_BLEND_STATE)
        return 1;
    if (hlsl_version_lt(ctx, 4, 1))
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
                entry->name, true, i, true, 0);
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
        if (!is_numeric_fx_4_type(var->data_type))
            continue;

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

struct fx_parser
{
    const uint8_t *ptr, *start, *end;
    struct vkd3d_shader_message_context *message_context;
    struct vkd3d_string_buffer buffer;
    unsigned int indent;
    struct
    {
        unsigned int major;
        unsigned int minor;
    } version;
    struct
    {
        const uint8_t *ptr;
        const uint8_t *end;
        uint32_t size;
    } unstructured;
    uint32_t buffer_count;
    uint32_t object_count;
    uint32_t group_count;
    struct
    {
        uint32_t count;
        uint32_t *types;
    } objects;
    bool failed;
};

static uint32_t fx_parser_read_u32(struct fx_parser *parser)
{
    uint32_t ret;

    if ((parser->end - parser->ptr) < sizeof(uint32_t))
    {
        parser->failed = true;
        return 0;
    }

    ret = *(uint32_t *)parser->ptr;
    parser->ptr += sizeof(uint32_t);

    return ret;
}

static void fx_parser_read_u32s(struct fx_parser *parser, void *dst, size_t size)
{
    uint32_t *ptr = dst;
    size_t i;

    for (i = 0; i < size / sizeof(uint32_t); ++i)
        ptr[i] = fx_parser_read_u32(parser);
}

static void fx_parser_skip(struct fx_parser *parser, size_t size)
{
    if ((parser->end - parser->ptr) < size)
    {
        parser->ptr = parser->end;
        parser->failed = true;
        return;
    }
    parser->ptr += size;
}

static void VKD3D_PRINTF_FUNC(3, 4) fx_parser_error(struct fx_parser *parser, enum vkd3d_shader_error error,
        const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(parser->message_context, NULL, error, format, args);
    va_end(args);

    parser->failed = true;
}

static const void *fx_parser_get_unstructured_ptr(struct fx_parser *parser, uint32_t offset, size_t size)
{
    const uint8_t *ptr = parser->unstructured.ptr;

    if (offset >= parser->unstructured.size
            || size > parser->unstructured.size - offset)
    {
        parser->failed = true;
        return NULL;
    }

    return &ptr[offset];
}

static const void *fx_parser_get_ptr(struct fx_parser *parser, size_t size)
{
    if (parser->end - parser->ptr < size)
    {
        parser->failed = true;
        return NULL;
    }

    return parser->ptr;
}

static uint32_t fx_parser_read_unstructured(struct fx_parser *parser, void *dst, uint32_t offset, size_t size)
{
    const uint8_t *ptr;

    memset(dst, 0, size);
    if (!(ptr = fx_parser_get_unstructured_ptr(parser, offset, size)))
        return offset;

    memcpy(dst, ptr, size);
    return offset + size;
}

static void parse_fx_start_indent(struct fx_parser *parser)
{
    ++parser->indent;
}

static void parse_fx_end_indent(struct fx_parser *parser)
{
    --parser->indent;
}

static void parse_fx_print_indent(struct fx_parser *parser)
{
    vkd3d_string_buffer_printf(&parser->buffer, "%*s", 4 * parser->indent, "");
}

static const char *fx_2_get_string(struct fx_parser *parser, uint32_t offset, uint32_t *size)
{
    const char *ptr;

    fx_parser_read_unstructured(parser, size, offset, sizeof(*size));
    ptr = fx_parser_get_unstructured_ptr(parser, offset + 4, *size);

    if (!ptr)
    {
        parser->failed = true;
        return "<invalid>";
    }

    return ptr;
}

static unsigned int fx_get_fx_2_type_size(struct fx_parser *parser, uint32_t *offset)
{
    uint32_t element_count, member_count, class, columns, rows;
    unsigned int size = 0;

    fx_parser_read_unstructured(parser, &class, *offset + 4, sizeof(class));
    fx_parser_read_unstructured(parser, &element_count, *offset + 16, sizeof(element_count));

    if (class == D3DXPC_STRUCT)
    {
        *offset = fx_parser_read_unstructured(parser, &member_count, *offset + 20, sizeof(member_count));

        for (uint32_t i = 0; i < member_count; ++i)
            size += fx_get_fx_2_type_size(parser, offset);
    }
    else if (class == D3DXPC_VECTOR)
    {
        fx_parser_read_unstructured(parser, &columns, *offset + 20, sizeof(columns));
        *offset = fx_parser_read_unstructured(parser, &rows, *offset + 24, sizeof(rows));
        size = rows * columns * sizeof(float);
    }
    else if (class == D3DXPC_MATRIX_ROWS
            || class == D3DXPC_MATRIX_COLUMNS
            || class == D3DXPC_SCALAR)
    {
        fx_parser_read_unstructured(parser, &rows, *offset + 20, sizeof(rows));
        *offset = fx_parser_read_unstructured(parser, &columns, *offset + 24, sizeof(columns));
        size = rows * columns * sizeof(float);
    }
    else
    {
        *offset += 20;
    }

    if (element_count)
        size *= element_count;
    return size;
}

static const char *const fx_2_types[] =
{
    [D3DXPT_VOID]           = "void",
    [D3DXPT_BOOL]           = "bool",
    [D3DXPT_INT]            = "int",
    [D3DXPT_FLOAT]          = "float",
    [D3DXPT_STRING]         = "string",
    [D3DXPT_TEXTURE]        = "texture",
    [D3DXPT_TEXTURE1D]      = "texture1D",
    [D3DXPT_TEXTURE2D]      = "texture2D",
    [D3DXPT_TEXTURE3D]      = "texture3D",
    [D3DXPT_TEXTURECUBE]    = "textureCUBE",
    [D3DXPT_SAMPLER]        = "sampler",
    [D3DXPT_SAMPLER1D]      = "sampler1D",
    [D3DXPT_SAMPLER2D]      = "sampler2D",
    [D3DXPT_SAMPLER3D]      = "sampler3D",
    [D3DXPT_SAMPLERCUBE]    = "samplerCUBE",
    [D3DXPT_PIXELSHADER]    = "PixelShader",
    [D3DXPT_VERTEXSHADER]   = "VertexShader",
    [D3DXPT_PIXELFRAGMENT]  = "<pixel-fragment>",
    [D3DXPT_VERTEXFRAGMENT] = "<vertex-fragment>",
    [D3DXPT_UNSUPPORTED]    = "<unsupported>",
};

static void fx_parse_fx_2_type(struct fx_parser *parser, uint32_t offset)
{
    uint32_t type, class, rows, columns;
    const char *name;

    fx_parser_read_unstructured(parser, &type, offset, sizeof(type));
    fx_parser_read_unstructured(parser, &class, offset + 4, sizeof(class));

    if (class == D3DXPC_STRUCT)
        name = "struct";
    else
        name = type < ARRAY_SIZE(fx_2_types) ? fx_2_types[type] : "<unknown>";

    vkd3d_string_buffer_printf(&parser->buffer, "%s", name);
    if (class == D3DXPC_VECTOR)
    {
        fx_parser_read_unstructured(parser, &columns, offset + 20, sizeof(columns));
        fx_parser_read_unstructured(parser, &rows, offset + 24, sizeof(rows));
        vkd3d_string_buffer_printf(&parser->buffer, "%u", columns);
    }
    else if (class == D3DXPC_MATRIX_ROWS || class == D3DXPC_MATRIX_COLUMNS)
    {
        fx_parser_read_unstructured(parser, &rows, offset + 20, sizeof(rows));
        fx_parser_read_unstructured(parser, &columns, offset + 24, sizeof(columns));
        vkd3d_string_buffer_printf(&parser->buffer, "%ux%u", rows, columns);
    }
}

static void parse_fx_2_object_value(struct fx_parser *parser, uint32_t element_count,
        uint32_t type, uint32_t offset)
{
    uint32_t id;

    element_count = max(element_count, 1);

    for (uint32_t i = 0; i < element_count; ++i, offset += 4)
    {
        fx_parser_read_unstructured(parser, &id, offset, sizeof(id));
        vkd3d_string_buffer_printf(&parser->buffer, "<object id %u>", id);
        if (element_count > 1)
            vkd3d_string_buffer_printf(&parser->buffer, ", ");
        if (id < parser->objects.count)
            parser->objects.types[id] = type;
        else
            fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_DATA,
                    "Initializer object id exceeds the number of objects in the effect.");
    }


}

static void parse_fx_2_numeric_value(struct fx_parser *parser, uint32_t offset,
        unsigned int size, uint32_t base_type)
{
    unsigned int i, comp_count;

    comp_count = size / sizeof(uint32_t);
    if (comp_count > 1)
        vkd3d_string_buffer_printf(&parser->buffer, "{");
    for (i = 0; i < comp_count; ++i)
    {
        union hlsl_constant_value_component value;

        fx_parser_read_unstructured(parser, &value, offset, sizeof(uint32_t));

        if (base_type == D3DXPT_INT)
            vkd3d_string_buffer_printf(&parser->buffer, "%d", value.i);
        else if (base_type == D3DXPT_BOOL)
            vkd3d_string_buffer_printf(&parser->buffer, "%s", value.u ? "true" : "false" );
        else
            vkd3d_string_buffer_print_f32(&parser->buffer, value.f);

        if (i < comp_count - 1)
            vkd3d_string_buffer_printf(&parser->buffer, ", ");

        offset += sizeof(uint32_t);
    }
    if (comp_count > 1)
        vkd3d_string_buffer_printf(&parser->buffer, "}");
}

static void fx_parse_fx_2_parameter(struct fx_parser *parser, uint32_t offset)
{
    struct fx_2_var
    {
        uint32_t type;
        uint32_t class;
        uint32_t name;
        uint32_t semantic;
        uint32_t element_count;
    } var;
    const char *name;
    uint32_t size;

    fx_parser_read_unstructured(parser, &var, offset, sizeof(var));

    fx_parse_fx_2_type(parser, offset);

    name = fx_2_get_string(parser, var.name, &size);
    fx_print_string(&parser->buffer, " ", name, size);
    if (var.element_count)
        vkd3d_string_buffer_printf(&parser->buffer, "[%u]", var.element_count);
}

static bool is_fx_2_sampler(uint32_t type)
{
    return type == D3DXPT_SAMPLER
            || type == D3DXPT_SAMPLER1D
            || type == D3DXPT_SAMPLER2D
            || type == D3DXPT_SAMPLER3D
            || type == D3DXPT_SAMPLERCUBE;
}

static void fx_parse_fx_2_assignment(struct fx_parser *parser, const struct fx_assignment *entry);

static void parse_fx_2_sampler(struct fx_parser *parser, uint32_t element_count,
        uint32_t offset)
{
    struct fx_assignment entry;
    uint32_t count;

    element_count = max(element_count, 1);

    vkd3d_string_buffer_printf(&parser->buffer, "\n");
    for (uint32_t i = 0; i < element_count; ++i)
    {
        fx_parser_read_unstructured(parser, &count, offset, sizeof(count));
        offset += sizeof(count);

        parse_fx_start_indent(parser);
        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "{\n");
        parse_fx_start_indent(parser);
        for (uint32_t j = 0; j < count; ++j, offset += sizeof(entry))
        {
            fx_parser_read_unstructured(parser, &entry, offset, sizeof(entry));

            parse_fx_print_indent(parser);
            fx_parse_fx_2_assignment(parser, &entry);
        }
        parse_fx_end_indent(parser);
        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "},\n");
        parse_fx_end_indent(parser);
    }
}

static void fx_parse_fx_2_initial_value(struct fx_parser *parser, uint32_t param, uint32_t value)
{
    struct fx_2_var
    {
        uint32_t type;
        uint32_t class;
        uint32_t name;
        uint32_t semantic;
        uint32_t element_count;
    } var;
    unsigned int size;
    uint32_t offset;

    if (!value)
        return;

    fx_parser_read_unstructured(parser, &var, param, sizeof(var));

    offset = param;
    size = fx_get_fx_2_type_size(parser, &offset);

    vkd3d_string_buffer_printf(&parser->buffer, " = ");
    if (var.element_count)
        vkd3d_string_buffer_printf(&parser->buffer, "{ ");

    if (var.class == D3DXPC_OBJECT)
    {
        if (is_fx_2_sampler(var.type))
            parse_fx_2_sampler(parser, var.element_count, value);
        else
            parse_fx_2_object_value(parser, var.element_count, var.type, value);
    }
    else
    {
        parse_fx_2_numeric_value(parser, value, size, var.type);
    }

    if (var.element_count)
        vkd3d_string_buffer_printf(&parser->buffer, " }");
}

static void fx_parse_fx_2_annotations(struct fx_parser *parser, uint32_t count)
{
    uint32_t param, value;

    if (parser->failed || !count)
        return;

    vkd3d_string_buffer_printf(&parser->buffer, "\n");
    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "<\n");
    parse_fx_start_indent(parser);

    for (uint32_t i = 0; i < count; ++i)
    {
        param = fx_parser_read_u32(parser);
        value = fx_parser_read_u32(parser);

        parse_fx_print_indent(parser);
        fx_parse_fx_2_parameter(parser, param);
        fx_parse_fx_2_initial_value(parser, param, value);
        vkd3d_string_buffer_printf(&parser->buffer, ";\n");
    }

    parse_fx_end_indent(parser);
    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, ">");
}

static void fx_parse_fx_2_assignment(struct fx_parser *parser, const struct fx_assignment *entry)
{
    const struct rhs_named_value *named_value = NULL;
    const struct fx_2_state *state = NULL;

    if (entry->id <= ARRAY_SIZE(fx_2_states))
    {
        state = &fx_2_states[entry->id];

        vkd3d_string_buffer_printf(&parser->buffer, "%s", state->name);
        if (state->array_size > 1)
            vkd3d_string_buffer_printf(&parser->buffer, "[%u]", entry->lhs_index);
    }
    else
    {
        vkd3d_string_buffer_printf(&parser->buffer, "<unrecognized state %u>", entry->id);
    }
    vkd3d_string_buffer_printf(&parser->buffer, " = ");

    if (state && state->type == FX_UINT)
    {
        const struct rhs_named_value *ptr = state->values;
        uint32_t value;

        fx_parser_read_unstructured(parser, &value, entry->value, sizeof(value));

        while (ptr->name)
        {
            if (value == ptr->value)
            {
                named_value = ptr;
                break;
            }
            ++ptr;
        }
    }

    if (named_value)
    {
        vkd3d_string_buffer_printf(&parser->buffer, "%s /* %u */", named_value->name, named_value->value);
    }
    else if (state)
    {
        if (state->type == FX_UINT || state->type == FX_FLOAT)
        {
            uint32_t offset = entry->type;
            unsigned int size;

            size = fx_get_fx_2_type_size(parser, &offset);
            parse_fx_2_numeric_value(parser, entry->value, size, entry->type);
        }
        else if (state->type == FX_VERTEXSHADER || state->type == FX_PIXELSHADER)
        {
            uint32_t id;

            fx_parser_read_unstructured(parser, &id, entry->value, sizeof(id));
            vkd3d_string_buffer_printf(&parser->buffer, "<object id %u>", id);
        }
        else
        {
            vkd3d_string_buffer_printf(&parser->buffer, "<ignored>");
        }
    }
    else
    {
        vkd3d_string_buffer_printf(&parser->buffer, "<ignored>");
    }
    vkd3d_string_buffer_printf(&parser->buffer, ";\n");
}

static void fx_parse_fx_2_technique(struct fx_parser *parser)
{
    struct fx_technique
    {
        uint32_t name;
        uint32_t annotation_count;
        uint32_t pass_count;
    } technique;
    struct fx_pass
    {
        uint32_t name;
        uint32_t annotation_count;
        uint32_t assignment_count;
    } pass;
    const char *name;
    uint32_t size;

    if (parser->failed)
        return;

    fx_parser_read_u32s(parser, &technique, sizeof(technique));

    name = fx_2_get_string(parser, technique.name, &size);

    parse_fx_print_indent(parser);
    fx_print_string(&parser->buffer, "technique ", name, size);
    fx_parse_fx_2_annotations(parser, technique.annotation_count);

    vkd3d_string_buffer_printf(&parser->buffer, "\n");
    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "{\n");

    parse_fx_start_indent(parser);
    for (uint32_t i = 0; i < technique.pass_count; ++i)
    {
        fx_parser_read_u32s(parser, &pass, sizeof(pass));
        name = fx_2_get_string(parser, pass.name, &size);

        parse_fx_print_indent(parser);
        fx_print_string(&parser->buffer, "pass ", name, size);
        fx_parse_fx_2_annotations(parser, pass.annotation_count);

        vkd3d_string_buffer_printf(&parser->buffer, "\n");
        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "{\n");

        parse_fx_start_indent(parser);
        for (uint32_t j = 0; j < pass.assignment_count; ++j)
        {
            struct fx_assignment entry;

            parse_fx_print_indent(parser);
            fx_parser_read_u32s(parser, &entry, sizeof(entry));
            fx_parse_fx_2_assignment(parser, &entry);
        }
        parse_fx_end_indent(parser);

        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "}\n\n");
    }

    parse_fx_end_indent(parser);

    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "}\n\n");
}

static void fx_2_parse_parameters(struct fx_parser *parser, uint32_t count)
{
    struct fx_2_parameter
    {
        uint32_t type;
        uint32_t value;
        uint32_t flags;
        uint32_t annotation_count;
    } param;

    for (uint32_t i = 0; i < count; ++i)
    {
        fx_parser_read_u32s(parser, &param, sizeof(param));

        fx_parse_fx_2_parameter(parser, param.type);
        fx_parse_fx_2_annotations(parser, param.annotation_count);
        fx_parse_fx_2_initial_value(parser, param.type, param.value);
        vkd3d_string_buffer_printf(&parser->buffer, ";\n");
    }
    if (count)
        vkd3d_string_buffer_printf(&parser->buffer, "\n");
}

static void fx_parse_shader_blob(struct fx_parser *parser, enum vkd3d_shader_source_type source_type,
        const void *data, uint32_t data_size)
{
    struct vkd3d_shader_compile_info info = { 0 };
    struct vkd3d_shader_code output;
    const char *p, *q, *end;
    int ret;

    static const struct vkd3d_shader_compile_option options[] =
    {
        {VKD3D_SHADER_COMPILE_OPTION_API_VERSION, VKD3D_SHADER_API_VERSION_1_16},
    };

    info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.source.code = data;
    info.source.size = data_size;
    info.source_type = source_type;
    info.target_type = VKD3D_SHADER_TARGET_D3D_ASM;
    info.options = options;
    info.option_count = ARRAY_SIZE(options);
    info.log_level = VKD3D_SHADER_LOG_INFO;

    if ((ret = vkd3d_shader_compile(&info, &output, NULL)) < 0)
    {
        fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_DATA,
                "Failed to disassemble shader blob.");
        return;
    }
    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "asm {\n");

    parse_fx_start_indent(parser);

    end = (const char *)output.code + output.size;
    for (p = output.code; p < end; p = q)
    {
        if (!(q = memchr(p, '\n', end - p)))
            q = end;
        else
            ++q;

        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "%.*s", (int)(q - p), p);
    }

    parse_fx_end_indent(parser);
    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "}");

    vkd3d_shader_free_shader_code(&output);
}

static void fx_parse_fx_2_data_blob(struct fx_parser *parser)
{
    uint32_t id, size;
    const void *data;

    id = fx_parser_read_u32(parser);
    size = fx_parser_read_u32(parser);

    parse_fx_print_indent(parser);
    if (id < parser->objects.count)
    {
        uint32_t type = parser->objects.types[id];
        switch (type)
        {
            case D3DXPT_STRING:
            case D3DXPT_TEXTURE:
            case D3DXPT_TEXTURE1D:
            case D3DXPT_TEXTURE2D:
            case D3DXPT_TEXTURE3D:
            case D3DXPT_TEXTURECUBE:
            case D3DXPT_PIXELSHADER:
            case D3DXPT_VERTEXSHADER:
                vkd3d_string_buffer_printf(&parser->buffer, "%s object %u size %u bytes%s\n",
                        fx_2_types[type], id, size, size ? ":" : ",");

                if (size)
                {
                    data = fx_parser_get_ptr(parser, size);

                    if (type == D3DXPT_STRING)
                    {
                        parse_fx_start_indent(parser);
                        parse_fx_print_indent(parser);
                        fx_print_string(&parser->buffer, "\"", (const char *)data, size);
                        vkd3d_string_buffer_printf(&parser->buffer, "\"");
                        parse_fx_end_indent(parser);
                    }
                    else if (type == D3DXPT_PIXELSHADER || type == D3DXPT_VERTEXSHADER)
                    {
                        fx_parse_shader_blob(parser, VKD3D_SHADER_SOURCE_D3D_BYTECODE, data, size);
                    }
                    vkd3d_string_buffer_printf(&parser->buffer, "\n");
                }
                break;
            default:
                vkd3d_string_buffer_printf(&parser->buffer, "<type%u> object %u size %u bytes\n", type, id, size);
        }
    }
    else
    {
        vkd3d_string_buffer_printf(&parser->buffer, "object %u - out-of-range id\n", id);
    }

    fx_parser_skip(parser, align(size, 4));
}

static void fx_dump_blob(struct fx_parser *parser, const void *blob, uint32_t size)
{
    const uint32_t *data = blob;
    unsigned int i, j, n;

    size /= sizeof(*data);
    i = 0;
    while (i < size)
    {
        parse_fx_print_indent(parser);
        n = min(size - i, 8);
        for (j = 0; j < n; ++j)
            vkd3d_string_buffer_printf(&parser->buffer, "0x%08x,", data[i + j]);
        i += n;
        vkd3d_string_buffer_printf(&parser->buffer, "\n");
    }
}

static void fx_parse_fx_2_array_selector(struct fx_parser *parser, uint32_t size)
{
    const uint8_t *end = parser->ptr + size;
    uint32_t name_size, blob_size = 0;
    const void *blob = NULL;
    const char *name;

    name_size = fx_parser_read_u32(parser);
    name = fx_parser_get_ptr(parser, name_size);
    fx_parser_skip(parser, name_size);

    if (!name || (uint8_t *)name >= end)
        fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_DATA,
                "Malformed name entry in the array selector.");

    if (parser->ptr <= end)
    {
        blob_size = end - parser->ptr;
        blob = fx_parser_get_ptr(parser, blob_size);
        fx_parser_skip(parser, blob_size);
    }
    else
    {
        fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_DATA,
                "Malformed blob entry in the array selector.");
    }

    if (name)
    {
        fx_print_string(&parser->buffer, "array \"", name, name_size);
        vkd3d_string_buffer_printf(&parser->buffer, "\"\n");
    }
    if (blob)
    {
        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "selector blob size %u\n", blob_size);
        fx_dump_blob(parser, blob, blob_size);
    }
}

static void fx_parse_fx_2_complex_state(struct fx_parser *parser)
{
    struct
    {
        uint32_t technique;
        uint32_t index;
        uint32_t element;
        uint32_t state;
        uint32_t assignment_type;
    } state;
    const char *data;
    uint32_t size;

    fx_parser_read_u32s(parser, &state, sizeof(state));

    if (state.technique == ~0u)
    {
        vkd3d_string_buffer_printf(&parser->buffer, "parameter %u[%u], state %u =\n",
                state.index, state.element, state.state);
    }
    else
    {
        vkd3d_string_buffer_printf(&parser->buffer, "technique %u, pass %u, state %u =\n",
                state.technique, state.index, state.state);
    }

    size = fx_parser_read_u32(parser);

    parse_fx_print_indent(parser);

    if (state.assignment_type == FX_2_ASSIGNMENT_PARAMETER)
    {
        data = fx_parser_get_ptr(parser, size);
        fx_print_string(&parser->buffer, "parameter \"", data, size);
        vkd3d_string_buffer_printf(&parser->buffer, "\"\n");
        fx_parser_skip(parser, align(size, 4));
    }
    else if (state.assignment_type == FX_2_ASSIGNMENT_ARRAY_SELECTOR)
    {
        fx_parse_fx_2_array_selector(parser, size);
    }
    else
    {
        vkd3d_string_buffer_printf(&parser->buffer, "blob size %u\n", size);
        data = fx_parser_get_ptr(parser, size);
        fx_dump_blob(parser, data, size);
        fx_parser_skip(parser, align(size, 4));
    }
}

static void fx_2_parse(struct fx_parser *parser)
{
    uint32_t i, size, parameter_count, technique_count, blob_count, state_count;

    fx_parser_skip(parser, sizeof(uint32_t)); /* Version */
    size = fx_parser_read_u32(parser);

    parser->unstructured.ptr = parser->ptr;
    parser->unstructured.end = parser->ptr + size;
    parser->unstructured.size = size;
    fx_parser_skip(parser, size);

    parameter_count = fx_parser_read_u32(parser);
    technique_count = fx_parser_read_u32(parser);
    fx_parser_read_u32(parser); /* Shader count */
    parser->objects.count = fx_parser_read_u32(parser);

    if (!(parser->objects.types = calloc(parser->objects.count, sizeof(*parser->objects.types))))
    {
        fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_OUT_OF_MEMORY, "Out of memory.");
        return;
    }

    fx_2_parse_parameters(parser, parameter_count);
    for (i = 0; i < technique_count; ++i)
        fx_parse_fx_2_technique(parser);

    blob_count = fx_parser_read_u32(parser);
    state_count = fx_parser_read_u32(parser);

    vkd3d_string_buffer_printf(&parser->buffer, "object data {\n");
    parse_fx_start_indent(parser);
    for (i = 0; i < blob_count; ++i)
        fx_parse_fx_2_data_blob(parser);
    parse_fx_end_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "}\n\n");

    vkd3d_string_buffer_printf(&parser->buffer, "state data {\n");
    parse_fx_start_indent(parser);
    for (i = 0; i < state_count; ++i)
        fx_parse_fx_2_complex_state(parser);
    parse_fx_end_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "}\n");
}

static const char *fx_4_get_string(struct fx_parser *parser, uint32_t offset)
{
    const uint8_t *ptr = parser->unstructured.ptr;
    const uint8_t *end = parser->unstructured.end;

    if (offset >= parser->unstructured.size)
    {
        parser->failed = true;
        return "<invalid>";
    }

    ptr += offset;

    while (ptr < end && *ptr)
        ++ptr;

    if (*ptr)
    {
        parser->failed = true;
        return "<invalid>";
    }

    return (const char *)(parser->unstructured.ptr + offset);
}

static void parse_fx_4_numeric_value(struct fx_parser *parser, uint32_t offset,
        const struct fx_4_binary_type *type)
{
    unsigned int base_type, comp_count;
    size_t i;

    base_type = (type->typeinfo >> FX_4_NUMERIC_BASE_TYPE_SHIFT) & 0xf;

    comp_count = type->packed_size / sizeof(uint32_t);
    for (i = 0; i < comp_count; ++i)
    {
        union hlsl_constant_value_component value;

        fx_parser_read_unstructured(parser, &value, offset, sizeof(uint32_t));

        if (base_type == FX_4_NUMERIC_TYPE_FLOAT)
            vkd3d_string_buffer_print_f32(&parser->buffer, value.f);
        else if (base_type == FX_4_NUMERIC_TYPE_INT)
            vkd3d_string_buffer_printf(&parser->buffer, "%d", value.i);
        else if (base_type == FX_4_NUMERIC_TYPE_UINT)
            vkd3d_string_buffer_printf(&parser->buffer, "%u", value.u);
        else if (base_type == FX_4_NUMERIC_TYPE_BOOL)
            vkd3d_string_buffer_printf(&parser->buffer, "%s", value.u ? "true" : "false" );
        else
            vkd3d_string_buffer_printf(&parser->buffer, "%#x", value.u);

        if (i < comp_count - 1)
            vkd3d_string_buffer_printf(&parser->buffer, ", ");

        offset += sizeof(uint32_t);
    }
}

static void fx_4_parse_string_initializer(struct fx_parser *parser, uint32_t offset)
{
    const char *str = fx_4_get_string(parser, offset);
    vkd3d_string_buffer_printf(&parser->buffer, "\"%s\"", str);
}

static void fx_parse_fx_4_annotations(struct fx_parser *parser)
{
    struct fx_4_annotation
    {
        uint32_t name;
        uint32_t type;
    } var;
    struct fx_4_binary_type type;
    const char *name, *type_name;
    uint32_t count, i, value;

    if (parser->failed)
        return;

    count = fx_parser_read_u32(parser);

    if (!count)
        return;

    vkd3d_string_buffer_printf(&parser->buffer, "\n");
    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "<\n");
    parse_fx_start_indent(parser);

    for (i = 0; i < count; ++i)
    {
        fx_parser_read_u32s(parser, &var, sizeof(var));
        fx_parser_read_unstructured(parser, &type, var.type, sizeof(type));

        name = fx_4_get_string(parser, var.name);
        type_name = fx_4_get_string(parser, type.name);

        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "%s %s", type_name, name);
        if (type.element_count)
            vkd3d_string_buffer_printf(&parser->buffer, "[%u]", type.element_count);
        vkd3d_string_buffer_printf(&parser->buffer, " = ");
        if (type.element_count)
            vkd3d_string_buffer_printf(&parser->buffer, "{ ");

        if (type.class == FX_4_TYPE_CLASS_NUMERIC)
        {
            value = fx_parser_read_u32(parser);
            parse_fx_4_numeric_value(parser, value, &type);
        }
        else if (type.class == FX_4_TYPE_CLASS_OBJECT && type.typeinfo == FX_4_OBJECT_TYPE_STRING)
        {
            uint32_t element_count = max(type.element_count, 1);

            for (uint32_t j = 0; j < element_count; ++j)
            {
                value = fx_parser_read_u32(parser);
                fx_4_parse_string_initializer(parser, value);
                if (j < element_count - 1)
                    vkd3d_string_buffer_printf(&parser->buffer, ", ");
            }
        }
        else
        {
            fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_DATA,
                    "Only numeric and string types are supported in annotations.");
        }

        if (type.element_count)
            vkd3d_string_buffer_printf(&parser->buffer, " }");
        vkd3d_string_buffer_printf(&parser->buffer, ";\n");
    }
    parse_fx_end_indent(parser);

    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, ">");
}

static void fx_parse_fx_4_numeric_variables(struct fx_parser *parser, uint32_t count)
{
    struct fx_4_numeric_variable
    {
        uint32_t name;
        uint32_t type;
        uint32_t semantic;
        uint32_t offset;
        uint32_t value;
        uint32_t flags;
    } var;
    const char *name, *semantic, *type_name;
    struct fx_4_binary_type type;
    uint32_t i;

    for (i = 0; i < count; ++i)
    {
        fx_parser_read_u32s(parser, &var, sizeof(var));
        fx_parser_read_unstructured(parser, &type, var.type, sizeof(type));

        name = fx_4_get_string(parser, var.name);
        type_name = fx_4_get_string(parser, type.name);

        vkd3d_string_buffer_printf(&parser->buffer, "    %s %s", type_name, name);
        if (type.element_count)
            vkd3d_string_buffer_printf(&parser->buffer, "[%u]", type.element_count);

        if (var.semantic)
        {
            semantic = fx_4_get_string(parser, var.semantic);
            vkd3d_string_buffer_printf(&parser->buffer, " : %s", semantic);
        }
        fx_parse_fx_4_annotations(parser);

        if (var.value)
        {
            vkd3d_string_buffer_printf(&parser->buffer, " = { ");
            parse_fx_4_numeric_value(parser, var.value, &type);
            vkd3d_string_buffer_printf(&parser->buffer, " }");
        }
        vkd3d_string_buffer_printf(&parser->buffer, ";    // Offset: %u, size %u.\n", var.offset, type.unpacked_size);
    }
}

static void fx_parse_buffers(struct fx_parser *parser)
{
    struct fx_buffer
    {
        uint32_t name;
        uint32_t size;
        uint32_t flags;
        uint32_t count;
        uint32_t bind_point;
    } buffer;
    const char *name;
    uint32_t i;

    if (parser->failed)
        return;

    for (i = 0; i < parser->buffer_count; ++i)
    {
        fx_parser_read_u32s(parser, &buffer, sizeof(buffer));

        name = fx_4_get_string(parser, buffer.name);

        vkd3d_string_buffer_printf(&parser->buffer, "cbuffer %s", name);
        fx_parse_fx_4_annotations(parser);

        vkd3d_string_buffer_printf(&parser->buffer, "\n{\n");
        parse_fx_start_indent(parser);
        fx_parse_fx_4_numeric_variables(parser, buffer.count);
        parse_fx_end_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "}\n\n");
    }
}

static void fx_4_parse_shader_blob(struct fx_parser *parser, unsigned int object_type, const struct fx_5_shader *shader)
{
    const void *data = NULL;
    uint32_t data_size;

    if (!shader->offset)
    {
        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "NULL");
        return;
    }

    fx_parser_read_unstructured(parser, &data_size, shader->offset, sizeof(data_size));
    if (data_size)
        data = fx_parser_get_unstructured_ptr(parser, shader->offset + 4, data_size);

    if (!data)
        return;

    fx_parse_shader_blob(parser, VKD3D_SHADER_SOURCE_DXBC_TPF, data, data_size);

    if (object_type == FX_4_OBJECT_TYPE_GEOMETRY_SHADER_SO && shader->sodecl[0])
    {
        vkd3d_string_buffer_printf(&parser->buffer, "\n/* Stream output declaration: \"%s\" */",
                fx_4_get_string(parser, shader->sodecl[0]));
    }
    else if (object_type == FX_5_OBJECT_TYPE_GEOMETRY_SHADER)
    {
        for (unsigned int i = 0; i < ARRAY_SIZE(shader->sodecl); ++i)
        {
           if (shader->sodecl[i])
               vkd3d_string_buffer_printf(&parser->buffer, "\n/* Stream output %u declaration: \"%s\" */",
                       i, fx_4_get_string(parser, shader->sodecl[i]));
        }
        if (shader->sodecl_count)
            vkd3d_string_buffer_printf(&parser->buffer, "\n/* Rasterized stream %u */", shader->rast_stream);
    }
}

static void fx_4_parse_shader_initializer(struct fx_parser *parser, unsigned int object_type)
{
    struct fx_5_shader shader = { 0 };

    switch (object_type)
    {
        case FX_4_OBJECT_TYPE_PIXEL_SHADER:
        case FX_4_OBJECT_TYPE_VERTEX_SHADER:
        case FX_4_OBJECT_TYPE_GEOMETRY_SHADER:
            shader.offset = fx_parser_read_u32(parser);
            break;

        case FX_4_OBJECT_TYPE_GEOMETRY_SHADER_SO:
            shader.offset    = fx_parser_read_u32(parser);
            shader.sodecl[0] = fx_parser_read_u32(parser);
            break;

        case FX_5_OBJECT_TYPE_GEOMETRY_SHADER:
        case FX_5_OBJECT_TYPE_COMPUTE_SHADER:
        case FX_5_OBJECT_TYPE_HULL_SHADER:
        case FX_5_OBJECT_TYPE_DOMAIN_SHADER:
            fx_parser_read_u32s(parser, &shader, sizeof(shader));
            break;

        default:
            parser->failed = true;
            return;
    }

    fx_4_parse_shader_blob(parser, object_type, &shader);
}

static bool fx_4_object_has_initializer(const struct fx_4_binary_type *type)
{
    switch (type->typeinfo)
    {
        case FX_4_OBJECT_TYPE_STRING:
        case FX_4_OBJECT_TYPE_BLEND_STATE:
        case FX_4_OBJECT_TYPE_DEPTH_STENCIL_STATE:
        case FX_4_OBJECT_TYPE_RASTERIZER_STATE:
        case FX_4_OBJECT_TYPE_SAMPLER_STATE:
        case FX_4_OBJECT_TYPE_PIXEL_SHADER:
        case FX_4_OBJECT_TYPE_VERTEX_SHADER:
        case FX_4_OBJECT_TYPE_GEOMETRY_SHADER:
        case FX_4_OBJECT_TYPE_GEOMETRY_SHADER_SO:
        case FX_5_OBJECT_TYPE_GEOMETRY_SHADER:
        case FX_5_OBJECT_TYPE_COMPUTE_SHADER:
        case FX_5_OBJECT_TYPE_HULL_SHADER:
        case FX_5_OBJECT_TYPE_DOMAIN_SHADER:
            return true;
        default:
            return false;
    }
}

static int fx_4_state_id_compare(const void *a, const void *b)
{
    const struct fx_4_state *state = b;
    int id = *(int *)a;

    return id - state->id;
}

static const struct
{
    uint32_t opcode;
    const char *name;
}
fx_4_fxlc_opcodes[] =
{
    { 0x100, "mov"   },
    { 0x101, "neg"   },
    { 0x103, "rcp"   },
    { 0x104, "frc"   },
    { 0x105, "exp"   },
    { 0x106, "log"   },
    { 0x107, "rsq"   },
    { 0x108, "sin"   },
    { 0x109, "cos"   },
    { 0x10a, "asin"  },
    { 0x10b, "acos"  },
    { 0x10c, "atan"  },
    { 0x112, "sqrt"  },
    { 0x120, "ineg"  },
    { 0x121, "not"   },
    { 0x130, "itof"  },
    { 0x131, "utof"  },
    { 0x133, "ftou"  },
    { 0x137, "ftob"  },
    { 0x139, "floor" },
    { 0x13a, "ceil"  },
    { 0x200, "min"   },
    { 0x201, "max"   },
    { 0x204, "add"   },
    { 0x205, "mul"   },
    { 0x206, "atan2" },
    { 0x208, "div"   },
    { 0x210, "bilt"  },
    { 0x211, "bige"  },
    { 0x212, "bieq"  },
    { 0x213, "bine"  },
    { 0x214, "buge"  },
    { 0x215, "bult"  },
    { 0x216, "iadd"  },
    { 0x219, "imul"  },
    { 0x21a, "udiv"  },
    { 0x21d, "imin"  },
    { 0x21e, "imax"  },
    { 0x21f, "umin"  },
    { 0x220, "umax"  },
    { 0x230, "and"   },
    { 0x231, "or"    },
    { 0x233, "xor"   },
    { 0x234, "ishl"  },
    { 0x235, "ishr"  },
    { 0x236, "ushr"  },
    { 0x301, "movc"  },
    { 0x500, "dot"   },
    { 0x70e, "d3ds_dotswiz" },
};

static const char *fx_4_get_fxlc_opcode_name(uint32_t opcode)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(fx_4_fxlc_opcodes); ++i)
    {
        if (fx_4_fxlc_opcodes[i].opcode == opcode)
            return fx_4_fxlc_opcodes[i].name;
    }

    return "<unrecognized>";
}

struct fx_4_fxlc_argument
{
    uint32_t flags;
    uint32_t reg_type;
    uint32_t address;
};

struct fx_4_ctab_entry
{
    uint32_t name;
    uint16_t register_set;
    uint16_t register_index;
    uint16_t register_count;
    uint16_t reserved;
    uint32_t typeinfo;
    uint32_t default_value;
};

struct fxlvm_code
{
    const float *cli4;
    uint32_t cli4_count;

    const struct fx_4_ctab_entry *constants;
    uint32_t ctab_offset;
    uint32_t ctab_count;
    const char *ctab;

    unsigned int comp_count;
    bool scalar;
};

static void fx_4_parse_print_swizzle(struct fx_parser *parser, const struct fxlvm_code *code, unsigned int addr)
{
    unsigned int comp_count = code->scalar ? 1 : code->comp_count;
    static const char comp[] = "xyzw";

    if (comp_count < 4)
        vkd3d_string_buffer_printf(&parser->buffer, ".%.*s", comp_count, &comp[addr % 4]);
}

static void fx_4_parse_fxlc_constant_argument(struct fx_parser *parser,
        const struct fx_4_fxlc_argument *arg, const struct fxlvm_code *code)
{
    uint32_t i, offset, register_index = arg->address / 4; /* Address counts in components. */

    for (i = 0; i < code->ctab_count; ++i)
    {
        const struct fx_4_ctab_entry *c = &code->constants[i];

        if (register_index < c->register_index || register_index - c->register_index >= c->register_count)
            continue;

        vkd3d_string_buffer_printf(&parser->buffer, "%s", &code->ctab[c->name]);

        /* Register offset within variable */
        offset = arg->address - c->register_index * 4;

        if (offset / 4)
            vkd3d_string_buffer_printf(&parser->buffer, "[%u]", offset / 4);
        fx_4_parse_print_swizzle(parser, code, offset);
        return;
    }

    vkd3d_string_buffer_printf(&parser->buffer, "(var-not-found)");
}

static void fx_4_parse_fxlc_argument(struct fx_parser *parser, uint32_t offset, const struct fxlvm_code *code)
{
    struct fx_4_fxlc_argument arg;
    uint32_t count;

    fx_parser_read_unstructured(parser, &arg, offset, sizeof(arg));

    switch (arg.reg_type)
    {
        case FX_4_FXLC_REG_LITERAL:
            count = code->scalar ? 1 : code->comp_count;
            if (arg.address >= code->cli4_count || count > code->cli4_count - arg.address)
            {
                vkd3d_string_buffer_printf(&parser->buffer, "(<out-of-bounds>)");
                parser->failed = true;
                break;
            }

            vkd3d_string_buffer_printf(&parser->buffer, "(");
            vkd3d_string_buffer_print_f32(&parser->buffer, code->cli4[arg.address]);
            for (unsigned int i = 1; i < code->comp_count; ++i)
            {
                vkd3d_string_buffer_printf(&parser->buffer, ", ");
                vkd3d_string_buffer_print_f32(&parser->buffer, code->cli4[arg.address + (code->scalar ? 0 : i)]);
            }
            vkd3d_string_buffer_printf(&parser->buffer, ")");
            break;

        case FX_4_FXLC_REG_CB:
            fx_4_parse_fxlc_constant_argument(parser, &arg, code);
            break;

        case FX_4_FXLC_REG_OUTPUT:
        case FX_4_FXLC_REG_TEMP:
            if (arg.reg_type == FX_4_FXLC_REG_OUTPUT)
                vkd3d_string_buffer_printf(&parser->buffer, "expr");
            else
                vkd3d_string_buffer_printf(&parser->buffer, "r%u", arg.address / 4);
            fx_4_parse_print_swizzle(parser, code, arg.address);
            break;

        default:
            vkd3d_string_buffer_printf(&parser->buffer, "<unknown register %u>", arg.reg_type);
            break;
    }
}

static void fx_4_parse_fxlvm_expression(struct fx_parser *parser, uint32_t offset)
{
    struct vkd3d_shader_dxbc_section_desc *section, fxlc, cli4, ctab;
    struct vkd3d_shader_dxbc_desc dxbc_desc;
    struct vkd3d_shader_code dxbc;
    uint32_t size, ins_count;
    struct fxlvm_code code;
    size_t i, j;

    offset = fx_parser_read_unstructured(parser, &size, offset, sizeof(size));

    dxbc.size = size;
    dxbc.code = fx_parser_get_unstructured_ptr(parser, offset, size);
    if (!dxbc.code)
        return;

    if (vkd3d_shader_parse_dxbc(&dxbc, 0, &dxbc_desc, NULL) < 0)
    {
        parser->failed = true;
        return;
    }

    memset(&fxlc, 0, sizeof(fxlc));
    memset(&cli4, 0, sizeof(cli4));
    memset(&ctab, 0, sizeof(ctab));
    for (i = 0; i < dxbc_desc.section_count; ++i)
    {
        section = &dxbc_desc.sections[i];

        if (section->tag == TAG_FXLC)
            fxlc = *section;
        else if (section->tag == TAG_CLI4)
            cli4 = *section;
        else if (section->tag == TAG_CTAB)
            ctab = *section;
    }

    vkd3d_shader_free_dxbc(&dxbc_desc);

    if (cli4.data.code)
    {
        uint32_t cli4_offset = offset + (size_t)cli4.data.code - (size_t)dxbc.code;

        fx_parser_read_unstructured(parser, &code.cli4_count, cli4_offset, sizeof(code.cli4_count));
        code.cli4 = fx_parser_get_unstructured_ptr(parser, cli4_offset + 4, code.cli4_count * sizeof(float));
    }

    if (ctab.data.code)
    {
        uint32_t ctab_offset = offset + (size_t)ctab.data.code - (size_t)dxbc.code;
        uint32_t consts_offset;

        fx_parser_read_unstructured(parser, &code.ctab_count, ctab_offset + 12, sizeof(code.ctab_count));
        fx_parser_read_unstructured(parser, &consts_offset, ctab_offset + 16, sizeof(consts_offset));

        code.ctab = ctab.data.code;
        code.constants = fx_parser_get_unstructured_ptr(parser,
                ctab_offset + consts_offset, code.ctab_count * sizeof(*code.constants));
    }

    offset += (size_t)fxlc.data.code - (size_t)dxbc.code;
    offset = fx_parser_read_unstructured(parser, &ins_count, offset, sizeof(ins_count));

    parse_fx_start_indent(parser);

    for (i = 0; i < ins_count; ++i)
    {
        uint32_t instr, opcode, src_count;
        struct fx_4_fxlc_argument arg;

        offset = fx_parser_read_unstructured(parser, &instr, offset, sizeof(instr));
        offset = fx_parser_read_unstructured(parser, &src_count, offset, sizeof(src_count));

        opcode = (instr >> FX_4_FXLC_OPCODE_SHIFT) & FX_4_FXLC_OPCODE_MASK;
        code.comp_count = instr & FX_4_FXLC_COMP_COUNT_MASK;
        code.scalar = false;

        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "%s ", fx_4_get_fxlc_opcode_name(opcode));

        /* Destination first. */
        fx_4_parse_fxlc_argument(parser, offset + sizeof(arg) * src_count, &code);

        for (j = 0; j < src_count; ++j)
        {
            vkd3d_string_buffer_printf(&parser->buffer, ", ");

            /* Scalar modifier applies only to first source. */
            code.scalar = j == 0 && !!(instr & FX_4_FXLC_IS_SCALAR_MASK);
            fx_4_parse_fxlc_argument(parser, offset, &code);

            offset += sizeof(arg);
        }

        /* Destination */
        offset += sizeof(arg);

        vkd3d_string_buffer_printf(&parser->buffer, "\n");
    }

    parse_fx_end_indent(parser);
}

static void fx_4_parse_state_object_initializer(struct fx_parser *parser, uint32_t count,
        enum hlsl_type_class type_class)
{
    struct fx_assignment entry;
    struct
    {
        uint32_t name;
        uint32_t index;
    } index;
    struct
    {
        uint32_t type;
        union
        {
            uint32_t u;
            float f;
        };
    } value;
    static const char *const value_types[FX_COMPONENT_TYPE_COUNT] =
    {
        [FX_BOOL]  = "bool",
        [FX_FLOAT] = "float",
        [FX_UINT]  = "uint",
        [FX_UINT8] = "byte",
    };
    const struct rhs_named_value *named_value;
    struct fx_5_shader shader = { 0 };
    struct fx_4_state_table table;
    unsigned int shader_type = 0;
    uint32_t i, j, comp_count;
    struct fx_4_state *state;

    table = fx_4_get_state_table(type_class, parser->version.major, parser->version.minor);

    for (i = 0; i < count; ++i)
    {
        fx_parser_read_u32s(parser, &entry, sizeof(entry));

        if (!(state = bsearch(&entry.id, table.ptr, table.count, sizeof(*table.ptr), fx_4_state_id_compare)))
        {
            fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_DATA, "Unrecognized state id %#x.", entry.id);
            break;
        }

        if (state->container != type_class)
        {
            fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_DATA,
                    "State '%s' does not belong to object type class %#x.", state->name, type_class);
            break;
        }

        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "%s", state->name);
        if (state->array_size > 1)
            vkd3d_string_buffer_printf(&parser->buffer, "[%u]", entry.lhs_index);
        vkd3d_string_buffer_printf(&parser->buffer, " = ");

        switch (entry.type)
        {
            case FX_4_ASSIGNMENT_CONSTANT:

                if (value_types[state->type])
                    vkd3d_string_buffer_printf(&parser->buffer, "%s", value_types[state->type]);
                if (state->dimx > 1)
                    vkd3d_string_buffer_printf(&parser->buffer, "%u", state->dimx);
                vkd3d_string_buffer_printf(&parser->buffer, "(");

                fx_parser_read_unstructured(parser, &comp_count, entry.value, sizeof(uint32_t));

                named_value = NULL;
                if (comp_count == 1 && state->values && (state->type == FX_UINT || state->type == FX_BOOL))
                {
                    const struct rhs_named_value *ptr = state->values;

                    fx_parser_read_unstructured(parser, &value, entry.value + 4, sizeof(value));

                    while (ptr->name)
                    {
                        if (value.u == ptr->value)
                        {
                            named_value = ptr;
                            break;
                        }
                        ++ptr;
                    }
                }

                if (named_value)
                {
                    vkd3d_string_buffer_printf(&parser->buffer, "%s /* %u */", named_value->name, named_value->value);
                }
                else
                {
                    uint32_t offset = entry.value + 4;

                    for (j = 0; j < comp_count; ++j, offset += sizeof(value))
                    {
                        fx_parser_read_unstructured(parser, &value, offset, sizeof(value));

                        if (state->type == FX_UINT8)
                            vkd3d_string_buffer_printf(&parser->buffer, "0x%.2x", value.u);
                        else if (state->type == FX_UINT)
                            vkd3d_string_buffer_printf(&parser->buffer, "%u", value.u);
                        else if (state->type == FX_FLOAT)
                            vkd3d_string_buffer_printf(&parser->buffer, "%g", value.f);

                        if (comp_count > 1 && j < comp_count - 1)
                            vkd3d_string_buffer_printf(&parser->buffer, ", ");
                    }
                }

                vkd3d_string_buffer_printf(&parser->buffer, ")");

                break;
            case FX_4_ASSIGNMENT_VARIABLE:
                vkd3d_string_buffer_printf(&parser->buffer, "%s", fx_4_get_string(parser, entry.value));
                break;
            case FX_4_ASSIGNMENT_ARRAY_CONSTANT_INDEX:
                fx_parser_read_unstructured(parser, &index, entry.value, sizeof(index));
                vkd3d_string_buffer_printf(&parser->buffer, "%s[%u]", fx_4_get_string(parser, index.name), index.index);
                break;
            case FX_4_ASSIGNMENT_ARRAY_VARIABLE_INDEX:
                fx_parser_read_unstructured(parser, &index, entry.value, sizeof(index));
                vkd3d_string_buffer_printf(&parser->buffer, "%s[%s]", fx_4_get_string(parser, index.name),
                        fx_4_get_string(parser, index.index));
                break;
            case FX_4_ASSIGNMENT_INDEX_EXPRESSION:
                fx_parser_read_unstructured(parser, &index, entry.value, sizeof(index));
                vkd3d_string_buffer_printf(&parser->buffer, "%s[eval(\n", fx_4_get_string(parser, index.name));
                fx_4_parse_fxlvm_expression(parser, index.index);
                parse_fx_print_indent(parser);
                vkd3d_string_buffer_printf(&parser->buffer, ")]");
                break;
            case FX_4_ASSIGNMENT_VALUE_EXPRESSION:
                vkd3d_string_buffer_printf(&parser->buffer, "eval(\n");
                fx_4_parse_fxlvm_expression(parser, entry.value);
                parse_fx_print_indent(parser);
                vkd3d_string_buffer_printf(&parser->buffer, ")");
                break;
            case FX_4_ASSIGNMENT_INLINE_SHADER:
            case FX_5_ASSIGNMENT_INLINE_SHADER:
            {
                bool shader5 = entry.type == FX_5_ASSIGNMENT_INLINE_SHADER;

                if (shader5)
                    fx_parser_read_unstructured(parser, &shader, entry.value, sizeof(shader));
                else
                    fx_parser_read_unstructured(parser, &shader, entry.value, 2 * sizeof(uint32_t));

                if (state->type == FX_PIXELSHADER)
                    shader_type = FX_4_OBJECT_TYPE_PIXEL_SHADER;
                else if (state->type == FX_VERTEXSHADER)
                    shader_type = FX_4_OBJECT_TYPE_VERTEX_SHADER;
                else if (state->type == FX_GEOMETRYSHADER)
                    shader_type = shader5 ? FX_5_OBJECT_TYPE_GEOMETRY_SHADER : FX_4_OBJECT_TYPE_GEOMETRY_SHADER_SO;
                else if (state->type == FX_HULLSHADER)
                    shader_type = FX_5_OBJECT_TYPE_HULL_SHADER;
                else if (state->type == FX_DOMAINSHADER)
                    shader_type = FX_5_OBJECT_TYPE_DOMAIN_SHADER;
                else if (state->type == FX_COMPUTESHADER)
                    shader_type = FX_5_OBJECT_TYPE_COMPUTE_SHADER;

                vkd3d_string_buffer_printf(&parser->buffer, "\n");
                parse_fx_start_indent(parser);
                fx_4_parse_shader_blob(parser, shader_type, &shader);
                parse_fx_end_indent(parser);
                break;
            }
            default:
                fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_NOT_IMPLEMENTED,
                        "Unsupported assignment type %u.", entry.type);
        }
        vkd3d_string_buffer_printf(&parser->buffer, ";\n");
    }
}

static void fx_4_parse_object_initializer(struct fx_parser *parser, const struct fx_4_binary_type *type)
{
    static const enum hlsl_type_class type_classes[] =
    {
        [FX_4_OBJECT_TYPE_BLEND_STATE]         = HLSL_CLASS_BLEND_STATE,
        [FX_4_OBJECT_TYPE_DEPTH_STENCIL_STATE] = HLSL_CLASS_DEPTH_STENCIL_STATE,
        [FX_4_OBJECT_TYPE_RASTERIZER_STATE]    = HLSL_CLASS_RASTERIZER_STATE,
        [FX_4_OBJECT_TYPE_SAMPLER_STATE]       = HLSL_CLASS_SAMPLER,
    };
    unsigned int i, element_count, count;
    uint32_t value;
    bool is_array;

    if (!fx_4_object_has_initializer(type))
        return;

    vkd3d_string_buffer_printf(&parser->buffer, " = {\n");
    element_count = max(type->element_count, 1);
    is_array = element_count > 1;
    for (i = 0; i < element_count; ++i)
    {
        switch (type->typeinfo)
        {
            case FX_4_OBJECT_TYPE_STRING:
                vkd3d_string_buffer_printf(&parser->buffer, "    ");
                value = fx_parser_read_u32(parser);
                fx_4_parse_string_initializer(parser, value);
                break;
            case FX_4_OBJECT_TYPE_BLEND_STATE:
            case FX_4_OBJECT_TYPE_DEPTH_STENCIL_STATE:
            case FX_4_OBJECT_TYPE_RASTERIZER_STATE:
            case FX_4_OBJECT_TYPE_SAMPLER_STATE:
                count = fx_parser_read_u32(parser);

                if (is_array)
                {
                    parse_fx_start_indent(parser);
                    parse_fx_print_indent(parser);
                    vkd3d_string_buffer_printf(&parser->buffer, "{\n");
                }
                parse_fx_start_indent(parser);
                fx_4_parse_state_object_initializer(parser, count, type_classes[type->typeinfo]);
                parse_fx_end_indent(parser);
                if (is_array)
                {
                    parse_fx_print_indent(parser);
                    vkd3d_string_buffer_printf(&parser->buffer, "}");
                    parse_fx_end_indent(parser);
                }
                break;
            case FX_4_OBJECT_TYPE_PIXEL_SHADER:
            case FX_4_OBJECT_TYPE_VERTEX_SHADER:
            case FX_4_OBJECT_TYPE_GEOMETRY_SHADER:
            case FX_4_OBJECT_TYPE_GEOMETRY_SHADER_SO:
            case FX_5_OBJECT_TYPE_GEOMETRY_SHADER:
            case FX_5_OBJECT_TYPE_COMPUTE_SHADER:
            case FX_5_OBJECT_TYPE_HULL_SHADER:
            case FX_5_OBJECT_TYPE_DOMAIN_SHADER:
                parse_fx_start_indent(parser);
                fx_4_parse_shader_initializer(parser, type->typeinfo);
                parse_fx_end_indent(parser);
                break;
            default:
                fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_NOT_IMPLEMENTED,
                        "Parsing object type %u is not implemented.", type->typeinfo);
                return;
        }
        vkd3d_string_buffer_printf(&parser->buffer, is_array ? ",\n" : "\n");
    }
    vkd3d_string_buffer_printf(&parser->buffer, "}");
}

static void fx_4_parse_objects(struct fx_parser *parser)
{
    struct fx_4_object_variable
    {
        uint32_t name;
        uint32_t type;
        uint32_t semantic;
        uint32_t bind_point;
    } var;
    struct fx_4_binary_type type;
    const char *name, *type_name;
    uint32_t i;

    if (parser->failed)
        return;

    for (i = 0; i < parser->object_count; ++i)
    {
        if (parser->failed)
            return;

        fx_parser_read_u32s(parser, &var, sizeof(var));
        fx_parser_read_unstructured(parser, &type, var.type, sizeof(type));

        name = fx_4_get_string(parser, var.name);
        type_name = fx_4_get_string(parser, type.name);
        vkd3d_string_buffer_printf(&parser->buffer, "%s %s", type_name, name);
        if (type.element_count)
            vkd3d_string_buffer_printf(&parser->buffer, "[%u]", type.element_count);

        fx_4_parse_object_initializer(parser, &type);
        vkd3d_string_buffer_printf(&parser->buffer, ";\n");

        fx_parse_fx_4_annotations(parser);
    }
}

static void fx_parse_fx_4_technique(struct fx_parser *parser)
{
    struct fx_technique
    {
        uint32_t name;
        uint32_t count;
    } technique;
    struct fx_pass
    {
        uint32_t name;
        uint32_t count;
    } pass;
    const char *name;
    uint32_t i;

    if (parser->failed)
        return;

    fx_parser_read_u32s(parser, &technique, sizeof(technique));

    name = fx_4_get_string(parser, technique.name);

    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "technique%u %s", parser->version.major == 4 ? 10 : 11, name);
    fx_parse_fx_4_annotations(parser);

    vkd3d_string_buffer_printf(&parser->buffer, "\n");
    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "{\n");

    parse_fx_start_indent(parser);
    for (i = 0; i < technique.count; ++i)
    {
        fx_parser_read_u32s(parser, &pass, sizeof(pass));
        name = fx_4_get_string(parser, pass.name);

        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "pass %s", name);
        fx_parse_fx_4_annotations(parser);

        vkd3d_string_buffer_printf(&parser->buffer, "\n");
        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "{\n");

        parse_fx_start_indent(parser);
        fx_4_parse_state_object_initializer(parser, pass.count, HLSL_CLASS_PASS);
        parse_fx_end_indent(parser);

        parse_fx_print_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "}\n\n");
    }

    parse_fx_end_indent(parser);

    parse_fx_print_indent(parser);
    vkd3d_string_buffer_printf(&parser->buffer, "}\n\n");
}

static void fx_parse_groups(struct fx_parser *parser)
{
    struct fx_group
    {
        uint32_t name;
        uint32_t count;
    } group;
    const char *name;
    uint32_t i, j;

    if (parser->failed)
        return;

    for (i = 0; i < parser->group_count; ++i)
    {
        fx_parser_read_u32s(parser, &group, sizeof(group));

        name = fx_4_get_string(parser, group.name);

        vkd3d_string_buffer_printf(&parser->buffer, "fxgroup %s", name);
        fx_parse_fx_4_annotations(parser);

        vkd3d_string_buffer_printf(&parser->buffer, "\n{\n");
        parse_fx_start_indent(parser);

        for (j = 0; j < group.count; ++j)
            fx_parse_fx_4_technique(parser);

        parse_fx_end_indent(parser);
        vkd3d_string_buffer_printf(&parser->buffer, "}\n\n");
    }
}

static void fx_4_parse(struct fx_parser *parser)
{
    struct fx_4_header
    {
        uint32_t version;
        uint32_t buffer_count;
        uint32_t numeric_variable_count;
        uint32_t object_count;
        uint32_t shared_buffer_count;
        uint32_t shared_numeric_variable_count;
        uint32_t shared_object_count;
        uint32_t technique_count;
        uint32_t unstructured_size;
        uint32_t string_count;
        uint32_t texture_count;
        uint32_t depth_stencil_state_count;
        uint32_t blend_state_count;
        uint32_t rasterizer_state_count;
        uint32_t sampler_state_count;
        uint32_t rtv_count;
        uint32_t dsv_count;
        uint32_t shader_count;
        uint32_t inline_shader_count;
    } header;
    uint32_t i;

    fx_parser_read_u32s(parser, &header, sizeof(header));
    parser->buffer_count = header.buffer_count;
    parser->object_count = header.object_count;

    if (parser->end - parser->ptr < header.unstructured_size)
    {
        fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_SIZE,
                "Invalid unstructured data size %u.", header.unstructured_size);
        return;
    }

    parser->unstructured.ptr = parser->ptr;
    parser->unstructured.end = parser->ptr + header.unstructured_size;
    parser->unstructured.size = header.unstructured_size;
    fx_parser_skip(parser, header.unstructured_size);

    fx_parse_buffers(parser);
    fx_4_parse_objects(parser);

    for (i = 0; i < header.technique_count; ++i)
        fx_parse_fx_4_technique(parser);
}

static void fx_5_parse(struct fx_parser *parser)
{
    struct fx_5_header
    {
        uint32_t version;
        uint32_t buffer_count;
        uint32_t numeric_variable_count;
        uint32_t object_count;
        uint32_t shared_buffer_count;
        uint32_t shared_numeric_variable_count;
        uint32_t shared_object_count;
        uint32_t technique_count;
        uint32_t unstructured_size;
        uint32_t string_count;
        uint32_t texture_count;
        uint32_t depth_stencil_state_count;
        uint32_t blend_state_count;
        uint32_t rasterizer_state_count;
        uint32_t sampler_state_count;
        uint32_t rtv_count;
        uint32_t dsv_count;
        uint32_t shader_count;
        uint32_t inline_shader_count;
        uint32_t group_count;
        uint32_t uav_count;
        uint32_t interface_variable_count;
        uint32_t interface_variable_element_count;
        uint32_t class_instance_element_count;
    } header;

    fx_parser_read_u32s(parser, &header, sizeof(header));
    parser->buffer_count = header.buffer_count;
    parser->object_count = header.object_count;
    parser->group_count = header.group_count;

    if (parser->end - parser->ptr < header.unstructured_size)
    {
        fx_parser_error(parser, VKD3D_SHADER_ERROR_FX_INVALID_SIZE,
                "Invalid unstructured data size %u.", header.unstructured_size);
        return;
    }

    parser->unstructured.ptr = parser->ptr;
    parser->unstructured.end = parser->ptr + header.unstructured_size;
    parser->unstructured.size = header.unstructured_size;
    fx_parser_skip(parser, header.unstructured_size);

    fx_parse_buffers(parser);
    fx_4_parse_objects(parser);

    fx_parse_groups(parser);
}

static void fx_parser_init(struct fx_parser *parser, const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context)
{
    memset(parser, 0, sizeof(*parser));
    parser->start = compile_info->source.code;
    parser->ptr = compile_info->source.code;
    parser->end = (uint8_t *)compile_info->source.code + compile_info->source.size;
    parser->message_context = message_context;
    vkd3d_string_buffer_init(&parser->buffer);
}

static void fx_parser_cleanup(struct fx_parser *parser)
{
    free(parser->objects.types);
}

int fx_parse(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    struct fx_parser parser;
    uint32_t version;

    fx_parser_init(&parser, compile_info, message_context);

    if (parser.end - parser.start < sizeof(version))
    {
        fx_parser_error(&parser, VKD3D_SHADER_ERROR_FX_INVALID_SIZE,
                "Source size %zu is smaller than the FX header size.", compile_info->source.size);
        return VKD3D_ERROR_INVALID_SHADER;
    }
    version = *(uint32_t *)parser.ptr;

    switch (version)
    {
        case 0xfeff0901:
            parser.version.major = 3;
            fx_2_parse(&parser);
            break;
        case 0xfeff1001:
            parser.version.major = 4;
            fx_4_parse(&parser);
            break;
        case 0xfeff1011:
            parser.version.major = 4;
            parser.version.minor = 1;
            fx_4_parse(&parser);
            break;
        case 0xfeff2001:
            parser.version.major = 5;
            fx_5_parse(&parser);
            break;
        default:
            fx_parser_error(&parser, VKD3D_SHADER_ERROR_FX_INVALID_VERSION,
                    "Invalid effect binary version value 0x%08x.", version);
            break;
    }

    vkd3d_shader_code_from_string_buffer(out, &parser.buffer);
    fx_parser_cleanup(&parser);

    if (parser.failed)
        return VKD3D_ERROR_INVALID_SHADER;
    return VKD3D_OK;
}
