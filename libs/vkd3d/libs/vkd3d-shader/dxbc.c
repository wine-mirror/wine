/*
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
 * Copyright 2010 Rico Schüller
 * Copyright 2017 Józef Kucia for CodeWeavers
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
#include "sm4.h"

#define SM4_MAX_SRC_COUNT 6
#define SM4_MAX_DST_COUNT 2

STATIC_ASSERT(SM4_MAX_SRC_COUNT <= SPIRV_MAX_SRC_COUNT);

void dxbc_writer_init(struct dxbc_writer *dxbc)
{
    memset(dxbc, 0, sizeof(*dxbc));
}

void dxbc_writer_add_section(struct dxbc_writer *dxbc, uint32_t tag, const void *data, size_t size)
{
    struct vkd3d_shader_dxbc_section_desc *section;

    assert(dxbc->section_count < ARRAY_SIZE(dxbc->sections));

    section = &dxbc->sections[dxbc->section_count++];
    section->tag = tag;
    section->data.code = data;
    section->data.size = size;
}

int vkd3d_shader_serialize_dxbc(size_t section_count, const struct vkd3d_shader_dxbc_section_desc *sections,
        struct vkd3d_shader_code *dxbc, char **messages)
{
    size_t size_position, offsets_position, checksum_position, i;
    struct vkd3d_bytecode_buffer buffer = {0};
    uint32_t checksum[4];

    TRACE("section_count %zu, sections %p, dxbc %p, messages %p.\n", section_count, sections, dxbc, messages);

    if (messages)
        *messages = NULL;

    put_u32(&buffer, TAG_DXBC);

    checksum_position = bytecode_get_size(&buffer);
    for (i = 0; i < 4; ++i)
        put_u32(&buffer, 0);

    put_u32(&buffer, 1); /* version */
    size_position = put_u32(&buffer, 0);
    put_u32(&buffer, section_count);

    offsets_position = bytecode_get_size(&buffer);
    for (i = 0; i < section_count; ++i)
        put_u32(&buffer, 0);

    for (i = 0; i < section_count; ++i)
    {
        set_u32(&buffer, offsets_position + i * sizeof(uint32_t), bytecode_get_size(&buffer));
        put_u32(&buffer, sections[i].tag);
        put_u32(&buffer, sections[i].data.size);
        bytecode_put_bytes(&buffer, sections[i].data.code, sections[i].data.size);
    }
    set_u32(&buffer, size_position, bytecode_get_size(&buffer));

    vkd3d_compute_dxbc_checksum(buffer.data, buffer.size, checksum);
    for (i = 0; i < 4; ++i)
        set_u32(&buffer, checksum_position + i * sizeof(uint32_t), checksum[i]);

    if (!buffer.status)
    {
        dxbc->code = buffer.data;
        dxbc->size = buffer.size;
    }
    return buffer.status;
}

int dxbc_writer_write(struct dxbc_writer *dxbc, struct vkd3d_shader_code *out)
{
    return vkd3d_shader_serialize_dxbc(dxbc->section_count, dxbc->sections, out, NULL);
}

struct vkd3d_shader_src_param_entry
{
    struct list entry;
    struct vkd3d_shader_src_param param;
};

struct vkd3d_shader_sm4_parser
{
    const uint32_t *start, *end;

    unsigned int output_map[MAX_REG_OUTPUT];

    struct vkd3d_shader_parser p;
};

struct vkd3d_sm4_opcode_info
{
    enum vkd3d_sm4_opcode opcode;
    enum vkd3d_shader_opcode handler_idx;
    char dst_info[SM4_MAX_DST_COUNT];
    char src_info[SM4_MAX_SRC_COUNT];
    void (*read_opcode_func)(struct vkd3d_shader_instruction *ins, uint32_t opcode, uint32_t opcode_token,
            const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv);
};

static const enum vkd3d_primitive_type output_primitive_type_table[] =
{
    /* UNKNOWN */                             VKD3D_PT_UNDEFINED,
    /* VKD3D_SM4_OUTPUT_PT_POINTLIST */       VKD3D_PT_POINTLIST,
    /* UNKNOWN */                             VKD3D_PT_UNDEFINED,
    /* VKD3D_SM4_OUTPUT_PT_LINESTRIP */       VKD3D_PT_LINESTRIP,
    /* UNKNOWN */                             VKD3D_PT_UNDEFINED,
    /* VKD3D_SM4_OUTPUT_PT_TRIANGLESTRIP */   VKD3D_PT_TRIANGLESTRIP,
};

static const enum vkd3d_primitive_type input_primitive_type_table[] =
{
    /* UNKNOWN */                             VKD3D_PT_UNDEFINED,
    /* VKD3D_SM4_INPUT_PT_POINT */            VKD3D_PT_POINTLIST,
    /* VKD3D_SM4_INPUT_PT_LINE */             VKD3D_PT_LINELIST,
    /* VKD3D_SM4_INPUT_PT_TRIANGLE */         VKD3D_PT_TRIANGLELIST,
    /* UNKNOWN */                             VKD3D_PT_UNDEFINED,
    /* UNKNOWN */                             VKD3D_PT_UNDEFINED,
    /* VKD3D_SM4_INPUT_PT_LINEADJ */          VKD3D_PT_LINELIST_ADJ,
    /* VKD3D_SM4_INPUT_PT_TRIANGLEADJ */      VKD3D_PT_TRIANGLELIST_ADJ,
};

static const enum vkd3d_shader_resource_type resource_type_table[] =
{
    /* 0 */                                       VKD3D_SHADER_RESOURCE_NONE,
    /* VKD3D_SM4_RESOURCE_BUFFER */               VKD3D_SHADER_RESOURCE_BUFFER,
    /* VKD3D_SM4_RESOURCE_TEXTURE_1D */           VKD3D_SHADER_RESOURCE_TEXTURE_1D,
    /* VKD3D_SM4_RESOURCE_TEXTURE_2D */           VKD3D_SHADER_RESOURCE_TEXTURE_2D,
    /* VKD3D_SM4_RESOURCE_TEXTURE_2DMS */         VKD3D_SHADER_RESOURCE_TEXTURE_2DMS,
    /* VKD3D_SM4_RESOURCE_TEXTURE_3D */           VKD3D_SHADER_RESOURCE_TEXTURE_3D,
    /* VKD3D_SM4_RESOURCE_TEXTURE_CUBE */         VKD3D_SHADER_RESOURCE_TEXTURE_CUBE,
    /* VKD3D_SM4_RESOURCE_TEXTURE_1DARRAY */      VKD3D_SHADER_RESOURCE_TEXTURE_1DARRAY,
    /* VKD3D_SM4_RESOURCE_TEXTURE_2DARRAY */      VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY,
    /* VKD3D_SM4_RESOURCE_TEXTURE_2DMSARRAY */    VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY,
    /* VKD3D_SM4_RESOURCE_TEXTURE_CUBEARRAY */    VKD3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY,
    /* VKD3D_SM4_RESOURCE_RAW_BUFFER */           VKD3D_SHADER_RESOURCE_BUFFER,
    /* VKD3D_SM4_RESOURCE_STRUCTURED_BUFFER */    VKD3D_SHADER_RESOURCE_BUFFER,
};

static const enum vkd3d_data_type data_type_table[] =
{
    /* 0 */                         VKD3D_DATA_FLOAT,
    /* VKD3D_SM4_DATA_UNORM */      VKD3D_DATA_UNORM,
    /* VKD3D_SM4_DATA_SNORM */      VKD3D_DATA_SNORM,
    /* VKD3D_SM4_DATA_INT */        VKD3D_DATA_INT,
    /* VKD3D_SM4_DATA_UINT */       VKD3D_DATA_UINT,
    /* VKD3D_SM4_DATA_FLOAT */      VKD3D_DATA_FLOAT,
    /* VKD3D_SM4_DATA_MIXED */      VKD3D_DATA_MIXED,
    /* VKD3D_SM4_DATA_DOUBLE */     VKD3D_DATA_DOUBLE,
    /* VKD3D_SM4_DATA_CONTINUED */  VKD3D_DATA_CONTINUED,
    /* VKD3D_SM4_DATA_UNUSED */     VKD3D_DATA_UNUSED,
};

static struct vkd3d_shader_sm4_parser *vkd3d_shader_sm4_parser(struct vkd3d_shader_parser *parser)
{
    return CONTAINING_RECORD(parser, struct vkd3d_shader_sm4_parser, p);
}

static bool shader_is_sm_5_1(const struct vkd3d_shader_sm4_parser *sm4)
{
    const struct vkd3d_shader_version *version = &sm4->p.shader_version;

    return version->major >= 5 && version->minor >= 1;
}

static bool shader_sm4_read_src_param(struct vkd3d_shader_sm4_parser *priv, const uint32_t **ptr,
        const uint32_t *end, enum vkd3d_data_type data_type, struct vkd3d_shader_src_param *src_param);
static bool shader_sm4_read_dst_param(struct vkd3d_shader_sm4_parser *priv, const uint32_t **ptr,
        const uint32_t *end, enum vkd3d_data_type data_type, struct vkd3d_shader_dst_param *dst_param);

static bool shader_sm4_read_register_space(struct vkd3d_shader_sm4_parser *priv,
        const uint32_t **ptr, const uint32_t *end, unsigned int *register_space)
{
    *register_space = 0;

    if (!shader_is_sm_5_1(priv))
        return true;

    if (*ptr >= end)
    {
        WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
        return false;
    }

    *register_space = *(*ptr)++;
    return true;
}

static void shader_sm4_read_conditional_op(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    shader_sm4_read_src_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_UINT,
            (struct vkd3d_shader_src_param *)&ins->src[0]);
    ins->flags = (opcode_token & VKD3D_SM4_CONDITIONAL_NZ) ?
            VKD3D_SHADER_CONDITIONAL_OP_NZ : VKD3D_SHADER_CONDITIONAL_OP_Z;
}

static void shader_sm4_read_shader_data(struct vkd3d_shader_instruction *ins, uint32_t opcode, uint32_t opcode_token,
        const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    struct vkd3d_shader_immediate_constant_buffer *icb;
    enum vkd3d_sm4_shader_data_type type;
    unsigned int icb_size;

    type = (opcode_token & VKD3D_SM4_SHADER_DATA_TYPE_MASK) >> VKD3D_SM4_SHADER_DATA_TYPE_SHIFT;
    if (type != VKD3D_SM4_SHADER_DATA_IMMEDIATE_CONSTANT_BUFFER)
    {
        FIXME("Ignoring shader data type %#x.\n", type);
        ins->handler_idx = VKD3DSIH_NOP;
        return;
    }

    ++tokens;
    icb_size = token_count - 1;
    if (icb_size % 4)
    {
        FIXME("Unexpected immediate constant buffer size %u.\n", icb_size);
        ins->handler_idx = VKD3DSIH_INVALID;
        return;
    }

    if (!(icb = vkd3d_malloc(offsetof(struct vkd3d_shader_immediate_constant_buffer, data[icb_size]))))
    {
        ERR("Failed to allocate immediate constant buffer, size %u.\n", icb_size);
        vkd3d_shader_parser_error(&priv->p, VKD3D_SHADER_ERROR_TPF_OUT_OF_MEMORY, "Out of memory.");
        ins->handler_idx = VKD3DSIH_INVALID;
        return;
    }
    icb->vec4_count = icb_size / 4;
    memcpy(icb->data, tokens, sizeof(*tokens) * icb_size);
    shader_instruction_array_add_icb(&priv->p.instructions, icb);
    ins->declaration.icb = icb;
}

static void shader_sm4_set_descriptor_register_range(struct vkd3d_shader_sm4_parser *sm4,
        const struct vkd3d_shader_register *reg, struct vkd3d_shader_register_range *range)
{
    range->first = reg->idx[1].offset;
    range->last = reg->idx[shader_is_sm_5_1(sm4) ? 2 : 1].offset;
    if (range->last < range->first)
    {
        FIXME("Invalid register range [%u:%u].\n", range->first, range->last);
        vkd3d_shader_parser_error(&sm4->p, VKD3D_SHADER_ERROR_TPF_INVALID_REGISTER_RANGE,
                "Last register %u must not be less than first register %u in range.\n", range->last, range->first);
    }
}

static void shader_sm4_read_dcl_resource(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    struct vkd3d_shader_semantic *semantic = &ins->declaration.semantic;
    enum vkd3d_sm4_resource_type resource_type;
    const uint32_t *end = &tokens[token_count];
    enum vkd3d_sm4_data_type data_type;
    enum vkd3d_data_type reg_data_type;
    DWORD components;
    unsigned int i;

    resource_type = (opcode_token & VKD3D_SM4_RESOURCE_TYPE_MASK) >> VKD3D_SM4_RESOURCE_TYPE_SHIFT;
    if (!resource_type || (resource_type >= ARRAY_SIZE(resource_type_table)))
    {
        FIXME("Unhandled resource type %#x.\n", resource_type);
        semantic->resource_type = VKD3D_SHADER_RESOURCE_NONE;
    }
    else
    {
        semantic->resource_type = resource_type_table[resource_type];
    }

    if (semantic->resource_type == VKD3D_SHADER_RESOURCE_TEXTURE_2DMS
            || semantic->resource_type == VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY)
    {
        semantic->sample_count = (opcode_token & VKD3D_SM4_RESOURCE_SAMPLE_COUNT_MASK) >> VKD3D_SM4_RESOURCE_SAMPLE_COUNT_SHIFT;
    }

    reg_data_type = opcode == VKD3D_SM4_OP_DCL_RESOURCE ? VKD3D_DATA_RESOURCE : VKD3D_DATA_UAV;
    shader_sm4_read_dst_param(priv, &tokens, end, reg_data_type, &semantic->resource.reg);
    shader_sm4_set_descriptor_register_range(priv, &semantic->resource.reg.reg, &semantic->resource.range);

    components = *tokens++;
    for (i = 0; i < VKD3D_VEC4_SIZE; i++)
    {
        data_type = VKD3D_SM4_TYPE_COMPONENT(components, i);

        if (!data_type || (data_type >= ARRAY_SIZE(data_type_table)))
        {
            FIXME("Unhandled data type %#x.\n", data_type);
            semantic->resource_data_type[i] = VKD3D_DATA_FLOAT;
        }
        else
        {
            semantic->resource_data_type[i] = data_type_table[data_type];
        }
    }

    if (reg_data_type == VKD3D_DATA_UAV)
        ins->flags = (opcode_token & VKD3D_SM5_UAV_FLAGS_MASK) >> VKD3D_SM5_UAV_FLAGS_SHIFT;

    shader_sm4_read_register_space(priv, &tokens, end, &semantic->resource.range.space);
}

static void shader_sm4_read_dcl_constant_buffer(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    const uint32_t *end = &tokens[token_count];

    shader_sm4_read_src_param(priv, &tokens, end, VKD3D_DATA_FLOAT, &ins->declaration.cb.src);
    shader_sm4_set_descriptor_register_range(priv, &ins->declaration.cb.src.reg, &ins->declaration.cb.range);
    if (opcode_token & VKD3D_SM4_INDEX_TYPE_MASK)
        ins->flags |= VKD3DSI_INDEXED_DYNAMIC;

    ins->declaration.cb.size = ins->declaration.cb.src.reg.idx[2].offset;
    ins->declaration.cb.range.space = 0;

    if (shader_is_sm_5_1(priv))
    {
        if (tokens >= end)
        {
            FIXME("Invalid ptr %p >= end %p.\n", tokens, end);
            return;
        }

        ins->declaration.cb.size = *tokens++;
        shader_sm4_read_register_space(priv, &tokens, end, &ins->declaration.cb.range.space);
    }
}

static void shader_sm4_read_dcl_sampler(struct vkd3d_shader_instruction *ins, uint32_t opcode, uint32_t opcode_token,
        const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    const uint32_t *end = &tokens[token_count];

    ins->flags = (opcode_token & VKD3D_SM4_SAMPLER_MODE_MASK) >> VKD3D_SM4_SAMPLER_MODE_SHIFT;
    if (ins->flags & ~VKD3D_SM4_SAMPLER_COMPARISON)
        FIXME("Unhandled sampler mode %#x.\n", ins->flags);
    shader_sm4_read_src_param(priv, &tokens, end, VKD3D_DATA_SAMPLER, &ins->declaration.sampler.src);
    shader_sm4_set_descriptor_register_range(priv, &ins->declaration.sampler.src.reg, &ins->declaration.sampler.range);
    shader_sm4_read_register_space(priv, &tokens, end, &ins->declaration.sampler.range.space);
}

static void shader_sm4_read_dcl_index_range(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_OPAQUE,
            &ins->declaration.index_range.dst);
    ins->declaration.index_range.register_count = *tokens;
}

static void shader_sm4_read_dcl_output_topology(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    enum vkd3d_sm4_output_primitive_type primitive_type;

    primitive_type = (opcode_token & VKD3D_SM4_PRIMITIVE_TYPE_MASK) >> VKD3D_SM4_PRIMITIVE_TYPE_SHIFT;
    if (primitive_type >= ARRAY_SIZE(output_primitive_type_table))
        ins->declaration.primitive_type.type = VKD3D_PT_UNDEFINED;
    else
        ins->declaration.primitive_type.type = output_primitive_type_table[primitive_type];

    if (ins->declaration.primitive_type.type == VKD3D_PT_UNDEFINED)
        FIXME("Unhandled output primitive type %#x.\n", primitive_type);
}

