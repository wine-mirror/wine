/*
 * Copyright 2023 Conor McCarthy for CodeWeavers
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

static inline bool shader_register_is_phase_instance_id(const struct vkd3d_shader_register *reg)
{
    return reg->type == VKD3DSPR_FORKINSTID || reg->type == VKD3DSPR_JOININSTID;
}

static bool shader_instruction_is_dcl(const struct vkd3d_shader_instruction *ins)
{
    return (VKD3DSIH_DCL <= ins->handler_idx && ins->handler_idx <= VKD3DSIH_DCL_VERTICES_OUT)
            || ins->handler_idx == VKD3DSIH_HS_DECLS;
}

static void vkd3d_shader_instruction_make_nop(struct vkd3d_shader_instruction *ins)
{
    ins->handler_idx = VKD3DSIH_NOP;
    ins->dst_count = 0;
    ins->src_count = 0;
    ins->dst = NULL;
    ins->src = NULL;
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
            reg->type = VKD3DSPR_IMMCONST;
            reg->precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
            reg->non_uniform = false;
            reg->idx[0].offset = ~0u;
            reg->idx[0].rel_addr = NULL;
            reg->idx[1].offset = ~0u;
            reg->idx[1].rel_addr = NULL;
            reg->idx[2].offset = ~0u;
            reg->idx[2].rel_addr = NULL;
            reg->idx_count = 0;
            reg->immconst_type = VKD3D_IMMCONST_SCALAR;
            reg->u.immconst_uint[0] = instance_id;
            continue;
        }
        shader_register_eliminate_phase_addressing(reg, instance_id);
    }

    for (i = 0; i < ins->dst_count; ++i)
        shader_register_eliminate_phase_addressing((struct vkd3d_shader_register *)&ins->dst[i].reg, instance_id);
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

static enum vkd3d_result remap_output_signature(struct vkd3d_shader_parser *parser,
        const struct vkd3d_shader_compile_info *compile_info)
{
    struct shader_signature *signature = &parser->shader_desc.output_signature;
    const struct vkd3d_shader_varying_map_info *varying_map;
    unsigned int i;

    if (!(varying_map = vkd3d_find_struct(compile_info->next, VARYING_MAP_INFO)))
        return VKD3D_OK;

    for (i = 0; i < signature->element_count; ++i)
    {
        const struct vkd3d_shader_varying_map *map = find_varying_map(varying_map, i);
        struct signature_element *e = &signature->elements[i];

        if (map)
        {
            unsigned int input_mask = map->input_mask;

            e->target_location = map->input_register_index;

            /* It is illegal in Vulkan if the next shader uses the same varying
             * location with a different mask. */
            if (input_mask && input_mask != e->mask)
            {
                vkd3d_shader_parser_error(parser, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                        "Aborting due to not yet implemented feature: "
                        "Output mask %#x does not match input mask %#x.",
                        e->mask, input_mask);
                return VKD3D_ERROR_NOT_IMPLEMENTED;
            }
        }
        else
        {
            e->target_location = SIGNATURE_TARGET_LOCATION_UNUSED;
        }
    }

    for (i = 0; i < varying_map->varying_count; ++i)
    {
        if (varying_map->varying_map[i].output_signature_index >= signature->element_count)
        {
            vkd3d_shader_parser_error(parser, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                    "Aborting due to not yet implemented feature: "
                    "The next stage consumes varyings not written by this stage.");
            return VKD3D_ERROR_NOT_IMPLEMENTED;
        }
    }

    return VKD3D_OK;
}

struct hull_flattener
{
    struct vkd3d_shader_instruction_array instructions;

    unsigned int max_temp_count;
    unsigned int temp_dcl_idx;

    unsigned int instance_count;
    unsigned int phase_body_idx;
    enum vkd3d_shader_opcode phase;
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

    if (ins->handler_idx == VKD3DSIH_HS_FORK_PHASE || ins->handler_idx == VKD3DSIH_HS_JOIN_PHASE)
    {
        b = flattener_is_in_fork_or_join_phase(normaliser);
        /* Reset the phase info. */
        normaliser->phase_body_idx = ~0u;
        normaliser->phase = ins->handler_idx;
        normaliser->instance_count = 1;
        /* Leave the first occurrence and delete the rest. */
        if (b)
            vkd3d_shader_instruction_make_nop(ins);
        return;
    }
    else if (ins->handler_idx == VKD3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT
            || ins->handler_idx == VKD3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT)
    {
        normaliser->instance_count = ins->declaration.count + !ins->declaration.count;
        vkd3d_shader_instruction_make_nop(ins);
        return;
    }
    else if (ins->handler_idx == VKD3DSIH_DCL_INPUT && shader_register_is_phase_instance_id(
            &ins->declaration.dst.reg))
    {
        vkd3d_shader_instruction_make_nop(ins);
        return;
    }
    else if (ins->handler_idx == VKD3DSIH_DCL_TEMPS && normaliser->phase != VKD3DSIH_INVALID)
    {
        /* Leave only the first temp declaration and set it to the max count later. */
        if (!normaliser->max_temp_count)
            normaliser->temp_dcl_idx = index;
        else
            vkd3d_shader_instruction_make_nop(ins);
        normaliser->max_temp_count = max(normaliser->max_temp_count, ins->declaration.count);
        return;
    }

