/*
 * HLSL optimization and code generation
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

/* TODO: remove when no longer needed, only used for new_offset_instr_from_deref() */
static struct hlsl_ir_node *new_offset_from_path_index(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_type *type, struct hlsl_ir_node *base_offset, struct hlsl_ir_node *idx,
        enum hlsl_regset regset, unsigned int *offset_component, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *idx_offset = NULL;
    struct hlsl_ir_node *c;

    switch (type->class)
    {
        case HLSL_CLASS_VECTOR:
            *offset_component += hlsl_ir_constant(idx)->value.u[0].u;
            break;

        case HLSL_CLASS_MATRIX:
        {
            idx_offset = idx;
            break;
        }

        case HLSL_CLASS_ARRAY:
        {
            unsigned int size = hlsl_type_get_array_element_reg_size(type->e.array.type, regset);

            if (regset == HLSL_REGSET_NUMERIC)
            {
                assert(size % 4 == 0);
                size /= 4;
            }

            if (!(c = hlsl_new_uint_constant(ctx, size, loc)))
                return NULL;
            hlsl_block_add_instr(block, c);

            if (!(idx_offset = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, c, idx)))
                return NULL;
            hlsl_block_add_instr(block, idx_offset);

            break;
        }

        case HLSL_CLASS_STRUCT:
        {
            unsigned int field_idx = hlsl_ir_constant(idx)->value.u[0].u;
            struct hlsl_struct_field *field = &type->e.record.fields[field_idx];
            unsigned int field_offset = field->reg_offset[regset];

            if (regset == HLSL_REGSET_NUMERIC)
            {
                assert(*offset_component == 0);
                *offset_component = field_offset % 4;
                field_offset /= 4;
            }

            if (!(c = hlsl_new_uint_constant(ctx, field_offset, loc)))
                return NULL;
            hlsl_block_add_instr(block, c);

            idx_offset = c;

            break;
        }

        default:
            vkd3d_unreachable();
    }

    if (idx_offset)
    {
        if (!(base_offset = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, base_offset, idx_offset)))
            return NULL;
        hlsl_block_add_instr(block, base_offset);
    }

    return base_offset;
}

/* TODO: remove when no longer needed, only used for replace_deref_path_with_offset() */
static struct hlsl_ir_node *new_offset_instr_from_deref(struct hlsl_ctx *ctx, struct hlsl_block *block,
        const struct hlsl_deref *deref, unsigned int *offset_component, const struct vkd3d_shader_location *loc)
{
    enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);
    struct hlsl_ir_node *offset;
    struct hlsl_type *type;
    unsigned int i;

    *offset_component = 0;

    hlsl_block_init(block);

    if (!(offset = hlsl_new_uint_constant(ctx, 0, loc)))
        return NULL;
    hlsl_block_add_instr(block, offset);

    assert(deref->var);
    type = deref->var->data_type;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_block idx_block;

        hlsl_block_init(&idx_block);

        if (!(offset = new_offset_from_path_index(ctx, &idx_block, type, offset, deref->path[i].node,
                regset, offset_component, loc)))
        {
            hlsl_block_cleanup(&idx_block);
            return NULL;
        }

        hlsl_block_add_block(block, &idx_block);

        type = hlsl_get_element_type_from_path_index(ctx, type, deref->path[i].node);
    }

    return offset;
}

/* TODO: remove when no longer needed, only used for transform_deref_paths_into_offsets() */
static bool replace_deref_path_with_offset(struct hlsl_ctx *ctx, struct hlsl_deref *deref,
        struct hlsl_ir_node *instr)
{
    unsigned int offset_component;
    struct hlsl_ir_node *offset;
    struct hlsl_block block;
    struct hlsl_type *type;

    assert(deref->var);
    assert(!hlsl_deref_is_lowered(deref));

    type = hlsl_deref_get_type(ctx, deref);

    /* Instructions that directly refer to structs or arrays (instead of single-register components)
     * are removed later by dce. So it is not a problem to just cleanup their derefs. */
    if (type->class == HLSL_CLASS_STRUCT || type->class == HLSL_CLASS_ARRAY)
    {
        hlsl_cleanup_deref(deref);
        return true;
    }

    deref->data_type = type;

    if (!(offset = new_offset_instr_from_deref(ctx, &block, deref, &offset_component, &instr->loc)))
        return false;
    list_move_before(&instr->entry, &block.instrs);

    hlsl_cleanup_deref(deref);
    hlsl_src_from_node(&deref->rel_offset, offset);
    deref->const_offset = offset_component;

    return true;
}

static bool clean_constant_deref_offset_srcs(struct hlsl_ctx *ctx, struct hlsl_deref *deref,
        struct hlsl_ir_node *instr)
{
    if (deref->rel_offset.node && deref->rel_offset.node->type == HLSL_IR_CONSTANT)
    {
        enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);

        if (regset == HLSL_REGSET_NUMERIC)
            deref->const_offset += 4 * hlsl_ir_constant(deref->rel_offset.node)->value.u[0].u;
        else
            deref->const_offset += hlsl_ir_constant(deref->rel_offset.node)->value.u[0].u;
        hlsl_src_remove(&deref->rel_offset);
        return true;
    }
    return false;
}


/* Split uniforms into two variables representing the constant and temp
 * registers, and copy the former to the latter, so that writes to uniforms
 * work. */
static void prepend_uniform_copy(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_var *temp)
{
    struct hlsl_ir_var *uniform;
    struct hlsl_ir_node *store;
    struct hlsl_ir_load *load;
    char *new_name;

    /* Use the synthetic name for the temp, rather than the uniform, so that we
     * can write the uniform name into the shader reflection data. */

    if (!(uniform = hlsl_new_var(ctx, temp->name, temp->data_type,
            &temp->loc, NULL, temp->storage_modifiers, &temp->reg_reservation)))
        return;
    list_add_before(&temp->scope_entry, &uniform->scope_entry);
    list_add_tail(&ctx->extern_vars, &uniform->extern_entry);
    uniform->is_uniform = 1;
    uniform->is_param = temp->is_param;
    uniform->buffer = temp->buffer;

    if (!(new_name = hlsl_sprintf_alloc(ctx, "<temp-%s>", temp->name)))
        return;
    temp->name = new_name;

    if (!(load = hlsl_new_var_load(ctx, uniform, &temp->loc)))
        return;
    list_add_head(&block->instrs, &load->node.entry);

    if (!(store = hlsl_new_simple_store(ctx, temp, &load->node)))
        return;
    list_add_after(&load->node.entry, &store->entry);
}

static void validate_field_semantic(struct hlsl_ctx *ctx, struct hlsl_struct_field *field)
{
    if (!field->semantic.name && hlsl_is_numeric_type(hlsl_get_multiarray_element_type(field->type))
            && !field->semantic.reported_missing)
    {
        hlsl_error(ctx, &field->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_SEMANTIC,
                "Field '%s' is missing a semantic.", field->name);
        field->semantic.reported_missing = true;
    }
}

static enum hlsl_base_type base_type_get_semantic_equivalent(enum hlsl_base_type base)
{
    if (base == HLSL_TYPE_BOOL)
        return HLSL_TYPE_UINT;
    if (base == HLSL_TYPE_INT)
        return HLSL_TYPE_UINT;
    if (base == HLSL_TYPE_HALF)
        return HLSL_TYPE_FLOAT;
    return base;
}

static bool types_are_semantic_equivalent(struct hlsl_ctx *ctx, const struct hlsl_type *type1,
        const struct hlsl_type *type2)
{
    if (ctx->profile->major_version < 4)
        return true;

    if (type1->dimx != type2->dimx)
        return false;

    return base_type_get_semantic_equivalent(type1->base_type)
            == base_type_get_semantic_equivalent(type2->base_type);
}

static struct hlsl_ir_var *add_semantic_var(struct hlsl_ctx *ctx, struct hlsl_ir_var *var,
        struct hlsl_type *type, unsigned int modifiers, struct hlsl_semantic *semantic,
        uint32_t index, bool output, const struct vkd3d_shader_location *loc)
{
    struct hlsl_semantic new_semantic;
    struct hlsl_ir_var *ext_var;
    char *new_name;

    if (!(new_name = hlsl_sprintf_alloc(ctx, "<%s-%s%u>", output ? "output" : "input", semantic->name, index)))
        return NULL;

    LIST_FOR_EACH_ENTRY(ext_var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!ascii_strcasecmp(ext_var->name, new_name))
        {
            if (output)
            {
                if (index >= semantic->reported_duplicated_output_next_index)
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                            "Output semantic \"%s%u\" is used multiple times.", semantic->name, index);
                    hlsl_note(ctx, &ext_var->loc, VKD3D_SHADER_LOG_ERROR,
                            "First use of \"%s%u\" is here.", semantic->name, index);
                    semantic->reported_duplicated_output_next_index = index + 1;
                }
            }
            else
            {
                if (index >= semantic->reported_duplicated_input_incompatible_next_index
                        && !types_are_semantic_equivalent(ctx, ext_var->data_type, type))
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                            "Input semantic \"%s%u\" is used multiple times with incompatible types.",
                            semantic->name, index);
                    hlsl_note(ctx, &ext_var->loc, VKD3D_SHADER_LOG_ERROR,
                            "First declaration of \"%s%u\" is here.", semantic->name, index);
                    semantic->reported_duplicated_input_incompatible_next_index = index + 1;
                }
            }

            vkd3d_free(new_name);
            return ext_var;
        }
    }

    if (!(new_semantic.name = hlsl_strdup(ctx, semantic->name)))
    {
        vkd3d_free(new_name);
        return NULL;
    }
    new_semantic.index = index;
    if (!(ext_var = hlsl_new_var(ctx, new_name, type, loc, &new_semantic, modifiers, NULL)))
    {
        vkd3d_free(new_name);
        hlsl_cleanup_semantic(&new_semantic);
        return NULL;
    }
    if (output)
        ext_var->is_output_semantic = 1;
    else
        ext_var->is_input_semantic = 1;
    ext_var->is_param = var->is_param;
    list_add_before(&var->scope_entry, &ext_var->scope_entry);
    list_add_tail(&ctx->extern_vars, &ext_var->extern_entry);

    return ext_var;
}

static void prepend_input_copy(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_load *lhs,
        unsigned int modifiers, struct hlsl_semantic *semantic, uint32_t semantic_index)
{
    struct hlsl_type *type = lhs->node.data_type, *vector_type_src, *vector_type_dst;
    struct vkd3d_shader_location *loc = &lhs->node.loc;
    struct hlsl_ir_var *var = lhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (!hlsl_is_numeric_type(type))
    {
        struct vkd3d_string_buffer *string;
        if (!(string = hlsl_type_to_string(ctx, type)))
            return;
        hlsl_fixme(ctx, &var->loc, "Input semantics for type %s.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
    if (!semantic->name)
        return;

    vector_type_dst = hlsl_get_vector_type(ctx, type->base_type, hlsl_type_minor_size(type));
    vector_type_src = vector_type_dst;
    if (ctx->profile->major_version < 4 && ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX)
        vector_type_src = hlsl_get_vector_type(ctx, type->base_type, 4);

    for (i = 0; i < hlsl_type_major_size(type); ++i)
    {
        struct hlsl_ir_node *store, *cast;
        struct hlsl_ir_var *input;
        struct hlsl_ir_load *load;

        if (!(input = add_semantic_var(ctx, var, vector_type_src, modifiers, semantic,
                semantic_index + i, false, loc)))
            return;

        if (!(load = hlsl_new_var_load(ctx, input, &var->loc)))
            return;
        list_add_after(&lhs->node.entry, &load->node.entry);

        if (!(cast = hlsl_new_cast(ctx, &load->node, vector_type_dst, &var->loc)))
            return;
        list_add_after(&load->node.entry, &cast->entry);

        if (type->class == HLSL_CLASS_MATRIX)
        {
            if (!(c = hlsl_new_uint_constant(ctx, i, &var->loc)))
                return;
            list_add_after(&cast->entry, &c->entry);

            if (!(store = hlsl_new_store_index(ctx, &lhs->src, c, cast, 0, &var->loc)))
                return;
            list_add_after(&c->entry, &store->entry);
        }
        else
        {
            assert(i == 0);

            if (!(store = hlsl_new_store_index(ctx, &lhs->src, NULL, cast, 0, &var->loc)))
                return;
            list_add_after(&cast->entry, &store->entry);
        }
    }
}

static void prepend_input_copy_recurse(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_load *lhs,
        unsigned int modifiers, struct hlsl_semantic *semantic, uint32_t semantic_index)
{
    struct vkd3d_shader_location *loc = &lhs->node.loc;
    struct hlsl_type *type = lhs->node.data_type;
    struct hlsl_ir_var *var = lhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (type->class == HLSL_CLASS_ARRAY || type->class == HLSL_CLASS_STRUCT)
    {
        struct hlsl_ir_load *element_load;
        struct hlsl_struct_field *field;
        uint32_t elem_semantic_index;

        for (i = 0; i < hlsl_type_element_count(type); ++i)
        {
            unsigned int element_modifiers = modifiers;

            if (type->class == HLSL_CLASS_ARRAY)
            {
                elem_semantic_index = semantic_index
                        + i * hlsl_type_get_array_element_reg_size(type->e.array.type, HLSL_REGSET_NUMERIC) / 4;
            }
            else
            {
                field = &type->e.record.fields[i];
                if (hlsl_type_is_resource(field->type))
                    continue;
                validate_field_semantic(ctx, field);
                semantic = &field->semantic;
                elem_semantic_index = semantic->index;
                loc = &field->loc;
                element_modifiers |= field->storage_modifiers;

                /* TODO: 'sample' modifier is not supported yet */

                /* 'nointerpolation' always takes precedence, next the same is done for 'sample',
                   remaining modifiers are combined. */
                if (element_modifiers & HLSL_STORAGE_NOINTERPOLATION)
                {
                    element_modifiers &= ~HLSL_INTERPOLATION_MODIFIERS_MASK;
                    element_modifiers |= HLSL_STORAGE_NOINTERPOLATION;
                }
            }

            if (!(c = hlsl_new_uint_constant(ctx, i, &var->loc)))
                return;
            list_add_after(&lhs->node.entry, &c->entry);

            /* This redundant load is expected to be deleted later by DCE. */
            if (!(element_load = hlsl_new_load_index(ctx, &lhs->src, c, loc)))
                return;
            list_add_after(&c->entry, &element_load->node.entry);

            prepend_input_copy_recurse(ctx, block, element_load, element_modifiers, semantic, elem_semantic_index);
        }
    }
    else
    {
        prepend_input_copy(ctx, block, lhs, modifiers, semantic, semantic_index);
    }
}

/* Split inputs into two variables representing the semantic and temp registers,
 * and copy the former to the latter, so that writes to input variables work. */
static void prepend_input_var_copy(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_var *var)
{
    struct hlsl_ir_load *load;

    /* This redundant load is expected to be deleted later by DCE. */
    if (!(load = hlsl_new_var_load(ctx, var, &var->loc)))
        return;
    list_add_head(&block->instrs, &load->node.entry);

    prepend_input_copy_recurse(ctx, block, load, var->storage_modifiers, &var->semantic, var->semantic.index);
}

static void append_output_copy(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_load *rhs,
        unsigned int modifiers, struct hlsl_semantic *semantic, uint32_t semantic_index)
{
    struct hlsl_type *type = rhs->node.data_type, *vector_type;
    struct vkd3d_shader_location *loc = &rhs->node.loc;
    struct hlsl_ir_var *var = rhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (!hlsl_is_numeric_type(type))
    {
        struct vkd3d_string_buffer *string;
        if (!(string = hlsl_type_to_string(ctx, type)))
            return;
        hlsl_fixme(ctx, &var->loc, "Output semantics for type %s.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
    if (!semantic->name)
        return;

    vector_type = hlsl_get_vector_type(ctx, type->base_type, hlsl_type_minor_size(type));

    for (i = 0; i < hlsl_type_major_size(type); ++i)
    {
        struct hlsl_ir_node *store;
        struct hlsl_ir_var *output;
        struct hlsl_ir_load *load;

        if (!(output = add_semantic_var(ctx, var, vector_type, modifiers, semantic, semantic_index + i, true, loc)))
            return;

        if (type->class == HLSL_CLASS_MATRIX)
        {
            if (!(c = hlsl_new_uint_constant(ctx, i, &var->loc)))
                return;
            hlsl_block_add_instr(block, c);

            if (!(load = hlsl_new_load_index(ctx, &rhs->src, c, &var->loc)))
                return;
            hlsl_block_add_instr(block, &load->node);
        }
        else
        {
            assert(i == 0);

            if (!(load = hlsl_new_load_index(ctx, &rhs->src, NULL, &var->loc)))
                return;
            hlsl_block_add_instr(block, &load->node);
        }

        if (!(store = hlsl_new_simple_store(ctx, output, &load->node)))
            return;
        hlsl_block_add_instr(block, store);
    }
}

static void append_output_copy_recurse(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_load *rhs,
        unsigned int modifiers, struct hlsl_semantic *semantic, uint32_t semantic_index)
{
    struct vkd3d_shader_location *loc = &rhs->node.loc;
    struct hlsl_type *type = rhs->node.data_type;
    struct hlsl_ir_var *var = rhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (type->class == HLSL_CLASS_ARRAY || type->class == HLSL_CLASS_STRUCT)
    {
        struct hlsl_ir_load *element_load;
        struct hlsl_struct_field *field;
        uint32_t elem_semantic_index;

        for (i = 0; i < hlsl_type_element_count(type); ++i)
        {
            if (type->class == HLSL_CLASS_ARRAY)
            {
                elem_semantic_index = semantic_index
                        + i * hlsl_type_get_array_element_reg_size(type->e.array.type, HLSL_REGSET_NUMERIC) / 4;
            }
            else
            {
                field = &type->e.record.fields[i];
                if (hlsl_type_is_resource(field->type))
                    continue;
                validate_field_semantic(ctx, field);
                semantic = &field->semantic;
                elem_semantic_index = semantic->index;
                loc = &field->loc;
            }

            if (!(c = hlsl_new_uint_constant(ctx, i, &var->loc)))
                return;
            hlsl_block_add_instr(block, c);

            if (!(element_load = hlsl_new_load_index(ctx, &rhs->src, c, loc)))
                return;
            hlsl_block_add_instr(block, &element_load->node);

            append_output_copy_recurse(ctx, block, element_load, modifiers, semantic, elem_semantic_index);
        }
    }
    else
    {
        append_output_copy(ctx, block, rhs, modifiers, semantic, semantic_index);
    }
}

/* Split outputs into two variables representing the temp and semantic
 * registers, and copy the former to the latter, so that reads from output
 * variables work. */
static void append_output_var_copy(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_var *var)
{
    struct hlsl_ir_load *load;

    /* This redundant load is expected to be deleted later by DCE. */
    if (!(load = hlsl_new_var_load(ctx, var, &var->loc)))
        return;
    hlsl_block_add_instr(block, &load->node);

    append_output_copy_recurse(ctx, block, load, var->storage_modifiers, &var->semantic, var->semantic.index);
}

bool hlsl_transform_ir(struct hlsl_ctx *ctx, bool (*func)(struct hlsl_ctx *ctx, struct hlsl_ir_node *, void *),
        struct hlsl_block *block, void *context)
{
    struct hlsl_ir_node *instr, *next;
    bool progress = false;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            progress |= hlsl_transform_ir(ctx, func, &iff->then_block, context);
            progress |= hlsl_transform_ir(ctx, func, &iff->else_block, context);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            progress |= hlsl_transform_ir(ctx, func, &hlsl_ir_loop(instr)->body, context);
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
            {
                progress |= hlsl_transform_ir(ctx, func, &c->body, context);
            }
        }

        progress |= func(ctx, instr, context);
    }

    return progress;
}

typedef bool (*PFN_lower_func)(struct hlsl_ctx *, struct hlsl_ir_node *, struct hlsl_block *);

static bool call_lower_func(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    PFN_lower_func func = context;
    struct hlsl_block block;

    hlsl_block_init(&block);
    if (func(ctx, instr, &block))
    {
        struct hlsl_ir_node *replacement = LIST_ENTRY(list_tail(&block.instrs), struct hlsl_ir_node, entry);

        list_move_before(&instr->entry, &block.instrs);
        hlsl_replace_node(instr, replacement);
        return true;
    }
    else
    {
        hlsl_block_cleanup(&block);
        return false;
    }
}

/* Specific form of transform_ir() for passes which convert a single instruction
 * to a block of one or more instructions. This helper takes care of setting up
 * the block and calling hlsl_replace_node_with_block(). */
static bool lower_ir(struct hlsl_ctx *ctx, PFN_lower_func func, struct hlsl_block *block)
{
    return hlsl_transform_ir(ctx, call_lower_func, block, func);
}

static bool transform_instr_derefs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    bool res;
    bool (*func)(struct hlsl_ctx *ctx, struct hlsl_deref *, struct hlsl_ir_node *) = context;

    switch(instr->type)
    {
        case HLSL_IR_LOAD:
            res = func(ctx, &hlsl_ir_load(instr)->src, instr);
            return res;

        case HLSL_IR_STORE:
            res = func(ctx, &hlsl_ir_store(instr)->lhs, instr);
            return res;

        case HLSL_IR_RESOURCE_LOAD:
            res = func(ctx, &hlsl_ir_resource_load(instr)->resource, instr);
            if (hlsl_ir_resource_load(instr)->sampler.var)
                res |= func(ctx, &hlsl_ir_resource_load(instr)->sampler, instr);
            return res;

        case HLSL_IR_RESOURCE_STORE:
            res = func(ctx, &hlsl_ir_resource_store(instr)->resource, instr);
            return res;

        default:
            return false;
    }
    return false;
}

static bool transform_derefs(struct hlsl_ctx *ctx,
        bool (*func)(struct hlsl_ctx *ctx, struct hlsl_deref *, struct hlsl_ir_node *),
        struct hlsl_block *block)
{
    return hlsl_transform_ir(ctx, transform_instr_derefs, block, func);
}

struct recursive_call_ctx
{
    const struct hlsl_ir_function_decl **backtrace;
    size_t count, capacity;
};

static bool find_recursive_calls(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct recursive_call_ctx *call_ctx = context;
    struct hlsl_ir_function_decl *decl;
    const struct hlsl_ir_call *call;
    size_t i;

    if (instr->type != HLSL_IR_CALL)
        return false;
    call = hlsl_ir_call(instr);
    decl = call->decl;

    for (i = 0; i < call_ctx->count; ++i)
    {
        if (call_ctx->backtrace[i] == decl)
        {
            hlsl_error(ctx, &call->node.loc, VKD3D_SHADER_ERROR_HLSL_RECURSIVE_CALL,
                    "Recursive call to \"%s\".", decl->func->name);
            /* Native returns E_NOTIMPL instead of E_FAIL here. */
            ctx->result = VKD3D_ERROR_NOT_IMPLEMENTED;
            return false;
        }
    }

    if (!hlsl_array_reserve(ctx, (void **)&call_ctx->backtrace, &call_ctx->capacity,
            call_ctx->count + 1, sizeof(*call_ctx->backtrace)))
        return false;
    call_ctx->backtrace[call_ctx->count++] = decl;

    hlsl_transform_ir(ctx, find_recursive_calls, &decl->body, call_ctx);

    --call_ctx->count;

    return false;
}

static void insert_early_return_break(struct hlsl_ctx *ctx,
        struct hlsl_ir_function_decl *func, struct hlsl_ir_node *cf_instr)
{
    struct hlsl_ir_node *iff, *jump;
    struct hlsl_block then_block;
    struct hlsl_ir_load *load;

    hlsl_block_init(&then_block);

    if (!(load = hlsl_new_var_load(ctx, func->early_return_var, &cf_instr->loc)))
        return;
    list_add_after(&cf_instr->entry, &load->node.entry);

    if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_BREAK, NULL, &cf_instr->loc)))
        return;
    hlsl_block_add_instr(&then_block, jump);

    if (!(iff = hlsl_new_if(ctx, &load->node, &then_block, NULL, &cf_instr->loc)))
        return;
    list_add_after(&load->node.entry, &iff->entry);
}