static void shader_sm4_read_dcl_input_primitive(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    enum vkd3d_sm4_input_primitive_type primitive_type;

    primitive_type = (opcode_token & VKD3D_SM4_PRIMITIVE_TYPE_MASK) >> VKD3D_SM4_PRIMITIVE_TYPE_SHIFT;
    if (VKD3D_SM5_INPUT_PT_PATCH1 <= primitive_type && primitive_type <= VKD3D_SM5_INPUT_PT_PATCH32)
    {
        ins->declaration.primitive_type.type = VKD3D_PT_PATCH;
        ins->declaration.primitive_type.patch_vertex_count = primitive_type - VKD3D_SM5_INPUT_PT_PATCH1 + 1;
    }
    else if (primitive_type >= ARRAY_SIZE(input_primitive_type_table))
    {
        ins->declaration.primitive_type.type = VKD3D_PT_UNDEFINED;
    }
    else
    {
        ins->declaration.primitive_type.type = input_primitive_type_table[primitive_type];
    }

    if (ins->declaration.primitive_type.type == VKD3D_PT_UNDEFINED)
        FIXME("Unhandled input primitive type %#x.\n", primitive_type);
}

static void shader_sm4_read_declaration_count(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.count = *tokens;
}

static void shader_sm4_read_declaration_dst(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_FLOAT, &ins->declaration.dst);
}

static void shader_sm4_read_declaration_register_semantic(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_FLOAT,
            &ins->declaration.register_semantic.reg);
    ins->declaration.register_semantic.sysval_semantic = *tokens;
}

static void shader_sm4_read_dcl_input_ps(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->flags = (opcode_token & VKD3D_SM4_INTERPOLATION_MODE_MASK) >> VKD3D_SM4_INTERPOLATION_MODE_SHIFT;
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_FLOAT, &ins->declaration.dst);
}

static void shader_sm4_read_dcl_input_ps_siv(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->flags = (opcode_token & VKD3D_SM4_INTERPOLATION_MODE_MASK) >> VKD3D_SM4_INTERPOLATION_MODE_SHIFT;
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_FLOAT,
            &ins->declaration.register_semantic.reg);
    ins->declaration.register_semantic.sysval_semantic = *tokens;
}

static void shader_sm4_read_dcl_indexable_temp(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.indexable_temp.register_idx = *tokens++;
    ins->declaration.indexable_temp.register_size = *tokens++;
    ins->declaration.indexable_temp.component_count = *tokens;
}

static void shader_sm4_read_dcl_global_flags(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->flags = (opcode_token & VKD3D_SM4_GLOBAL_FLAGS_MASK) >> VKD3D_SM4_GLOBAL_FLAGS_SHIFT;
}

static void shader_sm5_read_fcall(struct vkd3d_shader_instruction *ins, uint32_t opcode, uint32_t opcode_token,
        const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    struct vkd3d_shader_src_param *src_params = (struct vkd3d_shader_src_param *)ins->src;
    src_params[0].reg.u.fp_body_idx = *tokens++;
    shader_sm4_read_src_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_OPAQUE, &src_params[0]);
}

static void shader_sm5_read_dcl_function_body(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.index = *tokens;
}

static void shader_sm5_read_dcl_function_table(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.index = *tokens++;
    FIXME("Ignoring set of function bodies (count %u).\n", *tokens);
}

static void shader_sm5_read_dcl_interface(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.fp.index = *tokens++;
    ins->declaration.fp.body_count = *tokens++;
    ins->declaration.fp.array_size = *tokens >> VKD3D_SM5_FP_ARRAY_SIZE_SHIFT;
    ins->declaration.fp.table_count = *tokens++ & VKD3D_SM5_FP_TABLE_COUNT_MASK;
    FIXME("Ignoring set of function tables (count %u).\n", ins->declaration.fp.table_count);
}

static void shader_sm5_read_control_point_count(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.count = (opcode_token & VKD3D_SM5_CONTROL_POINT_COUNT_MASK)
            >> VKD3D_SM5_CONTROL_POINT_COUNT_SHIFT;
}

static void shader_sm5_read_dcl_tessellator_domain(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.tessellator_domain = (opcode_token & VKD3D_SM5_TESSELLATOR_MASK)
            >> VKD3D_SM5_TESSELLATOR_SHIFT;
}

static void shader_sm5_read_dcl_tessellator_partitioning(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.tessellator_partitioning = (opcode_token & VKD3D_SM5_TESSELLATOR_MASK)
            >> VKD3D_SM5_TESSELLATOR_SHIFT;
}

static void shader_sm5_read_dcl_tessellator_output_primitive(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.tessellator_output_primitive = (opcode_token & VKD3D_SM5_TESSELLATOR_MASK)
            >> VKD3D_SM5_TESSELLATOR_SHIFT;
}

static void shader_sm5_read_dcl_hs_max_tessfactor(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.max_tessellation_factor = *(float *)tokens;
}

static void shader_sm5_read_dcl_thread_group(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->declaration.thread_group_size.x = *tokens++;
    ins->declaration.thread_group_size.y = *tokens++;
    ins->declaration.thread_group_size.z = *tokens++;
}

static void shader_sm5_read_dcl_uav_raw(struct vkd3d_shader_instruction *ins, uint32_t opcode, uint32_t opcode_token,
        const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    struct vkd3d_shader_raw_resource *resource = &ins->declaration.raw_resource;
    const uint32_t *end = &tokens[token_count];

    shader_sm4_read_dst_param(priv, &tokens, end, VKD3D_DATA_UAV, &resource->resource.reg);
    shader_sm4_set_descriptor_register_range(priv, &resource->resource.reg.reg, &resource->resource.range);
    ins->flags = (opcode_token & VKD3D_SM5_UAV_FLAGS_MASK) >> VKD3D_SM5_UAV_FLAGS_SHIFT;
    shader_sm4_read_register_space(priv, &tokens, end, &resource->resource.range.space);
}

static void shader_sm5_read_dcl_uav_structured(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    struct vkd3d_shader_structured_resource *resource = &ins->declaration.structured_resource;
    const uint32_t *end = &tokens[token_count];

    shader_sm4_read_dst_param(priv, &tokens, end, VKD3D_DATA_UAV, &resource->resource.reg);
    shader_sm4_set_descriptor_register_range(priv, &resource->resource.reg.reg, &resource->resource.range);
    ins->flags = (opcode_token & VKD3D_SM5_UAV_FLAGS_MASK) >> VKD3D_SM5_UAV_FLAGS_SHIFT;
    resource->byte_stride = *tokens++;
    if (resource->byte_stride % 4)
        FIXME("Byte stride %u is not multiple of 4.\n", resource->byte_stride);
    shader_sm4_read_register_space(priv, &tokens, end, &resource->resource.range.space);
}

static void shader_sm5_read_dcl_tgsm_raw(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_FLOAT, &ins->declaration.tgsm_raw.reg);
    ins->declaration.tgsm_raw.byte_count = *tokens;
    if (ins->declaration.tgsm_raw.byte_count % 4)
        FIXME("Byte count %u is not multiple of 4.\n", ins->declaration.tgsm_raw.byte_count);
}

static void shader_sm5_read_dcl_tgsm_structured(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], VKD3D_DATA_FLOAT,
            &ins->declaration.tgsm_structured.reg);
    ins->declaration.tgsm_structured.byte_stride = *tokens++;
    ins->declaration.tgsm_structured.structure_count = *tokens;
    if (ins->declaration.tgsm_structured.byte_stride % 4)
        FIXME("Byte stride %u is not multiple of 4.\n", ins->declaration.tgsm_structured.byte_stride);
}

static void shader_sm5_read_dcl_resource_structured(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    struct vkd3d_shader_structured_resource *resource = &ins->declaration.structured_resource;
    const uint32_t *end = &tokens[token_count];

    shader_sm4_read_dst_param(priv, &tokens, end, VKD3D_DATA_RESOURCE, &resource->resource.reg);
    shader_sm4_set_descriptor_register_range(priv, &resource->resource.reg.reg, &resource->resource.range);
    resource->byte_stride = *tokens++;
    if (resource->byte_stride % 4)
        FIXME("Byte stride %u is not multiple of 4.\n", resource->byte_stride);
    shader_sm4_read_register_space(priv, &tokens, end, &resource->resource.range.space);
}

static void shader_sm5_read_dcl_resource_raw(struct vkd3d_shader_instruction *ins, uint32_t opcode,
        uint32_t opcode_token, const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    struct vkd3d_shader_raw_resource *resource = &ins->declaration.raw_resource;
    const uint32_t *end = &tokens[token_count];

    shader_sm4_read_dst_param(priv, &tokens, end, VKD3D_DATA_RESOURCE, &resource->resource.reg);
    shader_sm4_set_descriptor_register_range(priv, &resource->resource.reg.reg, &resource->resource.range);
    shader_sm4_read_register_space(priv, &tokens, end, &resource->resource.range.space);
}

static void shader_sm5_read_sync(struct vkd3d_shader_instruction *ins, uint32_t opcode, uint32_t opcode_token,
        const uint32_t *tokens, unsigned int token_count, struct vkd3d_shader_sm4_parser *priv)
{
    ins->flags = (opcode_token & VKD3D_SM5_SYNC_FLAGS_MASK) >> VKD3D_SM5_SYNC_FLAGS_SHIFT;
}

/*
 * d -> VKD3D_DATA_DOUBLE
 * f -> VKD3D_DATA_FLOAT
 * i -> VKD3D_DATA_INT
 * u -> VKD3D_DATA_UINT
 * O -> VKD3D_DATA_OPAQUE
 * R -> VKD3D_DATA_RESOURCE
 * S -> VKD3D_DATA_SAMPLER
 * U -> VKD3D_DATA_UAV
 */
