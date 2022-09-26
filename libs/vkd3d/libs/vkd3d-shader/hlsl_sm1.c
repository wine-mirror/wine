/*
 * HLSL code generation for DXBC shader models 1-3
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

bool hlsl_sm1_register_from_semantic(struct hlsl_ctx *ctx, const struct hlsl_semantic *semantic,
        bool output, D3DSHADER_PARAM_REGISTER_TYPE *type, unsigned int *reg)
{
    unsigned int i;

    static const struct
    {
        const char *semantic;
        bool output;
        enum vkd3d_shader_type shader_type;
        unsigned int major_version;
        D3DSHADER_PARAM_REGISTER_TYPE type;
        DWORD offset;
    }
    register_table[] =
    {
        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_COLOROUT},
        {"depth",       true,  VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_DEPTHOUT},
        {"sv_depth",    true,  VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_DEPTHOUT},
        {"sv_target",   true,  VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_COLOROUT},
        {"color",       false, VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_INPUT},
        {"texcoord",    false, VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_TEXTURE},

        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_COLOROUT},
        {"depth",       true,  VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_DEPTHOUT},
        {"sv_depth",    true,  VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_DEPTHOUT},
        {"sv_target",   true,  VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_COLOROUT},
        {"sv_position", false, VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_MISCTYPE,    D3DSMO_POSITION},
        {"vface",       false, VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_MISCTYPE,    D3DSMO_FACE},
        {"vpos",        false, VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_MISCTYPE,    D3DSMO_POSITION},

        {"color",       true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_ATTROUT},
        {"fog",         true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_RASTOUT,     D3DSRO_FOG},
        {"position",    true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_RASTOUT,     D3DSRO_POSITION},
        {"psize",       true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_RASTOUT,     D3DSRO_POINT_SIZE},
        {"sv_position", true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_RASTOUT,     D3DSRO_POSITION},
        {"texcoord",    true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_TEXCRDOUT},

        {"color",       true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_ATTROUT},
        {"fog",         true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_RASTOUT,     D3DSRO_FOG},
        {"position",    true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_RASTOUT,     D3DSRO_POSITION},
        {"psize",       true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_RASTOUT,     D3DSRO_POINT_SIZE},
        {"sv_position", true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_RASTOUT,     D3DSRO_POSITION},
        {"texcoord",    true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_TEXCRDOUT},
    };

    for (i = 0; i < ARRAY_SIZE(register_table); ++i)
    {
        if (!ascii_strcasecmp(semantic->name, register_table[i].semantic)
                && output == register_table[i].output
                && ctx->profile->type == register_table[i].shader_type
                && ctx->profile->major_version == register_table[i].major_version)
        {
            *type = register_table[i].type;
            if (register_table[i].type == D3DSPR_MISCTYPE || register_table[i].type == D3DSPR_RASTOUT)
                *reg = register_table[i].offset;
            else
                *reg = semantic->index;
            return true;
        }
    }

    return false;
}

bool hlsl_sm1_usage_from_semantic(const struct hlsl_semantic *semantic, D3DDECLUSAGE *usage, uint32_t *usage_idx)
{
    static const struct
    {
        const char *name;
        D3DDECLUSAGE usage;
    }
    semantics[] =
    {
        {"binormal",        D3DDECLUSAGE_BINORMAL},
        {"blendindices",    D3DDECLUSAGE_BLENDINDICES},
        {"blendweight",     D3DDECLUSAGE_BLENDWEIGHT},
        {"color",           D3DDECLUSAGE_COLOR},
        {"depth",           D3DDECLUSAGE_DEPTH},
        {"fog",             D3DDECLUSAGE_FOG},
        {"normal",          D3DDECLUSAGE_NORMAL},
        {"position",        D3DDECLUSAGE_POSITION},
        {"positiont",       D3DDECLUSAGE_POSITIONT},
        {"psize",           D3DDECLUSAGE_PSIZE},
        {"sample",          D3DDECLUSAGE_SAMPLE},
        {"sv_depth",        D3DDECLUSAGE_DEPTH},
        {"sv_position",     D3DDECLUSAGE_POSITION},
        {"sv_target",       D3DDECLUSAGE_COLOR},
        {"tangent",         D3DDECLUSAGE_TANGENT},
        {"tessfactor",      D3DDECLUSAGE_TESSFACTOR},
        {"texcoord",        D3DDECLUSAGE_TEXCOORD},
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(semantics); ++i)
    {
        if (!ascii_strcasecmp(semantic->name, semantics[i].name))
        {
            *usage = semantics[i].usage;
            *usage_idx = semantic->index;
            return true;
        }
    }

    return false;
}

static uint32_t sm1_version(enum vkd3d_shader_type type, unsigned int major, unsigned int minor)
{
    if (type == VKD3D_SHADER_TYPE_VERTEX)
        return D3DVS_VERSION(major, minor);
    else
        return D3DPS_VERSION(major, minor);
}

static D3DXPARAMETER_CLASS sm1_class(const struct hlsl_type *type)
{
    switch (type->type)
    {
        case HLSL_CLASS_ARRAY:
            return sm1_class(type->e.array.type);
        case HLSL_CLASS_MATRIX:
            assert(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);
            if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
                return D3DXPC_MATRIX_COLUMNS;
            else
                return D3DXPC_MATRIX_ROWS;
        case HLSL_CLASS_OBJECT:
            return D3DXPC_OBJECT;
        case HLSL_CLASS_SCALAR:
            return D3DXPC_SCALAR;
        case HLSL_CLASS_STRUCT:
            return D3DXPC_STRUCT;
        case HLSL_CLASS_VECTOR:
            return D3DXPC_VECTOR;
        default:
            ERR("Invalid class %#x.\n", type->type);
            assert(0);
            return 0;
    }
}

static D3DXPARAMETER_TYPE sm1_base_type(const struct hlsl_type *type)
{
    switch (type->base_type)
    {
        case HLSL_TYPE_BOOL:
            return D3DXPT_BOOL;
        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_HALF:
            return D3DXPT_FLOAT;
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
            return D3DXPT_INT;
        case HLSL_TYPE_PIXELSHADER:
            return D3DXPT_PIXELSHADER;
        case HLSL_TYPE_SAMPLER:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                    return D3DXPT_SAMPLER1D;
                case HLSL_SAMPLER_DIM_2D:
                    return D3DXPT_SAMPLER2D;
                case HLSL_SAMPLER_DIM_3D:
                    return D3DXPT_SAMPLER3D;
                case HLSL_SAMPLER_DIM_CUBE:
                    return D3DXPT_SAMPLERCUBE;
                case HLSL_SAMPLER_DIM_GENERIC:
                    return D3DXPT_SAMPLER;
                default:
                    ERR("Invalid dimension %#x.\n", type->sampler_dim);
            }
            break;
        case HLSL_TYPE_STRING:
            return D3DXPT_STRING;
        case HLSL_TYPE_TEXTURE:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                    return D3DXPT_TEXTURE1D;
                case HLSL_SAMPLER_DIM_2D:
                    return D3DXPT_TEXTURE2D;
                case HLSL_SAMPLER_DIM_3D:
                    return D3DXPT_TEXTURE3D;
                case HLSL_SAMPLER_DIM_CUBE:
                    return D3DXPT_TEXTURECUBE;
                case HLSL_SAMPLER_DIM_GENERIC:
                    return D3DXPT_TEXTURE;
                default:
                    ERR("Invalid dimension %#x.\n", type->sampler_dim);
            }
            break;
        case HLSL_TYPE_VERTEXSHADER:
            return D3DXPT_VERTEXSHADER;
        case HLSL_TYPE_VOID:
            return D3DXPT_VOID;
        default:
            assert(0);
    }
    assert(0);
    return 0;
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

static void write_sm1_type(struct vkd3d_bytecode_buffer *buffer, struct hlsl_type *type, unsigned int ctab_start)
{
    const struct hlsl_type *array_type = get_array_type(type);
    unsigned int array_size = get_array_size(type);
    unsigned int field_count = 0;
    size_t fields_offset = 0;
    size_t i;

    if (type->bytecode_offset)
        return;

    if (array_type->type == HLSL_CLASS_STRUCT)
    {
        field_count = array_type->e.record.field_count;

        for (i = 0; i < field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            field->name_bytecode_offset = put_string(buffer, field->name);
            write_sm1_type(buffer, field->type, ctab_start);
        }

        fields_offset = bytecode_get_size(buffer) - ctab_start;

        for (i = 0; i < field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            put_u32(buffer, field->name_bytecode_offset - ctab_start);
            put_u32(buffer, field->type->bytecode_offset - ctab_start);
        }
    }

    type->bytecode_offset = put_u32(buffer, vkd3d_make_u32(sm1_class(type), sm1_base_type(type)));
    put_u32(buffer, vkd3d_make_u32(type->dimy, type->dimx));
    put_u32(buffer, vkd3d_make_u32(array_size, field_count));
    put_u32(buffer, fields_offset);
}

static void sm1_sort_extern(struct list *sorted, struct hlsl_ir_var *to_sort)
{
    struct hlsl_ir_var *var;

    list_remove(&to_sort->extern_entry);

    LIST_FOR_EACH_ENTRY(var, sorted, struct hlsl_ir_var, extern_entry)
    {
        if (strcmp(to_sort->name, var->name) < 0)
        {
            list_add_before(&var->extern_entry, &to_sort->extern_entry);
            return;
        }
    }

    list_add_tail(sorted, &to_sort->extern_entry);
}

static void sm1_sort_externs(struct hlsl_ctx *ctx)
{
    struct list sorted = LIST_INIT(sorted);
    struct hlsl_ir_var *var, *next;

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        sm1_sort_extern(&sorted, var);
    list_move_tail(&ctx->extern_vars, &sorted);
}

static void write_sm1_uniforms(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        struct hlsl_ir_function_decl *entry_func)
{
    size_t ctab_offset, ctab_start, ctab_end, vars_start, size_offset, creator_offset, offset;
    unsigned int uniform_count = 0;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->semantic.name && var->reg.allocated)
        {
            ++uniform_count;

            if (var->is_param && var->is_uniform)
            {
                struct vkd3d_string_buffer *name;

                if (!(name = hlsl_get_string_buffer(ctx)))
                {
                    buffer->status = VKD3D_ERROR_OUT_OF_MEMORY;
                    return;
                }
                vkd3d_string_buffer_printf(name, "$%s", var->name);
                vkd3d_free((char *)var->name);
                var->name = hlsl_strdup(ctx, name->buffer);
                hlsl_release_string_buffer(ctx, name);
            }
        }
    }

    sm1_sort_externs(ctx);

    size_offset = put_u32(buffer, 0);
    ctab_offset = put_u32(buffer, VKD3D_MAKE_TAG('C','T','A','B'));

    ctab_start = put_u32(buffer, sizeof(D3DXSHADER_CONSTANTTABLE));
    creator_offset = put_u32(buffer, 0);
    put_u32(buffer, sm1_version(ctx->profile->type, ctx->profile->major_version, ctx->profile->minor_version));
    put_u32(buffer, uniform_count);
    put_u32(buffer, sizeof(D3DXSHADER_CONSTANTTABLE)); /* offset of constants */
    put_u32(buffer, 0); /* FIXME: flags */
    put_u32(buffer, 0); /* FIXME: target string */

    vars_start = bytecode_get_size(buffer);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->semantic.name && var->reg.allocated)
        {
            put_u32(buffer, 0); /* name */
            if (var->data_type->type == HLSL_CLASS_OBJECT
                    && (var->data_type->base_type == HLSL_TYPE_SAMPLER
                    || var->data_type->base_type == HLSL_TYPE_TEXTURE))
            {
                put_u32(buffer, vkd3d_make_u32(D3DXRS_SAMPLER, var->reg.id));
                put_u32(buffer, 1);
            }
            else
            {
                put_u32(buffer, vkd3d_make_u32(D3DXRS_FLOAT4, var->reg.id));
                put_u32(buffer, var->data_type->reg_size / 4);
            }
            put_u32(buffer, 0); /* type */
            put_u32(buffer, 0); /* FIXME: default value */
        }
    }

    uniform_count = 0;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->semantic.name && var->reg.allocated)
        {
            size_t var_offset = vars_start + (uniform_count * 5 * sizeof(uint32_t));
            size_t name_offset;

            name_offset = put_string(buffer, var->name);
            set_u32(buffer, var_offset, name_offset - ctab_start);

            write_sm1_type(buffer, var->data_type, ctab_start);
            set_u32(buffer, var_offset + 3 * sizeof(uint32_t), var->data_type->bytecode_offset - ctab_start);
            ++uniform_count;
        }
    }

    offset = put_string(buffer, vkd3d_shader_get_version(NULL, NULL));
    set_u32(buffer, creator_offset, offset - ctab_start);

    ctab_end = bytecode_get_size(buffer);
    set_u32(buffer, size_offset, vkd3d_make_u32(D3DSIO_COMMENT, (ctab_end - ctab_offset) / sizeof(uint32_t)));
}