/* Remove HLSL_IR_JUMP_RETURN calls by altering subsequent control flow. */
static bool lower_return(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        struct hlsl_block *block, bool in_loop)
{
    struct hlsl_ir_node *return_instr = NULL, *cf_instr = NULL;
    struct hlsl_ir_node *instr, *next;
    bool has_early_return = false;

    /* SM1 has no function calls. SM4 does, but native d3dcompiler inlines
     * everything anyway. We are safest following suit.
     *
     * The basic idea is to keep track of whether the function has executed an
     * early return in a synthesized boolean variable (func->early_return_var)
     * and guard all code after the return on that variable being false. In the
     * case of loops we also replace the return with a break.
     *
     * The following algorithm loops over instructions in a block, recursing
     * into inferior CF blocks, until it hits one of the following two things:
     *
     * - A return statement. In this case, we remove everything after the return
     *   statement in this block. We have to stop and do this in a separate
     *   loop, because instructions must be deleted in reverse order (due to
     *   def-use chains.)
     *
     *   If we're inside of a loop CF block, we can instead just turn the
     *   return into a break, which offers the right semantics—except that it
     *   won't break out of nested loops.
     *
     * - A CF block which contains a return statement. After calling
     *   lower_return() on the CF block body, we stop, pull out everything after
     *   the CF instruction, shove it into an if block, and then lower that if
     *   block.
     *
     *   (We could return a "did we make progress" boolean like hlsl_transform_ir()
     *   and run this pass multiple times, but we already know the only block
     *   that still needs to be addressed, so there's not much point.)
     *
     *   If we're inside of a loop CF block, we again do things differently. We
     *   already turned any returns into breaks. If the block we just processed
     *   was conditional, then "break" did our work for us. If it was a loop,
     *   we need to propagate that break to the outer loop.
     *
     * We return true if there was an early return anywhere in the block we just
     * processed (including CF contained inside that block).
     */

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_CALL)
        {
            struct hlsl_ir_call *call = hlsl_ir_call(instr);

            lower_return(ctx, call->decl, &call->decl->body, false);
        }
        else if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            has_early_return |= lower_return(ctx, func, &iff->then_block, in_loop);
            has_early_return |= lower_return(ctx, func, &iff->else_block, in_loop);

            if (has_early_return)
            {
                /* If we're in a loop, we don't need to do anything here. We
                 * turned the return into a break, and that will already skip
                 * anything that comes after this "if" block. */
                if (!in_loop)
                {
                    cf_instr = instr;
                    break;
                }
            }
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            has_early_return |= lower_return(ctx, func, &hlsl_ir_loop(instr)->body, true);

            if (has_early_return)
            {
                if (in_loop)
                {
                    /* "instr" is a nested loop. "return" breaks out of all
                     * loops, so break out of this one too now. */
                    insert_early_return_break(ctx, func, instr);
                }
                else
                {
                    cf_instr = instr;
                    break;
                }
            }
        }
        else if (instr->type == HLSL_IR_JUMP)
        {
            struct hlsl_ir_jump *jump = hlsl_ir_jump(instr);
            struct hlsl_ir_node *constant, *store;

            if (jump->type == HLSL_IR_JUMP_RETURN)
            {
                if (!(constant = hlsl_new_bool_constant(ctx, true, &jump->node.loc)))
                    return false;
                list_add_before(&jump->node.entry, &constant->entry);

                if (!(store = hlsl_new_simple_store(ctx, func->early_return_var, constant)))
                    return false;
                list_add_after(&constant->entry, &store->entry);

                has_early_return = true;
                if (in_loop)
                {
                    jump->type = HLSL_IR_JUMP_BREAK;
                }
                else
                {
                    return_instr = instr;
                    break;
                }
            }
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
            {
                has_early_return |= lower_return(ctx, func, &c->body, true);
            }

            if (has_early_return)
            {
                if (in_loop)
                {
                    /* For a 'switch' nested in a loop append a break after the 'switch'. */
                    insert_early_return_break(ctx, func, instr);
                }
                else
                {
                    cf_instr = instr;
                    break;
                }
            }
        }
    }

    if (return_instr)
    {
        /* If we're in a loop, we should have used "break" instead. */
        assert(!in_loop);

        /* Iterate in reverse, to avoid use-after-free when unlinking sources from
         * the "uses" list. */
        LIST_FOR_EACH_ENTRY_SAFE_REV(instr, next, &block->instrs, struct hlsl_ir_node, entry)
        {
            list_remove(&instr->entry);
            hlsl_free_instr(instr);

            /* Yes, we just freed it, but we're comparing pointers. */
            if (instr == return_instr)
                break;
        }
    }
    else if (cf_instr)
    {
        struct list *tail = list_tail(&block->instrs);
        struct hlsl_ir_node *not, *iff;
        struct hlsl_block then_block;
        struct hlsl_ir_load *load;

        /* If we're in a loop, we should have used "break" instead. */
        assert(!in_loop);

        if (tail == &cf_instr->entry)
            return has_early_return;

        hlsl_block_init(&then_block);
        list_move_slice_tail(&then_block.instrs, list_next(&block->instrs, &cf_instr->entry), tail);
        lower_return(ctx, func, &then_block, in_loop);

        if (!(load = hlsl_new_var_load(ctx, func->early_return_var, &cf_instr->loc)))
            return false;
        hlsl_block_add_instr(block, &load->node);

        if (!(not = hlsl_new_unary_expr(ctx, HLSL_OP1_LOGIC_NOT, &load->node, &cf_instr->loc)))
            return false;
        hlsl_block_add_instr(block, not);

        if (!(iff = hlsl_new_if(ctx, not, &then_block, NULL, &cf_instr->loc)))
            return false;
        list_add_tail(&block->instrs, &iff->entry);
    }

    return has_early_return;
}

/* Remove HLSL_IR_CALL instructions by inlining them. */
static bool lower_calls(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    const struct hlsl_ir_function_decl *decl;
    struct hlsl_ir_call *call;
    struct hlsl_block block;

    if (instr->type != HLSL_IR_CALL)
        return false;
    call = hlsl_ir_call(instr);
    decl = call->decl;

    if (!decl->has_body)
        hlsl_error(ctx, &call->node.loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED,
                "Function \"%s\" is not defined.", decl->func->name);

    if (!hlsl_clone_block(ctx, &block, &decl->body))
        return false;
    list_move_before(&call->node.entry, &block.instrs);

    list_remove(&call->node.entry);
    hlsl_free_instr(&call->node);
    return true;
}

static struct hlsl_ir_node *add_zero_mipmap_level(struct hlsl_ctx *ctx, struct hlsl_ir_node *index,
        const struct vkd3d_shader_location *loc)
{
    unsigned int dim_count = index->data_type->dimx;
    struct hlsl_ir_node *store, *zero;
    struct hlsl_ir_load *coords_load;
    struct hlsl_deref coords_deref;
    struct hlsl_ir_var *coords;

    assert(dim_count < 4);

    if (!(coords = hlsl_new_synthetic_var(ctx, "coords",
            hlsl_get_vector_type(ctx, HLSL_TYPE_UINT, dim_count + 1), loc)))
        return NULL;

    hlsl_init_simple_deref_from_var(&coords_deref, coords);
    if (!(store = hlsl_new_store_index(ctx, &coords_deref, NULL, index, (1u << dim_count) - 1, loc)))
        return NULL;
    list_add_after(&index->entry, &store->entry);

    if (!(zero = hlsl_new_uint_constant(ctx, 0, loc)))
        return NULL;
    list_add_after(&store->entry, &zero->entry);

    if (!(store = hlsl_new_store_index(ctx, &coords_deref, NULL, zero, 1u << dim_count, loc)))
        return NULL;
    list_add_after(&zero->entry, &store->entry);

    if (!(coords_load = hlsl_new_var_load(ctx, coords, loc)))
        return NULL;
    list_add_after(&store->entry, &coords_load->node.entry);

    return &coords_load->node;
}

/* hlsl_ir_swizzle nodes that directly point to a matrix value are only a parse-time construct that
 * represents matrix swizzles (e.g. mat._m01_m23) before we know if they will be used in the lhs of
 * an assignment or as a value made from different components of the matrix. The former cases should
 * have already been split into several separate assignments, but the latter are lowered by this
 * pass. */