static const struct vkd3d_sm4_opcode_info opcode_table[] =
{
    {VKD3D_SM4_OP_ADD,                              VKD3DSIH_ADD,                              "f",    "ff"},
    {VKD3D_SM4_OP_AND,                              VKD3DSIH_AND,                              "u",    "uu"},
    {VKD3D_SM4_OP_BREAK,                            VKD3DSIH_BREAK,                            "",     ""},
    {VKD3D_SM4_OP_BREAKC,                           VKD3DSIH_BREAKP,                           "",     "u",
            shader_sm4_read_conditional_op},
    {VKD3D_SM4_OP_CASE,                             VKD3DSIH_CASE,                             "",     "u"},
    {VKD3D_SM4_OP_CONTINUE,                         VKD3DSIH_CONTINUE,                         "",     ""},
    {VKD3D_SM4_OP_CONTINUEC,                        VKD3DSIH_CONTINUEP,                        "",     "u",
            shader_sm4_read_conditional_op},
    {VKD3D_SM4_OP_CUT,                              VKD3DSIH_CUT,                              "",     ""},
    {VKD3D_SM4_OP_DEFAULT,                          VKD3DSIH_DEFAULT,                          "",     ""},
    {VKD3D_SM4_OP_DERIV_RTX,                        VKD3DSIH_DSX,                              "f",    "f"},
    {VKD3D_SM4_OP_DERIV_RTY,                        VKD3DSIH_DSY,                              "f",    "f"},
    {VKD3D_SM4_OP_DISCARD,                          VKD3DSIH_TEXKILL,                          "",     "u",
            shader_sm4_read_conditional_op},
    {VKD3D_SM4_OP_DIV,                              VKD3DSIH_DIV,                              "f",    "ff"},
    {VKD3D_SM4_OP_DP2,                              VKD3DSIH_DP2,                              "f",    "ff"},
    {VKD3D_SM4_OP_DP3,                              VKD3DSIH_DP3,                              "f",    "ff"},
    {VKD3D_SM4_OP_DP4,                              VKD3DSIH_DP4,                              "f",    "ff"},
    {VKD3D_SM4_OP_ELSE,                             VKD3DSIH_ELSE,                             "",     ""},
    {VKD3D_SM4_OP_EMIT,                             VKD3DSIH_EMIT,                             "",     ""},
    {VKD3D_SM4_OP_ENDIF,                            VKD3DSIH_ENDIF,                            "",     ""},
    {VKD3D_SM4_OP_ENDLOOP,                          VKD3DSIH_ENDLOOP,                          "",     ""},
    {VKD3D_SM4_OP_ENDSWITCH,                        VKD3DSIH_ENDSWITCH,                        "",     ""},
    {VKD3D_SM4_OP_EQ,                               VKD3DSIH_EQ,                               "u",    "ff"},
    {VKD3D_SM4_OP_EXP,                              VKD3DSIH_EXP,                              "f",    "f"},
    {VKD3D_SM4_OP_FRC,                              VKD3DSIH_FRC,                              "f",    "f"},
    {VKD3D_SM4_OP_FTOI,                             VKD3DSIH_FTOI,                             "i",    "f"},
    {VKD3D_SM4_OP_FTOU,                             VKD3DSIH_FTOU,                             "u",    "f"},
    {VKD3D_SM4_OP_GE,                               VKD3DSIH_GE,                               "u",    "ff"},
    {VKD3D_SM4_OP_IADD,                             VKD3DSIH_IADD,                             "i",    "ii"},
    {VKD3D_SM4_OP_IF,                               VKD3DSIH_IF,                               "",     "u",
            shader_sm4_read_conditional_op},
    {VKD3D_SM4_OP_IEQ,                              VKD3DSIH_IEQ,                              "u",    "ii"},
    {VKD3D_SM4_OP_IGE,                              VKD3DSIH_IGE,                              "u",    "ii"},
    {VKD3D_SM4_OP_ILT,                              VKD3DSIH_ILT,                              "u",    "ii"},
    {VKD3D_SM4_OP_IMAD,                             VKD3DSIH_IMAD,                             "i",    "iii"},
    {VKD3D_SM4_OP_IMAX,                             VKD3DSIH_IMAX,                             "i",    "ii"},
    {VKD3D_SM4_OP_IMIN,                             VKD3DSIH_IMIN,                             "i",    "ii"},
    {VKD3D_SM4_OP_IMUL,                             VKD3DSIH_IMUL,                             "ii",   "ii"},
    {VKD3D_SM4_OP_INE,                              VKD3DSIH_INE,                              "u",    "ii"},
    {VKD3D_SM4_OP_INEG,                             VKD3DSIH_INEG,                             "i",    "i"},
    {VKD3D_SM4_OP_ISHL,                             VKD3DSIH_ISHL,                             "i",    "ii"},
    {VKD3D_SM4_OP_ISHR,                             VKD3DSIH_ISHR,                             "i",    "ii"},
    {VKD3D_SM4_OP_ITOF,                             VKD3DSIH_ITOF,                             "f",    "i"},
    {VKD3D_SM4_OP_LABEL,                            VKD3DSIH_LABEL,                            "",     "O"},
    {VKD3D_SM4_OP_LD,                               VKD3DSIH_LD,                               "u",    "iR"},
    {VKD3D_SM4_OP_LD2DMS,                           VKD3DSIH_LD2DMS,                           "u",    "iRi"},
    {VKD3D_SM4_OP_LOG,                              VKD3DSIH_LOG,                              "f",    "f"},
    {VKD3D_SM4_OP_LOOP,                             VKD3DSIH_LOOP,                             "",     ""},
    {VKD3D_SM4_OP_LT,                               VKD3DSIH_LT,                               "u",    "ff"},
    {VKD3D_SM4_OP_MAD,                              VKD3DSIH_MAD,                              "f",    "fff"},
    {VKD3D_SM4_OP_MIN,                              VKD3DSIH_MIN,                              "f",    "ff"},
    {VKD3D_SM4_OP_MAX,                              VKD3DSIH_MAX,                              "f",    "ff"},
    {VKD3D_SM4_OP_SHADER_DATA,                      VKD3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER,    "",     "",
            shader_sm4_read_shader_data},
    {VKD3D_SM4_OP_MOV,                              VKD3DSIH_MOV,                              "f",    "f"},
    {VKD3D_SM4_OP_MOVC,                             VKD3DSIH_MOVC,                             "f",    "uff"},
    {VKD3D_SM4_OP_MUL,                              VKD3DSIH_MUL,                              "f",    "ff"},
    {VKD3D_SM4_OP_NE,                               VKD3DSIH_NE,                               "u",    "ff"},
    {VKD3D_SM4_OP_NOP,                              VKD3DSIH_NOP,                              "",     ""},
    {VKD3D_SM4_OP_NOT,                              VKD3DSIH_NOT,                              "u",    "u"},
    {VKD3D_SM4_OP_OR,                               VKD3DSIH_OR,                               "u",    "uu"},
    {VKD3D_SM4_OP_RESINFO,                          VKD3DSIH_RESINFO,                          "f",    "iR"},
    {VKD3D_SM4_OP_RET,                              VKD3DSIH_RET,                              "",     ""},
    {VKD3D_SM4_OP_RETC,                             VKD3DSIH_RETP,                             "",     "u",
            shader_sm4_read_conditional_op},
    {VKD3D_SM4_OP_ROUND_NE,                         VKD3DSIH_ROUND_NE,                         "f",    "f"},
    {VKD3D_SM4_OP_ROUND_NI,                         VKD3DSIH_ROUND_NI,                         "f",    "f"},
    {VKD3D_SM4_OP_ROUND_PI,                         VKD3DSIH_ROUND_PI,                         "f",    "f"},
    {VKD3D_SM4_OP_ROUND_Z,                          VKD3DSIH_ROUND_Z,                          "f",    "f"},
    {VKD3D_SM4_OP_RSQ,                              VKD3DSIH_RSQ,                              "f",    "f"},
    {VKD3D_SM4_OP_SAMPLE,                           VKD3DSIH_SAMPLE,                           "u",    "fRS"},
    {VKD3D_SM4_OP_SAMPLE_C,                         VKD3DSIH_SAMPLE_C,                         "f",    "fRSf"},
    {VKD3D_SM4_OP_SAMPLE_C_LZ,                      VKD3DSIH_SAMPLE_C_LZ,                      "f",    "fRSf"},
    {VKD3D_SM4_OP_SAMPLE_LOD,                       VKD3DSIH_SAMPLE_LOD,                       "u",    "fRSf"},
    {VKD3D_SM4_OP_SAMPLE_GRAD,                      VKD3DSIH_SAMPLE_GRAD,                      "u",    "fRSff"},
    {VKD3D_SM4_OP_SAMPLE_B,                         VKD3DSIH_SAMPLE_B,                         "u",    "fRSf"},
    {VKD3D_SM4_OP_SQRT,                             VKD3DSIH_SQRT,                             "f",    "f"},
    {VKD3D_SM4_OP_SWITCH,                           VKD3DSIH_SWITCH,                           "",     "i"},
    {VKD3D_SM4_OP_SINCOS,                           VKD3DSIH_SINCOS,                           "ff",   "f"},
    {VKD3D_SM4_OP_UDIV,                             VKD3DSIH_UDIV,                             "uu",   "uu"},
    {VKD3D_SM4_OP_ULT,                              VKD3DSIH_ULT,                              "u",    "uu"},
    {VKD3D_SM4_OP_UGE,                              VKD3DSIH_UGE,                              "u",    "uu"},
    {VKD3D_SM4_OP_UMUL,                             VKD3DSIH_UMUL,                             "uu",   "uu"},
    {VKD3D_SM4_OP_UMAX,                             VKD3DSIH_UMAX,                             "u",    "uu"},
    {VKD3D_SM4_OP_UMIN,                             VKD3DSIH_UMIN,                             "u",    "uu"},
    {VKD3D_SM4_OP_USHR,                             VKD3DSIH_USHR,                             "u",    "uu"},
    {VKD3D_SM4_OP_UTOF,                             VKD3DSIH_UTOF,                             "f",    "u"},
    {VKD3D_SM4_OP_XOR,                              VKD3DSIH_XOR,                              "u",    "uu"},
    {VKD3D_SM4_OP_DCL_RESOURCE,                     VKD3DSIH_DCL,                              "",     "",
            shader_sm4_read_dcl_resource},
    {VKD3D_SM4_OP_DCL_CONSTANT_BUFFER,              VKD3DSIH_DCL_CONSTANT_BUFFER,              "",     "",
            shader_sm4_read_dcl_constant_buffer},
    {VKD3D_SM4_OP_DCL_SAMPLER,                      VKD3DSIH_DCL_SAMPLER,                      "",     "",
            shader_sm4_read_dcl_sampler},
    {VKD3D_SM4_OP_DCL_INDEX_RANGE,                  VKD3DSIH_DCL_INDEX_RANGE,                  "",     "",
            shader_sm4_read_dcl_index_range},
    {VKD3D_SM4_OP_DCL_OUTPUT_TOPOLOGY,              VKD3DSIH_DCL_OUTPUT_TOPOLOGY,              "",     "",
            shader_sm4_read_dcl_output_topology},
    {VKD3D_SM4_OP_DCL_INPUT_PRIMITIVE,              VKD3DSIH_DCL_INPUT_PRIMITIVE,              "",     "",
            shader_sm4_read_dcl_input_primitive},
    {VKD3D_SM4_OP_DCL_VERTICES_OUT,                 VKD3DSIH_DCL_VERTICES_OUT,                 "",     "",
            shader_sm4_read_declaration_count},
    {VKD3D_SM4_OP_DCL_INPUT,                        VKD3DSIH_DCL_INPUT,                        "",     "",
            shader_sm4_read_declaration_dst},
    {VKD3D_SM4_OP_DCL_INPUT_SGV,                    VKD3DSIH_DCL_INPUT_SGV,                    "",     "",
            shader_sm4_read_declaration_register_semantic},
    {VKD3D_SM4_OP_DCL_INPUT_SIV,                    VKD3DSIH_DCL_INPUT_SIV,                    "",     "",
            shader_sm4_read_declaration_register_semantic},
    {VKD3D_SM4_OP_DCL_INPUT_PS,                     VKD3DSIH_DCL_INPUT_PS,                     "",     "",
            shader_sm4_read_dcl_input_ps},
    {VKD3D_SM4_OP_DCL_INPUT_PS_SGV,                 VKD3DSIH_DCL_INPUT_PS_SGV,                 "",     "",
            shader_sm4_read_declaration_register_semantic},
    {VKD3D_SM4_OP_DCL_INPUT_PS_SIV,                 VKD3DSIH_DCL_INPUT_PS_SIV,                 "",     "",
            shader_sm4_read_dcl_input_ps_siv},
    {VKD3D_SM4_OP_DCL_OUTPUT,                       VKD3DSIH_DCL_OUTPUT,                       "",     "",
            shader_sm4_read_declaration_dst},
    {VKD3D_SM4_OP_DCL_OUTPUT_SIV,                   VKD3DSIH_DCL_OUTPUT_SIV,                   "",     "",
            shader_sm4_read_declaration_register_semantic},
    {VKD3D_SM4_OP_DCL_TEMPS,                        VKD3DSIH_DCL_TEMPS,                        "",     "",
            shader_sm4_read_declaration_count},
    {VKD3D_SM4_OP_DCL_INDEXABLE_TEMP,               VKD3DSIH_DCL_INDEXABLE_TEMP,               "",     "",
            shader_sm4_read_dcl_indexable_temp},
    {VKD3D_SM4_OP_DCL_GLOBAL_FLAGS,                 VKD3DSIH_DCL_GLOBAL_FLAGS,                 "",     "",
            shader_sm4_read_dcl_global_flags},
    {VKD3D_SM4_OP_LOD,                              VKD3DSIH_LOD,                              "f",    "fRS"},
    {VKD3D_SM4_OP_GATHER4,                          VKD3DSIH_GATHER4,                          "u",    "fRS"},
    {VKD3D_SM4_OP_SAMPLE_POS,                       VKD3DSIH_SAMPLE_POS,                       "f",    "Ru"},
    {VKD3D_SM4_OP_SAMPLE_INFO,                      VKD3DSIH_SAMPLE_INFO,                      "f",    "R"},
    {VKD3D_SM5_OP_HS_DECLS,                         VKD3DSIH_HS_DECLS,                         "",     ""},
    {VKD3D_SM5_OP_HS_CONTROL_POINT_PHASE,           VKD3DSIH_HS_CONTROL_POINT_PHASE,           "",     ""},
    {VKD3D_SM5_OP_HS_FORK_PHASE,                    VKD3DSIH_HS_FORK_PHASE,                    "",     ""},
    {VKD3D_SM5_OP_HS_JOIN_PHASE,                    VKD3DSIH_HS_JOIN_PHASE,                    "",     ""},
    {VKD3D_SM5_OP_EMIT_STREAM,                      VKD3DSIH_EMIT_STREAM,                      "",     "f"},
    {VKD3D_SM5_OP_CUT_STREAM,                       VKD3DSIH_CUT_STREAM,                       "",     "f"},
    {VKD3D_SM5_OP_FCALL,                            VKD3DSIH_FCALL,                            "",     "O",
            shader_sm5_read_fcall},
    {VKD3D_SM5_OP_BUFINFO,                          VKD3DSIH_BUFINFO,                          "i",    "U"},
    {VKD3D_SM5_OP_DERIV_RTX_COARSE,                 VKD3DSIH_DSX_COARSE,                       "f",    "f"},
    {VKD3D_SM5_OP_DERIV_RTX_FINE,                   VKD3DSIH_DSX_FINE,                         "f",    "f"},
    {VKD3D_SM5_OP_DERIV_RTY_COARSE,                 VKD3DSIH_DSY_COARSE,                       "f",    "f"},
    {VKD3D_SM5_OP_DERIV_RTY_FINE,                   VKD3DSIH_DSY_FINE,                         "f",    "f"},
    {VKD3D_SM5_OP_GATHER4_C,                        VKD3DSIH_GATHER4_C,                        "f",    "fRSf"},
    {VKD3D_SM5_OP_GATHER4_PO,                       VKD3DSIH_GATHER4_PO,                       "f",    "fiRS"},
    {VKD3D_SM5_OP_GATHER4_PO_C,                     VKD3DSIH_GATHER4_PO_C,                     "f",    "fiRSf"},
    {VKD3D_SM5_OP_RCP,                              VKD3DSIH_RCP,                              "f",    "f"},
    {VKD3D_SM5_OP_F32TOF16,                         VKD3DSIH_F32TOF16,                         "u",    "f"},
    {VKD3D_SM5_OP_F16TOF32,                         VKD3DSIH_F16TOF32,                         "f",    "u"},
    {VKD3D_SM5_OP_COUNTBITS,                        VKD3DSIH_COUNTBITS,                        "u",    "u"},
    {VKD3D_SM5_OP_FIRSTBIT_HI,                      VKD3DSIH_FIRSTBIT_HI,                      "u",    "u"},
    {VKD3D_SM5_OP_FIRSTBIT_LO,                      VKD3DSIH_FIRSTBIT_LO,                      "u",    "u"},
    {VKD3D_SM5_OP_FIRSTBIT_SHI,                     VKD3DSIH_FIRSTBIT_SHI,                     "u",    "i"},
    {VKD3D_SM5_OP_UBFE,                             VKD3DSIH_UBFE,                             "u",    "iiu"},
    {VKD3D_SM5_OP_IBFE,                             VKD3DSIH_IBFE,                             "i",    "iii"},
    {VKD3D_SM5_OP_BFI,                              VKD3DSIH_BFI,                              "u",    "iiuu"},
    {VKD3D_SM5_OP_BFREV,                            VKD3DSIH_BFREV,                            "u",    "u"},
    {VKD3D_SM5_OP_SWAPC,                            VKD3DSIH_SWAPC,                            "ff",   "uff"},
    {VKD3D_SM5_OP_DCL_STREAM,                       VKD3DSIH_DCL_STREAM,                       "",     "O"},
    {VKD3D_SM5_OP_DCL_FUNCTION_BODY,                VKD3DSIH_DCL_FUNCTION_BODY,                "",     "",
            shader_sm5_read_dcl_function_body},
    {VKD3D_SM5_OP_DCL_FUNCTION_TABLE,               VKD3DSIH_DCL_FUNCTION_TABLE,               "",     "",
            shader_sm5_read_dcl_function_table},
    {VKD3D_SM5_OP_DCL_INTERFACE,                    VKD3DSIH_DCL_INTERFACE,                    "",     "",
            shader_sm5_read_dcl_interface},
    {VKD3D_SM5_OP_DCL_INPUT_CONTROL_POINT_COUNT,    VKD3DSIH_DCL_INPUT_CONTROL_POINT_COUNT,    "",     "",
            shader_sm5_read_control_point_count},
    {VKD3D_SM5_OP_DCL_OUTPUT_CONTROL_POINT_COUNT,   VKD3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT,   "",     "",
            shader_sm5_read_control_point_count},
    {VKD3D_SM5_OP_DCL_TESSELLATOR_DOMAIN,           VKD3DSIH_DCL_TESSELLATOR_DOMAIN,           "",     "",
            shader_sm5_read_dcl_tessellator_domain},
    {VKD3D_SM5_OP_DCL_TESSELLATOR_PARTITIONING,     VKD3DSIH_DCL_TESSELLATOR_PARTITIONING,     "",     "",
            shader_sm5_read_dcl_tessellator_partitioning},
    {VKD3D_SM5_OP_DCL_TESSELLATOR_OUTPUT_PRIMITIVE, VKD3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE, "",     "",
            shader_sm5_read_dcl_tessellator_output_primitive},
    {VKD3D_SM5_OP_DCL_HS_MAX_TESSFACTOR,            VKD3DSIH_DCL_HS_MAX_TESSFACTOR,            "",     "",
            shader_sm5_read_dcl_hs_max_tessfactor},
    {VKD3D_SM5_OP_DCL_HS_FORK_PHASE_INSTANCE_COUNT, VKD3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT, "",     "",
            shader_sm4_read_declaration_count},
    {VKD3D_SM5_OP_DCL_HS_JOIN_PHASE_INSTANCE_COUNT, VKD3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT, "",     "",
            shader_sm4_read_declaration_count},
    {VKD3D_SM5_OP_DCL_THREAD_GROUP,                 VKD3DSIH_DCL_THREAD_GROUP,                 "",     "",
            shader_sm5_read_dcl_thread_group},
    {VKD3D_SM5_OP_DCL_UAV_TYPED,                    VKD3DSIH_DCL_UAV_TYPED,                    "",     "",
            shader_sm4_read_dcl_resource},
    {VKD3D_SM5_OP_DCL_UAV_RAW,                      VKD3DSIH_DCL_UAV_RAW,                      "",     "",
            shader_sm5_read_dcl_uav_raw},
    {VKD3D_SM5_OP_DCL_UAV_STRUCTURED,               VKD3DSIH_DCL_UAV_STRUCTURED,               "",     "",
            shader_sm5_read_dcl_uav_structured},
    {VKD3D_SM5_OP_DCL_TGSM_RAW,                     VKD3DSIH_DCL_TGSM_RAW,                     "",     "",
            shader_sm5_read_dcl_tgsm_raw},
    {VKD3D_SM5_OP_DCL_TGSM_STRUCTURED,              VKD3DSIH_DCL_TGSM_STRUCTURED,              "",     "",
            shader_sm5_read_dcl_tgsm_structured},
    {VKD3D_SM5_OP_DCL_RESOURCE_RAW,                 VKD3DSIH_DCL_RESOURCE_RAW,                 "",     "",
            shader_sm5_read_dcl_resource_raw},
    {VKD3D_SM5_OP_DCL_RESOURCE_STRUCTURED,          VKD3DSIH_DCL_RESOURCE_STRUCTURED,          "",     "",
            shader_sm5_read_dcl_resource_structured},
    {VKD3D_SM5_OP_LD_UAV_TYPED,                     VKD3DSIH_LD_UAV_TYPED,                     "u",    "iU"},
    {VKD3D_SM5_OP_STORE_UAV_TYPED,                  VKD3DSIH_STORE_UAV_TYPED,                  "U",    "iu"},
    {VKD3D_SM5_OP_LD_RAW,                           VKD3DSIH_LD_RAW,                           "u",    "iU"},
    {VKD3D_SM5_OP_STORE_RAW,                        VKD3DSIH_STORE_RAW,                        "U",    "uu"},
    {VKD3D_SM5_OP_LD_STRUCTURED,                    VKD3DSIH_LD_STRUCTURED,                    "u",    "iiR"},
    {VKD3D_SM5_OP_STORE_STRUCTURED,                 VKD3DSIH_STORE_STRUCTURED,                 "U",    "iiu"},
    {VKD3D_SM5_OP_ATOMIC_AND,                       VKD3DSIH_ATOMIC_AND,                       "U",    "iu"},
    {VKD3D_SM5_OP_ATOMIC_OR,                        VKD3DSIH_ATOMIC_OR,                        "U",    "iu"},
    {VKD3D_SM5_OP_ATOMIC_XOR,                       VKD3DSIH_ATOMIC_XOR,                       "U",    "iu"},
    {VKD3D_SM5_OP_ATOMIC_CMP_STORE,                 VKD3DSIH_ATOMIC_CMP_STORE,                 "U",    "iuu"},
    {VKD3D_SM5_OP_ATOMIC_IADD,                      VKD3DSIH_ATOMIC_IADD,                      "U",    "ii"},
    {VKD3D_SM5_OP_ATOMIC_IMAX,                      VKD3DSIH_ATOMIC_IMAX,                      "U",    "ii"},
    {VKD3D_SM5_OP_ATOMIC_IMIN,                      VKD3DSIH_ATOMIC_IMIN,                      "U",    "ii"},
    {VKD3D_SM5_OP_ATOMIC_UMAX,                      VKD3DSIH_ATOMIC_UMAX,                      "U",    "iu"},
    {VKD3D_SM5_OP_ATOMIC_UMIN,                      VKD3DSIH_ATOMIC_UMIN,                      "U",    "iu"},
    {VKD3D_SM5_OP_IMM_ATOMIC_ALLOC,                 VKD3DSIH_IMM_ATOMIC_ALLOC,                 "u",    "U"},
    {VKD3D_SM5_OP_IMM_ATOMIC_CONSUME,               VKD3DSIH_IMM_ATOMIC_CONSUME,               "u",    "U"},
    {VKD3D_SM5_OP_IMM_ATOMIC_IADD,                  VKD3DSIH_IMM_ATOMIC_IADD,                  "uU",   "ii"},
    {VKD3D_SM5_OP_IMM_ATOMIC_AND,                   VKD3DSIH_IMM_ATOMIC_AND,                   "uU",   "iu"},
    {VKD3D_SM5_OP_IMM_ATOMIC_OR,                    VKD3DSIH_IMM_ATOMIC_OR,                    "uU",   "iu"},
    {VKD3D_SM5_OP_IMM_ATOMIC_XOR,                   VKD3DSIH_IMM_ATOMIC_XOR,                   "uU",   "iu"},
    {VKD3D_SM5_OP_IMM_ATOMIC_EXCH,                  VKD3DSIH_IMM_ATOMIC_EXCH,                  "uU",   "iu"},
    {VKD3D_SM5_OP_IMM_ATOMIC_CMP_EXCH,              VKD3DSIH_IMM_ATOMIC_CMP_EXCH,              "uU",   "iuu"},
    {VKD3D_SM5_OP_IMM_ATOMIC_IMAX,                  VKD3DSIH_IMM_ATOMIC_IMAX,                  "iU",   "ii"},
    {VKD3D_SM5_OP_IMM_ATOMIC_IMIN,                  VKD3DSIH_IMM_ATOMIC_IMIN,                  "iU",   "ii"},
    {VKD3D_SM5_OP_IMM_ATOMIC_UMAX,                  VKD3DSIH_IMM_ATOMIC_UMAX,                  "uU",   "iu"},
    {VKD3D_SM5_OP_IMM_ATOMIC_UMIN,                  VKD3DSIH_IMM_ATOMIC_UMIN,                  "uU",   "iu"},
    {VKD3D_SM5_OP_SYNC,                             VKD3DSIH_SYNC,                             "",     "",
            shader_sm5_read_sync},
    {VKD3D_SM5_OP_DADD,                             VKD3DSIH_DADD,                             "d",    "dd"},
    {VKD3D_SM5_OP_DMAX,                             VKD3DSIH_DMAX,                             "d",    "dd"},
    {VKD3D_SM5_OP_DMIN,                             VKD3DSIH_DMIN,                             "d",    "dd"},
    {VKD3D_SM5_OP_DMUL,                             VKD3DSIH_DMUL,                             "d",    "dd"},
    {VKD3D_SM5_OP_DEQ,                              VKD3DSIH_DEQ,                              "u",    "dd"},
    {VKD3D_SM5_OP_DGE,                              VKD3DSIH_DGE,                              "u",    "dd"},
    {VKD3D_SM5_OP_DLT,                              VKD3DSIH_DLT,                              "u",    "dd"},
    {VKD3D_SM5_OP_DNE,                              VKD3DSIH_DNE,                              "u",    "dd"},
    {VKD3D_SM5_OP_DMOV,                             VKD3DSIH_DMOV,                             "d",    "d"},
    {VKD3D_SM5_OP_DMOVC,                            VKD3DSIH_DMOVC,                            "d",    "udd"},
    {VKD3D_SM5_OP_DTOF,                             VKD3DSIH_DTOF,                             "f",    "d"},
    {VKD3D_SM5_OP_FTOD,                             VKD3DSIH_FTOD,                             "d",    "f"},
    {VKD3D_SM5_OP_EVAL_SAMPLE_INDEX,                VKD3DSIH_EVAL_SAMPLE_INDEX,                "f",    "fi"},
    {VKD3D_SM5_OP_EVAL_CENTROID,                    VKD3DSIH_EVAL_CENTROID,                    "f",    "f"},
    {VKD3D_SM5_OP_DCL_GS_INSTANCES,                 VKD3DSIH_DCL_GS_INSTANCES,                 "",     "",
            shader_sm4_read_declaration_count},
    {VKD3D_SM5_OP_DDIV,                             VKD3DSIH_DDIV,                             "d",    "dd"},
    {VKD3D_SM5_OP_DFMA,                             VKD3DSIH_DFMA,                             "d",    "ddd"},
    {VKD3D_SM5_OP_DRCP,                             VKD3DSIH_DRCP,                             "d",    "d"},
    {VKD3D_SM5_OP_MSAD,                             VKD3DSIH_MSAD,                             "u",    "uuu"},
    {VKD3D_SM5_OP_DTOI,                             VKD3DSIH_DTOI,                             "i",    "d"},
    {VKD3D_SM5_OP_DTOU,                             VKD3DSIH_DTOU,                             "u",    "d"},
    {VKD3D_SM5_OP_ITOD,                             VKD3DSIH_ITOD,                             "d",    "i"},
    {VKD3D_SM5_OP_UTOD,                             VKD3DSIH_UTOD,                             "d",    "u"},
    {VKD3D_SM5_OP_GATHER4_S,                        VKD3DSIH_GATHER4_S,                        "uu",   "fRS"},
    {VKD3D_SM5_OP_GATHER4_C_S,                      VKD3DSIH_GATHER4_C_S,                      "fu",   "fRSf"},
    {VKD3D_SM5_OP_GATHER4_PO_S,                     VKD3DSIH_GATHER4_PO_S,                     "fu",   "fiRS"},
    {VKD3D_SM5_OP_GATHER4_PO_C_S,                   VKD3DSIH_GATHER4_PO_C_S,                   "fu",   "fiRSf"},
    {VKD3D_SM5_OP_LD_S,                             VKD3DSIH_LD_S,                             "uu",   "iR"},
    {VKD3D_SM5_OP_LD2DMS_S,                         VKD3DSIH_LD2DMS_S,                         "uu",   "iRi"},
    {VKD3D_SM5_OP_LD_UAV_TYPED_S,                   VKD3DSIH_LD_UAV_TYPED_S,                   "uu",   "iU"},
    {VKD3D_SM5_OP_LD_RAW_S,                         VKD3DSIH_LD_RAW_S,                         "uu",   "iU"},
    {VKD3D_SM5_OP_LD_STRUCTURED_S,                  VKD3DSIH_LD_STRUCTURED_S,                  "uu",   "iiR"},
    {VKD3D_SM5_OP_SAMPLE_LOD_S,                     VKD3DSIH_SAMPLE_LOD_S,                     "uu",   "fRSf"},
    {VKD3D_SM5_OP_SAMPLE_C_LZ_S,                    VKD3DSIH_SAMPLE_C_LZ_S,                    "fu",   "fRSf"},
    {VKD3D_SM5_OP_SAMPLE_CL_S,                      VKD3DSIH_SAMPLE_CL_S,                      "uu",   "fRSf"},
    {VKD3D_SM5_OP_SAMPLE_B_CL_S,                    VKD3DSIH_SAMPLE_B_CL_S,                    "uu",   "fRSff"},
    {VKD3D_SM5_OP_SAMPLE_GRAD_CL_S,                 VKD3DSIH_SAMPLE_GRAD_CL_S,                 "uu",   "fRSfff"},
    {VKD3D_SM5_OP_SAMPLE_C_CL_S,                    VKD3DSIH_SAMPLE_C_CL_S,                    "fu",   "fRSff"},
    {VKD3D_SM5_OP_CHECK_ACCESS_FULLY_MAPPED,        VKD3DSIH_CHECK_ACCESS_FULLY_MAPPED,        "u",    "u"},
};