static uint32_t sm1_encode_register_type(D3DSHADER_PARAM_REGISTER_TYPE type)
{
    return ((type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK)
            | ((type << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2);
}

struct sm1_instruction
{
    D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode;

    struct sm1_dst_register
    {
        D3DSHADER_PARAM_REGISTER_TYPE type;
        D3DSHADER_PARAM_DSTMOD_TYPE mod;
        unsigned int writemask;
        uint32_t reg;
    } dst;

    struct sm1_src_register
    {
        D3DSHADER_PARAM_REGISTER_TYPE type;
        D3DSHADER_PARAM_SRCMOD_TYPE mod;
        unsigned int swizzle;
        uint32_t reg;
    } srcs[2];
    unsigned int src_count;

    unsigned int has_dst;
};

static void write_sm1_dst_register(struct vkd3d_bytecode_buffer *buffer, const struct sm1_dst_register *reg)
{
    assert(reg->writemask);
    put_u32(buffer, (1u << 31) | sm1_encode_register_type(reg->type) | reg->mod | (reg->writemask << 16) | reg->reg);
}

static void write_sm1_src_register(struct vkd3d_bytecode_buffer *buffer,
        const struct sm1_src_register *reg, unsigned int dst_writemask)
{
    unsigned int swizzle = hlsl_map_swizzle(reg->swizzle, dst_writemask);

    put_u32(buffer, (1u << 31) | sm1_encode_register_type(reg->type) | reg->mod | (swizzle << 16) | reg->reg);
}

static void write_sm1_instruction(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct sm1_instruction *instr)
{
    uint32_t token = instr->opcode;
    unsigned int i;

    if (ctx->profile->major_version > 1)
        token |= (instr->has_dst + instr->src_count) << D3DSI_INSTLENGTH_SHIFT;
    put_u32(buffer, token);

    if (instr->has_dst)
        write_sm1_dst_register(buffer, &instr->dst);

    for (i = 0; i < instr->src_count; ++i)
        write_sm1_src_register(buffer, &instr->srcs[i], instr->dst.writemask);
};

static void write_sm1_binary_op(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode, const struct hlsl_reg *dst,
        const struct hlsl_reg *src1, const struct hlsl_reg *src2)
{
    const struct sm1_instruction instr =
    {
        .opcode = opcode,

        .dst.type = D3DSPR_TEMP,
        .dst.writemask = dst->writemask,
        .dst.reg = dst->id,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(src1->writemask),
        .srcs[0].reg = src1->id,
        .srcs[1].type = D3DSPR_TEMP,
        .srcs[1].swizzle = hlsl_swizzle_from_writemask(src2->writemask),
        .srcs[1].reg = src2->id,
        .src_count = 2,
    };
    write_sm1_instruction(ctx, buffer, &instr);
}

static void write_sm1_unary_op(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode, const struct hlsl_reg *dst,
        const struct hlsl_reg *src, D3DSHADER_PARAM_SRCMOD_TYPE src_mod)
{
    const struct sm1_instruction instr =
    {
        .opcode = opcode,

        .dst.type = D3DSPR_TEMP,
        .dst.writemask = dst->writemask,
        .dst.reg = dst->id,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(src->writemask),
        .srcs[0].reg = src->id,
        .srcs[0].mod = src_mod,
        .src_count = 1,
    };
    write_sm1_instruction(ctx, buffer, &instr);
}

static void write_sm1_constant_defs(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer)
{
    unsigned int i, x;

    for (i = 0; i < ctx->constant_defs.count; ++i)
    {
        uint32_t token = D3DSIO_DEF;
        const struct sm1_dst_register reg =
        {
            .type = D3DSPR_CONST,
            .writemask = VKD3DSP_WRITEMASK_ALL,
            .reg = i,
        };

        if (ctx->profile->major_version > 1)
            token |= 5 << D3DSI_INSTLENGTH_SHIFT;
        put_u32(buffer, token);

        write_sm1_dst_register(buffer, &reg);
        for (x = 0; x < 4; ++x)
            put_f32(buffer, ctx->constant_defs.values[i].f[x]);
    }
}

static void write_sm1_semantic_dcl(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_var *var, bool output)
{
    struct sm1_dst_register reg = {0};
    uint32_t token, usage_idx;
    D3DDECLUSAGE usage;
    bool ret;

    if (hlsl_sm1_register_from_semantic(ctx, &var->semantic, output, &reg.type, &reg.reg))
    {
        usage = 0;
        usage_idx = 0;
    }
    else
    {
        ret = hlsl_sm1_usage_from_semantic(&var->semantic, &usage, &usage_idx);
        assert(ret);
        reg.type = output ? D3DSPR_OUTPUT : D3DSPR_INPUT;
        reg.reg = var->reg.id;
    }

    token = D3DSIO_DCL;
    if (ctx->profile->major_version > 1)
        token |= 2 << D3DSI_INSTLENGTH_SHIFT;
    put_u32(buffer, token);

    token = (1u << 31);
    token |= usage << D3DSP_DCL_USAGE_SHIFT;
    token |= usage_idx << D3DSP_DCL_USAGEINDEX_SHIFT;
    put_u32(buffer, token);

    reg.writemask = (1 << var->data_type->dimx) - 1;
    write_sm1_dst_register(buffer, &reg);
}

static void write_sm1_semantic_dcls(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer)
{
    bool write_in = false, write_out = false;
    struct hlsl_ir_var *var;

    if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
        write_in = true;
    else if (ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX && ctx->profile->major_version == 3)
        write_in = write_out = true;
    else if (ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX && ctx->profile->major_version < 3)
        write_in = true;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (write_in && var->is_input_semantic)
            write_sm1_semantic_dcl(ctx, buffer, var, false);
        if (write_out && var->is_output_semantic)
            write_sm1_semantic_dcl(ctx, buffer, var, true);
    }
}

static void write_sm1_constant(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_constant *constant = hlsl_ir_constant(instr);
    struct sm1_instruction sm1_instr =
    {
        .opcode = D3DSIO_MOV,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = instr->reg.id,
        .dst.writemask = instr->reg.writemask,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_CONST,
        .srcs[0].reg = constant->reg.id,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(constant->reg.writemask),
        .src_count = 1,
    };

    assert(instr->reg.allocated);
    assert(constant->reg.allocated);
    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_expr(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_node *instr)
{
    struct hlsl_ir_expr *expr = hlsl_ir_expr(instr);
    struct hlsl_ir_node *arg1 = expr->operands[0].node;
    struct hlsl_ir_node *arg2 = expr->operands[1].node;
    unsigned int i;

    assert(instr->reg.allocated);

    if (instr->data_type->base_type != HLSL_TYPE_FLOAT)
    {
        /* These need to be lowered. */
        hlsl_fixme(ctx, &instr->loc, "SM1 non-float expression.");
        return;
    }

    switch (expr->op)
    {
        case HLSL_OP1_NEG:
            write_sm1_unary_op(ctx, buffer, D3DSIO_MOV, &instr->reg, &arg1->reg, D3DSPSM_NEG);
            break;

        case HLSL_OP1_RCP:
            for (i = 0; i < instr->data_type->dimx; ++i)
            {
                struct hlsl_reg src = arg1->reg, dst = instr->reg;

                src.writemask = hlsl_combine_writemasks(src.writemask, 1u << i);
                dst.writemask = hlsl_combine_writemasks(dst.writemask, 1u << i);
                write_sm1_unary_op(ctx, buffer, D3DSIO_RCP, &dst, &src, 0);
            }
            break;

        case HLSL_OP2_ADD:
            write_sm1_binary_op(ctx, buffer, D3DSIO_ADD, &instr->reg, &arg1->reg, &arg2->reg);
            break;

        case HLSL_OP2_MAX:
            write_sm1_binary_op(ctx, buffer, D3DSIO_MAX, &instr->reg, &arg1->reg, &arg2->reg);
            break;

        case HLSL_OP2_MIN:
            write_sm1_binary_op(ctx, buffer, D3DSIO_MIN, &instr->reg, &arg1->reg, &arg2->reg);
            break;

        case HLSL_OP2_MUL:
            write_sm1_binary_op(ctx, buffer, D3DSIO_MUL, &instr->reg, &arg1->reg, &arg2->reg);
            break;

        default:
            hlsl_fixme(ctx, &instr->loc, "SM1 \"%s\" expression.", debug_hlsl_expr_op(expr->op));
            break;
    }
}

static void write_sm1_load(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_load *load = hlsl_ir_load(instr);
    const struct hlsl_reg reg = hlsl_reg_from_deref(ctx, &load->src);
    struct sm1_instruction sm1_instr =
    {
        .opcode = D3DSIO_MOV,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = instr->reg.id,
        .dst.writemask = instr->reg.writemask,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].reg = reg.id,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(reg.writemask),
        .src_count = 1,
    };

    assert(instr->reg.allocated);

    if (load->src.var->is_uniform)
    {
        assert(reg.allocated);
        sm1_instr.srcs[0].type = D3DSPR_CONST;
    }
    else if (load->src.var->is_input_semantic)
    {
        if (!hlsl_sm1_register_from_semantic(ctx, &load->src.var->semantic,
                false, &sm1_instr.srcs[0].type, &sm1_instr.srcs[0].reg))
        {
            assert(reg.allocated);
            sm1_instr.srcs[0].type = D3DSPR_INPUT;
            sm1_instr.srcs[0].reg = reg.id;
        }
        else
            sm1_instr.srcs[0].swizzle = hlsl_swizzle_from_writemask((1 << load->src.var->data_type->dimx) - 1);
    }

    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_store(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_store *store = hlsl_ir_store(instr);
    const struct hlsl_ir_node *rhs = store->rhs.node;
    const struct hlsl_reg reg = hlsl_reg_from_deref(ctx, &store->lhs);
    struct sm1_instruction sm1_instr =
    {
        .opcode = D3DSIO_MOV,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = reg.id,
        .dst.writemask = hlsl_combine_writemasks(reg.writemask, store->writemask),
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].reg = rhs->reg.id,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(rhs->reg.writemask),
        .src_count = 1,
    };

    if (store->lhs.var->data_type->type == HLSL_CLASS_MATRIX)
    {
        FIXME("Matrix writemasks need to be lowered.\n");
        return;
    }

    if (store->lhs.var->is_output_semantic)
    {
        if (!hlsl_sm1_register_from_semantic(ctx, &store->lhs.var->semantic,
                true, &sm1_instr.dst.type, &sm1_instr.dst.reg))
        {
            assert(reg.allocated);
            sm1_instr.dst.type = D3DSPR_OUTPUT;
            sm1_instr.dst.reg = reg.id;
        }
        else
            sm1_instr.dst.writemask = (1u << store->lhs.var->data_type->dimx) - 1;
    }
    else
        assert(reg.allocated);

    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_swizzle(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_swizzle *swizzle = hlsl_ir_swizzle(instr);
    const struct hlsl_ir_node *val = swizzle->val.node;
    struct sm1_instruction sm1_instr =
    {
        .opcode = D3DSIO_MOV,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = instr->reg.id,
        .dst.writemask = instr->reg.writemask,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].reg = val->reg.id,
        .srcs[0].swizzle = hlsl_combine_swizzles(hlsl_swizzle_from_writemask(val->reg.writemask),
                swizzle->swizzle, instr->data_type->dimx),
        .src_count = 1,
    };

    assert(instr->reg.allocated);
    assert(val->reg.allocated);
    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_instructions(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_function_decl *entry_func)
{
    const struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &entry_func->body.instrs, struct hlsl_ir_node, entry)
    {
        if (instr->data_type)
        {
            if (instr->data_type->type == HLSL_CLASS_MATRIX)
            {
                /* These need to be lowered. */
                hlsl_fixme(ctx, &instr->loc, "SM1 matrix expression.");
                continue;
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
                write_sm1_constant(ctx, buffer, instr);
                break;

            case HLSL_IR_EXPR:
                write_sm1_expr(ctx, buffer, instr);
                break;

            case HLSL_IR_LOAD:
                write_sm1_load(ctx, buffer, instr);
                break;

            case HLSL_IR_STORE:
                write_sm1_store(ctx, buffer, instr);
                break;

            case HLSL_IR_SWIZZLE:
                write_sm1_swizzle(ctx, buffer, instr);
                break;

            default:
                FIXME("Unhandled instruction type %s.\n", hlsl_node_type_to_string(instr->type));
        }
    }
}

int hlsl_sm1_write(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func, struct vkd3d_shader_code *out)
{
    struct vkd3d_bytecode_buffer buffer = {0};
    int ret;

    put_u32(&buffer, sm1_version(ctx->profile->type, ctx->profile->major_version, ctx->profile->minor_version));

    write_sm1_uniforms(ctx, &buffer, entry_func);

    write_sm1_constant_defs(ctx, &buffer);
    write_sm1_semantic_dcls(ctx, &buffer);
    write_sm1_instructions(ctx, &buffer, entry_func);

    put_u32(&buffer, D3DSIO_END);

    if (!(ret = buffer.status))
    {
        out->code = buffer.data;
        out->size = buffer.size;
    }
    return ret;
}