static bool lower_matrix_swizzles(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_swizzle *swizzle;
    struct hlsl_ir_load *var_load;
    struct hlsl_deref var_deref;
    struct hlsl_type *matrix_type;
    struct hlsl_ir_var *var;
    unsigned int x, y, k, i;

    if (instr->type != HLSL_IR_SWIZZLE)
        return false;
    swizzle = hlsl_ir_swizzle(instr);
    matrix_type = swizzle->val.node->data_type;
    if (matrix_type->class != HLSL_CLASS_MATRIX)
        return false;

    if (!(var = hlsl_new_synthetic_var(ctx, "matrix-swizzle", instr->data_type, &instr->loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    for (i = 0; i < instr->data_type->dimx; ++i)
    {
        struct hlsl_block store_block;
        struct hlsl_ir_node *load;

        y = (swizzle->swizzle >> (8 * i + 4)) & 0xf;
        x = (swizzle->swizzle >> 8 * i) & 0xf;
        k = y * matrix_type->dimx + x;

        if (!(load = hlsl_add_load_component(ctx, block, swizzle->val.node, k, &instr->loc)))
            return false;

        if (!hlsl_new_store_component(ctx, &store_block, &var_deref, i, load))
            return false;
        hlsl_block_add_block(block, &store_block);
    }

    if (!(var_load = hlsl_new_var_load(ctx, var, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, &var_load->node);

    return true;
}

/* hlsl_ir_index nodes are a parse-time construct used to represent array indexing and struct
 * record access before knowing if they will be used in the lhs of an assignment --in which case
 * they are lowered into a deref-- or as the load of an element within a larger value.
 * For the latter case, this pass takes care of lowering hlsl_ir_indexes into individual
 * hlsl_ir_loads, or individual hlsl_ir_resource_loads, in case the indexing is a
 * resource access. */
static bool lower_index_loads(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *val, *store;
    struct hlsl_deref var_deref;
    struct hlsl_ir_index *index;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;

    if (instr->type != HLSL_IR_INDEX)
        return false;
    index = hlsl_ir_index(instr);
    val = index->val.node;

    if (hlsl_index_is_resource_access(index))
    {
        unsigned int dim_count = hlsl_sampler_dim_count(val->data_type->sampler_dim);
        struct hlsl_ir_node *coords = index->idx.node;
        struct hlsl_resource_load_params params = {0};
        struct hlsl_ir_node *load;

        assert(coords->data_type->class == HLSL_CLASS_VECTOR);
        assert(coords->data_type->base_type == HLSL_TYPE_UINT);
        assert(coords->data_type->dimx == dim_count);

        if (!(coords = add_zero_mipmap_level(ctx, coords, &instr->loc)))
            return false;

        params.type = HLSL_RESOURCE_LOAD;
        params.resource = val;
        params.coords = coords;
        params.format = val->data_type->e.resource_format;

        if (!(load = hlsl_new_resource_load(ctx, &params, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, load);
        return true;
    }

    if (!(var = hlsl_new_synthetic_var(ctx, "index-val", val->data_type, &instr->loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    if (!(store = hlsl_new_simple_store(ctx, var, val)))
        return false;
    hlsl_block_add_instr(block, store);

    if (hlsl_index_is_noncontiguous(index))
    {
        struct hlsl_ir_node *mat = index->val.node;
        struct hlsl_deref row_deref;
        unsigned int i;

        assert(!hlsl_type_is_row_major(mat->data_type));

        if (!(var = hlsl_new_synthetic_var(ctx, "row", instr->data_type, &instr->loc)))
            return false;
        hlsl_init_simple_deref_from_var(&row_deref, var);

        for (i = 0; i < mat->data_type->dimx; ++i)
        {
            struct hlsl_ir_node *c;

            if (!(c = hlsl_new_uint_constant(ctx, i, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, c);

            if (!(load = hlsl_new_load_index(ctx, &var_deref, c, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, &load->node);

            if (!(load = hlsl_new_load_index(ctx, &load->src, index->idx.node, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, &load->node);

            if (!(store = hlsl_new_store_index(ctx, &row_deref, c, &load->node, 0, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, store);
        }

        if (!(load = hlsl_new_var_load(ctx, var, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, &load->node);
    }
    else
    {
        if (!(load = hlsl_new_load_index(ctx, &var_deref, index->idx.node, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, &load->node);
    }
    return true;
}

/* Lower casts from vec1 to vecN to swizzles. */
static bool lower_broadcasts(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    const struct hlsl_type *src_type, *dst_type;
    struct hlsl_type *dst_scalar_type;
    struct hlsl_ir_expr *cast;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    cast = hlsl_ir_expr(instr);
    if (cast->op != HLSL_OP1_CAST)
        return false;
    src_type = cast->operands[0].node->data_type;
    dst_type = cast->node.data_type;

    if (src_type->class <= HLSL_CLASS_VECTOR && dst_type->class <= HLSL_CLASS_VECTOR && src_type->dimx == 1)
    {
        struct hlsl_ir_node *new_cast, *swizzle;

        dst_scalar_type = hlsl_get_scalar_type(ctx, dst_type->base_type);
        /* We need to preserve the cast since it might be doing more than just
         * turning the scalar into a vector. */
        if (!(new_cast = hlsl_new_cast(ctx, cast->operands[0].node, dst_scalar_type, &cast->node.loc)))
            return false;
        hlsl_block_add_instr(block, new_cast);

        if (dst_type->dimx != 1)
        {
            if (!(swizzle = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, X, X, X), dst_type->dimx, new_cast, &cast->node.loc)))
                return false;
            hlsl_block_add_instr(block, swizzle);
        }

        return true;
    }

    return false;
}

/* Allocate a unique, ordered index to each instruction, which will be used for
 * copy propagation and computing liveness ranges.
 * Index 0 means unused; index 1 means function entry, so start at 2. */
static unsigned int index_instructions(struct hlsl_block *block, unsigned int index)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        instr->index = index++;

        if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);
            index = index_instructions(&iff->then_block, index);
            index = index_instructions(&iff->else_block, index);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            index = index_instructions(&hlsl_ir_loop(instr)->body, index);
            hlsl_ir_loop(instr)->next_index = index;
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
            {
                index = index_instructions(&c->body, index);
            }
        }
    }

    return index;
}

/*
 * Copy propagation. The basic idea is to recognize instruction sequences of the
 * form:
 *
 *   2: <any instruction>
 *   3: v = @2
 *   4: load(v)
 *
 * and replace the load (@4) with the original instruction (@2).
 * This works for multiple components, even if they're written using separate
 * store instructions, as long as the rhs is the same in every case. This basic
 * detection is implemented by copy_propagation_replace_with_single_instr().
 *
 * In some cases, the load itself might not have a single source, but a
 * subsequent swizzle might; hence we also try to replace swizzles of loads.
 *
 * We use the same infrastructure to implement a more specialized
 * transformation. We recognize sequences of the form:
 *
 *   2: 123
 *   3: var.x = @2
 *   4: 345
 *   5: var.y = @4
 *   6: load(var.xy)
 *
 * where the load (@6) originates from different sources but that are constant,
 * and transform it into a single constant vector. This latter pass is done
 * by copy_propagation_replace_with_constant_vector().
 *
 * This is a specialized form of vectorization, and begs the question: why does
 * the load need to be involved? Can we just vectorize the stores into a single
 * instruction, and then use "normal" copy-prop to convert that into a single
 * vector?
 *
 * In general, the answer is yes, but there is a special case which necessitates
 * the use of this transformation: non-uniform control flow. Copy-prop can act
 * across some control flow, and in cases like the following:
 *
 *   2: 123
 *   3: var.x = @2
 *   4: if (...)
 *   5:    456
 *   6:    var.y = @5
 *   7:    load(var.xy)
 *
 * we can copy-prop the load (@7) into a constant vector {123, 456}, but we
 * cannot easily vectorize the stores @3 and @6.
 */

struct copy_propagation_value
{
    unsigned int timestamp;
    /* If node is NULL, the value was dynamically written and thus, it is unknown.*/
    struct hlsl_ir_node *node;
    unsigned int component;
};

struct copy_propagation_component_trace
{
    struct copy_propagation_value *records;
    size_t record_count, record_capacity;
};

struct copy_propagation_var_def
{
    struct rb_entry entry;
    struct hlsl_ir_var *var;
    struct copy_propagation_component_trace traces[];
};

struct copy_propagation_state
{
    struct rb_tree var_defs;
    struct copy_propagation_state *parent;
};

static int copy_propagation_var_def_compare(const void *key, const struct rb_entry *entry)
{
    struct copy_propagation_var_def *var_def = RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);
    uintptr_t key_int = (uintptr_t)key, entry_int = (uintptr_t)var_def->var;

    return (key_int > entry_int) - (key_int < entry_int);
}

static void copy_propagation_var_def_destroy(struct rb_entry *entry, void *context)
{
    struct copy_propagation_var_def *var_def = RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);
    unsigned int component_count = hlsl_type_component_count(var_def->var->data_type);
    unsigned int i;

    for (i = 0; i < component_count; ++i)
        vkd3d_free(var_def->traces[i].records);
    vkd3d_free(var_def);
}

static struct copy_propagation_value *copy_propagation_get_value_at_time(
        struct copy_propagation_component_trace *trace, unsigned int time)
{
    int r;

    for (r = trace->record_count - 1; r >= 0; --r)
    {
        if (trace->records[r].timestamp < time)
            return &trace->records[r];
    }

    return NULL;
}

static struct copy_propagation_value *copy_propagation_get_value(const struct copy_propagation_state *state,
        const struct hlsl_ir_var *var, unsigned int component, unsigned int time)
{
    for (; state; state = state->parent)
    {
        struct rb_entry *entry = rb_get(&state->var_defs, var);
        if (entry)
        {
            struct copy_propagation_var_def *var_def = RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);
            unsigned int component_count = hlsl_type_component_count(var->data_type);
            struct copy_propagation_value *value;

            assert(component < component_count);
            value = copy_propagation_get_value_at_time(&var_def->traces[component], time);

            if (!value)
                continue;

            if (value->node)
                return value;
            else
                return NULL;
        }
    }

    return NULL;
}

static struct copy_propagation_var_def *copy_propagation_create_var_def(struct hlsl_ctx *ctx,
        struct copy_propagation_state *state, struct hlsl_ir_var *var)
{
    struct rb_entry *entry = rb_get(&state->var_defs, var);
    struct copy_propagation_var_def *var_def;
    unsigned int component_count = hlsl_type_component_count(var->data_type);
    int res;

    if (entry)
        return RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);

    if (!(var_def = hlsl_alloc(ctx, offsetof(struct copy_propagation_var_def, traces[component_count]))))
        return NULL;

    var_def->var = var;

    res = rb_put(&state->var_defs, var, &var_def->entry);
    assert(!res);

    return var_def;
}

static void copy_propagation_trace_record_value(struct hlsl_ctx *ctx,
        struct copy_propagation_component_trace *trace, struct hlsl_ir_node *node,
        unsigned int component, unsigned int time)
{
    assert(!trace->record_count || trace->records[trace->record_count - 1].timestamp < time);

    if (!hlsl_array_reserve(ctx, (void **)&trace->records, &trace->record_capacity,
            trace->record_count + 1, sizeof(trace->records[0])))
        return;

    trace->records[trace->record_count].timestamp = time;
    trace->records[trace->record_count].node = node;
    trace->records[trace->record_count].component = component;

    ++trace->record_count;
}

static void copy_propagation_invalidate_variable(struct hlsl_ctx *ctx, struct copy_propagation_var_def *var_def,
        unsigned int comp, unsigned char writemask, unsigned int time)
{
    unsigned i;

    TRACE("Invalidate variable %s[%u]%s.\n", var_def->var->name, comp, debug_hlsl_writemask(writemask));

    for (i = 0; i < 4; ++i)
    {
        if (writemask & (1u << i))
        {
            struct copy_propagation_component_trace *trace = &var_def->traces[comp + i];

            /* Don't add an invalidate record if it is already present. */
            if (trace->record_count && trace->records[trace->record_count - 1].timestamp == time)
            {
                assert(!trace->records[trace->record_count - 1].node);
                continue;
            }

            copy_propagation_trace_record_value(ctx, trace, NULL, 0, time);
        }
    }
}

static void copy_propagation_invalidate_variable_from_deref_recurse(struct hlsl_ctx *ctx,
        struct copy_propagation_var_def *var_def, const struct hlsl_deref *deref,
        struct hlsl_type *type, unsigned int depth, unsigned int comp_start, unsigned char writemask,
        unsigned int time)
{
    unsigned int i, subtype_comp_count;
    struct hlsl_ir_node *path_node;
    struct hlsl_type *subtype;

    if (depth == deref->path_len)
    {
        copy_propagation_invalidate_variable(ctx, var_def, comp_start, writemask, time);
        return;
    }

    path_node = deref->path[depth].node;
    subtype = hlsl_get_element_type_from_path_index(ctx, type, path_node);

    if (type->class == HLSL_CLASS_STRUCT)
    {
        unsigned int idx = hlsl_ir_constant(path_node)->value.u[0].u;

        for (i = 0; i < idx; ++i)
            comp_start += hlsl_type_component_count(type->e.record.fields[i].type);

        copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, subtype,
                depth + 1, comp_start, writemask, time);
    }
    else
    {
        subtype_comp_count = hlsl_type_component_count(subtype);

        if (path_node->type == HLSL_IR_CONSTANT)
        {
            copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, subtype,
                    depth + 1, hlsl_ir_constant(path_node)->value.u[0].u * subtype_comp_count,
                    writemask, time);
        }
        else
        {
            for (i = 0; i < hlsl_type_element_count(type); ++i)
            {
                copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, subtype,
                        depth + 1, i * subtype_comp_count, writemask, time);
            }
        }
    }
}

static void copy_propagation_invalidate_variable_from_deref(struct hlsl_ctx *ctx,
        struct copy_propagation_var_def *var_def, const struct hlsl_deref *deref,
        unsigned char writemask, unsigned int time)
{
    copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, deref->var->data_type,
            0, 0, writemask, time);
}

static void copy_propagation_set_value(struct hlsl_ctx *ctx, struct copy_propagation_var_def *var_def,
        unsigned int comp, unsigned char writemask, struct hlsl_ir_node *instr, unsigned int time)
{
    unsigned int i, j = 0;

    for (i = 0; i < 4; ++i)
    {
        if (writemask & (1u << i))
        {
            struct copy_propagation_component_trace *trace = &var_def->traces[comp + i];

            TRACE("Variable %s[%u] is written by instruction %p%s.\n",
                    var_def->var->name, comp + i, instr, debug_hlsl_writemask(1u << i));

            copy_propagation_trace_record_value(ctx, trace, instr, j++, time);
        }
    }
}

static bool copy_propagation_replace_with_single_instr(struct hlsl_ctx *ctx,
        const struct copy_propagation_state *state, const struct hlsl_ir_load *load,
        unsigned int swizzle, struct hlsl_ir_node *instr)
{
    const unsigned int instr_component_count = hlsl_type_component_count(instr->data_type);
    const struct hlsl_deref *deref = &load->src;
    const struct hlsl_ir_var *var = deref->var;
    struct hlsl_ir_node *new_instr = NULL;
    unsigned int time = load->node.index;
    unsigned int start, count, i;
    unsigned int ret_swizzle = 0;

    if (!hlsl_component_index_range_from_deref(ctx, deref, &start, &count))
        return false;

    for (i = 0; i < instr_component_count; ++i)
    {
        struct copy_propagation_value *value;

        if (!(value = copy_propagation_get_value(state, var, start + hlsl_swizzle_get_component(swizzle, i),
                time)))
            return false;

        if (!new_instr)
        {
            new_instr = value->node;
        }
        else if (new_instr != value->node)
        {
            TRACE("No single source for propagating load from %s[%u-%u]%s\n",
                    var->name, start, start + count, debug_hlsl_swizzle(swizzle, instr_component_count));
            return false;
        }
        ret_swizzle |= value->component << HLSL_SWIZZLE_SHIFT(i);
    }

    TRACE("Load from %s[%u-%u]%s propagated as instruction %p%s.\n",
            var->name, start, start + count, debug_hlsl_swizzle(swizzle, instr_component_count),
            new_instr, debug_hlsl_swizzle(ret_swizzle, instr_component_count));

    if (instr->data_type->class != HLSL_CLASS_OBJECT)
    {
        struct hlsl_ir_node *swizzle_node;

        if (!(swizzle_node = hlsl_new_swizzle(ctx, ret_swizzle, instr_component_count, new_instr, &instr->loc)))
            return false;
        list_add_before(&instr->entry, &swizzle_node->entry);
        new_instr = swizzle_node;
    }

    hlsl_replace_node(instr, new_instr);
    return true;
}

static bool copy_propagation_replace_with_constant_vector(struct hlsl_ctx *ctx,
        const struct copy_propagation_state *state, const struct hlsl_ir_load *load,
        unsigned int swizzle, struct hlsl_ir_node *instr)
{
    const unsigned int instr_component_count = hlsl_type_component_count(instr->data_type);
    const struct hlsl_deref *deref = &load->src;
    const struct hlsl_ir_var *var = deref->var;
    struct hlsl_constant_value values = {0};
    unsigned int time = load->node.index;
    unsigned int start, count, i;
    struct hlsl_ir_node *cons;

    if (!hlsl_component_index_range_from_deref(ctx, deref, &start, &count))
        return false;

    for (i = 0; i < instr_component_count; ++i)
    {
        struct copy_propagation_value *value;

        if (!(value = copy_propagation_get_value(state, var, start + hlsl_swizzle_get_component(swizzle, i),
                time)) || value->node->type != HLSL_IR_CONSTANT)
            return false;

        values.u[i] = hlsl_ir_constant(value->node)->value.u[value->component];
    }

    if (!(cons = hlsl_new_constant(ctx, instr->data_type, &values, &instr->loc)))
        return false;
    list_add_before(&instr->entry, &cons->entry);

    TRACE("Load from %s[%u-%u]%s turned into a constant %p.\n",
            var->name, start, start + count, debug_hlsl_swizzle(swizzle, instr_component_count), cons);

    hlsl_replace_node(instr, cons);
    return true;
}

static bool copy_propagation_transform_load(struct hlsl_ctx *ctx,
        struct hlsl_ir_load *load, struct copy_propagation_state *state)
{
    struct hlsl_type *type = load->node.data_type;

    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_OBJECT:
            break;

        case HLSL_CLASS_MATRIX:
        case HLSL_CLASS_ARRAY:
        case HLSL_CLASS_STRUCT:
            /* FIXME: Actually we shouldn't even get here, but we don't split
             * matrices yet. */
            return false;
    }

    if (copy_propagation_replace_with_constant_vector(ctx, state, load, HLSL_SWIZZLE(X, Y, Z, W), &load->node))
        return true;

    if (copy_propagation_replace_with_single_instr(ctx, state, load, HLSL_SWIZZLE(X, Y, Z, W), &load->node))
        return true;

    return false;
}

static bool copy_propagation_transform_swizzle(struct hlsl_ctx *ctx,
        struct hlsl_ir_swizzle *swizzle, struct copy_propagation_state *state)
{
    struct hlsl_ir_load *load;

    if (swizzle->val.node->type != HLSL_IR_LOAD)
        return false;
    load = hlsl_ir_load(swizzle->val.node);

    if (copy_propagation_replace_with_constant_vector(ctx, state, load, swizzle->swizzle, &swizzle->node))
        return true;

    if (copy_propagation_replace_with_single_instr(ctx, state, load, swizzle->swizzle, &swizzle->node))
        return true;

    return false;
}

static bool copy_propagation_transform_object_load(struct hlsl_ctx *ctx,
        struct hlsl_deref *deref, struct copy_propagation_state *state, unsigned int time)
{
    struct copy_propagation_value *value;
    struct hlsl_ir_load *load;
    unsigned int start, count;

    if (!hlsl_component_index_range_from_deref(ctx, deref, &start, &count))
        return false;
    assert(count == 1);

    if (!(value = copy_propagation_get_value(state, deref->var, start, time)))
        return false;
    assert(value->component == 0);

    /* Only HLSL_IR_LOAD can produce an object. */
    load = hlsl_ir_load(value->node);

    /* As we are replacing the instruction's deref (with the one in the hlsl_ir_load) and not the
     * instruction itself, we won't be able to rely on the value retrieved by
     * copy_propagation_get_value() for the new deref in subsequent iterations of copy propagation.
     * This is because another value may be written to that deref between the hlsl_ir_load and
     * this instruction.
     *
     * For this reason, we only replace the new deref when it corresponds to a uniform variable,
     * which cannot be written to.
     *
     * In a valid shader, all object references must resolve statically to a single uniform object.
     * If this is the case, we can expect copy propagation on regular store/loads and the other
     * compilation passes to replace all hlsl_ir_loads with loads to uniform objects, so this
     * implementation is complete, even with this restriction.
     */
    if (!load->src.var->is_uniform)
    {
        TRACE("Ignoring load from non-uniform object variable %s\n", load->src.var->name);
        return false;
    }

    hlsl_cleanup_deref(deref);
    hlsl_copy_deref(ctx, deref, &load->src);

    return true;
}

static bool copy_propagation_transform_resource_load(struct hlsl_ctx *ctx,
        struct hlsl_ir_resource_load *load, struct copy_propagation_state *state)
{
    bool progress = false;

    progress |= copy_propagation_transform_object_load(ctx, &load->resource, state, load->node.index);
    if (load->sampler.var)
        progress |= copy_propagation_transform_object_load(ctx, &load->sampler, state, load->node.index);
    return progress;
}

static bool copy_propagation_transform_resource_store(struct hlsl_ctx *ctx,
        struct hlsl_ir_resource_store *store, struct copy_propagation_state *state)
{
    bool progress = false;

    progress |= copy_propagation_transform_object_load(ctx, &store->resource, state, store->node.index);
    return progress;
}

static void copy_propagation_record_store(struct hlsl_ctx *ctx, struct hlsl_ir_store *store,
        struct copy_propagation_state *state)
{
    struct copy_propagation_var_def *var_def;
    struct hlsl_deref *lhs = &store->lhs;
    struct hlsl_ir_var *var = lhs->var;
    unsigned int start, count;

    if (!(var_def = copy_propagation_create_var_def(ctx, state, var)))
        return;

    if (hlsl_component_index_range_from_deref(ctx, lhs, &start, &count))
    {
        unsigned int writemask = store->writemask;

        if (store->rhs.node->data_type->class == HLSL_CLASS_OBJECT)
            writemask = VKD3DSP_WRITEMASK_0;
        copy_propagation_set_value(ctx, var_def, start, writemask, store->rhs.node, store->node.index);
    }
    else
    {
        copy_propagation_invalidate_variable_from_deref(ctx, var_def, lhs, store->writemask,
                store->node.index);
    }
}

static void copy_propagation_state_init(struct hlsl_ctx *ctx, struct copy_propagation_state *state,
        struct copy_propagation_state *parent)
{
    rb_init(&state->var_defs, copy_propagation_var_def_compare);
    state->parent = parent;
}

static void copy_propagation_state_destroy(struct copy_propagation_state *state)
{
    rb_destroy(&state->var_defs, copy_propagation_var_def_destroy, NULL);
}

static void copy_propagation_invalidate_from_block(struct hlsl_ctx *ctx, struct copy_propagation_state *state,
        struct hlsl_block *block, unsigned int time)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (instr->type)
        {
            case HLSL_IR_STORE:
            {
                struct hlsl_ir_store *store = hlsl_ir_store(instr);
                struct copy_propagation_var_def *var_def;
                struct hlsl_deref *lhs = &store->lhs;
                struct hlsl_ir_var *var = lhs->var;

                if (!(var_def = copy_propagation_create_var_def(ctx, state, var)))
                    continue;

                copy_propagation_invalidate_variable_from_deref(ctx, var_def, lhs, store->writemask, time);

                break;
            }

            case HLSL_IR_IF:
            {
                struct hlsl_ir_if *iff = hlsl_ir_if(instr);

                copy_propagation_invalidate_from_block(ctx, state, &iff->then_block, time);
                copy_propagation_invalidate_from_block(ctx, state, &iff->else_block, time);

                break;
            }

            case HLSL_IR_LOOP:
            {
                struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);

                copy_propagation_invalidate_from_block(ctx, state, &loop->body, time);

                break;
            }

            case HLSL_IR_SWITCH:
            {
                struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
                struct hlsl_ir_switch_case *c;

                LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                {
                    copy_propagation_invalidate_from_block(ctx, state, &c->body, time);
                }

                break;
            }

            default:
                break;
        }
    }
}

static bool copy_propagation_transform_block(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct copy_propagation_state *state);

static bool copy_propagation_process_if(struct hlsl_ctx *ctx, struct hlsl_ir_if *iff,
        struct copy_propagation_state *state)
{
    struct copy_propagation_state inner_state;
    bool progress = false;

    copy_propagation_state_init(ctx, &inner_state, state);
    progress |= copy_propagation_transform_block(ctx, &iff->then_block, &inner_state);
    copy_propagation_state_destroy(&inner_state);

    copy_propagation_state_init(ctx, &inner_state, state);
    progress |= copy_propagation_transform_block(ctx, &iff->else_block, &inner_state);
    copy_propagation_state_destroy(&inner_state);

    /* Ideally we'd invalidate the outer state looking at what was
     * touched in the two inner states, but this doesn't work for
     * loops (because we need to know what is invalidated in advance),
     * so we need copy_propagation_invalidate_from_block() anyway. */
    copy_propagation_invalidate_from_block(ctx, state, &iff->then_block, iff->node.index);
    copy_propagation_invalidate_from_block(ctx, state, &iff->else_block, iff->node.index);

    return progress;
}

static bool copy_propagation_process_loop(struct hlsl_ctx *ctx, struct hlsl_ir_loop *loop,
        struct copy_propagation_state *state)
{
    struct copy_propagation_state inner_state;
    bool progress = false;

    copy_propagation_invalidate_from_block(ctx, state, &loop->body, loop->node.index);

    copy_propagation_state_init(ctx, &inner_state, state);
    progress |= copy_propagation_transform_block(ctx, &loop->body, &inner_state);
    copy_propagation_state_destroy(&inner_state);

    return progress;
}

static bool copy_propagation_process_switch(struct hlsl_ctx *ctx, struct hlsl_ir_switch *s,
        struct copy_propagation_state *state)
{
    struct copy_propagation_state inner_state;
    struct hlsl_ir_switch_case *c;
    bool progress = false;

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        copy_propagation_state_init(ctx, &inner_state, state);
        progress |= copy_propagation_transform_block(ctx, &c->body, &inner_state);
        copy_propagation_state_destroy(&inner_state);
    }

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        copy_propagation_invalidate_from_block(ctx, state, &c->body, s->node.index);
    }

    return progress;
}

static bool copy_propagation_transform_block(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct copy_propagation_state *state)
{
    struct hlsl_ir_node *instr, *next;
    bool progress = false;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (instr->type)
        {
            case HLSL_IR_LOAD:
                progress |= copy_propagation_transform_load(ctx, hlsl_ir_load(instr), state);
                break;

            case HLSL_IR_RESOURCE_LOAD:
                progress |= copy_propagation_transform_resource_load(ctx, hlsl_ir_resource_load(instr), state);
                break;

            case HLSL_IR_RESOURCE_STORE:
                progress |= copy_propagation_transform_resource_store(ctx, hlsl_ir_resource_store(instr), state);
                break;

            case HLSL_IR_STORE:
                copy_propagation_record_store(ctx, hlsl_ir_store(instr), state);
                break;

            case HLSL_IR_SWIZZLE:
                progress |= copy_propagation_transform_swizzle(ctx, hlsl_ir_swizzle(instr), state);
                break;

            case HLSL_IR_IF:
                progress |= copy_propagation_process_if(ctx, hlsl_ir_if(instr), state);
                break;

            case HLSL_IR_LOOP:
                progress |= copy_propagation_process_loop(ctx, hlsl_ir_loop(instr), state);
                break;

            case HLSL_IR_SWITCH:
                progress |= copy_propagation_process_switch(ctx, hlsl_ir_switch(instr), state);
                break;

            default:
                break;
        }
    }

    return progress;
}

bool hlsl_copy_propagation_execute(struct hlsl_ctx *ctx, struct hlsl_block *block)
{
    struct copy_propagation_state state;
    bool progress;

    index_instructions(block, 2);

    copy_propagation_state_init(ctx, &state, NULL);

    progress = copy_propagation_transform_block(ctx, block, &state);

    copy_propagation_state_destroy(&state);

    return progress;
}

static void note_non_static_deref_expressions(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        const char *usage)
{
    unsigned int i;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;

        assert(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            hlsl_note(ctx, &path_node->loc, VKD3D_SHADER_LOG_ERROR,
                    "Expression for %s within \"%s\" cannot be resolved statically.",
                    usage, deref->var->name);
    }
}

static bool validate_static_object_references(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr,
        void *context)
{
    unsigned int start, count;

    if (instr->type == HLSL_IR_RESOURCE_LOAD)
    {
        struct hlsl_ir_resource_load *load = hlsl_ir_resource_load(instr);

        if (!load->resource.var->is_uniform)
        {
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                    "Loaded resource must have a single uniform source.");
        }
        else if (!hlsl_component_index_range_from_deref(ctx, &load->resource, &start, &count))
        {
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                    "Loaded resource from \"%s\" must be determinable at compile time.",
                    load->resource.var->name);
            note_non_static_deref_expressions(ctx, &load->resource, "loaded resource");
        }

        if (load->sampler.var)
        {
            if (!load->sampler.var->is_uniform)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Resource load sampler must have a single uniform source.");
            }
            else if (!hlsl_component_index_range_from_deref(ctx, &load->sampler, &start, &count))
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Resource load sampler from \"%s\" must be determinable at compile time.",
                        load->sampler.var->name);
                note_non_static_deref_expressions(ctx, &load->sampler, "resource load sampler");
            }
        }
    }
    else if (instr->type == HLSL_IR_RESOURCE_STORE)
    {
        struct hlsl_ir_resource_store *store = hlsl_ir_resource_store(instr);

        if (!store->resource.var->is_uniform)
        {
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                    "Accessed resource must have a single uniform source.");
        }
        else if (!hlsl_component_index_range_from_deref(ctx, &store->resource, &start, &count))
        {
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                    "Accessed resource from \"%s\" must be determinable at compile time.",
                    store->resource.var->name);
            note_non_static_deref_expressions(ctx, &store->resource, "accessed resource");
        }
    }

    return false;
}