static const enum vkd3d_shader_register_type register_type_table[] =
{
    /* VKD3D_SM4_RT_TEMP */                    VKD3DSPR_TEMP,
    /* VKD3D_SM4_RT_INPUT */                   VKD3DSPR_INPUT,
    /* VKD3D_SM4_RT_OUTPUT */                  VKD3DSPR_OUTPUT,
    /* VKD3D_SM4_RT_INDEXABLE_TEMP */          VKD3DSPR_IDXTEMP,
    /* VKD3D_SM4_RT_IMMCONST */                VKD3DSPR_IMMCONST,
    /* VKD3D_SM4_RT_IMMCONST64 */              VKD3DSPR_IMMCONST64,
    /* VKD3D_SM4_RT_SAMPLER */                 VKD3DSPR_SAMPLER,
    /* VKD3D_SM4_RT_RESOURCE */                VKD3DSPR_RESOURCE,
    /* VKD3D_SM4_RT_CONSTBUFFER */             VKD3DSPR_CONSTBUFFER,
    /* VKD3D_SM4_RT_IMMCONSTBUFFER */          VKD3DSPR_IMMCONSTBUFFER,
    /* UNKNOWN */                              ~0u,
    /* VKD3D_SM4_RT_PRIMID */                  VKD3DSPR_PRIMID,
    /* VKD3D_SM4_RT_DEPTHOUT */                VKD3DSPR_DEPTHOUT,
    /* VKD3D_SM4_RT_NULL */                    VKD3DSPR_NULL,
    /* VKD3D_SM4_RT_RASTERIZER */              VKD3DSPR_RASTERIZER,
    /* VKD3D_SM4_RT_OMASK */                   VKD3DSPR_SAMPLEMASK,
    /* VKD3D_SM5_RT_STREAM */                  VKD3DSPR_STREAM,
    /* VKD3D_SM5_RT_FUNCTION_BODY */           VKD3DSPR_FUNCTIONBODY,
    /* UNKNOWN */                              ~0u,
    /* VKD3D_SM5_RT_FUNCTION_POINTER */        VKD3DSPR_FUNCTIONPOINTER,
    /* UNKNOWN */                              ~0u,
    /* UNKNOWN */                              ~0u,
    /* VKD3D_SM5_RT_OUTPUT_CONTROL_POINT_ID */ VKD3DSPR_OUTPOINTID,
    /* VKD3D_SM5_RT_FORK_INSTANCE_ID */        VKD3DSPR_FORKINSTID,
    /* VKD3D_SM5_RT_JOIN_INSTANCE_ID */        VKD3DSPR_JOININSTID,
    /* VKD3D_SM5_RT_INPUT_CONTROL_POINT */     VKD3DSPR_INCONTROLPOINT,
    /* VKD3D_SM5_RT_OUTPUT_CONTROL_POINT */    VKD3DSPR_OUTCONTROLPOINT,
    /* VKD3D_SM5_RT_PATCH_CONSTANT_DATA */     VKD3DSPR_PATCHCONST,
    /* VKD3D_SM5_RT_DOMAIN_LOCATION */         VKD3DSPR_TESSCOORD,
    /* UNKNOWN */                              ~0u,
    /* VKD3D_SM5_RT_UAV */                     VKD3DSPR_UAV,
    /* VKD3D_SM5_RT_SHARED_MEMORY */           VKD3DSPR_GROUPSHAREDMEM,
    /* VKD3D_SM5_RT_THREAD_ID */               VKD3DSPR_THREADID,
    /* VKD3D_SM5_RT_THREAD_GROUP_ID */         VKD3DSPR_THREADGROUPID,
    /* VKD3D_SM5_RT_LOCAL_THREAD_ID */         VKD3DSPR_LOCALTHREADID,
    /* VKD3D_SM5_RT_COVERAGE */                VKD3DSPR_COVERAGE,
    /* VKD3D_SM5_RT_LOCAL_THREAD_INDEX */      VKD3DSPR_LOCALTHREADINDEX,
    /* VKD3D_SM5_RT_GS_INSTANCE_ID */          VKD3DSPR_GSINSTID,
    /* VKD3D_SM5_RT_DEPTHOUT_GREATER_EQUAL */  VKD3DSPR_DEPTHOUTGE,
    /* VKD3D_SM5_RT_DEPTHOUT_LESS_EQUAL */     VKD3DSPR_DEPTHOUTLE,
    /* VKD3D_SM5_RT_CYCLE_COUNTER */           ~0u,
    /* VKD3D_SM5_RT_OUTPUT_STENCIL_REF */      VKD3DSPR_OUTSTENCILREF,
};

static const enum vkd3d_shader_register_precision register_precision_table[] =
{
    /* VKD3D_SM4_REGISTER_PRECISION_DEFAULT */      VKD3D_SHADER_REGISTER_PRECISION_DEFAULT,
    /* VKD3D_SM4_REGISTER_PRECISION_MIN_FLOAT_16 */ VKD3D_SHADER_REGISTER_PRECISION_MIN_FLOAT_16,
    /* VKD3D_SM4_REGISTER_PRECISION_MIN_FLOAT_10 */ VKD3D_SHADER_REGISTER_PRECISION_MIN_FLOAT_10,
    /* UNKNOWN */                                   VKD3D_SHADER_REGISTER_PRECISION_INVALID,
    /* VKD3D_SM4_REGISTER_PRECISION_MIN_INT_16 */   VKD3D_SHADER_REGISTER_PRECISION_MIN_INT_16,
    /* VKD3D_SM4_REGISTER_PRECISION_MIN_UINT_16 */  VKD3D_SHADER_REGISTER_PRECISION_MIN_UINT_16,
};

static const struct vkd3d_sm4_opcode_info *get_opcode_info(enum vkd3d_sm4_opcode opcode)
{
    unsigned int i;

    for (i = 0; i < sizeof(opcode_table) / sizeof(*opcode_table); ++i)
    {
        if (opcode == opcode_table[i].opcode) return &opcode_table[i];
    }

    return NULL;
}

static void map_register(const struct vkd3d_shader_sm4_parser *sm4, struct vkd3d_shader_register *reg)
{
    switch (sm4->p.shader_version.type)
    {
        case VKD3D_SHADER_TYPE_PIXEL:
            if (reg->type == VKD3DSPR_OUTPUT)
            {
                unsigned int reg_idx = reg->idx[0].offset;

                if (reg_idx >= ARRAY_SIZE(sm4->output_map))
                {
                    ERR("Invalid output index %u.\n", reg_idx);
                    break;
                }

                reg->type = VKD3DSPR_COLOROUT;
                reg->idx[0].offset = sm4->output_map[reg_idx];
            }
            break;

        default:
            break;
    }
}

static enum vkd3d_data_type map_data_type(char t)
{
    switch (t)
    {
        case 'd':
            return VKD3D_DATA_DOUBLE;
        case 'f':
            return VKD3D_DATA_FLOAT;
        case 'i':
            return VKD3D_DATA_INT;
        case 'u':
            return VKD3D_DATA_UINT;
        case 'O':
            return VKD3D_DATA_OPAQUE;
        case 'R':
            return VKD3D_DATA_RESOURCE;
        case 'S':
            return VKD3D_DATA_SAMPLER;
        case 'U':
            return VKD3D_DATA_UAV;
        default:
            ERR("Invalid data type '%c'.\n", t);
            return VKD3D_DATA_FLOAT;
    }
}

static void shader_sm4_destroy(struct vkd3d_shader_parser *parser)
{
    struct vkd3d_shader_sm4_parser *sm4 = vkd3d_shader_sm4_parser(parser);

    shader_instruction_array_destroy(&parser->instructions);
    free_shader_desc(&parser->shader_desc);
    vkd3d_free(sm4);
}

static bool shader_sm4_read_reg_idx(struct vkd3d_shader_sm4_parser *priv, const uint32_t **ptr,
        const uint32_t *end, uint32_t addressing, struct vkd3d_shader_register_index *reg_idx)
{
    if (addressing & VKD3D_SM4_ADDRESSING_RELATIVE)
    {
        struct vkd3d_shader_src_param *rel_addr = shader_parser_get_src_params(&priv->p, 1);

        if (!(reg_idx->rel_addr = rel_addr))
        {
            ERR("Failed to get src param for relative addressing.\n");
            return false;
        }

        if (addressing & VKD3D_SM4_ADDRESSING_OFFSET)
            reg_idx->offset = *(*ptr)++;
        else
            reg_idx->offset = 0;
        shader_sm4_read_src_param(priv, ptr, end, VKD3D_DATA_INT, rel_addr);
    }
    else
    {
        reg_idx->rel_addr = NULL;
        reg_idx->offset = *(*ptr)++;
    }

    return true;
}

static bool sm4_register_is_descriptor(enum vkd3d_sm4_register_type register_type)
{
    switch (register_type)
    {
        case VKD3D_SM4_RT_SAMPLER:
        case VKD3D_SM4_RT_RESOURCE:
        case VKD3D_SM4_RT_CONSTBUFFER:
        case VKD3D_SM5_RT_UAV:
            return true;

        default:
            return false;
    }
}

static bool shader_sm4_read_param(struct vkd3d_shader_sm4_parser *priv, const uint32_t **ptr, const uint32_t *end,
        enum vkd3d_data_type data_type, struct vkd3d_shader_register *param, enum vkd3d_shader_src_modifier *modifier)
{
    enum vkd3d_sm4_register_precision precision;
    enum vkd3d_sm4_register_type register_type;
    enum vkd3d_sm4_extended_operand_type type;
    enum vkd3d_sm4_register_modifier m;
    uint32_t token, order, extended;

    if (*ptr >= end)
    {
        WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
        return false;
    }
    token = *(*ptr)++;

    register_type = (token & VKD3D_SM4_REGISTER_TYPE_MASK) >> VKD3D_SM4_REGISTER_TYPE_SHIFT;
    if (register_type >= ARRAY_SIZE(register_type_table)
            || register_type_table[register_type] == VKD3DSPR_INVALID)
    {
        FIXME("Unhandled register type %#x.\n", register_type);
        param->type = VKD3DSPR_TEMP;
    }
    else
    {
        param->type = register_type_table[register_type];
    }
    param->precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    param->non_uniform = false;
    param->data_type = data_type;

    *modifier = VKD3DSPSM_NONE;
    if (token & VKD3D_SM4_EXTENDED_OPERAND)
    {
        if (*ptr >= end)
        {
            WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
            return false;
        }
        extended = *(*ptr)++;

        if (extended & VKD3D_SM4_EXTENDED_OPERAND)
        {
            FIXME("Skipping second-order extended operand.\n");
            *ptr += *ptr < end;
        }

        type = extended & VKD3D_SM4_EXTENDED_OPERAND_TYPE_MASK;
        if (type == VKD3D_SM4_EXTENDED_OPERAND_MODIFIER)
        {
            m = (extended & VKD3D_SM4_REGISTER_MODIFIER_MASK) >> VKD3D_SM4_REGISTER_MODIFIER_SHIFT;
            switch (m)
            {
                case VKD3D_SM4_REGISTER_MODIFIER_NEGATE:
                    *modifier = VKD3DSPSM_NEG;
                    break;

                case VKD3D_SM4_REGISTER_MODIFIER_ABS:
                    *modifier = VKD3DSPSM_ABS;
                    break;

                case VKD3D_SM4_REGISTER_MODIFIER_ABS_NEGATE:
                    *modifier = VKD3DSPSM_ABSNEG;
                    break;

                default:
                    FIXME("Unhandled register modifier %#x.\n", m);
                    /* fall-through */
                case VKD3D_SM4_REGISTER_MODIFIER_NONE:
                    break;
            }

            precision = (extended & VKD3D_SM4_REGISTER_PRECISION_MASK) >> VKD3D_SM4_REGISTER_PRECISION_SHIFT;
            if (precision >= ARRAY_SIZE(register_precision_table)
                    || register_precision_table[precision] == VKD3D_SHADER_REGISTER_PRECISION_INVALID)
            {
                FIXME("Unhandled register precision %#x.\n", precision);
                param->precision = VKD3D_SHADER_REGISTER_PRECISION_INVALID;
            }
            else
            {
                param->precision = register_precision_table[precision];
            }

            if (extended & VKD3D_SM4_REGISTER_NON_UNIFORM_MASK)
                param->non_uniform = true;

            extended &= ~(VKD3D_SM4_EXTENDED_OPERAND_TYPE_MASK | VKD3D_SM4_REGISTER_MODIFIER_MASK
                    | VKD3D_SM4_REGISTER_PRECISION_MASK | VKD3D_SM4_REGISTER_NON_UNIFORM_MASK
                    | VKD3D_SM4_EXTENDED_OPERAND);
            if (extended)
                FIXME("Skipping unhandled extended operand bits 0x%08x.\n", extended);
        }
        else if (type)
        {
            FIXME("Skipping unhandled extended operand token 0x%08x (type %#x).\n", extended, type);
        }
    }