    if (normaliser->phase == VKD3DSIH_INVALID || shader_instruction_is_dcl(ins))
        return;

    if (normaliser->phase_body_idx == ~0u)
        normaliser->phase_body_idx = index;

    if (ins->handler_idx == VKD3DSIH_RET)
    {
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

void shader_register_init(struct vkd3d_shader_register *reg, enum vkd3d_shader_register_type reg_type,
        enum vkd3d_data_type data_type, unsigned int idx_count)
{
    reg->type = reg_type;
    reg->precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    reg->non_uniform = false;
    reg->data_type = data_type;
    reg->idx[0].offset = ~0u;
    reg->idx[0].rel_addr = NULL;
    reg->idx[1].offset = ~0u;
    reg->idx[1].rel_addr = NULL;
    reg->idx[2].offset = ~0u;
    reg->idx[2].rel_addr = NULL;
    reg->idx_count = idx_count;
    reg->immconst_type = VKD3D_IMMCONST_SCALAR;
}

void shader_instruction_init(struct vkd3d_shader_instruction *ins, enum vkd3d_shader_opcode handler_idx)
{
    memset(ins, 0, sizeof(*ins));
    ins->handler_idx = handler_idx;
}

static enum vkd3d_result instruction_array_flatten_hull_shader_phases(struct vkd3d_shader_instruction_array *src_instructions)
{
    struct hull_flattener flattener = {*src_instructions};
    struct vkd3d_shader_instruction_array *instructions;
    struct shader_phase_location_array locations;
    enum vkd3d_result result = VKD3D_OK;
    unsigned int i;

    instructions = &flattener.instructions;

    flattener.phase = VKD3DSIH_INVALID;
    for (i = 0, locations.count = 0; i < instructions->count; ++i)
        flattener_eliminate_phase_related_dcls(&flattener, i, &locations);

    if ((result = flattener_flatten_phases(&flattener, &locations)) < 0)
        return result;

    if (flattener.phase != VKD3DSIH_INVALID)
    {
        if (flattener.temp_dcl_idx)
            instructions->elements[flattener.temp_dcl_idx].declaration.count = flattener.max_temp_count;

        if (!shader_instruction_array_reserve(&flattener.instructions, flattener.instructions.count + 1))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        shader_instruction_init(&instructions->elements[instructions->count++], VKD3DSIH_RET);
    }

    *src_instructions = flattener.instructions;
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

static struct vkd3d_shader_src_param *instruction_array_create_outpointid_param(
        struct vkd3d_shader_instruction_array *instructions)
{
    struct vkd3d_shader_src_param *rel_addr;

    if (!(rel_addr = shader_src_param_allocator_get(&instructions->src_params, 1)))
        return NULL;

    shader_register_init(&rel_addr->reg, VKD3DSPR_OUTPOINTID, VKD3D_DATA_UINT, 0);
    rel_addr->swizzle = 0;
    rel_addr->modifiers = 0;

    return rel_addr;
}

static void shader_dst_param_normalise_outpointid(struct vkd3d_shader_dst_param *dst_param,
        struct control_point_normaliser *normaliser)
{
    struct vkd3d_shader_register *reg = &dst_param->reg;

    if (control_point_normaliser_is_in_control_point_phase(normaliser) && reg->type == VKD3DSPR_OUTPUT)
    {
        /* The TPF reader validates idx_count. */
        assert(reg->idx_count == 1);
        reg->idx[1] = reg->idx[0];
        /* The control point id param is implicit here. Avoid later complications by inserting it. */
        reg->idx[0].offset = 0;
        reg->idx[0].rel_addr = normaliser->outpointid_param;
        ++reg->idx_count;
    }
}

static void shader_dst_param_io_init(struct vkd3d_shader_dst_param *param, const struct signature_element *e,
        enum vkd3d_shader_register_type reg_type, unsigned int idx_count)
{
    param->write_mask = e->mask;
    param->modifiers = 0;
    param->shift = 0;
    shader_register_init(&param->reg, reg_type, vkd3d_data_type_from_component_type(e->component_type), idx_count);
}

static enum vkd3d_result control_point_normaliser_emit_hs_input(struct control_point_normaliser *normaliser,
        const struct shader_signature *s, unsigned int input_control_point_count, unsigned int dst)
{
    struct vkd3d_shader_instruction *ins;
    struct vkd3d_shader_dst_param *param;
    const struct signature_element *e;
    unsigned int i, count;

    for (i = 0, count = 1; i < s->element_count; ++i)
        count += !!s->elements[i].used_mask;

    if (!shader_instruction_array_reserve(&normaliser->instructions, normaliser->instructions.count + count))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    memmove(&normaliser->instructions.elements[dst + count], &normaliser->instructions.elements[dst],
            (normaliser->instructions.count - dst) * sizeof(*normaliser->instructions.elements));
    normaliser->instructions.count += count;

    ins = &normaliser->instructions.elements[dst];
    shader_instruction_init(ins, VKD3DSIH_HS_CONTROL_POINT_PHASE);
    ins->flags = 1;
    ++ins;

    for (i = 0; i < s->element_count; ++i)
    {
        e = &s->elements[i];
        if (!e->used_mask)
            continue;

        if (e->sysval_semantic != VKD3D_SHADER_SV_NONE)
        {
            shader_instruction_init(ins, VKD3DSIH_DCL_INPUT_SIV);
            param = &ins->declaration.register_semantic.reg;
            ins->declaration.register_semantic.sysval_semantic = vkd3d_siv_from_sysval(e->sysval_semantic);
        }
        else
        {
            shader_instruction_init(ins, VKD3DSIH_DCL_INPUT);
            param = &ins->declaration.dst;
        }

        shader_dst_param_io_init(param, e, VKD3DSPR_INPUT, 2);
        param->reg.idx[0].offset = input_control_point_count;
        param->reg.idx[1].offset = i;

        ++ins;
    }

    return VKD3D_OK;
}

static enum vkd3d_result instruction_array_normalise_hull_shader_control_point_io(
        struct vkd3d_shader_instruction_array *src_instructions, const struct shader_signature *input_signature)
{
    struct vkd3d_shader_instruction_array *instructions;
    struct control_point_normaliser normaliser;
    unsigned int input_control_point_count;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_result ret;
    unsigned int i, j;

    if (!(normaliser.outpointid_param = instruction_array_create_outpointid_param(src_instructions)))
    {
        ERR("Failed to allocate src param.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    normaliser.instructions = *src_instructions;
    instructions = &normaliser.instructions;
    normaliser.phase = VKD3DSIH_INVALID;

    for (i = 0; i < normaliser.instructions.count; ++i)
    {
        ins = &instructions->elements[i];

        switch (ins->handler_idx)
        {
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                normaliser.phase = ins->handler_idx;
                break;
            default:
                if (shader_instruction_is_dcl(ins))
                    break;
                for (j = 0; j < ins->dst_count; ++j)
                    shader_dst_param_normalise_outpointid((struct vkd3d_shader_dst_param *)&ins->dst[j], &normaliser);
                break;
        }
    }

    normaliser.phase = VKD3DSIH_INVALID;
    input_control_point_count = 1;

    for (i = 0; i < instructions->count; ++i)
    {
        ins = &instructions->elements[i];

        switch (ins->handler_idx)
        {
            case VKD3DSIH_DCL_INPUT_CONTROL_POINT_COUNT:
                input_control_point_count = ins->declaration.count;
                break;
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
                *src_instructions = normaliser.instructions;
                return VKD3D_OK;
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                ret = control_point_normaliser_emit_hs_input(&normaliser, input_signature,
                        input_control_point_count, i);
                *src_instructions = normaliser.instructions;
                return ret;
            default:
                break;
        }
    }

    *src_instructions = normaliser.instructions;
    return VKD3D_OK;
}

struct io_normaliser
{
    struct vkd3d_shader_instruction_array instructions;
    enum vkd3d_shader_type shader_type;
    struct shader_signature *input_signature;
    struct shader_signature *output_signature;
    struct shader_signature *patch_constant_signature;

    unsigned int max_temp_count;
    unsigned int temp_dcl_idx;

    unsigned int instance_count;
    unsigned int phase_body_idx;
    enum vkd3d_shader_opcode phase;
    unsigned int output_control_point_count;

    struct vkd3d_shader_src_param *outpointid_param;

    struct vkd3d_shader_dst_param *input_dcl_params[MAX_REG_OUTPUT];
    struct vkd3d_shader_dst_param *output_dcl_params[MAX_REG_OUTPUT];
    struct vkd3d_shader_dst_param *pc_dcl_params[MAX_REG_OUTPUT];
    uint8_t input_range_map[MAX_REG_OUTPUT][VKD3D_VEC4_SIZE];
    uint8_t output_range_map[MAX_REG_OUTPUT][VKD3D_VEC4_SIZE];
    uint8_t pc_range_map[MAX_REG_OUTPUT][VKD3D_VEC4_SIZE];
};

static bool io_normaliser_is_in_fork_or_join_phase(const struct io_normaliser *normaliser)
{
    return normaliser->phase == VKD3DSIH_HS_FORK_PHASE || normaliser->phase == VKD3DSIH_HS_JOIN_PHASE;
}

static bool io_normaliser_is_in_control_point_phase(const struct io_normaliser *normaliser)
{
    return normaliser->phase == VKD3DSIH_HS_CONTROL_POINT_PHASE;
}

static unsigned int shader_signature_find_element_for_reg(const struct shader_signature *signature,
        unsigned int reg_idx, unsigned int write_mask)
{
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        struct signature_element *e = &signature->elements[i];
        if (e->register_index <= reg_idx && e->register_index + e->register_count > reg_idx
                && (e->mask & write_mask) == write_mask)
        {
            return i;
        }
    }

    /* Validated in the TPF reader. */
    vkd3d_unreachable();
}

static unsigned int range_map_get_register_count(uint8_t range_map[][VKD3D_VEC4_SIZE],
        unsigned int register_idx, unsigned int write_mask)
{
    return range_map[register_idx][vkd3d_write_mask_get_component_idx(write_mask)];
}

static void range_map_set_register_range(uint8_t range_map[][VKD3D_VEC4_SIZE], unsigned int register_idx,
        unsigned int register_count, unsigned int write_mask, bool is_dcl_indexrange)
{
    unsigned int i, j, r, c, component_idx, component_count;

    assert(write_mask <= VKD3DSP_WRITEMASK_ALL);
    component_idx = vkd3d_write_mask_get_component_idx(write_mask);
    component_count = vkd3d_write_mask_component_count(write_mask);

    assert(register_idx < MAX_REG_OUTPUT && MAX_REG_OUTPUT - register_idx >= register_count);

    if (range_map[register_idx][component_idx] > register_count && is_dcl_indexrange)
    {
        /* Validated in the TPF reader. */
        assert(range_map[register_idx][component_idx] != UINT8_MAX);
        return;
    }
    if (range_map[register_idx][component_idx] == register_count)
    {
        /* Already done. This happens when fxc splits a register declaration by
         * component(s). The dcl_indexrange instructions are split too. */
        return;
    }
    range_map[register_idx][component_idx] = register_count;

    for (i = 0; i < register_count; ++i)
    {
        r = register_idx + i;
        for (j = !i; j < component_count; ++j)
        {
            c = component_idx + j;
            /* A synthetic patch constant range which overlaps an existing range can start upstream of it
             * for fork/join phase instancing, but ranges declared by dcl_indexrange should not overlap.
             * The latter is validated in the TPF reader. */
            assert(!range_map[r][c] || !is_dcl_indexrange);
            range_map[r][c] = UINT8_MAX;
        }
    }
}

static void io_normaliser_add_index_range(struct io_normaliser *normaliser,
        const struct vkd3d_shader_instruction *ins)
{
    const struct vkd3d_shader_index_range *range = &ins->declaration.index_range;
    const struct vkd3d_shader_register *reg = &range->dst.reg;
    unsigned int reg_idx, write_mask, element_idx;
    const struct shader_signature *signature;
    uint8_t (*range_map)[VKD3D_VEC4_SIZE];

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
    write_mask = range->dst.write_mask;
    element_idx = shader_signature_find_element_for_reg(signature, reg_idx, write_mask);
    range_map_set_register_range(range_map, reg_idx, range->register_count,
            signature->elements[element_idx].mask, true);
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
static void shader_signature_map_patch_constant_index_ranges(struct shader_signature *s,
        uint8_t range_map[][VKD3D_VEC4_SIZE])
{
    struct signature_element *e, *f;
    unsigned int i, j, register_count;

    qsort(s->elements, s->element_count, sizeof(s->elements[0]), signature_element_mask_compare);

    for (i = 0; i < s->element_count; i += register_count)
    {
        e = &s->elements[i];
        register_count = 1;

        if (!e->sysval_semantic)
            continue;

        for (j = i + 1; j < s->element_count; ++j, ++register_count)
        {
            f = &s->elements[j];
            if (f->register_index != e->register_index + register_count || !sysval_semantics_should_merge(e, f))
                break;
        }
        if (register_count < 2)
            continue;

        range_map_set_register_range(range_map, e->register_index, register_count, e->mask, false);
    }
}

static int signature_element_register_compare(const void *a, const void *b)
{
    const struct signature_element *e = a, *f = b;

    return vkd3d_u32_compare(e->register_index, f->register_index);
}

static int signature_element_index_compare(const void *a, const void *b)
{
    const struct signature_element *e = a, *f = b;

    return vkd3d_u32_compare(e->sort_index, f->sort_index);
}

static bool shader_signature_merge(struct shader_signature *s, uint8_t range_map[][VKD3D_VEC4_SIZE],
        bool is_patch_constant)
{
    unsigned int i, j, element_count, new_count, register_count;
    struct signature_element *elements;
    struct signature_element *e, *f;

    element_count = s->element_count;
    if (!(elements = vkd3d_malloc(element_count * sizeof(*elements))))
        return false;
    memcpy(elements, s->elements, element_count * sizeof(*elements));

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
            assert(!(e->mask & f->mask));

            e->mask |= f->mask;
            e->used_mask |= f->used_mask;
            e->semantic_index = min(e->semantic_index, f->semantic_index);
        }
    }
    element_count = new_count;
    vkd3d_free(s->elements);
    s->elements = elements;
    s->element_count = element_count;

    if (is_patch_constant)
        shader_signature_map_patch_constant_index_ranges(s, range_map);

    for (i = 0, new_count = 0; i < element_count; i += register_count, elements[new_count++] = *e)
    {
        e = &elements[i];
        register_count = 1;

        if (e->register_index >= MAX_REG_OUTPUT)
            continue;

        register_count = range_map_get_register_count(range_map, e->register_index, e->mask);
        assert(register_count != UINT8_MAX);
        register_count += !register_count;

        if (register_count > 1)
        {
            TRACE("Merging %s, base reg %u, count %u.\n", e->semantic_name, e->register_index, register_count);
            e->register_count = register_count;
        }
    }
    element_count = new_count;

    /* Restoring the original order is required for sensible trace output. */
    qsort(elements, element_count, sizeof(elements[0]), signature_element_index_compare);

    s->element_count = element_count;

    return true;
}

static bool sysval_semantic_is_tess_factor(enum vkd3d_shader_sysval_semantic sysval_semantic)
{
    return sysval_semantic >= VKD3D_SHADER_SV_TESS_FACTOR_QUADEDGE
        && sysval_semantic <= VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN;
}

static unsigned int shader_register_normalise_arrayed_addressing(struct vkd3d_shader_register *reg,
        unsigned int id_idx, unsigned int register_index)
{
    assert(id_idx < ARRAY_SIZE(reg->idx) - 1);

    /* For a relative-addressed register index, move the id up a slot to separate it from the address,
     * because rel_addr can be replaced with a constant offset in some cases. */
    if (reg->idx[id_idx].rel_addr)
    {
        reg->idx[id_idx + 1].rel_addr = NULL;
        reg->idx[id_idx + 1].offset = reg->idx[id_idx].offset;
        reg->idx[id_idx].offset -= register_index;
        ++id_idx;
    }
    /* Otherwise we have no address for the arrayed register, so insert one. This happens e.g. where
     * tessellation level registers are merged into an array because they're an array in SPIR-V. */
    else
    {
        ++id_idx;
        memmove(&reg->idx[1], &reg->idx[0], id_idx * sizeof(reg->idx[0]));
        reg->idx[0].rel_addr = NULL;
        reg->idx[0].offset = reg->idx[id_idx].offset - register_index;
    }

    return id_idx;
}

static bool shader_dst_param_io_normalise(struct vkd3d_shader_dst_param *dst_param, bool is_io_dcl,
         struct io_normaliser *normaliser)
 {
    unsigned int id_idx, reg_idx, write_mask, element_idx;
    struct vkd3d_shader_register *reg = &dst_param->reg;
    struct vkd3d_shader_dst_param **dcl_params;
    const struct shader_signature *signature;
    const struct signature_element *e;

    if ((reg->type == VKD3DSPR_OUTPUT && io_normaliser_is_in_fork_or_join_phase(normaliser))
            || reg->type == VKD3DSPR_PATCHCONST)
    {
        signature = normaliser->patch_constant_signature;
        /* Convert patch constant outputs to the patch constant register type to avoid the need
         * to convert compiler symbols when accessed as inputs in a later stage. */
        reg->type = VKD3DSPR_PATCHCONST;
        dcl_params = normaliser->pc_dcl_params;
    }
    else if (reg->type == VKD3DSPR_OUTPUT || dst_param->reg.type == VKD3DSPR_COLOROUT)
    {
        signature = normaliser->output_signature;
        dcl_params = normaliser->output_dcl_params;
    }
    else if (dst_param->reg.type == VKD3DSPR_INCONTROLPOINT || dst_param->reg.type == VKD3DSPR_INPUT)
    {
        signature = normaliser->input_signature;
        dcl_params = normaliser->input_dcl_params;
    }
    else
    {
        return true;
    }

    id_idx = reg->idx_count - 1;
    reg_idx = reg->idx[id_idx].offset;
    write_mask = dst_param->write_mask;
    element_idx = shader_signature_find_element_for_reg(signature, reg_idx, write_mask);
    e = &signature->elements[element_idx];

    dst_param->write_mask >>= vkd3d_write_mask_get_component_idx(e->mask);
    if (is_io_dcl)
    {
        /* Validated in the TPF reader. */
        assert(element_idx < ARRAY_SIZE(normaliser->input_dcl_params));

        if (dcl_params[element_idx])
        {
            /* Merge split declarations into a single one. */
            dcl_params[element_idx]->write_mask |= dst_param->write_mask;
            /* Turn this into a nop. */
            return false;
        }
        else
        {
            dcl_params[element_idx] = dst_param;
        }
    }

    if (io_normaliser_is_in_control_point_phase(normaliser) && reg->type == VKD3DSPR_OUTPUT)
    {
        if (is_io_dcl)
        {
            /* Emit an array size for the control points for consistency with inputs. */
            reg->idx[0].offset = normaliser->output_control_point_count;
        }
        else
        {
            /* The control point id param. */
            assert(reg->idx[0].rel_addr);
        }
        id_idx = 1;
    }

    if ((e->register_count > 1 || sysval_semantic_is_tess_factor(e->sysval_semantic)))
    {
        if (is_io_dcl)
        {
            /* For control point I/O, idx 0 contains the control point count.
             * Ensure it is moved up to the next slot. */
            reg->idx[id_idx].offset = reg->idx[0].offset;
            reg->idx[0].offset = e->register_count;
            ++id_idx;
        }
        else
        {
            id_idx = shader_register_normalise_arrayed_addressing(reg, id_idx, e->register_index);
        }
    }

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
            signature = normaliser->patch_constant_signature;
            break;
        case VKD3DSPR_INCONTROLPOINT:
            if (normaliser->shader_type == VKD3D_SHADER_TYPE_HULL)
                reg->type = VKD3DSPR_INPUT;
            /* fall through */
        case VKD3DSPR_INPUT:
            signature = normaliser->input_signature;
            break;
        case VKD3DSPR_OUTCONTROLPOINT:
            if (normaliser->shader_type == VKD3D_SHADER_TYPE_HULL)
                reg->type = VKD3DSPR_OUTPUT;
            /* fall through */
        case VKD3DSPR_OUTPUT:
            signature = normaliser->output_signature;
            break;
        default:
            return;
    }

    id_idx = reg->idx_count - 1;
    reg_idx = reg->idx[id_idx].offset;
    write_mask = VKD3DSP_WRITEMASK_0 << vkd3d_swizzle_get_component(src_param->swizzle, 0);
    element_idx = shader_signature_find_element_for_reg(signature, reg_idx, write_mask);

    e = &signature->elements[element_idx];
    if ((e->register_count > 1 || sysval_semantic_is_tess_factor(e->sysval_semantic)))
        id_idx = shader_register_normalise_arrayed_addressing(reg, id_idx, e->register_index);
    reg->idx[id_idx].offset = element_idx;
    reg->idx_count = id_idx + 1;

    if ((component_idx = vkd3d_write_mask_get_component_idx(e->mask)))
    {
        for (i = 0; i < VKD3D_VEC4_SIZE; ++i)
            if (vkd3d_swizzle_get_component(src_param->swizzle, i))
                src_param->swizzle -= component_idx << VKD3D_SHADER_SWIZZLE_SHIFT(i);
    }
}

static void shader_instruction_normalise_io_params(struct vkd3d_shader_instruction *ins,
        struct io_normaliser *normaliser)
{
    struct vkd3d_shader_register *reg;
    bool keep = true;
    unsigned int i;

    switch (ins->handler_idx)
    {
        case VKD3DSIH_DCL_INPUT:
            if (normaliser->shader_type == VKD3D_SHADER_TYPE_HULL)
            {
                reg = &ins->declaration.dst.reg;
                /* We don't need to keep OUTCONTROLPOINT or PATCHCONST input declarations since their
                * equivalents were declared earlier, but INCONTROLPOINT may be the first occurrence. */
                if (reg->type == VKD3DSPR_OUTCONTROLPOINT || reg->type == VKD3DSPR_PATCHCONST)
                    vkd3d_shader_instruction_make_nop(ins);
                else if (reg->type == VKD3DSPR_INCONTROLPOINT)
                    reg->type = VKD3DSPR_INPUT;
            }
            /* fall through */
        case VKD3DSIH_DCL_INPUT_PS:
        case VKD3DSIH_DCL_OUTPUT:
            keep = shader_dst_param_io_normalise(&ins->declaration.dst, true, normaliser);
            break;
        case VKD3DSIH_DCL_INPUT_SGV:
        case VKD3DSIH_DCL_INPUT_SIV:
        case VKD3DSIH_DCL_INPUT_PS_SGV:
        case VKD3DSIH_DCL_INPUT_PS_SIV:
        case VKD3DSIH_DCL_OUTPUT_SIV:
            keep = shader_dst_param_io_normalise(&ins->declaration.register_semantic.reg, true,
                    normaliser);
            break;
        case VKD3DSIH_HS_CONTROL_POINT_PHASE:
        case VKD3DSIH_HS_FORK_PHASE:
        case VKD3DSIH_HS_JOIN_PHASE:
            normaliser->phase = ins->handler_idx;
            memset(normaliser->input_dcl_params, 0, sizeof(normaliser->input_dcl_params));
            memset(normaliser->output_dcl_params, 0, sizeof(normaliser->output_dcl_params));
            memset(normaliser->pc_dcl_params, 0, sizeof(normaliser->pc_dcl_params));
            break;
        default:
            if (shader_instruction_is_dcl(ins))
                break;
            for (i = 0; i < ins->dst_count; ++i)
                shader_dst_param_io_normalise((struct vkd3d_shader_dst_param *)&ins->dst[i], false, normaliser);
            for (i = 0; i < ins->src_count; ++i)
                shader_src_param_io_normalise((struct vkd3d_shader_src_param *)&ins->src[i], normaliser);
            break;
    }

    if (!keep)
        shader_instruction_init(ins, VKD3DSIH_NOP);
}

static enum vkd3d_result instruction_array_normalise_io_registers(struct vkd3d_shader_instruction_array *instructions,
        enum vkd3d_shader_type shader_type, struct shader_signature *input_signature,
        struct shader_signature *output_signature, struct shader_signature *patch_constant_signature)
{
    struct io_normaliser normaliser = {*instructions};
    struct vkd3d_shader_instruction *ins;
    bool has_control_point_phase;
    unsigned int i, j;

    normaliser.phase = VKD3DSIH_INVALID;
    normaliser.shader_type = shader_type;
    normaliser.input_signature = input_signature;
    normaliser.output_signature = output_signature;
    normaliser.patch_constant_signature = patch_constant_signature;

    for (i = 0, has_control_point_phase = false; i < instructions->count; ++i)
    {
        ins = &instructions->elements[i];

        switch (ins->handler_idx)
        {
            case VKD3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT:
                normaliser.output_control_point_count = ins->declaration.count;
                break;
            case VKD3DSIH_DCL_INDEX_RANGE:
                io_normaliser_add_index_range(&normaliser, ins);
                vkd3d_shader_instruction_make_nop(ins);
                break;
            case VKD3DSIH_HS_CONTROL_POINT_PHASE:
                has_control_point_phase = true;
                /* fall through */
            case VKD3DSIH_HS_FORK_PHASE:
            case VKD3DSIH_HS_JOIN_PHASE:
                normaliser.phase = ins->handler_idx;
                break;
            default:
                break;
        }
    }

    if (normaliser.shader_type == VKD3D_SHADER_TYPE_HULL && !has_control_point_phase)
    {
        /* Inputs and outputs must match for the default phase, so merge ranges must match too. */
        for (i = 0; i < MAX_REG_OUTPUT; ++i)
        {
            for (j = 0; j < VKD3D_VEC4_SIZE; ++j)
            {
                if (!normaliser.input_range_map[i][j] && normaliser.output_range_map[i][j])
                    normaliser.input_range_map[i][j] = normaliser.output_range_map[i][j];
                else if (normaliser.input_range_map[i][j] && !normaliser.output_range_map[i][j])
                    normaliser.output_range_map[i][j] = normaliser.input_range_map[i][j];
                else assert(normaliser.input_range_map[i][j] == normaliser.output_range_map[i][j]);
            }
        }
    }

    if (!shader_signature_merge(input_signature, normaliser.input_range_map, false)
            || !shader_signature_merge(output_signature, normaliser.output_range_map, false)
            || !shader_signature_merge(patch_constant_signature, normaliser.pc_range_map, true))
    {
        *instructions = normaliser.instructions;
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    normaliser.phase = VKD3DSIH_INVALID;
    for (i = 0; i < normaliser.instructions.count; ++i)
        shader_instruction_normalise_io_params(&normaliser.instructions.elements[i], &normaliser);

    *instructions = normaliser.instructions;
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
    struct vkd3d_shader_parser *parser;
    struct flat_constant_def *defs;
    size_t def_count, defs_capacity;
};

static bool get_flat_constant_register_type(const struct vkd3d_shader_register *reg,
        enum vkd3d_shader_d3dbc_constant_register *set, uint32_t *index)
{
    static const struct
    {
        enum vkd3d_shader_register_type type;
        enum vkd3d_shader_d3dbc_constant_register set;
        uint32_t offset;
    }
    regs[] =
    {
        {VKD3DSPR_CONST, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 0},
        {VKD3DSPR_CONST2, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 2048},
        {VKD3DSPR_CONST3, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 4096},
        {VKD3DSPR_CONST4, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 6144},
        {VKD3DSPR_CONSTINT, VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER, 0},
        {VKD3DSPR_CONSTBOOL, VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER, 0},
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(regs); ++i)
    {
        if (reg->type == regs[i].type)
        {
            if (reg->idx[0].rel_addr)
            {
                FIXME("Unhandled relative address.\n");
                return false;
            }

            *set = regs[i].set;
            *index = regs[i].offset + reg->idx[0].offset;
            return true;
        }
    }

    return false;
}

static void shader_register_normalise_flat_constants(struct vkd3d_shader_src_param *param,
        const struct flat_constants_normaliser *normaliser)
{
    enum vkd3d_shader_d3dbc_constant_register set;
    uint32_t index;
    size_t i, j;

    if (!get_flat_constant_register_type(&param->reg, &set, &index))
        return;

    for (i = 0; i < normaliser->def_count; ++i)
    {
        if (normaliser->defs[i].set == set && normaliser->defs[i].index == index)
        {
            param->reg.type = VKD3DSPR_IMMCONST;
            param->reg.idx_count = 0;
            param->reg.immconst_type = VKD3D_IMMCONST_VEC4;
            for (j = 0; j < 4; ++j)
                param->reg.u.immconst_uint[j] = normaliser->defs[i].value[j];
            return;
        }
    }

    param->reg.type = VKD3DSPR_CONSTBUFFER;
    param->reg.idx[0].offset = set; /* register ID */
    param->reg.idx[1].offset = set; /* register index */
    param->reg.idx[2].offset = index; /* buffer index */
    param->reg.idx_count = 3;
}

static enum vkd3d_result instruction_array_normalise_flat_constants(struct vkd3d_shader_parser *parser)
{
    struct flat_constants_normaliser normaliser = {.parser = parser};
    unsigned int i, j;

    for (i = 0; i < parser->instructions.count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &parser->instructions.elements[i];

        if (ins->handler_idx == VKD3DSIH_DEF || ins->handler_idx == VKD3DSIH_DEFI || ins->handler_idx == VKD3DSIH_DEFB)
        {
            struct flat_constant_def *def;

            if (!vkd3d_array_reserve((void **)&normaliser.defs, &normaliser.defs_capacity,
                    normaliser.def_count + 1, sizeof(*normaliser.defs)))
            {
                vkd3d_free(normaliser.defs);
                return VKD3D_ERROR_OUT_OF_MEMORY;
            }

            def = &normaliser.defs[normaliser.def_count++];

            get_flat_constant_register_type((struct vkd3d_shader_register *)&ins->dst[0].reg, &def->set, &def->index);
            for (j = 0; j < 4; ++j)
                def->value[j] = ins->src[0].reg.u.immconst_uint[j];

            vkd3d_shader_instruction_make_nop(ins);
        }
        else
        {
            for (j = 0; j < ins->src_count; ++j)
                shader_register_normalise_flat_constants((struct vkd3d_shader_src_param *)&ins->src[j], &normaliser);
        }
    }

    vkd3d_free(normaliser.defs);
    return VKD3D_OK;
}

enum vkd3d_result vkd3d_shader_normalise(struct vkd3d_shader_parser *parser,
        const struct vkd3d_shader_compile_info *compile_info)
{
    struct vkd3d_shader_instruction_array *instructions = &parser->instructions;
    enum vkd3d_result result = VKD3D_OK;

    if (parser->shader_desc.is_dxil)
        return result;

    if (parser->shader_version.type != VKD3D_SHADER_TYPE_PIXEL
            && (result = remap_output_signature(parser, compile_info)) < 0)
        return result;

    if (parser->shader_version.type == VKD3D_SHADER_TYPE_HULL
            && (result = instruction_array_flatten_hull_shader_phases(instructions)) >= 0)
    {
        result = instruction_array_normalise_hull_shader_control_point_io(instructions,
                &parser->shader_desc.input_signature);
    }
    if (result >= 0)
        result = instruction_array_normalise_io_registers(instructions, parser->shader_version.type,
                &parser->shader_desc.input_signature, &parser->shader_desc.output_signature,
                &parser->shader_desc.patch_constant_signature);

    if (result >= 0)
        result = instruction_array_normalise_flat_constants(parser);

    if (result >= 0 && TRACE_ON())
        vkd3d_shader_trace(instructions, &parser->shader_version);

    return result;
}