static bool is_vec1(const struct hlsl_type *type)
{
    return (type->class == HLSL_CLASS_SCALAR) || (type->class == HLSL_CLASS_VECTOR && type->dimx == 1);
}

static bool fold_redundant_casts(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    if (instr->type == HLSL_IR_EXPR)
    {
        struct hlsl_ir_expr *expr = hlsl_ir_expr(instr);
        const struct hlsl_type *dst_type = expr->node.data_type;
        const struct hlsl_type *src_type;

        if (expr->op != HLSL_OP1_CAST)
            return false;

        src_type = expr->operands[0].node->data_type;

        if (hlsl_types_are_equal(src_type, dst_type)
                || (src_type->base_type == dst_type->base_type && is_vec1(src_type) && is_vec1(dst_type)))
        {
            hlsl_replace_node(&expr->node, expr->operands[0].node);
            return true;
        }
    }

    return false;
}

/* Copy an element of a complex variable. Helper for
 * split_array_copies(), split_struct_copies() and
 * split_matrix_copies(). Inserts new instructions right before
 * "store". */
static bool split_copy(struct hlsl_ctx *ctx, struct hlsl_ir_store *store,
        const struct hlsl_ir_load *load, const unsigned int idx, struct hlsl_type *type)
{
    struct hlsl_ir_node *split_store, *c;
    struct hlsl_ir_load *split_load;

    if (!(c = hlsl_new_uint_constant(ctx, idx, &store->node.loc)))
        return false;
    list_add_before(&store->node.entry, &c->entry);

    if (!(split_load = hlsl_new_load_index(ctx, &load->src, c, &store->node.loc)))
        return false;
    list_add_before(&store->node.entry, &split_load->node.entry);

    if (!(split_store = hlsl_new_store_index(ctx, &store->lhs, c, &split_load->node, 0, &store->node.loc)))
        return false;
    list_add_before(&store->node.entry, &split_store->entry);

    return true;
}

static bool split_array_copies(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    const struct hlsl_ir_node *rhs;
    struct hlsl_type *element_type;
    const struct hlsl_type *type;
    struct hlsl_ir_store *store;
    unsigned int i;

    if (instr->type != HLSL_IR_STORE)
        return false;

    store = hlsl_ir_store(instr);
    rhs = store->rhs.node;
    type = rhs->data_type;
    if (type->class != HLSL_CLASS_ARRAY)
        return false;
    element_type = type->e.array.type;

    if (rhs->type != HLSL_IR_LOAD)
    {
        hlsl_fixme(ctx, &instr->loc, "Array store rhs is not HLSL_IR_LOAD. Broadcast may be missing.");
        return false;
    }

    for (i = 0; i < type->e.array.elements_count; ++i)
    {
        if (!split_copy(ctx, store, hlsl_ir_load(rhs), i, element_type))
            return false;
    }

    /* Remove the store instruction, so that we can split structs which contain
     * other structs. Although assignments produce a value, we don't allow
     * HLSL_IR_STORE to be used as a source. */
    list_remove(&store->node.entry);
    hlsl_free_instr(&store->node);
    return true;
}

static bool split_struct_copies(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    const struct hlsl_ir_node *rhs;
    const struct hlsl_type *type;
    struct hlsl_ir_store *store;
    size_t i;

    if (instr->type != HLSL_IR_STORE)
        return false;

    store = hlsl_ir_store(instr);
    rhs = store->rhs.node;
    type = rhs->data_type;
    if (type->class != HLSL_CLASS_STRUCT)
        return false;

    if (rhs->type != HLSL_IR_LOAD)
    {
        hlsl_fixme(ctx, &instr->loc, "Struct store rhs is not HLSL_IR_LOAD. Broadcast may be missing.");
        return false;
    }

    for (i = 0; i < type->e.record.field_count; ++i)
    {
        const struct hlsl_struct_field *field = &type->e.record.fields[i];

        if (!split_copy(ctx, store, hlsl_ir_load(rhs), i, field->type))
            return false;
    }

    /* Remove the store instruction, so that we can split structs which contain
     * other structs. Although assignments produce a value, we don't allow
     * HLSL_IR_STORE to be used as a source. */
    list_remove(&store->node.entry);
    hlsl_free_instr(&store->node);
    return true;
}

static bool split_matrix_copies(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    const struct hlsl_ir_node *rhs;
    struct hlsl_type *element_type;
    const struct hlsl_type *type;
    unsigned int i;
    struct hlsl_ir_store *store;

    if (instr->type != HLSL_IR_STORE)
        return false;

    store = hlsl_ir_store(instr);
    rhs = store->rhs.node;
    type = rhs->data_type;
    if (type->class != HLSL_CLASS_MATRIX)
        return false;
    element_type = hlsl_get_vector_type(ctx, type->base_type, hlsl_type_minor_size(type));

    if (rhs->type != HLSL_IR_LOAD)
    {
        hlsl_fixme(ctx, &instr->loc, "Copying from unsupported node type.");
        return false;
    }

    for (i = 0; i < hlsl_type_major_size(type); ++i)
    {
        if (!split_copy(ctx, store, hlsl_ir_load(rhs), i, element_type))
            return false;
    }

    list_remove(&store->node.entry);
    hlsl_free_instr(&store->node);
    return true;
}

static bool lower_narrowing_casts(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    const struct hlsl_type *src_type, *dst_type;
    struct hlsl_type *dst_vector_type;
    struct hlsl_ir_expr *cast;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    cast = hlsl_ir_expr(instr);
    if (cast->op != HLSL_OP1_CAST)
        return false;
    src_type = cast->operands[0].node->data_type;
    dst_type = cast->node.data_type;

    if (src_type->class <= HLSL_CLASS_VECTOR && dst_type->class <= HLSL_CLASS_VECTOR && dst_type->dimx < src_type->dimx)
    {
        struct hlsl_ir_node *new_cast, *swizzle;

        dst_vector_type = hlsl_get_vector_type(ctx, dst_type->base_type, src_type->dimx);
        /* We need to preserve the cast since it might be doing more than just
         * narrowing the vector. */
        if (!(new_cast = hlsl_new_cast(ctx, cast->operands[0].node, dst_vector_type, &cast->node.loc)))
            return false;
        hlsl_block_add_instr(block, new_cast);

        if (!(swizzle = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, Y, Z, W), dst_type->dimx, new_cast, &cast->node.loc)))
            return false;
        hlsl_block_add_instr(block, swizzle);

        return true;
    }

    return false;
}

static bool fold_swizzle_chains(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_swizzle *swizzle;
    struct hlsl_ir_node *next_instr;

    if (instr->type != HLSL_IR_SWIZZLE)
        return false;
    swizzle = hlsl_ir_swizzle(instr);

    next_instr = swizzle->val.node;

    if (next_instr->type == HLSL_IR_SWIZZLE)
    {
        struct hlsl_ir_node *new_swizzle;
        unsigned int combined_swizzle;

        combined_swizzle = hlsl_combine_swizzles(hlsl_ir_swizzle(next_instr)->swizzle,
                swizzle->swizzle, instr->data_type->dimx);
        next_instr = hlsl_ir_swizzle(next_instr)->val.node;

        if (!(new_swizzle = hlsl_new_swizzle(ctx, combined_swizzle, instr->data_type->dimx, next_instr, &instr->loc)))
            return false;

        list_add_before(&instr->entry, &new_swizzle->entry);
        hlsl_replace_node(instr, new_swizzle);
        return true;
    }

    return false;
}

static bool remove_trivial_swizzles(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_swizzle *swizzle;
    unsigned int i;

    if (instr->type != HLSL_IR_SWIZZLE)
        return false;
    swizzle = hlsl_ir_swizzle(instr);

    if (instr->data_type->dimx != swizzle->val.node->data_type->dimx)
        return false;

    for (i = 0; i < instr->data_type->dimx; ++i)
        if (hlsl_swizzle_get_component(swizzle->swizzle, i) != i)
            return false;

    hlsl_replace_node(instr, swizzle->val.node);

    return true;
}

static bool remove_trivial_conditional_branches(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_constant *condition;
    struct hlsl_ir_if *iff;

    if (instr->type != HLSL_IR_IF)
        return false;
    iff = hlsl_ir_if(instr);
    if (iff->condition.node->type != HLSL_IR_CONSTANT)
        return false;
    condition = hlsl_ir_constant(iff->condition.node);

    list_move_before(&instr->entry, condition->value.u[0].u ? &iff->then_block.instrs : &iff->else_block.instrs);
    list_remove(&instr->entry);
    hlsl_free_instr(instr);

    return true;
}

static bool normalize_switch_cases(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_switch_case *c, *def = NULL;
    bool missing_terminal_break = false;
    struct hlsl_ir_node *node;
    struct hlsl_ir_jump *jump;
    struct hlsl_ir_switch *s;

    if (instr->type != HLSL_IR_SWITCH)
        return false;
    s = hlsl_ir_switch(instr);

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        bool terminal_break = false;

        if (list_empty(&c->body.instrs))
        {
            terminal_break = !!list_next(&s->cases, &c->entry);
        }
        else
        {
            node = LIST_ENTRY(list_tail(&c->body.instrs), struct hlsl_ir_node, entry);
            if (node->type == HLSL_IR_JUMP)
            {
                jump = hlsl_ir_jump(node);
                terminal_break = jump->type == HLSL_IR_JUMP_BREAK;
            }
        }

        missing_terminal_break |= !terminal_break;

        if (!terminal_break)
        {
            if (c->is_default)
            {
                hlsl_error(ctx, &c->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                        "The 'default' case block is not terminated with 'break' or 'return'.");
            }
            else
            {
                hlsl_error(ctx, &c->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                        "Switch case block '%u' is not terminated with 'break' or 'return'.", c->value);
            }
        }
    }

    if (missing_terminal_break)
        return true;

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        if (c->is_default)
        {
            def = c;

            /* Remove preceding empty cases. */
            while (list_prev(&s->cases, &def->entry))
            {
                c = LIST_ENTRY(list_prev(&s->cases, &def->entry), struct hlsl_ir_switch_case, entry);
                if (!list_empty(&c->body.instrs))
                    break;
                hlsl_free_ir_switch_case(c);
            }

            if (list_empty(&def->body.instrs))
            {
                /* Remove following empty cases. */
                while (list_next(&s->cases, &def->entry))
                {
                    c = LIST_ENTRY(list_next(&s->cases, &def->entry), struct hlsl_ir_switch_case, entry);
                    if (!list_empty(&c->body.instrs))
                        break;
                    hlsl_free_ir_switch_case(c);
                }

                /* Merge with the next case. */
                if (list_next(&s->cases, &def->entry))
                {
                    c = LIST_ENTRY(list_next(&s->cases, &def->entry), struct hlsl_ir_switch_case, entry);
                    c->is_default = true;
                    hlsl_free_ir_switch_case(def);
                    def = c;
                }
            }

            break;
        }
    }

    if (def)
    {
        list_remove(&def->entry);
    }
    else
    {
        struct hlsl_ir_node *jump;

        if (!(def = hlsl_new_switch_case(ctx, 0, true, NULL, &s->node.loc)))
            return true;
        if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_BREAK, NULL, &s->node.loc)))
        {
            hlsl_free_ir_switch_case(def);
            return true;
        }
        hlsl_block_add_instr(&def->body, jump);
    }
    list_add_tail(&s->cases, &def->entry);

    return true;
}