    order = (token & VKD3D_SM4_REGISTER_ORDER_MASK) >> VKD3D_SM4_REGISTER_ORDER_SHIFT;

    if (order < 1)
    {
        param->idx[0].offset = ~0u;
        param->idx[0].rel_addr = NULL;
    }
    else
    {
        DWORD addressing = (token & VKD3D_SM4_ADDRESSING_MASK0) >> VKD3D_SM4_ADDRESSING_SHIFT0;
        if (!(shader_sm4_read_reg_idx(priv, ptr, end, addressing, &param->idx[0])))
        {
            ERR("Failed to read register index.\n");
            return false;
        }
    }

    if (order < 2)
    {
        param->idx[1].offset = ~0u;
        param->idx[1].rel_addr = NULL;
    }
    else
    {
        DWORD addressing = (token & VKD3D_SM4_ADDRESSING_MASK1) >> VKD3D_SM4_ADDRESSING_SHIFT1;
        if (!(shader_sm4_read_reg_idx(priv, ptr, end, addressing, &param->idx[1])))
        {
            ERR("Failed to read register index.\n");
            return false;
        }
    }

    if (order < 3)
    {
        param->idx[2].offset = ~0u;
        param->idx[2].rel_addr = NULL;
    }
    else
    {
        DWORD addressing = (token & VKD3D_SM4_ADDRESSING_MASK2) >> VKD3D_SM4_ADDRESSING_SHIFT2;
        if (!(shader_sm4_read_reg_idx(priv, ptr, end, addressing, &param->idx[2])))
        {
            ERR("Failed to read register index.\n");
            return false;
        }
    }

    if (order > 3)
    {
        WARN("Unhandled order %u.\n", order);
        return false;
    }

    if (register_type == VKD3D_SM4_RT_IMMCONST || register_type == VKD3D_SM4_RT_IMMCONST64)
    {
        enum vkd3d_sm4_dimension dimension = (token & VKD3D_SM4_DIMENSION_MASK) >> VKD3D_SM4_DIMENSION_SHIFT;
        unsigned int dword_count;

        switch (dimension)
        {
            case VKD3D_SM4_DIMENSION_SCALAR:
                param->immconst_type = VKD3D_IMMCONST_SCALAR;
                dword_count = 1 + (register_type == VKD3D_SM4_RT_IMMCONST64);
                if (end - *ptr < dword_count)
                {
                    WARN("Invalid ptr %p, end %p.\n", *ptr, end);
                    return false;
                }
                memcpy(param->u.immconst_uint, *ptr, dword_count * sizeof(DWORD));
                *ptr += dword_count;
                break;

            case VKD3D_SM4_DIMENSION_VEC4:
                param->immconst_type = VKD3D_IMMCONST_VEC4;
                if (end - *ptr < VKD3D_VEC4_SIZE)
                {
                    WARN("Invalid ptr %p, end %p.\n", *ptr, end);
                    return false;
                }
                memcpy(param->u.immconst_uint, *ptr, VKD3D_VEC4_SIZE * sizeof(DWORD));
                *ptr += 4;
                break;

            default:
                FIXME("Unhandled dimension %#x.\n", dimension);
                break;
        }
    }
    else if (!shader_is_sm_5_1(priv) && sm4_register_is_descriptor(register_type))
    {
        /* SM5.1 places a symbol identifier in idx[0] and moves
         * other values up one slot. Normalize to SM5.1. */
        param->idx[2] = param->idx[1];
        param->idx[1] = param->idx[0];
    }

    map_register(priv, param);

    return true;
}

static bool shader_sm4_is_scalar_register(const struct vkd3d_shader_register *reg)
{
    switch (reg->type)
    {
        case VKD3DSPR_COVERAGE:
        case VKD3DSPR_DEPTHOUT:
        case VKD3DSPR_DEPTHOUTGE:
        case VKD3DSPR_DEPTHOUTLE:
        case VKD3DSPR_GSINSTID:
        case VKD3DSPR_LOCALTHREADINDEX:
        case VKD3DSPR_OUTPOINTID:
        case VKD3DSPR_PRIMID:
        case VKD3DSPR_SAMPLEMASK:
        case VKD3DSPR_OUTSTENCILREF:
            return true;
        default:
            return false;
    }
}

static uint32_t swizzle_from_sm4(uint32_t s)
{
    return vkd3d_shader_create_swizzle(s & 0x3, (s >> 2) & 0x3, (s >> 4) & 0x3, (s >> 6) & 0x3);
}

static bool shader_sm4_read_src_param(struct vkd3d_shader_sm4_parser *priv, const uint32_t **ptr,
        const uint32_t *end, enum vkd3d_data_type data_type, struct vkd3d_shader_src_param *src_param)
{
    DWORD token;

    if (*ptr >= end)
    {
        WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
        return false;
    }
    token = **ptr;

    if (!shader_sm4_read_param(priv, ptr, end, data_type, &src_param->reg, &src_param->modifiers))
    {
        ERR("Failed to read parameter.\n");
        return false;
    }

    if (src_param->reg.type == VKD3DSPR_IMMCONST || src_param->reg.type == VKD3DSPR_IMMCONST64)
    {
        src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    }
    else
    {
        enum vkd3d_sm4_swizzle_type swizzle_type =
                (token & VKD3D_SM4_SWIZZLE_TYPE_MASK) >> VKD3D_SM4_SWIZZLE_TYPE_SHIFT;

        switch (swizzle_type)
        {
            case VKD3D_SM4_SWIZZLE_NONE:
                if (shader_sm4_is_scalar_register(&src_param->reg))
                    src_param->swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
                else
                    src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
                break;

            case VKD3D_SM4_SWIZZLE_SCALAR:
                src_param->swizzle = (token & VKD3D_SM4_SWIZZLE_MASK) >> VKD3D_SM4_SWIZZLE_SHIFT;
                src_param->swizzle = (src_param->swizzle & 0x3) * 0x01010101;
                break;

            case VKD3D_SM4_SWIZZLE_VEC4:
                src_param->swizzle = swizzle_from_sm4((token & VKD3D_SM4_SWIZZLE_MASK) >> VKD3D_SM4_SWIZZLE_SHIFT);
                break;

            default:
                FIXME("Unhandled swizzle type %#x.\n", swizzle_type);
                break;
        }
    }

    return true;
}

static bool shader_sm4_read_dst_param(struct vkd3d_shader_sm4_parser *priv, const uint32_t **ptr,
        const uint32_t *end, enum vkd3d_data_type data_type, struct vkd3d_shader_dst_param *dst_param)
{
    enum vkd3d_shader_src_modifier modifier;
    DWORD token;

    if (*ptr >= end)
    {
        WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
        return false;
    }
    token = **ptr;

    if (!shader_sm4_read_param(priv, ptr, end, data_type, &dst_param->reg, &modifier))
    {
        ERR("Failed to read parameter.\n");
        return false;
    }

    if (modifier != VKD3DSPSM_NONE)
    {
        ERR("Invalid source modifier %#x on destination register.\n", modifier);
        return false;
    }

    dst_param->write_mask = (token & VKD3D_SM4_WRITEMASK_MASK) >> VKD3D_SM4_WRITEMASK_SHIFT;
    if (data_type == VKD3D_DATA_DOUBLE)
        dst_param->write_mask = vkd3d_write_mask_64_from_32(dst_param->write_mask);
    /* Scalar registers are declared with no write mask in shader bytecode. */
    if (!dst_param->write_mask && shader_sm4_is_scalar_register(&dst_param->reg))
        dst_param->write_mask = VKD3DSP_WRITEMASK_0;
    dst_param->modifiers = 0;
    dst_param->shift = 0;

    return true;
}

static void shader_sm4_read_instruction_modifier(DWORD modifier, struct vkd3d_shader_instruction *ins)
{
    enum vkd3d_sm4_instruction_modifier modifier_type = modifier & VKD3D_SM4_MODIFIER_MASK;

    switch (modifier_type)
    {
        case VKD3D_SM4_MODIFIER_AOFFIMMI:
        {
            static const DWORD recognized_bits = VKD3D_SM4_INSTRUCTION_MODIFIER
                    | VKD3D_SM4_MODIFIER_MASK
                    | VKD3D_SM4_AOFFIMMI_U_MASK
                    | VKD3D_SM4_AOFFIMMI_V_MASK
                    | VKD3D_SM4_AOFFIMMI_W_MASK;

            /* Bit fields are used for sign extension. */
            struct
            {
                int u : 4;
                int v : 4;
                int w : 4;
            } aoffimmi;

            if (modifier & ~recognized_bits)
                FIXME("Unhandled instruction modifier %#x.\n", modifier);

            aoffimmi.u = (modifier & VKD3D_SM4_AOFFIMMI_U_MASK) >> VKD3D_SM4_AOFFIMMI_U_SHIFT;
            aoffimmi.v = (modifier & VKD3D_SM4_AOFFIMMI_V_MASK) >> VKD3D_SM4_AOFFIMMI_V_SHIFT;
            aoffimmi.w = (modifier & VKD3D_SM4_AOFFIMMI_W_MASK) >> VKD3D_SM4_AOFFIMMI_W_SHIFT;
            ins->texel_offset.u = aoffimmi.u;
            ins->texel_offset.v = aoffimmi.v;
            ins->texel_offset.w = aoffimmi.w;
            break;
        }

        case VKD3D_SM5_MODIFIER_DATA_TYPE:
        {
            DWORD components = (modifier & VKD3D_SM5_MODIFIER_DATA_TYPE_MASK) >> VKD3D_SM5_MODIFIER_DATA_TYPE_SHIFT;
            unsigned int i;

            for (i = 0; i < VKD3D_VEC4_SIZE; i++)
            {
                enum vkd3d_sm4_data_type data_type = VKD3D_SM4_TYPE_COMPONENT(components, i);

                if (!data_type || (data_type >= ARRAY_SIZE(data_type_table)))
                {
                    FIXME("Unhandled data type %#x.\n", data_type);
                    ins->resource_data_type[i] = VKD3D_DATA_FLOAT;
                }
                else
                {
                    ins->resource_data_type[i] = data_type_table[data_type];
                }
            }
            break;
        }

        case VKD3D_SM5_MODIFIER_RESOURCE_TYPE:
        {
            enum vkd3d_sm4_resource_type resource_type
                    = (modifier & VKD3D_SM5_MODIFIER_RESOURCE_TYPE_MASK) >> VKD3D_SM5_MODIFIER_RESOURCE_TYPE_SHIFT;

            if (resource_type == VKD3D_SM4_RESOURCE_RAW_BUFFER)
                ins->raw = true;
            else if (resource_type == VKD3D_SM4_RESOURCE_STRUCTURED_BUFFER)
                ins->structured = true;

            if (resource_type < ARRAY_SIZE(resource_type_table))
                ins->resource_type = resource_type_table[resource_type];
            else
            {
                FIXME("Unhandled resource type %#x.\n", resource_type);
                ins->resource_type = VKD3D_SHADER_RESOURCE_NONE;
            }

            ins->resource_stride
                    = (modifier & VKD3D_SM5_MODIFIER_RESOURCE_STRIDE_MASK) >> VKD3D_SM5_MODIFIER_RESOURCE_STRIDE_SHIFT;
            break;
        }

        default:
            FIXME("Unhandled instruction modifier %#x.\n", modifier);
    }
}

static void shader_sm4_read_instruction(struct vkd3d_shader_parser *parser, struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_shader_sm4_parser *sm4 = vkd3d_shader_sm4_parser(parser);
    const struct vkd3d_sm4_opcode_info *opcode_info;
    uint32_t opcode_token, opcode, previous_token;
    struct vkd3d_shader_dst_param *dst_params;
    struct vkd3d_shader_src_param *src_params;
    const uint32_t **ptr = &parser->ptr;
    unsigned int i, len;
    size_t remaining;
    const uint32_t *p;
    DWORD precise;

    if (*ptr >= sm4->end)
    {
        WARN("End of byte-code, failed to read opcode.\n");
        goto fail;
    }
    remaining = sm4->end - *ptr;

    ++parser->location.line;

    opcode_token = *(*ptr)++;
    opcode = opcode_token & VKD3D_SM4_OPCODE_MASK;

    len = ((opcode_token & VKD3D_SM4_INSTRUCTION_LENGTH_MASK) >> VKD3D_SM4_INSTRUCTION_LENGTH_SHIFT);
    if (!len)
    {
        if (remaining < 2)
        {
            WARN("End of byte-code, failed to read length token.\n");
            goto fail;
        }
        len = **ptr;
    }
    if (!len || remaining < len)
    {
        WARN("Read invalid length %u (remaining %zu).\n", len, remaining);
        goto fail;
    }
    --len;

    if (!(opcode_info = get_opcode_info(opcode)))
    {
        FIXME("Unrecognized opcode %#x, opcode_token 0x%08x.\n", opcode, opcode_token);
        ins->handler_idx = VKD3DSIH_INVALID;
        *ptr += len;
        return;
    }

    ins->handler_idx = opcode_info->handler_idx;
    ins->flags = 0;
    ins->coissue = false;
    ins->raw = false;
    ins->structured = false;
    ins->predicate = NULL;
    ins->dst_count = strnlen(opcode_info->dst_info, SM4_MAX_DST_COUNT);
    ins->src_count = strnlen(opcode_info->src_info, SM4_MAX_SRC_COUNT);
    ins->src = src_params = shader_parser_get_src_params(parser, ins->src_count);
    if (!src_params && ins->src_count)
    {
        ERR("Failed to allocate src parameters.\n");
        vkd3d_shader_parser_error(parser, VKD3D_SHADER_ERROR_TPF_OUT_OF_MEMORY, "Out of memory.");
        ins->handler_idx = VKD3DSIH_INVALID;
        return;
    }
    ins->resource_type = VKD3D_SHADER_RESOURCE_NONE;
    ins->resource_stride = 0;
    ins->resource_data_type[0] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[1] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[2] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[3] = VKD3D_DATA_FLOAT;
    memset(&ins->texel_offset, 0, sizeof(ins->texel_offset));

    p = *ptr;
    *ptr += len;

    if (opcode_info->read_opcode_func)
    {
        ins->dst = NULL;
        ins->dst_count = 0;
        opcode_info->read_opcode_func(ins, opcode, opcode_token, p, len, sm4);
    }
    else
    {
        enum vkd3d_shader_dst_modifier instruction_dst_modifier = VKD3DSPDM_NONE;

        previous_token = opcode_token;
        while (previous_token & VKD3D_SM4_INSTRUCTION_MODIFIER && p != *ptr)
            shader_sm4_read_instruction_modifier(previous_token = *p++, ins);

        ins->flags = (opcode_token & VKD3D_SM4_INSTRUCTION_FLAGS_MASK) >> VKD3D_SM4_INSTRUCTION_FLAGS_SHIFT;
        if (ins->flags & VKD3D_SM4_INSTRUCTION_FLAG_SATURATE)
        {
            ins->flags &= ~VKD3D_SM4_INSTRUCTION_FLAG_SATURATE;
            instruction_dst_modifier = VKD3DSPDM_SATURATE;
        }
        precise = (opcode_token & VKD3D_SM5_PRECISE_MASK) >> VKD3D_SM5_PRECISE_SHIFT;
        ins->flags |= precise << VKD3DSI_PRECISE_SHIFT;

        ins->dst = dst_params = shader_parser_get_dst_params(parser, ins->dst_count);
        if (!dst_params && ins->dst_count)
        {
            ERR("Failed to allocate dst parameters.\n");
            vkd3d_shader_parser_error(parser, VKD3D_SHADER_ERROR_TPF_OUT_OF_MEMORY, "Out of memory.");
            ins->handler_idx = VKD3DSIH_INVALID;
            return;
        }
        for (i = 0; i < ins->dst_count; ++i)
        {
            if (!(shader_sm4_read_dst_param(sm4, &p, *ptr, map_data_type(opcode_info->dst_info[i]),
                    &dst_params[i])))
            {
                ins->handler_idx = VKD3DSIH_INVALID;
                return;
            }
            dst_params[i].modifiers |= instruction_dst_modifier;
        }

        for (i = 0; i < ins->src_count; ++i)
        {
            if (!(shader_sm4_read_src_param(sm4, &p, *ptr, map_data_type(opcode_info->src_info[i]),
                    &src_params[i])))
            {
                ins->handler_idx = VKD3DSIH_INVALID;
                return;
            }
        }
    }

    return;

fail:
    *ptr = sm4->end;
    ins->handler_idx = VKD3DSIH_INVALID;
    return;
}

static bool shader_sm4_is_end(struct vkd3d_shader_parser *parser)
{
    struct vkd3d_shader_sm4_parser *sm4 = vkd3d_shader_sm4_parser(parser);

    return parser->ptr == sm4->end;
}

static const struct vkd3d_shader_parser_ops shader_sm4_parser_ops =
{
    .parser_destroy = shader_sm4_destroy,
};

