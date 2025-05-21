/*
 * Copyright 2023 Conor McCarthy for CodeWeavers
 * Copyright 2023-2024 Elizabeth Figura for CodeWeavers
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
#include "vkd3d_types.h"

struct vsir_transformation_context
{
    enum vkd3d_result result;
    struct vsir_program *program;
    uint64_t config_flags;
    const struct vkd3d_shader_compile_info *compile_info;
    struct vkd3d_shader_message_context *message_context;
};

static int convert_parameter_info(const struct vkd3d_shader_compile_info *compile_info,
        unsigned int *ret_count, const struct vkd3d_shader_parameter1 **ret_parameters)
{
    const struct vkd3d_shader_spirv_target_info *spirv_info;
    struct vkd3d_shader_parameter1 *parameters;

    *ret_count = 0;
    *ret_parameters = NULL;

    if (!(spirv_info = vkd3d_find_struct(compile_info->next, SPIRV_TARGET_INFO)) || !spirv_info->parameter_count)
        return VKD3D_OK;

    if (!(parameters = vkd3d_calloc(spirv_info->parameter_count, sizeof(*parameters))))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    for (unsigned int i = 0; i < spirv_info->parameter_count; ++i)
    {
        const struct vkd3d_shader_parameter *src = &spirv_info->parameters[i];
        struct vkd3d_shader_parameter1 *dst = &parameters[i];

        dst->name = src->name;
        dst->type = src->type;
        dst->data_type = src->data_type;

        if (src->type == VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
        {
            dst->u.immediate_constant.u.u32 = src->u.immediate_constant.u.u32;
        }
        else if (src->type == VKD3D_SHADER_PARAMETER_TYPE_SPECIALIZATION_CONSTANT)
        {
            dst->u.specialization_constant = src->u.specialization_constant;
        }
        else
        {
            ERR("Invalid parameter type %#x.\n", src->type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    *ret_count = spirv_info->parameter_count;
    *ret_parameters = parameters;

    return VKD3D_OK;
}

bool vsir_program_init(struct vsir_program *program, const struct vkd3d_shader_compile_info *compile_info,
        const struct vkd3d_shader_version *version, unsigned int reserve, enum vsir_control_flow_type cf_type,
        enum vsir_normalisation_level normalisation_level)
{
    memset(program, 0, sizeof(*program));

    if (compile_info)
    {
        const struct vkd3d_shader_parameter_info *parameter_info;

        if ((parameter_info = vkd3d_find_struct(compile_info->next, PARAMETER_INFO)))
        {
            program->parameter_count = parameter_info->parameter_count;
            program->parameters = parameter_info->parameters;
        }
        else
        {
            if (convert_parameter_info(compile_info, &program->parameter_count, &program->parameters) < 0)
                return false;
            program->free_parameters = true;
        }
    }

    program->shader_version = *version;
    program->cf_type = cf_type;
    program->normalisation_level = normalisation_level;
    return shader_instruction_array_init(&program->instructions, reserve);
}

void vsir_program_cleanup(struct vsir_program *program)
{
    size_t i;

    if (program->free_parameters)
        vkd3d_free((void *)program->parameters);
    for (i = 0; i < program->block_name_count; ++i)
        vkd3d_free((void *)program->block_names[i]);
    vkd3d_free(program->block_names);
    shader_instruction_array_destroy(&program->instructions);
    shader_signature_cleanup(&program->input_signature);
    shader_signature_cleanup(&program->output_signature);
    shader_signature_cleanup(&program->patch_constant_signature);
    vkd3d_shader_free_scan_descriptor_info1(&program->descriptors);
}

const struct vkd3d_shader_parameter1 *vsir_program_get_parameter(
        const struct vsir_program *program, enum vkd3d_shader_parameter_name name)
{
    for (unsigned int i = 0; i < program->parameter_count; ++i)
    {
        if (program->parameters[i].name == name)
            return &program->parameters[i];
    }

    return NULL;
}

static struct signature_element *vsir_signature_find_element_by_name(
        const struct shader_signature *signature, const char *semantic_name, unsigned int semantic_index)
{
    for (unsigned int i = 0; i < signature->element_count; ++i)
    {
        if (!ascii_strcasecmp(signature->elements[i].semantic_name, semantic_name)
                && signature->elements[i].semantic_index == semantic_index)
            return &signature->elements[i];
    }

    return NULL;
}

bool vsir_signature_find_sysval(const struct shader_signature *signature,
        enum vkd3d_shader_sysval_semantic sysval, unsigned int semantic_index, unsigned int *element_index)
{
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];
        if (e->sysval_semantic == sysval && e->semantic_index == semantic_index)
        {
            *element_index = i;
            return true;
        }
    }

    return false;
}

void vsir_register_init(struct vkd3d_shader_register *reg, enum vkd3d_shader_register_type reg_type,
        enum vkd3d_data_type data_type, unsigned int idx_count)
{
    reg->type = reg_type;
    reg->precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    reg->non_uniform = false;
    reg->data_type = data_type;
    reg->idx[0].offset = ~0u;
    reg->idx[0].rel_addr = NULL;
    reg->idx[0].is_in_bounds = false;
    reg->idx[1].offset = ~0u;
    reg->idx[1].rel_addr = NULL;
    reg->idx[1].is_in_bounds = false;
    reg->idx[2].offset = ~0u;
    reg->idx[2].rel_addr = NULL;
    reg->idx[2].is_in_bounds = false;
    reg->idx_count = idx_count;
    reg->dimension = VSIR_DIMENSION_SCALAR;
    reg->alignment = 0;
}

static inline bool shader_register_is_phase_instance_id(const struct vkd3d_shader_register *reg)
{
    return reg->type == VKD3DSPR_FORKINSTID || reg->type == VKD3DSPR_JOININSTID;
}

void vsir_src_param_init(struct vkd3d_shader_src_param *param, enum vkd3d_shader_register_type reg_type,
        enum vkd3d_data_type data_type, unsigned int idx_count)
{
    vsir_register_init(&param->reg, reg_type, data_type, idx_count);
    param->swizzle = 0;
    param->modifiers = VKD3DSPSM_NONE;
}

static void src_param_init_const_uint(struct vkd3d_shader_src_param *src, uint32_t value)
{
    vsir_src_param_init(src, VKD3DSPR_IMMCONST, VKD3D_DATA_UINT, 0);
    src->reg.u.immconst_u32[0] = value;
}

static void vsir_src_param_init_io(struct vkd3d_shader_src_param *src,
        enum vkd3d_shader_register_type reg_type, const struct signature_element *e, unsigned int idx_count)
{
    vsir_src_param_init(src, reg_type, vkd3d_data_type_from_component_type(e->component_type), idx_count);
    src->reg.dimension = VSIR_DIMENSION_VEC4;
    src->swizzle = vsir_swizzle_from_writemask(e->mask);
}

void vsir_src_param_init_label(struct vkd3d_shader_src_param *param, unsigned int label_id)
{
    vsir_src_param_init(param, VKD3DSPR_LABEL, VKD3D_DATA_UNUSED, 1);
    param->reg.dimension = VSIR_DIMENSION_NONE;
    param->reg.idx[0].offset = label_id;
}

static void src_param_init_parameter(struct vkd3d_shader_src_param *src, uint32_t idx, enum vkd3d_data_type type)
{
    vsir_src_param_init(src, VKD3DSPR_PARAMETER, type, 1);
    src->reg.idx[0].offset = idx;
}

static void src_param_init_parameter_vec4(struct vkd3d_shader_src_param *src, uint32_t idx, enum vkd3d_data_type type)
{
    vsir_src_param_init(src, VKD3DSPR_PARAMETER, type, 1);
    src->reg.idx[0].offset = idx;
    src->reg.dimension = VSIR_DIMENSION_VEC4;
    src->swizzle = VKD3D_SHADER_NO_SWIZZLE;
}

static void vsir_src_param_init_resource(struct vkd3d_shader_src_param *src, unsigned int id, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_RESOURCE, VKD3D_DATA_UNUSED, 2);
    src->reg.idx[0].offset = id;
    src->reg.idx[1].offset = idx;
    src->reg.dimension = VSIR_DIMENSION_VEC4;
    src->swizzle = VKD3D_SHADER_NO_SWIZZLE;
}

static void vsir_src_param_init_sampler(struct vkd3d_shader_src_param *src, unsigned int id, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_SAMPLER, VKD3D_DATA_UNUSED, 2);
    src->reg.idx[0].offset = id;
    src->reg.idx[1].offset = idx;
    src->reg.dimension = VSIR_DIMENSION_NONE;
}

static void src_param_init_ssa_bool(struct vkd3d_shader_src_param *src, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_SSA, VKD3D_DATA_BOOL, 1);
    src->reg.idx[0].offset = idx;
}

static void src_param_init_ssa_float(struct vkd3d_shader_src_param *src, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_SSA, VKD3D_DATA_FLOAT, 1);
    src->reg.idx[0].offset = idx;
}

static void src_param_init_ssa_float4(struct vkd3d_shader_src_param *src, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_SSA, VKD3D_DATA_FLOAT, 1);
    src->reg.idx[0].offset = idx;
    src->reg.dimension = VSIR_DIMENSION_VEC4;
    src->swizzle = VKD3D_SHADER_NO_SWIZZLE;
}

static void src_param_init_temp_bool(struct vkd3d_shader_src_param *src, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_TEMP, VKD3D_DATA_BOOL, 1);
    src->reg.idx[0].offset = idx;
}

static void src_param_init_temp_float(struct vkd3d_shader_src_param *src, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    src->reg.idx[0].offset = idx;
}

static void src_param_init_temp_float4(struct vkd3d_shader_src_param *src, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    src->reg.dimension = VSIR_DIMENSION_VEC4;
    src->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    src->reg.idx[0].offset = idx;
}

static void src_param_init_temp_uint(struct vkd3d_shader_src_param *src, unsigned int idx)
{
    vsir_src_param_init(src, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
    src->reg.idx[0].offset = idx;
}

void vsir_dst_param_init(struct vkd3d_shader_dst_param *param, enum vkd3d_shader_register_type reg_type,
        enum vkd3d_data_type data_type, unsigned int idx_count)
{
    vsir_register_init(&param->reg, reg_type, data_type, idx_count);
    param->write_mask = VKD3DSP_WRITEMASK_0;
    param->modifiers = VKD3DSPDM_NONE;
    param->shift = 0;
}

static void vsir_dst_param_init_io(struct vkd3d_shader_dst_param *dst, enum vkd3d_shader_register_type reg_type,
        const struct signature_element *e, unsigned int idx_count)
{
    vsir_dst_param_init(dst, reg_type, vkd3d_data_type_from_component_type(e->component_type), idx_count);
    dst->reg.dimension = VSIR_DIMENSION_VEC4;
    dst->write_mask = e->mask;
}

void vsir_dst_param_init_null(struct vkd3d_shader_dst_param *dst)
{
    vsir_dst_param_init(dst, VKD3DSPR_NULL, VKD3D_DATA_UNUSED, 0);
    dst->reg.dimension = VSIR_DIMENSION_NONE;
    dst->write_mask = 0;
}

static void dst_param_init_ssa_bool(struct vkd3d_shader_dst_param *dst, unsigned int idx)
{
    vsir_dst_param_init(dst, VKD3DSPR_SSA, VKD3D_DATA_BOOL, 1);
    dst->reg.idx[0].offset = idx;
}

static void dst_param_init_ssa_float(struct vkd3d_shader_dst_param *dst, unsigned int idx)
{
    vsir_dst_param_init(dst, VKD3DSPR_SSA, VKD3D_DATA_FLOAT, 1);
    dst->reg.idx[0].offset = idx;
}

static void dst_param_init_ssa_float4(struct vkd3d_shader_dst_param *dst, unsigned int idx)
{
    vsir_dst_param_init(dst, VKD3DSPR_SSA, VKD3D_DATA_FLOAT, 1);
    dst->reg.idx[0].offset = idx;
    dst->reg.dimension = VSIR_DIMENSION_VEC4;
    dst->write_mask = VKD3DSP_WRITEMASK_ALL;
}

static void dst_param_init_temp_bool(struct vkd3d_shader_dst_param *dst, unsigned int idx)
{
    vsir_dst_param_init(dst, VKD3DSPR_TEMP, VKD3D_DATA_BOOL, 1);
    dst->reg.idx[0].offset = idx;
}

static void dst_param_init_temp_float4(struct vkd3d_shader_dst_param *dst, unsigned int idx)
{
    vsir_dst_param_init(dst, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    dst->reg.idx[0].offset = idx;
    dst->reg.dimension = VSIR_DIMENSION_VEC4;
}

static void dst_param_init_temp_uint(struct vkd3d_shader_dst_param *dst, unsigned int idx)
{
    vsir_dst_param_init(dst, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
    dst->reg.idx[0].offset = idx;
}

static void dst_param_init_output(struct vkd3d_shader_dst_param *dst,
        enum vkd3d_data_type data_type, uint32_t idx, uint32_t write_mask)
{
    vsir_dst_param_init(dst, VKD3DSPR_OUTPUT, data_type, 1);
    dst->reg.idx[0].offset = idx;
    dst->reg.dimension = VSIR_DIMENSION_VEC4;
    dst->write_mask = write_mask;
}

void vsir_instruction_init(struct vkd3d_shader_instruction *ins, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_opcode opcode)
{
    memset(ins, 0, sizeof(*ins));
    ins->location = *location;
    ins->opcode = opcode;
}

bool vsir_instruction_init_with_params(struct vsir_program *program,
        struct vkd3d_shader_instruction *ins, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_opcode opcode, unsigned int dst_count, unsigned int src_count)
{
    vsir_instruction_init(ins, location, opcode);
    ins->dst_count = dst_count;
    ins->src_count = src_count;

    if (!(ins->dst = vsir_program_get_dst_params(program, ins->dst_count)))
    {
        ERR("Failed to allocate %u destination parameters.\n", dst_count);
        return false;
    }

    if (!(ins->src = vsir_program_get_src_params(program, ins->src_count)))
    {
        ERR("Failed to allocate %u source parameters.\n", src_count);
        return false;
    }

    memset(ins->dst, 0, sizeof(*ins->dst) * ins->dst_count);
    memset(ins->src, 0, sizeof(*ins->src) * ins->src_count);
    return true;
}

static bool vsir_instruction_init_label(struct vkd3d_shader_instruction *ins,
        const struct vkd3d_shader_location *location, unsigned int label_id, struct vsir_program *program)
{
    struct vkd3d_shader_src_param *src_param;

    if (!(src_param = vsir_program_get_src_params(program, 1)))
        return false;

    vsir_src_param_init_label(src_param, label_id);

    vsir_instruction_init(ins, location, VKD3DSIH_LABEL);
    ins->src = src_param;
    ins->src_count = 1;

    return true;
}

static bool vsir_instruction_is_dcl(const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_shader_opcode opcode = instruction->opcode;
    return (VKD3DSIH_DCL <= opcode && opcode <= VKD3DSIH_DCL_VERTICES_OUT)
            || opcode == VKD3DSIH_HS_DECLS;
}

static void vkd3d_shader_instruction_make_nop(struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_shader_location location = ins->location;

    vsir_instruction_init(ins, &location, VKD3DSIH_NOP);
}

static bool get_opcode_from_rel_op(enum vkd3d_shader_rel_op rel_op, enum vkd3d_data_type data_type,
        enum vkd3d_shader_opcode *opcode, bool *requires_swap)
{
    switch (rel_op)
    {
        case VKD3D_SHADER_REL_OP_LT:
        case VKD3D_SHADER_REL_OP_GT:
            *requires_swap = (rel_op == VKD3D_SHADER_REL_OP_GT);
            if (data_type == VKD3D_DATA_FLOAT)
            {
                *opcode = VKD3DSIH_LTO;
                return true;
            }
            break;

        case VKD3D_SHADER_REL_OP_GE:
        case VKD3D_SHADER_REL_OP_LE:
            *requires_swap = (rel_op == VKD3D_SHADER_REL_OP_LE);
            if (data_type == VKD3D_DATA_FLOAT)
            {
                *opcode = VKD3DSIH_GEO;
                return true;
            }
            break;

        case VKD3D_SHADER_REL_OP_EQ:
            *requires_swap = false;
            if (data_type == VKD3D_DATA_FLOAT)
            {
                *opcode = VKD3DSIH_EQO;
                return true;
            }
            break;

        case VKD3D_SHADER_REL_OP_NE:
            *requires_swap = false;
            if (data_type == VKD3D_DATA_FLOAT)
            {
                *opcode = VKD3DSIH_NEO;
                return true;
            }
            break;
    }
    return false;
}

static enum vkd3d_result vsir_program_normalize_addr(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_instruction *ins, *ins2;
    unsigned int tmp_idx = ~0u;
    unsigned int i, k, r;

    for (i = 0; i < program->instructions.count; ++i)
    {
        ins = &program->instructions.elements[i];

        if (ins->opcode == VKD3DSIH_MOV && ins->dst[0].reg.type == VKD3DSPR_ADDR)
        {
            if (tmp_idx == ~0u)
                tmp_idx = program->temp_count++;

            ins->opcode = VKD3DSIH_FTOU;
            vsir_register_init(&ins->dst[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
            ins->dst[0].reg.idx[0].offset = tmp_idx;
            ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
        }
        else if (ins->opcode == VKD3DSIH_MOVA)
        {
            if (tmp_idx == ~0u)
                tmp_idx = program->temp_count++;

            if (!shader_instruction_array_insert_at(&program->instructions, i + 1, 1))
                return VKD3D_ERROR_OUT_OF_MEMORY;
            ins = &program->instructions.elements[i];
            ins2 = &program->instructions.elements[i + 1];

            ins->opcode = VKD3DSIH_ROUND_NE;
            vsir_register_init(&ins->dst[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
            ins->dst[0].reg.idx[0].offset = tmp_idx;
            ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;

            if (!vsir_instruction_init_with_params(program, ins2, &ins->location, VKD3DSIH_FTOU, 1, 1))
                return VKD3D_ERROR_OUT_OF_MEMORY;

            vsir_register_init(&ins2->dst[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
            ins2->dst[0].reg.idx[0].offset = tmp_idx;
            ins2->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
            ins2->dst[0].write_mask = ins->dst[0].write_mask;

            vsir_register_init(&ins2->src[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
            ins2->src[0].reg.idx[0].offset = tmp_idx;
            ins2->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
            ins2->src[0].swizzle = vsir_swizzle_from_writemask(ins2->dst[0].write_mask);
        }

        for (k = 0; k < ins->src_count; ++k)
        {
            struct vkd3d_shader_src_param *src = &ins->src[k];

            for (r = 0; r < src->reg.idx_count; ++r)
            {
                struct vkd3d_shader_src_param *rel = src->reg.idx[r].rel_addr;

                if (rel && rel->reg.type == VKD3DSPR_ADDR)
                {
                    if (tmp_idx == ~0u)
                        tmp_idx = program->temp_count++;

                    vsir_register_init(&rel->reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
                    rel->reg.idx[0].offset = tmp_idx;
                    rel->reg.dimension = VSIR_DIMENSION_VEC4;
                }
            }
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_ifc(struct vsir_program *program,
        struct vkd3d_shader_instruction *ifc, unsigned int *tmp_idx,
        struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    size_t pos = ifc - instructions->elements;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_shader_opcode opcode;
    bool swap;

    if (!shader_instruction_array_insert_at(instructions, pos + 1, 2))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ifc = &instructions->elements[pos];

    if (*tmp_idx == ~0u)
        *tmp_idx = program->temp_count++;

    /* Replace ifc comparison with actual comparison, saving the result in the tmp register. */
    if (!(get_opcode_from_rel_op(ifc->flags, ifc->src[0].reg.data_type, &opcode, &swap)))
    {
        vkd3d_shader_error(message_context, &ifc->location, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                "Aborting due to not yet implemented feature: opcode for rel_op %u and data type %u.",
                ifc->flags, ifc->src[0].reg.data_type);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }

    ins = &instructions->elements[pos + 1];
    if (!vsir_instruction_init_with_params(program, ins, &ifc->location, opcode, 1, 2))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    vsir_register_init(&ins->dst[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
    ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->dst[0].reg.idx[0].offset = *tmp_idx;
    ins->dst[0].write_mask = VKD3DSP_WRITEMASK_0;

    ins->src[0] = ifc->src[swap];
    ins->src[1] = ifc->src[!swap];

    /* Create new if instruction using the previous result. */
    ins = &instructions->elements[pos + 2];
    if (!vsir_instruction_init_with_params(program, ins, &ifc->location, VKD3DSIH_IF, 0, 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ins->flags = VKD3D_SHADER_CONDITIONAL_OP_NZ;

    vsir_register_init(&ins->src[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
    ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->src[0].reg.idx[0].offset = *tmp_idx;
    ins->src[0].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);

    /* Make the original instruction no-op */
    vkd3d_shader_instruction_make_nop(ifc);

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_texkill(struct vsir_program *program,
        struct vkd3d_shader_instruction *texkill, unsigned int *tmp_idx)
{
    const unsigned int components_read = 3 + (program->shader_version.major >= 2);
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    size_t pos = texkill - instructions->elements;
    struct vkd3d_shader_instruction *ins;
    unsigned int j;

    if (!shader_instruction_array_insert_at(instructions, pos + 1, components_read + 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    texkill = &instructions->elements[pos];

    if (*tmp_idx == ~0u)
        *tmp_idx = program->temp_count++;

    /* tmp = ins->src[0] < 0  */

    ins = &instructions->elements[pos + 1];
    if (!vsir_instruction_init_with_params(program, ins, &texkill->location, VKD3DSIH_LTO, 1, 2))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    vsir_register_init(&ins->dst[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
    ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->dst[0].reg.idx[0].offset = *tmp_idx;
    ins->dst[0].write_mask = VKD3DSP_WRITEMASK_ALL;

    ins->src[0].reg = texkill->src[0].reg;
    ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;
    vsir_register_init(&ins->src[1].reg, VKD3DSPR_IMMCONST, VKD3D_DATA_FLOAT, 0);
    ins->src[1].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->src[1].reg.u.immconst_f32[0] = 0.0f;
    ins->src[1].reg.u.immconst_f32[1] = 0.0f;
    ins->src[1].reg.u.immconst_f32[2] = 0.0f;
    ins->src[1].reg.u.immconst_f32[3] = 0.0f;

    /* tmp.x = tmp.x || tmp.y */
    /* tmp.x = tmp.x || tmp.z */
    /* tmp.x = tmp.x || tmp.w, if sm >= 2.0 */

    for (j = 1; j < components_read; ++j)
    {
        ins = &instructions->elements[pos + 1 + j];
        if (!(vsir_instruction_init_with_params(program, ins, &texkill->location, VKD3DSIH_OR, 1, 2)))
            return VKD3D_ERROR_OUT_OF_MEMORY;

        vsir_register_init(&ins->dst[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
        ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->dst[0].reg.idx[0].offset = *tmp_idx;
        ins->dst[0].write_mask = VKD3DSP_WRITEMASK_0;

        vsir_register_init(&ins->src[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
        ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->src[0].reg.idx[0].offset = *tmp_idx;
        ins->src[0].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
        vsir_register_init(&ins->src[1].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
        ins->src[1].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->src[1].reg.idx[0].offset = *tmp_idx;
        ins->src[1].swizzle = vkd3d_shader_create_swizzle(j, j, j, j);
    }

    /* discard_nz tmp.x */

    ins = &instructions->elements[pos + 1 + components_read];
    if (!(vsir_instruction_init_with_params(program, ins, &texkill->location, VKD3DSIH_DISCARD, 0, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ins->flags = VKD3D_SHADER_CONDITIONAL_OP_NZ;

    vsir_register_init(&ins->src[0].reg, VKD3DSPR_TEMP, VKD3D_DATA_UINT, 1);
    ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->src[0].reg.idx[0].offset = *tmp_idx;
    ins->src[0].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);

    /* Make the original instruction no-op */
    vkd3d_shader_instruction_make_nop(texkill);

    return VKD3D_OK;
}

/* The Shader Model 5 Assembly documentation states: "If components of a mad
 * instruction are tagged as precise, the hardware must execute a mad instruction
 * or the exact equivalent, and it cannot split it into a multiply followed by an add."
 * But DXIL.rst states the opposite: "Floating point multiply & add. This operation is
 * not fused for "precise" operations."
 * Windows drivers seem to conform with the latter, for SM 4-5 and SM 6. */
static enum vkd3d_result vsir_program_lower_precise_mad(struct vsir_program *program,
        struct vkd3d_shader_instruction *mad, unsigned int *tmp_idx)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_instruction *mul_ins, *add_ins;
    size_t pos = mad - instructions->elements;
    struct vkd3d_shader_dst_param *mul_dst;

    if (!(mad->flags & VKD3DSI_PRECISE_XYZW))
        return VKD3D_OK;

    if (!shader_instruction_array_insert_at(instructions, pos + 1, 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    mad = &instructions->elements[pos];

    if (*tmp_idx == ~0u)
        *tmp_idx = program->temp_count++;

    mul_ins = &instructions->elements[pos];
    add_ins = &instructions->elements[pos + 1];

    mul_ins->opcode = VKD3DSIH_MUL;
    mul_ins->src_count = 2;

    if (!(vsir_instruction_init_with_params(program, add_ins, &mul_ins->location, VKD3DSIH_ADD, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    add_ins->flags = mul_ins->flags & VKD3DSI_PRECISE_XYZW;

    mul_dst = mul_ins->dst;
    *add_ins->dst = *mul_dst;

    mul_dst->modifiers = 0;
    vsir_register_init(&mul_dst->reg, VKD3DSPR_TEMP, mul_ins->src[0].reg.data_type, 1);
    mul_dst->reg.dimension = add_ins->dst->reg.dimension;
    mul_dst->reg.idx[0].offset = *tmp_idx;

    add_ins->src[0].reg = mul_dst->reg;
    add_ins->src[0].swizzle = vsir_swizzle_from_writemask(mul_dst->write_mask);
    add_ins->src[0].modifiers = 0;
    add_ins->src[1] = mul_ins->src[2];

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_sm1_sincos(struct vsir_program *program,
        struct vkd3d_shader_instruction *sincos)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    size_t pos = sincos - instructions->elements;
    struct vkd3d_shader_instruction *ins;
    unsigned int s;

    if (sincos->dst_count != 1)
        return VKD3D_OK;

    if (!shader_instruction_array_insert_at(instructions, pos + 1, 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    sincos = &instructions->elements[pos];

    ins = &instructions->elements[pos + 1];

    if (!(vsir_instruction_init_with_params(program, ins, &sincos->location, VKD3DSIH_SINCOS, 2, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    ins->flags = sincos->flags;

    *ins->src = *sincos->src;
    /* Set the source swizzle to replicate the first component. */
    s = vsir_swizzle_get_component(sincos->src->swizzle, 0);
    ins->src->swizzle = vkd3d_shader_create_swizzle(s, s, s, s);

    if (sincos->dst->write_mask & VKD3DSP_WRITEMASK_1)
    {
        ins->dst[0] = *sincos->dst;
        ins->dst[0].write_mask = VKD3DSP_WRITEMASK_1;
    }
    else
    {
        vsir_dst_param_init_null(&ins->dst[0]);
    }

    if (sincos->dst->write_mask & VKD3DSP_WRITEMASK_0)
    {
        ins->dst[1] = *sincos->dst;
        ins->dst[1].write_mask = VKD3DSP_WRITEMASK_0;
    }
    else
    {
        vsir_dst_param_init_null(&ins->dst[1]);
    }

    /* Make the original instruction no-op */
    vkd3d_shader_instruction_make_nop(sincos);

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_texldp(struct vsir_program *program,
        struct vkd3d_shader_instruction *tex, unsigned int *tmp_idx)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_location *location = &tex->location;
    struct vkd3d_shader_instruction *div_ins, *tex_ins;
    size_t pos = tex - instructions->elements;
    unsigned int w_comp;

    w_comp = vsir_swizzle_get_component(tex->src[0].swizzle, 3);

    if (!shader_instruction_array_insert_at(instructions, pos + 1, 2))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    tex = &instructions->elements[pos];

    if (*tmp_idx == ~0u)
        *tmp_idx = program->temp_count++;

    div_ins = &instructions->elements[pos + 1];
    tex_ins = &instructions->elements[pos + 2];

    if (!vsir_instruction_init_with_params(program, div_ins, location, VKD3DSIH_DIV, 1, 2))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    vsir_dst_param_init(&div_ins->dst[0], VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    div_ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
    div_ins->dst[0].reg.idx[0].offset = *tmp_idx;
    div_ins->dst[0].write_mask = VKD3DSP_WRITEMASK_ALL;

    div_ins->src[0] = tex->src[0];

    div_ins->src[1] = tex->src[0];
    div_ins->src[1].swizzle = vkd3d_shader_create_swizzle(w_comp, w_comp, w_comp, w_comp);

    if (!vsir_instruction_init_with_params(program, tex_ins, location, VKD3DSIH_TEX, 1, 2))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    tex_ins->dst[0] = tex->dst[0];

    tex_ins->src[0].reg = div_ins->dst[0].reg;
    tex_ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;

    tex_ins->src[1] = tex->src[1];

    vkd3d_shader_instruction_make_nop(tex);

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_tex(struct vsir_program *program,
        struct vkd3d_shader_instruction *tex, struct vkd3d_shader_message_context *message_context)
{
    unsigned int idx = tex->src[1].reg.idx[0].offset;
    struct vkd3d_shader_src_param *srcs;

    VKD3D_ASSERT(tex->src[1].reg.idx_count == 1);
    VKD3D_ASSERT(!tex->src[1].reg.idx[0].rel_addr);

    if (!(srcs = shader_src_param_allocator_get(&program->instructions.src_params, 4)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    srcs[0] = tex->src[0];
    vsir_src_param_init_resource(&srcs[1], idx, idx);
    vsir_src_param_init_sampler(&srcs[2], idx, idx);

    if (!tex->flags)
    {
        tex->opcode = VKD3DSIH_SAMPLE;
        tex->src = srcs;
        tex->src_count = 3;
    }
    else if (tex->flags == VKD3DSI_TEXLD_BIAS)
    {
        enum vkd3d_shader_swizzle_component w = vsir_swizzle_get_component(srcs[0].swizzle, 3);

        tex->opcode = VKD3DSIH_SAMPLE_B;
        tex->src = srcs;
        tex->src_count = 4;

        srcs[3] = tex->src[0];
        srcs[3].swizzle = vkd3d_shader_create_swizzle(w, w, w, w);
    }
    else
    {
        vkd3d_shader_error(message_context, &tex->location,
                VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED, "Unhandled tex flags %#x.", tex->flags);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_texldd(struct vsir_program *program,
        struct vkd3d_shader_instruction *texldd)
{
    unsigned int idx = texldd->src[1].reg.idx[0].offset;
    struct vkd3d_shader_src_param *srcs;

    VKD3D_ASSERT(texldd->src[1].reg.idx_count == 1);
    VKD3D_ASSERT(!texldd->src[1].reg.idx[0].rel_addr);

    if (!(srcs = shader_src_param_allocator_get(&program->instructions.src_params, 5)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    srcs[0] = texldd->src[0];
    vsir_src_param_init_resource(&srcs[1], idx, idx);
    vsir_src_param_init_sampler(&srcs[2], idx, idx);
    srcs[3] = texldd->src[2];
    srcs[4] = texldd->src[3];

    texldd->opcode = VKD3DSIH_SAMPLE_GRAD;
    texldd->src = srcs;
    texldd->src_count = 5;

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_dcl_input(struct vsir_program *program,
        struct vkd3d_shader_instruction *ins, struct vsir_transformation_context *ctx)
{
    switch (ins->declaration.dst.reg.type)
    {
        case VKD3DSPR_INPUT:
        case VKD3DSPR_OUTPUT:
        case VKD3DSPR_PATCHCONST:
        case VKD3DSPR_INCONTROLPOINT:
        case VKD3DSPR_OUTCONTROLPOINT:
            break;

        case VKD3DSPR_PRIMID:
        case VKD3DSPR_FORKINSTID:
        case VKD3DSPR_JOININSTID:
        case VKD3DSPR_THREADID:
        case VKD3DSPR_THREADGROUPID:
        case VKD3DSPR_LOCALTHREADID:
        case VKD3DSPR_LOCALTHREADINDEX:
        case VKD3DSPR_COVERAGE:
        case VKD3DSPR_TESSCOORD:
        case VKD3DSPR_OUTPOINTID:
        case VKD3DSPR_GSINSTID:
        case VKD3DSPR_WAVELANECOUNT:
        case VKD3DSPR_WAVELANEINDEX:
            bitmap_set(program->io_dcls, ins->declaration.dst.reg.type);
            break;

        default:
            vkd3d_shader_error(ctx->message_context, &ins->location,
                    VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Internal compiler error: invalid register type %#x for DCL_INPUT.",
                    ins->declaration.dst.reg.type);
            return VKD3D_ERROR;
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_dcl_output(struct vsir_program *program,
        struct vkd3d_shader_instruction *ins, struct vsir_transformation_context *ctx)
{
    switch (ins->declaration.dst.reg.type)
    {
        case VKD3DSPR_INPUT:
        case VKD3DSPR_OUTPUT:
        case VKD3DSPR_PATCHCONST:
        case VKD3DSPR_INCONTROLPOINT:
        case VKD3DSPR_OUTCONTROLPOINT:
            break;

        case VKD3DSPR_DEPTHOUT:
        case VKD3DSPR_SAMPLEMASK:
        case VKD3DSPR_DEPTHOUTGE:
        case VKD3DSPR_DEPTHOUTLE:
        case VKD3DSPR_OUTSTENCILREF:
            bitmap_set(program->io_dcls, ins->declaration.dst.reg.type);
            break;

        default:
            vkd3d_shader_error(ctx->message_context, &ins->location,
                    VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Internal compiler error: invalid register type %#x for DCL_OUTPUT.",
                    ins->declaration.dst.reg.type);
            return VKD3D_ERROR;
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_lower_instructions(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_message_context *message_context = ctx->message_context;
    unsigned int tmp_idx = ~0u, i;
    enum vkd3d_result ret;

    for (i = 0; i < instructions->count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &instructions->elements[i];

        switch (ins->opcode)
        {
            case VKD3DSIH_IFC:
                if ((ret = vsir_program_lower_ifc(program, ins, &tmp_idx, message_context)) < 0)
                    return ret;
                break;

            case VKD3DSIH_TEXKILL:
                if ((ret = vsir_program_lower_texkill(program, ins, &tmp_idx)) < 0)
                    return ret;
                break;

            case VKD3DSIH_MAD:
                if ((ret = vsir_program_lower_precise_mad(program, ins, &tmp_idx)) < 0)
                    return ret;
                break;

            case VKD3DSIH_DCL:
            case VKD3DSIH_DCL_CONSTANT_BUFFER:
            case VKD3DSIH_DCL_GLOBAL_FLAGS:
            case VKD3DSIH_DCL_SAMPLER:
            case VKD3DSIH_DCL_TEMPS:
            case VKD3DSIH_DCL_TESSELLATOR_DOMAIN:
            case VKD3DSIH_DCL_THREAD_GROUP:
            case VKD3DSIH_DCL_UAV_TYPED:
                vkd3d_shader_instruction_make_nop(ins);
                break;

            case VKD3DSIH_DCL_INPUT:
                vsir_program_lower_dcl_input(program, ins, ctx);
                vkd3d_shader_instruction_make_nop(ins);
                break;

            case VKD3DSIH_DCL_OUTPUT:
                vsir_program_lower_dcl_output(program, ins, ctx);
                vkd3d_shader_instruction_make_nop(ins);
                break;

            case VKD3DSIH_DCL_INPUT_SGV:
            case VKD3DSIH_DCL_INPUT_SIV:
            case VKD3DSIH_DCL_INPUT_PS:
            case VKD3DSIH_DCL_INPUT_PS_SGV:
            case VKD3DSIH_DCL_INPUT_PS_SIV:
            case VKD3DSIH_DCL_OUTPUT_SIV:
                vkd3d_shader_instruction_make_nop(ins);
                break;

            case VKD3DSIH_SINCOS:
                if ((ret = vsir_program_lower_sm1_sincos(program, ins)) < 0)
                    return ret;
                break;

            case VKD3DSIH_TEX:
                if (ins->flags == VKD3DSI_TEXLD_PROJECT)
                {
                    if ((ret = vsir_program_lower_texldp(program, ins, &tmp_idx)) < 0)
                        return ret;
                }
                else
                {
                    if ((ret = vsir_program_lower_tex(program, ins, message_context)) < 0)
                        return ret;
                }
                break;

            case VKD3DSIH_TEXLDD:
                if ((ret = vsir_program_lower_texldd(program, ins)) < 0)
                    return ret;
                break;

            case VKD3DSIH_TEXBEM:
            case VKD3DSIH_TEXBEML:
            case VKD3DSIH_TEXCOORD:
            case VKD3DSIH_TEXDEPTH:
            case VKD3DSIH_TEXDP3:
            case VKD3DSIH_TEXDP3TEX:
            case VKD3DSIH_TEXLDL:
            case VKD3DSIH_TEXM3x2PAD:
            case VKD3DSIH_TEXM3x2TEX:
            case VKD3DSIH_TEXM3x3DIFF:
            case VKD3DSIH_TEXM3x3PAD:
            case VKD3DSIH_TEXM3x3SPEC:
            case VKD3DSIH_TEXM3x3TEX:
            case VKD3DSIH_TEXM3x3VSPEC:
            case VKD3DSIH_TEXREG2AR:
            case VKD3DSIH_TEXREG2GB:
            case VKD3DSIH_TEXREG2RGB:
                vkd3d_shader_error(ctx->message_context, &ins->location, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                        "Aborting due to unimplemented feature: Combined sampler instruction %#x.",
                        ins->opcode);
                return VKD3D_ERROR_NOT_IMPLEMENTED;

            default:
                break;
        }
    }

    return VKD3D_OK;
}

static void shader_register_eliminate_phase_addressing(struct vkd3d_shader_register *reg,
        unsigned int instance_id)
{
    unsigned int i;

    for (i = 0; i < reg->idx_count; ++i)
    {
        if (reg->idx[i].rel_addr && shader_register_is_phase_instance_id(&reg->idx[i].rel_addr->reg))
        {
            reg->idx[i].rel_addr = NULL;
            reg->idx[i].offset += instance_id;
        }
    }
}

static void shader_instruction_eliminate_phase_instance_id(struct vkd3d_shader_instruction *ins,
        unsigned int instance_id)
{
    struct vkd3d_shader_register *reg;
    unsigned int i;

    for (i = 0; i < ins->src_count; ++i)
    {
        reg = (struct vkd3d_shader_register *)&ins->src[i].reg;
        if (shader_register_is_phase_instance_id(reg))
        {
            vsir_register_init(reg, VKD3DSPR_IMMCONST, reg->data_type, 0);
            reg->u.immconst_u32[0] = instance_id;
            continue;
        }
        shader_register_eliminate_phase_addressing(reg, instance_id);
    }

    for (i = 0; i < ins->dst_count; ++i)
        shader_register_eliminate_phase_addressing(&ins->dst[i].reg, instance_id);
}

/* Ensure that the program closes with a ret. sm1 programs do not, by default.
 * Many of our IR passes rely on this in order to insert instructions at the
 * end of execution. */
static enum vkd3d_result vsir_program_ensure_ret(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    static const struct vkd3d_shader_location no_loc;
    if (program->instructions.count
            && program->instructions.elements[program->instructions.count - 1].opcode == VKD3DSIH_RET)
        return VKD3D_OK;

    if (!shader_instruction_array_insert_at(&program->instructions, program->instructions.count, 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_instruction_init(&program->instructions.elements[program->instructions.count - 1], &no_loc, VKD3DSIH_RET);
    return VKD3D_OK;
}

static bool add_signature_element(struct shader_signature *signature, const char *semantic_name,
        uint32_t semantic_index, uint32_t mask, uint32_t register_index,
        enum vkd3d_shader_interpolation_mode interpolation_mode)
{
    struct signature_element *new_elements, *e;

    if (!(new_elements = vkd3d_realloc(signature->elements,
            (signature->element_count + 1) * sizeof(*signature->elements))))
        return false;
    signature->elements = new_elements;
    e = &signature->elements[signature->element_count++];
    memset(e, 0, sizeof(*e));
    e->semantic_name = vkd3d_strdup(semantic_name);
    e->semantic_index = semantic_index;
    e->sysval_semantic = VKD3D_SHADER_SV_NONE;
    e->component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    e->register_count = 1;
    e->mask = mask;
    e->used_mask = mask;
    e->register_index = register_index;
    e->target_location = register_index;
    e->interpolation_mode = interpolation_mode;
    return true;
}

static enum vkd3d_result vsir_program_add_diffuse_output(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct shader_signature *signature = &program->output_signature;
    struct signature_element *e;

    if (program->shader_version.type != VKD3D_SHADER_TYPE_VERTEX)
        return VKD3D_OK;

    if ((e = vsir_signature_find_element_by_name(signature, "COLOR", 0)))
    {
        program->diffuse_written_mask = e->mask;
        e->mask = VKD3DSP_WRITEMASK_ALL;

        return VKD3D_OK;
    }

    if (!add_signature_element(signature, "COLOR", 0, VKD3DSP_WRITEMASK_ALL, SM1_COLOR_REGISTER_OFFSET, VKD3DSIM_NONE))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    return VKD3D_OK;
}

/* Uninitialized components of diffuse yield 1.0 in SM1-2. Implement this by
 * always writing diffuse in those versions, even if the PS doesn't read it. */
static enum vkd3d_result vsir_program_ensure_diffuse(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    static const struct vkd3d_shader_location no_loc;
    struct vkd3d_shader_instruction *ins;
    unsigned int i;

    if (program->shader_version.type != VKD3D_SHADER_TYPE_VERTEX
            || program->diffuse_written_mask == VKD3DSP_WRITEMASK_ALL)
        return VKD3D_OK;

    /* Write the instruction after all LABEL, DCL, and NOP instructions.
     * We need to skip NOP instructions because they might result from removed
     * DCLs, and there could still be DCLs after NOPs. */
    for (i = 0; i < program->instructions.count; ++i)
    {
        ins = &program->instructions.elements[i];

        if (!vsir_instruction_is_dcl(ins) && ins->opcode != VKD3DSIH_LABEL && ins->opcode != VKD3DSIH_NOP)
            break;
    }

    if (!shader_instruction_array_insert_at(&program->instructions, i, 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ins = &program->instructions.elements[i];

    vsir_instruction_init_with_params(program, ins, &no_loc, VKD3DSIH_MOV, 1, 1);
    vsir_dst_param_init(&ins->dst[0], VKD3DSPR_ATTROUT, VKD3D_DATA_FLOAT, 1);
    ins->dst[0].reg.idx[0].offset = 0;
    ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->dst[0].write_mask = VKD3DSP_WRITEMASK_ALL & ~program->diffuse_written_mask;
    vsir_src_param_init(&ins->src[0], VKD3DSPR_IMMCONST, VKD3D_DATA_FLOAT, 0);
    ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
    for (i = 0; i < 4; ++i)
        ins->src[0].reg.u.immconst_f32[i] = 1.0f;
    return VKD3D_OK;
}

static const struct vkd3d_shader_varying_map *find_varying_map(
        const struct vkd3d_shader_varying_map_info *varying_map, unsigned int signature_idx)
{
    unsigned int i;

    for (i = 0; i < varying_map->varying_count; ++i)
    {
        if (varying_map->varying_map[i].output_signature_index == signature_idx)
            return &varying_map->varying_map[i];
    }

    return NULL;
}

static bool target_allows_subset_masks(const struct vkd3d_shader_compile_info *info)
{
    const struct vkd3d_shader_spirv_target_info *spirv_info;
    enum vkd3d_shader_spirv_environment environment;

    switch (info->target_type)
    {
        case VKD3D_SHADER_TARGET_SPIRV_BINARY:
            spirv_info = vkd3d_find_struct(info->next, SPIRV_TARGET_INFO);
            environment = spirv_info ? spirv_info->environment : VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_0;

            switch (environment)
            {
                case VKD3D_SHADER_SPIRV_ENVIRONMENT_OPENGL_4_5:
                    return true;

                case VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_0:
                case VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_1:
                    /* FIXME: Allow KHR_maintenance4. */
                    return false;

                default:
                    FIXME("Unrecognized environment %#x.\n", environment);
                    return false;
            }

        default:
            return true;
    }
}

static void remove_unread_output_components(const struct shader_signature *signature,
        struct vkd3d_shader_instruction *ins, struct vkd3d_shader_dst_param *dst)
{
    const struct signature_element *e;

    switch (dst->reg.type)
    {
        case VKD3DSPR_OUTPUT:
        case VKD3DSPR_TEXCRDOUT:
            e = vsir_signature_find_element_for_reg(signature, dst->reg.idx[0].offset, 0);
            break;

        case VKD3DSPR_ATTROUT:
            e = vsir_signature_find_element_for_reg(signature,
                    SM1_COLOR_REGISTER_OFFSET + dst->reg.idx[0].offset, 0);
            break;

        case VKD3DSPR_RASTOUT:
            e = vsir_signature_find_element_for_reg(signature,
                    SM1_RASTOUT_REGISTER_OFFSET + dst->reg.idx[0].offset, 0);
            break;

        default:
            return;
    }

    /* We already changed the mask earlier. */
    dst->write_mask &= e->mask;

    if (!dst->write_mask)
    {
        if (ins->dst_count == 1)
            vkd3d_shader_instruction_make_nop(ins);
        else
            vsir_dst_param_init_null(dst);
    }
}

static enum vkd3d_result vsir_program_remap_output_signature(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    const struct vkd3d_shader_location location = {.source_name = ctx->compile_info->source_name};
    struct vkd3d_shader_message_context *message_context = ctx->message_context;
    const struct vkd3d_shader_compile_info *compile_info = ctx->compile_info;
    bool allows_subset_masks = target_allows_subset_masks(compile_info);
    struct shader_signature *signature = &program->output_signature;
    unsigned int orig_element_count = signature->element_count;
    const struct vkd3d_shader_varying_map_info *varying_map;
    struct signature_element *new_elements, *e;
    unsigned int uninit_varying_count = 0;
    unsigned int subset_varying_count = 0;
    unsigned int new_register_count = 0;
    unsigned int i;

    if (!(varying_map = vkd3d_find_struct(compile_info->next, VARYING_MAP_INFO)))
        return VKD3D_OK;

    for (i = 0; i < signature->element_count; ++i)
    {
        const struct vkd3d_shader_varying_map *map = find_varying_map(varying_map, i);

        e = &signature->elements[i];
        if (map)
        {
            unsigned int input_mask = map->input_mask;

            e->target_location = map->input_register_index;

            TRACE("Mapping signature index %u (mask %#x) to target location %u (mask %#x).\n",
                    i, e->mask, map->input_register_index, map->input_mask);

            if ((input_mask & e->mask) == input_mask)
            {
                ++subset_varying_count;
                if (!allows_subset_masks)
                {
                    e->mask = input_mask;
                    e->used_mask &= input_mask;
                }
            }
            else if (input_mask && input_mask != e->mask)
            {
                vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                        "Aborting due to not yet implemented feature: "
                        "Input mask %#x reads components not written in output mask %#x.",
                        input_mask, e->mask);
                return VKD3D_ERROR_NOT_IMPLEMENTED;
            }
        }
        else
        {
            TRACE("Marking signature index %u (mask %#x) as unused.\n", i, e->mask);

            e->target_location = SIGNATURE_TARGET_LOCATION_UNUSED;
        }

        new_register_count = max(new_register_count, e->register_index + 1);
    }

    /* Handle uninitialized varyings by writing them before every ret.
     *
     * As far as sm1-sm3 is concerned, drivers disagree on what uninitialized
     * varyings contain.
     *
     * - Diffuse (COLOR0) reliably contains (1, 1, 1, 1) in SM1/2.
     *   In SM3 it may contain (0, 0, 0, 0), (0, 0, 0, 1), or (1, 1, 1, 1).
     *
     * - Specular (COLOR1) contains (0, 0, 0, 0) or (0, 0, 0, 1).
     *   WARP writes (1, 1, 1, 1).
     *
     * - Anything else contains (0, 0, 0, 0) or (0, 0, 0, 1).
     *
     * We don't have enough knowledge to identify diffuse here. Instead we deal
     * with that in vsir_program_ensure_diffuse(), by always writing diffuse if
     * the shader doesn't.
     */

    for (i = 0; i < varying_map->varying_count; ++i)
    {
        if (varying_map->varying_map[i].output_signature_index >= signature->element_count)
            ++uninit_varying_count;
    }

    if (!(new_elements = vkd3d_realloc(signature->elements,
            (signature->element_count + uninit_varying_count) * sizeof(*signature->elements))))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    signature->elements = new_elements;

    for (i = 0; i < varying_map->varying_count; ++i)
    {
        const struct vkd3d_shader_varying_map *map = &varying_map->varying_map[i];

        if (map->output_signature_index < orig_element_count)
            continue;

        TRACE("Synthesizing zero value for uninitialized output %u (mask %u).\n",
                map->input_register_index, map->input_mask);
        e = &signature->elements[signature->element_count++];
        memset(e, 0, sizeof(*e));
        e->sysval_semantic = VKD3D_SHADER_SV_NONE;
        e->component_type = VKD3D_SHADER_COMPONENT_FLOAT;
        e->register_count = 1;
        e->mask = map->input_mask;
        e->used_mask = map->input_mask;
        e->register_index = new_register_count++;
        e->target_location = map->input_register_index;
        e->interpolation_mode = VKD3DSIM_LINEAR;
    }

    /* Write each uninitialized varying before each ret. */
    for (i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];
        struct vkd3d_shader_location loc;

        if (ins->opcode != VKD3DSIH_RET)
            continue;

        loc = ins->location;
        if (!shader_instruction_array_insert_at(&program->instructions, i, uninit_varying_count))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        ins = &program->instructions.elements[i];

        for (unsigned int j = signature->element_count - uninit_varying_count; j < signature->element_count; ++j)
        {
            e = &signature->elements[j];

            vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_MOV, 1, 1);
            dst_param_init_output(&ins->dst[0], VKD3D_DATA_FLOAT, e->register_index, e->mask);
            vsir_src_param_init(&ins->src[0], VKD3DSPR_IMMCONST, VKD3D_DATA_FLOAT, 0);
            ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
            ++ins;
        }

        i += uninit_varying_count;
    }

    /* Vulkan (without KHR_maintenance4) disallows any mismatching masks,
     * including when the input mask is a proper subset of the output mask.
     * Resolve this by rewriting the shader to remove unread components from
     * any writes to the output variable. */

    if (!subset_varying_count || allows_subset_masks)
        return VKD3D_OK;

    for (i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];

        for (unsigned int j = 0; j < ins->dst_count; ++j)
            remove_unread_output_components(signature, ins, &ins->dst[j]);
    }

    return VKD3D_OK;
}

struct hull_flattener
{
    struct vkd3d_shader_instruction_array instructions;

    unsigned int instance_count;
    unsigned int phase_body_idx;
    enum vkd3d_shader_opcode phase;
    struct vkd3d_shader_location last_ret_location;
};

static bool flattener_is_in_fork_or_join_phase(const struct hull_flattener *flattener)
{
    return flattener->phase == VKD3DSIH_HS_FORK_PHASE || flattener->phase == VKD3DSIH_HS_JOIN_PHASE;
}

struct shader_phase_location
{
    unsigned int index;
    unsigned int instance_count;
    unsigned int instruction_count;
};

struct shader_phase_location_array
{
    /* Unlikely worst case: one phase for each component of each output register. */
    struct shader_phase_location locations[MAX_REG_OUTPUT * VKD3D_VEC4_SIZE];
    unsigned int count;
};

static void flattener_eliminate_phase_related_dcls(struct hull_flattener *normaliser,
        unsigned int index, struct shader_phase_location_array *locations)
{
    struct vkd3d_shader_instruction *ins = &normaliser->instructions.elements[index];
    struct shader_phase_location *loc;
    bool b;

    if (ins->opcode == VKD3DSIH_HS_FORK_PHASE || ins->opcode == VKD3DSIH_HS_JOIN_PHASE)
    {
        b = flattener_is_in_fork_or_join_phase(normaliser);
        /* Reset the phase info. */
        normaliser->phase_body_idx = ~0u;
        normaliser->phase = ins->opcode;
        normaliser->instance_count = 1;
        /* Leave the first occurrence and delete the rest. */
        if (b)
            vkd3d_shader_instruction_make_nop(ins);
        return;
    }
    else if (ins->opcode == VKD3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT
            || ins->opcode == VKD3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT)
    {
        normaliser->instance_count = ins->declaration.count + !ins->declaration.count;
        vkd3d_shader_instruction_make_nop(ins);
        return;
    }

    if (normaliser->phase == VKD3DSIH_INVALID || vsir_instruction_is_dcl(ins))
        return;

    if (normaliser->phase_body_idx == ~0u)
        normaliser->phase_body_idx = index;

    if (ins->opcode == VKD3DSIH_RET)
    {
        normaliser->last_ret_location = ins->location;
        vkd3d_shader_instruction_make_nop(ins);
        if (locations->count >= ARRAY_SIZE(locations->locations))
        {
            FIXME("Insufficient space for phase location.\n");
            return;
        }
        loc = &locations->locations[locations->count++];
        loc->index = normaliser->phase_body_idx;
        loc->instance_count = normaliser->instance_count;
        loc->instruction_count = index - normaliser->phase_body_idx;
    }
}

static enum vkd3d_result flattener_flatten_phases(struct hull_flattener *normaliser,
        struct shader_phase_location_array *locations)
{
    struct shader_phase_location *loc;
    unsigned int i, j, k, end, count;

    for (i = 0, count = 0; i < locations->count; ++i)
        count += (locations->locations[i].instance_count - 1) * locations->locations[i].instruction_count;

    if (!shader_instruction_array_reserve(&normaliser->instructions, normaliser->instructions.count + count))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    end = normaliser->instructions.count;
    normaliser->instructions.count += count;

    for (i = locations->count; i > 0; --i)
    {
        loc = &locations->locations[i - 1];
        j = loc->index + loc->instruction_count;
        memmove(&normaliser->instructions.elements[j + count], &normaliser->instructions.elements[j],
                (end - j) * sizeof(*normaliser->instructions.elements));
        end = j;
        count -= (loc->instance_count - 1) * loc->instruction_count;
        loc->index += count;
    }

    for (i = 0, count = 0; i < locations->count; ++i)
    {
        loc = &locations->locations[i];
        /* Make a copy of the non-dcl instructions for each instance. */
        for (j = 1; j < loc->instance_count; ++j)
        {
            for (k = 0; k < loc->instruction_count; ++k)
            {
                if (!shader_instruction_array_clone_instruction(&normaliser->instructions,
                        loc->index + loc->instruction_count * j + k, loc->index + k))
                    return VKD3D_ERROR_OUT_OF_MEMORY;
            }
        }
        /* Replace each reference to the instance id with a constant instance id. */
        for (j = 0; j < loc->instance_count; ++j)
        {
            for (k = 0; k < loc->instruction_count; ++k)
                shader_instruction_eliminate_phase_instance_id(
                        &normaliser->instructions.elements[loc->index + loc->instruction_count * j + k], j);
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_flatten_hull_shader_phases(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct hull_flattener flattener = {program->instructions};
    struct vkd3d_shader_instruction_array *instructions;
    struct shader_phase_location_array locations;
    enum vkd3d_result result = VKD3D_OK;
    unsigned int i;

    instructions = &flattener.instructions;

    flattener.phase = VKD3DSIH_INVALID;
    for (i = 0, locations.count = 0; i < instructions->count; ++i)
        flattener_eliminate_phase_related_dcls(&flattener, i, &locations);
    bitmap_clear(program->io_dcls, VKD3DSPR_FORKINSTID);
    bitmap_clear(program->io_dcls, VKD3DSPR_JOININSTID);

    if ((result = flattener_flatten_phases(&flattener, &locations)) < 0)
        return result;

    if (flattener.phase != VKD3DSIH_INVALID)
    {
        if (!shader_instruction_array_reserve(&flattener.instructions, flattener.instructions.count + 1))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_instruction_init(&instructions->elements[instructions->count++], &flattener.last_ret_location, VKD3DSIH_RET);
    }

    program->instructions = flattener.instructions;
    return result;
}

struct control_point_normaliser
{
    struct vkd3d_shader_instruction_array instructions;
    enum vkd3d_shader_opcode phase;
    struct vkd3d_shader_src_param *outpointid_param;
};

static bool control_point_normaliser_is_in_control_point_phase(const struct control_point_normaliser *normaliser)
{
    return normaliser->phase == VKD3DSIH_HS_CONTROL_POINT_PHASE;
}

struct vkd3d_shader_src_param *vsir_program_create_outpointid_param(struct vsir_program *program)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_src_param *rel_addr;

    if (instructions->outpointid_param)
        return instructions->outpointid_param;

    if (!(rel_addr = shader_src_param_allocator_get(&instructions->src_params, 1)))
        return NULL;

    vsir_register_init(&rel_addr->reg, VKD3DSPR_OUTPOINTID, VKD3D_DATA_UINT, 0);
    rel_addr->swizzle = 0;
    rel_addr->modifiers = 0;

    instructions->outpointid_param = rel_addr;
    return rel_addr;
}

static void shader_dst_param_normalise_outpointid(struct vkd3d_shader_dst_param *dst_param,
        struct control_point_normaliser *normaliser)
{
    struct vkd3d_shader_register *reg = &dst_param->reg;

    if (control_point_normaliser_is_in_control_point_phase(normaliser) && reg->type == VKD3DSPR_OUTPUT)
    {
        /* The TPF reader validates idx_count. */
        VKD3D_ASSERT(reg->idx_count == 1);
        reg->idx[1] = reg->idx[0];
        /* The control point id param is implicit here. Avoid later complications by inserting it. */
        reg->idx[0].offset = 0;
        reg->idx[0].rel_addr = normaliser->outpointid_param;
        ++reg->idx_count;
    }
}

static enum vkd3d_result control_point_normaliser_emit_hs_input(struct control_point_normaliser *normaliser,
        const struct shader_signature *s, unsigned int input_control_point_count, unsigned int dst,
        const struct vkd3d_shader_location *location)
{
    struct vkd3d_shader_instruction *ins;
    const struct signature_element *e;
    unsigned int i, count = 2;

    for (i = 0; i < s->element_count; ++i)
        count += !!s->elements[i].used_mask;

    if (!shader_instruction_array_reserve(&normaliser->instructions, normaliser->instructions.count + count))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    memmove(&normaliser->instructions.elements[dst + count], &normaliser->instructions.elements[dst],
            (normaliser->instructions.count - dst) * sizeof(*normaliser->instructions.elements));
    normaliser->instructions.count += count;

    ins = &normaliser->instructions.elements[dst];
    vsir_instruction_init(ins, location, VKD3DSIH_HS_CONTROL_POINT_PHASE);

    ++ins;

    for (i = 0; i < s->element_count; ++i)
    {
        e = &s->elements[i];
        if (!e->used_mask)
            continue;

        vsir_instruction_init(ins, location, VKD3DSIH_MOV);
        ins->dst = shader_dst_param_allocator_get(&normaliser->instructions.dst_params, 1);
        ins->dst_count = 1;
        ins->src = shader_src_param_allocator_get(&normaliser->instructions.src_params, 1);
        ins->src_count = 1;

        if (!ins->dst || ! ins->src)
        {
            WARN("Failed to allocate dst/src param.\n");
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }

        vsir_dst_param_init_io(&ins->dst[0], VKD3DSPR_OUTPUT, e, 2);
        ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->dst[0].reg.idx[0].offset = 0;
        ins->dst[0].reg.idx[0].rel_addr = normaliser->outpointid_param;
        ins->dst[0].reg.idx[1].offset = e->register_index;

        vsir_src_param_init_io(&ins->src[0], VKD3DSPR_INPUT, e, 2);
        ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->src[0].reg.idx[0].offset = 0;
        ins->src[0].reg.idx[0].rel_addr = normaliser->outpointid_param;
        ins->src[0].reg.idx[1].offset = e->register_index;

        ++ins;
    }

    vsir_instruction_init(ins, location, VKD3DSIH_RET);

    return VKD3D_OK;
}

static enum vkd3d_result instruction_array_normalise_hull_shader_control_point_io(
        struct vsir_program *program, struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_instruction_array *instructions;
    struct control_point_normaliser normaliser;
    unsigned int input_control_point_count;
    struct vkd3d_shader_location location;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_result ret;
    unsigned int i, j;

    VKD3D_ASSERT(program->normalisation_level == VSIR_NORMALISED_SM4);

    if (program->shader_version.type != VKD3D_SHADER_TYPE_HULL)
    {
        program->normalisation_level = VSIR_NORMALISED_HULL_CONTROL_POINT_IO;
        return VKD3D_OK;
    }

    if (!(normaliser.outpointid_param = vsir_program_create_outpointid_param(program)))
    {
        ERR("Failed to allocate src param.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    normaliser.instructions = program->instructions;
    instructions = &normaliser.instructions;
    normaliser.phase = VKD3DSIH_INVALID;

    for (i = 0; i < normaliser.instructions.count; ++i)
    {
        ins = &instructions->elements[i];

        switch (ins->opcode)
        {
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                normaliser.phase = ins->opcode;
                break;
            default:
                if (vsir_instruction_is_dcl(ins))
                    break;
                for (j = 0; j < ins->dst_count; ++j)
                    shader_dst_param_normalise_outpointid(&ins->dst[j], &normaliser);
                break;
        }
    }

    normaliser.phase = VKD3DSIH_INVALID;
    input_control_point_count = 1;

    for (i = 0; i < instructions->count; ++i)
    {
        ins = &instructions->elements[i];

        switch (ins->opcode)
        {
            case VKD3DSIH_DCL_INPUT_CONTROL_POINT_COUNT:
                input_control_point_count = ins->declaration.count;
                break;
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
                program->instructions = normaliser.instructions;
                program->normalisation_level = VSIR_NORMALISED_HULL_CONTROL_POINT_IO;
                return VKD3D_OK;
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                /* ins may be relocated if the instruction array expands. */
                location = ins->location;
                ret = control_point_normaliser_emit_hs_input(&normaliser, &program->input_signature,
                        input_control_point_count, i, &location);
                program->instructions = normaliser.instructions;
                program->normalisation_level = VSIR_NORMALISED_HULL_CONTROL_POINT_IO;
                return ret;
            default:
                break;
        }
    }

    program->instructions = normaliser.instructions;
    program->normalisation_level = VSIR_NORMALISED_HULL_CONTROL_POINT_IO;
    return VKD3D_OK;
}

struct io_normaliser_register_data
{
    struct
    {
        uint8_t register_count;
        uint32_t mask;
        uint32_t used_mask;
    } component[VKD3D_VEC4_SIZE];
};


struct io_normaliser
{
    struct vkd3d_shader_message_context *message_context;
    struct vkd3d_shader_instruction_array instructions;
    enum vkd3d_shader_type shader_type;
    uint8_t major;
    struct shader_signature *input_signature;
    struct shader_signature *output_signature;
    struct shader_signature *patch_constant_signature;

    unsigned int instance_count;
    unsigned int phase_body_idx;
    enum vkd3d_shader_opcode phase;
    unsigned int output_control_point_count;

    struct vkd3d_shader_src_param *outpointid_param;

    struct vkd3d_shader_dst_param *input_dcl_params[MAX_REG_OUTPUT];
    struct vkd3d_shader_dst_param *output_dcl_params[MAX_REG_OUTPUT];
    struct vkd3d_shader_dst_param *pc_dcl_params[MAX_REG_OUTPUT];
    struct io_normaliser_register_data input_range_map[MAX_REG_OUTPUT];
    struct io_normaliser_register_data output_range_map[MAX_REG_OUTPUT];
    struct io_normaliser_register_data pc_range_map[MAX_REG_OUTPUT];

    bool use_vocp;
};

static bool io_normaliser_is_in_fork_or_join_phase(const struct io_normaliser *normaliser)
{
    return normaliser->phase == VKD3DSIH_HS_FORK_PHASE || normaliser->phase == VKD3DSIH_HS_JOIN_PHASE;
}

static bool shader_signature_find_element_for_reg(const struct shader_signature *signature,
        unsigned int reg_idx, unsigned int write_mask, unsigned int *element_idx)
{
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];
        if (e->register_index <= reg_idx && e->register_count > reg_idx - e->register_index
                && (e->mask & write_mask) == write_mask)
        {
            *element_idx = i;
            return true;
        }
    }

    return false;
}

struct signature_element *vsir_signature_find_element_for_reg(const struct shader_signature *signature,
        unsigned int reg_idx, unsigned int write_mask)
{
    unsigned int element_idx;

    if (shader_signature_find_element_for_reg(signature, reg_idx, write_mask, &element_idx))
        return &signature->elements[element_idx];

    return NULL;
}

static unsigned int range_map_get_register_count(struct io_normaliser_register_data range_map[],
        unsigned int register_idx, uint32_t write_mask)
{
    return range_map[register_idx].component[vsir_write_mask_get_component_idx(write_mask)].register_count;
}

static enum vkd3d_result range_map_set_register_range(struct io_normaliser *normaliser,
        struct io_normaliser_register_data range_map[], unsigned int register_idx,
        unsigned int register_count, uint32_t mask, uint32_t used_mask, bool is_dcl_indexrange)
{
    unsigned int i, j, r, c, component_idx, component_count;

    VKD3D_ASSERT(mask <= VKD3DSP_WRITEMASK_ALL);
    component_idx = vsir_write_mask_get_component_idx(mask);
    component_count = vsir_write_mask_component_count(mask);

    VKD3D_ASSERT(register_idx < MAX_REG_OUTPUT && MAX_REG_OUTPUT - register_idx >= register_count);

    if (range_map[register_idx].component[component_idx].register_count > register_count && is_dcl_indexrange)
    {
        if (range_map[register_idx].component[component_idx].register_count == UINT8_MAX)
        {
            WARN("Conflicting index ranges.\n");
            vkd3d_shader_error(normaliser->message_context, NULL,
                    VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE, "Conflicting index ranges.");
            return VKD3D_ERROR_INVALID_SHADER;
        }
        return VKD3D_OK;
    }
    if (range_map[register_idx].component[component_idx].register_count == register_count)
    {
        /* Already done. This happens when fxc splits a register declaration by
         * component(s). The dcl_indexrange instructions are split too. */
        return VKD3D_OK;
    }
    range_map[register_idx].component[component_idx].register_count = register_count;
    range_map[register_idx].component[component_idx].mask = mask;
    range_map[register_idx].component[component_idx].used_mask = used_mask;

    for (i = 0; i < register_count; ++i)
    {
        r = register_idx + i;
        for (j = !i; j < component_count; ++j)
        {
            c = component_idx + j;
            /* A synthetic patch constant range which overlaps an existing range can start upstream of it
             * for fork/join phase instancing, but ranges declared by dcl_indexrange should not overlap.
             * The latter is validated in the TPF reader. */
            if (range_map[r].component[c].register_count && is_dcl_indexrange)
            {
                WARN("Conflicting index ranges.\n");
                vkd3d_shader_error(normaliser->message_context, NULL,
                        VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE, "Conflicting index ranges.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            range_map[r].component[c].register_count = UINT8_MAX;
            range_map[r].component[c].mask = mask;
            range_map[r].component[c].used_mask = used_mask;
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result io_normaliser_add_index_range(struct io_normaliser *normaliser,
        const struct vkd3d_shader_instruction *ins)
{
    const struct vkd3d_shader_index_range *range = &ins->declaration.index_range;
    const struct vkd3d_shader_register *reg = &range->dst.reg;
    struct io_normaliser_register_data *range_map;
    const struct shader_signature *signature;
    uint32_t mask, used_mask;
    unsigned int reg_idx, i;

    switch (reg->type)
    {
        case VKD3DSPR_INPUT:
        case VKD3DSPR_INCONTROLPOINT:
            range_map = normaliser->input_range_map;
            signature = normaliser->input_signature;
            break;
        case VKD3DSPR_OUTCONTROLPOINT:
            range_map = normaliser->output_range_map;
            signature = normaliser->output_signature;
            break;
        case VKD3DSPR_OUTPUT:
            if (!io_normaliser_is_in_fork_or_join_phase(normaliser))
            {
                range_map = normaliser->output_range_map;
                signature = normaliser->output_signature;
                break;
            }
            /* fall through */
        case VKD3DSPR_PATCHCONST:
            range_map = normaliser->pc_range_map;
            signature = normaliser->patch_constant_signature;
            break;
        default:
            /* Validated in the TPF reader. */
            vkd3d_unreachable();
    }

    reg_idx = reg->idx[reg->idx_count - 1].offset;
    mask = range->dst.write_mask;
    used_mask = 0;

    for (i = 0; i < range->register_count; ++i)
    {
        struct signature_element *element;

        if ((element = vsir_signature_find_element_for_reg(signature, reg_idx + i, mask)))
        {
            mask |= element->mask;
            used_mask |= element->used_mask;
        }
    }

    return range_map_set_register_range(normaliser, range_map, reg_idx, range->register_count, mask, used_mask, true);
}

static int signature_element_mask_compare(const void *a, const void *b)
{
    const struct signature_element *e = a, *f = b;
    int ret;

    return (ret = vkd3d_u32_compare(e->mask, f->mask)) ? ret : vkd3d_u32_compare(e->register_index, f->register_index);
}

static bool sysval_semantics_should_merge(const struct signature_element *e, const struct signature_element *f)
{
    if (e->sysval_semantic < VKD3D_SHADER_SV_TESS_FACTOR_QUADEDGE
            || e->sysval_semantic > VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN)
        return false;

    return e->sysval_semantic == f->sysval_semantic
            /* Line detail and density must be merged together to match the SPIR-V array.
             * This deletes one of the two sysvals, but these are not used. */
            || (e->sysval_semantic == VKD3D_SHADER_SV_TESS_FACTOR_LINEDET
            && f->sysval_semantic == VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN)
            || (e->sysval_semantic == VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN
            && f->sysval_semantic == VKD3D_SHADER_SV_TESS_FACTOR_LINEDET);
}

/* Merge tess factor sysvals because they are an array in SPIR-V. */
static enum vkd3d_result shader_signature_map_patch_constant_index_ranges(struct io_normaliser *normaliser,
        struct shader_signature *s, struct io_normaliser_register_data range_map[])
{
    unsigned int i, j, register_count;
    struct signature_element *e, *f;
    enum vkd3d_result ret;

    qsort(s->elements, s->element_count, sizeof(s->elements[0]), signature_element_mask_compare);

    for (i = 0; i < s->element_count; i += register_count)
    {
        uint32_t used_mask;

        e = &s->elements[i];
        register_count = 1;

        if (!e->sysval_semantic)
            continue;

        used_mask = e->used_mask;
        for (j = i + 1; j < s->element_count; ++j, ++register_count)
        {
            f = &s->elements[j];
            if (f->register_index != e->register_index + register_count || !sysval_semantics_should_merge(e, f))
                break;
            used_mask |= f->used_mask;
        }
        if (register_count < 2)
            continue;

        if ((ret = range_map_set_register_range(normaliser, range_map,
                e->register_index, register_count, e->mask, used_mask, false)) < 0)
            return ret;
    }

    return VKD3D_OK;
}

static int signature_element_register_compare(const void *a, const void *b)
{
    const struct signature_element *e = a, *f = b;
    int ret;

    if ((ret = vkd3d_u32_compare(e->register_index, f->register_index)))
        return ret;

    /* System values like SV_RenderTargetArrayIndex and SV_ViewPortArrayIndex
     * can get packed into the same I/O register as non-system values, but
     * only at the end. E.g.:
     *
     *     vs_4_0
     *     ...
     *     .output
     *     ...
     *     .param B.x, o1.x, uint
     *     .param C.y, o1.y, uint
     *     .param SV_RenderTargetArrayIndex.z, o1.z, uint, RTINDEX
     *     .text
     *     ...
     *     mov o1.xy, v1.xyxx
     *     mov o1.z, v1.z
     *     ret
     *
     * Because I/O normalisation doesn't split writes like the mov to o1.xy
     * above, we want to make sure that o1.x and o1.y continue to be packed
     * into a single register after I/O normalisation, so we order system
     * values after non-system values here, allowing the non-system values to
     * get merged into a single register. */
    return vkd3d_u32_compare(f->sysval_semantic, e->sysval_semantic);
}

static int signature_element_index_compare(const void *a, const void *b)
{
    const struct signature_element *e = a, *f = b;

    return vkd3d_u32_compare(e->sort_index, f->sort_index);
}

static enum vkd3d_result shader_signature_merge(struct io_normaliser *normaliser,
        struct shader_signature *s, struct io_normaliser_register_data range_map[],
        bool is_patch_constant)
{
    unsigned int i, j, element_count, new_count, register_count;
    struct signature_element *elements;
    enum vkd3d_result ret = VKD3D_OK;
    struct signature_element *e, *f;
    bool used;

    element_count = s->element_count;
    if (!(elements = vkd3d_malloc(element_count * sizeof(*elements))))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    if (element_count)
        memcpy(elements, s->elements, element_count * sizeof(*elements));

    for (i = 0; i < element_count; ++i)
        elements[i].sort_index = i;

    qsort(elements, element_count, sizeof(elements[0]), signature_element_register_compare);

    for (i = 0, new_count = 0; i < element_count; i = j, elements[new_count++] = *e)
    {
        e = &elements[i];
        j = i + 1;

        if (e->register_index == ~0u)
            continue;

        /* Do not merge if the register index will be relative-addressed. */
        if (range_map_get_register_count(range_map, e->register_index, e->mask) > 1)
            continue;

        used = e->used_mask;

        for (; j < element_count; ++j)
        {
            f = &elements[j];

            /* Merge different components of the same register unless sysvals are different,
             * or it will be relative-addressed. */
            if (f->register_index != e->register_index || f->sysval_semantic != e->sysval_semantic
                    || range_map_get_register_count(range_map, f->register_index, f->mask) > 1)
                break;

            TRACE("Merging %s, reg %u, mask %#x, sysval %#x with %s, mask %#x, sysval %#x.\n", e->semantic_name,
                    e->register_index, e->mask, e->sysval_semantic, f->semantic_name, f->mask, f->sysval_semantic);
            VKD3D_ASSERT(!(e->mask & f->mask));

            e->mask |= f->mask;
            e->used_mask |= f->used_mask;
            e->semantic_index = min(e->semantic_index, f->semantic_index);

            /* The first element may have no interpolation mode if it is unused. Elements which
             * actually have different interpolation modes are assigned different registers. */
            if (f->used_mask && !used)
            {
                if (e->interpolation_mode && e->interpolation_mode != f->interpolation_mode)
                    FIXME("Mismatching interpolation modes %u and %u.\n", e->interpolation_mode, f->interpolation_mode);
                else
                    e->interpolation_mode = f->interpolation_mode;
            }

            vkd3d_free((void *)f->semantic_name);
        }
    }
    element_count = new_count;
    vkd3d_free(s->elements);
    s->elements = elements;
    s->element_count = element_count;

    if (is_patch_constant
            && (ret = shader_signature_map_patch_constant_index_ranges(normaliser, s, range_map)) < 0)
        goto out;

    for (i = 0, new_count = 0; i < element_count; ++i)
    {
        e = &elements[i];
        register_count = 1;

        if (e->register_index >= MAX_REG_OUTPUT)
        {
            elements[new_count++] = *e;
            continue;
        }

        register_count = range_map_get_register_count(range_map, e->register_index, e->mask);

        if (register_count == UINT8_MAX)
        {
            TRACE("Register %u mask %#x semantic %s%u has already been merged, dropping it.\n",
                    e->register_index, e->mask, e->semantic_name, e->semantic_index);
            vkd3d_free((void *)e->semantic_name);
            continue;
        }

        if (register_count > 0)
        {
            TRACE("Register %u mask %#x semantic %s%u is used as merge destination.\n",
                    e->register_index, e->mask, e->semantic_name, e->semantic_index);
            e->register_count = register_count;
            e->mask = range_map[e->register_index].component[vsir_write_mask_get_component_idx(e->mask)].mask;
            e->used_mask = range_map[e->register_index].component[vsir_write_mask_get_component_idx(e->mask)].used_mask;
        }

        elements[new_count++] = *e;
    }
    s->element_count = new_count;

out:
    /* Restoring the original order is required for sensible trace output. */
    qsort(s->elements, s->element_count, sizeof(elements[0]), signature_element_index_compare);

    return ret;
}

static unsigned int shader_register_normalise_arrayed_addressing(struct vkd3d_shader_register *reg,
        unsigned int id_idx, unsigned int register_index)
{
    VKD3D_ASSERT(id_idx < ARRAY_SIZE(reg->idx) - 1);

    /* Make room for the array index at the front of the array. */
    ++id_idx;
    memmove(&reg->idx[1], &reg->idx[0], id_idx * sizeof(reg->idx[0]));

    /* The array index inherits the register relative address, but is offsetted
     * by the signature element register index. */
    reg->idx[0].rel_addr = reg->idx[id_idx].rel_addr;
    reg->idx[0].offset = reg->idx[id_idx].offset - register_index;
    reg->idx[id_idx].rel_addr = NULL;

    /* The signature index offset will be fixed in the caller. */

    return id_idx;
}

static bool shader_dst_param_io_normalise(struct vkd3d_shader_dst_param *dst_param,
         struct io_normaliser *normaliser)
 {
    unsigned int id_idx, reg_idx, write_mask, element_idx;
    struct vkd3d_shader_register *reg = &dst_param->reg;
    const struct shader_signature *signature;
    const struct signature_element *e;

    write_mask = dst_param->write_mask;

    switch (reg->type)
    {
        case VKD3DSPR_OUTPUT:
            reg_idx = reg->idx[reg->idx_count - 1].offset;
            if (io_normaliser_is_in_fork_or_join_phase(normaliser))
            {
                signature = normaliser->patch_constant_signature;
                /* Convert patch constant outputs to the patch constant register type to avoid the need
                 * to convert compiler symbols when accessed as inputs in a later stage. */
                reg->type = VKD3DSPR_PATCHCONST;
            }
            else
            {
                signature = normaliser->output_signature;
            }
            break;

        case VKD3DSPR_PATCHCONST:
            reg_idx = reg->idx[reg->idx_count - 1].offset;
            signature = normaliser->patch_constant_signature;
            break;

        case VKD3DSPR_TEXCRDOUT:
        case VKD3DSPR_COLOROUT:
            reg_idx = reg->idx[0].offset;
            signature = normaliser->output_signature;
            reg->type = VKD3DSPR_OUTPUT;
            break;

        case VKD3DSPR_INCONTROLPOINT:
        case VKD3DSPR_INPUT:
            reg_idx = reg->idx[reg->idx_count - 1].offset;
            signature = normaliser->input_signature;
            reg->type = VKD3DSPR_INPUT;
            break;

        case VKD3DSPR_ATTROUT:
            reg_idx = SM1_COLOR_REGISTER_OFFSET + reg->idx[0].offset;
            signature = normaliser->output_signature;
            reg->type = VKD3DSPR_OUTPUT;
            break;

        case VKD3DSPR_RASTOUT:
            /* Leave point size as a system value for the backends to consume. */
            if (reg->idx[0].offset == VSIR_RASTOUT_POINT_SIZE)
                return true;
            reg_idx = SM1_RASTOUT_REGISTER_OFFSET + reg->idx[0].offset;
            signature = normaliser->output_signature;
            reg->type = VKD3DSPR_OUTPUT;
            /* Fog and point size are scalar, but fxc/d3dcompiler emits a full
             * write mask when writing to them. */
            if (reg->idx[0].offset > 0)
                write_mask = VKD3DSP_WRITEMASK_0;
            break;

        default:
            return true;
    }

    id_idx = reg->idx_count - 1;
    if (!shader_signature_find_element_for_reg(signature, reg_idx, write_mask, &element_idx))
        vkd3d_unreachable();
    e = &signature->elements[element_idx];

    if ((e->register_count > 1 || vsir_sysval_semantic_is_tess_factor(e->sysval_semantic)))
        id_idx = shader_register_normalise_arrayed_addressing(reg, id_idx, e->register_index);

    /* Replace the register index with the signature element index */
    reg->idx[id_idx].offset = element_idx;
    reg->idx_count = id_idx + 1;

    return true;
}

static void shader_src_param_io_normalise(struct vkd3d_shader_src_param *src_param,
        struct io_normaliser *normaliser)
{
    unsigned int i, id_idx, reg_idx, write_mask, element_idx, component_idx;
    struct vkd3d_shader_register *reg = &src_param->reg;
    const struct shader_signature *signature;
    const struct signature_element *e;

    /* Input/output registers from one phase can be used as inputs in
     * subsequent phases. Specifically:
     *
     *   - Control phase inputs are available as "vicp" in fork and join
     *     phases.
     *   - Control phase outputs are available as "vocp" in fork and join
     *     phases.
     *   - Fork phase patch constants are available as "vpc" in join
     *     phases.
     *
     * We handle "vicp" here by converting INCONTROLPOINT src registers to
     * type INPUT so they match the control phase declarations. We handle
     * "vocp" by converting OUTCONTROLPOINT registers to type OUTPUT.
     * Merging fork and join phases handles "vpc". */

    switch (reg->type)
    {
        case VKD3DSPR_PATCHCONST:
            reg_idx = reg->idx[reg->idx_count - 1].offset;
            signature = normaliser->patch_constant_signature;
            break;

        case VKD3DSPR_INCONTROLPOINT:
            reg->type = VKD3DSPR_INPUT;
            /* fall through */
        case VKD3DSPR_INPUT:
            if (normaliser->major < 3 && normaliser->shader_type == VKD3D_SHADER_TYPE_PIXEL)
                reg_idx = SM1_COLOR_REGISTER_OFFSET + reg->idx[0].offset;
            else
                reg_idx = reg->idx[reg->idx_count - 1].offset;
            signature = normaliser->input_signature;
            break;

        case VKD3DSPR_OUTCONTROLPOINT:
            reg->type = VKD3DSPR_OUTPUT;
            if (io_normaliser_is_in_fork_or_join_phase(normaliser))
                normaliser->use_vocp = true;
            /* fall through */
        case VKD3DSPR_OUTPUT:
            reg_idx = reg->idx[reg->idx_count - 1].offset;
            signature = normaliser->output_signature;
            break;

        case VKD3DSPR_TEXTURE:
            reg->type = VKD3DSPR_INPUT;
            reg_idx = reg->idx[0].offset;
            signature = normaliser->input_signature;
            break;

        default:
            return;
    }

    id_idx = reg->idx_count - 1;
    write_mask = VKD3DSP_WRITEMASK_0 << vsir_swizzle_get_component(src_param->swizzle, 0);
    if (!shader_signature_find_element_for_reg(signature, reg_idx, write_mask, &element_idx))
        vkd3d_unreachable();

    e = &signature->elements[element_idx];
    if ((e->register_count > 1 || vsir_sysval_semantic_is_tess_factor(e->sysval_semantic)))
        id_idx = shader_register_normalise_arrayed_addressing(reg, id_idx, e->register_index);
    reg->idx[id_idx].offset = element_idx;
    reg->idx_count = id_idx + 1;

    if ((component_idx = vsir_write_mask_get_component_idx(e->mask)))
    {
        for (i = 0; i < VKD3D_VEC4_SIZE; ++i)
            if (vsir_swizzle_get_component(src_param->swizzle, i))
                src_param->swizzle -= component_idx << VKD3D_SHADER_SWIZZLE_SHIFT(i);
    }
}

static void shader_instruction_normalise_io_params(struct vkd3d_shader_instruction *ins,
        struct io_normaliser *normaliser)
{
    unsigned int i;

    switch (ins->opcode)
    {
        case VKD3DSIH_HS_CONTROL_POINT_PHASE:
        case VKD3DSIH_HS_FORK_PHASE:
        case VKD3DSIH_HS_JOIN_PHASE:
            normaliser->phase = ins->opcode;
            memset(normaliser->input_dcl_params, 0, sizeof(normaliser->input_dcl_params));
            memset(normaliser->output_dcl_params, 0, sizeof(normaliser->output_dcl_params));
            memset(normaliser->pc_dcl_params, 0, sizeof(normaliser->pc_dcl_params));
            break;
        default:
            if (vsir_instruction_is_dcl(ins))
                break;
            for (i = 0; i < ins->dst_count; ++i)
                shader_dst_param_io_normalise(&ins->dst[i], normaliser);
            for (i = 0; i < ins->src_count; ++i)
                shader_src_param_io_normalise(&ins->src[i], normaliser);
            break;
    }
}

static enum vkd3d_result vsir_program_normalise_io_registers(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct io_normaliser normaliser = {ctx->message_context, program->instructions};
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_result ret;
    unsigned int i;

    VKD3D_ASSERT(program->normalisation_level == VSIR_NORMALISED_HULL_CONTROL_POINT_IO);

    normaliser.phase = VKD3DSIH_INVALID;
    normaliser.shader_type = program->shader_version.type;
    normaliser.major = program->shader_version.major;
    normaliser.input_signature = &program->input_signature;
    normaliser.output_signature = &program->output_signature;
    normaliser.patch_constant_signature = &program->patch_constant_signature;

    for (i = 0; i < program->instructions.count; ++i)
    {
        ins = &program->instructions.elements[i];

        switch (ins->opcode)
        {
            case VKD3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT:
                normaliser.output_control_point_count = ins->declaration.count;
                break;
            case VKD3DSIH_DCL_INDEX_RANGE:
                if ((ret = io_normaliser_add_index_range(&normaliser, ins)) < 0)
                    return ret;
                vkd3d_shader_instruction_make_nop(ins);
                break;
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                normaliser.phase = ins->opcode;
                break;
            default:
                break;
        }
    }

    if ((ret = shader_signature_merge(&normaliser, &program->input_signature, normaliser.input_range_map, false)) < 0
            || (ret = shader_signature_merge(&normaliser, &program->output_signature,
                    normaliser.output_range_map, false)) < 0
            || (ret = shader_signature_merge(&normaliser, &program->patch_constant_signature,
                    normaliser.pc_range_map, true)) < 0)
    {
        program->instructions = normaliser.instructions;
        return ret;
    }

    normaliser.phase = VKD3DSIH_INVALID;
    for (i = 0; i < normaliser.instructions.count; ++i)
        shader_instruction_normalise_io_params(&normaliser.instructions.elements[i], &normaliser);

    program->instructions = normaliser.instructions;
    program->use_vocp = normaliser.use_vocp;
    program->normalisation_level = VSIR_NORMALISED_SM6;
    return VKD3D_OK;
}

struct flat_constant_def
{
    enum vkd3d_shader_d3dbc_constant_register set;
    uint32_t index;
    uint32_t value[4];
};

struct flat_constants_normaliser
{
    struct flat_constant_def *defs;
    size_t def_count, defs_capacity;
};

static bool get_flat_constant_register_type(const struct vkd3d_shader_register *reg,
        enum vkd3d_shader_d3dbc_constant_register *set, uint32_t *index,
        struct vkd3d_shader_src_param **rel_addr)
{
    static const struct
    {
        enum vkd3d_shader_register_type type;
        enum vkd3d_shader_d3dbc_constant_register set;
    }
    regs[] =
    {
        {VKD3DSPR_CONST, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER},
        {VKD3DSPR_CONSTINT, VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER},
        {VKD3DSPR_CONSTBOOL, VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER},
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(regs); ++i)
    {
        if (reg->type == regs[i].type)
        {
            if (rel_addr)
                *rel_addr = reg->idx[0].rel_addr;
            *set = regs[i].set;
            *index = reg->idx[0].offset;
            return true;
        }
    }

    return false;
}

static void shader_register_normalise_flat_constants(struct vkd3d_shader_src_param *param,
        const struct flat_constants_normaliser *normaliser)
{
    enum vkd3d_shader_d3dbc_constant_register set;
    struct vkd3d_shader_src_param *rel_addr;
    uint32_t index;
    size_t i, j;

    if (!get_flat_constant_register_type(&param->reg, &set, &index, &rel_addr))
        return;

    for (i = 0; i < normaliser->def_count; ++i)
    {
        if (normaliser->defs[i].set == set && normaliser->defs[i].index == index)
        {
            param->reg.type = VKD3DSPR_IMMCONST;
            param->reg.idx_count = 0;
            param->reg.dimension = VSIR_DIMENSION_VEC4;
            for (j = 0; j < 4; ++j)
                param->reg.u.immconst_u32[j] = normaliser->defs[i].value[j];
            return;
        }
    }

    param->reg.type = VKD3DSPR_CONSTBUFFER;
    param->reg.idx[0].offset = set; /* register ID */
    param->reg.idx[0].rel_addr = NULL;
    param->reg.idx[1].offset = set; /* register index */
    param->reg.idx[1].rel_addr = NULL;
    param->reg.idx[2].offset = index; /* buffer index */
    param->reg.idx[2].rel_addr = rel_addr;
    param->reg.idx_count = 3;
}

static enum vkd3d_result vsir_program_normalise_flat_constants(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct flat_constants_normaliser normaliser = {0};
    unsigned int i, j;

    for (i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];

        if (ins->opcode == VKD3DSIH_DEF || ins->opcode == VKD3DSIH_DEFI || ins->opcode == VKD3DSIH_DEFB)
        {
            struct flat_constant_def *def;

            if (!vkd3d_array_reserve((void **)&normaliser.defs, &normaliser.defs_capacity,
                    normaliser.def_count + 1, sizeof(*normaliser.defs)))
            {
                vkd3d_free(normaliser.defs);
                return VKD3D_ERROR_OUT_OF_MEMORY;
            }

            def = &normaliser.defs[normaliser.def_count++];

            get_flat_constant_register_type(&ins->dst[0].reg, &def->set, &def->index, NULL);
            for (j = 0; j < 4; ++j)
                def->value[j] = ins->src[0].reg.u.immconst_u32[j];

            vkd3d_shader_instruction_make_nop(ins);
        }
        else
        {
            for (j = 0; j < ins->src_count; ++j)
                shader_register_normalise_flat_constants(&ins->src[j], &normaliser);
        }
    }

    vkd3d_free(normaliser.defs);
    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_remove_dead_code(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    size_t i, depth = 0;
    bool dead = false;

    for (i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];

        switch (ins->opcode)
        {
            case VKD3DSIH_IF:
            case VKD3DSIH_LOOP:
            case VKD3DSIH_SWITCH:
                if (dead)
                {
                    vkd3d_shader_instruction_make_nop(ins);
                    ++depth;
                }
                break;

            case VKD3DSIH_ENDIF:
            case VKD3DSIH_ENDLOOP:
            case VKD3DSIH_ENDSWITCH:
            case VKD3DSIH_ELSE:
                if (dead)
                {
                    if (depth > 0)
                    {
                        if (ins->opcode != VKD3DSIH_ELSE)
                            --depth;
                        vkd3d_shader_instruction_make_nop(ins);
                    }
                    else
                    {
                        dead = false;
                    }
                }
                break;

            /* `depth' is counted with respect to where the dead code
             * segment began. So it starts at zero and it signals the
             * termination of the dead code segment when it would
             * become negative. */
            case VKD3DSIH_BREAK:
            case VKD3DSIH_RET:
            case VKD3DSIH_CONTINUE:
                if (dead)
                {
                    vkd3d_shader_instruction_make_nop(ins);
                }
                else
                {
                    dead = true;
                    depth = 0;
                }
                break;

            /* If `case' or `default' appears at zero depth, it means
             * that they are a possible target for the corresponding
             * switch, so the code is live again. */
            case VKD3DSIH_CASE:
            case VKD3DSIH_DEFAULT:
                if (dead)
                {
                    if (depth == 0)
                        dead = false;
                    else
                        vkd3d_shader_instruction_make_nop(ins);
                }
                break;

            /* Phase instructions can only appear in hull shaders and
             * outside of any block. When a phase returns, control is
             * moved to the following phase, so they make code live
             * again. */
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                dead = false;
                break;

            default:
                if (dead)
                    vkd3d_shader_instruction_make_nop(ins);
                break;
        }
    }

    return VKD3D_OK;
}

struct cf_flattener_if_info
{
    struct vkd3d_shader_src_param *false_param;
    unsigned int id;
    uint32_t merge_block_id;
    unsigned int else_block_id;
};

struct cf_flattener_loop_info
{
    unsigned int header_block_id;
    unsigned int continue_block_id;
    uint32_t merge_block_id;
};

struct cf_flattener_switch_case
{
    unsigned int value;
    unsigned int block_id;
};

struct cf_flattener_switch_info
{
    size_t ins_location;
    const struct vkd3d_shader_src_param *condition;
    unsigned int id;
    unsigned int merge_block_id;
    unsigned int default_block_id;
    struct cf_flattener_switch_case *cases;
    size_t cases_size;
    unsigned int cases_count;
};

struct cf_flattener_info
{
    union
    {
        struct cf_flattener_if_info if_;
        struct cf_flattener_loop_info loop;
        struct cf_flattener_switch_info switch_;
    } u;

    enum
    {
        VKD3D_BLOCK_IF,
        VKD3D_BLOCK_LOOP,
        VKD3D_BLOCK_SWITCH,
    } current_block;
    bool inside_block;
};

struct cf_flattener
{
    struct vsir_program *program;

    struct vkd3d_shader_location location;
    enum vkd3d_result status;

    struct vkd3d_shader_instruction *instructions;
    size_t instruction_capacity;
    size_t instruction_count;

    unsigned int block_id;
    const char **block_names;
    size_t block_name_capacity;
    size_t block_name_count;

    unsigned int branch_id;
    unsigned int loop_id;
    unsigned int switch_id;

    unsigned int control_flow_depth;
    struct cf_flattener_info *control_flow_info;
    size_t control_flow_info_size;
};

static void cf_flattener_set_error(struct cf_flattener *flattener, enum vkd3d_result error)
{
    if (flattener->status != VKD3D_OK)
        return;
    flattener->status = error;
}

static struct vkd3d_shader_instruction *cf_flattener_require_space(struct cf_flattener *flattener, size_t count)
{
    if (!vkd3d_array_reserve((void **)&flattener->instructions, &flattener->instruction_capacity,
            flattener->instruction_count + count, sizeof(*flattener->instructions)))
    {
        ERR("Failed to allocate instructions.\n");
        cf_flattener_set_error(flattener, VKD3D_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    return &flattener->instructions[flattener->instruction_count];
}

static bool cf_flattener_copy_instruction(struct cf_flattener *flattener,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_shader_instruction *dst_ins;

    if (instruction->opcode == VKD3DSIH_NOP)
        return true;

    if (!(dst_ins = cf_flattener_require_space(flattener, 1)))
        return false;

    *dst_ins = *instruction;
    ++flattener->instruction_count;
    return true;
}

static unsigned int cf_flattener_alloc_block_id(struct cf_flattener *flattener)
{
    return ++flattener->block_id;
}

static struct vkd3d_shader_src_param *instruction_src_params_alloc(struct vkd3d_shader_instruction *ins,
        unsigned int count, struct cf_flattener *flattener)
{
    struct vkd3d_shader_src_param *params;

    if (!(params = vsir_program_get_src_params(flattener->program, count)))
    {
        cf_flattener_set_error(flattener, VKD3D_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    ins->src = params;
    ins->src_count = count;
    return params;
}

static void cf_flattener_emit_label(struct cf_flattener *flattener, unsigned int label_id)
{
    struct vkd3d_shader_instruction *ins;

    if (!(ins = cf_flattener_require_space(flattener, 1)))
        return;
    if (vsir_instruction_init_label(ins, &flattener->location, label_id, flattener->program))
        ++flattener->instruction_count;
    else
        cf_flattener_set_error(flattener, VKD3D_ERROR_OUT_OF_MEMORY);
}

/* For conditional branches, this returns the false target branch parameter. */
static struct vkd3d_shader_src_param *cf_flattener_emit_branch(struct cf_flattener *flattener,
        unsigned int merge_block_id, unsigned int continue_block_id,
        const struct vkd3d_shader_src_param *condition, unsigned int true_id, unsigned int false_id,
        unsigned int flags)
{
    struct vkd3d_shader_src_param *src_params, *false_branch_param;
    struct vkd3d_shader_instruction *ins;

    if (!(ins = cf_flattener_require_space(flattener, 1)))
        return NULL;
    vsir_instruction_init(ins, &flattener->location, VKD3DSIH_BRANCH);

    if (condition)
    {
        if (!(src_params = instruction_src_params_alloc(ins, 4 + !!continue_block_id, flattener)))
            return NULL;
        src_params[0] = *condition;
        if (flags == VKD3D_SHADER_CONDITIONAL_OP_Z)
        {
            vsir_src_param_init_label(&src_params[1], false_id);
            vsir_src_param_init_label(&src_params[2], true_id);
            false_branch_param = &src_params[1];
        }
        else
        {
            vsir_src_param_init_label(&src_params[1], true_id);
            vsir_src_param_init_label(&src_params[2], false_id);
            false_branch_param = &src_params[2];
        }
        vsir_src_param_init_label(&src_params[3], merge_block_id);
        if (continue_block_id)
            vsir_src_param_init_label(&src_params[4], continue_block_id);
    }
    else
    {
        if (!(src_params = instruction_src_params_alloc(ins, merge_block_id ? 3 : 1, flattener)))
            return NULL;
        vsir_src_param_init_label(&src_params[0], true_id);
        if (merge_block_id)
        {
            /* An unconditional branch may only have merge information for a loop, which
             * must have both a merge block and continue block. */
            vsir_src_param_init_label(&src_params[1], merge_block_id);
            vsir_src_param_init_label(&src_params[2], continue_block_id);
        }
        false_branch_param = NULL;
    }

    ++flattener->instruction_count;

    return false_branch_param;
}

static void cf_flattener_emit_conditional_branch_and_merge(struct cf_flattener *flattener,
        const struct vkd3d_shader_src_param *condition, unsigned int true_id, unsigned int flags)
{
    unsigned int merge_block_id;

    merge_block_id = cf_flattener_alloc_block_id(flattener);
    cf_flattener_emit_branch(flattener, merge_block_id, 0, condition, true_id, merge_block_id, flags);
    cf_flattener_emit_label(flattener, merge_block_id);
}

static void cf_flattener_emit_unconditional_branch(struct cf_flattener *flattener, unsigned int target_block_id)
{
    cf_flattener_emit_branch(flattener, 0, 0, NULL, target_block_id, 0, 0);
}

static struct cf_flattener_info *cf_flattener_push_control_flow_level(struct cf_flattener *flattener)
{
    if (!vkd3d_array_reserve((void **)&flattener->control_flow_info, &flattener->control_flow_info_size,
            flattener->control_flow_depth + 1, sizeof(*flattener->control_flow_info)))
    {
        ERR("Failed to allocate control flow info structure.\n");
        cf_flattener_set_error(flattener, VKD3D_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    return &flattener->control_flow_info[flattener->control_flow_depth++];
}

static void cf_flattener_pop_control_flow_level(struct cf_flattener *flattener)
{
    struct cf_flattener_info *cf_info;

    cf_info = &flattener->control_flow_info[--flattener->control_flow_depth];
    memset(cf_info, 0, sizeof(*cf_info));
}

static struct cf_flattener_info *cf_flattener_find_innermost_loop(struct cf_flattener *flattener)
{
    int depth;

    for (depth = flattener->control_flow_depth - 1; depth >= 0; --depth)
    {
        if (flattener->control_flow_info[depth].current_block == VKD3D_BLOCK_LOOP)
            return &flattener->control_flow_info[depth];
    }

    return NULL;
}

static struct cf_flattener_info *cf_flattener_find_innermost_breakable_cf_construct(struct cf_flattener *flattener)
{
    int depth;

    for (depth = flattener->control_flow_depth - 1; depth >= 0; --depth)
    {
        if (flattener->control_flow_info[depth].current_block == VKD3D_BLOCK_LOOP
                || flattener->control_flow_info[depth].current_block == VKD3D_BLOCK_SWITCH)
            return &flattener->control_flow_info[depth];
    }

    return NULL;
}

static void VKD3D_PRINTF_FUNC(3, 4) cf_flattener_create_block_name(struct cf_flattener *flattener,
        unsigned int block_id, const char *fmt, ...)
{
    struct vkd3d_string_buffer buffer;
    size_t block_name_count;
    va_list args;

    --block_id;

    block_name_count = max(flattener->block_name_count, block_id + 1);
    if (!vkd3d_array_reserve((void **)&flattener->block_names, &flattener->block_name_capacity,
            block_name_count, sizeof(*flattener->block_names)))
        return;
    memset(&flattener->block_names[flattener->block_name_count], 0,
            (block_name_count - flattener->block_name_count) * sizeof(*flattener->block_names));
    flattener->block_name_count = block_name_count;

    vkd3d_string_buffer_init(&buffer);
    va_start(args, fmt);
    vkd3d_string_buffer_vprintf(&buffer, fmt, args);
    va_end(args);

    flattener->block_names[block_id] = buffer.buffer;
}

static enum vkd3d_result cf_flattener_iterate_instruction_array(struct cf_flattener *flattener,
        struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_instruction_array *instructions;
    struct vsir_program *program = flattener->program;
    bool is_hull_shader, after_declarations_section;
    struct vkd3d_shader_instruction *dst_ins;
    size_t i;

    instructions = &program->instructions;
    is_hull_shader = program->shader_version.type == VKD3D_SHADER_TYPE_HULL;
    after_declarations_section = is_hull_shader;

    if (!cf_flattener_require_space(flattener, instructions->count + 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    for (i = 0; i < instructions->count; ++i)
    {
        unsigned int loop_header_block_id, loop_body_block_id, continue_block_id, merge_block_id, true_block_id;
        const struct vkd3d_shader_instruction *instruction = &instructions->elements[i];
        const struct vkd3d_shader_src_param *src = instruction->src;
        struct cf_flattener_info *cf_info;

        flattener->location = instruction->location;

        /* Declarations should occur before the first code block, which in hull shaders is marked by the first
         * phase instruction, and in all other shader types begins with the first label instruction.
         * Declaring an indexable temp with function scope is not considered a declaration,
         * because it needs to live inside a function. */
        if (!after_declarations_section && instruction->opcode != VKD3DSIH_NOP)
        {
            bool is_function_indexable = instruction->opcode == VKD3DSIH_DCL_INDEXABLE_TEMP
                    && instruction->declaration.indexable_temp.has_function_scope;

            if (!vsir_instruction_is_dcl(instruction) || is_function_indexable)
            {
                after_declarations_section = true;
                cf_flattener_emit_label(flattener, cf_flattener_alloc_block_id(flattener));
            }
        }

        cf_info = flattener->control_flow_depth
                ? &flattener->control_flow_info[flattener->control_flow_depth - 1] : NULL;

        switch (instruction->opcode)
        {
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                if (!cf_flattener_copy_instruction(flattener, instruction))
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                if (instruction->opcode != VKD3DSIH_HS_CONTROL_POINT_PHASE || !instruction->flags)
                    after_declarations_section = false;
                break;

            case VKD3DSIH_LABEL:
                vkd3d_shader_error(message_context, &instruction->location,
                        VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                        "Aborting due to not yet implemented feature: Label instruction.");
                return VKD3D_ERROR_NOT_IMPLEMENTED;

            case VKD3DSIH_IF:
                if (!(cf_info = cf_flattener_push_control_flow_level(flattener)))
                    return VKD3D_ERROR_OUT_OF_MEMORY;

                true_block_id = cf_flattener_alloc_block_id(flattener);
                merge_block_id = cf_flattener_alloc_block_id(flattener);
                cf_info->u.if_.false_param = cf_flattener_emit_branch(flattener, merge_block_id, 0,
                        src, true_block_id, merge_block_id, instruction->flags);
                if (!cf_info->u.if_.false_param)
                    return VKD3D_ERROR_OUT_OF_MEMORY;

                cf_flattener_emit_label(flattener, true_block_id);

                cf_info->u.if_.id = flattener->branch_id;
                cf_info->u.if_.merge_block_id = merge_block_id;
                cf_info->u.if_.else_block_id = 0;
                cf_info->inside_block = true;
                cf_info->current_block = VKD3D_BLOCK_IF;

                cf_flattener_create_block_name(flattener, merge_block_id, "branch%u_merge", flattener->branch_id);
                cf_flattener_create_block_name(flattener, true_block_id, "branch%u_true", flattener->branch_id);
                ++flattener->branch_id;
                break;

            case VKD3DSIH_ELSE:
                if (cf_info->inside_block)
                    cf_flattener_emit_unconditional_branch(flattener, cf_info->u.if_.merge_block_id);

                cf_info->u.if_.else_block_id = cf_flattener_alloc_block_id(flattener);
                cf_info->u.if_.false_param->reg.idx[0].offset = cf_info->u.if_.else_block_id;

                cf_flattener_create_block_name(flattener,
                        cf_info->u.if_.else_block_id, "branch%u_false", cf_info->u.if_.id);
                cf_flattener_emit_label(flattener, cf_info->u.if_.else_block_id);

                cf_info->inside_block = true;
                break;

            case VKD3DSIH_ENDIF:
                if (cf_info->inside_block)
                    cf_flattener_emit_unconditional_branch(flattener, cf_info->u.if_.merge_block_id);

                cf_flattener_emit_label(flattener, cf_info->u.if_.merge_block_id);

                cf_flattener_pop_control_flow_level(flattener);
                break;

            case VKD3DSIH_LOOP:
                if (!(cf_info = cf_flattener_push_control_flow_level(flattener)))
                    return VKD3D_ERROR_OUT_OF_MEMORY;

                loop_header_block_id = cf_flattener_alloc_block_id(flattener);
                loop_body_block_id = cf_flattener_alloc_block_id(flattener);
                continue_block_id = cf_flattener_alloc_block_id(flattener);
                merge_block_id = cf_flattener_alloc_block_id(flattener);

                cf_flattener_emit_unconditional_branch(flattener, loop_header_block_id);
                cf_flattener_emit_label(flattener, loop_header_block_id);
                cf_flattener_emit_branch(flattener, merge_block_id, continue_block_id,
                        NULL, loop_body_block_id, 0, 0);

                cf_flattener_emit_label(flattener, loop_body_block_id);

                cf_info->u.loop.header_block_id = loop_header_block_id;
                cf_info->u.loop.continue_block_id = continue_block_id;
                cf_info->u.loop.merge_block_id = merge_block_id;
                cf_info->current_block = VKD3D_BLOCK_LOOP;
                cf_info->inside_block = true;

                cf_flattener_create_block_name(flattener, loop_header_block_id, "loop%u_header", flattener->loop_id);
                cf_flattener_create_block_name(flattener, loop_body_block_id, "loop%u_body", flattener->loop_id);
                cf_flattener_create_block_name(flattener, continue_block_id, "loop%u_continue", flattener->loop_id);
                cf_flattener_create_block_name(flattener, merge_block_id, "loop%u_merge", flattener->loop_id);
                ++flattener->loop_id;
                break;

            case VKD3DSIH_ENDLOOP:
                if (cf_info->inside_block)
                    cf_flattener_emit_unconditional_branch(flattener, cf_info->u.loop.continue_block_id);

                cf_flattener_emit_label(flattener, cf_info->u.loop.continue_block_id);
                cf_flattener_emit_unconditional_branch(flattener, cf_info->u.loop.header_block_id);
                cf_flattener_emit_label(flattener, cf_info->u.loop.merge_block_id);

                cf_flattener_pop_control_flow_level(flattener);
                break;

            case VKD3DSIH_SWITCH:
                if (!(cf_info = cf_flattener_push_control_flow_level(flattener)))
                    return VKD3D_ERROR_OUT_OF_MEMORY;

                merge_block_id = cf_flattener_alloc_block_id(flattener);

                cf_info->u.switch_.ins_location = flattener->instruction_count;
                cf_info->u.switch_.condition = src;

                if (!(dst_ins = cf_flattener_require_space(flattener, 1)))
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                vsir_instruction_init(dst_ins, &instruction->location, VKD3DSIH_SWITCH_MONOLITHIC);
                ++flattener->instruction_count;

                cf_info->u.switch_.id = flattener->switch_id;
                cf_info->u.switch_.merge_block_id = merge_block_id;
                cf_info->u.switch_.cases = NULL;
                cf_info->u.switch_.cases_size = 0;
                cf_info->u.switch_.cases_count = 0;
                cf_info->u.switch_.default_block_id = 0;
                cf_info->inside_block = false;
                cf_info->current_block = VKD3D_BLOCK_SWITCH;

                cf_flattener_create_block_name(flattener, merge_block_id, "switch%u_merge", flattener->switch_id);
                ++flattener->switch_id;

                if (!vkd3d_array_reserve((void **)&cf_info->u.switch_.cases, &cf_info->u.switch_.cases_size,
                        10, sizeof(*cf_info->u.switch_.cases)))
                    return VKD3D_ERROR_OUT_OF_MEMORY;

                break;

            case VKD3DSIH_ENDSWITCH:
            {
                struct vkd3d_shader_src_param *src_params;
                unsigned int j;

                if (!cf_info->u.switch_.default_block_id)
                    cf_info->u.switch_.default_block_id = cf_info->u.switch_.merge_block_id;

                cf_flattener_emit_label(flattener, cf_info->u.switch_.merge_block_id);

                /* The SWITCH instruction is completed when the endswitch
                 * instruction is processed because we do not know the number
                 * of case statements or the default block id in advance.*/
                dst_ins = &flattener->instructions[cf_info->u.switch_.ins_location];
                if (!(src_params = instruction_src_params_alloc(dst_ins, cf_info->u.switch_.cases_count * 2 + 3, flattener)))
                {
                    vkd3d_free(cf_info->u.switch_.cases);
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                }
                src_params[0] = *cf_info->u.switch_.condition;
                vsir_src_param_init_label(&src_params[1], cf_info->u.switch_.default_block_id);
                vsir_src_param_init_label(&src_params[2], cf_info->u.switch_.merge_block_id);
                for (j = 0; j < cf_info->u.switch_.cases_count; ++j)
                {
                    unsigned int index = j * 2 + 3;
                    vsir_src_param_init(&src_params[index], VKD3DSPR_IMMCONST, VKD3D_DATA_UINT, 0);
                    src_params[index].reg.u.immconst_u32[0] = cf_info->u.switch_.cases[j].value;
                    vsir_src_param_init_label(&src_params[index + 1], cf_info->u.switch_.cases[j].block_id);
                }
                vkd3d_free(cf_info->u.switch_.cases);

                cf_flattener_pop_control_flow_level(flattener);
                break;
            }

            case VKD3DSIH_CASE:
            {
                unsigned int label_id, value;

                if (src->swizzle != VKD3D_SHADER_SWIZZLE(X, X, X, X))
                {
                    WARN("Unexpected src swizzle %#x.\n", src->swizzle);
                    vkd3d_shader_error(message_context, &instruction->location,
                            VKD3D_SHADER_ERROR_VSIR_INVALID_SWIZZLE,
                            "The swizzle for a switch case value is not scalar X.");
                    cf_flattener_set_error(flattener, VKD3D_ERROR_INVALID_SHADER);
                }
                value = *src->reg.u.immconst_u32;

                if (!vkd3d_array_reserve((void **)&cf_info->u.switch_.cases, &cf_info->u.switch_.cases_size,
                        cf_info->u.switch_.cases_count + 1, sizeof(*cf_info->u.switch_.cases)))
                    return VKD3D_ERROR_OUT_OF_MEMORY;

                label_id = cf_flattener_alloc_block_id(flattener);
                if (cf_info->inside_block) /* fall-through */
                    cf_flattener_emit_unconditional_branch(flattener, label_id);

                cf_info->u.switch_.cases[cf_info->u.switch_.cases_count].value = value;
                cf_info->u.switch_.cases[cf_info->u.switch_.cases_count].block_id = label_id;
                ++cf_info->u.switch_.cases_count;

                cf_flattener_emit_label(flattener, label_id);
                cf_flattener_create_block_name(flattener, label_id, "switch%u_case%u", cf_info->u.switch_.id, value);
                cf_info->inside_block = true;
                break;
            }

            case VKD3DSIH_DEFAULT:
                cf_info->u.switch_.default_block_id = cf_flattener_alloc_block_id(flattener);
                if (cf_info->inside_block) /* fall-through */
                    cf_flattener_emit_unconditional_branch(flattener, cf_info->u.switch_.default_block_id);

                cf_flattener_emit_label(flattener, cf_info->u.switch_.default_block_id);

                cf_flattener_create_block_name(flattener, cf_info->u.switch_.default_block_id,
                        "switch%u_default", cf_info->u.switch_.id);
                cf_info->inside_block = true;
                break;

            case VKD3DSIH_BREAK:
            {
                struct cf_flattener_info *breakable_cf_info;

                if (!(breakable_cf_info = cf_flattener_find_innermost_breakable_cf_construct(flattener)))
                {
                    FIXME("Unhandled break instruction.\n");
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                if (breakable_cf_info->current_block == VKD3D_BLOCK_LOOP)
                {
                    cf_flattener_emit_unconditional_branch(flattener, breakable_cf_info->u.loop.merge_block_id);
                }
                else if (breakable_cf_info->current_block == VKD3D_BLOCK_SWITCH)
                {
                    cf_flattener_emit_unconditional_branch(flattener, breakable_cf_info->u.switch_.merge_block_id);
                }

                cf_info->inside_block = false;
                break;
            }

            case VKD3DSIH_BREAKP:
            {
                struct cf_flattener_info *loop_cf_info;

                if (!(loop_cf_info = cf_flattener_find_innermost_loop(flattener)))
                {
                    ERR("Invalid 'breakc' instruction outside loop.\n");
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                cf_flattener_emit_conditional_branch_and_merge(flattener,
                        src, loop_cf_info->u.loop.merge_block_id, instruction->flags);
                break;
            }

            case VKD3DSIH_CONTINUE:
            {
                struct cf_flattener_info *loop_cf_info;

                if (!(loop_cf_info = cf_flattener_find_innermost_loop(flattener)))
                {
                    ERR("Invalid 'continue' instruction outside loop.\n");
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                cf_flattener_emit_unconditional_branch(flattener, loop_cf_info->u.loop.continue_block_id);

                cf_info->inside_block = false;
                break;
            }

            case VKD3DSIH_CONTINUEP:
            {
                struct cf_flattener_info *loop_cf_info;

                if (!(loop_cf_info = cf_flattener_find_innermost_loop(flattener)))
                {
                    ERR("Invalid 'continuec' instruction outside loop.\n");
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                cf_flattener_emit_conditional_branch_and_merge(flattener,
                        src, loop_cf_info->u.loop.continue_block_id, instruction->flags);
                break;
            }

            case VKD3DSIH_RET:
                if (!cf_flattener_copy_instruction(flattener, instruction))
                    return VKD3D_ERROR_OUT_OF_MEMORY;

                if (cf_info)
                    cf_info->inside_block = false;
                break;

            default:
                if (!cf_flattener_copy_instruction(flattener, instruction))
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                break;
        }
    }

    return flattener->status;
}

static enum vkd3d_result vsir_program_flatten_control_flow_constructs(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_message_context *message_context = ctx->message_context;
    struct cf_flattener flattener = {.program = program};
    enum vkd3d_result result;

    VKD3D_ASSERT(program->cf_type == VSIR_CF_STRUCTURED);

    if ((result = cf_flattener_iterate_instruction_array(&flattener, message_context)) >= 0)
    {
        vkd3d_free(program->instructions.elements);
        program->instructions.elements = flattener.instructions;
        program->instructions.capacity = flattener.instruction_capacity;
        program->instructions.count = flattener.instruction_count;
        program->block_count = flattener.block_id;
        program->cf_type = VSIR_CF_BLOCKS;
    }
    else
    {
        vkd3d_free(flattener.instructions);
    }

    vkd3d_free(flattener.control_flow_info);
    /* Simpler to always free these in vsir_program_cleanup(). */
    program->block_names = flattener.block_names;
    program->block_name_count = flattener.block_name_count;

    return result;
}

static unsigned int label_from_src_param(const struct vkd3d_shader_src_param *param)
{
    VKD3D_ASSERT(param->reg.type == VKD3DSPR_LABEL);
    return param->reg.idx[0].offset;
}

static bool reserve_instructions(struct vkd3d_shader_instruction **instructions, size_t *capacity, size_t count)
{
    if (!vkd3d_array_reserve((void **)instructions, capacity, count, sizeof(**instructions)))
    {
        ERR("Failed to allocate instructions.\n");
        return false;
    }

    return true;
}

/* A record represents replacing a jump from block `switch_label' to
 * block `target_label' with a jump from block `if_label' to block
 * `target_label'. */
struct lower_switch_to_if_ladder_block_mapping
{
    unsigned int switch_label;
    unsigned int if_label;
    unsigned int target_label;
};

static bool lower_switch_to_if_ladder_add_block_mapping(struct lower_switch_to_if_ladder_block_mapping **block_map,
        size_t *map_capacity, size_t *map_count, unsigned int switch_label, unsigned int if_label, unsigned int target_label)
{
    if (!vkd3d_array_reserve((void **)block_map, map_capacity, *map_count + 1, sizeof(**block_map)))
    {
        ERR("Failed to allocate block mapping.\n");
        return false;
    }

    (*block_map)[*map_count].switch_label = switch_label;
    (*block_map)[*map_count].if_label = if_label;
    (*block_map)[*map_count].target_label = target_label;

    *map_count += 1;

    return true;
}

static enum vkd3d_result vsir_program_lower_switch_to_selection_ladder(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    unsigned int block_count = program->block_count, ssa_count = program->ssa_count, current_label = 0, if_label;
    size_t ins_capacity = 0, ins_count = 0, i, map_capacity = 0, map_count = 0;
    struct vkd3d_shader_instruction *instructions = NULL;
    struct lower_switch_to_if_ladder_block_mapping *block_map = NULL;

    VKD3D_ASSERT(program->cf_type == VSIR_CF_BLOCKS);

    if (!reserve_instructions(&instructions, &ins_capacity, program->instructions.count))
        goto fail;

    /* First subpass: convert SWITCH_MONOLITHIC instructions to
     * selection ladders, keeping a map between blocks before and
     * after the subpass. */
    for (i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];
        unsigned int case_count, j, default_label;

        switch (ins->opcode)
        {
            case VKD3DSIH_LABEL:
                current_label = label_from_src_param(&ins->src[0]);
                if (!reserve_instructions(&instructions, &ins_capacity, ins_count + 1))
                    goto fail;
                instructions[ins_count++] = *ins;
                continue;

            case VKD3DSIH_SWITCH_MONOLITHIC:
                break;

            default:
                if (!reserve_instructions(&instructions, &ins_capacity, ins_count + 1))
                    goto fail;
                instructions[ins_count++] = *ins;
                continue;
        }

        case_count = (ins->src_count - 3) / 2;
        default_label = label_from_src_param(&ins->src[1]);

        /* In principle we can have a switch with no cases, and we
         * just have to jump to the default label. */
        if (case_count == 0)
        {
            if (!reserve_instructions(&instructions, &ins_capacity, ins_count + 1))
                goto fail;

            if (!vsir_instruction_init_with_params(program, &instructions[ins_count],
                    &ins->location, VKD3DSIH_BRANCH, 0, 1))
                goto fail;
            vsir_src_param_init_label(&instructions[ins_count].src[0], default_label);
            ++ins_count;
        }

        if (!reserve_instructions(&instructions, &ins_capacity, ins_count + 3 * case_count - 1))
            goto fail;

        if_label = current_label;

        for (j = 0; j < case_count; ++j)
        {
            unsigned int fallthrough_label, case_label = label_from_src_param(&ins->src[3 + 2 * j + 1]);

            if (!vsir_instruction_init_with_params(program,
                    &instructions[ins_count], &ins->location, VKD3DSIH_IEQ, 1, 2))
                goto fail;
            dst_param_init_ssa_bool(&instructions[ins_count].dst[0], ssa_count);
            instructions[ins_count].src[0] = ins->src[0];
            instructions[ins_count].src[1] = ins->src[3 + 2 * j];
            ++ins_count;

            /* For all cases except the last one we fall through to
             * the following case; the last one has to jump to the
             * default label. */
            if (j == case_count - 1)
                fallthrough_label = default_label;
            else
                fallthrough_label = block_count + 1;

            if (!vsir_instruction_init_with_params(program, &instructions[ins_count],
                    &ins->location, VKD3DSIH_BRANCH, 0, 3))
                goto fail;
            src_param_init_ssa_bool(&instructions[ins_count].src[0], ssa_count);
            vsir_src_param_init_label(&instructions[ins_count].src[1], case_label);
            vsir_src_param_init_label(&instructions[ins_count].src[2], fallthrough_label);
            ++ins_count;

            ++ssa_count;

            if (!lower_switch_to_if_ladder_add_block_mapping(&block_map, &map_capacity, &map_count,
                    current_label, if_label, case_label))
                goto fail;

            if (j == case_count - 1)
            {
                if (!lower_switch_to_if_ladder_add_block_mapping(&block_map, &map_capacity, &map_count,
                        current_label, if_label, default_label))
                    goto fail;
            }
            else
            {
                if (!vsir_instruction_init_with_params(program,
                        &instructions[ins_count], &ins->location, VKD3DSIH_LABEL, 0, 1))
                    goto fail;
                vsir_src_param_init_label(&instructions[ins_count].src[0], ++block_count);
                ++ins_count;

                if_label = block_count;
            }
        }
    }

    vkd3d_free(program->instructions.elements);
    vkd3d_free(block_map);
    program->instructions.elements = instructions;
    program->instructions.capacity = ins_capacity;
    program->instructions.count = ins_count;
    program->block_count = block_count;
    program->ssa_count = ssa_count;

    return VKD3D_OK;

fail:
    vkd3d_free(instructions);
    vkd3d_free(block_map);

    return VKD3D_ERROR_OUT_OF_MEMORY;
}

struct ssas_to_temps_alloc
{
    unsigned int *table;
    unsigned int next_temp_idx;
};

static bool ssas_to_temps_alloc_init(struct ssas_to_temps_alloc *alloc, unsigned int ssa_count, unsigned int temp_count)
{
    size_t i = ssa_count * sizeof(*alloc->table);

    if (!(alloc->table = vkd3d_malloc(i)))
    {
        ERR("Failed to allocate SSA table.\n");
        return false;
    }
    memset(alloc->table, 0xff, i);

    alloc->next_temp_idx = temp_count;
    return true;
}

/* This is idempotent: it can be safely applied more than once on the
 * same register. */
static void materialize_ssas_to_temps_process_reg(struct vsir_program *program, struct ssas_to_temps_alloc *alloc,
        struct vkd3d_shader_register *reg)
{
    unsigned int i;

    if (reg->type == VKD3DSPR_SSA && alloc->table[reg->idx[0].offset] != UINT_MAX)
    {
        reg->type = VKD3DSPR_TEMP;
        reg->idx[0].offset = alloc->table[reg->idx[0].offset];
    }

    for (i = 0; i < reg->idx_count; ++i)
        if (reg->idx[i].rel_addr)
            materialize_ssas_to_temps_process_reg(program, alloc, &reg->idx[i].rel_addr->reg);
}

struct ssas_to_temps_block_info
{
    struct phi_incoming_to_temp
    {
        struct vkd3d_shader_src_param *src;
        struct vkd3d_shader_dst_param *dst;
    } *incomings;
    size_t incoming_capacity;
    size_t incoming_count;
};

static void ssas_to_temps_block_info_cleanup(struct ssas_to_temps_block_info *block_info,
        size_t count)
{
    size_t i;

    for (i = 0; i < count; ++i)
        vkd3d_free(block_info[i].incomings);

    vkd3d_free(block_info);
}

static enum vkd3d_result vsir_program_materialise_phi_ssas_to_temps(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    size_t ins_capacity = 0, ins_count = 0, phi_count, incoming_count, i;
    struct ssas_to_temps_block_info *info, *block_info = NULL;
    struct vkd3d_shader_instruction *instructions = NULL;
    struct ssas_to_temps_alloc alloc = {0};
    unsigned int current_label = 0;

    VKD3D_ASSERT(program->cf_type == VSIR_CF_BLOCKS);

    if (!(block_info = vkd3d_calloc(program->block_count, sizeof(*block_info))))
    {
        ERR("Failed to allocate block info array.\n");
        goto fail;
    }

    if (!ssas_to_temps_alloc_init(&alloc, program->ssa_count, program->temp_count))
        goto fail;

    for (i = 0, phi_count = 0, incoming_count = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];
        unsigned int j, temp_idx;

        /* Only phi src/dst SSA values need be converted here. Structurisation may
         * introduce new cases of undominated SSA use, which will be handled later. */
        if (ins->opcode != VKD3DSIH_PHI)
            continue;
        ++phi_count;

        temp_idx = alloc.next_temp_idx++;

        for (j = 0; j < ins->src_count; j += 2)
        {
            struct phi_incoming_to_temp *incoming;
            unsigned int label;

            label = label_from_src_param(&ins->src[j + 1]);
            VKD3D_ASSERT(label);

            info = &block_info[label - 1];

            if (!(vkd3d_array_reserve((void **)&info->incomings, &info->incoming_capacity, info->incoming_count + 1,
                    sizeof(*info->incomings))))
                goto fail;

            incoming = &info->incomings[info->incoming_count++];
            incoming->src = &ins->src[j];
            incoming->dst = ins->dst;

            alloc.table[ins->dst->reg.idx[0].offset] = temp_idx;

            ++incoming_count;
        }

        materialize_ssas_to_temps_process_reg(program, &alloc, &ins->dst->reg);
    }

    if (!phi_count)
        goto done;

    if (!reserve_instructions(&instructions, &ins_capacity, program->instructions.count + incoming_count - phi_count))
        goto fail;

    for (i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *mov_ins, *ins = &program->instructions.elements[i];
        size_t j;

        for (j = 0; j < ins->dst_count; ++j)
            materialize_ssas_to_temps_process_reg(program, &alloc, &ins->dst[j].reg);

        for (j = 0; j < ins->src_count; ++j)
            materialize_ssas_to_temps_process_reg(program, &alloc, &ins->src[j].reg);

        switch (ins->opcode)
        {
            case VKD3DSIH_LABEL:
                current_label = label_from_src_param(&ins->src[0]);
                break;

            case VKD3DSIH_BRANCH:
            case VKD3DSIH_SWITCH_MONOLITHIC:
                info = &block_info[current_label - 1];

                for (j = 0; j < info->incoming_count; ++j)
                {
                    struct phi_incoming_to_temp *incoming = &info->incomings[j];

                    mov_ins = &instructions[ins_count++];
                    if (!vsir_instruction_init_with_params(program, mov_ins, &ins->location, VKD3DSIH_MOV, 1, 0))
                        goto fail;
                    *mov_ins->dst = *incoming->dst;
                    mov_ins->src = incoming->src;
                    mov_ins->src_count = 1;
                }
                break;

            case VKD3DSIH_PHI:
                continue;

            default:
                break;
        }

        instructions[ins_count++] = *ins;
    }

    vkd3d_free(program->instructions.elements);
    program->instructions.elements = instructions;
    program->instructions.capacity = ins_capacity;
    program->instructions.count = ins_count;
    program->temp_count = alloc.next_temp_idx;
done:
    ssas_to_temps_block_info_cleanup(block_info, program->block_count);
    vkd3d_free(alloc.table);

    return VKD3D_OK;

fail:
    vkd3d_free(instructions);
    ssas_to_temps_block_info_cleanup(block_info, program->block_count);
    vkd3d_free(alloc.table);

    return VKD3D_ERROR_OUT_OF_MEMORY;
}

struct vsir_block_list
{
    struct vsir_block **blocks;
    size_t count, capacity;
};

static void vsir_block_list_init(struct vsir_block_list *list)
{
    memset(list, 0, sizeof(*list));
}

static void vsir_block_list_cleanup(struct vsir_block_list *list)
{
    vkd3d_free(list->blocks);
}

static enum vkd3d_result vsir_block_list_add_checked(struct vsir_block_list *list, struct vsir_block *block)
{
    if (!vkd3d_array_reserve((void **)&list->blocks, &list->capacity, list->count + 1, sizeof(*list->blocks)))
    {
        ERR("Cannot extend block list.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    list->blocks[list->count++] = block;

    return VKD3D_OK;
}

static enum vkd3d_result vsir_block_list_add(struct vsir_block_list *list, struct vsir_block *block)
{
    size_t i;

    for (i = 0; i < list->count; ++i)
        if (block == list->blocks[i])
            return VKD3D_FALSE;

    return vsir_block_list_add_checked(list, block);
}

/* It is guaranteed that the relative order is kept. */
static void vsir_block_list_remove_index(struct vsir_block_list *list, size_t idx)
{
    --list->count;
    memmove(&list->blocks[idx], &list->blocks[idx + 1], (list->count - idx) * sizeof(*list->blocks));
}

struct vsir_block
{
    unsigned int label, order_pos;
    /* `begin' points to the instruction immediately following the
     * LABEL that introduces the block. `end' points to the terminator
     * instruction (either BRANCH or RET). They can coincide, meaning
     * that the block is empty. */
    struct vkd3d_shader_instruction *begin, *end;
    struct vsir_block_list predecessors, successors;
    uint32_t *dominates;
};

static enum vkd3d_result vsir_block_init(struct vsir_block *block, unsigned int label, size_t block_count)
{
    size_t byte_count;

    if (block_count > SIZE_MAX - (sizeof(*block->dominates) * CHAR_BIT - 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    byte_count = VKD3D_BITMAP_SIZE(block_count) * sizeof(*block->dominates);

    VKD3D_ASSERT(label);
    memset(block, 0, sizeof(*block));
    block->label = label;
    vsir_block_list_init(&block->predecessors);
    vsir_block_list_init(&block->successors);

    if (!(block->dominates = vkd3d_malloc(byte_count)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    memset(block->dominates, 0xff, byte_count);

    return VKD3D_OK;
}

static void vsir_block_cleanup(struct vsir_block *block)
{
    if (block->label == 0)
        return;
    vsir_block_list_cleanup(&block->predecessors);
    vsir_block_list_cleanup(&block->successors);
    vkd3d_free(block->dominates);
}

static int block_compare(const void *ptr1, const void *ptr2)
{
    const struct vsir_block *block1 = *(const struct vsir_block **)ptr1;
    const struct vsir_block *block2 = *(const struct vsir_block **)ptr2;

    return vkd3d_u32_compare(block1->label, block2->label);
}

static void vsir_block_list_sort(struct vsir_block_list *list)
{
    qsort(list->blocks, list->count, sizeof(*list->blocks), block_compare);
}

static bool vsir_block_list_search(struct vsir_block_list *list, struct vsir_block *block)
{
    return !!bsearch(&block, list->blocks, list->count, sizeof(*list->blocks), block_compare);
}

struct vsir_cfg_structure_list
{
    struct vsir_cfg_structure *structures;
    size_t count, capacity;
    unsigned int end;
};

struct vsir_cfg_structure
{
    enum vsir_cfg_structure_type
    {
        /* Execute a block of the original VSIR program. */
        STRUCTURE_TYPE_BLOCK,
        /* Execute a loop, which is identified by an index. */
        STRUCTURE_TYPE_LOOP,
        /* Execute a selection construct. */
        STRUCTURE_TYPE_SELECTION,
        /* Execute a `return' or a (possibly) multilevel `break' or
         * `continue', targeting a loop by its index. If `condition'
         * is non-NULL, then the jump is conditional (this is
         * currently not allowed for `return'). */
        STRUCTURE_TYPE_JUMP,
    } type;
    union
    {
        struct vsir_block *block;
        struct vsir_cfg_structure_loop
        {
            struct vsir_cfg_structure_list body;
            unsigned idx;
            bool needs_trampoline;
            struct vsir_cfg_structure *outer_loop;
        } loop;
        struct vsir_cfg_structure_selection
        {
            struct vkd3d_shader_src_param *condition;
            struct vsir_cfg_structure_list if_body;
            struct vsir_cfg_structure_list else_body;
            bool invert_condition;
        } selection;
        struct vsir_cfg_structure_jump
        {
            enum vsir_cfg_jump_type
            {
                /* NONE is available as an intermediate value, but it
                 * is not allowed in valid structured programs. */
                JUMP_NONE,
                JUMP_BREAK,
                JUMP_CONTINUE,
                JUMP_RET,
            } type;
            unsigned int target;
            struct vkd3d_shader_src_param *condition;
            bool invert_condition;
            bool needs_launcher;
        } jump;
    } u;
};

static void vsir_cfg_structure_init(struct vsir_cfg_structure *structure, enum vsir_cfg_structure_type type);
static void vsir_cfg_structure_cleanup(struct vsir_cfg_structure *structure);

static void vsir_cfg_structure_list_cleanup(struct vsir_cfg_structure_list *list)
{
    unsigned int i;

    for (i = 0; i < list->count; ++i)
        vsir_cfg_structure_cleanup(&list->structures[i]);
    vkd3d_free(list->structures);
}

static struct vsir_cfg_structure *vsir_cfg_structure_list_append(struct vsir_cfg_structure_list *list,
        enum vsir_cfg_structure_type type)
{
    struct vsir_cfg_structure *ret;

    if (!vkd3d_array_reserve((void **)&list->structures, &list->capacity, list->count + 1,
            sizeof(*list->structures)))
        return NULL;

    ret = &list->structures[list->count++];

    vsir_cfg_structure_init(ret, type);

    return ret;
}

static enum vkd3d_result vsir_cfg_structure_list_append_from_region(struct vsir_cfg_structure_list *list,
        struct vsir_cfg_structure *begin, size_t size)
{
    if (!vkd3d_array_reserve((void **)&list->structures, &list->capacity, list->count + size,
            sizeof(*list->structures)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if (size)
        memcpy(&list->structures[list->count], begin, size * sizeof(*begin));

    list->count += size;

    return VKD3D_OK;
}

static void vsir_cfg_structure_init(struct vsir_cfg_structure *structure, enum vsir_cfg_structure_type type)
{
    memset(structure, 0, sizeof(*structure));
    structure->type = type;
}

static void vsir_cfg_structure_cleanup(struct vsir_cfg_structure *structure)
{
    switch (structure->type)
    {
        case STRUCTURE_TYPE_LOOP:
            vsir_cfg_structure_list_cleanup(&structure->u.loop.body);
            break;

        case STRUCTURE_TYPE_SELECTION:
            vsir_cfg_structure_list_cleanup(&structure->u.selection.if_body);
            vsir_cfg_structure_list_cleanup(&structure->u.selection.else_body);
            break;

        default:
            break;
    }
}

struct vsir_cfg_emit_target
{
    struct vkd3d_shader_instruction *instructions;
    size_t ins_capacity, ins_count;
    unsigned int jump_target_temp_idx;
    unsigned int temp_count;
};

struct vsir_cfg
{
    struct vkd3d_shader_message_context *message_context;
    struct vsir_program *program;
    size_t function_begin;
    size_t function_end;
    struct vsir_block *blocks;
    struct vsir_block *entry;
    size_t block_count;
    struct vkd3d_string_buffer debug_buffer;

    struct vsir_block_list *loops;
    size_t loops_count, loops_capacity;
    size_t *loops_by_header;

    struct vsir_block_list order;
    struct cfg_loop_interval
    {
        /* `begin' is the position of the first block of the loop in
         * the topological sort; `end' is the position of the first
         * block after the loop. In other words, `begin' is where a
         * `continue' instruction would jump and `end' is where a
         * `break' instruction would jump. */
        unsigned int begin, end;
        /* Each loop interval can be natural or synthetic. Natural
         * intervals are added to represent loops given by CFG back
         * edges. Synthetic intervals do not correspond to loops in
         * the input CFG, but are added to leverage their `break'
         * instruction in order to execute forward edges.
         *
         * For a synthetic loop interval it's not really important
         * which one is the `begin' block, since we don't need to
         * execute `continue' for them. So we have some leeway for
         * moving it provided that these conditions are met: 1. the
         * interval must contain all `break' instructions that target
         * it, which in practice means that `begin' can be moved
         * backward and not forward; 2. intervals must remain properly
         * nested (for each pair of intervals, either one contains the
         * other or they are disjoint).
         *
         * Subject to these conditions, we try to reuse the same loop
         * as much as possible (if many forward edges target the same
         * block), but we still try to keep `begin' as forward as
         * possible, to keep the loop scope as small as possible. */
        bool synthetic;
        /* The number of jump instructions (both conditional and
         * unconditional) that target this loop. */
        unsigned int target_count;
    } *loop_intervals;
    size_t loop_interval_count, loop_interval_capacity;

    struct vsir_cfg_structure_list structured_program;

    struct vsir_cfg_emit_target *target;
};

static void vsir_cfg_cleanup(struct vsir_cfg *cfg)
{
    size_t i;

    for (i = 0; i < cfg->block_count; ++i)
        vsir_block_cleanup(&cfg->blocks[i]);

    for (i = 0; i < cfg->loops_count; ++i)
        vsir_block_list_cleanup(&cfg->loops[i]);

    vsir_block_list_cleanup(&cfg->order);

    vsir_cfg_structure_list_cleanup(&cfg->structured_program);

    vkd3d_free(cfg->blocks);
    vkd3d_free(cfg->loops);
    vkd3d_free(cfg->loops_by_header);
    vkd3d_free(cfg->loop_intervals);

    if (TRACE_ON())
        vkd3d_string_buffer_cleanup(&cfg->debug_buffer);
}

static enum vkd3d_result vsir_cfg_add_loop_interval(struct vsir_cfg *cfg, unsigned int begin,
        unsigned int end, bool synthetic)
{
    struct cfg_loop_interval *interval;

    if (!vkd3d_array_reserve((void **)&cfg->loop_intervals, &cfg->loop_interval_capacity,
            cfg->loop_interval_count + 1, sizeof(*cfg->loop_intervals)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    interval = &cfg->loop_intervals[cfg->loop_interval_count++];

    interval->begin = begin;
    interval->end = end;
    interval->synthetic = synthetic;
    interval->target_count = 0;

    return VKD3D_OK;
}

static bool vsir_block_dominates(struct vsir_block *b1, struct vsir_block *b2)
{
    return bitmap_is_set(b1->dominates, b2->label - 1);
}

static enum vkd3d_result vsir_cfg_add_edge(struct vsir_cfg *cfg, struct vsir_block *block,
        struct vkd3d_shader_src_param *successor_param)
{
    unsigned int target = label_from_src_param(successor_param);
    struct vsir_block *successor = &cfg->blocks[target - 1];
    enum vkd3d_result ret;

    VKD3D_ASSERT(successor->label != 0);

    if ((ret = vsir_block_list_add(&block->successors, successor)) < 0)
        return ret;

    if ((ret = vsir_block_list_add(&successor->predecessors, block)) < 0)
        return ret;

    return VKD3D_OK;
}

static void vsir_cfg_dump_dot(struct vsir_cfg *cfg)
{
    size_t i, j;

    TRACE("digraph cfg {\n");

    for (i = 0; i < cfg->block_count; ++i)
    {
        struct vsir_block *block = &cfg->blocks[i];
        const char *shape;

        if (block->label == 0)
            continue;

        switch (block->end->opcode)
        {
            case VKD3DSIH_RET:
                shape = "trapezium";
                break;

            case VKD3DSIH_BRANCH:
                shape = vsir_register_is_label(&block->end->src[0].reg) ? "ellipse" : "box";
                break;

            default:
                vkd3d_unreachable();
        }

        TRACE("  n%u [label=\"%u\", shape=\"%s\"];\n", block->label, block->label, shape);

        for (j = 0; j < block->successors.count; ++j)
            TRACE("  n%u -> n%u;\n", block->label, block->successors.blocks[j]->label);
    }

    TRACE("}\n");
}

static void vsir_cfg_structure_list_dump(struct vsir_cfg *cfg, struct vsir_cfg_structure_list *list);

static void vsir_cfg_structure_dump(struct vsir_cfg *cfg, struct vsir_cfg_structure *structure)
{
    switch (structure->type)
    {
        case STRUCTURE_TYPE_BLOCK:
            TRACE("%sblock %u\n", cfg->debug_buffer.buffer, structure->u.block->label);
            break;

        case STRUCTURE_TYPE_LOOP:
            TRACE("%s%u : loop {\n", cfg->debug_buffer.buffer, structure->u.loop.idx);

            vsir_cfg_structure_list_dump(cfg, &structure->u.loop.body);

            TRACE("%s}  # %u%s\n", cfg->debug_buffer.buffer, structure->u.loop.idx,
                    structure->u.loop.needs_trampoline ? ", tramp" : "");
            break;

        case STRUCTURE_TYPE_SELECTION:
            TRACE("%sif {\n", cfg->debug_buffer.buffer);

            vsir_cfg_structure_list_dump(cfg, &structure->u.selection.if_body);

            if (structure->u.selection.else_body.count == 0)
            {
                TRACE("%s}\n", cfg->debug_buffer.buffer);
            }
            else
            {
                TRACE("%s} else {\n", cfg->debug_buffer.buffer);

                vsir_cfg_structure_list_dump(cfg, &structure->u.selection.else_body);

                TRACE("%s}\n", cfg->debug_buffer.buffer);
            }
            break;

        case STRUCTURE_TYPE_JUMP:
        {
            const char *type_str;

            switch (structure->u.jump.type)
            {
                case JUMP_RET:
                    TRACE("%sret\n", cfg->debug_buffer.buffer);
                    return;

                case JUMP_BREAK:
                    type_str = "break";
                    break;

                case JUMP_CONTINUE:
                    type_str = "continue";
                    break;

                default:
                    vkd3d_unreachable();
            }

            TRACE("%s%s%s %u%s\n", cfg->debug_buffer.buffer, type_str,
                    structure->u.jump.condition ? "c" : "", structure->u.jump.target,
                    structure->u.jump.needs_launcher ? "  # launch" : "");
            break;
        }

        default:
            vkd3d_unreachable();
    }
}

static void vsir_cfg_structure_list_dump(struct vsir_cfg *cfg, struct vsir_cfg_structure_list *list)
{
    unsigned int i;

    vkd3d_string_buffer_printf(&cfg->debug_buffer, "  ");

    for (i = 0; i < list->count; ++i)
        vsir_cfg_structure_dump(cfg, &list->structures[i]);

    vkd3d_string_buffer_truncate(&cfg->debug_buffer, cfg->debug_buffer.content_size - 2);
}

static void vsir_cfg_dump_structured_program(struct vsir_cfg *cfg)
{
    unsigned int i;

    for (i = 0; i < cfg->structured_program.count; ++i)
        vsir_cfg_structure_dump(cfg, &cfg->structured_program.structures[i]);
}

static enum vkd3d_result vsir_cfg_init(struct vsir_cfg *cfg, struct vsir_program *program,
        struct vkd3d_shader_message_context *message_context, struct vsir_cfg_emit_target *target,
        size_t *pos)
{
    struct vsir_block *current_block = NULL;
    enum vkd3d_result ret;
    size_t i;

    memset(cfg, 0, sizeof(*cfg));
    cfg->message_context = message_context;
    cfg->program = program;
    cfg->block_count = program->block_count;
    cfg->target = target;
    cfg->function_begin = *pos;

    vsir_block_list_init(&cfg->order);

    if (!(cfg->blocks = vkd3d_calloc(cfg->block_count, sizeof(*cfg->blocks))))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if (TRACE_ON())
        vkd3d_string_buffer_init(&cfg->debug_buffer);

    for (i = *pos; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *instruction = &program->instructions.elements[i];
        bool finish = false;

        switch (instruction->opcode)
        {
            case VKD3DSIH_PHI:
            case VKD3DSIH_SWITCH_MONOLITHIC:
                vkd3d_unreachable();

            case VKD3DSIH_LABEL:
            {
                unsigned int label = label_from_src_param(&instruction->src[0]);

                VKD3D_ASSERT(!current_block);
                VKD3D_ASSERT(label > 0);
                VKD3D_ASSERT(label <= cfg->block_count);
                current_block = &cfg->blocks[label - 1];
                VKD3D_ASSERT(current_block->label == 0);
                if ((ret = vsir_block_init(current_block, label, program->block_count)) < 0)
                    goto fail;
                current_block->begin = &program->instructions.elements[i + 1];
                if (!cfg->entry)
                    cfg->entry = current_block;
                break;
            }

            case VKD3DSIH_BRANCH:
            case VKD3DSIH_RET:
                VKD3D_ASSERT(current_block);
                current_block->end = instruction;
                current_block = NULL;
                break;

            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                VKD3D_ASSERT(!current_block);
                finish = true;
                break;

            default:
                break;
        }

        if (finish)
            break;
    }

    *pos = i;
    cfg->function_end = *pos;

    for (i = 0; i < cfg->block_count; ++i)
    {
        struct vsir_block *block = &cfg->blocks[i];

        if (block->label == 0)
            continue;

        switch (block->end->opcode)
        {
            case VKD3DSIH_RET:
                break;

            case VKD3DSIH_BRANCH:
                if (vsir_register_is_label(&block->end->src[0].reg))
                {
                    if ((ret = vsir_cfg_add_edge(cfg, block, &block->end->src[0])) < 0)
                        goto fail;
                }
                else
                {
                    if ((ret = vsir_cfg_add_edge(cfg, block, &block->end->src[1])) < 0)
                        goto fail;

                    if ((ret = vsir_cfg_add_edge(cfg, block, &block->end->src[2])) < 0)
                        goto fail;
                }
                break;

            default:
                vkd3d_unreachable();
        }
    }

    if (TRACE_ON())
        vsir_cfg_dump_dot(cfg);

    return VKD3D_OK;

fail:
    vsir_cfg_cleanup(cfg);

    return ret;
}

/* Block A dominates block B if every path from the entry point to B
 * must pass through A. Naively compute the set of blocks that are
 * dominated by `reference' by running a graph visit starting from the
 * entry point (which must be the initial value of `current') and
 * avoiding `reference'. Running this for all the blocks takes
 * quadratic time: if in the future something better is sought after,
 * the standard tool seems to be the Lengauer-Tarjan algorithm. */
static void vsir_cfg_compute_dominators_recurse(struct vsir_block *current, struct vsir_block *reference)
{
    size_t i;

    VKD3D_ASSERT(current->label != 0);

    if (current == reference)
        return;

    if (!bitmap_is_set(reference->dominates, current->label - 1))
        return;

    bitmap_clear(reference->dominates, current->label - 1);

    for (i = 0; i < current->successors.count; ++i)
        vsir_cfg_compute_dominators_recurse(current->successors.blocks[i], reference);
}

static void vsir_cfg_compute_dominators(struct vsir_cfg *cfg)
{
    size_t i, j;

    for (i = 0; i < cfg->block_count; ++i)
    {
        struct vsir_block *block = &cfg->blocks[i];

        if (block->label == 0)
            continue;

        vsir_cfg_compute_dominators_recurse(cfg->entry, block);

        if (TRACE_ON())
        {
            vkd3d_string_buffer_printf(&cfg->debug_buffer, "Block %u dominates:", block->label);
            for (j = 0; j < cfg->block_count; j++)
            {
                struct vsir_block *block2 = &cfg->blocks[j];

                if (block2->label == 0 || !vsir_block_dominates(block, block2))
                    continue;

                if (cfg->debug_buffer.content_size > 512)
                {
                    TRACE("%s...\n", cfg->debug_buffer.buffer);
                    vkd3d_string_buffer_clear(&cfg->debug_buffer);
                    vkd3d_string_buffer_printf(&cfg->debug_buffer, "Block %u dominates: ...", block->label);
                }
                vkd3d_string_buffer_printf(&cfg->debug_buffer, " %u", block2->label);
            }
            TRACE("%s\n", cfg->debug_buffer.buffer);
            vkd3d_string_buffer_clear(&cfg->debug_buffer);
        }
    }
}

/* A back edge is an edge X -> Y for which block Y dominates block
 * X. All the other edges are forward edges, and it is required that
 * the input CFG is reducible, i.e., it is acyclic once you strip away
 * the back edges.
 *
 * Each back edge X -> Y defines a loop: block X is the header block,
 * block Y is the back edge block, and the loop consists of all the
 * blocks which are dominated by the header block and have a path to
 * the back edge block that doesn't pass through the header block
 * (including the header block itself). It can be proved that all the
 * blocks in such a path (connecting a loop block to the back edge
 * block without passing through the header block) belong to the same
 * loop.
 *
 * If the input CFG is reducible its loops are properly nested (i.e.,
 * each two loops are either disjoint or one is contained in the
 * other), provided that each block has at most one incoming back
 * edge. If this condition does not hold, a synthetic block can be
 * introduced as the only back edge block for the given header block,
 * with all the previous back edge now being forward edges to the
 * synthetic block. This is not currently implemented (but it is
 * rarely found in practice anyway). */
static enum vkd3d_result vsir_cfg_scan_loop(struct vsir_block_list *loop, struct vsir_block *block,
        struct vsir_block *header)
{
    enum vkd3d_result ret;
    size_t i;

    if ((ret = vsir_block_list_add(loop, block)) < 0)
        return ret;

    if (ret == VKD3D_FALSE || block == header)
        return VKD3D_OK;

    for (i = 0; i < block->predecessors.count; ++i)
    {
        if ((ret = vsir_cfg_scan_loop(loop, block->predecessors.blocks[i], header)) < 0)
            return ret;
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_compute_loops(struct vsir_cfg *cfg)
{
    size_t i, j, k;

    if (!(cfg->loops_by_header = vkd3d_calloc(cfg->block_count, sizeof(*cfg->loops_by_header))))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    memset(cfg->loops_by_header, 0xff, cfg->block_count * sizeof(*cfg->loops_by_header));

    for (i = 0; i < cfg->block_count; ++i)
    {
        struct vsir_block *block = &cfg->blocks[i];

        if (block->label == 0)
            continue;

        for (j = 0; j < block->successors.count; ++j)
        {
            struct vsir_block *header = block->successors.blocks[j];
            struct vsir_block_list *loop;
            enum vkd3d_result ret;

            /* Is this a back edge? */
            if (!vsir_block_dominates(header, block))
                continue;

            if (!vkd3d_array_reserve((void **)&cfg->loops, &cfg->loops_capacity, cfg->loops_count + 1, sizeof(*cfg->loops)))
                return VKD3D_ERROR_OUT_OF_MEMORY;

            loop = &cfg->loops[cfg->loops_count];
            vsir_block_list_init(loop);

            if ((ret = vsir_cfg_scan_loop(loop, block, header)) < 0)
                return ret;

            vsir_block_list_sort(loop);

            if (TRACE_ON())
            {
                vkd3d_string_buffer_printf(&cfg->debug_buffer, "Back edge %u -> %u with loop:", block->label, header->label);

                for (k = 0; k < loop->count; ++k)
                {
                    if (cfg->debug_buffer.content_size > 512)
                    {
                        TRACE("%s...\n", cfg->debug_buffer.buffer);
                        vkd3d_string_buffer_clear(&cfg->debug_buffer);
                        vkd3d_string_buffer_printf(&cfg->debug_buffer, "Back edge %u -> %u with loop: ...",
                                block->label, header->label);
                    }
                    vkd3d_string_buffer_printf(&cfg->debug_buffer, " %u", loop->blocks[k]->label);
                }

                TRACE("%s\n", cfg->debug_buffer.buffer);
                vkd3d_string_buffer_clear(&cfg->debug_buffer);
            }

            if (cfg->loops_by_header[header->label - 1] != SIZE_MAX)
            {
                FIXME("Block %u is header to more than one loop, this is not implemented.\n", header->label);
                vkd3d_shader_error(cfg->message_context, &header->begin->location, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                        "Block %u is header to more than one loop, this is not implemented.", header->label);
                return VKD3D_ERROR_NOT_IMPLEMENTED;
            }

            cfg->loops_by_header[header->label - 1] = cfg->loops_count;

            ++cfg->loops_count;
        }
    }

    return VKD3D_OK;
}

struct vsir_cfg_node_sorter
{
    struct vsir_cfg *cfg;
    struct vsir_cfg_node_sorter_stack_item
    {
        struct vsir_block_list *loop;
        unsigned int seen_count;
        unsigned int begin;
    } *stack;
    size_t stack_count, stack_capacity;
    struct vsir_block_list available_blocks;
};

/* Topologically sort the blocks according to the forward edges. By
 * definition if the input CFG is reducible then its forward edges
 * form a DAG, so a topological sorting exists. In order to compute it
 * we keep an array with the incoming degree for each block and an
 * available list of all the blocks whose incoming degree has reached
 * zero. At each step we pick a block from the available list and
 * strip it away from the graph, updating the incoming degrees and
 * available list.
 *
 * In principle at each step we can pick whatever node we want from
 * the available list, and will get a topological sort
 * anyway. However, we use these two criteria to give to the computed
 * order additional properties:
 *
 *  1. we keep track of which loops we're into, and pick blocks
 *     belonging to the current innermost loop, so that loops are kept
 *     contiguous in the order; this can always be done when the input
 *     CFG is reducible;
 *
 *  2. subject to the requirement above, we always pick the most
 *     recently added block to the available list, because this tends
 *     to keep related blocks and require fewer control flow
 *     primitives.
 */
static enum vkd3d_result vsir_cfg_sort_nodes(struct vsir_cfg *cfg)
{
    struct vsir_cfg_node_sorter sorter = { .cfg = cfg };
    unsigned int *in_degrees = NULL;
    enum vkd3d_result ret;
    size_t i;

    if (!(in_degrees = vkd3d_calloc(cfg->block_count, sizeof(*in_degrees))))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    for (i = 0; i < cfg->block_count; ++i)
    {
        struct vsir_block *block = &cfg->blocks[i];

        if (block->label == 0)
        {
            in_degrees[i] = UINT_MAX;
            continue;
        }

        in_degrees[i] = block->predecessors.count;

        /* Do not count back edges. */
        if (cfg->loops_by_header[i] != SIZE_MAX)
        {
            VKD3D_ASSERT(in_degrees[i] > 0);
            in_degrees[i] -= 1;
        }

        if (in_degrees[i] == 0 && block != cfg->entry)
        {
            WARN("Unexpected entry point %u.\n", block->label);
            vkd3d_shader_error(cfg->message_context, &block->begin->location, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                    "Block %u is unreachable from the entry point.", block->label);
            ret = VKD3D_ERROR_INVALID_SHADER;
            goto fail;
        }
    }

    if (in_degrees[cfg->entry->label - 1] != 0)
    {
        WARN("Entry point has %u incoming forward edges.\n", in_degrees[cfg->entry->label - 1]);
        vkd3d_shader_error(cfg->message_context, &cfg->entry->begin->location, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "The entry point block has %u incoming forward edges.", in_degrees[cfg->entry->label - 1]);
        ret = VKD3D_ERROR_INVALID_SHADER;
        goto fail;
    }

    vsir_block_list_init(&sorter.available_blocks);

    if ((ret = vsir_block_list_add_checked(&sorter.available_blocks, cfg->entry)) < 0)
        goto fail;

    while (sorter.available_blocks.count != 0)
    {
        struct vsir_cfg_node_sorter_stack_item *inner_stack_item = NULL;
        struct vsir_block *block;
        size_t new_seen_count;

        if (sorter.stack_count != 0)
            inner_stack_item = &sorter.stack[sorter.stack_count - 1];

        for (i = sorter.available_blocks.count - 1; ; --i)
        {
            if (i == SIZE_MAX)
            {
                ERR("Couldn't find any viable next block, is the input CFG reducible?\n");
                ret = VKD3D_ERROR_INVALID_SHADER;
                goto fail;
            }

            block = sorter.available_blocks.blocks[i];

            if (!inner_stack_item || vsir_block_list_search(inner_stack_item->loop, block))
                break;
        }

        /* If the node is a loop header, open the loop. */
        if (sorter.cfg->loops_by_header[block->label - 1] != SIZE_MAX)
        {
            struct vsir_block_list *loop = &sorter.cfg->loops[sorter.cfg->loops_by_header[block->label - 1]];

            if (loop)
            {
                if (!vkd3d_array_reserve((void **)&sorter.stack, &sorter.stack_capacity,
                        sorter.stack_count + 1, sizeof(*sorter.stack)))
                    return VKD3D_ERROR_OUT_OF_MEMORY;

                inner_stack_item = &sorter.stack[sorter.stack_count++];
                inner_stack_item->loop = loop;
                inner_stack_item->seen_count = 0;
                inner_stack_item->begin = sorter.cfg->order.count;
            }
        }

        vsir_block_list_remove_index(&sorter.available_blocks, i);
        block->order_pos = cfg->order.count;
        if ((ret = vsir_block_list_add_checked(&cfg->order, block)) < 0)
            goto fail;

        /* Close loops: since each loop is a strict subset of any
         * outer loop, we just need to track how many blocks we've
         * seen; when I close a loop I mark the same number of seen
         * blocks for the next outer loop. */
        new_seen_count = 1;
        while (sorter.stack_count != 0)
        {
            inner_stack_item = &sorter.stack[sorter.stack_count - 1];

            inner_stack_item->seen_count += new_seen_count;

            VKD3D_ASSERT(inner_stack_item->seen_count <= inner_stack_item->loop->count);
            if (inner_stack_item->seen_count != inner_stack_item->loop->count)
                break;

            if ((ret = vsir_cfg_add_loop_interval(cfg, inner_stack_item->begin,
                    cfg->order.count, false)) < 0)
                goto fail;

            new_seen_count = inner_stack_item->loop->count;
            --sorter.stack_count;
        }

        /* Remove (forward) edges and make new nodes available. */
        for (i = 0; i < block->successors.count; ++i)
        {
            struct vsir_block *successor = block->successors.blocks[i];

            if (vsir_block_dominates(successor, block))
                continue;

            VKD3D_ASSERT(in_degrees[successor->label - 1] > 0);
            --in_degrees[successor->label - 1];

            if (in_degrees[successor->label - 1] == 0)
            {
                if ((ret = vsir_block_list_add_checked(&sorter.available_blocks, successor)) < 0)
                    goto fail;
            }
        }
    }

    if (cfg->order.count != cfg->block_count)
    {
        /* There is a cycle of forward edges. */
        WARN("The control flow graph is not reducible.\n");
        vkd3d_shader_error(cfg->message_context, &cfg->entry->begin->location, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "The control flow graph is not reducible.");
        ret = VKD3D_ERROR_INVALID_SHADER;
        goto fail;
    }

    VKD3D_ASSERT(sorter.stack_count == 0);

    vkd3d_free(in_degrees);
    vkd3d_free(sorter.stack);
    vsir_block_list_cleanup(&sorter.available_blocks);

    if (TRACE_ON())
    {
        vkd3d_string_buffer_printf(&cfg->debug_buffer, "Block order:");

        for (i = 0; i < cfg->order.count; ++i)
        {
            if (cfg->debug_buffer.content_size > 512)
            {
                TRACE("%s...\n", cfg->debug_buffer.buffer);
                vkd3d_string_buffer_clear(&cfg->debug_buffer);
                vkd3d_string_buffer_printf(&cfg->debug_buffer, "Block order: ...");
            }
            vkd3d_string_buffer_printf(&cfg->debug_buffer, " %u", cfg->order.blocks[i]->label);
        }

        TRACE("%s\n", cfg->debug_buffer.buffer);
        vkd3d_string_buffer_clear(&cfg->debug_buffer);
    }

    return VKD3D_OK;

fail:
    vkd3d_free(in_degrees);
    vkd3d_free(sorter.stack);
    vsir_block_list_cleanup(&sorter.available_blocks);

    return ret;
}

/* Sort loop intervals first by ascending begin time and then by
 * descending end time, so that inner intervals appear after outer
 * ones and disjoint intervals appear in their proper order. */
static int compare_loop_intervals(const void *ptr1, const void *ptr2)
{
    const struct cfg_loop_interval *interval1 = ptr1;
    const struct cfg_loop_interval *interval2 = ptr2;

    if (interval1->begin != interval2->begin)
        return vkd3d_u32_compare(interval1->begin, interval2->begin);

    return -vkd3d_u32_compare(interval1->end, interval2->end);
}

static enum vkd3d_result vsir_cfg_generate_synthetic_loop_intervals(struct vsir_cfg *cfg)
{
    enum vkd3d_result ret;
    size_t i, j, k;

    for (i = 0; i < cfg->block_count; ++i)
    {
        struct vsir_block *block = &cfg->blocks[i];

        if (block->label == 0)
            continue;

        for (j = 0; j < block->successors.count; ++j)
        {
            struct vsir_block *successor = block->successors.blocks[j];
            struct cfg_loop_interval *extend = NULL;
            unsigned int begin;
            enum
            {
                ACTION_DO_NOTHING,
                ACTION_CREATE_NEW,
                ACTION_EXTEND,
            } action = ACTION_CREATE_NEW;

            /* We've already constructed loop intervals for the back
             * edges, there's nothing more to do. */
            if (vsir_block_dominates(successor, block))
                continue;

            VKD3D_ASSERT(block->order_pos < successor->order_pos);

            /* Jumping from a block to the following one is always
             * possible, so nothing to do. */
            if (block->order_pos + 1 == successor->order_pos)
                continue;

            /* Let's look for a loop interval that already breaks at
             * `successor' and either contains or can be extended to
             * contain `block'. */
            for (k = 0; k < cfg->loop_interval_count; ++k)
            {
                struct cfg_loop_interval *interval = &cfg->loop_intervals[k];

                if (interval->end != successor->order_pos)
                    continue;

                if (interval->begin <= block->order_pos)
                {
                    action = ACTION_DO_NOTHING;
                    break;
                }

                if (interval->synthetic)
                {
                    action = ACTION_EXTEND;
                    extend = interval;
                    break;
                }
            }

            if (action == ACTION_DO_NOTHING)
                continue;

            /* Ok, we have to decide where the new or replacing
             * interval has to begin. These are the rules: 1. it must
             * begin before `block'; 2. intervals must be properly
             * nested; 3. the new interval should begin as late as
             * possible, to limit control flow depth and extension. */
            begin = block->order_pos;

            /* Our candidate interval is always [begin,
             * successor->order_pos), and we move `begin' backward
             * until the candidate interval contains all the intervals
             * whose endpoint lies in the candidate interval
             * itself. */
            for (k = 0; k < cfg->loop_interval_count; ++k)
            {
                struct cfg_loop_interval *interval = &cfg->loop_intervals[k];

                if (begin < interval->end && interval->end < successor->order_pos)
                    begin = min(begin, interval->begin);
            }

            /* New we have to care about the intervals whose begin
             * point lies in the candidate interval. We cannot move
             * the candidate interval endpoint, because it is
             * important that the loop break target matches
             * `successor'. So we have to move that interval's begin
             * point to the begin point of the candidate interval,
             * i.e. `begin'. But what if the interval we should extend
             * backward is not synthetic? This cannot happen,
             * fortunately, because it would mean that there is a jump
             * entering a loop via a block which is not the loop
             * header, so the CFG would not be reducible. */
            for (k = 0; k < cfg->loop_interval_count; ++k)
            {
                struct cfg_loop_interval *interval = &cfg->loop_intervals[k];

                if (interval->begin < successor->order_pos && successor->order_pos < interval->end)
                {
                    if (interval->synthetic)
                        interval->begin = min(begin, interval->begin);
                    VKD3D_ASSERT(begin >= interval->begin);
                }
            }

            if (action == ACTION_EXTEND)
                extend->begin = begin;
            else if ((ret = vsir_cfg_add_loop_interval(cfg, begin, successor->order_pos, true)) < 0)
                return ret;
        }
    }

    if (cfg->loop_intervals)
        qsort(cfg->loop_intervals, cfg->loop_interval_count, sizeof(*cfg->loop_intervals), compare_loop_intervals);

    if (TRACE_ON())
        for (i = 0; i < cfg->loop_interval_count; ++i)
            TRACE("%s loop interval %u - %u\n", cfg->loop_intervals[i].synthetic ? "Synthetic" : "Natural",
                    cfg->loop_intervals[i].begin, cfg->loop_intervals[i].end);

    return VKD3D_OK;
}

struct vsir_cfg_edge_action
{
    enum vsir_cfg_jump_type jump_type;
    unsigned int target;
    struct vsir_block *successor;
};

static void vsir_cfg_compute_edge_action(struct vsir_cfg *cfg, struct vsir_block *block,
        struct vsir_block *successor, struct vsir_cfg_edge_action *action)
{
    unsigned int i;

    action->target = UINT_MAX;
    action->successor = successor;

    if (successor->order_pos <= block->order_pos)
    {
        /* The successor is before the current block, so we have to
         * use `continue'. The target loop is the innermost that
         * contains the current block and has the successor as
         * `continue' target. */
        for (i = 0; i < cfg->loop_interval_count; ++i)
        {
            struct cfg_loop_interval *interval = &cfg->loop_intervals[i];

            if (interval->begin == successor->order_pos && block->order_pos < interval->end)
                action->target = i;

            if (interval->begin > successor->order_pos)
                break;
        }

        VKD3D_ASSERT(action->target != UINT_MAX);
        action->jump_type = JUMP_CONTINUE;
    }
    else
    {
        /* The successor is after the current block, so we have to use
         * `break', or possibly just jump to the following block. The
         * target loop is the outermost that contains the current
         * block and has the successor as `break' target. */
        for (i = 0; i < cfg->loop_interval_count; ++i)
        {
            struct cfg_loop_interval *interval = &cfg->loop_intervals[i];

            if (interval->begin <= block->order_pos && interval->end == successor->order_pos)
            {
                action->target = i;
                break;
            }
        }

        if (action->target == UINT_MAX)
        {
            VKD3D_ASSERT(successor->order_pos == block->order_pos + 1);
            action->jump_type = JUMP_NONE;
        }
        else
        {
            action->jump_type = JUMP_BREAK;
        }
    }
}

static enum vkd3d_result vsir_cfg_build_structured_program(struct vsir_cfg *cfg)
{
    unsigned int i, stack_depth = 1, open_interval_idx = 0;
    struct vsir_cfg_structure_list **stack = NULL;

    /* It's enough to allocate up to the maximum interval stacking
     * depth (plus one for the full program), but this is simpler. */
    if (!(stack = vkd3d_calloc(cfg->loop_interval_count + 1, sizeof(*stack))))
        goto fail;
    cfg->structured_program.end = cfg->order.count;
    stack[0] = &cfg->structured_program;

    for (i = 0; i < cfg->order.count; ++i)
    {
        struct vsir_block *block = cfg->order.blocks[i];
        struct vsir_cfg_structure *structure;

        VKD3D_ASSERT(stack_depth > 0);

        /* Open loop intervals. */
        while (open_interval_idx < cfg->loop_interval_count)
        {
            struct cfg_loop_interval *interval = &cfg->loop_intervals[open_interval_idx];

            if (interval->begin != i)
                break;

            if (!(structure = vsir_cfg_structure_list_append(stack[stack_depth - 1], STRUCTURE_TYPE_LOOP)))
                goto fail;
            structure->u.loop.idx = open_interval_idx++;

            structure->u.loop.body.end = interval->end;
            stack[stack_depth++] = &structure->u.loop.body;
        }

        /* Execute the block. */
        if (!(structure = vsir_cfg_structure_list_append(stack[stack_depth - 1], STRUCTURE_TYPE_BLOCK)))
            goto fail;
        structure->u.block = block;

        /* Generate between zero and two jump instructions. */
        switch (block->end->opcode)
        {
            case VKD3DSIH_BRANCH:
            {
                struct vsir_cfg_edge_action action_true, action_false;
                bool invert_condition = false;

                if (vsir_register_is_label(&block->end->src[0].reg))
                {
                    unsigned int target = label_from_src_param(&block->end->src[0]);
                    struct vsir_block *successor = &cfg->blocks[target - 1];

                    vsir_cfg_compute_edge_action(cfg, block, successor, &action_true);
                    action_false = action_true;
                }
                else
                {
                    unsigned int target = label_from_src_param(&block->end->src[1]);
                    struct vsir_block *successor = &cfg->blocks[target - 1];

                    vsir_cfg_compute_edge_action(cfg, block, successor, &action_true);

                    target = label_from_src_param(&block->end->src[2]);
                    successor = &cfg->blocks[target - 1];

                    vsir_cfg_compute_edge_action(cfg, block, successor, &action_false);
                }

                /* This will happen if the branch is unconditional,
                 * but also if it's conditional with the same target
                 * in both branches, which can happen in some corner
                 * cases, e.g. when converting switch instructions to
                 * selection ladders. */
                if (action_true.successor == action_false.successor)
                {
                    VKD3D_ASSERT(action_true.jump_type == action_false.jump_type);
                }
                else
                {
                    /* At most one branch can just fall through to the
                     * next block, in which case we make sure it's the
                     * false branch. */
                    if (action_true.jump_type == JUMP_NONE)
                    {
                        invert_condition = true;
                    }
                    else if (stack_depth >= 2)
                    {
                        struct vsir_cfg_structure_list *inner_loop_frame = stack[stack_depth - 2];
                        struct vsir_cfg_structure *inner_loop = &inner_loop_frame->structures[inner_loop_frame->count - 1];

                        VKD3D_ASSERT(inner_loop->type == STRUCTURE_TYPE_LOOP);

                        /* Otherwise, if one of the branches is
                         * continue-ing the inner loop we're inside,
                         * make sure it's the false branch (because it
                         * will be optimized out later). */
                        if (action_true.jump_type == JUMP_CONTINUE && action_true.target == inner_loop->u.loop.idx)
                            invert_condition = true;
                    }

                    if (invert_condition)
                    {
                        struct vsir_cfg_edge_action tmp = action_true;
                        action_true = action_false;
                        action_false = tmp;
                    }

                    VKD3D_ASSERT(action_true.jump_type != JUMP_NONE);

                    if (!(structure = vsir_cfg_structure_list_append(stack[stack_depth - 1], STRUCTURE_TYPE_JUMP)))
                        goto fail;
                    structure->u.jump.type = action_true.jump_type;
                    structure->u.jump.target = action_true.target;
                    structure->u.jump.condition = &block->end->src[0];
                    structure->u.jump.invert_condition = invert_condition;
                }

                if (action_false.jump_type != JUMP_NONE)
                {
                    if (!(structure = vsir_cfg_structure_list_append(stack[stack_depth - 1], STRUCTURE_TYPE_JUMP)))
                        goto fail;
                    structure->u.jump.type = action_false.jump_type;
                    structure->u.jump.target = action_false.target;
                }
                break;
            }

            case VKD3DSIH_RET:
                if (!(structure = vsir_cfg_structure_list_append(stack[stack_depth - 1], STRUCTURE_TYPE_JUMP)))
                    goto fail;
                structure->u.jump.type = JUMP_RET;
                break;

            default:
                vkd3d_unreachable();
        }

        /* Close loop intervals. */
        while (stack_depth > 0)
        {
            if (stack[stack_depth - 1]->end != i + 1)
                break;

            --stack_depth;
        }
    }

    VKD3D_ASSERT(stack_depth == 0);
    VKD3D_ASSERT(open_interval_idx == cfg->loop_interval_count);

    if (TRACE_ON())
        vsir_cfg_dump_structured_program(cfg);

    vkd3d_free(stack);

    return VKD3D_OK;

fail:
    vkd3d_free(stack);

    return VKD3D_ERROR_OUT_OF_MEMORY;
}

static void vsir_cfg_remove_trailing_continue(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_list *list, unsigned int target)
{
    struct vsir_cfg_structure *last = &list->structures[list->count - 1];

    if (last->type == STRUCTURE_TYPE_JUMP && last->u.jump.type == JUMP_CONTINUE
            && !last->u.jump.condition && last->u.jump.target == target)
    {
        --list->count;
        VKD3D_ASSERT(cfg->loop_intervals[target].target_count > 0);
        --cfg->loop_intervals[target].target_count;
    }
}

static struct vsir_cfg_structure *vsir_cfg_get_trailing_break(struct vsir_cfg_structure_list *list)
{
    struct vsir_cfg_structure *structure;
    size_t count = list->count;

    if (count == 0)
        return NULL;

    structure = &list->structures[count - 1];

    if (structure->type != STRUCTURE_TYPE_JUMP || structure->u.jump.type != JUMP_BREAK
            || structure->u.jump.condition)
        return NULL;

    return structure;
}

/* When the last instruction in both branches of a selection construct
 * is an unconditional break, any of them can be moved after the
 * selection construct. If they break the same loop both of them can
 * be moved out, otherwise we can choose which one: we choose the one
 * that breaks the innermost loop, because we hope to eventually
 * remove the loop itself.
 *
 * In principle a similar movement could be done when the last
 * instructions are continue and continue, or continue and break. But
 * in practice I don't think those situations can happen given the
 * previous passes we do on the program, so we don't care. */
static enum vkd3d_result vsir_cfg_move_breaks_out_of_selections(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_list *list)
{
    struct vsir_cfg_structure *selection, *if_break, *else_break, *new_break;
    unsigned int if_target, else_target, max_target;
    size_t pos = list->count - 1;

    selection = &list->structures[pos];
    VKD3D_ASSERT(selection->type == STRUCTURE_TYPE_SELECTION);

    if_break = vsir_cfg_get_trailing_break(&selection->u.selection.if_body);
    else_break = vsir_cfg_get_trailing_break(&selection->u.selection.else_body);

    if (!if_break || !else_break)
        return VKD3D_OK;

    if_target = if_break->u.jump.target;
    else_target = else_break->u.jump.target;
    max_target = max(if_target, else_target);

    if (!(new_break = vsir_cfg_structure_list_append(list, STRUCTURE_TYPE_JUMP)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    new_break->u.jump.type = JUMP_BREAK;
    new_break->u.jump.target = max_target;
    ++cfg->loop_intervals[max_target].target_count;

    /* Pointer `selection' could have been invalidated by the append
     * operation. */
    selection = &list->structures[pos];
    VKD3D_ASSERT(selection->type == STRUCTURE_TYPE_SELECTION);

    if (if_target == max_target)
    {
        --selection->u.selection.if_body.count;
        VKD3D_ASSERT(cfg->loop_intervals[if_target].target_count > 0);
        --cfg->loop_intervals[if_target].target_count;
    }

    if (else_target == max_target)
    {
        --selection->u.selection.else_body.count;
        VKD3D_ASSERT(cfg->loop_intervals[else_target].target_count > 0);
        --cfg->loop_intervals[else_target].target_count;
    }

    /* If a branch becomes empty, make it the else branch, so we save a block. */
    if (selection->u.selection.if_body.count == 0)
    {
        struct vsir_cfg_structure_list tmp;

        selection->u.selection.invert_condition = !selection->u.selection.invert_condition;
        tmp = selection->u.selection.if_body;
        selection->u.selection.if_body = selection->u.selection.else_body;
        selection->u.selection.else_body = tmp;
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_move_breaks_out_of_selections_recursively(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_list *list)
{
    struct vsir_cfg_structure *trailing;

    if (list->count == 0)
        return VKD3D_OK;

    trailing = &list->structures[list->count - 1];

    if (trailing->type != STRUCTURE_TYPE_SELECTION)
        return VKD3D_OK;

    vsir_cfg_move_breaks_out_of_selections_recursively(cfg, &trailing->u.selection.if_body);
    vsir_cfg_move_breaks_out_of_selections_recursively(cfg, &trailing->u.selection.else_body);

    return vsir_cfg_move_breaks_out_of_selections(cfg, list);
}

static enum vkd3d_result vsir_cfg_synthesize_selections(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_list *list)
{
    enum vkd3d_result ret;
    size_t i;

    for (i = 0; i < list->count; ++i)
    {
        struct vsir_cfg_structure *structure = &list->structures[i], new_selection, *new_jump;

        if (structure->type != STRUCTURE_TYPE_JUMP || !structure->u.jump.condition)
            continue;

        vsir_cfg_structure_init(&new_selection, STRUCTURE_TYPE_SELECTION);
        new_selection.u.selection.condition = structure->u.jump.condition;
        new_selection.u.selection.invert_condition = structure->u.jump.invert_condition;

        if (!(new_jump = vsir_cfg_structure_list_append(&new_selection.u.selection.if_body,
                STRUCTURE_TYPE_JUMP)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        new_jump->u.jump.type = structure->u.jump.type;
        new_jump->u.jump.target = structure->u.jump.target;

        /* Move the rest of the structure list in the else branch
         * rather than leaving it after the selection construct. The
         * reason is that this is more conducive to further
         * optimization, because all the conditional `break's appear
         * as the last instruction of a branch of a cascade of
         * selection constructs at the end of the structure list we're
         * processing, instead of being buried in the middle of the
         * structure list itself. */
        if ((ret = vsir_cfg_structure_list_append_from_region(&new_selection.u.selection.else_body,
                &list->structures[i + 1], list->count - i - 1)) < 0)
            return ret;

        *structure = new_selection;
        list->count = i + 1;

        if ((ret = vsir_cfg_synthesize_selections(cfg, &structure->u.selection.else_body)) < 0)
            return ret;

        if ((ret = vsir_cfg_move_breaks_out_of_selections(cfg, list)) < 0)
            return ret;

        break;
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_append_loop(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_list *new_list, struct vsir_cfg_structure *loop)
{
    struct vsir_cfg_structure_list *loop_body = &loop->u.loop.body;
    unsigned int target, loop_idx = loop->u.loop.idx;
    struct vsir_cfg_structure *trailing_break;
    enum vkd3d_result ret;

    trailing_break = vsir_cfg_get_trailing_break(loop_body);

    /* If the loop's last instruction is not a break, we cannot remove
     * the loop itself. */
    if (!trailing_break)
    {
        if ((ret = vsir_cfg_structure_list_append_from_region(new_list, loop, 1)) < 0)
            return ret;
        memset(loop, 0, sizeof(*loop));
        return VKD3D_OK;
    }

    target = trailing_break->u.jump.target;
    VKD3D_ASSERT(cfg->loop_intervals[target].target_count > 0);

    /* If the loop is not targeted by any jump, we can remove it. The
     * trailing `break' then targets another loop, so we have to keep
     * it. */
    if (cfg->loop_intervals[loop_idx].target_count == 0)
    {
        if ((ret = vsir_cfg_structure_list_append_from_region(new_list,
                &loop_body->structures[0], loop_body->count)) < 0)
            return ret;
        loop_body->count = 0;
        return VKD3D_OK;
    }

    /* If the loop is targeted only by its own trailing `break'
     * instruction, then we can remove it together with the `break'
     * itself. */
    if (target == loop_idx && cfg->loop_intervals[loop_idx].target_count == 1)
    {
        --cfg->loop_intervals[loop_idx].target_count;
        if ((ret = vsir_cfg_structure_list_append_from_region(new_list,
                &loop_body->structures[0], loop_body->count - 1)) < 0)
            return ret;
        loop_body->count = 0;
        return VKD3D_OK;
    }

    if ((ret = vsir_cfg_structure_list_append_from_region(new_list, loop, 1)) < 0)
        return ret;
    memset(loop, 0, sizeof(*loop));

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_optimize_recurse(struct vsir_cfg *cfg, struct vsir_cfg_structure_list *list)
{
    struct vsir_cfg_structure_list old_list = *list, *new_list = list;
    enum vkd3d_result ret;
    size_t i;

    memset(new_list, 0, sizeof(*new_list));

    for (i = 0; i < old_list.count; ++i)
    {
        struct vsir_cfg_structure *loop = &old_list.structures[i], *selection;
        struct vsir_cfg_structure_list *loop_body;

        if (loop->type != STRUCTURE_TYPE_LOOP)
        {
            if ((ret = vsir_cfg_structure_list_append_from_region(new_list, loop, 1)) < 0)
                goto out;
            memset(loop, 0, sizeof(*loop));
            continue;
        }

        loop_body = &loop->u.loop.body;

        if (loop_body->count == 0)
        {
            if ((ret = vsir_cfg_structure_list_append_from_region(new_list, loop, 1)) < 0)
                goto out;
            memset(loop, 0, sizeof(*loop));
            continue;
        }

        vsir_cfg_remove_trailing_continue(cfg, loop_body, loop->u.loop.idx);

        if ((ret = vsir_cfg_optimize_recurse(cfg, loop_body)) < 0)
            goto out;

        if ((ret = vsir_cfg_synthesize_selections(cfg, loop_body)) < 0)
            goto out;

        if ((ret = vsir_cfg_append_loop(cfg, new_list, loop)) < 0)
            goto out;

        /* If the last pushed instruction is a selection and one of the branches terminates with a
         * `break', start pushing to the other branch, in the hope of eventually push a `break'
         * there too and be able to remove a loop. */
        if (new_list->count == 0)
            continue;

        selection = &new_list->structures[new_list->count - 1];

        if (selection->type == STRUCTURE_TYPE_SELECTION)
        {
            if (vsir_cfg_get_trailing_break(&selection->u.selection.if_body))
                new_list = &selection->u.selection.else_body;
            else if (vsir_cfg_get_trailing_break(&selection->u.selection.else_body))
                new_list = &selection->u.selection.if_body;
        }
    }

    ret = vsir_cfg_move_breaks_out_of_selections_recursively(cfg, list);

out:
    vsir_cfg_structure_list_cleanup(&old_list);

    return ret;
}

static void vsir_cfg_count_targets(struct vsir_cfg *cfg, struct vsir_cfg_structure_list *list)
{
    size_t i;

    for (i = 0; i < list->count; ++i)
    {
        struct vsir_cfg_structure *structure = &list->structures[i];

        switch (structure->type)
        {
            case STRUCTURE_TYPE_BLOCK:
                break;

            case STRUCTURE_TYPE_LOOP:
                vsir_cfg_count_targets(cfg, &structure->u.loop.body);
                break;

            case STRUCTURE_TYPE_SELECTION:
                vsir_cfg_count_targets(cfg, &structure->u.selection.if_body);
                vsir_cfg_count_targets(cfg, &structure->u.selection.else_body);
                break;

            case STRUCTURE_TYPE_JUMP:
                if (structure->u.jump.type == JUMP_BREAK || structure->u.jump.type == JUMP_CONTINUE)
                    ++cfg->loop_intervals[structure->u.jump.target].target_count;
                break;
        }
    }
}

/* Trampolines are code gadgets used to emulate multilevel jumps (which are not natively supported
 * by SPIR-V). A trampoline is inserted just after a loop and checks whether control has reached the
 * intended site (i.e., we just jumped out of the target block) or if other levels of jumping are
 * needed. For each jump a trampoline is required for all the loops between the jump itself and the
 * target loop, excluding the target loop itself. */
static void vsir_cfg_mark_trampolines(struct vsir_cfg *cfg, struct vsir_cfg_structure_list *list,
        struct vsir_cfg_structure *loop)
{
    size_t i;

    for (i = 0; i < list->count; ++i)
    {
        struct vsir_cfg_structure *structure = &list->structures[i];

        switch (structure->type)
        {
            case STRUCTURE_TYPE_BLOCK:
                break;

            case STRUCTURE_TYPE_LOOP:
                structure->u.loop.outer_loop = loop;
                vsir_cfg_mark_trampolines(cfg, &structure->u.loop.body, structure);
                break;

            case STRUCTURE_TYPE_SELECTION:
                vsir_cfg_mark_trampolines(cfg, &structure->u.selection.if_body, loop);
                vsir_cfg_mark_trampolines(cfg, &structure->u.selection.else_body, loop);
                break;

            case STRUCTURE_TYPE_JUMP:
            {
                struct vsir_cfg_structure *l;
                if (structure->u.jump.type != JUMP_BREAK && structure->u.jump.type != JUMP_CONTINUE)
                    break;
                for (l = loop; l && l->u.loop.idx != structure->u.jump.target; l = l->u.loop.outer_loop)
                {
                    VKD3D_ASSERT(l->type == STRUCTURE_TYPE_LOOP);
                    l->u.loop.needs_trampoline = true;
                }
                break;
            }
        }
    }
}

/* Launchers are the counterpart of trampolines. A launcher is inserted just before a jump, and
 * writes in a well-known variable what is the target of the jump. Trampolines will then read that
 * variable to decide how to redirect the jump to its intended target. A launcher is needed each
 * time the innermost loop containing the jump itself has a trampoline (independently of whether the
 * jump is targeting that loop or not). */
static void vsir_cfg_mark_launchers(struct vsir_cfg *cfg, struct vsir_cfg_structure_list *list,
        struct vsir_cfg_structure *loop)
{
    size_t i;

    for (i = 0; i < list->count; ++i)
    {
        struct vsir_cfg_structure *structure = &list->structures[i];

        switch (structure->type)
        {
            case STRUCTURE_TYPE_BLOCK:
                break;

            case STRUCTURE_TYPE_LOOP:
                vsir_cfg_mark_launchers(cfg, &structure->u.loop.body, structure);
                break;

            case STRUCTURE_TYPE_SELECTION:
                vsir_cfg_mark_launchers(cfg, &structure->u.selection.if_body, loop);
                vsir_cfg_mark_launchers(cfg, &structure->u.selection.else_body, loop);
                break;

            case STRUCTURE_TYPE_JUMP:
                if (structure->u.jump.type != JUMP_BREAK && structure->u.jump.type != JUMP_CONTINUE)
                    break;
                VKD3D_ASSERT(loop && loop->type == STRUCTURE_TYPE_LOOP);
                if (loop->u.loop.needs_trampoline)
                    structure->u.jump.needs_launcher = true;
                break;
        }
    }
}

static enum vkd3d_result vsir_cfg_optimize(struct vsir_cfg *cfg)
{
    enum vkd3d_result ret;

    vsir_cfg_count_targets(cfg, &cfg->structured_program);

    ret = vsir_cfg_optimize_recurse(cfg, &cfg->structured_program);

    /* Trampolines and launchers cannot be marked with the same pass,
     * because a jump might have to be marked as launcher even when it
     * targets its innermost loop, if other jumps in the same loop
     * need a trampoline anyway. So launchers can be discovered only
     * once all the trampolines are known. */
    vsir_cfg_mark_trampolines(cfg, &cfg->structured_program, NULL);
    vsir_cfg_mark_launchers(cfg, &cfg->structured_program, NULL);

    if (TRACE_ON())
        vsir_cfg_dump_structured_program(cfg);

    return ret;
}

static enum vkd3d_result vsir_cfg_structure_list_emit(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_list *list, unsigned int loop_idx);

static enum vkd3d_result vsir_cfg_structure_list_emit_block(struct vsir_cfg *cfg,
        struct vsir_block *block)
{
    struct vsir_cfg_emit_target *target = cfg->target;

    if (!reserve_instructions(&target->instructions, &target->ins_capacity,
            target->ins_count + (block->end - block->begin)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    memcpy(&target->instructions[target->ins_count], block->begin,
            (char *)block->end - (char *)block->begin);

    target->ins_count += block->end - block->begin;

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_structure_list_emit_loop(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_loop *loop, unsigned int loop_idx)
{
    struct vsir_cfg_emit_target *target = cfg->target;
    const struct vkd3d_shader_location no_loc = {0};
    enum vkd3d_result ret;

    if (!reserve_instructions(&target->instructions, &target->ins_capacity, target->ins_count + 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    vsir_instruction_init(&target->instructions[target->ins_count++], &no_loc, VKD3DSIH_LOOP);

    if ((ret = vsir_cfg_structure_list_emit(cfg, &loop->body, loop->idx)) < 0)
        return ret;

    if (!reserve_instructions(&target->instructions, &target->ins_capacity, target->ins_count + 5))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    vsir_instruction_init(&target->instructions[target->ins_count++], &no_loc, VKD3DSIH_ENDLOOP);

    /* Add a trampoline to implement multilevel jumping depending on the stored
     * jump_target value. */
    if (loop->needs_trampoline)
    {
        /* If the multilevel jump is a `continue' and the target is the loop we're inside
         * right now, then we can finally do the `continue'. */
        const unsigned int outer_continue_target = loop_idx << 1 | 1;
        /* If the multilevel jump is a `continue' to any other target, or if it is a `break'
         * and the target is not the loop we just finished emitting, then it means that
         * we have to reach an outer loop, so we keep breaking. */
        const unsigned int inner_break_target = loop->idx << 1;

        if (!vsir_instruction_init_with_params(cfg->program, &target->instructions[target->ins_count],
                &no_loc, VKD3DSIH_IEQ, 1, 2))
            return VKD3D_ERROR_OUT_OF_MEMORY;

        dst_param_init_temp_bool(&target->instructions[target->ins_count].dst[0], target->temp_count);
        src_param_init_temp_uint(&target->instructions[target->ins_count].src[0], target->jump_target_temp_idx);
        src_param_init_const_uint(&target->instructions[target->ins_count].src[1], outer_continue_target);

        ++target->ins_count;

        if (!vsir_instruction_init_with_params(cfg->program, &target->instructions[target->ins_count],
                &no_loc, VKD3DSIH_CONTINUEP, 0, 1))
            return VKD3D_ERROR_OUT_OF_MEMORY;

        src_param_init_temp_bool(&target->instructions[target->ins_count].src[0], target->temp_count);

        ++target->ins_count;
        ++target->temp_count;

        if (!vsir_instruction_init_with_params(cfg->program, &target->instructions[target->ins_count],
                &no_loc, VKD3DSIH_IEQ, 1, 2))
            return VKD3D_ERROR_OUT_OF_MEMORY;

        dst_param_init_temp_bool(&target->instructions[target->ins_count].dst[0], target->temp_count);
        src_param_init_temp_uint(&target->instructions[target->ins_count].src[0], target->jump_target_temp_idx);
        src_param_init_const_uint(&target->instructions[target->ins_count].src[1], inner_break_target);

        ++target->ins_count;

        if (!vsir_instruction_init_with_params(cfg->program, &target->instructions[target->ins_count],
                &no_loc, VKD3DSIH_BREAKP, 0, 1))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        target->instructions[target->ins_count].flags |= VKD3D_SHADER_CONDITIONAL_OP_Z;

        src_param_init_temp_bool(&target->instructions[target->ins_count].src[0], target->temp_count);

        ++target->ins_count;
        ++target->temp_count;
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_structure_list_emit_selection(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_selection *selection, unsigned int loop_idx)
{
    struct vsir_cfg_emit_target *target = cfg->target;
    const struct vkd3d_shader_location no_loc = {0};
    enum vkd3d_result ret;

    if (!reserve_instructions(&target->instructions, &target->ins_capacity, target->ins_count + 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if (!vsir_instruction_init_with_params(cfg->program, &target->instructions[target->ins_count],
            &no_loc, VKD3DSIH_IF, 0, 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    target->instructions[target->ins_count].src[0] = *selection->condition;

    if (selection->invert_condition)
        target->instructions[target->ins_count].flags |= VKD3D_SHADER_CONDITIONAL_OP_Z;

    ++target->ins_count;

    if ((ret = vsir_cfg_structure_list_emit(cfg, &selection->if_body, loop_idx)) < 0)
        return ret;

    if (selection->else_body.count != 0)
    {
        if (!reserve_instructions(&target->instructions, &target->ins_capacity, target->ins_count + 1))
            return VKD3D_ERROR_OUT_OF_MEMORY;

        vsir_instruction_init(&target->instructions[target->ins_count++], &no_loc, VKD3DSIH_ELSE);

        if ((ret = vsir_cfg_structure_list_emit(cfg, &selection->else_body, loop_idx)) < 0)
            return ret;
    }

    if (!reserve_instructions(&target->instructions, &target->ins_capacity, target->ins_count + 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    vsir_instruction_init(&target->instructions[target->ins_count++], &no_loc, VKD3DSIH_ENDIF);

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_structure_list_emit_jump(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_jump *jump, unsigned int loop_idx)
{
    struct vsir_cfg_emit_target *target = cfg->target;
    const struct vkd3d_shader_location no_loc = {0};
    /* Encode the jump target as the loop index plus a bit to remember whether
     * we're breaking or continue-ing. */
    unsigned int jump_target = jump->target << 1;
    enum vkd3d_shader_opcode opcode;

    switch (jump->type)
    {
        case JUMP_CONTINUE:
            /* If we're continue-ing the loop we're directly inside, then we can emit a
             * `continue'. Otherwise we first have to break all the loops between here
             * and the loop to continue, recording our intention to continue
             * in the lowest bit of jump_target. */
            if (jump->target == loop_idx)
            {
                opcode = jump->condition ? VKD3DSIH_CONTINUEP : VKD3DSIH_CONTINUE;
                break;
            }
            jump_target |= 1;
            /* fall through */

        case JUMP_BREAK:
            opcode = jump->condition ? VKD3DSIH_BREAKP : VKD3DSIH_BREAK;
            break;

        case JUMP_RET:
            VKD3D_ASSERT(!jump->condition);
            opcode = VKD3DSIH_RET;
            break;

        default:
            vkd3d_unreachable();
    }

    if (!reserve_instructions(&target->instructions, &target->ins_capacity, target->ins_count + 2))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if (jump->needs_launcher)
    {
        if (!vsir_instruction_init_with_params(cfg->program, &target->instructions[target->ins_count],
                &no_loc, VKD3DSIH_MOV, 1, 1))
            return VKD3D_ERROR_OUT_OF_MEMORY;

        dst_param_init_temp_uint(&target->instructions[target->ins_count].dst[0], target->jump_target_temp_idx);
        src_param_init_const_uint(&target->instructions[target->ins_count].src[0], jump_target);

        ++target->ins_count;
    }

    if (!vsir_instruction_init_with_params(cfg->program, &target->instructions[target->ins_count],
            &no_loc, opcode, 0, !!jump->condition))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if (jump->invert_condition)
        target->instructions[target->ins_count].flags |= VKD3D_SHADER_CONDITIONAL_OP_Z;

    if (jump->condition)
        target->instructions[target->ins_count].src[0] = *jump->condition;

    ++target->ins_count;

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_structure_list_emit(struct vsir_cfg *cfg,
        struct vsir_cfg_structure_list *list, unsigned int loop_idx)
{
    enum vkd3d_result ret;
    size_t i;

    for (i = 0; i < list->count; ++i)
    {
        struct vsir_cfg_structure *structure = &list->structures[i];

        switch (structure->type)
        {
            case STRUCTURE_TYPE_BLOCK:
                if ((ret = vsir_cfg_structure_list_emit_block(cfg, structure->u.block)) < 0)
                    return ret;
                break;

            case STRUCTURE_TYPE_LOOP:
                if ((ret = vsir_cfg_structure_list_emit_loop(cfg, &structure->u.loop, loop_idx)) < 0)
                    return ret;
                break;

            case STRUCTURE_TYPE_SELECTION:
                if ((ret = vsir_cfg_structure_list_emit_selection(cfg, &structure->u.selection,
                        loop_idx)) < 0)
                    return ret;
                break;

            case STRUCTURE_TYPE_JUMP:
                if ((ret = vsir_cfg_structure_list_emit_jump(cfg, &structure->u.jump,
                        loop_idx)) < 0)
                    return ret;
                break;

            default:
                vkd3d_unreachable();
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_cfg_emit_structured_program(struct vsir_cfg *cfg)
{
    return vsir_cfg_structure_list_emit(cfg, &cfg->structured_program, UINT_MAX);
}

static enum vkd3d_result vsir_program_structurize_function(struct vsir_program *program,
        struct vkd3d_shader_message_context *message_context, struct vsir_cfg_emit_target *target,
        size_t *pos)
{
    enum vkd3d_result ret;
    struct vsir_cfg cfg;

    if ((ret = vsir_cfg_init(&cfg, program, message_context, target, pos)) < 0)
        return ret;

    vsir_cfg_compute_dominators(&cfg);

    if ((ret = vsir_cfg_compute_loops(&cfg)) < 0)
        goto out;

    if ((ret = vsir_cfg_sort_nodes(&cfg)) < 0)
        goto out;

    if ((ret = vsir_cfg_generate_synthetic_loop_intervals(&cfg)) < 0)
        goto out;

    if ((ret = vsir_cfg_build_structured_program(&cfg)) < 0)
        goto out;

    if ((ret = vsir_cfg_optimize(&cfg)) < 0)
        goto out;

    ret = vsir_cfg_emit_structured_program(&cfg);

out:
    vsir_cfg_cleanup(&cfg);

    return ret;
}

static enum vkd3d_result vsir_program_structurize(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_message_context *message_context = ctx->message_context;
    struct vsir_cfg_emit_target target = {0};
    enum vkd3d_result ret;
    size_t i;

    VKD3D_ASSERT(program->cf_type == VSIR_CF_BLOCKS);

    target.jump_target_temp_idx = program->temp_count;
    target.temp_count = program->temp_count + 1;

    if (!reserve_instructions(&target.instructions, &target.ins_capacity, program->instructions.count))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    for (i = 0; i < program->instructions.count;)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];

        switch (ins->opcode)
        {
            case VKD3DSIH_LABEL:
                VKD3D_ASSERT(program->shader_version.type != VKD3D_SHADER_TYPE_HULL);
                TRACE("Structurizing a non-hull shader.\n");
                if ((ret = vsir_program_structurize_function(program, message_context,
                        &target, &i)) < 0)
                    goto fail;
                VKD3D_ASSERT(i == program->instructions.count);
                break;

            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                VKD3D_ASSERT(program->shader_version.type == VKD3D_SHADER_TYPE_HULL);
                TRACE("Structurizing phase %u of a hull shader.\n", ins->opcode);
                target.instructions[target.ins_count++] = *ins;
                ++i;
                if ((ret = vsir_program_structurize_function(program, message_context,
                        &target, &i)) < 0)
                    goto fail;
                break;

            default:
                if (!reserve_instructions(&target.instructions, &target.ins_capacity, target.ins_count + 1))
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                target.instructions[target.ins_count++] = *ins;
                ++i;
                break;
        }
    }

    vkd3d_free(program->instructions.elements);
    program->instructions.elements = target.instructions;
    program->instructions.capacity = target.ins_capacity;
    program->instructions.count = target.ins_count;
    program->temp_count = target.temp_count;
    program->cf_type = VSIR_CF_STRUCTURED;

    return VKD3D_OK;

fail:
    vkd3d_free(target.instructions);

    return ret;
}

static void register_map_undominated_use(struct vkd3d_shader_register *reg, struct ssas_to_temps_alloc *alloc,
        struct vsir_block *block, struct vsir_block **origin_blocks)
{
    unsigned int i;

    if (register_is_ssa(reg))
    {
        i = reg->idx[0].offset;
        if (alloc->table[i] == UINT_MAX && !vsir_block_dominates(origin_blocks[i], block))
            alloc->table[i] = alloc->next_temp_idx++;
    }

    for (i = 0; i < reg->idx_count; ++i)
        if (reg->idx[i].rel_addr)
            register_map_undominated_use(&reg->idx[i].rel_addr->reg, alloc, block, origin_blocks);
}

/* Drivers are not necessarily optimised to handle very large numbers of temps. For example,
 * using them only where necessary fixes stuttering issues in Horizon Zero Dawn on RADV.
 * This can also result in the backend emitting less code because temps typically need an
 * access chain and a load/store. Conversion of phi SSA values to temps should eliminate all
 * undominated SSA use, but structurisation may create new occurrences. */
static enum vkd3d_result vsir_cfg_materialize_undominated_ssas_to_temps(struct vsir_cfg *cfg)
{
    struct vsir_program *program = cfg->program;
    struct ssas_to_temps_alloc alloc = {0};
    struct vsir_block **origin_blocks;
    unsigned int j;
    size_t i;

    if (!(origin_blocks = vkd3d_calloc(program->ssa_count, sizeof(*origin_blocks))))
    {
        ERR("Failed to allocate origin block array.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    if (!ssas_to_temps_alloc_init(&alloc, program->ssa_count, program->temp_count))
    {
        vkd3d_free(origin_blocks);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < cfg->block_count; ++i)
    {
        struct vsir_block *block = &cfg->blocks[i];
        struct vkd3d_shader_instruction *ins;

        if (block->label == 0)
            continue;

        for (ins = block->begin; ins <= block->end; ++ins)
        {
            for (j = 0; j < ins->dst_count; ++j)
            {
                if (register_is_ssa(&ins->dst[j].reg))
                    origin_blocks[ins->dst[j].reg.idx[0].offset] = block;
            }
        }
    }

    for (i = 0; i < cfg->block_count; ++i)
    {
        struct vsir_block *block = &cfg->blocks[i];
        struct vkd3d_shader_instruction *ins;

        if (block->label == 0)
            continue;

        for (ins = block->begin; ins <= block->end; ++ins)
        {
            for (j = 0; j < ins->src_count; ++j)
                register_map_undominated_use(&ins->src[j].reg, &alloc, block, origin_blocks);
        }
    }

    if (alloc.next_temp_idx == program->temp_count)
        goto done;

    TRACE("Emitting temps for %u values with undominated usage.\n", alloc.next_temp_idx - program->temp_count);

    for (i = cfg->function_begin; i < cfg->function_end; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];

        for (j = 0; j < ins->dst_count; ++j)
            materialize_ssas_to_temps_process_reg(program, &alloc, &ins->dst[j].reg);

        for (j = 0; j < ins->src_count; ++j)
            materialize_ssas_to_temps_process_reg(program, &alloc, &ins->src[j].reg);
    }

    program->temp_count = alloc.next_temp_idx;
done:
    vkd3d_free(origin_blocks);
    vkd3d_free(alloc.table);

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_materialize_undominated_ssas_to_temps_in_function(
        struct vsir_program *program, struct vkd3d_shader_message_context *message_context,
        size_t *pos)
{
    enum vkd3d_result ret;
    struct vsir_cfg cfg;

    if ((ret = vsir_cfg_init(&cfg, program, message_context, NULL, pos)) < 0)
        return ret;

    vsir_cfg_compute_dominators(&cfg);

    ret = vsir_cfg_materialize_undominated_ssas_to_temps(&cfg);

    vsir_cfg_cleanup(&cfg);

    return ret;
}

static enum vkd3d_result vsir_program_materialize_undominated_ssas_to_temps(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_message_context *message_context = ctx->message_context;
    enum vkd3d_result ret;
    size_t i;

    VKD3D_ASSERT(program->cf_type == VSIR_CF_BLOCKS);

    for (i = 0; i < program->instructions.count;)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];

        switch (ins->opcode)
        {
            case VKD3DSIH_LABEL:
                VKD3D_ASSERT(program->shader_version.type != VKD3D_SHADER_TYPE_HULL);
                TRACE("Materializing undominated SSAs in a non-hull shader.\n");
                if ((ret = vsir_program_materialize_undominated_ssas_to_temps_in_function(
                        program, message_context, &i)) < 0)
                    return ret;
                VKD3D_ASSERT(i == program->instructions.count);
                break;

            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                VKD3D_ASSERT(program->shader_version.type == VKD3D_SHADER_TYPE_HULL);
                TRACE("Materializing undominated SSAs in phase %u of a hull shader.\n", ins->opcode);
                ++i;
                if ((ret = vsir_program_materialize_undominated_ssas_to_temps_in_function(
                        program, message_context, &i)) < 0)
                    return ret;
                break;

            default:
                ++i;
                break;
        }
    }

    return VKD3D_OK;
}

static bool use_flat_interpolation(const struct vsir_program *program,
        struct vkd3d_shader_message_context *message_context, bool *flat)
{
    static const struct vkd3d_shader_location no_loc;
    const struct vkd3d_shader_parameter1 *parameter;

    *flat = false;

    if (!(parameter = vsir_program_get_parameter(program, VKD3D_SHADER_PARAMETER_NAME_FLAT_INTERPOLATION)))
        return true;

    if (parameter->type != VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
    {
        vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                "Unsupported flat interpolation parameter type %#x.", parameter->type);
        return false;
    }
    if (parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32)
    {
        vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid flat interpolation parameter data type %#x.", parameter->data_type);
        return false;
    }

    *flat = parameter->u.immediate_constant.u.u32;
    return true;
}

static enum vkd3d_result vsir_program_apply_flat_interpolation(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    unsigned int i;
    bool flat;

    if (program->shader_version.type != VKD3D_SHADER_TYPE_PIXEL || program->shader_version.major >= 4)
        return VKD3D_OK;

    if (!use_flat_interpolation(program, ctx->message_context, &flat))
        return VKD3D_ERROR_INVALID_ARGUMENT;

    if (!flat)
        return VKD3D_OK;

    for (i = 0; i < program->input_signature.element_count; ++i)
    {
        struct signature_element *element = &program->input_signature.elements[i];

        if (!ascii_strcasecmp(element->semantic_name, "COLOR"))
            element->interpolation_mode = VKD3DSIM_CONSTANT;
    }

    return VKD3D_OK;
}

static enum vkd3d_result insert_alpha_test_before_ret(struct vsir_program *program,
        const struct vkd3d_shader_instruction *ret, enum vkd3d_shader_comparison_func compare_func,
        const struct vkd3d_shader_parameter1 *ref, uint32_t colour_signature_idx,
        uint32_t colour_temp, size_t *ret_pos, struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    const struct vkd3d_shader_location loc = ret->location;
    static const struct vkd3d_shader_location no_loc;
    size_t pos = ret - instructions->elements;
    struct vkd3d_shader_instruction *ins;

    static const struct
    {
        enum vkd3d_shader_opcode float_opcode;
        enum vkd3d_shader_opcode uint_opcode;
        bool swap;
    }
    opcodes[] =
    {
        [VKD3D_SHADER_COMPARISON_FUNC_EQUAL]         = {VKD3DSIH_EQO, VKD3DSIH_IEQ},
        [VKD3D_SHADER_COMPARISON_FUNC_NOT_EQUAL]     = {VKD3DSIH_NEO, VKD3DSIH_INE},
        [VKD3D_SHADER_COMPARISON_FUNC_GREATER_EQUAL] = {VKD3DSIH_GEO, VKD3DSIH_UGE},
        [VKD3D_SHADER_COMPARISON_FUNC_LESS]          = {VKD3DSIH_LTO, VKD3DSIH_ULT},
        [VKD3D_SHADER_COMPARISON_FUNC_LESS_EQUAL]    = {VKD3DSIH_GEO, VKD3DSIH_UGE, true},
        [VKD3D_SHADER_COMPARISON_FUNC_GREATER]       = {VKD3DSIH_LTO, VKD3DSIH_ULT, true},
    };

    if (compare_func == VKD3D_SHADER_COMPARISON_FUNC_NEVER)
    {
        if (!shader_instruction_array_insert_at(&program->instructions, pos, 1))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        ret = NULL;
        ins = &program->instructions.elements[pos];

        vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_DISCARD, 0, 1);
        ins->flags = VKD3D_SHADER_CONDITIONAL_OP_Z;
        src_param_init_const_uint(&ins->src[0], 0);

        *ret_pos = pos + 1;
        return VKD3D_OK;
    }

    if (!shader_instruction_array_insert_at(&program->instructions, pos, 3))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ret = NULL;
    ins = &program->instructions.elements[pos];

    switch (ref->data_type)
    {
        case VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32:
            vsir_instruction_init_with_params(program, ins, &loc, opcodes[compare_func].float_opcode, 1, 2);
            src_param_init_temp_float(&ins->src[opcodes[compare_func].swap ? 1 : 0], colour_temp);
            src_param_init_parameter(&ins->src[opcodes[compare_func].swap ? 0 : 1],
                    VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_REF, VKD3D_DATA_FLOAT);
            break;

        case VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32:
            vsir_instruction_init_with_params(program, ins, &loc, opcodes[compare_func].uint_opcode, 1, 2);
            src_param_init_temp_uint(&ins->src[opcodes[compare_func].swap ? 1 : 0], colour_temp);
            src_param_init_parameter(&ins->src[opcodes[compare_func].swap ? 0 : 1],
                    VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_REF, VKD3D_DATA_UINT);
            break;

        case VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32_VEC4:
            vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_PARAMETER,
                    "Alpha test reference data type must be a single component.");
            return VKD3D_ERROR_INVALID_ARGUMENT;

        default:
            FIXME("Unhandled parameter data type %#x.\n", ref->data_type);
            return VKD3D_ERROR_NOT_IMPLEMENTED;
    }

    dst_param_init_ssa_bool(&ins->dst[0], program->ssa_count);
    ins->src[opcodes[compare_func].swap ? 1 : 0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->src[opcodes[compare_func].swap ? 1 : 0].swizzle = VKD3D_SHADER_SWIZZLE(W, W, W, W);

    ++ins;
    vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_DISCARD, 0, 1);
    ins->flags = VKD3D_SHADER_CONDITIONAL_OP_Z;
    src_param_init_ssa_bool(&ins->src[0], program->ssa_count);

    ++program->ssa_count;

    ++ins;
    vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_MOV, 1, 1);
    vsir_dst_param_init(&ins->dst[0], VKD3DSPR_OUTPUT, VKD3D_DATA_FLOAT, 1);
    ins->dst[0].reg.idx[0].offset = colour_signature_idx;
    ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->dst[0].write_mask = program->output_signature.elements[colour_signature_idx].mask;
    src_param_init_temp_float(&ins->src[0], colour_temp);
    ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;

    *ret_pos = pos + 3;
    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_insert_alpha_test(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_message_context *message_context = ctx->message_context;
    const struct vkd3d_shader_parameter1 *func = NULL, *ref = NULL;
    uint32_t colour_signature_idx, colour_temp = ~0u;
    static const struct vkd3d_shader_location no_loc;
    enum vkd3d_shader_comparison_func compare_func;
    struct vkd3d_shader_instruction *ins;
    size_t new_pos;
    int ret;

    if (program->shader_version.type != VKD3D_SHADER_TYPE_PIXEL)
        return VKD3D_OK;

    if (!vsir_signature_find_sysval(&program->output_signature, VKD3D_SHADER_SV_TARGET, 0, &colour_signature_idx)
            || !(program->output_signature.elements[colour_signature_idx].mask & VKD3DSP_WRITEMASK_3))
        return VKD3D_OK;

    if (!(func = vsir_program_get_parameter(program, VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_FUNC))
            || !(ref = vsir_program_get_parameter(program, VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_REF)))
        return VKD3D_OK;

    if (func->type != VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
    {
        vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                "Unsupported alpha test function parameter type %#x.", func->type);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }
    if (func->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32)
    {
        vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid alpha test function parameter data type %#x.", func->data_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    compare_func = func->u.immediate_constant.u.u32;

    if (compare_func == VKD3D_SHADER_COMPARISON_FUNC_ALWAYS)
        return VKD3D_OK;

    /* We're going to be reading from the output, so we need to go
     * through the whole shader and convert it to a temp. */

    if (compare_func != VKD3D_SHADER_COMPARISON_FUNC_NEVER)
        colour_temp = program->temp_count++;

    for (size_t i = 0; i < program->instructions.count; ++i)
    {
        ins = &program->instructions.elements[i];

        if (vsir_instruction_is_dcl(ins))
            continue;

        if (ins->opcode == VKD3DSIH_RET)
        {
            if ((ret = insert_alpha_test_before_ret(program, ins, compare_func,
                    ref, colour_signature_idx, colour_temp, &new_pos, message_context)) < 0)
                return ret;
            i = new_pos;
            continue;
        }

        /* No need to convert it if the comparison func is NEVER; we don't
         * read from the output in that case. */
        if (compare_func == VKD3D_SHADER_COMPARISON_FUNC_NEVER)
            continue;

        for (size_t j = 0; j < ins->dst_count; ++j)
        {
            struct vkd3d_shader_dst_param *dst = &ins->dst[j];

            /* Note we run after I/O normalization. */
            if (dst->reg.type == VKD3DSPR_OUTPUT && dst->reg.idx[0].offset == colour_signature_idx)
            {
                dst->reg.type = VKD3DSPR_TEMP;
                dst->reg.idx[0].offset = colour_temp;
            }
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result insert_clip_planes_before_ret(struct vsir_program *program,
        const struct vkd3d_shader_instruction *ret, uint32_t mask, uint32_t position_signature_idx,
        uint32_t position_temp, uint32_t low_signature_idx, uint32_t high_signature_idx, size_t *ret_pos)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    const struct vkd3d_shader_location loc = ret->location;
    size_t pos = ret - instructions->elements;
    struct vkd3d_shader_instruction *ins;
    unsigned int output_idx = 0;

    if (!shader_instruction_array_insert_at(&program->instructions, pos, vkd3d_popcount(mask) + 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ret = NULL;
    ins = &program->instructions.elements[pos];

    for (unsigned int i = 0; i < 8; ++i)
    {
        if (!(mask & (1u << i)))
            continue;

        vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_DP4, 1, 2);
        src_param_init_temp_float4(&ins->src[0], position_temp);
        src_param_init_parameter(&ins->src[1], VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_0 + i, VKD3D_DATA_FLOAT);
        ins->src[1].swizzle = VKD3D_SHADER_NO_SWIZZLE;
        ins->src[1].reg.dimension = VSIR_DIMENSION_VEC4;

        vsir_dst_param_init(&ins->dst[0], VKD3DSPR_OUTPUT, VKD3D_DATA_FLOAT, 1);
        if (output_idx < 4)
            ins->dst[0].reg.idx[0].offset = low_signature_idx;
        else
            ins->dst[0].reg.idx[0].offset = high_signature_idx;
        ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->dst[0].write_mask = (1u << (output_idx % 4));
        ++output_idx;

        ++ins;
    }

    vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_MOV, 1, 1);
    vsir_dst_param_init(&ins->dst[0], VKD3DSPR_OUTPUT, VKD3D_DATA_FLOAT, 1);
    ins->dst[0].reg.idx[0].offset = position_signature_idx;
    ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->dst[0].write_mask = program->output_signature.elements[position_signature_idx].mask;
    src_param_init_temp_float(&ins->src[0], position_temp);
    ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;

    *ret_pos = pos + vkd3d_popcount(mask) + 1;
    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_insert_clip_planes(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct shader_signature *signature = &program->output_signature;
    unsigned int low_signature_idx = ~0u, high_signature_idx = ~0u;
    const struct vkd3d_shader_parameter1 *mask_parameter = NULL;
    struct signature_element *new_elements, *clip_element;
    uint32_t position_signature_idx, position_temp, mask;
    static const struct vkd3d_shader_location no_loc;
    struct vkd3d_shader_instruction *ins;
    unsigned int plane_count;
    size_t new_pos;
    int ret;

    if (program->shader_version.type != VKD3D_SHADER_TYPE_VERTEX)
        return VKD3D_OK;

    for (unsigned int i = 0; i < program->parameter_count; ++i)
    {
        const struct vkd3d_shader_parameter1 *parameter = &program->parameters[i];

        if (parameter->name == VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_MASK)
            mask_parameter = parameter;
    }

    if (!mask_parameter)
        return VKD3D_OK;

    if (mask_parameter->type != VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
    {
        vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                "Unsupported clip plane mask parameter type %#x.", mask_parameter->type);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }
    if (mask_parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32)
    {
        vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid clip plane mask parameter data type %#x.", mask_parameter->data_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    mask = mask_parameter->u.immediate_constant.u.u32;

    if (!mask)
        return VKD3D_OK;

    for (unsigned int i = 0; i < signature->element_count; ++i)
    {
        if (signature->elements[i].sysval_semantic == VKD3D_SHADER_SV_CLIP_DISTANCE)
        {
            vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_PARAMETER,
                    "Clip planes cannot be used if the shader writes clip distance.");
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    if (!vsir_signature_find_sysval(signature, VKD3D_SHADER_SV_POSITION, 0, &position_signature_idx))
    {
        vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_MISSING_SEMANTIC,
                "Shader does not write position.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    /* Append the clip plane signature indices. */

    plane_count = vkd3d_popcount(mask);

    if (!(new_elements = vkd3d_realloc(signature->elements,
            (signature->element_count + 2) * sizeof(*signature->elements))))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    signature->elements = new_elements;

    low_signature_idx = signature->element_count;
    clip_element = &signature->elements[signature->element_count++];
    memset(clip_element, 0, sizeof(*clip_element));
    clip_element->sysval_semantic = VKD3D_SHADER_SV_CLIP_DISTANCE;
    clip_element->component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    clip_element->register_count = 1;
    clip_element->mask = vkd3d_write_mask_from_component_count(min(plane_count, 4));
    clip_element->used_mask = clip_element->mask;
    clip_element->min_precision = VKD3D_SHADER_MINIMUM_PRECISION_NONE;

    if (plane_count > 4)
    {
        high_signature_idx = signature->element_count;
        clip_element = &signature->elements[signature->element_count++];
        memset(clip_element, 0, sizeof(*clip_element));
        clip_element->sysval_semantic = VKD3D_SHADER_SV_CLIP_DISTANCE;
        clip_element->semantic_index = 1;
        clip_element->component_type = VKD3D_SHADER_COMPONENT_FLOAT;
        clip_element->register_count = 1;
        clip_element->mask = vkd3d_write_mask_from_component_count(plane_count - 4);
        clip_element->used_mask = clip_element->mask;
        clip_element->min_precision = VKD3D_SHADER_MINIMUM_PRECISION_NONE;
    }

    /* We're going to be reading from the output position, so we need to go
     * through the whole shader and convert it to a temp. */

    position_temp = program->temp_count++;

    for (size_t i = 0; i < program->instructions.count; ++i)
    {
        ins = &program->instructions.elements[i];

        if (vsir_instruction_is_dcl(ins))
            continue;

        if (ins->opcode == VKD3DSIH_RET)
        {
            if ((ret = insert_clip_planes_before_ret(program, ins, mask, position_signature_idx,
                    position_temp, low_signature_idx, high_signature_idx, &new_pos)) < 0)
                return ret;
            i = new_pos;
            continue;
        }

        for (size_t j = 0; j < ins->dst_count; ++j)
        {
            struct vkd3d_shader_dst_param *dst = &ins->dst[j];

            /* Note we run after I/O normalization. */
            if (dst->reg.type == VKD3DSPR_OUTPUT && dst->reg.idx[0].offset == position_signature_idx)
            {
                dst->reg.type = VKD3DSPR_TEMP;
                dst->reg.idx[0].offset = position_temp;
            }
        }
    }

    return VKD3D_OK;
}

static bool is_pre_rasterization_shader(enum vkd3d_shader_type type)
{
    return type == VKD3D_SHADER_TYPE_VERTEX
            || type == VKD3D_SHADER_TYPE_HULL
            || type == VKD3D_SHADER_TYPE_DOMAIN
            || type == VKD3D_SHADER_TYPE_GEOMETRY;
}

static enum vkd3d_result insert_point_size_before_ret(struct vsir_program *program,
        const struct vkd3d_shader_instruction *ret, size_t *ret_pos)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    const struct vkd3d_shader_location loc = ret->location;
    size_t pos = ret - instructions->elements;
    struct vkd3d_shader_instruction *ins;

    if (!shader_instruction_array_insert_at(&program->instructions, pos, 1))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ret = NULL;
    ins = &program->instructions.elements[pos];

    vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_MOV, 1, 1);
    vsir_dst_param_init(&ins->dst[0], VKD3DSPR_RASTOUT, VKD3D_DATA_FLOAT, 1);
    ins->dst[0].reg.idx[0].offset = VSIR_RASTOUT_POINT_SIZE;
    src_param_init_parameter(&ins->src[0], VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE, VKD3D_DATA_FLOAT);

    *ret_pos = pos + 1;
    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_insert_point_size(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    const struct vkd3d_shader_parameter1 *size_parameter = NULL;
    static const struct vkd3d_shader_location no_loc;

    if (program->has_point_size)
        return VKD3D_OK;

    if (!is_pre_rasterization_shader(program->shader_version.type))
        return VKD3D_OK;

    for (unsigned int i = 0; i < program->parameter_count; ++i)
    {
        const struct vkd3d_shader_parameter1 *parameter = &program->parameters[i];

        if (parameter->name == VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE)
            size_parameter = parameter;
    }

    if (!size_parameter)
        return VKD3D_OK;

    if (size_parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32)
    {
        vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid point size parameter data type %#x.", size_parameter->data_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    program->has_point_size = true;

    /* Append a point size write before each ret. */
    for (size_t i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];

        if (ins->opcode == VKD3DSIH_RET)
        {
            size_t new_pos;
            int ret;

            if ((ret = insert_point_size_before_ret(program, ins, &new_pos)) < 0)
                return ret;
            i = new_pos;
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_insert_point_size_clamp(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    const struct vkd3d_shader_parameter1 *min_parameter = NULL, *max_parameter = NULL;
    static const struct vkd3d_shader_location no_loc;

    if (!program->has_point_size)
        return VKD3D_OK;

    if (!is_pre_rasterization_shader(program->shader_version.type))
        return VKD3D_OK;

    for (unsigned int i = 0; i < program->parameter_count; ++i)
    {
        const struct vkd3d_shader_parameter1 *parameter = &program->parameters[i];

        if (parameter->name == VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE_MIN)
            min_parameter = parameter;
        else if (parameter->name == VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE_MAX)
            max_parameter = parameter;
    }

    if (!min_parameter && !max_parameter)
        return VKD3D_OK;

    if (min_parameter && min_parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32)
    {
        vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid minimum point size parameter data type %#x.", min_parameter->data_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (max_parameter && max_parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32)
    {
        vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid maximum point size parameter data type %#x.", max_parameter->data_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    /* Replace writes to the point size by inserting a clamp before each write. */

    for (size_t i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];
        const struct vkd3d_shader_location *loc;
        unsigned int ssa_value;
        bool clamp = false;

        if (vsir_instruction_is_dcl(ins))
            continue;

        for (size_t j = 0; j < ins->dst_count; ++j)
        {
            struct vkd3d_shader_dst_param *dst = &ins->dst[j];

            /* Note we run after I/O normalization. */
            if (dst->reg.type == VKD3DSPR_RASTOUT)
            {
                dst_param_init_ssa_float(dst, program->ssa_count);
                ssa_value = program->ssa_count++;
                clamp = true;
            }
        }

        if (!clamp)
            continue;

        if (!shader_instruction_array_insert_at(&program->instructions, i + 1, !!min_parameter + !!max_parameter))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        ins = &program->instructions.elements[i + 1];

        loc = &program->instructions.elements[i].location;

        if (min_parameter)
        {
            vsir_instruction_init_with_params(program, ins, loc, VKD3DSIH_MAX, 1, 2);
            src_param_init_ssa_float(&ins->src[0], ssa_value);
            src_param_init_parameter(&ins->src[1], VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE_MIN, VKD3D_DATA_FLOAT);
            if (max_parameter)
            {
                dst_param_init_ssa_float(&ins->dst[0], program->ssa_count);
                ssa_value = program->ssa_count++;
            }
            else
            {
                vsir_dst_param_init(&ins->dst[0], VKD3DSPR_RASTOUT, VKD3D_DATA_FLOAT, 1);
                ins->dst[0].reg.idx[0].offset = VSIR_RASTOUT_POINT_SIZE;
            }
            ++ins;
            ++i;
        }

        if (max_parameter)
        {
            vsir_instruction_init_with_params(program, ins, loc, VKD3DSIH_MIN, 1, 2);
            src_param_init_ssa_float(&ins->src[0], ssa_value);
            src_param_init_parameter(&ins->src[1], VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE_MAX, VKD3D_DATA_FLOAT);
            vsir_dst_param_init(&ins->dst[0], VKD3DSPR_RASTOUT, VKD3D_DATA_FLOAT, 1);
            ins->dst[0].reg.idx[0].offset = VSIR_RASTOUT_POINT_SIZE;

            ++i;
        }
    }

    return VKD3D_OK;
}

static bool has_texcoord_signature_element(const struct shader_signature *signature)
{
    for (size_t i = 0; i < signature->element_count; ++i)
    {
        if (!ascii_strcasecmp(signature->elements[i].semantic_name, "TEXCOORD"))
            return true;
    }
    return false;
}

/* Returns true if replacement was done. */
static bool replace_texcoord_with_point_coord(struct vsir_program *program,
        struct vkd3d_shader_src_param *src, unsigned int coord_temp)
{
    uint32_t prev_swizzle = src->swizzle;
    const struct signature_element *e;

    /* The input semantic may have a nontrivial mask, which we need to
     * correct for. E.g. if the mask is .yz, and we read from .y, that needs
     * to become .x. */
    static const uint32_t inverse_swizzles[16] =
    {
        /* Use _ for "undefined" components, for clarity. */
#define VKD3D_SHADER_SWIZZLE__ VKD3D_SHADER_SWIZZLE_X
        0,
        /* .x    */ VKD3D_SHADER_SWIZZLE(X, _, _, _),
        /* .y    */ VKD3D_SHADER_SWIZZLE(_, X, _, _),
        /* .xy   */ VKD3D_SHADER_SWIZZLE(X, Y, _, _),
        /* .z    */ VKD3D_SHADER_SWIZZLE(_, _, X, _),
        /* .xz   */ VKD3D_SHADER_SWIZZLE(X, _, Y, _),
        /* .yz   */ VKD3D_SHADER_SWIZZLE(_, X, Y, _),
        /* .xyz  */ VKD3D_SHADER_SWIZZLE(X, Y, Z, _),
        /* .w    */ VKD3D_SHADER_SWIZZLE(_, _, _, X),
        /* .xw   */ VKD3D_SHADER_SWIZZLE(X, _, _, Y),
        /* .yw   */ VKD3D_SHADER_SWIZZLE(_, X, _, Y),
        /* .xyw  */ VKD3D_SHADER_SWIZZLE(X, Y, _, Z),
        /* .zw   */ VKD3D_SHADER_SWIZZLE(_, _, X, Y),
        /* .xzw  */ VKD3D_SHADER_SWIZZLE(X, _, Y, Z),
        /* .yzw  */ VKD3D_SHADER_SWIZZLE(_, X, Y, Z),
        /* .xyzw */ VKD3D_SHADER_SWIZZLE(X, Y, Z, W),
#undef VKD3D_SHADER_SWIZZLE__
    };

    if (src->reg.type != VKD3DSPR_INPUT)
        return false;
    e = &program->input_signature.elements[src->reg.idx[0].offset];

    if (ascii_strcasecmp(e->semantic_name, "TEXCOORD"))
        return false;

    src->reg.type = VKD3DSPR_TEMP;
    src->reg.idx[0].offset = coord_temp;

    /* If the mask is already contiguous and zero-based, no need to remap
     * the swizzle. */
    if (!(e->mask & (e->mask + 1)))
        return true;

    src->swizzle = 0;
    for (unsigned int i = 0; i < 4; ++i)
    {
        src->swizzle |= vsir_swizzle_get_component(inverse_swizzles[e->mask],
                vsir_swizzle_get_component(prev_swizzle, i)) << VKD3D_SHADER_SWIZZLE_SHIFT(i);
    }

    return true;
}

static enum vkd3d_result vsir_program_insert_point_coord(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    const struct vkd3d_shader_parameter1 *sprite_parameter = NULL;
    static const struct vkd3d_shader_location no_loc;
    struct vkd3d_shader_instruction *ins;
    bool used_texcoord = false;
    unsigned int coord_temp;
    size_t i, insert_pos;

    if (program->shader_version.type != VKD3D_SHADER_TYPE_PIXEL)
        return VKD3D_OK;

    for (i = 0; i < program->parameter_count; ++i)
    {
        const struct vkd3d_shader_parameter1 *parameter = &program->parameters[i];

        if (parameter->name == VKD3D_SHADER_PARAMETER_NAME_POINT_SPRITE)
            sprite_parameter = parameter;
    }

    if (!sprite_parameter)
        return VKD3D_OK;

    if (sprite_parameter->type != VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
    {
        vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                "Unsupported point sprite parameter type %#x.", sprite_parameter->type);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }
    if (sprite_parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32)
    {
        vkd3d_shader_error(ctx->message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid point sprite parameter data type %#x.", sprite_parameter->data_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    if (!sprite_parameter->u.immediate_constant.u.u32)
        return VKD3D_OK;

    if (!has_texcoord_signature_element(&program->input_signature))
        return VKD3D_OK;

    /* VKD3DSPR_POINTCOORD is a two-component value; fill the remaining two
     * components with zeroes. */
    coord_temp = program->temp_count++;

    /* Construct the new temp after all LABEL, DCL, and NOP instructions.
     * We need to skip NOP instructions because they might result from removed
     * DCLs, and there could still be DCLs after NOPs. */
    for (i = 0; i < program->instructions.count; ++i)
    {
        ins = &program->instructions.elements[i];

        if (!vsir_instruction_is_dcl(ins) && ins->opcode != VKD3DSIH_LABEL && ins->opcode != VKD3DSIH_NOP)
            break;
    }

    insert_pos = i;

    /* Replace each texcoord read with a read from the point coord. */
    for (; i < program->instructions.count; ++i)
    {
        ins = &program->instructions.elements[i];

        if (vsir_instruction_is_dcl(ins))
            continue;

        for (unsigned int j = 0; j < ins->src_count; ++j)
        {
            used_texcoord |= replace_texcoord_with_point_coord(program, &ins->src[j], coord_temp);

            for (unsigned int k = 0; k < ins->src[j].reg.idx_count; ++k)
            {
                if (ins->src[j].reg.idx[k].rel_addr)
                    used_texcoord |= replace_texcoord_with_point_coord(program,
                            ins->src[j].reg.idx[k].rel_addr, coord_temp);
            }
        }

        for (unsigned int j = 0; j < ins->dst_count; ++j)
        {
            for (unsigned int k = 0; k < ins->dst[j].reg.idx_count; ++k)
            {
                if (ins->dst[j].reg.idx[k].rel_addr)
                    used_texcoord |= replace_texcoord_with_point_coord(program,
                            ins->dst[j].reg.idx[k].rel_addr, coord_temp);
            }
        }
    }

    if (used_texcoord)
    {
        if (!shader_instruction_array_insert_at(&program->instructions, insert_pos, 2))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        ins = &program->instructions.elements[insert_pos];

        vsir_instruction_init_with_params(program, ins, &no_loc, VKD3DSIH_MOV, 1, 1);
        dst_param_init_temp_float4(&ins->dst[0], coord_temp);
        ins->dst[0].write_mask = VKD3DSP_WRITEMASK_0 | VKD3DSP_WRITEMASK_1;
        vsir_src_param_init(&ins->src[0], VKD3DSPR_POINT_COORD, VKD3D_DATA_FLOAT, 0);
        ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;
        ++ins;

        vsir_instruction_init_with_params(program, ins, &no_loc, VKD3DSIH_MOV, 1, 1);
        dst_param_init_temp_float4(&ins->dst[0], coord_temp);
        ins->dst[0].write_mask = VKD3DSP_WRITEMASK_2 | VKD3DSP_WRITEMASK_3;
        vsir_src_param_init(&ins->src[0], VKD3DSPR_IMMCONST, VKD3D_DATA_FLOAT, 0);
        ins->src[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ++ins;

        program->has_point_coord = true;
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_add_fog_input(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct shader_signature *signature = &program->input_signature;
    uint32_t register_idx = 0;

    if (program->shader_version.type != VKD3D_SHADER_TYPE_PIXEL)
        return VKD3D_OK;

    if (!vsir_program_get_parameter(program, VKD3D_SHADER_PARAMETER_NAME_FOG_FRAGMENT_MODE))
        return VKD3D_OK;

    /* We could check the value and skip this if NONE, but chances are if a
     * user specifies the fog fragment mode as a parameter, they'll want to
     * enable it dynamically. Always specifying it (and hence always outputting
     * it from the VS) avoids an extra VS variant. */

    if (vsir_signature_find_element_by_name(signature, "FOG", 0))
        return VKD3D_OK;

    for (unsigned int i = 0; i < signature->element_count; ++i)
        register_idx = max(register_idx, signature->elements[i].register_index + 1);

    if (!add_signature_element(signature, "FOG", 0, VKD3DSP_WRITEMASK_0, register_idx, VKD3DSIM_LINEAR))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    return VKD3D_OK;
}

static enum vkd3d_result insert_fragment_fog_before_ret(struct vsir_program *program,
        const struct vkd3d_shader_instruction *ret, enum vkd3d_shader_fog_fragment_mode mode,
        uint32_t fog_signature_idx, uint32_t colour_signature_idx, uint32_t colour_temp,
        size_t *ret_pos, struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_location loc = ret->location;
    uint32_t ssa_factor = program->ssa_count++;
    size_t pos = ret - instructions->elements;
    struct vkd3d_shader_instruction *ins;
    uint32_t ssa_temp, ssa_temp2;

    switch (mode)
    {
        case VKD3D_SHADER_FOG_FRAGMENT_LINEAR:
            /* We generate the following code:
             *
             * add sr0, FOG_END, -vFOG.x
             * mul_sat srFACTOR, sr0, FOG_SCALE
             */
            if (!shader_instruction_array_insert_at(&program->instructions, pos, 4))
                return VKD3D_ERROR_OUT_OF_MEMORY;
            ret = NULL;

            *ret_pos = pos + 4;

            ssa_temp = program->ssa_count++;

            ins = &program->instructions.elements[pos];

            vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_ADD, 1, 2);
            dst_param_init_ssa_float(&ins->dst[0], ssa_temp);
            src_param_init_parameter(&ins->src[0], VKD3D_SHADER_PARAMETER_NAME_FOG_END, VKD3D_DATA_FLOAT);
            vsir_src_param_init(&ins->src[1], VKD3DSPR_INPUT, VKD3D_DATA_FLOAT, 1);
            ins->src[1].reg.idx[0].offset = fog_signature_idx;
            ins->src[1].reg.dimension = VSIR_DIMENSION_VEC4;
            ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
            ins->src[1].modifiers = VKD3DSPSM_NEG;

            vsir_instruction_init_with_params(program, ++ins, &loc, VKD3DSIH_MUL, 1, 2);
            dst_param_init_ssa_float(&ins->dst[0], ssa_factor);
            ins->dst[0].modifiers = VKD3DSPDM_SATURATE;
            src_param_init_ssa_float(&ins->src[0], ssa_temp);
            src_param_init_parameter(&ins->src[1], VKD3D_SHADER_PARAMETER_NAME_FOG_SCALE, VKD3D_DATA_FLOAT);
            break;

        case VKD3D_SHADER_FOG_FRAGMENT_EXP:
            /* We generate the following code:
             *
             * mul sr0, FOG_SCALE, vFOG.x
             * exp_sat srFACTOR, -sr0
             */
            if (!shader_instruction_array_insert_at(&program->instructions, pos, 4))
                return VKD3D_ERROR_OUT_OF_MEMORY;
            ret = NULL;

            *ret_pos = pos + 4;

            ssa_temp = program->ssa_count++;

            ins = &program->instructions.elements[pos];

            vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_MUL, 1, 2);
            dst_param_init_ssa_float(&ins->dst[0], ssa_temp);
            src_param_init_parameter(&ins->src[0], VKD3D_SHADER_PARAMETER_NAME_FOG_SCALE, VKD3D_DATA_FLOAT);
            vsir_src_param_init(&ins->src[1], VKD3DSPR_INPUT, VKD3D_DATA_FLOAT, 1);
            ins->src[1].reg.idx[0].offset = fog_signature_idx;
            ins->src[1].reg.dimension = VSIR_DIMENSION_VEC4;
            ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);

            vsir_instruction_init_with_params(program, ++ins, &loc, VKD3DSIH_EXP, 1, 1);
            dst_param_init_ssa_float(&ins->dst[0], ssa_factor);
            ins->dst[0].modifiers = VKD3DSPDM_SATURATE;
            src_param_init_ssa_float(&ins->src[0], ssa_temp);
            ins->src[0].modifiers = VKD3DSPSM_NEG;
            break;

        case VKD3D_SHADER_FOG_FRAGMENT_EXP2:
            /* We generate the following code:
             *
             * mul sr0, FOG_SCALE, vFOG.x
             * mul sr1, sr0, sr0
             * exp_sat srFACTOR, -sr1
             */
            if (!shader_instruction_array_insert_at(&program->instructions, pos, 5))
                return VKD3D_ERROR_OUT_OF_MEMORY;
            ret = NULL;

            *ret_pos = pos + 5;

            ssa_temp = program->ssa_count++;
            ssa_temp2 = program->ssa_count++;

            ins = &program->instructions.elements[pos];

            vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_MUL, 1, 2);
            dst_param_init_ssa_float(&ins->dst[0], ssa_temp);
            src_param_init_parameter(&ins->src[0], VKD3D_SHADER_PARAMETER_NAME_FOG_SCALE, VKD3D_DATA_FLOAT);
            vsir_src_param_init(&ins->src[1], VKD3DSPR_INPUT, VKD3D_DATA_FLOAT, 1);
            ins->src[1].reg.idx[0].offset = fog_signature_idx;
            ins->src[1].reg.dimension = VSIR_DIMENSION_VEC4;
            ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);

            vsir_instruction_init_with_params(program, ++ins, &loc, VKD3DSIH_MUL, 1, 2);
            dst_param_init_ssa_float(&ins->dst[0], ssa_temp2);
            src_param_init_ssa_float(&ins->src[0], ssa_temp);
            src_param_init_ssa_float(&ins->src[1], ssa_temp);

            vsir_instruction_init_with_params(program, ++ins, &loc, VKD3DSIH_EXP, 1, 1);
            dst_param_init_ssa_float(&ins->dst[0], ssa_factor);
            ins->dst[0].modifiers = VKD3DSPDM_SATURATE;
            src_param_init_ssa_float(&ins->src[0], ssa_temp2);
            ins->src[0].modifiers = VKD3DSPSM_NEG;
            break;

        default:
            vkd3d_unreachable();
    }

    /* We generate the following code:
     *
     * add sr0, FRAG_COLOUR, -FOG_COLOUR
     * mad oC0, sr0, srFACTOR, FOG_COLOUR
     */

    vsir_instruction_init_with_params(program, ++ins, &loc, VKD3DSIH_ADD, 1, 2);
    dst_param_init_ssa_float4(&ins->dst[0], program->ssa_count++);
    src_param_init_temp_float4(&ins->src[0], colour_temp);
    src_param_init_parameter_vec4(&ins->src[1], VKD3D_SHADER_PARAMETER_NAME_FOG_COLOUR, VKD3D_DATA_FLOAT);
    ins->src[1].modifiers = VKD3DSPSM_NEG;

    vsir_instruction_init_with_params(program, ++ins, &loc, VKD3DSIH_MAD, 1, 3);
    dst_param_init_output(&ins->dst[0], VKD3D_DATA_FLOAT, colour_signature_idx,
            program->output_signature.elements[colour_signature_idx].mask);
    src_param_init_ssa_float4(&ins->src[0], program->ssa_count - 1);
    src_param_init_ssa_float(&ins->src[1], ssa_factor);
    src_param_init_parameter_vec4(&ins->src[2], VKD3D_SHADER_PARAMETER_NAME_FOG_COLOUR, VKD3D_DATA_FLOAT);

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_insert_fragment_fog(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_message_context *message_context = ctx->message_context;
    uint32_t colour_signature_idx, fog_signature_idx, colour_temp;
    const struct vkd3d_shader_parameter1 *mode_parameter = NULL;
    static const struct vkd3d_shader_location no_loc;
    const struct signature_element *fog_element;
    enum vkd3d_shader_fog_fragment_mode mode;
    struct vkd3d_shader_instruction *ins;
    size_t new_pos;
    int ret;

    if (program->shader_version.type != VKD3D_SHADER_TYPE_PIXEL)
        return VKD3D_OK;

    if (!vsir_signature_find_sysval(&program->output_signature, VKD3D_SHADER_SV_TARGET, 0, &colour_signature_idx))
        return VKD3D_OK;

    if (!(mode_parameter = vsir_program_get_parameter(program, VKD3D_SHADER_PARAMETER_NAME_FOG_FRAGMENT_MODE)))
        return VKD3D_OK;

    if (mode_parameter->type != VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
    {
        vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                "Unsupported fog fragment mode parameter type %#x.", mode_parameter->type);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }
    if (mode_parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32)
    {
        vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid fog fragment mode parameter data type %#x.", mode_parameter->data_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    mode = mode_parameter->u.immediate_constant.u.u32;

    if (mode == VKD3D_SHADER_FOG_FRAGMENT_NONE)
        return VKD3D_OK;

    /* Should have been added by vsir_program_add_fog_input(). */
    if (!(fog_element = vsir_signature_find_element_by_name(&program->input_signature, "FOG", 0)))
    {
        ERR("Fog input not found.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    fog_signature_idx = fog_element - program->input_signature.elements;

    /* We're going to be reading from the output, so we need to go
     * through the whole shader and convert it to a temp. */
    colour_temp = program->temp_count++;

    for (size_t i = 0; i < program->instructions.count; ++i)
    {
        ins = &program->instructions.elements[i];

        if (vsir_instruction_is_dcl(ins))
            continue;

        if (ins->opcode == VKD3DSIH_RET)
        {
            if ((ret = insert_fragment_fog_before_ret(program, ins, mode, fog_signature_idx,
                    colour_signature_idx, colour_temp, &new_pos, message_context)) < 0)
                return ret;
            i = new_pos;
            continue;
        }

        for (size_t j = 0; j < ins->dst_count; ++j)
        {
            struct vkd3d_shader_dst_param *dst = &ins->dst[j];

            /* Note we run after I/O normalization. */
            if (dst->reg.type == VKD3DSPR_OUTPUT && dst->reg.idx[0].offset == colour_signature_idx)
            {
                dst->reg.type = VKD3DSPR_TEMP;
                dst->reg.idx[0].offset = colour_temp;
            }
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_add_fog_output(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct shader_signature *signature = &program->output_signature;
    const struct vkd3d_shader_parameter1 *source_parameter;
    uint32_t register_idx = 0;

    if (!is_pre_rasterization_shader(program->shader_version.type))
        return VKD3D_OK;

    if (!(source_parameter = vsir_program_get_parameter(program, VKD3D_SHADER_PARAMETER_NAME_FOG_SOURCE)))
        return VKD3D_OK;

    if (source_parameter->type == VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
    {
        enum vkd3d_shader_fog_source source = source_parameter->u.immediate_constant.u.u32;

        if (source == VKD3D_SHADER_FOG_SOURCE_FOG)
            return VKD3D_OK;

        if (source == VKD3D_SHADER_FOG_SOURCE_FOG_OR_SPECULAR_W
                && !vsir_signature_find_element_by_name(signature, "COLOR", 1))
            return VKD3D_OK;
    }

    if (vsir_signature_find_element_by_name(signature, "FOG", 0))
        return VKD3D_OK;

    for (unsigned int i = 0; i < signature->element_count; ++i)
        register_idx = max(register_idx, signature->elements[i].register_index + 1);

    if (!add_signature_element(signature, "FOG", 0, VKD3DSP_WRITEMASK_0, register_idx, VKD3DSIM_LINEAR))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    return VKD3D_OK;
}

static enum vkd3d_result insert_vertex_fog_before_ret(struct vsir_program *program,
        const struct vkd3d_shader_instruction *ret, enum vkd3d_shader_fog_source source, uint32_t temp,
        uint32_t fog_signature_idx, uint32_t source_signature_idx, size_t *ret_pos)
{
    const struct signature_element *e = &program->output_signature.elements[source_signature_idx];
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    const struct vkd3d_shader_location loc = ret->location;
    size_t pos = ret - instructions->elements;
    struct vkd3d_shader_instruction *ins;

    if (!shader_instruction_array_insert_at(&program->instructions, pos, 2))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ret = NULL;

    ins = &program->instructions.elements[pos];

    /* Write the fog output. */
    vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_MOV, 1, 1);
    dst_param_init_output(&ins->dst[0], VKD3D_DATA_FLOAT, fog_signature_idx, 0x1);
    src_param_init_temp_float4(&ins->src[0], temp);
    if (source == VKD3D_SHADER_FOG_SOURCE_Z)
        ins->src[0].swizzle = VKD3D_SHADER_SWIZZLE(Z, Z, Z, Z);
    else /* Position or specular W. */
        ins->src[0].swizzle = VKD3D_SHADER_SWIZZLE(W, W, W, W);
    ++ins;

    /* Write the position or specular output. */
    vsir_instruction_init_with_params(program, ins, &loc, VKD3DSIH_MOV, 1, 1);
    dst_param_init_output(&ins->dst[0], vkd3d_data_type_from_component_type(e->component_type),
            source_signature_idx, e->mask);
    src_param_init_temp_float4(&ins->src[0], temp);
    ++ins;

    *ret_pos = pos + 2;
    return VKD3D_OK;
}

static enum vkd3d_result vsir_program_insert_vertex_fog(struct vsir_program *program,
        struct vsir_transformation_context *ctx)
{
    struct vkd3d_shader_message_context *message_context = ctx->message_context;
    const struct vkd3d_shader_parameter1 *source_parameter = NULL;
    uint32_t fog_signature_idx, source_signature_idx, temp;
    static const struct vkd3d_shader_location no_loc;
    enum vkd3d_shader_fog_source source;
    const struct signature_element *e;

    if (!is_pre_rasterization_shader(program->shader_version.type))
        return VKD3D_OK;

    if (!(source_parameter = vsir_program_get_parameter(program, VKD3D_SHADER_PARAMETER_NAME_FOG_SOURCE)))
        return VKD3D_OK;

    if (source_parameter->type != VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
    {
        vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                "Unsupported fog source parameter type %#x.", source_parameter->type);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }
    if (source_parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32)
    {
        vkd3d_shader_error(message_context, &no_loc, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid fog source parameter data type %#x.", source_parameter->data_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    source = source_parameter->u.immediate_constant.u.u32;

    TRACE("Fog source %#x.\n", source);

    if (source == VKD3D_SHADER_FOG_SOURCE_FOG)
        return VKD3D_OK;

    if (source == VKD3D_SHADER_FOG_SOURCE_FOG_OR_SPECULAR_W)
    {
        if (program->has_fog || !(e = vsir_signature_find_element_by_name(&program->output_signature, "COLOR", 1)))
            return VKD3D_OK;
        source_signature_idx = e - program->output_signature.elements;
    }
    else
    {
        if (!vsir_signature_find_sysval(&program->output_signature,
                VKD3D_SHADER_SV_POSITION, 0, &source_signature_idx))
        {
            vkd3d_shader_error(ctx->message_context, &no_loc,
                    VKD3D_SHADER_ERROR_VSIR_MISSING_SEMANTIC, "Shader does not write position.");
            return VKD3D_ERROR_INVALID_SHADER;
        }
    }

    if (!(e = vsir_signature_find_element_by_name(&program->output_signature, "FOG", 0)))
    {
        ERR("Fog output not found.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    fog_signature_idx = e - program->output_signature.elements;

    temp = program->temp_count++;

    /* Insert a fog write before each ret, and convert either specular or
     * position output to a temp. */
    for (size_t i = 0; i < program->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &program->instructions.elements[i];

        if (vsir_instruction_is_dcl(ins))
            continue;

        if (ins->opcode == VKD3DSIH_RET)
        {
            size_t new_pos;
            int ret;

            if ((ret = insert_vertex_fog_before_ret(program, ins, source, temp,
                    fog_signature_idx, source_signature_idx, &new_pos)) < 0)
                return ret;
            i = new_pos;
            continue;
        }

        for (size_t j = 0; j < ins->dst_count; ++j)
        {
            struct vkd3d_shader_dst_param *dst = &ins->dst[j];

            /* Note we run after I/O normalization. */
            if (dst->reg.type == VKD3DSPR_OUTPUT && dst->reg.idx[0].offset == source_signature_idx)
            {
                dst->reg.type = VKD3DSPR_TEMP;
                dst->reg.idx[0].offset = temp;
            }
        }
    }

    program->has_fog = true;

    return VKD3D_OK;
}

struct validation_context
{
    struct vkd3d_shader_message_context *message_context;
    const struct vsir_program *program;
    size_t instruction_idx;
    struct vkd3d_shader_location null_location;
    bool invalid_instruction_idx;
    enum vkd3d_result status;
    bool dcl_temps_found;
    enum vkd3d_shader_opcode phase;
    bool inside_block;

    struct validation_context_temp_data
    {
        enum vsir_dimension dimension;
        size_t first_seen;
    } *temps;

    struct validation_context_ssa_data
    {
        enum vsir_dimension dimension;
        enum vkd3d_data_type data_type;
        size_t first_seen;
        uint32_t write_mask;
        uint32_t read_mask;
        size_t first_assigned;
    } *ssas;

    enum vkd3d_shader_opcode *blocks;
    size_t depth;
    size_t blocks_capacity;

    unsigned int outer_tess_idxs[4];
    unsigned int inner_tess_idxs[2];

    struct validation_context_signature_data
    {
        struct validation_context_signature_stream_data
        {
            struct validation_context_signature_register_data
            {
                struct validation_context_signature_component_data
                {
                    const struct signature_element *element;
                } components[VKD3D_VEC4_SIZE];
            } registers[MAX_REG_OUTPUT];
        } streams[VKD3D_MAX_STREAM_COUNT];
    } input_signature_data, output_signature_data, patch_constant_signature_data;
};

static void VKD3D_PRINTF_FUNC(3, 4) validator_error(struct validation_context *ctx,
        enum vkd3d_shader_error error, const char *format, ...)
{
    struct vkd3d_string_buffer buf;
    va_list args;

    vkd3d_string_buffer_init(&buf);

    va_start(args, format);
    vkd3d_string_buffer_vprintf(&buf, format, args);
    va_end(args);

    if (ctx->invalid_instruction_idx)
    {
        vkd3d_shader_error(ctx->message_context, &ctx->null_location, error, "%s", buf.buffer);
        WARN("VSIR validation error: %s\n", buf.buffer);
    }
    else
    {
        const struct vkd3d_shader_instruction *ins = &ctx->program->instructions.elements[ctx->instruction_idx];
        vkd3d_shader_error(ctx->message_context, &ins->location, error,
                "instruction %zu: %s", ctx->instruction_idx + 1, buf.buffer);
        WARN("VSIR validation error: instruction %zu: %s\n", ctx->instruction_idx + 1, buf.buffer);
    }

    vkd3d_string_buffer_cleanup(&buf);

    if (!ctx->status)
        ctx->status = VKD3D_ERROR_INVALID_SHADER;
}

static void vsir_validate_register_without_indices(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    if (reg->idx_count != 0)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a register of type %#x.",
                reg->idx_count, reg->type);
}

enum vsir_signature_type
{
    SIGNATURE_TYPE_INPUT,
    SIGNATURE_TYPE_OUTPUT,
    SIGNATURE_TYPE_PATCH_CONSTANT,
};

enum vsir_io_reg_type
{
    REG_V,
    REG_O,
    REG_VPC,
    REG_VICP,
    REG_VOCP,
    REG_COUNT,
};

enum vsir_phase
{
    PHASE_NONE,
    PHASE_CONTROL_POINT,
    PHASE_FORK,
    PHASE_JOIN,
    PHASE_COUNT,
};

struct vsir_io_register_data
{
    unsigned int flags;
    enum vsir_signature_type signature_type;
    const struct shader_signature *signature;
    unsigned int control_point_count;
};

enum
{
    INPUT_BIT = (1u << 0),
    OUTPUT_BIT = (1u << 1),
    CONTROL_POINT_BIT = (1u << 2),
};

static const struct vsir_io_register_data vsir_sm4_io_register_data
        [VKD3D_SHADER_TYPE_GRAPHICS_COUNT][PHASE_COUNT][REG_COUNT] =
{
    [VKD3D_SHADER_TYPE_PIXEL][PHASE_NONE] =
    {
        [REG_V] = {INPUT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
    [VKD3D_SHADER_TYPE_VERTEX][PHASE_NONE] =
    {
        [REG_V] = {INPUT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
    [VKD3D_SHADER_TYPE_GEOMETRY][PHASE_NONE] =
    {
        [REG_V] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
    [VKD3D_SHADER_TYPE_HULL][PHASE_CONTROL_POINT] =
    {
        [REG_V] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
    [VKD3D_SHADER_TYPE_HULL][PHASE_FORK] =
    {
        [REG_VICP] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_VOCP] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_OUTPUT},
        /* According to MSDN, vpc is not allowed in fork phases. However we
         * don't really distinguish between fork and join phases, so we
         * allow it. */
        [REG_VPC] = {INPUT_BIT, SIGNATURE_TYPE_PATCH_CONSTANT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_PATCH_CONSTANT},
    },
    [VKD3D_SHADER_TYPE_HULL][PHASE_JOIN] =
    {
        [REG_VICP] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_VOCP] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_OUTPUT},
        [REG_VPC] = {INPUT_BIT, SIGNATURE_TYPE_PATCH_CONSTANT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_PATCH_CONSTANT},
    },
    [VKD3D_SHADER_TYPE_DOMAIN][PHASE_NONE] =
    {
        [REG_VICP] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_VPC] = {INPUT_BIT, SIGNATURE_TYPE_PATCH_CONSTANT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
};

static const struct vsir_io_register_data vsir_sm6_io_register_data
        [VKD3D_SHADER_TYPE_GRAPHICS_COUNT][PHASE_COUNT][REG_COUNT] =
{
    [VKD3D_SHADER_TYPE_PIXEL][PHASE_NONE] =
    {
        [REG_V] = {INPUT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
    [VKD3D_SHADER_TYPE_VERTEX][PHASE_NONE] =
    {
        [REG_V] = {INPUT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
    [VKD3D_SHADER_TYPE_GEOMETRY][PHASE_NONE] =
    {
        [REG_V] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
    [VKD3D_SHADER_TYPE_HULL][PHASE_CONTROL_POINT] =
    {
        [REG_V] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {OUTPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
    [VKD3D_SHADER_TYPE_HULL][PHASE_FORK] =
    {
        [REG_V] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_OUTPUT},
        [REG_VPC] = {INPUT_BIT | OUTPUT_BIT, SIGNATURE_TYPE_PATCH_CONSTANT},
    },
    [VKD3D_SHADER_TYPE_HULL][PHASE_JOIN] =
    {
        [REG_V] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_O] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_OUTPUT},
        [REG_VPC] = {INPUT_BIT | OUTPUT_BIT, SIGNATURE_TYPE_PATCH_CONSTANT},
    },
    [VKD3D_SHADER_TYPE_DOMAIN][PHASE_NONE] =
    {
        [REG_V] = {INPUT_BIT | CONTROL_POINT_BIT, SIGNATURE_TYPE_INPUT},
        [REG_VPC] = {INPUT_BIT, SIGNATURE_TYPE_PATCH_CONSTANT},
        [REG_O] = {OUTPUT_BIT, SIGNATURE_TYPE_OUTPUT},
    },
};

static const bool vsir_get_io_register_data(struct validation_context *ctx,
        enum vkd3d_shader_register_type register_type, struct vsir_io_register_data *data)
{
    const struct vsir_io_register_data (*signature_register_data)
            [VKD3D_SHADER_TYPE_GRAPHICS_COUNT][PHASE_COUNT][REG_COUNT];
    enum vsir_io_reg_type io_reg_type;
    enum vsir_phase phase;

    if (ctx->program->shader_version.type >= ARRAY_SIZE(*signature_register_data))
        return NULL;

    if (ctx->program->normalisation_level >= VSIR_NORMALISED_SM6)
        signature_register_data = &vsir_sm6_io_register_data;
    else
        signature_register_data = &vsir_sm4_io_register_data;

    switch (register_type)
    {
        case VKD3DSPR_INPUT:           io_reg_type = REG_V; break;
        case VKD3DSPR_OUTPUT:          io_reg_type = REG_O; break;
        case VKD3DSPR_INCONTROLPOINT:  io_reg_type = REG_VICP; break;
        case VKD3DSPR_OUTCONTROLPOINT: io_reg_type = REG_VOCP; break;
        case VKD3DSPR_PATCHCONST:      io_reg_type = REG_VPC; break;

        default:
            return NULL;
    }

    switch (ctx->phase)
    {
        case VKD3DSIH_HS_CONTROL_POINT_PHASE: phase = PHASE_CONTROL_POINT; break;
        case VKD3DSIH_HS_FORK_PHASE:          phase = PHASE_FORK; break;
        case VKD3DSIH_HS_JOIN_PHASE:          phase = PHASE_JOIN; break;
        case VKD3DSIH_INVALID:                phase = PHASE_NONE; break;

        default:
            vkd3d_unreachable();
    }

    *data = (*signature_register_data)[ctx->program->shader_version.type][phase][io_reg_type];

    if (!(data->flags & (INPUT_BIT | OUTPUT_BIT)))
        return false;

    /* VSIR_NORMALISED_HULL_CONTROL_POINT_IO differs from VSIR_NORMALISED_SM4
     * for just a single flag. So we don't keep a whole copy of it, but just
     * patch SM4 when needed. */
    if (ctx->program->normalisation_level == VSIR_NORMALISED_HULL_CONTROL_POINT_IO
            && ctx->program->shader_version.type == VKD3D_SHADER_TYPE_HULL
            && phase == PHASE_CONTROL_POINT && io_reg_type == REG_O)
    {
        VKD3D_ASSERT(!(data->flags & CONTROL_POINT_BIT));
        data->flags |= CONTROL_POINT_BIT;
    }

    switch (data->signature_type)
    {
        case SIGNATURE_TYPE_INPUT:
            data->signature = &ctx->program->input_signature;
            data->control_point_count = ctx->program->input_control_point_count;
            return true;

        case SIGNATURE_TYPE_OUTPUT:
            data->signature = &ctx->program->output_signature;
            data->control_point_count = ctx->program->output_control_point_count;
            return true;

        case SIGNATURE_TYPE_PATCH_CONSTANT:
            data->signature = &ctx->program->patch_constant_signature;
            return true;

        default:
            vkd3d_unreachable();
    }
}

static void vsir_validate_io_register(struct validation_context *ctx, const struct vkd3d_shader_register *reg)
{
    unsigned int control_point_index, control_point_count;
    const struct shader_signature *signature;
    struct vsir_io_register_data io_reg_data;
    bool has_control_point;

    if (!vsir_get_io_register_data(ctx, reg->type, &io_reg_data))
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                "Invalid usage of register type %#x.", reg->type);
        return;
    }

    signature = io_reg_data.signature;
    has_control_point = io_reg_data.flags & CONTROL_POINT_BIT;
    control_point_count = io_reg_data.control_point_count;

    if (ctx->program->normalisation_level < VSIR_NORMALISED_SM6)
    {
        /* Indices are [register] or [control point, register]. Both are
         * allowed to have a relative address. */
        unsigned int expected_idx_count = 1 + !!has_control_point;

        control_point_index = 0;

        if (reg->idx_count != expected_idx_count)
        {
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                    "Invalid index count %u for a register of type %#x.",
                    reg->idx_count, reg->type);
            return;
        }
    }
    else
    {
        struct signature_element *element;
        unsigned int expected_idx_count;
        unsigned int signature_idx;
        bool is_array = false;

        /* If the signature element is not an array, indices are
         * [signature] or [control point, signature]. If the signature
         * element is an array, indices are [array, signature] or
         * [array, control point, signature]. In any case `signature' is
         * not allowed to have a relative address, while the others are.
         */
        if (reg->idx_count < 1)
        {
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                    "Invalid index count %u for a register of type %#x.",
                    reg->idx_count, reg->type);
            return;
        }

        if (reg->idx[reg->idx_count - 1].rel_addr)
        {
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                    "Non-NULL relative address for the signature index of a register of type %#x.",
                    reg->type);
            return;
        }

        signature_idx = reg->idx[reg->idx_count - 1].offset;

        if (signature_idx >= signature->element_count)
        {
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                    "Signature index %u exceeds the signature size %u in a register of type %#x.",
                    signature_idx, signature->element_count, reg->type);
            return;
        }

        element = &signature->elements[signature_idx];
        if (element->register_count > 1 || vsir_sysval_semantic_is_tess_factor(element->sysval_semantic))
            is_array = true;

        expected_idx_count = 1 + !!has_control_point + !!is_array;
        control_point_index = !!is_array;

        if (reg->idx_count != expected_idx_count)
        {
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                    "Invalid index count %u for a register of type %#x.",
                    reg->idx_count, reg->type);
            return;
        }

        if (is_array && !reg->idx[0].rel_addr && reg->idx[0].offset >= element->register_count)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                    "Array index %u exceeds the signature element register count %u in a register of type %#x.",
                    reg->idx[0].offset, element->register_count, reg->type);
    }

    if (has_control_point && !reg->idx[control_point_index].rel_addr
            && reg->idx[control_point_index].offset >= control_point_count)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Control point index %u exceeds the control point count %u in a register of type %#x.",
                reg->idx[control_point_index].offset, control_point_count, reg->type);
}

static void vsir_validate_temp_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    struct validation_context_temp_data *data;

    if (reg->idx_count != 1)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a TEMP register.",
                reg->idx_count);
        return;
    }

    if (reg->idx[0].rel_addr)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Non-NULL relative address for a TEMP register.");

    if (reg->idx[0].offset >= ctx->program->temp_count)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "TEMP register index %u exceeds the maximum count %u.",
                reg->idx[0].offset, ctx->program->temp_count);
        return;
    }

    data = &ctx->temps[reg->idx[0].offset];

    if (reg->dimension == VSIR_DIMENSION_NONE)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid dimension NONE for a TEMP register.");
        return;
    }

    /* TEMP registers can be scalar or vec4, provided that
     * each individual register always appears with the same
     * dimension. */
    if (data->dimension == VSIR_DIMENSION_NONE)
    {
        data->dimension = reg->dimension;
        data->first_seen = ctx->instruction_idx;
    }
    else if (data->dimension != reg->dimension)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid dimension %#x for a TEMP register: "
                "it has already been seen with dimension %#x at instruction %zu.",
                reg->dimension, data->dimension, data->first_seen);
    }
}

static void vsir_validate_rastout_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    if (reg->idx_count != 1)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a RASTOUT register.",
                reg->idx_count);
        return;
    }

    if (reg->idx[0].rel_addr)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Non-NULL relative address for a RASTOUT register.");

    if (reg->idx[0].offset >= 3)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Invalid offset for a RASTOUT register.");
}

static void vsir_validate_misctype_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    if (reg->idx_count != 1)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a MISCTYPE register.",
                reg->idx_count);
        return;
    }

    if (reg->idx[0].rel_addr)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Non-NULL relative address for a MISCTYPE register.");

    if (reg->idx[0].offset >= 2)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Invalid offset for a MISCTYPE register.");
}

static void vsir_validate_label_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    if (reg->precision != VKD3D_SHADER_REGISTER_PRECISION_DEFAULT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_PRECISION,
                "Invalid precision %#x for a LABEL register.", reg->precision);

    if (reg->data_type != VKD3D_DATA_UNUSED)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid data type %#x for a LABEL register.", reg->data_type);

    if (reg->dimension != VSIR_DIMENSION_NONE)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid dimension %#x for a LABEL register.", reg->dimension);

    if (reg->idx_count != 1)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a LABEL register.", reg->idx_count);
        return;
    }

    if (reg->idx[0].rel_addr)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Non-NULL relative address for a LABEL register.");

    /* Index == 0 is invalid, but it is temporarily allowed
     * for intermediate stages. Once we support validation
     * dialects we can selectively check for that. */
    if (reg->idx[0].offset > ctx->program->block_count)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "LABEL register index %u exceeds the maximum count %u.",
                reg->idx[0].offset, ctx->program->block_count);
}

static void vsir_validate_descriptor_indices(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg, enum vkd3d_shader_descriptor_type type, const char *name)
{
    const struct vkd3d_shader_descriptor_info1 *descriptor;

    if (reg->idx[0].rel_addr)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Non-NULL indirect address for the ID of a register of type \"%s\".", name);

    if (!ctx->program->has_descriptor_info)
        return;

    if (!(descriptor = vkd3d_shader_find_descriptor(&ctx->program->descriptors, type, reg->idx[0].offset)))
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "No matching descriptor found for register %s%u.", name, reg->idx[0].offset);
        return;
    }

    if (!reg->idx[1].rel_addr && (reg->idx[1].offset < descriptor->register_index
            || reg->idx[1].offset - descriptor->register_index >= descriptor->count))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Register index %u doesn't belong to the range [%u, %u] for register %s%u.",
                reg->idx[1].offset, descriptor->register_index,
                descriptor->register_index + descriptor->count - 1, name, reg->idx[0].offset);
}

static void vsir_validate_constbuffer_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    if (reg->precision != VKD3D_SHADER_REGISTER_PRECISION_DEFAULT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_PRECISION,
                "Invalid precision %#x for a CONSTBUFFER register.", reg->precision);

    if (reg->dimension != VSIR_DIMENSION_VEC4)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid dimension %#x for a CONSTBUFFER register.", reg->dimension);

    if (reg->idx_count != 3)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a CONSTBUFFER register.", reg->idx_count);
        return;
    }

    vsir_validate_descriptor_indices(ctx, reg, VKD3D_SHADER_DESCRIPTOR_TYPE_CBV, "cb");
}

static void vsir_validate_sampler_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    if (reg->precision != VKD3D_SHADER_REGISTER_PRECISION_DEFAULT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_PRECISION,
                "Invalid precision %#x for a SAMPLER register.", reg->precision);

    if (reg->data_type != VKD3D_DATA_UNUSED)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid data type %#x for a SAMPLER register.", reg->data_type);

    /* VEC4 is allowed in gather operations. */
    if (reg->dimension == VSIR_DIMENSION_SCALAR)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid dimension SCALAR for a SAMPLER register.");

    if (reg->idx_count != 2)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a SAMPLER register.", reg->idx_count);
        return;
    }

    vsir_validate_descriptor_indices(ctx, reg, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, "s");
}

static void vsir_validate_resource_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    if (reg->precision != VKD3D_SHADER_REGISTER_PRECISION_DEFAULT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_PRECISION,
                "Invalid precision %#x for a RESOURCE register.", reg->precision);

    if (reg->data_type != VKD3D_DATA_UNUSED)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid data type %#x for a RESOURCE register.", reg->data_type);

    if (reg->dimension != VSIR_DIMENSION_VEC4)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid dimension %#x for a RESOURCE register.", reg->dimension);

    if (reg->idx_count != 2)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a RESOURCE register.", reg->idx_count);
        return;
    }

    vsir_validate_descriptor_indices(ctx, reg, VKD3D_SHADER_DESCRIPTOR_TYPE_SRV, "t");
}

static void vsir_validate_uav_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    if (reg->precision != VKD3D_SHADER_REGISTER_PRECISION_DEFAULT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_PRECISION,
                "Invalid precision %#x for a UAV register.",
                reg->precision);

    if (reg->data_type != VKD3D_DATA_UNUSED)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid data type %#x for a UAV register.",
                reg->data_type);

    /* NONE is allowed in counter operations. */
    if (reg->dimension == VSIR_DIMENSION_SCALAR)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid dimension %#x for a UAV register.",
                reg->dimension);

    if (reg->idx_count != 2)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a UAV register.",
                reg->idx_count);
        return;
    }

    vsir_validate_descriptor_indices(ctx, reg, VKD3D_SHADER_DESCRIPTOR_TYPE_UAV, "u");
}

static void vsir_validate_ssa_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    struct validation_context_ssa_data *data;

    if (reg->idx_count != 1)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u for a SSA register.",
                reg->idx_count);
        return;
    }

    if (reg->idx[0].rel_addr)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Non-NULL relative address for a SSA register.");

    if (reg->idx[0].offset >= ctx->program->ssa_count)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "SSA register index %u exceeds the maximum count %u.",
                reg->idx[0].offset, ctx->program->ssa_count);
        return;
    }

    data = &ctx->ssas[reg->idx[0].offset];

    if (reg->dimension == VSIR_DIMENSION_NONE)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid dimension NONE for a SSA register.");
        return;
    }

    /* SSA registers can be scalar or vec4, provided that each
     * individual register always appears with the same
     * dimension. */
    if (data->dimension == VSIR_DIMENSION_NONE)
    {
        data->dimension = reg->dimension;
        data->data_type = reg->data_type;
        data->first_seen = ctx->instruction_idx;
    }
    else
    {
        if (data->dimension != reg->dimension)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                    "Invalid dimension %#x for a SSA register: "
                    "it has already been seen with dimension %#x at instruction %zu.",
                    reg->dimension, data->dimension, data->first_seen);

        if (data_type_is_64_bit(data->data_type) != data_type_is_64_bit(reg->data_type))
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                    "Invalid data type %#x for a SSA register: "
                    "it has already been seen with data type %#x at instruction %zu.",
                    reg->data_type, data->data_type, data->first_seen);
    }
}

static void vsir_validate_src_param(struct validation_context *ctx,
        const struct vkd3d_shader_src_param *src);

static void vsir_validate_register(struct validation_context *ctx,
        const struct vkd3d_shader_register *reg)
{
    unsigned int i;

    if (reg->type >= VKD3DSPR_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE, "Invalid register type %#x.",
                reg->type);

    if (reg->precision >= VKD3D_SHADER_REGISTER_PRECISION_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_PRECISION, "Invalid register precision %#x.",
                reg->precision);

    if (reg->data_type >= VKD3D_DATA_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE, "Invalid register data type %#x.",
                reg->data_type);

    if (reg->dimension >= VSIR_DIMENSION_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION, "Invalid register dimension %#x.",
                reg->dimension);

    if (reg->idx_count > ARRAY_SIZE(reg->idx))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT, "Invalid register index count %u.",
                reg->idx_count);

    for (i = 0; i < min(reg->idx_count, ARRAY_SIZE(reg->idx)); ++i)
    {
        const struct vkd3d_shader_src_param *param = reg->idx[i].rel_addr;
        if (param)
        {
            vsir_validate_src_param(ctx, param);

            switch (param->reg.type)
            {
                case VKD3DSPR_TEMP:
                case VKD3DSPR_SSA:
                case VKD3DSPR_ADDR:
                case VKD3DSPR_LOOP:
                case VKD3DSPR_OUTPOINTID:
                    break;

                default:
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                            "Invalid register type %#x for a relative address parameter.",
                            param->reg.type);
                    break;
            }
        }
    }

    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            vsir_validate_temp_register(ctx, reg);
            break;

        case VKD3DSPR_INPUT:
            vsir_validate_io_register(ctx, reg);
            break;

        case VKD3DSPR_RASTOUT:
            vsir_validate_rastout_register(ctx, reg);
            break;

        case VKD3DSPR_OUTPUT:
            vsir_validate_io_register(ctx, reg);
            break;

        case VKD3DSPR_DEPTHOUT:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_MISCTYPE:
            vsir_validate_misctype_register(ctx, reg);
            break;

        case VKD3DSPR_LABEL:
            vsir_validate_label_register(ctx, reg);
            break;

        case VKD3DSPR_IMMCONST:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_IMMCONST64:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_CONSTBUFFER:
            vsir_validate_constbuffer_register(ctx, reg);
            break;

        case VKD3DSPR_PRIMID:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_NULL:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_SAMPLER:
            vsir_validate_sampler_register(ctx, reg);
            break;

        case VKD3DSPR_RESOURCE:
            vsir_validate_resource_register(ctx, reg);
            break;

        case VKD3DSPR_UAV:
            vsir_validate_uav_register(ctx, reg);
            break;

        case VKD3DSPR_OUTPOINTID:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_FORKINSTID:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_JOININSTID:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_INCONTROLPOINT:
            vsir_validate_io_register(ctx, reg);
            break;

        case VKD3DSPR_OUTCONTROLPOINT:
            vsir_validate_io_register(ctx, reg);
            break;

        case VKD3DSPR_PATCHCONST:
            vsir_validate_io_register(ctx, reg);
            break;

        case VKD3DSPR_TESSCOORD:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_THREADID:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_THREADGROUPID:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_LOCALTHREADID:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_LOCALTHREADINDEX:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_COVERAGE:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_SAMPLEMASK:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_GSINSTID:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_DEPTHOUTGE:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_DEPTHOUTLE:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_OUTSTENCILREF:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_SSA:
            vsir_validate_ssa_register(ctx, reg);
            break;

        case VKD3DSPR_WAVELANECOUNT:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        case VKD3DSPR_WAVELANEINDEX:
            vsir_validate_register_without_indices(ctx, reg);
            break;

        default:
            break;
    }
}

static void vsir_validate_io_dst_param(struct validation_context *ctx,
        const struct vkd3d_shader_dst_param *dst)
{
    struct vsir_io_register_data io_reg_data;

    if (!vsir_get_io_register_data(ctx, dst->reg.type, &io_reg_data) || !(io_reg_data.flags & OUTPUT_BIT))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                "Invalid register type %#x used as destination parameter.", dst->reg.type);
}

static void vsir_validate_dst_param(struct validation_context *ctx,
        const struct vkd3d_shader_dst_param *dst)
{
    vsir_validate_register(ctx, &dst->reg);

    if (dst->write_mask & ~VKD3DSP_WRITEMASK_ALL)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK, "Destination has invalid write mask %#x.",
                dst->write_mask);

    switch (dst->reg.dimension)
    {
        case VSIR_DIMENSION_SCALAR:
            if (dst->write_mask != VKD3DSP_WRITEMASK_0)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK, "Scalar destination has invalid write mask %#x.",
                    dst->write_mask);
            break;

        case VSIR_DIMENSION_VEC4:
            if (dst->write_mask == 0)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK, "Vec4 destination has empty write mask.");
            break;

        default:
            if (dst->write_mask != 0)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK, "Destination of dimension %u has invalid write mask %#x.",
                    dst->reg.dimension, dst->write_mask);
            break;
    }

    if (dst->modifiers & ~VKD3DSPDM_MASK)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_MODIFIERS, "Destination has invalid modifiers %#x.",
                dst->modifiers);

    switch (dst->shift)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 13:
        case 14:
        case 15:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SHIFT, "Destination has invalid shift %#x.",
                    dst->shift);
    }

    switch (dst->reg.type)
    {
        case VKD3DSPR_SSA:
            if (dst->reg.dimension == VSIR_DIMENSION_VEC4
                    && dst->write_mask != VKD3DSP_WRITEMASK_0
                    && dst->write_mask != (VKD3DSP_WRITEMASK_0 | VKD3DSP_WRITEMASK_1)
                    && dst->write_mask != (VKD3DSP_WRITEMASK_0 | VKD3DSP_WRITEMASK_1 | VKD3DSP_WRITEMASK_2)
                    && dst->write_mask != VKD3DSP_WRITEMASK_ALL)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK,
                        "SSA register has invalid write mask %#x.", dst->write_mask);

            if (dst->reg.idx[0].offset < ctx->program->ssa_count)
            {
                struct validation_context_ssa_data *data = &ctx->ssas[dst->reg.idx[0].offset];

                if (data->write_mask == 0)
                {
                    data->write_mask = dst->write_mask;
                    data->first_assigned = ctx->instruction_idx;
                }
                else
                {
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SSA_USAGE,
                            "SSA register is already assigned at instruction %zu.",
                            data->first_assigned);
                }
            }
            break;

        case VKD3DSPR_IMMCONST:
        case VKD3DSPR_IMMCONST64:
        case VKD3DSPR_CONSTBUFFER:
        case VKD3DSPR_IMMCONSTBUFFER:
        case VKD3DSPR_SAMPLER:
        case VKD3DSPR_RESOURCE:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid %#x register used as destination parameter.", dst->reg.type);
            break;

        case VKD3DSPR_INPUT:
            vsir_validate_io_dst_param(ctx, dst);
            break;

        case VKD3DSPR_OUTPUT:
            vsir_validate_io_dst_param(ctx, dst);
            break;

        case VKD3DSPR_INCONTROLPOINT:
            vsir_validate_io_dst_param(ctx, dst);
            break;

        case VKD3DSPR_OUTCONTROLPOINT:
            vsir_validate_io_dst_param(ctx, dst);
            break;

        case VKD3DSPR_PATCHCONST:
            vsir_validate_io_dst_param(ctx, dst);
            break;

        default:
            break;
    }
}

static void vsir_validate_io_src_param(struct validation_context *ctx,
        const struct vkd3d_shader_src_param *src)
{
    struct vsir_io_register_data io_reg_data;

    if (!vsir_get_io_register_data(ctx, src->reg.type, &io_reg_data) || !(io_reg_data.flags & INPUT_BIT))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                "Invalid register type %#x used as source parameter.", src->reg.type);
}

static void vsir_validate_src_param(struct validation_context *ctx,
        const struct vkd3d_shader_src_param *src)
{
    vsir_validate_register(ctx, &src->reg);

    if (src->swizzle & ~0x03030303u)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SWIZZLE, "Source has invalid swizzle %#x.",
                src->swizzle);

    if (src->reg.dimension != VSIR_DIMENSION_VEC4 && src->swizzle != 0)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SWIZZLE, "Source of dimension %u has invalid swizzle %#x.",
                src->reg.dimension, src->swizzle);

    if (src->modifiers >= VKD3DSPSM_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_MODIFIERS, "Source has invalid modifiers %#x.",
                src->modifiers);

    switch (src->reg.type)
    {
        case VKD3DSPR_SSA:
            if (src->reg.idx[0].offset < ctx->program->ssa_count)
            {
                struct validation_context_ssa_data *data = &ctx->ssas[src->reg.idx[0].offset];
                unsigned int i;

                for (i = 0; i < VKD3D_VEC4_SIZE; ++i)
                    data->read_mask |= (1u << vsir_swizzle_get_component(src->swizzle, i));
            }
            break;

        case VKD3DSPR_NULL:
        case VKD3DSPR_DEPTHOUT:
        case VKD3DSPR_DEPTHOUTGE:
        case VKD3DSPR_DEPTHOUTLE:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register of type %#x used as source parameter.", src->reg.type);
            break;

        case VKD3DSPR_INPUT:
            vsir_validate_io_src_param(ctx, src);
            break;

        case VKD3DSPR_OUTPUT:
            vsir_validate_io_src_param(ctx, src);
            break;

        case VKD3DSPR_INCONTROLPOINT:
            vsir_validate_io_src_param(ctx, src);
            break;

        case VKD3DSPR_OUTCONTROLPOINT:
            vsir_validate_io_src_param(ctx, src);
            break;

        case VKD3DSPR_PATCHCONST:
            vsir_validate_io_src_param(ctx, src);
            break;

        default:
            break;
    }
}

static void vsir_validate_dst_count(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction, unsigned int count)
{
    if (instruction->dst_count != count)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DEST_COUNT,
                "Invalid destination count %u for an instruction of type %#x, expected %u.",
                        instruction->dst_count, instruction->opcode, count);
}

static void vsir_validate_src_count(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction, unsigned int count)
{
    if (instruction->src_count != count)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SOURCE_COUNT,
                "Invalid source count %u for an instruction of type %#x, expected %u.",
                instruction->src_count, instruction->opcode, count);
}

static bool vsir_validate_src_min_count(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction, unsigned int count)
{
    if (instruction->src_count < count)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SOURCE_COUNT,
                "Invalid source count %u for an instruction of type %#x, expected at least %u.",
                instruction->src_count, instruction->opcode, count);
        return false;
    }

    return true;
}

static bool vsir_validate_src_max_count(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction, unsigned int count)
{
    if (instruction->src_count > count)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SOURCE_COUNT,
                "Invalid source count %u for an instruction of type %#x, expected at most %u.",
                instruction->src_count, instruction->opcode, count);
        return false;
    }

    return true;
}

static const char * const signature_type_names[] =
{
    [SIGNATURE_TYPE_INPUT] = "input",
    [SIGNATURE_TYPE_OUTPUT] = "output",
    [SIGNATURE_TYPE_PATCH_CONSTANT] = "patch constant",
};

#define PS_BIT (1u << VKD3D_SHADER_TYPE_PIXEL)
#define VS_BIT (1u << VKD3D_SHADER_TYPE_VERTEX)
#define GS_BIT (1u << VKD3D_SHADER_TYPE_GEOMETRY)
#define HS_BIT (1u << VKD3D_SHADER_TYPE_HULL)
#define DS_BIT (1u << VKD3D_SHADER_TYPE_DOMAIN)
#define CS_BIT (1u << VKD3D_SHADER_TYPE_COMPUTE)

static const struct sysval_validation_data_element
{
    unsigned int input;
    unsigned int output;
    unsigned int patch_constant;
    enum vkd3d_shader_component_type data_type;
    unsigned int component_count;
}
sysval_validation_data[] =
{
    [VKD3D_SHADER_SV_POSITION] = {PS_BIT | GS_BIT | HS_BIT | DS_BIT, VS_BIT | GS_BIT | HS_BIT | DS_BIT, 0,
            VKD3D_SHADER_COMPONENT_FLOAT, 4},
    [VKD3D_SHADER_SV_CLIP_DISTANCE] = {PS_BIT | GS_BIT | HS_BIT | DS_BIT, PS_BIT | VS_BIT | GS_BIT | HS_BIT | DS_BIT, 0,
            VKD3D_SHADER_COMPONENT_FLOAT, 4},
    [VKD3D_SHADER_SV_CULL_DISTANCE] = {PS_BIT | GS_BIT | HS_BIT | DS_BIT, PS_BIT | VS_BIT | GS_BIT | HS_BIT | DS_BIT, 0,
            VKD3D_SHADER_COMPONENT_FLOAT, 4},
    [VKD3D_SHADER_SV_TESS_FACTOR_QUADEDGE] = {0, 0, HS_BIT | DS_BIT, VKD3D_SHADER_COMPONENT_FLOAT, 1},
    [VKD3D_SHADER_SV_TESS_FACTOR_QUADINT] = {0, 0, HS_BIT | DS_BIT, VKD3D_SHADER_COMPONENT_FLOAT, 1},
    [VKD3D_SHADER_SV_TESS_FACTOR_TRIEDGE] = {0, 0, HS_BIT | DS_BIT, VKD3D_SHADER_COMPONENT_FLOAT, 1},
    [VKD3D_SHADER_SV_TESS_FACTOR_TRIINT] = {0, 0, HS_BIT | DS_BIT, VKD3D_SHADER_COMPONENT_FLOAT, 1},
    [VKD3D_SHADER_SV_TESS_FACTOR_LINEDET] = {0, 0, HS_BIT | DS_BIT, VKD3D_SHADER_COMPONENT_FLOAT, 1},
    [VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN] = {0, 0, HS_BIT | DS_BIT, VKD3D_SHADER_COMPONENT_FLOAT, 1},
};

static void vsir_validate_signature_element(struct validation_context *ctx,
        const struct shader_signature *signature, struct validation_context_signature_data *signature_data,
        enum vsir_signature_type signature_type, unsigned int idx)
{
    enum vkd3d_tessellator_domain expected_tess_domain = VKD3D_TESSELLATOR_DOMAIN_INVALID;
    bool integer_type = false, is_outer = false, is_gs_output, require_index = true;
    const char *signature_type_name = signature_type_names[signature_type];
    const struct signature_element *element = &signature->elements[idx];
    unsigned int semantic_index_max = 0, i, j;

    if (element->register_count == 0)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Invalid zero register count.", idx, signature_type_name);

    if (ctx->program->normalisation_level < VSIR_NORMALISED_SM6 && element->register_count != 1)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Invalid register count %u.", idx, signature_type_name,
                element->register_count);

    if (element->register_index != UINT_MAX && (element->register_index >= MAX_REG_OUTPUT
            || MAX_REG_OUTPUT - element->register_index < element->register_count))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Invalid register index %u and count %u.",
                idx, signature_type_name, element->register_index, element->register_count);

    is_gs_output = ctx->program->shader_version.type == VKD3D_SHADER_TYPE_GEOMETRY
            && signature_type == SIGNATURE_TYPE_OUTPUT;
    if (element->stream_index >= VKD3D_MAX_STREAM_COUNT || (element->stream_index != 0 && !is_gs_output))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Invalid stream index %u.",
                idx, signature_type_name, element->stream_index);

    if (element->mask == 0 || (element->mask & ~0xf))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Invalid mask %#x.", idx, signature_type_name, element->mask);

    if (!vkd3d_bitmask_is_contiguous(element->mask))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Non-contiguous mask %#x.",
                idx, signature_type_name, element->mask);

    if (ctx->program->normalisation_level >= VSIR_NORMALISED_SM4)
    {
        if ((element->used_mask & element->mask) != element->used_mask)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                    "element %u of %s signature: Invalid usage mask %#x with mask %#x.",
                    idx, signature_type_name, element->used_mask, element->mask);
    }
    else
    {
        if (element->used_mask & ~0xf)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                    "element %u of %s signature: Invalid usage mask %#x.",
                    idx, signature_type_name, element->used_mask);
    }

    switch (element->sysval_semantic)
    {
        case VKD3D_SHADER_SV_NONE:
        case VKD3D_SHADER_SV_TARGET:
            break;

        case VKD3D_SHADER_SV_POSITION:
        case VKD3D_SHADER_SV_CLIP_DISTANCE:
        case VKD3D_SHADER_SV_CULL_DISTANCE:
        case VKD3D_SHADER_SV_RENDER_TARGET_ARRAY_INDEX:
        case VKD3D_SHADER_SV_VIEWPORT_ARRAY_INDEX:
        case VKD3D_SHADER_SV_VERTEX_ID:
        case VKD3D_SHADER_SV_PRIMITIVE_ID:
        case VKD3D_SHADER_SV_INSTANCE_ID:
        case VKD3D_SHADER_SV_IS_FRONT_FACE:
        case VKD3D_SHADER_SV_SAMPLE_INDEX:
        case VKD3D_SHADER_SV_DEPTH:
        case VKD3D_SHADER_SV_COVERAGE:
        case VKD3D_SHADER_SV_DEPTH_GREATER_EQUAL:
        case VKD3D_SHADER_SV_DEPTH_LESS_EQUAL:
        case VKD3D_SHADER_SV_STENCIL_REF:
            require_index = false;
            break;

        case VKD3D_SHADER_SV_TESS_FACTOR_QUADEDGE:
            expected_tess_domain = VKD3D_TESSELLATOR_DOMAIN_QUAD;
            semantic_index_max = 4;
            is_outer = true;
            break;

        case VKD3D_SHADER_SV_TESS_FACTOR_QUADINT:
            expected_tess_domain = VKD3D_TESSELLATOR_DOMAIN_QUAD;
            semantic_index_max = 2;
            is_outer = false;
            break;

        case VKD3D_SHADER_SV_TESS_FACTOR_TRIEDGE:
            expected_tess_domain = VKD3D_TESSELLATOR_DOMAIN_TRIANGLE;
            semantic_index_max = 3;
            is_outer = true;
            break;

        case VKD3D_SHADER_SV_TESS_FACTOR_TRIINT:
            expected_tess_domain = VKD3D_TESSELLATOR_DOMAIN_TRIANGLE;
            semantic_index_max = 1;
            is_outer = false;
            break;

        case VKD3D_SHADER_SV_TESS_FACTOR_LINEDET:
        case VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN:
            expected_tess_domain = VKD3D_TESSELLATOR_DOMAIN_LINE;
            semantic_index_max = 2;
            is_outer = true;
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                    "element %u of %s signature: Invalid system value semantic %#x.",
                    idx, signature_type_name, element->sysval_semantic);
            break;
    }

    if (require_index && element->register_index == UINT_MAX)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: System value semantic %#x requires a register index.",
                idx, signature_type_name, element->sysval_semantic);

    if (expected_tess_domain != VKD3D_TESSELLATOR_DOMAIN_INVALID)
    {
        if (signature_type != SIGNATURE_TYPE_PATCH_CONSTANT)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                    "element %u of %s signature: System value semantic %#x is only valid "
                    "in the patch constant signature.",
                    idx, signature_type_name, element->sysval_semantic);

        if (ctx->program->tess_domain != expected_tess_domain)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                    "element %u of %s signature: Invalid system value semantic %#x for tessellator domain %#x.",
                    idx, signature_type_name, element->sysval_semantic, ctx->program->tess_domain);

        if (element->semantic_index >= semantic_index_max)
        {
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                    "element %u of %s signature: Invalid semantic index %u for system value semantic %#x.",
                    idx, signature_type_name, element->semantic_index, element->sysval_semantic);
        }
        else
        {
            unsigned int *idx_pos = &(is_outer ? ctx->outer_tess_idxs : ctx->inner_tess_idxs)[element->semantic_index];

            if (*idx_pos != ~0u)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "element %u of %s signature: Duplicate semantic index %u for system value semantic %#x.",
                        idx, signature_type_name, element->semantic_index, element->sysval_semantic);
            else
                *idx_pos = idx;
        }
    }

    if (element->sysval_semantic < ARRAY_SIZE(sysval_validation_data))
    {
        const struct sysval_validation_data_element *data = &sysval_validation_data[element->sysval_semantic];

        if (data->input || data->output || data->patch_constant)
        {
            unsigned int mask;

            switch (signature_type)
            {
                case SIGNATURE_TYPE_INPUT:
                    mask = data->input;
                    break;

                case SIGNATURE_TYPE_OUTPUT:
                    mask = data->output;
                    break;

                case SIGNATURE_TYPE_PATCH_CONSTANT:
                    mask = data->patch_constant;
                    break;

                default:
                    vkd3d_unreachable();
            }

            if (!(mask & (1u << ctx->program->shader_version.type)))
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "element %u of %s signature: Invalid system value semantic %#x.",
                        idx, signature_type_name, element->sysval_semantic);
        }

        if (data->component_count != 0)
        {
            if (element->component_type != data->data_type)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "element %u of %s signature: Invalid data type %#x for system value semantic %#x.",
                        idx, signature_type_name, element->component_type, element->sysval_semantic);

            if (vsir_write_mask_component_count(element->mask) > data->component_count)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "element %u of %s signature: Invalid mask %#x for system value semantic %#x.",
                        idx, signature_type_name, element->mask, element->sysval_semantic);
        }
    }

    switch (element->component_type)
    {
        case VKD3D_SHADER_COMPONENT_INT:
        case VKD3D_SHADER_COMPONENT_UINT:
        case VKD3D_SHADER_COMPONENT_INT16:
        case VKD3D_SHADER_COMPONENT_UINT16:
            integer_type = true;
            break;

        case VKD3D_SHADER_COMPONENT_FLOAT:
        case VKD3D_SHADER_COMPONENT_FLOAT16:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                    "element %u of %s signature: Invalid component type %#x.",
                    idx, signature_type_name, element->component_type);
            break;
    }

    if (element->min_precision >= VKD3D_SHADER_MINIMUM_PRECISION_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Invalid minimum precision %#x.",
                idx, signature_type_name, element->min_precision);

    if (element->interpolation_mode >= VKD3DSIM_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Invalid interpolation mode %#x.",
                idx, signature_type_name, element->interpolation_mode);

    if (integer_type && element->interpolation_mode != VKD3DSIM_NONE
            && element->interpolation_mode != VKD3DSIM_CONSTANT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "element %u of %s signature: Invalid interpolation mode %#x for integer component type.",
                idx, signature_type_name, element->interpolation_mode);

    if (element->stream_index >= VKD3D_MAX_STREAM_COUNT || !require_index)
        return;

    for (i = element->register_index; i < MAX_REG_OUTPUT
            && i - element->register_index < element->register_count; ++i)
    {
        struct validation_context_signature_stream_data *stream_data = &signature_data->streams[element->stream_index];
        struct validation_context_signature_register_data *register_data = &stream_data->registers[i];

        for (j = 0; j < VKD3D_VEC4_SIZE; ++j)
        {
            struct validation_context_signature_component_data *component_data = &register_data->components[j];

            if (!(element->mask & (1u << j)))
                continue;

            if (!component_data->element)
                component_data->element = element;
            else
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "element %u of %s signature: Conflict with element %zu.",
                        idx, signature_type_name, component_data->element - signature->elements);
        }
    }
}

static const unsigned int allowed_signature_phases[] =
{
    [SIGNATURE_TYPE_INPUT]          = PS_BIT | VS_BIT | GS_BIT | HS_BIT | DS_BIT,
    [SIGNATURE_TYPE_OUTPUT]         = PS_BIT | VS_BIT | GS_BIT | HS_BIT | DS_BIT,
    [SIGNATURE_TYPE_PATCH_CONSTANT] = HS_BIT | DS_BIT,
};

static void vsir_validate_signature(struct validation_context *ctx, const struct shader_signature *signature,
        struct validation_context_signature_data *signature_data, enum vsir_signature_type signature_type)
{
    unsigned int i;

    if (signature->element_count != 0 && !(allowed_signature_phases[signature_type]
            & (1u << ctx->program->shader_version.type)))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                "Unexpected %s signature.", signature_type_names[signature_type]);

    for (i = 0; i < signature->element_count; ++i)
        vsir_validate_signature_element(ctx, signature, signature_data, signature_type, i);

    if (signature_type == SIGNATURE_TYPE_PATCH_CONSTANT)
    {
        const struct signature_element *first_element, *element;
        unsigned int expected_outer_count = 0;
        unsigned int expected_inner_count = 0;

        switch (ctx->program->tess_domain)
        {
            case VKD3D_TESSELLATOR_DOMAIN_QUAD:
                expected_outer_count = 4;
                expected_inner_count = 2;
                break;

            case VKD3D_TESSELLATOR_DOMAIN_TRIANGLE:
                expected_outer_count = 3;
                expected_inner_count = 1;
                break;

            case VKD3D_TESSELLATOR_DOMAIN_LINE:
                expected_outer_count = 2;
                expected_inner_count = 0;
                break;

            default:
                break;
        }

        /* After I/O normalisation tessellation factors are merged in a single array. */
        if (ctx->program->normalisation_level >= VSIR_NORMALISED_SM6)
        {
            expected_outer_count = min(1, expected_outer_count);
            expected_inner_count = min(1, expected_inner_count);
        }

        first_element = NULL;
        for (i = 0; i < expected_outer_count; ++i)
        {
            if (ctx->outer_tess_idxs[i] == ~0u)
            {
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "Missing outer system value semantic %u.", i);
            }
            else
            {
                element = &signature->elements[ctx->outer_tess_idxs[i]];

                if (!first_element)
                {
                    first_element = element;
                    continue;
                }

                if (element->register_index != first_element->register_index + i)
                {
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                            "Invalid register index %u for outer system value semantic %u, expected %u.",
                            element->register_index, i, first_element->register_index + i);
                }

                if (element->mask != first_element->mask)
                {
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK,
                            "Invalid mask %#x for outer system value semantic %u, expected %#x.",
                            element->mask, i, first_element->mask);
                }
            }
        }

        first_element = NULL;
        for (i = 0; i < expected_inner_count; ++i)
        {
            if (ctx->inner_tess_idxs[i] == ~0u)
            {
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "Missing inner system value semantic %u.", i);
            }
            else
            {
                element = &signature->elements[ctx->inner_tess_idxs[i]];

                if (!first_element)
                {
                    first_element = element;
                    continue;
                }

                if (element->register_index != first_element->register_index + i)
                {
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                            "Invalid register index %u for inner system value semantic %u, expected %u.",
                            element->register_index, i, first_element->register_index + i);
                }

                if (element->mask != first_element->mask)
                {
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK,
                            "Invalid mask %#x for inner system value semantic %u, expected %#x.",
                            element->mask, i, first_element->mask);
                }
            }
        }
    }
}

static void vsir_validate_descriptors(struct validation_context *ctx)
{
    const struct vkd3d_shader_scan_descriptor_info1 *descriptors = &ctx->program->descriptors;
    unsigned int i;

    for (i = 0; i < descriptors->descriptor_count; ++i)
    {
        const struct vkd3d_shader_descriptor_info1 *descriptor = &descriptors->descriptors[i];

        if (descriptor->type >= VKD3D_SHADER_DESCRIPTOR_TYPE_COUNT)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DESCRIPTOR_TYPE,
                    "Descriptor %u has invalid type %#x.", i, descriptor->type);

        if (descriptor->resource_type >= VKD3D_SHADER_RESOURCE_TYPE_COUNT)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_RESOURCE_TYPE,
                    "Descriptor %u has invalid resource type %#x.", i, descriptor->resource_type);
        else if ((descriptor->resource_type == VKD3D_SHADER_RESOURCE_NONE)
                != (descriptor->type == VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER))
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_RESOURCE_TYPE,
                    "Descriptor %u has invalid resource type %#x for descriptor type %#x.",
                    i, descriptor->resource_type, descriptor->type);

        if (descriptor->resource_data_type >= VKD3D_DATA_COUNT)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                    "Descriptor %u has invalid resource data type %#x.", i, descriptor->resource_data_type);
        else if ((descriptor->resource_data_type == VKD3D_DATA_UNUSED)
                != (descriptor->type == VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER))
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                    "Descriptor %u has invalid resource data type %#x for descriptor type %#x.",
                    i, descriptor->resource_data_type, descriptor->type);

        if (!descriptor->count || (descriptor->count > UINT_MAX - descriptor->register_index
                && descriptor->count != UINT_MAX))
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DESCRIPTOR_COUNT,
                    "Descriptor %u has invalid descriptor count %u starting at index %u.",
                    i, descriptor->count, descriptor->register_index);
    }
}

static const char *name_from_cf_type(enum vsir_control_flow_type type)
{
    switch (type)
    {
        case VSIR_CF_STRUCTURED:
            return "structured";
        case VSIR_CF_BLOCKS:
            return "block-based";
        default:
            vkd3d_unreachable();
    }
}

static void vsir_validate_cf_type(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction, enum vsir_control_flow_type expected_type)
{
    if (ctx->program->cf_type != expected_type)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW, "Invalid instruction %#x in %s shader.",
                instruction->opcode, name_from_cf_type(ctx->program->cf_type));
}

static void vsir_validator_push_block(struct validation_context *ctx, enum vkd3d_shader_opcode opcode)
{
    if (!vkd3d_array_reserve((void **)&ctx->blocks, &ctx->blocks_capacity, ctx->depth + 1, sizeof(*ctx->blocks)))
    {
        ctx->status = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }
    ctx->blocks[ctx->depth++] = opcode;
}

static void vsir_validate_hull_shader_phase(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (ctx->program->shader_version.type != VKD3D_SHADER_TYPE_HULL)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_HANDLER,
                "Phase instruction %#x is only valid in a hull shader.",
                instruction->opcode);
    if (ctx->depth != 0)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "Phase instruction %#x must appear to top level.",
                instruction->opcode);
    ctx->phase = instruction->opcode;
    ctx->dcl_temps_found = false;
}

static void vsir_validate_branch(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    size_t i;

    vsir_validate_cf_type(ctx, instruction, VSIR_CF_BLOCKS);
    vsir_validate_dst_count(ctx, instruction, 0);

    if (!vsir_validate_src_min_count(ctx, instruction, 1))
        return;

    if (vsir_register_is_label(&instruction->src[0].reg))
    {
        /* Unconditional branch: parameters are jump label,
         * optional merge label, optional continue label. */
        vsir_validate_src_max_count(ctx, instruction, 3);

        for (i = 0; i < instruction->src_count; ++i)
        {
            if (!vsir_register_is_label(&instruction->src[i].reg))
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                        "Invalid register of type %#x in unconditional BRANCH instruction, expected LABEL.",
                        instruction->src[i].reg.type);
        }
    }
    else
    {
        /* Conditional branch: parameters are condition, true
         * jump label, false jump label, optional merge label,
         * optional continue label. */
        vsir_validate_src_min_count(ctx, instruction, 3);
        vsir_validate_src_max_count(ctx, instruction, 5);

        for (i = 1; i < instruction->src_count; ++i)
        {
            if (!vsir_register_is_label(&instruction->src[i].reg))
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                        "Invalid register of type %#x in conditional BRANCH instruction, expected LABEL.",
                        instruction->src[i].reg.type);
        }
    }

    ctx->inside_block = false;
}

static void vsir_validate_dcl_gs_instances(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (!instruction->declaration.count || instruction->declaration.count > 32)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_GS, "GS instance count %u is invalid.",
                instruction->declaration.count);
}

static void vsir_validate_dcl_hs_max_tessfactor(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    /* Exclude non-finite values. */
    if (!(instruction->declaration.max_tessellation_factor >= 1.0f
            && instruction->declaration.max_tessellation_factor <= 64.0f))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_TESSELLATION,
                "Max tessellation factor %f is invalid.",
                instruction->declaration.max_tessellation_factor);
}

static void vsir_validate_dcl_index_range(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    unsigned int i, j, base_register_idx, effective_write_mask = 0, control_point_count, first_component = UINT_MAX;
    const struct vkd3d_shader_index_range *range = &instruction->declaration.index_range;
    enum vkd3d_shader_sysval_semantic sysval = ~0u;
    const struct shader_signature *signature;
    struct vsir_io_register_data io_reg_data;
    bool has_control_point;

    if (ctx->program->normalisation_level >= VSIR_NORMALISED_SM6)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_HANDLER,
                "DCL_INDEX_RANGE is not allowed with fully normalised input/output.");
        return;
    }

    if (range->dst.modifiers != VKD3DSPDM_NONE)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_MODIFIERS,
                "Invalid modifier %#x on a DCL_INDEX_RANGE destination parameter.", range->dst.modifiers);

    if (range->dst.shift != 0)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SHIFT,
                "Invalid shift %u on a DCL_INDEX_RANGE destination parameter.", range->dst.shift);

    if (!vsir_get_io_register_data(ctx, range->dst.reg.type, &io_reg_data))
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                "Invalid register type %#x in DCL_INDEX_RANGE instruction.",
                range->dst.reg.type);
        return;
    }

    signature = io_reg_data.signature;
    has_control_point = io_reg_data.flags & CONTROL_POINT_BIT;
    control_point_count = io_reg_data.control_point_count;

    if (range->dst.reg.idx_count != 1 + !!has_control_point)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX_COUNT,
                "Invalid index count %u in DCL_INDEX_RANGE instruction.",
                range->dst.reg.idx_count);
        return;
    }

    if (range->dst.reg.idx[0].rel_addr || (has_control_point && range->dst.reg.idx[1].rel_addr))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                "Invalid relative address in DCL_INDEX_RANGE instruction.");

    if (has_control_point)
    {
        if (range->dst.reg.idx[0].offset != control_point_count)
        {
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                    "Invalid control point index %u in DCL_INDEX_RANGE instruction, expected %u.",
                    range->dst.reg.idx[0].offset, control_point_count);
        }

        base_register_idx = range->dst.reg.idx[1].offset;
    }
    else
    {
        base_register_idx = range->dst.reg.idx[0].offset;
    }

    if (range->register_count < 2)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_RANGE,
                "Invalid register count %u in DCL_INDEX_RANGE instruction, expected at least 2.",
                range->register_count);
        return;
    }

    /* Check that for each register in the range the write mask intersects at
     * most one (and possibly zero) signature elements. Keep track of the union
     * of all signature element masks. */
    for (i = 0; i < range->register_count; ++i)
    {
        bool found = false;

        for (j = 0; j < signature->element_count; ++j)
        {
            const struct signature_element *element = &signature->elements[j];

            if (base_register_idx + i != element->register_index || !(range->dst.write_mask & element->mask))
                continue;

            if (found)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK,
                        "Invalid write mask %#x on a DCL_INDEX_RANGE destination parameter.",
                        range->dst.write_mask);

            found = true;

            if (first_component == UINT_MAX)
                first_component = vsir_write_mask_get_component_idx(element->mask);
            else if (first_component != vsir_write_mask_get_component_idx(element->mask))
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK,
                        "Signature masks are not left-aligned within a DCL_INDEX_RANGE.");

            effective_write_mask |= element->mask;
        }
    }

    /* Check again to have at most one intersection for each register, but this
     * time using the effective write mask. Also check that we have stabilized,
     * i.e., the effective write mask now contains all the signature element
     * masks. This important for being able to merge all the signature elements
     * in a single one without conflicts (there is no hard reason why we
     * couldn't support an effective write mask that stabilizes after more
     * iterations, but the code would be more complicated, and we avoid that if
     * we can). */
    for (i = 0; i < range->register_count; ++i)
    {
        bool found = false;

        for (j = 0; j < signature->element_count; ++j)
        {
            const struct signature_element *element = &signature->elements[j];

            if (base_register_idx + i != element->register_index || !(effective_write_mask & element->mask))
                continue;

            if (element->sysval_semantic != VKD3D_SHADER_SV_NONE
                    && !vsir_sysval_semantic_is_tess_factor(element->sysval_semantic))
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "Invalid sysval semantic %#x on a signature element touched by DCL_INDEX_RANGE.",
                        element->sysval_semantic);

            if (sysval == ~0u)
            {
                sysval = element->sysval_semantic;
                /* Line density and line detail can be arrayed together. */
                if (sysval == VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN)
                    sysval = VKD3D_SHADER_SV_TESS_FACTOR_LINEDET;
            }
            else
            {
                if (sysval != element->sysval_semantic)
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                            "Inconsistent sysval semantic %#x on a signature element touched by DCL_INDEX_RANGE, "
                            "%#x was already seen.",
                            element->sysval_semantic, sysval);
            }

            if (found)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK,
                        "Invalid write mask %#x on a DCL_INDEX_RANGE destination parameter.",
                        range->dst.write_mask);

            found = true;

            if (~effective_write_mask & element->mask)
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK,
                        "Invalid write mask %#x on a signature element touched by a "
                        "DCL_INDEX_RANGE instruction with effective write mask %#x.",
                        element->mask, effective_write_mask);

            if (first_component != vsir_write_mask_get_component_idx(element->mask))
                validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_WRITE_MASK,
                        "Signature element masks are not left-aligned within a DCL_INDEX_RANGE.");
        }
    }

    VKD3D_ASSERT(sysval != ~0u);
}

static void vsir_validate_dcl_input(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->declaration.dst.reg.type)
    {
        /* Signature input registers. */
        case VKD3DSPR_INPUT:
        case VKD3DSPR_INCONTROLPOINT:
        case VKD3DSPR_OUTCONTROLPOINT:
        case VKD3DSPR_PATCHCONST:
        /* Non-signature input registers. */
        case VKD3DSPR_PRIMID:
        case VKD3DSPR_FORKINSTID:
        case VKD3DSPR_JOININSTID:
        case VKD3DSPR_THREADID:
        case VKD3DSPR_THREADGROUPID:
        case VKD3DSPR_LOCALTHREADID:
        case VKD3DSPR_LOCALTHREADINDEX:
        case VKD3DSPR_COVERAGE:
        case VKD3DSPR_TESSCOORD:
        case VKD3DSPR_OUTPOINTID:
        case VKD3DSPR_GSINSTID:
        case VKD3DSPR_WAVELANECOUNT:
        case VKD3DSPR_WAVELANEINDEX:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register type %#x in instruction DCL_INPUT.",
                    instruction->declaration.dst.reg.type);
    }
}

static void vsir_validate_dcl_input_primitive(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (instruction->declaration.primitive_type.type == VKD3D_PT_UNDEFINED
            || instruction->declaration.primitive_type.type >= VKD3D_PT_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_GS, "GS input primitive %u is invalid.",
                instruction->declaration.primitive_type.type);
}

static void vsir_validate_dcl_input_ps(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->declaration.dst.reg.type)
    {
        case VKD3DSPR_INPUT:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register type %#x in instruction DCL_INPUT_PS.",
                    instruction->declaration.dst.reg.type);
    }
}

static void vsir_validate_dcl_input_ps_sgv(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->declaration.register_semantic.reg.reg.type)
    {
        case VKD3DSPR_INPUT:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register type %#x in instruction DCL_INPUT_PS_SGV.",
                    instruction->declaration.register_semantic.reg.reg.type);
    }
}

static void vsir_validate_dcl_input_ps_siv(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->declaration.register_semantic.reg.reg.type)
    {
        case VKD3DSPR_INPUT:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register type %#x in instruction DCL_INPUT_PS_SIV.",
                    instruction->declaration.register_semantic.reg.reg.type);
    }
}

static void vsir_validate_dcl_input_sgv(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->declaration.register_semantic.reg.reg.type)
    {
        case VKD3DSPR_INPUT:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register type %#x in instruction DCL_INPUT_SGV.",
                    instruction->declaration.register_semantic.reg.reg.type);
    }
}

static void vsir_validate_dcl_input_siv(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->declaration.register_semantic.reg.reg.type)
    {
        case VKD3DSPR_INPUT:
        case VKD3DSPR_PATCHCONST:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register type %#x in instruction DCL_INPUT_SIV.",
                    instruction->declaration.register_semantic.reg.reg.type);
    }
}

static void vsir_validate_dcl_output(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->declaration.dst.reg.type)
    {
        /* Signature output registers. */
        case VKD3DSPR_OUTPUT:
        case VKD3DSPR_PATCHCONST:
        /* Non-signature output registers. */
        case VKD3DSPR_DEPTHOUT:
        case VKD3DSPR_SAMPLEMASK:
        case VKD3DSPR_DEPTHOUTGE:
        case VKD3DSPR_DEPTHOUTLE:
        case VKD3DSPR_OUTSTENCILREF:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register type %#x in instruction DCL_OUTPUT.",
                    instruction->declaration.dst.reg.type);
    }
}

static void vsir_validate_dcl_output_control_point_count(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (!instruction->declaration.count || instruction->declaration.count > 32)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_TESSELLATION,
                "Output control point count %u is invalid.",
                instruction->declaration.count);
}

static void vsir_validate_dcl_output_siv(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->declaration.register_semantic.reg.reg.type)
    {
        case VKD3DSPR_OUTPUT:
        case VKD3DSPR_PATCHCONST:
            break;

        default:
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid register type %#x in instruction DCL_OUTPUT_SIV.",
                    instruction->declaration.register_semantic.reg.reg.type);
    }
}

static void vsir_validate_dcl_output_topology(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (instruction->declaration.primitive_type.type == VKD3D_PT_UNDEFINED
            || instruction->declaration.primitive_type.type >= VKD3D_PT_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_GS, "GS output primitive %u is invalid.",
                instruction->declaration.primitive_type.type);
}

static void vsir_validate_dcl_temps(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (ctx->dcl_temps_found)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_DUPLICATE_DCL_TEMPS,
                "Duplicate DCL_TEMPS instruction.");
    if (instruction->declaration.count > ctx->program->temp_count)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DCL_TEMPS,
                "Invalid DCL_TEMPS count %u, expected at most %u.",
                instruction->declaration.count, ctx->program->temp_count);
    ctx->dcl_temps_found = true;
}

static void vsir_validate_dcl_tessellator_domain(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (instruction->declaration.tessellator_domain == VKD3D_TESSELLATOR_DOMAIN_INVALID
            || instruction->declaration.tessellator_domain >= VKD3D_TESSELLATOR_DOMAIN_COUNT)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_TESSELLATION,
                "Tessellator domain %#x is invalid.", instruction->declaration.tessellator_domain);

    if (instruction->declaration.tessellator_domain != ctx->program->tess_domain)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_TESSELLATION,
                "DCL_TESSELLATOR_DOMAIN argument %#x doesn't match the shader tessellator domain %#x.",
                instruction->declaration.tessellator_domain, ctx->program->tess_domain);
}

static void vsir_validate_dcl_tessellator_output_primitive(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (!instruction->declaration.tessellator_output_primitive
            || instruction->declaration.tessellator_output_primitive
            > VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_TESSELLATION,
                "Tessellator output primitive %#x is invalid.",
                instruction->declaration.tessellator_output_primitive);
}

static void vsir_validate_dcl_tessellator_partitioning(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (!instruction->declaration.tessellator_partitioning
            || instruction->declaration.tessellator_partitioning
            > VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_TESSELLATION,
                "Tessellator partitioning %#x is invalid.",
                instruction->declaration.tessellator_partitioning);
}

static void vsir_validate_dcl_vertices_out(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    if (instruction->declaration.count > 1024)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_GS, "GS output vertex count %u is invalid.",
                instruction->declaration.count);
}

static void vsir_validate_else(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    if (ctx->depth == 0 || ctx->blocks[ctx->depth - 1] != VKD3DSIH_IF)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "ELSE instruction doesn't terminate IF block.");
    else
        ctx->blocks[ctx->depth - 1] = VKD3DSIH_ELSE;
}

static void vsir_validate_endif(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    if (ctx->depth == 0 || (ctx->blocks[ctx->depth - 1] != VKD3DSIH_IF
            && ctx->blocks[ctx->depth - 1] != VKD3DSIH_ELSE))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "ENDIF instruction doesn't terminate IF/ELSE block.");
    else
        --ctx->depth;
}

static void vsir_validate_endloop(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    if (ctx->depth == 0 || ctx->blocks[ctx->depth - 1] != VKD3DSIH_LOOP)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "ENDLOOP instruction doesn't terminate LOOP block.");
    else
        --ctx->depth;
}

static void vsir_validate_endrep(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    if (ctx->depth == 0 || ctx->blocks[ctx->depth - 1] != VKD3DSIH_REP)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "ENDREP instruction doesn't terminate REP block.");
    else
        --ctx->depth;
}

static void vsir_validate_endswitch(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    if (ctx->depth == 0 || ctx->blocks[ctx->depth - 1] != VKD3DSIH_SWITCH)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "ENDSWITCH instruction doesn't terminate SWITCH block.");
    else
        --ctx->depth;
}

static void vsir_validate_if(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    vsir_validator_push_block(ctx, VKD3DSIH_IF);
}

static void vsir_validate_ifc(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    vsir_validator_push_block(ctx, VKD3DSIH_IF);
}

static void vsir_validate_label(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_BLOCKS);
    if (instruction->src_count >= 1 && !vsir_register_is_label(&instruction->src[0].reg))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                "Invalid register of type %#x in a LABEL instruction, expected LABEL.",
                instruction->src[0].reg.type);

    if (ctx->inside_block)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                "Invalid LABEL instruction inside a block.");
    ctx->inside_block = true;
}

static void vsir_validate_loop(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    vsir_validate_src_count(ctx, instruction, ctx->program->shader_version.major <= 3 ? 2 : 0);
    vsir_validator_push_block(ctx, VKD3DSIH_LOOP);
}

static void vsir_validate_nop(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
}

static void vsir_validate_phi(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    unsigned int i, incoming_count;

    vsir_validate_cf_type(ctx, instruction, VSIR_CF_BLOCKS);

    vsir_validate_src_min_count(ctx, instruction, 2);

    if (instruction->src_count % 2 != 0)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SOURCE_COUNT,
                "Invalid source count %u for a PHI instruction, it must be an even number.",
                instruction->src_count);
    incoming_count = instruction->src_count / 2;

    for (i = 0; i < incoming_count; ++i)
    {
        unsigned int value_idx = 2 * i;
        unsigned int label_idx = 2 * i + 1;

        if (!register_is_constant_or_undef(&instruction->src[value_idx].reg)
                && !register_is_ssa(&instruction->src[value_idx].reg))
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid value register for incoming %u of type %#x in PHI instruction, "
                    "expected SSA, IMMCONST or IMMCONST64.", i, instruction->src[value_idx].reg.type);

        if (instruction->src[value_idx].reg.dimension != VSIR_DIMENSION_SCALAR)
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                    "Invalid value dimension %#x for incoming %u in PHI instruction, expected scalar.",
                    instruction->src[value_idx].reg.dimension, i);

        if (!vsir_register_is_label(&instruction->src[label_idx].reg))
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid label register for case %u of type %#x in PHI instruction, "
                    "expected LABEL.", i, instruction->src[value_idx].reg.type);
    }

    if (instruction->dst_count < 1)
        return;

    if (!register_is_ssa(&instruction->dst[0].reg))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                "Invalid destination of type %#x in PHI instruction, expected SSA.",
                instruction->dst[0].reg.type);

    if (instruction->dst[0].reg.dimension != VSIR_DIMENSION_SCALAR)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_DIMENSION,
                "Invalid destination dimension %#x in PHI instruction, expected scalar.",
                instruction->dst[0].reg.dimension);

    if (instruction->dst[0].modifiers != VKD3DSPDM_NONE)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_MODIFIERS,
                "Invalid modifiers %#x for the destination of a PHI instruction, expected none.",
                instruction->dst[0].modifiers);

    if (instruction->dst[0].shift != 0)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SHIFT,
                "Invalid shift %#x for the destination of a PHI instruction, expected none.",
                instruction->dst[0].shift);
}

static void vsir_validate_rep(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    vsir_validator_push_block(ctx, VKD3DSIH_REP);
}

static void vsir_validate_ret(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    ctx->inside_block = false;
}

static void vsir_validate_switch(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction)
{
    vsir_validate_cf_type(ctx, instruction, VSIR_CF_STRUCTURED);
    vsir_validator_push_block(ctx, VKD3DSIH_SWITCH);
}

static void vsir_validate_switch_monolithic(struct validation_context *ctx,
        const struct vkd3d_shader_instruction *instruction)
{
    unsigned int i, case_count;

    vsir_validate_cf_type(ctx, instruction, VSIR_CF_BLOCKS);

    /* Parameters are source, default label, merge label and
     * then pairs of constant value and case label. */

    if (!vsir_validate_src_min_count(ctx, instruction, 3))
        return;

    if (instruction->src_count % 2 != 1)
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SOURCE_COUNT,
                "Invalid source count %u for a monolithic SWITCH instruction, it must be an odd number.",
                instruction->src_count);

    if (!vsir_register_is_label(&instruction->src[1].reg))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                "Invalid default label register of type %#x in monolithic SWITCH instruction, expected LABEL.",
                instruction->src[1].reg.type);

    if (!vsir_register_is_label(&instruction->src[2].reg))
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                "Invalid merge label register of type %#x in monolithic SWITCH instruction, expected LABEL.",
                instruction->src[2].reg.type);

    case_count = (instruction->src_count - 3) / 2;

    for (i = 0; i < case_count; ++i)
    {
        unsigned int value_idx = 3 + 2 * i;
        unsigned int label_idx = 3 + 2 * i + 1;

        if (!register_is_constant(&instruction->src[value_idx].reg))
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid value register for case %u of type %#x in monolithic SWITCH instruction, "
                    "expected IMMCONST or IMMCONST64.", i, instruction->src[value_idx].reg.type);

        if (!vsir_register_is_label(&instruction->src[label_idx].reg))
            validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_REGISTER_TYPE,
                    "Invalid label register for case %u of type %#x in monolithic SWITCH instruction, "
                    "expected LABEL.", i, instruction->src[value_idx].reg.type);
    }

    ctx->inside_block = false;
}

struct vsir_validator_instruction_desc
{
    unsigned int dst_param_count;
    unsigned int src_param_count;
    void (*validate)(struct validation_context *ctx, const struct vkd3d_shader_instruction *instruction);
};

static const struct vsir_validator_instruction_desc vsir_validator_instructions[] =
{
    [VKD3DSIH_BRANCH] =                           {0, ~0u, vsir_validate_branch},
    [VKD3DSIH_HS_CONTROL_POINT_PHASE] =           {0,   0, vsir_validate_hull_shader_phase},
    [VKD3DSIH_HS_DECLS] =                         {0,   0, vsir_validate_hull_shader_phase},
    [VKD3DSIH_HS_FORK_PHASE] =                    {0,   0, vsir_validate_hull_shader_phase},
    [VKD3DSIH_HS_JOIN_PHASE] =                    {0,   0, vsir_validate_hull_shader_phase},
    [VKD3DSIH_DCL_GS_INSTANCES] =                 {0,   0, vsir_validate_dcl_gs_instances},
    [VKD3DSIH_DCL_HS_MAX_TESSFACTOR] =            {0,   0, vsir_validate_dcl_hs_max_tessfactor},
    [VKD3DSIH_DCL_INDEX_RANGE] =                  {0,   0, vsir_validate_dcl_index_range},
    [VKD3DSIH_DCL_INPUT] =                        {0,   0, vsir_validate_dcl_input},
    [VKD3DSIH_DCL_INPUT_PRIMITIVE] =              {0,   0, vsir_validate_dcl_input_primitive},
    [VKD3DSIH_DCL_INPUT_PS] =                     {0,   0, vsir_validate_dcl_input_ps},
    [VKD3DSIH_DCL_INPUT_PS_SGV] =                 {0,   0, vsir_validate_dcl_input_ps_sgv},
    [VKD3DSIH_DCL_INPUT_PS_SIV] =                 {0,   0, vsir_validate_dcl_input_ps_siv},
    [VKD3DSIH_DCL_INPUT_SGV] =                    {0,   0, vsir_validate_dcl_input_sgv},
    [VKD3DSIH_DCL_INPUT_SIV] =                    {0,   0, vsir_validate_dcl_input_siv},
    [VKD3DSIH_DCL_OUTPUT] =                       {0,   0, vsir_validate_dcl_output},
    [VKD3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT] =   {0,   0, vsir_validate_dcl_output_control_point_count},
    [VKD3DSIH_DCL_OUTPUT_SIV] =                   {0,   0, vsir_validate_dcl_output_siv},
    [VKD3DSIH_DCL_OUTPUT_TOPOLOGY] =              {0,   0, vsir_validate_dcl_output_topology},
    [VKD3DSIH_DCL_TEMPS] =                        {0,   0, vsir_validate_dcl_temps},
    [VKD3DSIH_DCL_TESSELLATOR_DOMAIN] =           {0,   0, vsir_validate_dcl_tessellator_domain},
    [VKD3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE] = {0,   0, vsir_validate_dcl_tessellator_output_primitive},
    [VKD3DSIH_DCL_TESSELLATOR_PARTITIONING] =     {0,   0, vsir_validate_dcl_tessellator_partitioning},
    [VKD3DSIH_DCL_VERTICES_OUT] =                 {0,   0, vsir_validate_dcl_vertices_out},
    [VKD3DSIH_ELSE] =                             {0,   0, vsir_validate_else},
    [VKD3DSIH_ENDIF] =                            {0,   0, vsir_validate_endif},
    [VKD3DSIH_ENDLOOP] =                          {0,   0, vsir_validate_endloop},
    [VKD3DSIH_ENDREP] =                           {0,   0, vsir_validate_endrep},
    [VKD3DSIH_ENDSWITCH] =                        {0,   0, vsir_validate_endswitch},
    [VKD3DSIH_IF] =                               {0,   1, vsir_validate_if},
    [VKD3DSIH_IFC] =                              {0,   2, vsir_validate_ifc},
    [VKD3DSIH_LABEL] =                            {0,   1, vsir_validate_label},
    [VKD3DSIH_LOOP] =                             {0, ~0u, vsir_validate_loop},
    [VKD3DSIH_NOP] =                              {0,   0, vsir_validate_nop},
    [VKD3DSIH_PHI] =                              {1, ~0u, vsir_validate_phi},
    [VKD3DSIH_REP] =                              {0,   1, vsir_validate_rep},
    [VKD3DSIH_RET] =                              {0,   0, vsir_validate_ret},
    [VKD3DSIH_SWITCH] =                           {0,   1, vsir_validate_switch},
    [VKD3DSIH_SWITCH_MONOLITHIC] =                {0, ~0u, vsir_validate_switch_monolithic},
};

static void vsir_validate_instruction(struct validation_context *ctx)
{
    const struct vkd3d_shader_version *version = &ctx->program->shader_version;
    const struct vkd3d_shader_instruction *instruction;
    size_t i;

    instruction = &ctx->program->instructions.elements[ctx->instruction_idx];

    for (i = 0; i < instruction->dst_count; ++i)
        vsir_validate_dst_param(ctx, &instruction->dst[i]);

    for (i = 0; i < instruction->src_count; ++i)
        vsir_validate_src_param(ctx, &instruction->src[i]);

    if (instruction->opcode >= VKD3DSIH_INVALID)
    {
        validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_HANDLER, "Invalid instruction handler %#x.",
                instruction->opcode);
    }

    if (version->type == VKD3D_SHADER_TYPE_HULL && ctx->phase == VKD3DSIH_INVALID)
    {
        switch (instruction->opcode)
        {
            case VKD3DSIH_NOP:
            case VKD3DSIH_HS_DECLS:
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                break;

            default:
                if (!vsir_instruction_is_dcl(instruction))
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_HANDLER,
                            "Instruction %#x appear before any phase instruction in a hull shader.",
                            instruction->opcode);
                break;
        }
    }

    if (ctx->program->cf_type == VSIR_CF_BLOCKS && !ctx->inside_block)
    {
        switch (instruction->opcode)
        {
            case VKD3DSIH_NOP:
            case VKD3DSIH_LABEL:
            case VKD3DSIH_HS_DECLS:
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                break;

            default:
                if (!vsir_instruction_is_dcl(instruction))
                    validator_error(ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW,
                            "Invalid instruction %#x outside any block.",
                            instruction->opcode);
                break;
        }
    }

    if (instruction->opcode < ARRAY_SIZE(vsir_validator_instructions))
    {
        const struct vsir_validator_instruction_desc *desc;

        desc = &vsir_validator_instructions[instruction->opcode];

        if (desc->validate)
        {
            if (desc->dst_param_count != ~0u)
                vsir_validate_dst_count(ctx, instruction, desc->dst_param_count);
            if (desc->src_param_count != ~0u)
                vsir_validate_src_count(ctx, instruction, desc->src_param_count);
            desc->validate(ctx, instruction);
        }
    }
}

enum vkd3d_result vsir_program_validate(struct vsir_program *program, uint64_t config_flags,
        const char *source_name, struct vkd3d_shader_message_context *message_context)
{
    struct validation_context ctx =
    {
        .message_context = message_context,
        .program = program,
        .null_location = {.source_name = source_name},
        .status = VKD3D_OK,
        .phase = VKD3DSIH_INVALID,
        .invalid_instruction_idx = true,
        .outer_tess_idxs[0] = ~0u,
        .outer_tess_idxs[1] = ~0u,
        .outer_tess_idxs[2] = ~0u,
        .outer_tess_idxs[3] = ~0u,
        .inner_tess_idxs[0] = ~0u,
        .inner_tess_idxs[1] = ~0u,
    };
    unsigned int i;

    if (!(config_flags & VKD3D_SHADER_CONFIG_FLAG_FORCE_VALIDATION))
        return VKD3D_OK;

    switch (program->shader_version.type)
    {
        case VKD3D_SHADER_TYPE_HULL:
        case VKD3D_SHADER_TYPE_DOMAIN:
            if (program->tess_domain == VKD3D_TESSELLATOR_DOMAIN_INVALID
                    || program->tess_domain >= VKD3D_TESSELLATOR_DOMAIN_COUNT)
                validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_TESSELLATION,
                        "Invalid tessellation domain %#x.", program->tess_domain);
            break;

        default:
            if (program->patch_constant_signature.element_count != 0)
                validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "Patch constant signature is only valid for hull and domain shaders.");

            if (program->tess_domain != VKD3D_TESSELLATOR_DOMAIN_INVALID)
                validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_TESSELLATION,
                        "Invalid tessellation domain %#x.", program->tess_domain);
    }

    switch (program->shader_version.type)
    {
        case VKD3D_SHADER_TYPE_DOMAIN:
            break;

        case VKD3D_SHADER_TYPE_HULL:
        case VKD3D_SHADER_TYPE_GEOMETRY:
            if (program->input_control_point_count == 0)
                validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "Invalid zero input control point count.");
            break;

        default:
            if (program->input_control_point_count != 0)
                validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "Invalid input control point count %u.",
                        program->input_control_point_count);
    }

    switch (program->shader_version.type)
    {
        case VKD3D_SHADER_TYPE_HULL:
            break;

        default:
            if (program->output_control_point_count != 0)
                validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "Invalid output control point count %u.",
                        program->output_control_point_count);
    }

    vsir_validate_signature(&ctx, &program->input_signature,
            &ctx.input_signature_data, SIGNATURE_TYPE_INPUT);
    vsir_validate_signature(&ctx, &program->output_signature,
            &ctx.output_signature_data, SIGNATURE_TYPE_OUTPUT);
    vsir_validate_signature(&ctx, &program->patch_constant_signature,
            &ctx.patch_constant_signature_data, SIGNATURE_TYPE_PATCH_CONSTANT);

    for (i = 0; i < sizeof(program->io_dcls) * CHAR_BIT; ++i)
    {
        if (!bitmap_is_set(program->io_dcls, i))
            continue;

        switch (i)
        {
            /* Input registers */
            case VKD3DSPR_PRIMID:
            case VKD3DSPR_FORKINSTID:
            case VKD3DSPR_JOININSTID:
            case VKD3DSPR_THREADID:
            case VKD3DSPR_THREADGROUPID:
            case VKD3DSPR_LOCALTHREADID:
            case VKD3DSPR_LOCALTHREADINDEX:
            case VKD3DSPR_COVERAGE:
            case VKD3DSPR_TESSCOORD:
            case VKD3DSPR_OUTPOINTID:
            case VKD3DSPR_GSINSTID:
            case VKD3DSPR_WAVELANECOUNT:
            case VKD3DSPR_WAVELANEINDEX:
            /* Output registers */
            case VKD3DSPR_DEPTHOUT:
            case VKD3DSPR_SAMPLEMASK:
            case VKD3DSPR_DEPTHOUTGE:
            case VKD3DSPR_DEPTHOUTLE:
            case VKD3DSPR_OUTSTENCILREF:
                break;

            default:
                validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SIGNATURE,
                        "Invalid input/output declaration %u.", i);
        }
    }

    vsir_validate_descriptors(&ctx);

    if (!(ctx.temps = vkd3d_calloc(ctx.program->temp_count, sizeof(*ctx.temps))))
        goto fail;

    if (!(ctx.ssas = vkd3d_calloc(ctx.program->ssa_count, sizeof(*ctx.ssas))))
        goto fail;

    ctx.invalid_instruction_idx = false;

    for (ctx.instruction_idx = 0; ctx.instruction_idx < program->instructions.count
            && ctx.status != VKD3D_ERROR_OUT_OF_MEMORY; ++ctx.instruction_idx)
        vsir_validate_instruction(&ctx);

    ctx.invalid_instruction_idx = true;

    if (ctx.depth != 0)
        validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW, "%zu nested blocks were not closed.", ctx.depth);

    if (ctx.inside_block)
        validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_CONTROL_FLOW, "Last block was not closed.");

    for (i = 0; i < ctx.program->ssa_count; ++i)
    {
        struct validation_context_ssa_data *data = &ctx.ssas[i];

        if ((data->write_mask | data->read_mask) != data->write_mask)
            validator_error(&ctx, VKD3D_SHADER_ERROR_VSIR_INVALID_SSA_USAGE,
                    "SSA register %u has invalid read mask %#x, which is not a subset of the write mask %#x "
                    "at the point of definition.", i, data->read_mask, data->write_mask);
    }

    vkd3d_free(ctx.blocks);
    vkd3d_free(ctx.temps);
    vkd3d_free(ctx.ssas);

    return ctx.status;

fail:
    vkd3d_free(ctx.blocks);
    vkd3d_free(ctx.temps);
    vkd3d_free(ctx.ssas);

    return VKD3D_ERROR_OUT_OF_MEMORY;
}

#define vsir_transform(ctx, step) vsir_transform_(ctx, #step, step)
static void vsir_transform_(
        struct vsir_transformation_context *ctx, const char *step_name,
        enum vkd3d_result (*step)(struct vsir_program *program, struct vsir_transformation_context *ctx))
{
    if (ctx->result < 0)
        return;

    if ((ctx->result = step(ctx->program, ctx)) < 0)
    {
        WARN("Transformation \"%s\" failed with result %d.\n", step_name, ctx->result);
        return;
    }

    if ((ctx->result = vsir_program_validate(ctx->program, ctx->config_flags,
            ctx->compile_info->source_name, ctx->message_context)) < 0)
    {
        WARN("Validation failed with result %d after transformation \"%s\".\n", ctx->result, step_name);
        return;
    }
}

/* Transformations which should happen at parse time, i.e. before scan
 * information is returned to the user.
 *
 * In particular, some passes need to modify the signature, and
 * vkd3d_shader_scan() should report the modified signature for the given
 * target. */
enum vkd3d_result vsir_program_transform_early(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_message_context *message_context)
{
    struct vsir_transformation_context ctx =
    {
        .result = VKD3D_OK,
        .program = program,
        .config_flags = config_flags,
        .compile_info = compile_info,
        .message_context = message_context,
    };

    /* For vsir_program_ensure_diffuse(). */
    if (program->shader_version.major <= 2)
        vsir_transform(&ctx, vsir_program_add_diffuse_output);

    /* For vsir_program_insert_fragment_fog(). */
    vsir_transform(&ctx, vsir_program_add_fog_input);

    /* For vsir_program_insert_vertex_fog(). */
    vsir_transform(&ctx, vsir_program_add_fog_output);

    return ctx.result;
}

enum vkd3d_result vsir_program_transform(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_message_context *message_context)
{
    struct vsir_transformation_context ctx =
    {
        .result = VKD3D_OK,
        .program = program,
        .config_flags = config_flags,
        .compile_info = compile_info,
        .message_context = message_context,
    };

    vsir_transform(&ctx, vsir_program_lower_instructions);

    if (program->shader_version.major >= 6)
    {
        vsir_transform(&ctx, vsir_program_materialise_phi_ssas_to_temps);
        vsir_transform(&ctx, vsir_program_lower_switch_to_selection_ladder);
        vsir_transform(&ctx, vsir_program_structurize);
        vsir_transform(&ctx, vsir_program_flatten_control_flow_constructs);
        vsir_transform(&ctx, vsir_program_materialize_undominated_ssas_to_temps);
    }
    else
    {
        vsir_transform(&ctx, vsir_program_ensure_ret);

        if (program->shader_version.major <= 2)
            vsir_transform(&ctx, vsir_program_ensure_diffuse);

        if (program->shader_version.major < 4)
            vsir_transform(&ctx, vsir_program_normalize_addr);

        if (program->shader_version.type != VKD3D_SHADER_TYPE_PIXEL)
            vsir_transform(&ctx, vsir_program_remap_output_signature);

        if (program->shader_version.type == VKD3D_SHADER_TYPE_HULL)
            vsir_transform(&ctx, vsir_program_flatten_hull_shader_phases);

        vsir_transform(&ctx, instruction_array_normalise_hull_shader_control_point_io);
        vsir_transform(&ctx, vsir_program_normalise_io_registers);
        vsir_transform(&ctx, vsir_program_normalise_flat_constants);
        vsir_transform(&ctx, vsir_program_remove_dead_code);

        if (compile_info->target_type != VKD3D_SHADER_TARGET_GLSL
                && compile_info->target_type != VKD3D_SHADER_TARGET_MSL)
            vsir_transform(&ctx, vsir_program_flatten_control_flow_constructs);
    }

    vsir_transform(&ctx, vsir_program_apply_flat_interpolation);
    vsir_transform(&ctx, vsir_program_insert_alpha_test);
    vsir_transform(&ctx, vsir_program_insert_clip_planes);
    vsir_transform(&ctx, vsir_program_insert_point_size);
    vsir_transform(&ctx, vsir_program_insert_point_size_clamp);
    vsir_transform(&ctx, vsir_program_insert_point_coord);
    vsir_transform(&ctx, vsir_program_insert_fragment_fog);
    vsir_transform(&ctx, vsir_program_insert_vertex_fog);

    if (TRACE_ON())
        vsir_program_trace(program);

    return ctx.result;
}