static bool lower_nonconstant_vector_derefs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *idx;
    struct hlsl_deref *deref;
    struct hlsl_type *type;
    unsigned int i;

    if (instr->type != HLSL_IR_LOAD)
        return false;

    deref = &hlsl_ir_load(instr)->src;
    assert(deref->var);

    if (deref->path_len == 0)
        return false;

    type = deref->var->data_type;
    for (i = 0; i < deref->path_len - 1; ++i)
        type = hlsl_get_element_type_from_path_index(ctx, type, deref->path[i].node);

    idx = deref->path[deref->path_len - 1].node;

    if (type->class == HLSL_CLASS_VECTOR && idx->type != HLSL_IR_CONSTANT)
    {
        struct hlsl_ir_node *eq, *swizzle, *dot, *c, *operands[HLSL_MAX_OPERANDS] = {0};
        struct hlsl_constant_value value;
        struct hlsl_ir_load *vector_load;
        enum hlsl_ir_expr_op op;

        if (!(vector_load = hlsl_new_load_parent(ctx, deref, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, &vector_load->node);

        if (!(swizzle = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, X, X, X), type->dimx, idx, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, swizzle);

        value.u[0].u = 0;
        value.u[1].u = 1;
        value.u[2].u = 2;
        value.u[3].u = 3;
        if (!(c = hlsl_new_constant(ctx, hlsl_get_vector_type(ctx, HLSL_TYPE_UINT, type->dimx), &value, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, c);

        operands[0] = swizzle;
        operands[1] = c;
        if (!(eq = hlsl_new_expr(ctx, HLSL_OP2_EQUAL, operands,
                hlsl_get_vector_type(ctx, HLSL_TYPE_BOOL, type->dimx), &instr->loc)))
            return false;
        hlsl_block_add_instr(block, eq);

        if (!(eq = hlsl_new_cast(ctx, eq, type, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, eq);

        op = HLSL_OP2_DOT;
        if (type->dimx == 1)
            op = type->base_type == HLSL_TYPE_BOOL ? HLSL_OP2_LOGIC_AND : HLSL_OP2_MUL;

        /* Note: We may be creating a DOT for bool vectors here, which we need to lower to
         * LOGIC_OR + LOGIC_AND. */
        operands[0] = &vector_load->node;
        operands[1] = eq;
        if (!(dot = hlsl_new_expr(ctx, op, operands, instr->data_type, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, dot);

        return true;
    }

    return false;
}

/* Lower combined samples and sampler variables to synthesized separated textures and samplers.
 * That is, translate SM1-style samples in the source to SM4-style samples in the bytecode. */
static bool lower_combined_samples(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_resource_load *load;
    struct vkd3d_string_buffer *name;
    struct hlsl_ir_var *var;
    unsigned int i;

    if (instr->type != HLSL_IR_RESOURCE_LOAD)
        return false;
    load = hlsl_ir_resource_load(instr);

    switch (load->load_type)
    {
        case HLSL_RESOURCE_LOAD:
        case HLSL_RESOURCE_GATHER_RED:
        case HLSL_RESOURCE_GATHER_GREEN:
        case HLSL_RESOURCE_GATHER_BLUE:
        case HLSL_RESOURCE_GATHER_ALPHA:
        case HLSL_RESOURCE_RESINFO:
        case HLSL_RESOURCE_SAMPLE_CMP:
        case HLSL_RESOURCE_SAMPLE_CMP_LZ:
        case HLSL_RESOURCE_SAMPLE_GRAD:
        case HLSL_RESOURCE_SAMPLE_INFO:
            return false;

        case HLSL_RESOURCE_SAMPLE:
        case HLSL_RESOURCE_SAMPLE_LOD:
        case HLSL_RESOURCE_SAMPLE_LOD_BIAS:
        case HLSL_RESOURCE_SAMPLE_PROJ:
            break;
    }
    if (load->sampler.var)
        return false;

    if (!hlsl_type_is_resource(load->resource.var->data_type))
    {
        hlsl_fixme(ctx, &instr->loc, "Lower combined samplers within structs.");
        return false;
    }

    assert(hlsl_deref_get_regset(ctx, &load->resource) == HLSL_REGSET_SAMPLERS);

    if (!(name = hlsl_get_string_buffer(ctx)))
        return false;
    vkd3d_string_buffer_printf(name, "<resource>%s", load->resource.var->name);

    TRACE("Lowering to separate resource %s.\n", debugstr_a(name->buffer));

    if (!(var = hlsl_get_var(ctx->globals, name->buffer)))
    {
        struct hlsl_type *texture_array_type = hlsl_new_texture_type(ctx, load->sampling_dim,
                hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4), 0);

        /* Create (possibly multi-dimensional) texture array type with the same dims as the sampler array. */
        struct hlsl_type *arr_type = load->resource.var->data_type;
        for (i = 0; i < load->resource.path_len; ++i)
        {
            assert(arr_type->class == HLSL_CLASS_ARRAY);
            texture_array_type = hlsl_new_array_type(ctx, texture_array_type, arr_type->e.array.elements_count);
            arr_type = arr_type->e.array.type;
        }

        if (!(var = hlsl_new_synthetic_var_named(ctx, name->buffer, texture_array_type, &instr->loc, false)))
        {
            hlsl_release_string_buffer(ctx, name);
            return false;
        }
        var->is_uniform = 1;
        var->is_separated_resource = true;

        list_add_tail(&ctx->extern_vars, &var->extern_entry);
    }
    hlsl_release_string_buffer(ctx, name);

    if (load->sampling_dim != var->data_type->sampler_dim)
    {
        hlsl_error(ctx, &load->node.loc, VKD3D_SHADER_ERROR_HLSL_INCONSISTENT_SAMPLER,
                "Cannot split combined samplers from \"%s\" if they have different usage dimensions.",
                load->resource.var->name);
        hlsl_note(ctx, &var->loc, VKD3D_SHADER_LOG_ERROR, "First use as combined sampler is here.");
        return false;

    }

    hlsl_copy_deref(ctx, &load->sampler, &load->resource);
    load->resource.var = var;
    assert(hlsl_deref_get_type(ctx, &load->resource)->base_type == HLSL_TYPE_TEXTURE);
    assert(hlsl_deref_get_type(ctx, &load->sampler)->base_type == HLSL_TYPE_SAMPLER);

    return true;
}

static void insert_ensuring_decreasing_bind_count(struct list *list, struct hlsl_ir_var *to_add,
        enum hlsl_regset regset)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, list, struct hlsl_ir_var, extern_entry)
    {
        if (var->bind_count[regset] < to_add->bind_count[regset])
        {
            list_add_before(&var->extern_entry, &to_add->extern_entry);
            return;
        }
    }

    list_add_tail(list, &to_add->extern_entry);
}

static bool sort_synthetic_separated_samplers_first(struct hlsl_ctx *ctx)
{
    struct list separated_resources;
    struct hlsl_ir_var *var, *next;

    list_init(&separated_resources);

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_separated_resource)
        {
            list_remove(&var->extern_entry);
            insert_ensuring_decreasing_bind_count(&separated_resources, var, HLSL_REGSET_TEXTURES);
        }
    }

    list_move_head(&ctx->extern_vars, &separated_resources);

    return false;
}

/* Lower DIV to RCP + MUL. */
static bool lower_division(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *rcp, *mul;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP2_DIV)
        return false;

    if (!(rcp = hlsl_new_unary_expr(ctx, HLSL_OP1_RCP, expr->operands[1].node, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, rcp);

    if (!(mul = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, expr->operands[0].node, rcp)))
        return false;
    hlsl_block_add_instr(block, mul);

    return true;
}

/* Lower SQRT to RSQ + RCP. */
static bool lower_sqrt(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *rsq, *rcp;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_SQRT)
        return false;

    if (!(rsq = hlsl_new_unary_expr(ctx, HLSL_OP1_RSQ, expr->operands[0].node, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, rsq);

    if (!(rcp = hlsl_new_unary_expr(ctx, HLSL_OP1_RCP, rsq, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, rcp);
    return true;
}

/* Lower DP2 to MUL + ADD */
static bool lower_dot(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *mul, *replacement, *zero, *add_x, *add_y;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_DOT)
        return false;
    if (arg1->data_type->dimx != 2)
        return false;

    if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 };

        if (!(zero = hlsl_new_float_constant(ctx, 0.0f, &expr->node.loc)))
            return false;
        hlsl_block_add_instr(block, zero);

        operands[0] = arg1;
        operands[1] = arg2;
        operands[2] = zero;

        if (!(replacement = hlsl_new_expr(ctx, HLSL_OP3_DP2ADD, operands, instr->data_type, &expr->node.loc)))
            return false;
    }
    else
    {
        if (!(mul = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, expr->operands[0].node, expr->operands[1].node)))
            return false;
        hlsl_block_add_instr(block, mul);

        if (!(add_x = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, X, X, X), instr->data_type->dimx, mul, &expr->node.loc)))
            return false;
        hlsl_block_add_instr(block, add_x);

        if (!(add_y = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Y, Y, Y, Y), instr->data_type->dimx, mul, &expr->node.loc)))
            return false;
        hlsl_block_add_instr(block, add_y);

        if (!(replacement = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, add_x, add_y)))
            return false;
    }
    hlsl_block_add_instr(block, replacement);

    return true;
}

/* Lower ABS to MAX */
static bool lower_abs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *replacement;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_ABS)
        return false;

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(replacement = hlsl_new_binary_expr(ctx, HLSL_OP2_MAX, neg, arg)))
        return false;
    hlsl_block_add_instr(block, replacement);

    return true;
}

/* Lower ROUND using FRC, ROUND(x) -> ((x + 0.5) - FRC(x + 0.5)). */
static bool lower_round(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *sum, *frc, *half, *replacement;
    struct hlsl_type *type = instr->data_type;
    struct hlsl_constant_value half_value;
    unsigned int i, component_count;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_ROUND)
        return false;

    component_count = hlsl_type_component_count(type);
    for (i = 0; i < component_count; ++i)
        half_value.u[i].f = 0.5f;
    if (!(half = hlsl_new_constant(ctx, type, &half_value, &expr->node.loc)))
        return false;
    hlsl_block_add_instr(block, half);

    if (!(sum = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, arg, half)))
        return false;
    hlsl_block_add_instr(block, sum);

    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, sum, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, frc, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(replacement = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, sum, neg)))
        return false;
    hlsl_block_add_instr(block, replacement);

    return true;
}

/* Lower CEIL to FRC */
static bool lower_ceil(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *sum, *frc;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_CEIL)
        return false;

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, neg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);

    if (!(sum = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, frc, arg)))
        return false;
    hlsl_block_add_instr(block, sum);

    return true;
}

/* Lower FLOOR to FRC */
static bool lower_floor(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *sum, *frc;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_FLOOR)
        return false;

    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, frc, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(sum = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, neg, arg)))
        return false;
    hlsl_block_add_instr(block, sum);

    return true;
}

/* Use 'movc' for the ternary operator. */
static bool lower_ternary(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 }, *replacement;
    struct hlsl_ir_node *zero, *cond, *first, *second;
    struct hlsl_constant_value zero_value = { 0 };
    struct hlsl_ir_expr *expr;
    struct hlsl_type *type;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP3_TERNARY)
        return false;

    cond = expr->operands[0].node;
    first = expr->operands[1].node;
    second = expr->operands[2].node;

    if (ctx->profile->major_version < 4 && ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        struct hlsl_ir_node *abs, *neg;

        if (!(abs = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, cond, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, abs);

        if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, abs, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, neg);

        operands[0] = neg;
        operands[1] = second;
        operands[2] = first;
        if (!(replacement = hlsl_new_expr(ctx, HLSL_OP3_CMP, operands, first->data_type, &instr->loc)))
            return false;
    }
    else if (ctx->profile->major_version < 4 && ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX)
    {
        hlsl_fixme(ctx, &instr->loc, "Ternary operator is not implemented for %s profile.", ctx->profile->name);
        return false;
    }
    else
    {
        if (cond->data_type->base_type == HLSL_TYPE_FLOAT)
        {
            if (!(zero = hlsl_new_constant(ctx, cond->data_type, &zero_value, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, zero);

            operands[0] = zero;
            operands[1] = cond;
            type = cond->data_type;
            type = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_BOOL, type->dimx, type->dimy);
            if (!(cond = hlsl_new_expr(ctx, HLSL_OP2_NEQUAL, operands, type, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, cond);
        }

        memset(operands, 0, sizeof(operands));
        operands[0] = cond;
        operands[1] = first;
        operands[2] = second;
        if (!(replacement = hlsl_new_expr(ctx, HLSL_OP3_MOVC, operands, first->data_type, &instr->loc)))
            return false;
    }

    hlsl_block_add_instr(block, replacement);
    return true;
}

static bool lower_casts_to_bool(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_type *type = instr->data_type, *arg_type;
    static const struct hlsl_constant_value zero_value;
    struct hlsl_ir_node *zero, *neq;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_CAST)
        return false;
    arg_type = expr->operands[0].node->data_type;
    if (type->class > HLSL_CLASS_VECTOR || arg_type->class > HLSL_CLASS_VECTOR)
        return false;
    if (type->base_type != HLSL_TYPE_BOOL)
        return false;

    /* Narrowing casts should have already been lowered. */
    assert(type->dimx == arg_type->dimx);

    zero = hlsl_new_constant(ctx, arg_type, &zero_value, &instr->loc);
    if (!zero)
        return false;
    hlsl_block_add_instr(block, zero);

    if (!(neq = hlsl_new_binary_expr(ctx, HLSL_OP2_NEQUAL, expr->operands[0].node, zero)))
        return false;
    neq->data_type = expr->node.data_type;
    hlsl_block_add_instr(block, neq);

    return true;
}

struct hlsl_ir_node *hlsl_add_conditional(struct hlsl_ctx *ctx, struct hlsl_block *instrs,
        struct hlsl_ir_node *condition, struct hlsl_ir_node *if_true, struct hlsl_ir_node *if_false)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS];
    struct hlsl_ir_node *cond;

    assert(hlsl_types_are_equal(if_true->data_type, if_false->data_type));

    operands[0] = condition;
    operands[1] = if_true;
    operands[2] = if_false;
    if (!(cond = hlsl_new_expr(ctx, HLSL_OP3_TERNARY, operands, if_true->data_type, &condition->loc)))
        return false;
    hlsl_block_add_instr(instrs, cond);

    return cond;
}