static bool shader_sm4_init(struct vkd3d_shader_sm4_parser *sm4, const uint32_t *byte_code,
        size_t byte_code_size, const char *source_name, const struct vkd3d_shader_signature *output_signature,
        struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_version version;
    uint32_t version_token, token_count;
    unsigned int i;

    if (byte_code_size / sizeof(*byte_code) < 2)
    {
        WARN("Invalid byte code size %lu.\n", (long)byte_code_size);
        return false;
    }

    version_token = byte_code[0];
    TRACE("Version: 0x%08x.\n", version_token);
    token_count = byte_code[1];
    TRACE("Token count: %u.\n", token_count);

    if (token_count < 2 || byte_code_size / sizeof(*byte_code) < token_count)
    {
        WARN("Invalid token count %u.\n", token_count);
        return false;
    }

    sm4->start = &byte_code[2];
    sm4->end = &byte_code[token_count];

    switch (version_token >> 16)
    {
        case VKD3D_SM4_PS:
            version.type = VKD3D_SHADER_TYPE_PIXEL;
            break;

        case VKD3D_SM4_VS:
            version.type = VKD3D_SHADER_TYPE_VERTEX;
            break;

        case VKD3D_SM4_GS:
            version.type = VKD3D_SHADER_TYPE_GEOMETRY;
            break;

        case VKD3D_SM5_HS:
            version.type = VKD3D_SHADER_TYPE_HULL;
            break;

        case VKD3D_SM5_DS:
            version.type = VKD3D_SHADER_TYPE_DOMAIN;
            break;

        case VKD3D_SM5_CS:
            version.type = VKD3D_SHADER_TYPE_COMPUTE;
            break;

        default:
            FIXME("Unrecognised shader type %#x.\n", version_token >> 16);
    }
    version.major = VKD3D_SM4_VERSION_MAJOR(version_token);
    version.minor = VKD3D_SM4_VERSION_MINOR(version_token);

    /* Estimate instruction count to avoid reallocation in most shaders. */
    if (!vkd3d_shader_parser_init(&sm4->p, message_context, source_name, &version, &shader_sm4_parser_ops,
            token_count / 7u + 20))
        return false;
    sm4->p.ptr = sm4->start;

    memset(sm4->output_map, 0xff, sizeof(sm4->output_map));
    for (i = 0; i < output_signature->element_count; ++i)
    {
        struct vkd3d_shader_signature_element *e = &output_signature->elements[i];

        if (version.type == VKD3D_SHADER_TYPE_PIXEL
                && ascii_strcasecmp(e->semantic_name, "SV_Target"))
            continue;
        if (e->register_index >= ARRAY_SIZE(sm4->output_map))
        {
            WARN("Invalid output index %u.\n", e->register_index);
            continue;
        }

        sm4->output_map[e->register_index] = e->semantic_index;
    }

    return true;
}

static bool require_space(size_t offset, size_t count, size_t size, size_t data_size)
{
    return !count || (data_size - offset) / count >= size;
}

static void read_dword(const char **ptr, uint32_t *d)
{
    memcpy(d, *ptr, sizeof(*d));
    *ptr += sizeof(*d);
}

static void read_float(const char **ptr, float *f)
{
    STATIC_ASSERT(sizeof(float) == sizeof(uint32_t));
    read_dword(ptr, (uint32_t *)f);
}

static void skip_dword_unknown(const char **ptr, unsigned int count)
{
    unsigned int i;
    uint32_t d;

    if (!count)
        return;

    WARN("Skipping %u unknown DWORDs:\n", count);
    for (i = 0; i < count; ++i)
    {
        read_dword(ptr, &d);
        WARN("\t0x%08x\n", d);
    }
}

static const char *shader_get_string(const char *data, size_t data_size, DWORD offset)
{
    size_t len, max_len;

    if (offset >= data_size)
    {
        WARN("Invalid offset %#x (data size %#lx).\n", offset, (long)data_size);
        return NULL;
    }

    max_len = data_size - offset;
    len = strnlen(data + offset, max_len);

    if (len == max_len)
        return NULL;

    return data + offset;
}

static int parse_dxbc(const struct vkd3d_shader_code *dxbc, struct vkd3d_shader_message_context *message_context,
        const char *source_name, struct vkd3d_shader_dxbc_desc *desc)
{
    const struct vkd3d_shader_location location = {.source_name = source_name};
    struct vkd3d_shader_dxbc_section_desc *sections, *section;
    uint32_t checksum[4], calculated_checksum[4];
    const char *data = dxbc->code;
    size_t data_size = dxbc->size;
    const char *ptr = data;
    uint32_t chunk_count;
    uint32_t total_size;
    uint32_t version;
    unsigned int i;
    uint32_t tag;

    if (data_size < VKD3D_DXBC_HEADER_SIZE)
    {
        WARN("Invalid data size %zu.\n", data_size);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_SIZE,
                "DXBC size %zu is smaller than the DXBC header size.", data_size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    read_dword(&ptr, &tag);
    TRACE("tag: %#x.\n", tag);

    if (tag != TAG_DXBC)
    {
        WARN("Wrong tag.\n");
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_MAGIC, "Invalid DXBC magic.");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    read_dword(&ptr, &checksum[0]);
    read_dword(&ptr, &checksum[1]);
    read_dword(&ptr, &checksum[2]);
    read_dword(&ptr, &checksum[3]);
    vkd3d_compute_dxbc_checksum(data, data_size, calculated_checksum);
    if (memcmp(checksum, calculated_checksum, sizeof(checksum)))
    {
        WARN("Checksum {0x%08x, 0x%08x, 0x%08x, 0x%08x} does not match "
                "calculated checksum {0x%08x, 0x%08x, 0x%08x, 0x%08x}.\n",
                checksum[0], checksum[1], checksum[2], checksum[3],
                calculated_checksum[0], calculated_checksum[1],
                calculated_checksum[2], calculated_checksum[3]);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_CHECKSUM,
                "Invalid DXBC checksum.");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    read_dword(&ptr, &version);
    TRACE("version: %#x.\n", version);
    if (version != 0x00000001)
    {
        WARN("Got unexpected DXBC version %#x.\n", version);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_VERSION,
                "DXBC version %#x is not supported.", version);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    read_dword(&ptr, &total_size);
    TRACE("total size: %#x\n", total_size);

    read_dword(&ptr, &chunk_count);
    TRACE("chunk count: %#x\n", chunk_count);

    if (!(sections = vkd3d_calloc(chunk_count, sizeof(*sections))))
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_OUT_OF_MEMORY, "Out of memory.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < chunk_count; ++i)
    {
        uint32_t chunk_tag, chunk_size;
        const char *chunk_ptr;
        uint32_t chunk_offset;

        read_dword(&ptr, &chunk_offset);
        TRACE("chunk %u at offset %#x\n", i, chunk_offset);

        if (chunk_offset >= data_size || !require_space(chunk_offset, 2, sizeof(DWORD), data_size))
        {
            WARN("Invalid chunk offset %#x (data size %zu).\n", chunk_offset, data_size);
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_CHUNK_OFFSET,
                    "DXBC chunk %u has invalid offset %#x (data size %#zx).", i, chunk_offset, data_size);
            vkd3d_free(sections);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }

        chunk_ptr = data + chunk_offset;

        read_dword(&chunk_ptr, &chunk_tag);
        read_dword(&chunk_ptr, &chunk_size);

        if (!require_space(chunk_ptr - data, 1, chunk_size, data_size))
        {
            WARN("Invalid chunk size %#x (data size %zu, chunk offset %#x).\n",
                    chunk_size, data_size, chunk_offset);
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_CHUNK_SIZE,
                    "DXBC chunk %u has invalid size %#x (data size %#zx, chunk offset %#x).",
                    i, chunk_offset, data_size, chunk_offset);
            vkd3d_free(sections);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }

        section = &sections[i];
        section->tag = chunk_tag;
        section->data.code = chunk_ptr;
        section->data.size = chunk_size;
    }

    desc->tag = tag;
    memcpy(desc->checksum, checksum, sizeof(checksum));
    desc->version = version;
    desc->size = total_size;
    desc->section_count = chunk_count;
    desc->sections = sections;

    return VKD3D_OK;
}

void vkd3d_shader_free_dxbc(struct vkd3d_shader_dxbc_desc *dxbc)
{
    TRACE("dxbc %p.\n", dxbc);

    vkd3d_free(dxbc->sections);
}

static int for_each_dxbc_section(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_message_context *message_context, const char *source_name,
        int (*section_handler)(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, void *ctx), void *ctx)
{
    struct vkd3d_shader_dxbc_desc desc;
    unsigned int i;
    int ret;

    if ((ret = parse_dxbc(dxbc, message_context, source_name, &desc)) < 0)
        return ret;

    for (i = 0; i < desc.section_count; ++i)
    {
        if ((ret = section_handler(&desc.sections[i], message_context, ctx)) < 0)
            break;
    }

    vkd3d_shader_free_dxbc(&desc);

    return ret;
}

int vkd3d_shader_parse_dxbc(const struct vkd3d_shader_code *dxbc,
        uint32_t flags, struct vkd3d_shader_dxbc_desc *desc, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("dxbc {%p, %zu}, flags %#x, desc %p, messages %p.\n", dxbc->code, dxbc->size, flags, desc, messages);

    if (messages)
        *messages = NULL;
    vkd3d_shader_message_context_init(&message_context, VKD3D_SHADER_LOG_INFO);

    ret = parse_dxbc(dxbc, &message_context, NULL, desc);

    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages) && ret >= 0)
    {
        vkd3d_shader_free_dxbc(desc);
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    }
    vkd3d_shader_message_context_cleanup(&message_context);

    if (ret < 0)
        memset(desc, 0, sizeof(*desc));

    return ret;
}