static bool lower_int_division(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *xor, *and, *abs1, *abs2, *div, *neg, *cast1, *cast2, *cast3, *high_bit;
    struct hlsl_type *type = instr->data_type, *utype;
    struct hlsl_constant_value high_bit_value;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_DIV)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->base_type != HLSL_TYPE_INT)
        return false;
    utype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_UINT, type->dimx, type->dimy);

    if (!(xor = hlsl_new_binary_expr(ctx, HLSL_OP2_BIT_XOR, arg1, arg2)))
        return false;
    hlsl_block_add_instr(block, xor);

    for (i = 0; i < type->dimx; ++i)
        high_bit_value.u[i].u = 0x80000000;
    if (!(high_bit = hlsl_new_constant(ctx, type, &high_bit_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, high_bit);

    if (!(and = hlsl_new_binary_expr(ctx, HLSL_OP2_BIT_AND, xor, high_bit)))
        return false;
    hlsl_block_add_instr(block, and);

    if (!(abs1 = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, arg1, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, abs1);

    if (!(cast1 = hlsl_new_cast(ctx, abs1, utype, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast1);

    if (!(abs2 = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, arg2, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, abs2);

    if (!(cast2 = hlsl_new_cast(ctx, abs2, utype, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast2);

    if (!(div = hlsl_new_binary_expr(ctx, HLSL_OP2_DIV, cast1, cast2)))
        return false;
    hlsl_block_add_instr(block, div);

    if (!(cast3 = hlsl_new_cast(ctx, div, type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast3);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, cast3, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    return hlsl_add_conditional(ctx, block, and, neg, cast3);
}

static bool lower_int_modulus(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *and, *abs1, *abs2, *div, *neg, *cast1, *cast2, *cast3, *high_bit;
    struct hlsl_type *type = instr->data_type, *utype;
    struct hlsl_constant_value high_bit_value;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_MOD)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->base_type != HLSL_TYPE_INT)
        return false;
    utype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_UINT, type->dimx, type->dimy);

    for (i = 0; i < type->dimx; ++i)
        high_bit_value.u[i].u = 0x80000000;
    if (!(high_bit = hlsl_new_constant(ctx, type, &high_bit_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, high_bit);

    if (!(and = hlsl_new_binary_expr(ctx, HLSL_OP2_BIT_AND, arg1, high_bit)))
        return false;
    hlsl_block_add_instr(block, and);

    if (!(abs1 = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, arg1, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, abs1);

    if (!(cast1 = hlsl_new_cast(ctx, abs1, utype, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast1);

    if (!(abs2 = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, arg2, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, abs2);

    if (!(cast2 = hlsl_new_cast(ctx, abs2, utype, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast2);

    if (!(div = hlsl_new_binary_expr(ctx, HLSL_OP2_MOD, cast1, cast2)))
        return false;
    hlsl_block_add_instr(block, div);

    if (!(cast3 = hlsl_new_cast(ctx, div, type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast3);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, cast3, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    return hlsl_add_conditional(ctx, block, and, neg, cast3);
}

static bool lower_int_abs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_type *type = instr->data_type;
    struct hlsl_ir_node *arg, *neg, *max;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);

    if (expr->op != HLSL_OP1_ABS)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->base_type != HLSL_TYPE_INT)
        return false;

    arg = expr->operands[0].node;

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(max = hlsl_new_binary_expr(ctx, HLSL_OP2_MAX, arg, neg)))
        return false;
    hlsl_block_add_instr(block, max);

    return true;
}

static bool lower_int_dot(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *mult, *comps[4] = {0}, *res;
    struct hlsl_type *type = instr->data_type;
    struct hlsl_ir_expr *expr;
    unsigned int i, dimx;
    bool is_bool;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);

    if (expr->op != HLSL_OP2_DOT)
        return false;

    if (type->base_type == HLSL_TYPE_INT || type->base_type == HLSL_TYPE_UINT
            || type->base_type == HLSL_TYPE_BOOL)
    {
        arg1 = expr->operands[0].node;
        arg2 = expr->operands[1].node;
        assert(arg1->data_type->dimx == arg2->data_type->dimx);
        dimx = arg1->data_type->dimx;
        is_bool = type->base_type == HLSL_TYPE_BOOL;

        if (!(mult = hlsl_new_binary_expr(ctx, is_bool ? HLSL_OP2_LOGIC_AND : HLSL_OP2_MUL, arg1, arg2)))
            return false;
        hlsl_block_add_instr(block, mult);

        for (i = 0; i < dimx; ++i)
        {
            unsigned int s = hlsl_swizzle_from_writemask(1 << i);

            if (!(comps[i] = hlsl_new_swizzle(ctx, s, 1, mult, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, comps[i]);
        }

        res = comps[0];
        for (i = 1; i < dimx; ++i)
        {
            if (!(res = hlsl_new_binary_expr(ctx, is_bool ? HLSL_OP2_LOGIC_OR : HLSL_OP2_ADD, res, comps[i])))
                return false;
            hlsl_block_add_instr(block, res);
        }

        return true;
    }

    return false;
}

static bool lower_float_modulus(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *mul1, *neg1, *ge, *neg2, *div, *mul2, *frc, *cond, *one, *mul3;
    struct hlsl_type *type = instr->data_type, *btype;
    struct hlsl_constant_value one_value;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_MOD)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->base_type != HLSL_TYPE_FLOAT)
        return false;
    btype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_BOOL, type->dimx, type->dimy);

    if (!(mul1 = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, arg2, arg1)))
        return false;
    hlsl_block_add_instr(block, mul1);

    if (!(neg1 = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, mul1, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg1);

    if (!(ge = hlsl_new_binary_expr(ctx, HLSL_OP2_GEQUAL, mul1, neg1)))
        return false;
    ge->data_type = btype;
    hlsl_block_add_instr(block, ge);

    if (!(neg2 = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg2, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg2);

    if (!(cond = hlsl_add_conditional(ctx, block, ge, arg2, neg2)))
        return false;

    for (i = 0; i < type->dimx; ++i)
        one_value.u[i].f = 1.0f;
    if (!(one = hlsl_new_constant(ctx, type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    if (!(div = hlsl_new_binary_expr(ctx, HLSL_OP2_DIV, one, cond)))
        return false;
    hlsl_block_add_instr(block, div);

    if (!(mul2 = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, div, arg1)))
        return false;
    hlsl_block_add_instr(block, mul2);

    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, mul2, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);

    if (!(mul3 = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, frc, cond)))
        return false;
    hlsl_block_add_instr(block, mul3);

    return true;
}

static bool lower_discard_neg(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_node *zero, *bool_false, *or, *cmp, *load;
    static const struct hlsl_constant_value zero_value;
    struct hlsl_type *arg_type, *cmp_type;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 };
    struct hlsl_ir_jump *jump;
    struct hlsl_block block;
    unsigned int i, count;

    if (instr->type != HLSL_IR_JUMP)
        return false;
    jump = hlsl_ir_jump(instr);
    if (jump->type != HLSL_IR_JUMP_DISCARD_NEG)
        return false;

    hlsl_block_init(&block);

    arg_type = jump->condition.node->data_type;
    if (!(zero = hlsl_new_constant(ctx, arg_type, &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, zero);

    operands[0] = jump->condition.node;
    operands[1] = zero;
    cmp_type = hlsl_get_numeric_type(ctx, arg_type->class, HLSL_TYPE_BOOL, arg_type->dimx, arg_type->dimy);
    if (!(cmp = hlsl_new_expr(ctx, HLSL_OP2_LESS, operands, cmp_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, cmp);

    if (!(bool_false = hlsl_new_constant(ctx, hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL), &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, bool_false);

    or = bool_false;

    count = hlsl_type_component_count(cmp_type);
    for (i = 0; i < count; ++i)
    {
        if (!(load = hlsl_add_load_component(ctx, &block, cmp, i, &instr->loc)))
            return false;

        if (!(or = hlsl_new_binary_expr(ctx, HLSL_OP2_LOGIC_OR, or, load)))
                return NULL;
        hlsl_block_add_instr(&block, or);
    }

    list_move_tail(&instr->entry, &block.instrs);
    hlsl_src_remove(&jump->condition);
    hlsl_src_from_node(&jump->condition, or);
    jump->type = HLSL_IR_JUMP_DISCARD_NZ;

    return true;
}

static bool dce(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    switch (instr->type)
    {
        case HLSL_IR_CONSTANT:
        case HLSL_IR_EXPR:
        case HLSL_IR_INDEX:
        case HLSL_IR_LOAD:
        case HLSL_IR_RESOURCE_LOAD:
        case HLSL_IR_SWIZZLE:
            if (list_empty(&instr->uses))
            {
                list_remove(&instr->entry);
                hlsl_free_instr(instr);
                return true;
            }
            break;

        case HLSL_IR_STORE:
        {
            struct hlsl_ir_store *store = hlsl_ir_store(instr);
            struct hlsl_ir_var *var = store->lhs.var;

            if (var->last_read < instr->index)
            {
                list_remove(&instr->entry);
                hlsl_free_instr(instr);
                return true;
            }
            break;
        }

        case HLSL_IR_CALL:
        case HLSL_IR_IF:
        case HLSL_IR_JUMP:
        case HLSL_IR_LOOP:
        case HLSL_IR_RESOURCE_STORE:
        case HLSL_IR_SWITCH:
            break;
    }

    return false;
}

static void dump_function(struct rb_entry *entry, void *context)
{
    struct hlsl_ir_function *func = RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);
    struct hlsl_ir_function_decl *decl;
    struct hlsl_ctx *ctx = context;

    LIST_FOR_EACH_ENTRY(decl, &func->overloads, struct hlsl_ir_function_decl, entry)
    {
        if (decl->has_body)
            hlsl_dump_function(ctx, decl);
    }
}

static bool mark_indexable_vars(struct hlsl_ctx *ctx, struct hlsl_deref *deref,
        struct hlsl_ir_node *instr)
{
    if (!deref->rel_offset.node)
        return false;

    assert(deref->var);
    assert(deref->rel_offset.node->type != HLSL_IR_CONSTANT);
    deref->var->indexable = true;

    return true;
}

static char get_regset_name(enum hlsl_regset regset)
{
    switch (regset)
    {
        case HLSL_REGSET_SAMPLERS:
            return 's';
        case HLSL_REGSET_TEXTURES:
            return 't';
        case HLSL_REGSET_UAVS:
            return 'u';
        case HLSL_REGSET_NUMERIC:
            vkd3d_unreachable();
    }
    vkd3d_unreachable();
}

static void allocate_register_reservations(struct hlsl_ctx *ctx)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int r;

        if (!hlsl_type_is_resource(var->data_type))
            continue;

        if (var->reg_reservation.reg_type)
        {
            for (r = 0; r <= HLSL_REGSET_LAST_OBJECT; ++r)
            {
                if (var->regs[r].allocation_size > 0)
                {
                    if (var->reg_reservation.reg_type != get_regset_name(r))
                    {
                        struct vkd3d_string_buffer *type_string;

                        /* We can throw this error because resources can only span across a single
                         * regset, but we have to check for multiple regsets if we support register
                         * reservations for structs for SM5. */
                        type_string = hlsl_type_to_string(ctx, var->data_type);
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                                "Object of type '%s' must be bound to register type '%c'.",
                                type_string->buffer, get_regset_name(r));
                        hlsl_release_string_buffer(ctx, type_string);
                    }
                    else
                    {
                        var->regs[r].allocated = true;
                        var->regs[r].id = var->reg_reservation.reg_index;
                        TRACE("Allocated reserved %s to %c%u-%c%u.\n", var->name, var->reg_reservation.reg_type,
                                var->reg_reservation.reg_index, var->reg_reservation.reg_type,
                                var->reg_reservation.reg_index + var->regs[r].allocation_size);
                    }
                }
            }
        }
    }
}

/* Compute the earliest and latest liveness for each variable. In the case that
 * a variable is accessed inside of a loop, we promote its liveness to extend
 * to at least the range of the entire loop. We also do this for nodes, so that
 * nodes produced before the loop have their temp register protected from being
 * overridden after the last read within an iteration. */
static void compute_liveness_recurse(struct hlsl_block *block, unsigned int loop_first, unsigned int loop_last)
{
    struct hlsl_ir_node *instr;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        const unsigned int last_read = loop_last ? max(instr->index, loop_last) : instr->index;

        switch (instr->type)
        {
        case HLSL_IR_CALL:
            /* We should have inlined all calls before computing liveness. */
            vkd3d_unreachable();

        case HLSL_IR_STORE:
        {
            struct hlsl_ir_store *store = hlsl_ir_store(instr);

            var = store->lhs.var;
            if (!var->first_write)
                var->first_write = loop_first ? min(instr->index, loop_first) : instr->index;
            store->rhs.node->last_read = last_read;
            if (store->lhs.rel_offset.node)
                store->lhs.rel_offset.node->last_read = last_read;
            break;
        }
        case HLSL_IR_EXPR:
        {
            struct hlsl_ir_expr *expr = hlsl_ir_expr(instr);
            unsigned int i;

            for (i = 0; i < ARRAY_SIZE(expr->operands) && expr->operands[i].node; ++i)
                expr->operands[i].node->last_read = last_read;
            break;
        }
        case HLSL_IR_IF:
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            compute_liveness_recurse(&iff->then_block, loop_first, loop_last);
            compute_liveness_recurse(&iff->else_block, loop_first, loop_last);
            iff->condition.node->last_read = last_read;
            break;
        }
        case HLSL_IR_LOAD:
        {
            struct hlsl_ir_load *load = hlsl_ir_load(instr);

            var = load->src.var;
            var->last_read = max(var->last_read, last_read);
            if (load->src.rel_offset.node)
                load->src.rel_offset.node->last_read = last_read;
            break;
        }
        case HLSL_IR_LOOP:
        {
            struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);

            compute_liveness_recurse(&loop->body, loop_first ? loop_first : instr->index,
                    loop_last ? loop_last : loop->next_index);
            break;
        }
        case HLSL_IR_RESOURCE_LOAD:
        {
            struct hlsl_ir_resource_load *load = hlsl_ir_resource_load(instr);

            var = load->resource.var;
            var->last_read = max(var->last_read, last_read);
            if (load->resource.rel_offset.node)
                load->resource.rel_offset.node->last_read = last_read;

            if ((var = load->sampler.var))
            {
                var->last_read = max(var->last_read, last_read);
                if (load->sampler.rel_offset.node)
                    load->sampler.rel_offset.node->last_read = last_read;
            }

            if (load->coords.node)
                load->coords.node->last_read = last_read;
            if (load->texel_offset.node)
                load->texel_offset.node->last_read = last_read;
            if (load->lod.node)
                load->lod.node->last_read = last_read;
            if (load->ddx.node)
                load->ddx.node->last_read = last_read;
            if (load->ddy.node)
                load->ddy.node->last_read = last_read;
            if (load->sample_index.node)
                load->sample_index.node->last_read = last_read;
            if (load->cmp.node)
                load->cmp.node->last_read = last_read;
            break;
        }
        case HLSL_IR_RESOURCE_STORE:
        {
            struct hlsl_ir_resource_store *store = hlsl_ir_resource_store(instr);

            var = store->resource.var;
            var->last_read = max(var->last_read, last_read);
            if (store->resource.rel_offset.node)
                store->resource.rel_offset.node->last_read = last_read;
            store->coords.node->last_read = last_read;
            store->value.node->last_read = last_read;
            break;
        }
        case HLSL_IR_SWIZZLE:
        {
            struct hlsl_ir_swizzle *swizzle = hlsl_ir_swizzle(instr);

            swizzle->val.node->last_read = last_read;
            break;
        }
        case HLSL_IR_INDEX:
        {
            struct hlsl_ir_index *index = hlsl_ir_index(instr);

            index->val.node->last_read = last_read;
            index->idx.node->last_read = last_read;
            break;
        }
        case HLSL_IR_JUMP:
        {
            struct hlsl_ir_jump *jump = hlsl_ir_jump(instr);

            if (jump->condition.node)
                jump->condition.node->last_read = last_read;
            break;
        }
        case HLSL_IR_SWITCH:
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                compute_liveness_recurse(&c->body, loop_first, loop_last);
            s->selector.node->last_read = last_read;
            break;
        }
        case HLSL_IR_CONSTANT:
            break;
        }
    }
}

static void compute_liveness(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    index_instructions(&entry_func->body, 2);

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
            var->first_write = var->last_read = 0;
    }

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_uniform || var->is_input_semantic)
            var->first_write = 1;
        else if (var->is_output_semantic)
            var->last_read = UINT_MAX;
    }

    compute_liveness_recurse(&entry_func->body, 0, 0);
}

struct register_allocator
{
    struct allocation
    {
        uint32_t reg;
        unsigned int writemask;
        unsigned int first_write, last_read;
    } *allocations;
    size_t count, capacity;

    /* Indexable temps are allocated separately and always keep their index regardless of their
     * lifetime. */
    size_t indexable_count;

    /* Total number of registers allocated so far. Used to declare sm4 temp count. */
    uint32_t reg_count;
};

static unsigned int get_available_writemask(const struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, uint32_t reg_idx)
{
    unsigned int writemask = VKD3DSP_WRITEMASK_ALL;
    size_t i;

    for (i = 0; i < allocator->count; ++i)
    {
        const struct allocation *allocation = &allocator->allocations[i];

        /* We do not overlap if first write == last read:
         * this is the case where we are allocating the result of that
         * expression, e.g. "add r0, r0, r1". */

        if (allocation->reg == reg_idx
                && first_write < allocation->last_read && last_read > allocation->first_write)
            writemask &= ~allocation->writemask;

        if (!writemask)
            break;
    }

    return writemask;
}

static void record_allocation(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        uint32_t reg_idx, unsigned int writemask, unsigned int first_write, unsigned int last_read)
{
    struct allocation *allocation;

    if (!hlsl_array_reserve(ctx, (void **)&allocator->allocations, &allocator->capacity,
            allocator->count + 1, sizeof(*allocator->allocations)))
        return;

    allocation = &allocator->allocations[allocator->count++];
    allocation->reg = reg_idx;
    allocation->writemask = writemask;
    allocation->first_write = first_write;
    allocation->last_read = last_read;

    allocator->reg_count = max(allocator->reg_count, reg_idx + 1);
}

/* reg_size is the number of register components to be reserved, while component_count is the number
 * of components for the register's writemask. In SM1, floats and vectors allocate the whole
 * register, even if they don't use it completely. */
static struct hlsl_reg allocate_register(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, unsigned int reg_size,
        unsigned int component_count)
{
    struct hlsl_reg ret = {0};
    unsigned int writemask;
    uint32_t reg_idx;

    assert(component_count <= reg_size);

    for (reg_idx = 0;; ++reg_idx)
    {
        writemask = get_available_writemask(allocator, first_write, last_read, reg_idx);

        if (vkd3d_popcount(writemask) >= reg_size)
        {
            writemask = hlsl_combine_writemasks(writemask, (1u << reg_size) - 1);
            break;
        }
    }

    record_allocation(ctx, allocator, reg_idx, writemask, first_write, last_read);

    ret.id = reg_idx;
    ret.allocation_size = 1;
    ret.writemask = hlsl_combine_writemasks(writemask, (1u << component_count) - 1);
    ret.allocated = true;
    return ret;
}

static bool is_range_available(const struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, uint32_t reg_idx, unsigned int reg_size)
{
    unsigned int last_reg_mask = (1u << (reg_size % 4)) - 1;
    unsigned int writemask;
    uint32_t i;

    for (i = 0; i < (reg_size / 4); ++i)
    {
        writemask = get_available_writemask(allocator, first_write, last_read, reg_idx + i);
        if (writemask != VKD3DSP_WRITEMASK_ALL)
            return false;
    }
    writemask = get_available_writemask(allocator, first_write, last_read, reg_idx + (reg_size / 4));
    if ((writemask & last_reg_mask) != last_reg_mask)
        return false;
    return true;
}

static struct hlsl_reg allocate_range(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, unsigned int reg_size)
{
    struct hlsl_reg ret = {0};
    uint32_t reg_idx;
    unsigned int i;

    for (reg_idx = 0;; ++reg_idx)
    {
        if (is_range_available(allocator, first_write, last_read, reg_idx, reg_size))
            break;
    }

    for (i = 0; i < reg_size / 4; ++i)
        record_allocation(ctx, allocator, reg_idx + i, VKD3DSP_WRITEMASK_ALL, first_write, last_read);
    if (reg_size % 4)
        record_allocation(ctx, allocator, reg_idx + (reg_size / 4), (1u << (reg_size % 4)) - 1, first_write, last_read);

    ret.id = reg_idx;
    ret.allocation_size = align(reg_size, 4) / 4;
    ret.allocated = true;
    return ret;
}

static struct hlsl_reg allocate_numeric_registers_for_type(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, const struct hlsl_type *type)
{
    unsigned int reg_size = type->reg_size[HLSL_REGSET_NUMERIC];

    if (type->class <= HLSL_CLASS_VECTOR)
        return allocate_register(ctx, allocator, first_write, last_read, reg_size, type->dimx);
    else
        return allocate_range(ctx, allocator, first_write, last_read, reg_size);
}

static const char *debug_register(char class, struct hlsl_reg reg, const struct hlsl_type *type)
{
    static const char writemask_offset[] = {'w','x','y','z'};
    unsigned int reg_size = type->reg_size[HLSL_REGSET_NUMERIC];

    if (reg_size > 4)
    {
        if (reg_size & 3)
            return vkd3d_dbg_sprintf("%c%u-%c%u.%c", class, reg.id, class, reg.id + (reg_size / 4),
                    writemask_offset[reg_size & 3]);

        return vkd3d_dbg_sprintf("%c%u-%c%u", class, reg.id, class, reg.id + (reg_size / 4) - 1);
    }
    return vkd3d_dbg_sprintf("%c%u%s", class, reg.id, debug_hlsl_writemask(reg.writemask));
}

static bool track_object_components_sampler_dim(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_resource_load *load;
    struct hlsl_ir_var *var;
    enum hlsl_regset regset;
    unsigned int index;

    if (instr->type != HLSL_IR_RESOURCE_LOAD)
        return false;

    load = hlsl_ir_resource_load(instr);
    var = load->resource.var;

    regset = hlsl_deref_get_regset(ctx, &load->resource);
    if (!hlsl_regset_index_from_deref(ctx, &load->resource, regset, &index))
        return false;

    if (regset == HLSL_REGSET_SAMPLERS)
    {
        enum hlsl_sampler_dim dim;

        assert(!load->sampler.var);

        dim = var->objects_usage[regset][index].sampler_dim;
        if (dim != load->sampling_dim)
        {
            if (dim == HLSL_SAMPLER_DIM_GENERIC)
            {
                var->objects_usage[regset][index].first_sampler_dim_loc = instr->loc;
            }
            else
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INCONSISTENT_SAMPLER,
                        "Inconsistent generic sampler usage dimension.");
                hlsl_note(ctx, &var->objects_usage[regset][index].first_sampler_dim_loc,
                        VKD3D_SHADER_LOG_ERROR, "First use is here.");
                return false;
            }
        }
    }
    var->objects_usage[regset][index].sampler_dim = load->sampling_dim;

    return false;
}

static bool track_object_components_usage(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_resource_load *load;
    struct hlsl_ir_var *var;
    enum hlsl_regset regset;
    unsigned int index;

    if (instr->type != HLSL_IR_RESOURCE_LOAD)
        return false;

    load = hlsl_ir_resource_load(instr);
    var = load->resource.var;

    regset = hlsl_deref_get_regset(ctx, &load->resource);

    if (!hlsl_regset_index_from_deref(ctx, &load->resource, regset, &index))
        return false;

    var->objects_usage[regset][index].used = true;
    var->bind_count[regset] = max(var->bind_count[regset], index + 1);
    if (load->sampler.var)
    {
        var = load->sampler.var;
        if (!hlsl_regset_index_from_deref(ctx, &load->sampler, HLSL_REGSET_SAMPLERS, &index))
            return false;

        var->objects_usage[HLSL_REGSET_SAMPLERS][index].used = true;
        var->bind_count[HLSL_REGSET_SAMPLERS] = max(var->bind_count[HLSL_REGSET_SAMPLERS], index + 1);
    }

    return false;
}

static void calculate_resource_register_counts(struct hlsl_ctx *ctx)
{
    struct hlsl_ir_var *var;
    struct hlsl_type *type;
    unsigned int k;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        type = var->data_type;

        for (k = 0; k <= HLSL_REGSET_LAST_OBJECT; ++k)
        {
            bool is_separated = var->is_separated_resource;

            if (var->bind_count[k] > 0)
                var->regs[k].allocation_size = (k == HLSL_REGSET_SAMPLERS || is_separated) ? var->bind_count[k] : type->reg_size[k];
        }
    }
}

static void allocate_variable_temp_register(struct hlsl_ctx *ctx,
        struct hlsl_ir_var *var, struct register_allocator *allocator)
{
    if (var->is_input_semantic || var->is_output_semantic || var->is_uniform)
        return;

    if (!var->regs[HLSL_REGSET_NUMERIC].allocated && var->last_read)
    {
        if (var->indexable)
        {
            var->regs[HLSL_REGSET_NUMERIC].id = allocator->indexable_count++;
            var->regs[HLSL_REGSET_NUMERIC].allocation_size = 1;
            var->regs[HLSL_REGSET_NUMERIC].writemask = 0;
            var->regs[HLSL_REGSET_NUMERIC].allocated = true;

            TRACE("Allocated %s to x%u[].\n", var->name, var->regs[HLSL_REGSET_NUMERIC].id);
        }
        else
        {
            var->regs[HLSL_REGSET_NUMERIC] = allocate_numeric_registers_for_type(ctx, allocator,
                    var->first_write, var->last_read, var->data_type);

            TRACE("Allocated %s to %s (liveness %u-%u).\n", var->name, debug_register('r',
                    var->regs[HLSL_REGSET_NUMERIC], var->data_type), var->first_write, var->last_read);
        }
    }
}

static void allocate_temp_registers_recurse(struct hlsl_ctx *ctx,
        struct hlsl_block *block, struct register_allocator *allocator)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        /* In SM4 all constants are inlined. */
        if (ctx->profile->major_version >= 4 && instr->type == HLSL_IR_CONSTANT)
            continue;

        if (!instr->reg.allocated && instr->last_read)
        {
            instr->reg = allocate_numeric_registers_for_type(ctx, allocator, instr->index, instr->last_read,
                    instr->data_type);
            TRACE("Allocated anonymous expression @%u to %s (liveness %u-%u).\n", instr->index,
                    debug_register('r', instr->reg, instr->data_type), instr->index, instr->last_read);
        }

        switch (instr->type)
        {
            case HLSL_IR_IF:
            {
                struct hlsl_ir_if *iff = hlsl_ir_if(instr);
                allocate_temp_registers_recurse(ctx, &iff->then_block, allocator);
                allocate_temp_registers_recurse(ctx, &iff->else_block, allocator);
                break;
            }

            case HLSL_IR_LOAD:
            {
                struct hlsl_ir_load *load = hlsl_ir_load(instr);
                /* We need to at least allocate a variable for undefs.
                 * FIXME: We should probably find a way to remove them instead. */
                allocate_variable_temp_register(ctx, load->src.var, allocator);
                break;
            }

            case HLSL_IR_LOOP:
            {
                struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);
                allocate_temp_registers_recurse(ctx, &loop->body, allocator);
                break;
            }

            case HLSL_IR_STORE:
            {
                struct hlsl_ir_store *store = hlsl_ir_store(instr);
                allocate_variable_temp_register(ctx, store->lhs.var, allocator);
                break;
            }

            case HLSL_IR_SWITCH:
            {
                struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
                struct hlsl_ir_switch_case *c;

                LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                {
                    allocate_temp_registers_recurse(ctx, &c->body, allocator);
                }
                break;
            }

            default:
                break;
        }
    }
}

static void record_constant(struct hlsl_ctx *ctx, unsigned int component_index, float f)
{
    struct hlsl_constant_defs *defs = &ctx->constant_defs;
    struct hlsl_constant_register *reg;
    size_t i;

    for (i = 0; i < defs->count; ++i)
    {
        reg = &defs->regs[i];
        if (reg->index == (component_index / 4))
        {
            reg->value.f[component_index % 4] = f;
            return;
        }
    }

    if (!hlsl_array_reserve(ctx, (void **)&defs->regs, &defs->size, defs->count + 1, sizeof(*defs->regs)))
        return;
    reg = &defs->regs[defs->count++];
    memset(reg, 0, sizeof(*reg));
    reg->index = component_index / 4;
    reg->value.f[component_index % 4] = f;
}

static void allocate_const_registers_recurse(struct hlsl_ctx *ctx,
        struct hlsl_block *block, struct register_allocator *allocator)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (instr->type)
        {
            case HLSL_IR_CONSTANT:
            {
                struct hlsl_ir_constant *constant = hlsl_ir_constant(instr);
                const struct hlsl_type *type = instr->data_type;
                unsigned int x, i;

                constant->reg = allocate_numeric_registers_for_type(ctx, allocator, 1, UINT_MAX, type);
                TRACE("Allocated constant @%u to %s.\n", instr->index, debug_register('c', constant->reg, type));

                assert(hlsl_is_numeric_type(type));
                assert(type->dimy == 1);
                assert(constant->reg.writemask);

                for (x = 0, i = 0; x < 4; ++x)
                {
                    const union hlsl_constant_value_component *value;
                    float f;

                    if (!(constant->reg.writemask & (1u << x)))
                        continue;
                    value = &constant->value.u[i++];

                    switch (type->base_type)
                    {
                        case HLSL_TYPE_BOOL:
                            f = !!value->u;
                            break;

                        case HLSL_TYPE_FLOAT:
                        case HLSL_TYPE_HALF:
                            f = value->f;
                            break;

                        case HLSL_TYPE_INT:
                            f = value->i;
                            break;

                        case HLSL_TYPE_UINT:
                            f = value->u;
                            break;

                        case HLSL_TYPE_DOUBLE:
                            FIXME("Double constant.\n");
                            return;

                        default:
                            vkd3d_unreachable();
                    }

                    record_constant(ctx, constant->reg.id * 4 + x, f);
                }

                break;
            }

            case HLSL_IR_IF:
            {
                struct hlsl_ir_if *iff = hlsl_ir_if(instr);
                allocate_const_registers_recurse(ctx, &iff->then_block, allocator);
                allocate_const_registers_recurse(ctx, &iff->else_block, allocator);
                break;
            }

            case HLSL_IR_LOOP:
            {
                struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);
                allocate_const_registers_recurse(ctx, &loop->body, allocator);
                break;
            }

            case HLSL_IR_SWITCH:
            {
                struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
                struct hlsl_ir_switch_case *c;

                LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                {
                    allocate_const_registers_recurse(ctx, &c->body, allocator);
                }
                break;
            }

            default:
                break;
        }
    }
}

static void allocate_const_registers(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct register_allocator allocator = {0};
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_uniform && var->last_read)
        {
            unsigned int reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];

            if (reg_size == 0)
                continue;

            var->regs[HLSL_REGSET_NUMERIC] = allocate_numeric_registers_for_type(ctx, &allocator,
                    1, UINT_MAX, var->data_type);
            TRACE("Allocated %s to %s.\n", var->name,
                    debug_register('c', var->regs[HLSL_REGSET_NUMERIC], var->data_type));
        }
    }

    allocate_const_registers_recurse(ctx, &entry_func->body, &allocator);

    vkd3d_free(allocator.allocations);
}

/* Simple greedy temporary register allocation pass that just assigns a unique
 * index to all (simultaneously live) variables or intermediate values. Agnostic
 * as to how many registers are actually available for the current backend, and
 * does not handle constants. */
static void allocate_temp_registers(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct register_allocator allocator = {0};

    /* ps_1_* outputs are special and go in temp register 0. */
    if (ctx->profile->major_version == 1 && ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        size_t i;

        for (i = 0; i < entry_func->parameters.count; ++i)
        {
            const struct hlsl_ir_var *var = entry_func->parameters.vars[i];

            if (var->is_output_semantic)
            {
                record_allocation(ctx, &allocator, 0, VKD3DSP_WRITEMASK_ALL, var->first_write, var->last_read);
                break;
            }
        }
    }

    allocate_temp_registers_recurse(ctx, &entry_func->body, &allocator);
    ctx->temp_count = allocator.reg_count;
    vkd3d_free(allocator.allocations);
}

static void allocate_semantic_register(struct hlsl_ctx *ctx, struct hlsl_ir_var *var, unsigned int *counter, bool output)
{
    static const char *const shader_names[] =
    {
        [VKD3D_SHADER_TYPE_PIXEL] = "Pixel",
        [VKD3D_SHADER_TYPE_VERTEX] = "Vertex",
        [VKD3D_SHADER_TYPE_GEOMETRY] = "Geometry",
        [VKD3D_SHADER_TYPE_HULL] = "Hull",
        [VKD3D_SHADER_TYPE_DOMAIN] = "Domain",
        [VKD3D_SHADER_TYPE_COMPUTE] = "Compute",
    };

    enum vkd3d_shader_register_type type;
    uint32_t reg;
    bool builtin;

    assert(var->semantic.name);

    if (ctx->profile->major_version < 4)
    {
        D3DSHADER_PARAM_REGISTER_TYPE sm1_type;
        D3DDECLUSAGE usage;
        uint32_t usage_idx;

        /* ps_1_* outputs are special and go in temp register 0. */
        if (ctx->profile->major_version == 1 && output && ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
            return;

        builtin = hlsl_sm1_register_from_semantic(ctx, &var->semantic, output, &sm1_type, &reg);
        if (!builtin && !hlsl_sm1_usage_from_semantic(&var->semantic, &usage, &usage_idx))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "Invalid semantic '%s'.", var->semantic.name);
            return;
        }

        if ((!output && !var->last_read) || (output && !var->first_write))
            return;
        type = (enum vkd3d_shader_register_type)sm1_type;
    }
    else
    {
        D3D_NAME usage;
        bool has_idx;

        if (!hlsl_sm4_usage_from_semantic(ctx, &var->semantic, output, &usage))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "Invalid semantic '%s'.", var->semantic.name);
            return;
        }
        if ((builtin = hlsl_sm4_register_from_semantic(ctx, &var->semantic, output, &type, &has_idx)))
            reg = has_idx ? var->semantic.index : 0;
    }

    if (builtin)
    {
        TRACE("%s %s semantic %s[%u] matches predefined register %#x[%u].\n", shader_names[ctx->profile->type],
                output ? "output" : "input", var->semantic.name, var->semantic.index, type, reg);
    }
    else
    {
        var->regs[HLSL_REGSET_NUMERIC].allocated = true;
        var->regs[HLSL_REGSET_NUMERIC].id = (*counter)++;
        var->regs[HLSL_REGSET_NUMERIC].allocation_size = 1;
        var->regs[HLSL_REGSET_NUMERIC].writemask = (1 << var->data_type->dimx) - 1;
        TRACE("Allocated %s to %s.\n", var->name, debug_register(output ? 'o' : 'v',
                var->regs[HLSL_REGSET_NUMERIC], var->data_type));
    }
}

static void allocate_semantic_registers(struct hlsl_ctx *ctx)
{
    unsigned int input_counter = 0, output_counter = 0;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_input_semantic)
            allocate_semantic_register(ctx, var, &input_counter, false);
        if (var->is_output_semantic)
            allocate_semantic_register(ctx, var, &output_counter, true);
    }
}

static const struct hlsl_buffer *get_reserved_buffer(struct hlsl_ctx *ctx, uint32_t index)
{
    const struct hlsl_buffer *buffer;

    LIST_FOR_EACH_ENTRY(buffer, &ctx->buffers, const struct hlsl_buffer, entry)
    {
        if (buffer->used_size && buffer->reservation.reg_type == 'b' && buffer->reservation.reg_index == index)
            return buffer;
    }
    return NULL;
}

static void calculate_buffer_offset(struct hlsl_ctx *ctx, struct hlsl_ir_var *var)
{
    unsigned int var_reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];
    enum hlsl_type_class var_class = var->data_type->class;
    struct hlsl_buffer *buffer = var->buffer;

    if (var->reg_reservation.offset_type == 'c')
    {
        if (var->reg_reservation.offset_index % 4)
        {
            if (var_class == HLSL_CLASS_MATRIX)
            {
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "packoffset() reservations with matrix types must be aligned with the beginning of a register.");
            }
            else if (var_class == HLSL_CLASS_ARRAY)
            {
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "packoffset() reservations with array types must be aligned with the beginning of a register.");
            }
            else if (var_class == HLSL_CLASS_STRUCT)
            {
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "packoffset() reservations with struct types must be aligned with the beginning of a register.");
            }
            else if (var_class == HLSL_CLASS_VECTOR)
            {
                unsigned int aligned_offset = hlsl_type_get_sm4_offset(var->data_type, var->reg_reservation.offset_index);

                if (var->reg_reservation.offset_index != aligned_offset)
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                            "packoffset() reservations with vector types cannot span multiple registers.");
            }
        }
        var->buffer_offset = var->reg_reservation.offset_index;
    }
    else
    {
        var->buffer_offset = hlsl_type_get_sm4_offset(var->data_type, buffer->size);
    }

    TRACE("Allocated buffer offset %u to %s.\n", var->buffer_offset, var->name);
    buffer->size = max(buffer->size, var->buffer_offset + var_reg_size);
    if (var->last_read)
        buffer->used_size = max(buffer->used_size, var->buffer_offset + var_reg_size);
}

static void validate_buffer_offsets(struct hlsl_ctx *ctx)
{
    struct hlsl_ir_var *var1, *var2;
    struct hlsl_buffer *buffer;

    LIST_FOR_EACH_ENTRY(var1, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var1->is_uniform || hlsl_type_is_resource(var1->data_type))
            continue;

        buffer = var1->buffer;
        if (!buffer->used_size)
            continue;

        LIST_FOR_EACH_ENTRY(var2, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            unsigned int var1_reg_size, var2_reg_size;

            if (!var2->is_uniform || hlsl_type_is_resource(var2->data_type))
                continue;

            if (var1 == var2 || var1->buffer != var2->buffer)
                continue;

            /* This is to avoid reporting the error twice for the same pair of overlapping variables. */
            if (strcmp(var1->name, var2->name) >= 0)
                continue;

            var1_reg_size = var1->data_type->reg_size[HLSL_REGSET_NUMERIC];
            var2_reg_size = var2->data_type->reg_size[HLSL_REGSET_NUMERIC];

            if (var1->buffer_offset < var2->buffer_offset + var2_reg_size
                    && var2->buffer_offset < var1->buffer_offset + var1_reg_size)
                hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "Invalid packoffset() reservation: Variables %s and %s overlap.",
                        var1->name, var2->name);
        }
    }

    LIST_FOR_EACH_ENTRY(var1, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        buffer = var1->buffer;
        if (!buffer || buffer == ctx->globals_buffer)
            continue;

        if (var1->reg_reservation.offset_type
                || (var1->data_type->class == HLSL_CLASS_OBJECT && var1->reg_reservation.reg_type))
            buffer->manually_packed_elements = true;
        else
            buffer->automatically_packed_elements = true;

        if (buffer->manually_packed_elements && buffer->automatically_packed_elements)
        {
            hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                    "packoffset() must be specified for all the buffer elements, or none of them.");
            break;
        }
    }
}

static void allocate_buffers(struct hlsl_ctx *ctx)
{
    struct hlsl_buffer *buffer;
    struct hlsl_ir_var *var;
    uint32_t index = 0;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_uniform && !hlsl_type_is_resource(var->data_type))
        {
            if (var->is_param)
                var->buffer = ctx->params_buffer;

            calculate_buffer_offset(ctx, var);
        }
    }

    validate_buffer_offsets(ctx);

    LIST_FOR_EACH_ENTRY(buffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (!buffer->used_size)
            continue;

        if (buffer->type == HLSL_BUFFER_CONSTANT)
        {
            if (buffer->reservation.reg_type == 'b')
            {
                const struct hlsl_buffer *reserved_buffer = get_reserved_buffer(ctx, buffer->reservation.reg_index);

                if (reserved_buffer && reserved_buffer != buffer)
                {
                    hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                            "Multiple buffers bound to cb%u.", buffer->reservation.reg_index);
                    hlsl_note(ctx, &reserved_buffer->loc, VKD3D_SHADER_LOG_ERROR,
                            "Buffer %s is already bound to cb%u.", reserved_buffer->name, buffer->reservation.reg_index);
                }

                buffer->reg.id = buffer->reservation.reg_index;
                buffer->reg.allocation_size = 1;
                buffer->reg.allocated = true;
                TRACE("Allocated reserved %s to cb%u.\n", buffer->name, index);
            }
            else if (!buffer->reservation.reg_type)
            {
                while (get_reserved_buffer(ctx, index))
                    ++index;

                buffer->reg.id = index;
                buffer->reg.allocation_size = 1;
                buffer->reg.allocated = true;
                TRACE("Allocated %s to cb%u.\n", buffer->name, index);
                ++index;
            }
            else
            {
                hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "Constant buffers must be allocated to register type 'b'.");
            }
        }
        else
        {
            FIXME("Allocate registers for texture buffers.\n");
        }
    }
}