static int shader_parse_signature(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, struct vkd3d_shader_signature *s)
{
    bool has_stream_index, has_min_precision;
    struct vkd3d_shader_signature_element *e;
    const char *data = section->data.code;
    uint32_t count, header_size;
    const char *ptr = data;
    unsigned int i;

    if (!require_space(0, 2, sizeof(uint32_t), section->data.size))
    {
        WARN("Invalid data size %#zx.\n", section->data.size);
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_DXBC_INVALID_SIGNATURE,
                "Section size %zu is smaller than the minimum signature header size.\n", section->data.size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    read_dword(&ptr, &count);
    TRACE("%u elements.\n", count);

    read_dword(&ptr, &header_size);
    i = header_size / sizeof(uint32_t);
    if (align(header_size, sizeof(uint32_t)) != header_size || i < 2
            || !require_space(2, i - 2, sizeof(uint32_t), section->data.size))
    {
        WARN("Invalid header size %#x.\n", header_size);
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_DXBC_INVALID_SIGNATURE,
                "Signature header size %#x is invalid.\n", header_size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    skip_dword_unknown(&ptr, i - 2);

    if (!require_space(ptr - data, count, 6 * sizeof(uint32_t), section->data.size))
    {
        WARN("Invalid count %#x (data size %#zx).\n", count, section->data.size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (!(e = vkd3d_calloc(count, sizeof(*e))))
    {
        ERR("Failed to allocate input signature memory.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    has_min_precision = section->tag == TAG_OSG1 || section->tag == TAG_PSG1 || section->tag == TAG_ISG1;
    has_stream_index = section->tag == TAG_OSG5 || has_min_precision;

    for (i = 0; i < count; ++i)
    {
        uint32_t name_offset, mask;

        if (has_stream_index)
            read_dword(&ptr, &e[i].stream_index);
        else
            e[i].stream_index = 0;

        read_dword(&ptr, &name_offset);
        if (!(e[i].semantic_name = shader_get_string(data, section->data.size, name_offset)))
        {
            WARN("Invalid name offset %#x (data size %#zx).\n", name_offset, section->data.size);
            vkd3d_free(e);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
        read_dword(&ptr, &e[i].semantic_index);
        read_dword(&ptr, &e[i].sysval_semantic);
        read_dword(&ptr, &e[i].component_type);
        read_dword(&ptr, &e[i].register_index);
        read_dword(&ptr, &mask);
        e[i].mask = mask & 0xff;
        e[i].used_mask = (mask >> 8) & 0xff;
        switch (section->tag)
        {
            case TAG_OSGN:
            case TAG_OSG1:
            case TAG_OSG5:
            case TAG_PCSG:
            case TAG_PSG1:
                e[i].used_mask = e[i].mask & ~e[i].used_mask;
                break;
        }

        if (has_min_precision)
            read_dword(&ptr, &e[i].min_precision);
        else
            e[i].min_precision = VKD3D_SHADER_MINIMUM_PRECISION_NONE;

        TRACE("Stream: %u, semantic: %s, semantic idx: %u, sysval_semantic %#x, "
                "type %u, register idx: %u, use_mask %#x, input_mask %#x, precision %u.\n",
                e[i].stream_index, debugstr_a(e[i].semantic_name), e[i].semantic_index, e[i].sysval_semantic,
                e[i].component_type, e[i].register_index, e[i].used_mask, e[i].mask, e[i].min_precision);
    }

    s->elements = e;
    s->element_count = count;

    return VKD3D_OK;
}

static int isgn_handler(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, void *ctx)
{
    struct vkd3d_shader_signature *is = ctx;

    if (section->tag != TAG_ISGN)
        return VKD3D_OK;

    if (is->elements)
    {
        FIXME("Multiple input signatures.\n");
        vkd3d_shader_free_shader_signature(is);
    }
    return shader_parse_signature(section, message_context, is);
}

int shader_parse_input_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_message_context *message_context, struct vkd3d_shader_signature *signature)
{
    int ret;

    memset(signature, 0, sizeof(*signature));
    if ((ret = for_each_dxbc_section(dxbc, message_context, NULL, isgn_handler, signature)) < 0)
        ERR("Failed to parse input signature.\n");

    return ret;
}

static int shdr_handler(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, void *context)
{
    struct vkd3d_shader_desc *desc = context;
    int ret;

    switch (section->tag)
    {
        case TAG_ISGN:
        case TAG_ISG1:
            if (desc->input_signature.elements)
            {
                FIXME("Multiple input signatures.\n");
                break;
            }
            if ((ret = shader_parse_signature(section, message_context, &desc->input_signature)) < 0)
                return ret;
            break;

        case TAG_OSGN:
        case TAG_OSG5:
        case TAG_OSG1:
            if (desc->output_signature.elements)
            {
                FIXME("Multiple output signatures.\n");
                break;
            }
            if ((ret = shader_parse_signature(section, message_context, &desc->output_signature)) < 0)
                return ret;
            break;

        case TAG_PCSG:
        case TAG_PSG1:
            if (desc->patch_constant_signature.elements)
            {
                FIXME("Multiple patch constant signatures.\n");
                break;
            }
            if ((ret = shader_parse_signature(section, message_context, &desc->patch_constant_signature)) < 0)
                return ret;
            break;

        case TAG_SHDR:
        case TAG_SHEX:
            if (desc->byte_code)
                FIXME("Multiple shader code chunks.\n");
            desc->byte_code = section->data.code;
            desc->byte_code_size = section->data.size;
            break;

        case TAG_AON9:
            TRACE("Skipping AON9 shader code chunk.\n");
            break;

        case TAG_DXIL:
            FIXME("Skipping DXIL shader model 6+ code chunk.\n");
            break;

        default:
            TRACE("Skipping chunk %#x.\n", section->tag);
            break;
    }

    return VKD3D_OK;
}

void free_shader_desc(struct vkd3d_shader_desc *desc)
{
    vkd3d_shader_free_shader_signature(&desc->input_signature);
    vkd3d_shader_free_shader_signature(&desc->output_signature);
    vkd3d_shader_free_shader_signature(&desc->patch_constant_signature);
}

static int shader_extract_from_dxbc(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_message_context *message_context, const char *source_name, struct vkd3d_shader_desc *desc)
{
    int ret;

    desc->byte_code = NULL;
    desc->byte_code_size = 0;
    memset(&desc->input_signature, 0, sizeof(desc->input_signature));
    memset(&desc->output_signature, 0, sizeof(desc->output_signature));
    memset(&desc->patch_constant_signature, 0, sizeof(desc->patch_constant_signature));

    ret = for_each_dxbc_section(dxbc, message_context, source_name, shdr_handler, desc);
    if (!desc->byte_code)
        ret = VKD3D_ERROR_INVALID_ARGUMENT;

    if (ret < 0)
    {
        WARN("Failed to parse shader, vkd3d result %d.\n", ret);
        free_shader_desc(desc);
    }

    return ret;
}

int vkd3d_shader_sm4_parser_create(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context, struct vkd3d_shader_parser **parser)
{
    struct vkd3d_shader_instruction_array *instructions;
    struct vkd3d_shader_desc *shader_desc;
    struct vkd3d_shader_instruction *ins;
    struct vkd3d_shader_sm4_parser *sm4;
    int ret;

    if (!(sm4 = vkd3d_calloc(1, sizeof(*sm4))))
    {
        ERR("Failed to allocate parser.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    shader_desc = &sm4->p.shader_desc;
    if ((ret = shader_extract_from_dxbc(&compile_info->source,
            message_context, compile_info->source_name, shader_desc)) < 0)
    {
        WARN("Failed to extract shader, vkd3d result %d.\n", ret);
        vkd3d_free(sm4);
        return ret;
    }

    if (!shader_sm4_init(sm4, shader_desc->byte_code, shader_desc->byte_code_size,
            compile_info->source_name, &shader_desc->output_signature, message_context))
    {
        WARN("Failed to initialise shader parser.\n");
        free_shader_desc(shader_desc);
        vkd3d_free(sm4);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    instructions = &sm4->p.instructions;
    while (!shader_sm4_is_end(&sm4->p))
    {
        if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
        {
            ERR("Failed to allocate instructions.\n");
            vkd3d_shader_parser_error(&sm4->p, VKD3D_SHADER_ERROR_TPF_OUT_OF_MEMORY, "Out of memory.");
            shader_sm4_destroy(&sm4->p);
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }
        ins = &instructions->elements[instructions->count];
        shader_sm4_read_instruction(&sm4->p, ins);

        if (ins->handler_idx == VKD3DSIH_INVALID)
        {
            WARN("Encountered unrecognized or invalid instruction.\n");
            shader_sm4_destroy(&sm4->p);
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }
        ++instructions->count;
    }

    *parser = &sm4->p;

    return VKD3D_OK;
}

/* root signatures */
#define VKD3D_ROOT_SIGNATURE_1_0_ROOT_DESCRIPTOR_FLAGS VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE

#define VKD3D_ROOT_SIGNATURE_1_0_DESCRIPTOR_RANGE_FLAGS \
        (VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE)

struct root_signature_parser_context
{
    const char *data;
    unsigned int data_size;
};

static int shader_parse_descriptor_ranges(struct root_signature_parser_context *context,
        unsigned int offset, unsigned int count, struct vkd3d_shader_descriptor_range *ranges)
{
    const char *ptr;
    unsigned int i;

    if (!require_space(offset, 5 * count, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        read_dword(&ptr, &ranges[i].range_type);
        read_dword(&ptr, &ranges[i].descriptor_count);
        read_dword(&ptr, &ranges[i].base_shader_register);
        read_dword(&ptr, &ranges[i].register_space);
        read_dword(&ptr, &ranges[i].descriptor_table_offset);

        TRACE("Type %#x, descriptor count %u, base shader register %u, "
                "register space %u, offset %u.\n",
                ranges[i].range_type, ranges[i].descriptor_count,
                ranges[i].base_shader_register, ranges[i].register_space,
                ranges[i].descriptor_table_offset);
    }

    return VKD3D_OK;
}

static void shader_validate_descriptor_range1(const struct vkd3d_shader_descriptor_range1 *range)
{
    unsigned int unknown_flags = range->flags & ~(VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_NONE
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    if (unknown_flags)
        FIXME("Unknown descriptor range flags %#x.\n", unknown_flags);
}

static int shader_parse_descriptor_ranges1(struct root_signature_parser_context *context,
        unsigned int offset, unsigned int count, struct vkd3d_shader_descriptor_range1 *ranges)
{
    const char *ptr;
    unsigned int i;

    if (!require_space(offset, 6 * count, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        read_dword(&ptr, &ranges[i].range_type);
        read_dword(&ptr, &ranges[i].descriptor_count);
        read_dword(&ptr, &ranges[i].base_shader_register);
        read_dword(&ptr, &ranges[i].register_space);
        read_dword(&ptr, &ranges[i].flags);
        read_dword(&ptr, &ranges[i].descriptor_table_offset);

        TRACE("Type %#x, descriptor count %u, base shader register %u, "
                "register space %u, flags %#x, offset %u.\n",
                ranges[i].range_type, ranges[i].descriptor_count,
                ranges[i].base_shader_register, ranges[i].register_space,
                ranges[i].flags, ranges[i].descriptor_table_offset);

        shader_validate_descriptor_range1(&ranges[i]);
    }

    return VKD3D_OK;
}

static int shader_parse_descriptor_table(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_descriptor_table *table)
{
    struct vkd3d_shader_descriptor_range *ranges;
    unsigned int count;
    const char *ptr;

    if (!require_space(offset, 2, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    read_dword(&ptr, &count);
    read_dword(&ptr, &offset);

    TRACE("Descriptor range count %u.\n", count);

    table->descriptor_range_count = count;

    if (!(ranges = vkd3d_calloc(count, sizeof(*ranges))))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    table->descriptor_ranges = ranges;
    return shader_parse_descriptor_ranges(context, offset, count, ranges);
}

static int shader_parse_descriptor_table1(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_descriptor_table1 *table)
{
    struct vkd3d_shader_descriptor_range1 *ranges;
    unsigned int count;
    const char *ptr;

    if (!require_space(offset, 2, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    read_dword(&ptr, &count);
    read_dword(&ptr, &offset);

    TRACE("Descriptor range count %u.\n", count);

    table->descriptor_range_count = count;

    if (!(ranges = vkd3d_calloc(count, sizeof(*ranges))))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    table->descriptor_ranges = ranges;
    return shader_parse_descriptor_ranges1(context, offset, count, ranges);
}

static int shader_parse_root_constants(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_constants *constants)
{
    const char *ptr;

    if (!require_space(offset, 3, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    read_dword(&ptr, &constants->shader_register);
    read_dword(&ptr, &constants->register_space);
    read_dword(&ptr, &constants->value_count);

    TRACE("Shader register %u, register space %u, 32-bit value count %u.\n",
            constants->shader_register, constants->register_space, constants->value_count);

    return VKD3D_OK;
}

static int shader_parse_root_descriptor(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_descriptor *descriptor)
{
    const char *ptr;

    if (!require_space(offset, 2, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    read_dword(&ptr, &descriptor->shader_register);
    read_dword(&ptr, &descriptor->register_space);

    TRACE("Shader register %u, register space %u.\n",
            descriptor->shader_register, descriptor->register_space);

    return VKD3D_OK;
}

static void shader_validate_root_descriptor1(const struct vkd3d_shader_root_descriptor1 *descriptor)
{
    unsigned int unknown_flags = descriptor->flags & ~(VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_NONE
            | VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE
            | VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
            | VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);

    if (unknown_flags)
        FIXME("Unknown root descriptor flags %#x.\n", unknown_flags);
}

static int shader_parse_root_descriptor1(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_descriptor1 *descriptor)
{
    const char *ptr;

    if (!require_space(offset, 3, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    read_dword(&ptr, &descriptor->shader_register);
    read_dword(&ptr, &descriptor->register_space);
    read_dword(&ptr, &descriptor->flags);

    TRACE("Shader register %u, register space %u, flags %#x.\n",
            descriptor->shader_register, descriptor->register_space, descriptor->flags);

    shader_validate_root_descriptor1(descriptor);

    return VKD3D_OK;
}

static int shader_parse_root_parameters(struct root_signature_parser_context *context,
        unsigned int offset, unsigned int count, struct vkd3d_shader_root_parameter *parameters)
{
    const char *ptr;
    unsigned int i;
    int ret;

    if (!require_space(offset, 3 * count, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        read_dword(&ptr, &parameters[i].parameter_type);
        read_dword(&ptr, &parameters[i].shader_visibility);
        read_dword(&ptr, &offset);

        TRACE("Type %#x, shader visibility %#x.\n",
                parameters[i].parameter_type, parameters[i].shader_visibility);

        switch (parameters[i].parameter_type)
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                ret = shader_parse_descriptor_table(context, offset, &parameters[i].u.descriptor_table);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                ret = shader_parse_root_constants(context, offset, &parameters[i].u.constants);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                ret = shader_parse_root_descriptor(context, offset, &parameters[i].u.descriptor);
                break;
            default:
                FIXME("Unrecognized type %#x.\n", parameters[i].parameter_type);
                return VKD3D_ERROR_INVALID_ARGUMENT;
        }

        if (ret < 0)
            return ret;
    }

    return VKD3D_OK;
}

static int shader_parse_root_parameters1(struct root_signature_parser_context *context,
        uint32_t offset, DWORD count, struct vkd3d_shader_root_parameter1 *parameters)
{
    const char *ptr;
    unsigned int i;
    int ret;

    if (!require_space(offset, 3 * count, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        read_dword(&ptr, &parameters[i].parameter_type);
        read_dword(&ptr, &parameters[i].shader_visibility);
        read_dword(&ptr, &offset);

        TRACE("Type %#x, shader visibility %#x.\n",
                parameters[i].parameter_type, parameters[i].shader_visibility);

        switch (parameters[i].parameter_type)
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                ret = shader_parse_descriptor_table1(context, offset, &parameters[i].u.descriptor_table);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                ret = shader_parse_root_constants(context, offset, &parameters[i].u.constants);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                ret = shader_parse_root_descriptor1(context, offset, &parameters[i].u.descriptor);
                break;
            default:
                FIXME("Unrecognized type %#x.\n", parameters[i].parameter_type);
                return VKD3D_ERROR_INVALID_ARGUMENT;
        }

        if (ret < 0)
            return ret;
    }

    return VKD3D_OK;
}

static int shader_parse_static_samplers(struct root_signature_parser_context *context,
        unsigned int offset, unsigned int count, struct vkd3d_shader_static_sampler_desc *sampler_descs)
{
    const char *ptr;
    unsigned int i;

    if (!require_space(offset, 13 * count, sizeof(DWORD), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        read_dword(&ptr, &sampler_descs[i].filter);
        read_dword(&ptr, &sampler_descs[i].address_u);
        read_dword(&ptr, &sampler_descs[i].address_v);
        read_dword(&ptr, &sampler_descs[i].address_w);
        read_float(&ptr, &sampler_descs[i].mip_lod_bias);
        read_dword(&ptr, &sampler_descs[i].max_anisotropy);
        read_dword(&ptr, &sampler_descs[i].comparison_func);
        read_dword(&ptr, &sampler_descs[i].border_colour);
        read_float(&ptr, &sampler_descs[i].min_lod);
        read_float(&ptr, &sampler_descs[i].max_lod);
        read_dword(&ptr, &sampler_descs[i].shader_register);
        read_dword(&ptr, &sampler_descs[i].register_space);
        read_dword(&ptr, &sampler_descs[i].shader_visibility);
    }

    return VKD3D_OK;
}

static int shader_parse_root_signature(const struct vkd3d_shader_code *data,
        struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    struct vkd3d_shader_root_signature_desc *v_1_0 = &desc->u.v_1_0;
    struct root_signature_parser_context context;
    unsigned int count, offset, version;
    const char *ptr = data->code;
    int ret;

    context.data = data->code;
    context.data_size = data->size;

    if (!require_space(0, 6, sizeof(uint32_t), data->size))
    {
        WARN("Invalid data size %#zx.\n", data->size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    read_dword(&ptr, &version);
    TRACE("Version %#x.\n", version);
    if (version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0 && version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        FIXME("Unknown version %#x.\n", version);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    desc->version = version;

    read_dword(&ptr, &count);
    read_dword(&ptr, &offset);
    TRACE("Parameter count %u, offset %u.\n", count, offset);

    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
    {
        v_1_0->parameter_count = count;
        if (v_1_0->parameter_count)
        {
            struct vkd3d_shader_root_parameter *parameters;
            if (!(parameters = vkd3d_calloc(v_1_0->parameter_count, sizeof(*parameters))))
                return VKD3D_ERROR_OUT_OF_MEMORY;
            v_1_0->parameters = parameters;
            if ((ret = shader_parse_root_parameters(&context, offset, count, parameters)) < 0)
                return ret;
        }
    }
    else
    {
        struct vkd3d_shader_root_signature_desc1 *v_1_1 = &desc->u.v_1_1;

        assert(version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1);

        v_1_1->parameter_count = count;
        if (v_1_1->parameter_count)
        {
            struct vkd3d_shader_root_parameter1 *parameters;
            if (!(parameters = vkd3d_calloc(v_1_1->parameter_count, sizeof(*parameters))))
                return VKD3D_ERROR_OUT_OF_MEMORY;
            v_1_1->parameters = parameters;
            if ((ret = shader_parse_root_parameters1(&context, offset, count, parameters)) < 0)
                return ret;
        }
    }

    read_dword(&ptr, &count);
    read_dword(&ptr, &offset);
    TRACE("Static sampler count %u, offset %u.\n", count, offset);

    v_1_0->static_sampler_count = count;
    if (v_1_0->static_sampler_count)
    {
        struct vkd3d_shader_static_sampler_desc *samplers;
        if (!(samplers = vkd3d_calloc(v_1_0->static_sampler_count, sizeof(*samplers))))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        v_1_0->static_samplers = samplers;
        if ((ret = shader_parse_static_samplers(&context, offset, count, samplers)) < 0)
            return ret;
    }

    read_dword(&ptr, &v_1_0->flags);
    TRACE("Flags %#x.\n", v_1_0->flags);

    return VKD3D_OK;
}

static int rts0_handler(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, void *context)
{
    struct vkd3d_shader_versioned_root_signature_desc *desc = context;

    if (section->tag != TAG_RTS0)
        return VKD3D_OK;

    return shader_parse_root_signature(&section->data, desc);
}

int vkd3d_shader_parse_root_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_versioned_root_signature_desc *root_signature, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("dxbc {%p, %zu}, root_signature %p, messages %p.\n", dxbc->code, dxbc->size, root_signature, messages);

    memset(root_signature, 0, sizeof(*root_signature));
    if (messages)
        *messages = NULL;
    vkd3d_shader_message_context_init(&message_context, VKD3D_SHADER_LOG_INFO);

    ret = for_each_dxbc_section(dxbc, &message_context, NULL, rts0_handler, root_signature);
    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;

    vkd3d_shader_message_context_cleanup(&message_context);
    if (ret < 0)
        vkd3d_shader_free_root_signature(root_signature);

    return ret;
}

static unsigned int versioned_root_signature_get_parameter_count(
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.parameter_count;
    else
        return desc->u.v_1_1.parameter_count;
}

static enum vkd3d_shader_root_parameter_type versioned_root_signature_get_parameter_type(
        const struct vkd3d_shader_versioned_root_signature_desc *desc, unsigned int i)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.parameters[i].parameter_type;
    else
        return desc->u.v_1_1.parameters[i].parameter_type;
}

static enum vkd3d_shader_visibility versioned_root_signature_get_parameter_shader_visibility(
        const struct vkd3d_shader_versioned_root_signature_desc *desc, unsigned int i)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.parameters[i].shader_visibility;
    else
        return desc->u.v_1_1.parameters[i].shader_visibility;
}

static const struct vkd3d_shader_root_constants *versioned_root_signature_get_root_constants(
        const struct vkd3d_shader_versioned_root_signature_desc *desc, unsigned int i)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return &desc->u.v_1_0.parameters[i].u.constants;
    else
        return &desc->u.v_1_1.parameters[i].u.constants;
}

static unsigned int versioned_root_signature_get_static_sampler_count(
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.static_sampler_count;
    else
        return desc->u.v_1_1.static_sampler_count;
}

static const struct vkd3d_shader_static_sampler_desc *versioned_root_signature_get_static_samplers(
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.static_samplers;
    else
        return desc->u.v_1_1.static_samplers;
}

static unsigned int versioned_root_signature_get_flags(const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.flags;
    else
        return desc->u.v_1_1.flags;
}

struct root_signature_writer_context
{
    struct vkd3d_shader_message_context message_context;

    struct vkd3d_bytecode_buffer buffer;

    size_t total_size_position;
    size_t chunk_position;
};

static size_t get_chunk_offset(struct root_signature_writer_context *context)
{
    return bytecode_get_size(&context->buffer) - context->chunk_position;
}

static void shader_write_root_signature_header(struct root_signature_writer_context *context)
{
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;
    unsigned int i;

    put_u32(buffer, TAG_DXBC);

    /* The checksum is computed when all data is generated. */
    for (i = 0; i < 4; ++i)
        put_u32(buffer, 0);
    put_u32(buffer, 1);
    context->total_size_position = put_u32(buffer, 0xffffffff);
    put_u32(buffer, 1); /* chunk count */
    put_u32(buffer, bytecode_get_size(buffer) + sizeof(uint32_t)); /* chunk offset */
    put_u32(buffer, TAG_RTS0);
    put_u32(buffer, 0xffffffff);
    context->chunk_position = bytecode_get_size(buffer);
}

static void shader_write_descriptor_ranges(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_descriptor_table *table)
{
    const struct vkd3d_shader_descriptor_range *ranges = table->descriptor_ranges;
    unsigned int i;

    for (i = 0; i < table->descriptor_range_count; ++i)
    {
        put_u32(buffer, ranges[i].range_type);
        put_u32(buffer, ranges[i].descriptor_count);
        put_u32(buffer, ranges[i].base_shader_register);
        put_u32(buffer, ranges[i].register_space);
        put_u32(buffer, ranges[i].descriptor_table_offset);
    }
}

static void shader_write_descriptor_ranges1(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_descriptor_table1 *table)
{
    const struct vkd3d_shader_descriptor_range1 *ranges = table->descriptor_ranges;
    unsigned int i;

    for (i = 0; i < table->descriptor_range_count; ++i)
    {
        put_u32(buffer, ranges[i].range_type);
        put_u32(buffer, ranges[i].descriptor_count);
        put_u32(buffer, ranges[i].base_shader_register);
        put_u32(buffer, ranges[i].register_space);
        put_u32(buffer, ranges[i].flags);
        put_u32(buffer, ranges[i].descriptor_table_offset);
    }
}

static void shader_write_descriptor_table(struct root_signature_writer_context *context,
        const struct vkd3d_shader_root_descriptor_table *table)
{
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;

    put_u32(buffer, table->descriptor_range_count);
    put_u32(buffer, get_chunk_offset(context) + sizeof(uint32_t)); /* offset */

    shader_write_descriptor_ranges(buffer, table);
}

static void shader_write_descriptor_table1(struct root_signature_writer_context *context,
        const struct vkd3d_shader_root_descriptor_table1 *table)
{
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;

    put_u32(buffer, table->descriptor_range_count);
    put_u32(buffer, get_chunk_offset(context) + sizeof(uint32_t)); /* offset */

    shader_write_descriptor_ranges1(buffer, table);
}

static void shader_write_root_constants(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_constants *constants)
{
    put_u32(buffer, constants->shader_register);
    put_u32(buffer, constants->register_space);
    put_u32(buffer, constants->value_count);
}

static void shader_write_root_descriptor(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_descriptor *descriptor)
{
    put_u32(buffer, descriptor->shader_register);
    put_u32(buffer, descriptor->register_space);
}

static void shader_write_root_descriptor1(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_descriptor1 *descriptor)
{
    put_u32(buffer, descriptor->shader_register);
    put_u32(buffer, descriptor->register_space);
    put_u32(buffer, descriptor->flags);
}

static int shader_write_root_parameters(struct root_signature_writer_context *context,
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    unsigned int parameter_count = versioned_root_signature_get_parameter_count(desc);
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;
    size_t parameters_position;
    unsigned int i;

    parameters_position = bytecode_get_size(buffer);
    for (i = 0; i < parameter_count; ++i)
    {
        put_u32(buffer, versioned_root_signature_get_parameter_type(desc, i));
        put_u32(buffer, versioned_root_signature_get_parameter_shader_visibility(desc, i));
        put_u32(buffer, 0xffffffff); /* offset */
    }

    for (i = 0; i < parameter_count; ++i)
    {
        set_u32(buffer, parameters_position + ((3 * i + 2) * sizeof(uint32_t)), get_chunk_offset(context));

        switch (versioned_root_signature_get_parameter_type(desc, i))
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
                    shader_write_descriptor_table(context, &desc->u.v_1_0.parameters[i].u.descriptor_table);
                else
                    shader_write_descriptor_table1(context, &desc->u.v_1_1.parameters[i].u.descriptor_table);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                shader_write_root_constants(buffer, versioned_root_signature_get_root_constants(desc, i));
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
                    shader_write_root_descriptor(buffer, &desc->u.v_1_0.parameters[i].u.descriptor);
                else
                    shader_write_root_descriptor1(buffer, &desc->u.v_1_1.parameters[i].u.descriptor);
                break;
            default:
                FIXME("Unrecognized type %#x.\n", versioned_root_signature_get_parameter_type(desc, i));
                vkd3d_shader_error(&context->message_context, NULL, VKD3D_SHADER_ERROR_RS_INVALID_ROOT_PARAMETER_TYPE,
                        "Invalid/unrecognised root signature root parameter type %#x.",
                        versioned_root_signature_get_parameter_type(desc, i));
                return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    return VKD3D_OK;
}

static void shader_write_static_samplers(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    const struct vkd3d_shader_static_sampler_desc *samplers = versioned_root_signature_get_static_samplers(desc);
    unsigned int i;

    for (i = 0; i < versioned_root_signature_get_static_sampler_count(desc); ++i)
    {
        put_u32(buffer, samplers[i].filter);
        put_u32(buffer, samplers[i].address_u);
        put_u32(buffer, samplers[i].address_v);
        put_u32(buffer, samplers[i].address_w);
        put_f32(buffer, samplers[i].mip_lod_bias);
        put_u32(buffer, samplers[i].max_anisotropy);
        put_u32(buffer, samplers[i].comparison_func);
        put_u32(buffer, samplers[i].border_colour);
        put_f32(buffer, samplers[i].min_lod);
        put_f32(buffer, samplers[i].max_lod);
        put_u32(buffer, samplers[i].shader_register);
        put_u32(buffer, samplers[i].register_space);
        put_u32(buffer, samplers[i].shader_visibility);
    }
}

static int shader_write_root_signature(struct root_signature_writer_context *context,
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;
    size_t samplers_offset_position;
    int ret;

    put_u32(buffer, desc->version);
    put_u32(buffer, versioned_root_signature_get_parameter_count(desc));
    put_u32(buffer, get_chunk_offset(context) + 4 * sizeof(uint32_t)); /* offset */
    put_u32(buffer, versioned_root_signature_get_static_sampler_count(desc));
    samplers_offset_position = put_u32(buffer, 0xffffffff);
    put_u32(buffer, versioned_root_signature_get_flags(desc));

    if ((ret = shader_write_root_parameters(context, desc)) < 0)
        return ret;

    set_u32(buffer, samplers_offset_position, get_chunk_offset(context));
    shader_write_static_samplers(buffer, desc);
    return 0;
}

static int validate_descriptor_table_v_1_0(const struct vkd3d_shader_root_descriptor_table *descriptor_table,
        struct vkd3d_shader_message_context *message_context)
{
    bool have_srv_uav_cbv = false;
    bool have_sampler = false;
    unsigned int i;

    for (i = 0; i < descriptor_table->descriptor_range_count; ++i)
    {
        const struct vkd3d_shader_descriptor_range *r = &descriptor_table->descriptor_ranges[i];

        if (r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_SRV
                || r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV
                || r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_CBV)
        {
            have_srv_uav_cbv = true;
        }
        else if (r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER)
        {
            have_sampler = true;
        }
        else
        {
            WARN("Invalid descriptor range type %#x.\n", r->range_type);
            vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_RS_INVALID_DESCRIPTOR_RANGE_TYPE,
                    "Invalid root signature descriptor range type %#x.", r->range_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    if (have_srv_uav_cbv && have_sampler)
    {
        WARN("Samplers cannot be mixed with CBVs/SRVs/UAVs in descriptor tables.\n");
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_RS_MIXED_DESCRIPTOR_RANGE_TYPES,
                "Encountered both CBV/SRV/UAV and sampler descriptor ranges in the same root descriptor table.");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    return VKD3D_OK;
}

static int validate_descriptor_table_v_1_1(const struct vkd3d_shader_root_descriptor_table1 *descriptor_table,
        struct vkd3d_shader_message_context *message_context)
{
    bool have_srv_uav_cbv = false;
    bool have_sampler = false;
    unsigned int i;

    for (i = 0; i < descriptor_table->descriptor_range_count; ++i)
    {
        const struct vkd3d_shader_descriptor_range1 *r = &descriptor_table->descriptor_ranges[i];

        if (r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_SRV
                || r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV
                || r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_CBV)
        {
            have_srv_uav_cbv = true;
        }
        else if (r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER)
        {
            have_sampler = true;
        }
        else
        {
            WARN("Invalid descriptor range type %#x.\n", r->range_type);
            vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_RS_INVALID_DESCRIPTOR_RANGE_TYPE,
                    "Invalid root signature descriptor range type %#x.", r->range_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    if (have_srv_uav_cbv && have_sampler)
    {
        WARN("Samplers cannot be mixed with CBVs/SRVs/UAVs in descriptor tables.\n");
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_RS_MIXED_DESCRIPTOR_RANGE_TYPES,
                "Encountered both CBV/SRV/UAV and sampler descriptor ranges in the same root descriptor table.");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    return VKD3D_OK;
}

static int validate_root_signature_desc(const struct vkd3d_shader_versioned_root_signature_desc *desc,
        struct vkd3d_shader_message_context *message_context)
{
    int ret = VKD3D_OK;
    unsigned int i;

    for (i = 0; i < versioned_root_signature_get_parameter_count(desc); ++i)
    {
        enum vkd3d_shader_root_parameter_type type;

        type = versioned_root_signature_get_parameter_type(desc, i);
        if (type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
                ret = validate_descriptor_table_v_1_0(&desc->u.v_1_0.parameters[i].u.descriptor_table, message_context);
            else
                ret = validate_descriptor_table_v_1_1(&desc->u.v_1_1.parameters[i].u.descriptor_table, message_context);
        }

        if (ret < 0)
            break;
    }

    return ret;
}

int vkd3d_shader_serialize_root_signature(const struct vkd3d_shader_versioned_root_signature_desc *root_signature,
        struct vkd3d_shader_code *dxbc, char **messages)
{
    struct root_signature_writer_context context;
    size_t total_size, chunk_size;
    uint32_t checksum[4];
    unsigned int i;
    int ret;

    TRACE("root_signature %p, dxbc %p, messages %p.\n", root_signature, dxbc, messages);

    if (messages)
        *messages = NULL;

    memset(&context, 0, sizeof(context));
    vkd3d_shader_message_context_init(&context.message_context, VKD3D_SHADER_LOG_INFO);

    if (root_signature->version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0
            && root_signature->version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        ret = VKD3D_ERROR_INVALID_ARGUMENT;
        WARN("Root signature version %#x not supported.\n", root_signature->version);
        vkd3d_shader_error(&context.message_context, NULL, VKD3D_SHADER_ERROR_RS_INVALID_VERSION,
                "Root signature version %#x is not supported.", root_signature->version);
        goto done;
    }

    if ((ret = validate_root_signature_desc(root_signature, &context.message_context)) < 0)
        goto done;

    memset(dxbc, 0, sizeof(*dxbc));
    shader_write_root_signature_header(&context);

    if ((ret = shader_write_root_signature(&context, root_signature)) < 0)
    {
        vkd3d_free(context.buffer.data);
        goto done;
    }

    if (context.buffer.status)
    {
        vkd3d_shader_error(&context.message_context, NULL, VKD3D_SHADER_ERROR_RS_OUT_OF_MEMORY,
                "Out of memory while writing root signature.");
        vkd3d_free(context.buffer.data);
        goto done;
    }

    total_size = bytecode_get_size(&context.buffer);
    chunk_size = get_chunk_offset(&context);
    set_u32(&context.buffer, context.total_size_position, total_size);
    set_u32(&context.buffer, context.chunk_position - sizeof(uint32_t), chunk_size);

    dxbc->code = context.buffer.data;
    dxbc->size = total_size;

    vkd3d_compute_dxbc_checksum(dxbc->code, dxbc->size, checksum);
    for (i = 0; i < 4; ++i)
        set_u32(&context.buffer, (i + 1) * sizeof(uint32_t), checksum[i]);

    ret = VKD3D_OK;

done:
    vkd3d_shader_message_context_trace_messages(&context.message_context);
    if (!vkd3d_shader_message_context_copy_messages(&context.message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    vkd3d_shader_message_context_cleanup(&context.message_context);
    return ret;
}

static void free_descriptor_ranges(const struct vkd3d_shader_root_parameter *parameters, unsigned int count)
{
    unsigned int i;

    if (!parameters)
        return;

    for (i = 0; i < count; ++i)
    {
        const struct vkd3d_shader_root_parameter *p = &parameters[i];

        if (p->parameter_type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            vkd3d_free((void *)p->u.descriptor_table.descriptor_ranges);
    }
}

static int convert_root_parameters_to_v_1_0(struct vkd3d_shader_root_parameter *dst,
        const struct vkd3d_shader_root_parameter1 *src, unsigned int count)
{
    const struct vkd3d_shader_descriptor_range1 *ranges1;
    struct vkd3d_shader_descriptor_range *ranges;
    unsigned int i, j;
    int ret;

    for (i = 0; i < count; ++i)
    {
        const struct vkd3d_shader_root_parameter1 *p1 = &src[i];
        struct vkd3d_shader_root_parameter *p = &dst[i];

        p->parameter_type = p1->parameter_type;
        switch (p->parameter_type)
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                ranges = NULL;
                if ((p->u.descriptor_table.descriptor_range_count = p1->u.descriptor_table.descriptor_range_count))
                {
                    if (!(ranges = vkd3d_calloc(p->u.descriptor_table.descriptor_range_count, sizeof(*ranges))))
                    {
                        ret = VKD3D_ERROR_OUT_OF_MEMORY;
                        goto fail;
                    }
                }
                p->u.descriptor_table.descriptor_ranges = ranges;
                ranges1 = p1->u.descriptor_table.descriptor_ranges;
                for (j = 0; j < p->u.descriptor_table.descriptor_range_count; ++j)
                {
                    ranges[j].range_type = ranges1[j].range_type;
                    ranges[j].descriptor_count = ranges1[j].descriptor_count;
                    ranges[j].base_shader_register = ranges1[j].base_shader_register;
                    ranges[j].register_space = ranges1[j].register_space;
                    ranges[j].descriptor_table_offset = ranges1[j].descriptor_table_offset;
                }
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                p->u.constants = p1->u.constants;
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                p->u.descriptor.shader_register = p1->u.descriptor.shader_register;
                p->u.descriptor.register_space = p1->u.descriptor.register_space;
                break;
            default:
                WARN("Invalid root parameter type %#x.\n", p->parameter_type);
                ret = VKD3D_ERROR_INVALID_ARGUMENT;
                goto fail;

        }
        p->shader_visibility = p1->shader_visibility;
    }

    return VKD3D_OK;

fail:
    free_descriptor_ranges(dst, i);
    return ret;
}

static int convert_root_signature_to_v1_0(struct vkd3d_shader_versioned_root_signature_desc *dst,
        const struct vkd3d_shader_versioned_root_signature_desc *src)
{
    const struct vkd3d_shader_root_signature_desc1 *src_desc = &src->u.v_1_1;
    struct vkd3d_shader_root_signature_desc *dst_desc = &dst->u.v_1_0;
    struct vkd3d_shader_static_sampler_desc *samplers = NULL;
    struct vkd3d_shader_root_parameter *parameters = NULL;
    int ret;

    if ((dst_desc->parameter_count = src_desc->parameter_count))
    {
        if (!(parameters = vkd3d_calloc(dst_desc->parameter_count, sizeof(*parameters))))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        if ((ret = convert_root_parameters_to_v_1_0(parameters, src_desc->parameters, src_desc->parameter_count)) < 0)
            goto fail;
    }
    dst_desc->parameters = parameters;
    if ((dst_desc->static_sampler_count = src_desc->static_sampler_count))
    {
        if (!(samplers = vkd3d_calloc(dst_desc->static_sampler_count, sizeof(*samplers))))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        memcpy(samplers, src_desc->static_samplers, src_desc->static_sampler_count * sizeof(*samplers));
    }
    dst_desc->static_samplers = samplers;
    dst_desc->flags = src_desc->flags;

    return VKD3D_OK;

fail:
    free_descriptor_ranges(parameters, dst_desc->parameter_count);
    vkd3d_free(parameters);
    vkd3d_free(samplers);
    return ret;
}

static void free_descriptor_ranges1(const struct vkd3d_shader_root_parameter1 *parameters, unsigned int count)
{
    unsigned int i;

    if (!parameters)
        return;

    for (i = 0; i < count; ++i)
    {
        const struct vkd3d_shader_root_parameter1 *p = &parameters[i];

        if (p->parameter_type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            vkd3d_free((void *)p->u.descriptor_table.descriptor_ranges);
    }
}

static int convert_root_parameters_to_v_1_1(struct vkd3d_shader_root_parameter1 *dst,
        const struct vkd3d_shader_root_parameter *src, unsigned int count)
{
    const struct vkd3d_shader_descriptor_range *ranges;
    struct vkd3d_shader_descriptor_range1 *ranges1;
    unsigned int i, j;
    int ret;

    for (i = 0; i < count; ++i)
    {
        const struct vkd3d_shader_root_parameter *p = &src[i];
        struct vkd3d_shader_root_parameter1 *p1 = &dst[i];

        p1->parameter_type = p->parameter_type;
        switch (p1->parameter_type)
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                ranges1 = NULL;
                if ((p1->u.descriptor_table.descriptor_range_count = p->u.descriptor_table.descriptor_range_count))
                {
                    if (!(ranges1 = vkd3d_calloc(p1->u.descriptor_table.descriptor_range_count, sizeof(*ranges1))))
                    {
                        ret = VKD3D_ERROR_OUT_OF_MEMORY;
                        goto fail;
                    }
                }
                p1->u.descriptor_table.descriptor_ranges = ranges1;
                ranges = p->u.descriptor_table.descriptor_ranges;
                for (j = 0; j < p1->u.descriptor_table.descriptor_range_count; ++j)
                {
                    ranges1[j].range_type = ranges[j].range_type;
                    ranges1[j].descriptor_count = ranges[j].descriptor_count;
                    ranges1[j].base_shader_register = ranges[j].base_shader_register;
                    ranges1[j].register_space = ranges[j].register_space;
                    ranges1[j].flags = VKD3D_ROOT_SIGNATURE_1_0_DESCRIPTOR_RANGE_FLAGS;
                    ranges1[j].descriptor_table_offset = ranges[j].descriptor_table_offset;
                }
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                p1->u.constants = p->u.constants;
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                p1->u.descriptor.shader_register = p->u.descriptor.shader_register;
                p1->u.descriptor.register_space = p->u.descriptor.register_space;
                p1->u.descriptor.flags = VKD3D_ROOT_SIGNATURE_1_0_ROOT_DESCRIPTOR_FLAGS;
                break;
            default:
                WARN("Invalid root parameter type %#x.\n", p1->parameter_type);
                ret = VKD3D_ERROR_INVALID_ARGUMENT;
                goto fail;

        }
        p1->shader_visibility = p->shader_visibility;
    }

    return VKD3D_OK;

fail:
    free_descriptor_ranges1(dst, i);
    return ret;
}

static int convert_root_signature_to_v1_1(struct vkd3d_shader_versioned_root_signature_desc *dst,
        const struct vkd3d_shader_versioned_root_signature_desc *src)
{
    const struct vkd3d_shader_root_signature_desc *src_desc = &src->u.v_1_0;
    struct vkd3d_shader_root_signature_desc1 *dst_desc = &dst->u.v_1_1;
    struct vkd3d_shader_static_sampler_desc *samplers = NULL;
    struct vkd3d_shader_root_parameter1 *parameters = NULL;
    int ret;

    if ((dst_desc->parameter_count = src_desc->parameter_count))
    {
        if (!(parameters = vkd3d_calloc(dst_desc->parameter_count, sizeof(*parameters))))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        if ((ret = convert_root_parameters_to_v_1_1(parameters, src_desc->parameters, src_desc->parameter_count)) < 0)
            goto fail;
    }
    dst_desc->parameters = parameters;
    if ((dst_desc->static_sampler_count = src_desc->static_sampler_count))
    {
        if (!(samplers = vkd3d_calloc(dst_desc->static_sampler_count, sizeof(*samplers))))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        memcpy(samplers, src_desc->static_samplers, src_desc->static_sampler_count * sizeof(*samplers));
    }
    dst_desc->static_samplers = samplers;
    dst_desc->flags = src_desc->flags;

    return VKD3D_OK;

fail:
    free_descriptor_ranges1(parameters, dst_desc->parameter_count);
    vkd3d_free(parameters);
    vkd3d_free(samplers);
    return ret;
}

int vkd3d_shader_convert_root_signature(struct vkd3d_shader_versioned_root_signature_desc *dst,
        enum vkd3d_shader_root_signature_version version, const struct vkd3d_shader_versioned_root_signature_desc *src)
{
    int ret;

    TRACE("dst %p, version %#x, src %p.\n", dst, version, src);

    if (src->version == version)
    {
        WARN("Nothing to convert.\n");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0 && version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        WARN("Root signature version %#x not supported.\n", version);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (src->version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0
            && src->version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        WARN("Root signature version %#x not supported.\n", src->version);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    memset(dst, 0, sizeof(*dst));
    dst->version = version;

    if (version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
    {
        ret = convert_root_signature_to_v1_0(dst, src);
    }
    else
    {
        assert(version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1);
        ret = convert_root_signature_to_v1_1(dst, src);
    }

    return ret;
}