static const struct hlsl_ir_var *get_allocated_object(struct hlsl_ctx *ctx, enum hlsl_regset regset,
        uint32_t index, bool allocated_only)
{
    const struct hlsl_ir_var *var;
    unsigned int start, count;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, const struct hlsl_ir_var, extern_entry)
    {
        if (var->reg_reservation.reg_type == get_regset_name(regset)
                && var->data_type->reg_size[regset])
        {
            /* Vars with a reservation prevent non-reserved vars from being
             * bound there even if the reserved vars aren't used. */
            start = var->reg_reservation.reg_index;
            count = var->data_type->reg_size[regset];

            if (!var->regs[regset].allocated && allocated_only)
                continue;
        }
        else if (var->regs[regset].allocated)
        {
            start = var->regs[regset].id;
            count = var->regs[regset].allocation_size;
        }
        else
        {
            continue;
        }

        if (start <= index && index < start + count)
            return var;
    }
    return NULL;
}

static void allocate_objects(struct hlsl_ctx *ctx, enum hlsl_regset regset)
{
    char regset_name = get_regset_name(regset);
    struct hlsl_ir_var *var;
    uint32_t min_index = 0;

    if (regset == HLSL_REGSET_UAVS)
    {
        LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            if (var->semantic.name && (!ascii_strcasecmp(var->semantic.name, "color")
                    || !ascii_strcasecmp(var->semantic.name, "sv_target")))
                min_index = max(min_index, var->semantic.index + 1);
        }
    }

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int count = var->regs[regset].allocation_size;

        if (count == 0)
            continue;

        /* The variable was already allocated if it has a reservation. */
        if (var->regs[regset].allocated)
        {
            const struct hlsl_ir_var *reserved_object, *last_reported = NULL;
            unsigned int index, i;

            if (var->regs[regset].id < min_index)
            {
                assert(regset == HLSL_REGSET_UAVS);
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                        "UAV index (%u) must be higher than the maximum render target index (%u).",
                        var->regs[regset].id, min_index - 1);
                continue;
            }

            for (i = 0; i < count; ++i)
            {
                index = var->regs[regset].id + i;

                /* get_allocated_object() may return "var" itself, but we
                 * actually want that, otherwise we'll end up reporting the
                 * same conflict between the same two variables twice. */
                reserved_object = get_allocated_object(ctx, regset, index, true);
                if (reserved_object && reserved_object != var && reserved_object != last_reported)
                {
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                            "Multiple variables bound to %c%u.", regset_name, index);
                    hlsl_note(ctx, &reserved_object->loc, VKD3D_SHADER_LOG_ERROR,
                            "Variable '%s' is already bound to %c%u.", reserved_object->name,
                            regset_name, index);
                    last_reported = reserved_object;
                }
            }
        }
        else
        {
            unsigned int index = min_index;
            unsigned int available = 0;

            while (available < count)
            {
                if (get_allocated_object(ctx, regset, index, false))
                    available = 0;
                else
                    ++available;
                ++index;
            }
            index -= count;

            var->regs[regset].id = index;
            var->regs[regset].allocated = true;
            TRACE("Allocated variable %s to %c%u-%c%u.\n", var->name, regset_name, index, regset_name,
                    index + count);
            ++index;
        }
    }
}

bool hlsl_component_index_range_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        unsigned int *start, unsigned int *count)
{
    struct hlsl_type *type = deref->var->data_type;
    unsigned int i, k;

    *start = 0;
    *count = 0;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        unsigned int idx = 0;

        assert(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            return false;

        /* We should always have generated a cast to UINT. */
        assert(path_node->data_type->class == HLSL_CLASS_SCALAR
                && path_node->data_type->base_type == HLSL_TYPE_UINT);

        idx = hlsl_ir_constant(path_node)->value.u[0].u;

        switch (type->class)
        {
            case HLSL_CLASS_VECTOR:
                if (idx >= type->dimx)
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Vector index is out of bounds. %u/%u", idx, type->dimx);
                    return false;
                }
                *start += idx;
                break;

            case HLSL_CLASS_MATRIX:
                if (idx >= hlsl_type_major_size(type))
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Matrix index is out of bounds. %u/%u", idx, hlsl_type_major_size(type));
                    return false;
                }
                if (hlsl_type_is_row_major(type))
                    *start += idx * type->dimx;
                else
                    *start += idx * type->dimy;
                break;

            case HLSL_CLASS_ARRAY:
                if (idx >= type->e.array.elements_count)
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Array index is out of bounds. %u/%u", idx, type->e.array.elements_count);
                    return false;
                }
                *start += idx * hlsl_type_component_count(type->e.array.type);
                break;

            case HLSL_CLASS_STRUCT:
                for (k = 0; k < idx; ++k)
                    *start += hlsl_type_component_count(type->e.record.fields[k].type);
                break;

            default:
                vkd3d_unreachable();
        }

        type = hlsl_get_element_type_from_path_index(ctx, type, path_node);
    }

    *count = hlsl_type_component_count(type);
    return true;
}

bool hlsl_regset_index_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        enum hlsl_regset regset, unsigned int *index)
{
    struct hlsl_type *type = deref->var->data_type;
    unsigned int i;

    assert(regset <= HLSL_REGSET_LAST_OBJECT);

    *index = 0;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        unsigned int idx = 0;

        assert(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            return false;

        /* We should always have generated a cast to UINT. */
        assert(path_node->data_type->class == HLSL_CLASS_SCALAR
                && path_node->data_type->base_type == HLSL_TYPE_UINT);

        idx = hlsl_ir_constant(path_node)->value.u[0].u;

        switch (type->class)
        {
            case HLSL_CLASS_ARRAY:
                if (idx >= type->e.array.elements_count)
                    return false;

                *index += idx * type->e.array.type->reg_size[regset];
                break;

            case HLSL_CLASS_STRUCT:
                *index += type->e.record.fields[idx].reg_offset[regset];
                break;

            default:
                vkd3d_unreachable();
        }

        type = hlsl_get_element_type_from_path_index(ctx, type, path_node);
    }

    assert(type->reg_size[regset] == 1);
    return true;
}

bool hlsl_offset_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref, unsigned int *offset)
{
    enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);
    struct hlsl_ir_node *offset_node = deref->rel_offset.node;
    unsigned int size;

    *offset = deref->const_offset;

    if (offset_node)
    {
        /* We should always have generated a cast to UINT. */
        assert(offset_node->data_type->class == HLSL_CLASS_SCALAR
                && offset_node->data_type->base_type == HLSL_TYPE_UINT);
        assert(offset_node->type != HLSL_IR_CONSTANT);
        return false;
    }

    size = deref->var->data_type->reg_size[regset];
    if (*offset >= size)
    {
        hlsl_error(ctx, &offset_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                "Dereference is out of bounds. %u/%u", *offset, size);
        return false;
    }

    return true;
}

unsigned int hlsl_offset_from_deref_safe(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    unsigned int offset;

    if (hlsl_offset_from_deref(ctx, deref, &offset))
        return offset;

    hlsl_fixme(ctx, &deref->rel_offset.node->loc, "Dereference with non-constant offset of type %s.",
            hlsl_node_type_to_string(deref->rel_offset.node->type));

    return 0;
}

struct hlsl_reg hlsl_reg_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    const struct hlsl_ir_var *var = deref->var;
    struct hlsl_reg ret = var->regs[HLSL_REGSET_NUMERIC];
    unsigned int offset = hlsl_offset_from_deref_safe(ctx, deref);

    assert(deref->data_type);
    assert(hlsl_is_numeric_type(deref->data_type));

    ret.id += offset / 4;

    ret.writemask = 0xf & (0xf << (offset % 4));
    if (var->regs[HLSL_REGSET_NUMERIC].writemask)
        ret.writemask = hlsl_combine_writemasks(var->regs[HLSL_REGSET_NUMERIC].writemask, ret.writemask);

    return ret;
}

static void parse_numthreads_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    unsigned int i;

    ctx->found_numthreads = 1;

    if (attr->args_count != 3)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 3 parameters for [numthreads] attribute, but got %u.", attr->args_count);
        return;
    }

    for (i = 0; i < attr->args_count; ++i)
    {
        const struct hlsl_ir_node *instr = attr->args[i].node;
        const struct hlsl_type *type = instr->data_type;
        const struct hlsl_ir_constant *constant;

        if (type->class != HLSL_CLASS_SCALAR
                || (type->base_type != HLSL_TYPE_INT && type->base_type != HLSL_TYPE_UINT))
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_type_to_string(ctx, type)))
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Wrong type for argument %u of [numthreads]: expected int or uint, but got %s.",
                        i, string->buffer);
            hlsl_release_string_buffer(ctx, string);
            break;
        }

        if (instr->type != HLSL_IR_CONSTANT)
        {
            hlsl_fixme(ctx, &instr->loc, "Non-constant expression in [numthreads] initializer.");
            break;
        }
        constant = hlsl_ir_constant(instr);

        if ((type->base_type == HLSL_TYPE_INT && constant->value.u[0].i <= 0)
                || (type->base_type == HLSL_TYPE_UINT && !constant->value.u[0].u))
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_THREAD_COUNT,
                    "Thread count must be a positive integer.");

        ctx->thread_count[i] = constant->value.u[0].u;
    }
}

static bool type_has_object_components(struct hlsl_type *type)
{
    if (type->class == HLSL_CLASS_OBJECT)
        return true;
    if (type->class == HLSL_CLASS_ARRAY)
        return type_has_object_components(type->e.array.type);
    if (type->class == HLSL_CLASS_STRUCT)
    {
        unsigned int i;

        for (i = 0; i < type->e.record.field_count; ++i)
        {
            if (type_has_object_components(type->e.record.fields[i].type))
                return true;
        }
    }
    return false;
}

static void remove_unreachable_code(struct hlsl_ctx *ctx, struct hlsl_block *body)
{
    struct hlsl_ir_node *instr, *next;
    struct hlsl_block block;
    struct list *start;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &body->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            remove_unreachable_code(ctx, &iff->then_block);
            remove_unreachable_code(ctx, &iff->else_block);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);

            remove_unreachable_code(ctx, &loop->body);
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
            {
                remove_unreachable_code(ctx, &c->body);
            }
        }
    }

    /* Remove instructions past unconditional jumps. */
    LIST_FOR_EACH_ENTRY(instr, &body->instrs, struct hlsl_ir_node, entry)
    {
        struct hlsl_ir_jump *jump;

        if (instr->type != HLSL_IR_JUMP)
            continue;

        jump = hlsl_ir_jump(instr);
        if (jump->type != HLSL_IR_JUMP_BREAK && jump->type != HLSL_IR_JUMP_CONTINUE)
            continue;

        if (!(start = list_next(&body->instrs, &instr->entry)))
            break;

        hlsl_block_init(&block);
        list_move_slice_tail(&block.instrs, start, list_tail(&body->instrs));
        hlsl_block_cleanup(&block);

        break;
    }
}

int hlsl_emit_bytecode(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func,
        enum vkd3d_shader_target_type target_type, struct vkd3d_shader_code *out)
{
    const struct hlsl_profile_info *profile = ctx->profile;
    struct hlsl_block *const body = &entry_func->body;
    struct recursive_call_ctx recursive_call_ctx;
    struct hlsl_ir_var *var;
    unsigned int i;
    bool progress;

    list_move_head(&body->instrs, &ctx->static_initializers.instrs);

    memset(&recursive_call_ctx, 0, sizeof(recursive_call_ctx));
    hlsl_transform_ir(ctx, find_recursive_calls, body, &recursive_call_ctx);
    vkd3d_free(recursive_call_ctx.backtrace);

    /* Avoid going into an infinite loop when processing call instructions.
     * lower_return() recurses into inferior calls. */
    if (ctx->result)
        return ctx->result;

    lower_return(ctx, entry_func, body, false);

    while (hlsl_transform_ir(ctx, lower_calls, body, NULL));

    lower_ir(ctx, lower_matrix_swizzles, body);
    lower_ir(ctx, lower_index_loads, body);

    LIST_FOR_EACH_ENTRY(var, &ctx->globals->vars, struct hlsl_ir_var, scope_entry)
    {
        if (var->storage_modifiers & HLSL_STORAGE_UNIFORM)
            prepend_uniform_copy(ctx, body, var);
    }

    for (i = 0; i < entry_func->parameters.count; ++i)
    {
        var = entry_func->parameters.vars[i];

        if (hlsl_type_is_resource(var->data_type) || (var->storage_modifiers & HLSL_STORAGE_UNIFORM))
        {
            prepend_uniform_copy(ctx, body, var);
        }
        else
        {
            if (type_has_object_components(var->data_type))
                hlsl_fixme(ctx, &var->loc, "Prepend uniform copies for object components within structs.");

            if (hlsl_get_multiarray_element_type(var->data_type)->class != HLSL_CLASS_STRUCT
                    && !var->semantic.name)
            {
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_SEMANTIC,
                        "Parameter \"%s\" is missing a semantic.", var->name);
                var->semantic.reported_missing = true;
            }

            if (var->storage_modifiers & HLSL_STORAGE_IN)
                prepend_input_var_copy(ctx, body, var);
            if (var->storage_modifiers & HLSL_STORAGE_OUT)
                append_output_var_copy(ctx, body, var);
        }
    }
    if (entry_func->return_var)
    {
        if (entry_func->return_var->data_type->class != HLSL_CLASS_STRUCT && !entry_func->return_var->semantic.name)
            hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_SEMANTIC,
                    "Entry point \"%s\" is missing a return value semantic.", entry_func->func->name);

        append_output_var_copy(ctx, body, entry_func->return_var);
    }

    for (i = 0; i < entry_func->attr_count; ++i)
    {
        const struct hlsl_attribute *attr = entry_func->attrs[i];

        if (!strcmp(attr->name, "numthreads") && profile->type == VKD3D_SHADER_TYPE_COMPUTE)
            parse_numthreads_attribute(ctx, attr);
        else
            hlsl_warning(ctx, &entry_func->attrs[i]->loc, VKD3D_SHADER_WARNING_HLSL_UNKNOWN_ATTRIBUTE,
                    "Ignoring unknown attribute \"%s\".", entry_func->attrs[i]->name);
    }

    if (profile->type == VKD3D_SHADER_TYPE_COMPUTE && !ctx->found_numthreads)
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [numthreads] attribute.", entry_func->func->name);

    if (profile->major_version >= 4)
    {
        hlsl_transform_ir(ctx, lower_discard_neg, body, NULL);
    }
    lower_ir(ctx, lower_broadcasts, body);
    while (hlsl_transform_ir(ctx, fold_redundant_casts, body, NULL));
    do
    {
        progress = hlsl_transform_ir(ctx, split_array_copies, body, NULL);
        progress |= hlsl_transform_ir(ctx, split_struct_copies, body, NULL);
    }
    while (progress);
    hlsl_transform_ir(ctx, split_matrix_copies, body, NULL);

    lower_ir(ctx, lower_narrowing_casts, body);
    lower_ir(ctx, lower_casts_to_bool, body);
    lower_ir(ctx, lower_int_dot, body);
    lower_ir(ctx, lower_int_division, body);
    lower_ir(ctx, lower_int_modulus, body);
    lower_ir(ctx, lower_int_abs, body);
    lower_ir(ctx, lower_float_modulus, body);
    hlsl_transform_ir(ctx, fold_redundant_casts, body, NULL);
    do
    {
        progress = hlsl_transform_ir(ctx, hlsl_fold_constant_exprs, body, NULL);
        progress |= hlsl_transform_ir(ctx, hlsl_fold_constant_swizzles, body, NULL);
        progress |= hlsl_copy_propagation_execute(ctx, body);
        progress |= hlsl_transform_ir(ctx, fold_swizzle_chains, body, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_swizzles, body, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_conditional_branches, body, NULL);
    }
    while (progress);
    remove_unreachable_code(ctx, body);
    hlsl_transform_ir(ctx, normalize_switch_cases, body, NULL);

    lower_ir(ctx, lower_nonconstant_vector_derefs, body);
    lower_ir(ctx, lower_casts_to_bool, body);
    lower_ir(ctx, lower_int_dot, body);

    hlsl_transform_ir(ctx, validate_static_object_references, body, NULL);
    hlsl_transform_ir(ctx, track_object_components_sampler_dim, body, NULL);
    if (profile->major_version >= 4)
        hlsl_transform_ir(ctx, lower_combined_samples, body, NULL);
    hlsl_transform_ir(ctx, track_object_components_usage, body, NULL);
    sort_synthetic_separated_samplers_first(ctx);

    lower_ir(ctx, lower_ternary, body);
    if (profile->major_version < 4)
    {
        lower_ir(ctx, lower_division, body);
        lower_ir(ctx, lower_sqrt, body);
        lower_ir(ctx, lower_dot, body);
        lower_ir(ctx, lower_round, body);
        lower_ir(ctx, lower_ceil, body);
        lower_ir(ctx, lower_floor, body);
    }

    if (profile->major_version < 2)
    {
        lower_ir(ctx, lower_abs, body);
    }

    /* TODO: move forward, remove when no longer needed */
    transform_derefs(ctx, replace_deref_path_with_offset, body);
    while (hlsl_transform_ir(ctx, hlsl_fold_constant_exprs, body, NULL));
    transform_derefs(ctx, clean_constant_deref_offset_srcs, body);

    do
        compute_liveness(ctx, entry_func);
    while (hlsl_transform_ir(ctx, dce, body, NULL));

    compute_liveness(ctx, entry_func);

    if (TRACE_ON())
        rb_for_each_entry(&ctx->functions, dump_function, ctx);

    transform_derefs(ctx, mark_indexable_vars, body);

    calculate_resource_register_counts(ctx);

    allocate_register_reservations(ctx);

    allocate_temp_registers(ctx, entry_func);
    if (profile->major_version < 4)
    {
        allocate_const_registers(ctx, entry_func);
    }
    else
    {
        allocate_buffers(ctx);
        allocate_objects(ctx, HLSL_REGSET_TEXTURES);
        allocate_objects(ctx, HLSL_REGSET_UAVS);
    }
    allocate_semantic_registers(ctx);
    allocate_objects(ctx, HLSL_REGSET_SAMPLERS);

    if (ctx->result)
        return ctx->result;

    switch (target_type)
    {
        case VKD3D_SHADER_TARGET_D3D_BYTECODE:
            return hlsl_sm1_write(ctx, entry_func, out);

        case VKD3D_SHADER_TARGET_DXBC_TPF:
            return hlsl_sm4_write(ctx, entry_func, out);

        default:
            ERR("Unsupported shader target type %#x.\n", target_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
    }
}
