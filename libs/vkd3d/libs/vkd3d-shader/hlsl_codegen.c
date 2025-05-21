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
#include "vkd3d_shader_private.h"
#include "d3dcommon.h"
#include <stdio.h>
#include <math.h>

/* The shift that corresponds to the D3D_SIF_TEXTURE_COMPONENTS mask. */
#define VKD3D_SM4_SIF_TEXTURE_COMPONENTS_SHIFT 2

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
            if (idx->type != HLSL_IR_CONSTANT)
            {
                hlsl_fixme(ctx, &idx->loc, "Non-constant vector addressing.");
                break;
            }
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
                VKD3D_ASSERT(size % 4 == 0);
                size /= 4;
            }

            c = hlsl_block_add_uint_constant(ctx, block, size, loc);

            idx_offset = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, c, idx);
            break;
        }

        case HLSL_CLASS_STRUCT:
        {
            unsigned int field_idx = hlsl_ir_constant(idx)->value.u[0].u;
            struct hlsl_struct_field *field = &type->e.record.fields[field_idx];
            unsigned int field_offset = field->reg_offset[regset];

            if (regset == HLSL_REGSET_NUMERIC)
            {
                VKD3D_ASSERT(*offset_component == 0);
                *offset_component = field_offset % 4;
                field_offset /= 4;
            }

            idx_offset = hlsl_block_add_uint_constant(ctx, block, field_offset, loc);
            break;
        }

        default:
            vkd3d_unreachable();
    }

    if (idx_offset)
        return hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, base_offset, idx_offset);
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

    offset = hlsl_block_add_uint_constant(ctx, block, 0, loc);

    VKD3D_ASSERT(deref->var);
    type = deref->var->data_type;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_block idx_block;

        hlsl_block_init(&idx_block);
        offset = new_offset_from_path_index(ctx, &idx_block, type, offset,
                deref->path[i].node, regset, offset_component, loc);
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

    VKD3D_ASSERT(deref->var);
    VKD3D_ASSERT(!hlsl_deref_is_lowered(deref));

    type = hlsl_deref_get_type(ctx, deref);

    /* Instructions that directly refer to structs or arrays (instead of single-register components)
     * are removed later by dce. So it is not a problem to just cleanup their derefs. */
    if (type->class == HLSL_CLASS_STRUCT || type->class == HLSL_CLASS_ARRAY)
    {
        hlsl_cleanup_deref(deref);
        return true;
    }

    deref->data_type = type;

    offset = new_offset_instr_from_deref(ctx, &block, deref, &offset_component, &instr->loc);
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


/* For a uniform variable, create a temp copy of it so, in case a value is
 * stored to the uniform at some point the shader, all derefs can be diverted
 * to this temp copy instead.
 * Also, promote the uniform to an extern var. */
static void prepend_uniform_copy(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_var *uniform)
{
    struct hlsl_ir_node *store;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *temp;
    char *new_name;

    uniform->is_uniform = 1;
    list_add_tail(&ctx->extern_vars, &uniform->extern_entry);

    if (!(new_name = hlsl_sprintf_alloc(ctx, "<temp-%s>", uniform->name)))
        return;

    if (!(temp = hlsl_new_var(ctx, new_name, uniform->data_type,
            &uniform->loc, NULL, uniform->storage_modifiers, NULL)))
    {
        vkd3d_free(new_name);
        return;
    }
    list_add_before(&uniform->scope_entry, &temp->scope_entry);

    uniform->temp_copy = temp;

    if (!(load = hlsl_new_var_load(ctx, uniform, &uniform->loc)))
        return;
    list_add_head(&block->instrs, &load->node.entry);

    if (!(store = hlsl_new_simple_store(ctx, temp, &load->node)))
        return;
    list_add_after(&load->node.entry, &store->entry);
}

/* If a uniform is written to at some point in the shader, all dereferences
 * must point to the temp copy instead, which is what this pass does. */
static bool divert_written_uniform_derefs_to_temp(struct hlsl_ctx *ctx, struct hlsl_deref *deref,
        struct hlsl_ir_node *instr)
{
    if (!deref->var->is_uniform || !deref->var->first_write)
        return false;

    /* Skip derefs from instructions before first write so copies from the
     * uniform to the temp are unaffected. */
    if (instr->index < deref->var->first_write)
        return false;

    VKD3D_ASSERT(deref->var->temp_copy);

    deref->var = deref->var->temp_copy;
    return true;
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
    switch (base)
    {
        case HLSL_TYPE_BOOL:
        case HLSL_TYPE_INT:
        case HLSL_TYPE_MIN16UINT:
        case HLSL_TYPE_UINT:
            return HLSL_TYPE_UINT;

        case HLSL_TYPE_HALF:
        case HLSL_TYPE_FLOAT:
            return HLSL_TYPE_FLOAT;

        case HLSL_TYPE_DOUBLE:
            return HLSL_TYPE_DOUBLE;
    }

    vkd3d_unreachable();
}

static bool types_are_semantic_equivalent(struct hlsl_ctx *ctx, const struct hlsl_type *type1,
        const struct hlsl_type *type2)
{
    if (ctx->profile->major_version < 4)
        return true;

    if (hlsl_type_is_primitive_array(type1))
    {
        return hlsl_type_is_primitive_array(type2)
                && type1->e.array.array_type == type2->e.array.array_type
                && type1->e.array.elements_count == type2->e.array.elements_count
                && types_are_semantic_equivalent(ctx, type1->e.array.type, type2->e.array.type);
    }

    if (type1->e.numeric.dimx != type2->e.numeric.dimx)
        return false;

    return base_type_get_semantic_equivalent(type1->e.numeric.type)
            == base_type_get_semantic_equivalent(type2->e.numeric.type);
}

static struct hlsl_ir_var *add_semantic_var(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        struct hlsl_ir_var *var, struct hlsl_type *type, uint32_t modifiers, struct hlsl_semantic *semantic,
        uint32_t index, bool output, bool force_align, bool create, const struct vkd3d_shader_location *loc)
{
    struct hlsl_semantic new_semantic;
    struct hlsl_ir_var *ext_var;
    const char *prefix;
    char *new_name;

    if (hlsl_type_is_primitive_array(type))
        prefix = type->e.array.array_type == HLSL_ARRAY_PATCH_OUTPUT ? "outputpatch" : "inputprim";
    else
        prefix = output ? "output" : "input";

    if (!(new_name = hlsl_sprintf_alloc(ctx, "<%s-%s%u>", prefix, semantic->name, index)))
        return NULL;

    LIST_FOR_EACH_ENTRY(ext_var, &func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!ascii_strcasecmp(ext_var->name, new_name))
        {
            VKD3D_ASSERT(hlsl_type_is_primitive_array(ext_var->data_type)
                    || ext_var->data_type->class <= HLSL_CLASS_VECTOR);
            VKD3D_ASSERT(hlsl_type_is_primitive_array(type) || type->class <= HLSL_CLASS_VECTOR);

            vkd3d_free(new_name);

            if (!create)
                return ext_var;

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

            return ext_var;
        }
    }

    VKD3D_ASSERT(create);

    if (!(hlsl_clone_semantic(ctx, &new_semantic, semantic)))
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
    ext_var->force_align = force_align;
    list_add_before(&var->scope_entry, &ext_var->scope_entry);
    list_add_tail(&func->extern_vars, &ext_var->extern_entry);

    return ext_var;
}

static uint32_t combine_field_storage_modifiers(uint32_t modifiers, uint32_t field_modifiers)
{
    field_modifiers |= modifiers;

    /* TODO: 'sample' modifier is not supported yet. */

    /* 'nointerpolation' always takes precedence, next the same is done for
     * 'sample', remaining modifiers are combined. */
    if (field_modifiers & HLSL_STORAGE_NOINTERPOLATION)
    {
        field_modifiers &= ~HLSL_INTERPOLATION_MODIFIERS_MASK;
        field_modifiers |= HLSL_STORAGE_NOINTERPOLATION;
    }

    return field_modifiers;
}

static void prepend_input_copy(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        struct hlsl_block *block, uint32_t prim_index, struct hlsl_ir_load *lhs,
        uint32_t modifiers, struct hlsl_semantic *semantic, uint32_t semantic_index, bool force_align)
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

    vector_type_dst = hlsl_get_vector_type(ctx, type->e.numeric.type, hlsl_type_minor_size(type));
    vector_type_src = vector_type_dst;
    if (ctx->profile->major_version < 4 && ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX)
        vector_type_src = hlsl_get_vector_type(ctx, type->e.numeric.type, 4);

    if (hlsl_type_major_size(type) > 1)
        force_align = true;

    for (i = 0; i < hlsl_type_major_size(type); ++i)
    {
        struct hlsl_ir_node *cast;
        struct hlsl_ir_var *input;
        struct hlsl_ir_load *load;

        if (hlsl_type_is_primitive_array(var->data_type))
        {
            struct hlsl_type *prim_type_src;
            struct hlsl_deref prim_deref;
            struct hlsl_ir_node *idx;

            if (!(prim_type_src = hlsl_new_array_type(ctx, vector_type_src, var->data_type->e.array.elements_count,
                    var->data_type->e.array.array_type)))
                return;
            prim_type_src->modifiers = var->data_type->modifiers & HLSL_PRIMITIVE_MODIFIERS_MASK;

            if (!(input = add_semantic_var(ctx, func, var, prim_type_src,
                    modifiers, semantic, semantic_index + i, false, force_align, true, loc)))
                return;
            hlsl_init_simple_deref_from_var(&prim_deref, input);

            idx = hlsl_block_add_uint_constant(ctx, block, prim_index, &var->loc);

            if (!(load = hlsl_new_load_index(ctx, &prim_deref, idx, loc)))
                return;
            hlsl_block_add_instr(block, &load->node);
        }
        else
        {
            if (!(input = add_semantic_var(ctx, func, var, vector_type_src,
                    modifiers, semantic, semantic_index + i, false, force_align, true, loc)))
                return;

            if (!(load = hlsl_new_var_load(ctx, input, &var->loc)))
                return;
            hlsl_block_add_instr(block, &load->node);
        }

        cast = hlsl_block_add_cast(ctx, block, &load->node, vector_type_dst, &var->loc);

        if (type->class == HLSL_CLASS_MATRIX)
        {
            c = hlsl_block_add_uint_constant(ctx, block, i, &var->loc);

            hlsl_block_add_store_index(ctx, block, &lhs->src, c, cast, 0, &var->loc);
        }
        else
        {
            VKD3D_ASSERT(i == 0);

            hlsl_block_add_store_index(ctx, block, &lhs->src, NULL, cast, 0, &var->loc);
        }
    }
}

static void prepend_input_copy_recurse(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        struct hlsl_block *block, uint32_t prim_index, struct hlsl_ir_load *lhs,
        uint32_t modifiers, struct hlsl_semantic *semantic, uint32_t semantic_index, bool force_align)
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
            uint32_t element_modifiers;

            if (type->class == HLSL_CLASS_ARRAY)
            {
                elem_semantic_index = semantic_index
                        + i * hlsl_type_get_array_element_reg_size(type->e.array.type, HLSL_REGSET_NUMERIC) / 4;
                element_modifiers = modifiers;
                force_align = true;

                if (hlsl_type_is_primitive_array(type))
                    prim_index = i;
            }
            else
            {
                field = &type->e.record.fields[i];
                if (hlsl_type_is_resource(field->type))
                {
                    hlsl_fixme(ctx, &field->loc, "Prepend uniform copies for resource components within structs.");
                    continue;
                }
                validate_field_semantic(ctx, field);
                semantic = &field->semantic;
                elem_semantic_index = semantic->index;
                loc = &field->loc;
                element_modifiers = combine_field_storage_modifiers(modifiers, field->storage_modifiers);
                force_align = (i == 0);
            }

            c = hlsl_block_add_uint_constant(ctx, block, i, &var->loc);

            /* This redundant load is expected to be deleted later by DCE. */
            if (!(element_load = hlsl_new_load_index(ctx, &lhs->src, c, loc)))
                return;
            hlsl_block_add_instr(block, &element_load->node);

            prepend_input_copy_recurse(ctx, func, block, prim_index, element_load,
                    element_modifiers, semantic, elem_semantic_index, force_align);
        }
    }
    else
    {
        prepend_input_copy(ctx, func, block, prim_index, lhs, modifiers, semantic, semantic_index, force_align);
    }
}

/* Split inputs into two variables representing the semantic and temp registers,
 * and copy the former to the latter, so that writes to input variables work. */
static void prepend_input_var_copy(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func, struct hlsl_ir_var *var)
{
    struct hlsl_ir_load *load;
    struct hlsl_block block;

    hlsl_block_init(&block);

    /* This redundant load is expected to be deleted later by DCE. */
    if (!(load = hlsl_new_var_load(ctx, var, &var->loc)))
        return;
    hlsl_block_add_instr(&block, &load->node);

    prepend_input_copy_recurse(ctx, func, &block, 0, load, var->storage_modifiers,
            &var->semantic, var->semantic.index, false);

    list_move_head(&func->body.instrs, &block.instrs);
}

static void append_output_copy(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_function_decl *func, struct hlsl_ir_load *rhs, uint32_t modifiers,
        struct hlsl_semantic *semantic, uint32_t semantic_index, bool force_align, bool create)
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

    vector_type = hlsl_get_vector_type(ctx, type->e.numeric.type, hlsl_type_minor_size(type));

    if (hlsl_type_major_size(type) > 1)
        force_align = true;

    for (i = 0; i < hlsl_type_major_size(type); ++i)
    {
        struct hlsl_ir_var *output;
        struct hlsl_ir_node *load;

        if (!(output = add_semantic_var(ctx, func, var, vector_type,
                modifiers, semantic, semantic_index + i, true, force_align, create, loc)))
            return;

        if (type->class == HLSL_CLASS_MATRIX)
        {
            c = hlsl_block_add_uint_constant(ctx, block, i, &var->loc);
            load = hlsl_block_add_load_index(ctx, block, &rhs->src, c, &var->loc);
        }
        else
        {
            VKD3D_ASSERT(i == 0);

            load = hlsl_block_add_load_index(ctx, block, &rhs->src, NULL, &var->loc);
        }

        hlsl_block_add_simple_store(ctx, block, output, load);
    }
}

static void append_output_copy_recurse(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_function_decl *func, const struct hlsl_type *type, struct hlsl_ir_load *rhs, uint32_t modifiers,
        struct hlsl_semantic *semantic, uint32_t semantic_index, bool force_align, bool create)
{
    struct vkd3d_shader_location *loc = &rhs->node.loc;
    struct hlsl_ir_var *var = rhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (type->class == HLSL_CLASS_ARRAY || type->class == HLSL_CLASS_STRUCT)
    {
        for (i = 0; i < hlsl_type_element_count(type); ++i)
        {
            uint32_t element_modifiers, elem_semantic_index;
            const struct hlsl_type *element_type;
            struct hlsl_ir_load *element_load;
            struct hlsl_struct_field *field;

            if (type->class == HLSL_CLASS_ARRAY)
            {
                elem_semantic_index = semantic_index
                        + i * hlsl_type_get_array_element_reg_size(type->e.array.type, HLSL_REGSET_NUMERIC) / 4;
                element_type = type->e.array.type;
                element_modifiers = modifiers;
                force_align = true;
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
                element_type = field->type;
                element_modifiers = combine_field_storage_modifiers(modifiers, field->storage_modifiers);
                force_align = (i == 0);
            }

            c = hlsl_block_add_uint_constant(ctx, block, i, &var->loc);

            if (!(element_load = hlsl_new_load_index(ctx, &rhs->src, c, loc)))
                return;
            hlsl_block_add_instr(block, &element_load->node);

            append_output_copy_recurse(ctx, block, func, element_type, element_load, element_modifiers, semantic,
                    elem_semantic_index, force_align, create);
        }
    }
    else
    {
        append_output_copy(ctx, block, func, rhs, modifiers, semantic, semantic_index, force_align, create);
    }
}

/* Split outputs into two variables representing the temp and semantic
 * registers, and copy the former to the latter, so that reads from output
 * variables work. */
static void append_output_var_copy(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func, struct hlsl_ir_var *var)
{
    struct hlsl_ir_load *load;

    /* This redundant load is expected to be deleted later by DCE. */
    if (!(load = hlsl_new_var_load(ctx, var, &var->loc)))
        return;
    hlsl_block_add_instr(&func->body, &load->node);

    append_output_copy_recurse(ctx, &func->body, func, var->data_type, load, var->storage_modifiers,
            &var->semantic, var->semantic.index, false, true);
}

bool hlsl_transform_ir(struct hlsl_ctx *ctx, bool (*func)(struct hlsl_ctx *ctx, struct hlsl_ir_node *, void *),
        struct hlsl_block *block, void *context)
{
    struct hlsl_ir_node *instr, *next;
    bool progress = false;

    if (ctx->result)
        return false;

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

        case HLSL_IR_INTERLOCKED:
            res = func(ctx, &hlsl_ir_interlocked(instr)->dst, instr);
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
    struct hlsl_block then_block;
    struct hlsl_ir_load *load;
    struct hlsl_ir_node *iff;

    hlsl_block_init(&then_block);

    if (!(load = hlsl_new_var_load(ctx, func->early_return_var, &cf_instr->loc)))
        return;
    list_add_after(&cf_instr->entry, &load->node.entry);

    hlsl_block_add_jump(ctx, &then_block, HLSL_IR_JUMP_BREAK, NULL, &cf_instr->loc);

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
        VKD3D_ASSERT(!in_loop);

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
        struct hlsl_ir_node *not, *load;
        struct hlsl_block then_block;

        /* If we're in a loop, we should have used "break" instead. */
        VKD3D_ASSERT(!in_loop);

        if (tail == &cf_instr->entry)
            return has_early_return;

        hlsl_block_init(&then_block);
        list_move_slice_tail(&then_block.instrs, list_next(&block->instrs, &cf_instr->entry), tail);
        lower_return(ctx, func, &then_block, in_loop);

        load = hlsl_block_add_simple_load(ctx, block, func->early_return_var, &cf_instr->loc);
        not = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_LOGIC_NOT, load, &cf_instr->loc);
        hlsl_block_add_if(ctx, block, not, &then_block, NULL, &cf_instr->loc);
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

static struct hlsl_ir_node *add_zero_mipmap_level(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_node *index, const struct vkd3d_shader_location *loc)
{
    unsigned int dim_count = index->data_type->e.numeric.dimx;
    struct hlsl_deref coords_deref;
    struct hlsl_ir_var *coords;
    struct hlsl_ir_node *zero;

    VKD3D_ASSERT(dim_count < 4);

    if (!(coords = hlsl_new_synthetic_var(ctx, "coords",
            hlsl_get_vector_type(ctx, HLSL_TYPE_UINT, dim_count + 1), loc)))
        return NULL;

    hlsl_init_simple_deref_from_var(&coords_deref, coords);
    hlsl_block_add_store_index(ctx, block, &coords_deref, NULL, index, (1u << dim_count) - 1, loc);

    zero = hlsl_block_add_uint_constant(ctx, block, 0, loc);
    hlsl_block_add_store_index(ctx, block, &coords_deref, NULL, zero, 1u << dim_count, loc);

    return hlsl_block_add_simple_load(ctx, block, coords, loc);
}

static bool lower_complex_casts(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    unsigned int src_comp_count, dst_comp_count;
    struct hlsl_type *src_type, *dst_type;
    struct hlsl_deref var_deref;
    bool broadcast, matrix_cast;
    struct hlsl_ir_node *arg;
    struct hlsl_ir_var *var;
    unsigned int dst_idx;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    if (hlsl_ir_expr(instr)->op != HLSL_OP1_CAST)
        return false;

    arg = hlsl_ir_expr(instr)->operands[0].node;
    dst_type = instr->data_type;
    src_type = arg->data_type;

    if (src_type->class <= HLSL_CLASS_VECTOR && dst_type->class <= HLSL_CLASS_VECTOR)
        return false;

    src_comp_count = hlsl_type_component_count(src_type);
    dst_comp_count = hlsl_type_component_count(dst_type);
    broadcast = hlsl_is_numeric_type(src_type) && src_type->e.numeric.dimx == 1 && src_type->e.numeric.dimy == 1;
    matrix_cast = !broadcast && dst_comp_count != src_comp_count
            && src_type->class == HLSL_CLASS_MATRIX && dst_type->class == HLSL_CLASS_MATRIX;

    VKD3D_ASSERT(src_comp_count >= dst_comp_count || broadcast);
    if (matrix_cast)
    {
        VKD3D_ASSERT(dst_type->e.numeric.dimx <= src_type->e.numeric.dimx);
        VKD3D_ASSERT(dst_type->e.numeric.dimy <= src_type->e.numeric.dimy);
    }

    if (!(var = hlsl_new_synthetic_var(ctx, "cast", dst_type, &instr->loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    for (dst_idx = 0; dst_idx < dst_comp_count; ++dst_idx)
    {
        struct hlsl_ir_node *component_load, *cast;
        struct hlsl_type *dst_comp_type;
        unsigned int src_idx;

        if (broadcast)
        {
            src_idx = 0;
        }
        else if (matrix_cast)
        {
            unsigned int x = dst_idx % dst_type->e.numeric.dimx, y = dst_idx / dst_type->e.numeric.dimx;

            src_idx = y * src_type->e.numeric.dimx + x;
        }
        else
        {
            src_idx = dst_idx;
        }

        dst_comp_type = hlsl_type_get_component_type(ctx, dst_type, dst_idx);
        component_load = hlsl_add_load_component(ctx, block, arg, src_idx, &arg->loc);
        cast = hlsl_block_add_cast(ctx, block, component_load, dst_comp_type, &arg->loc);

        hlsl_block_add_store_component(ctx, block, &var_deref, dst_idx, cast);
    }

    hlsl_block_add_simple_load(ctx, block, var, &instr->loc);
    return true;
}

/* hlsl_ir_swizzle nodes that directly point to a matrix value are only a parse-time construct that
 * represents matrix swizzles (e.g. mat._m01_m23) before we know if they will be used in the lhs of
 * an assignment or as a value made from different components of the matrix. The former cases should
 * have already been split into several separate assignments, but the latter are lowered by this
 * pass. */
static bool lower_matrix_swizzles(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_swizzle *swizzle;
    struct hlsl_deref var_deref;
    struct hlsl_type *matrix_type;
    struct hlsl_ir_var *var;
    unsigned int k, i;

    if (instr->type != HLSL_IR_SWIZZLE)
        return false;
    swizzle = hlsl_ir_swizzle(instr);
    matrix_type = swizzle->val.node->data_type;
    if (matrix_type->class != HLSL_CLASS_MATRIX)
        return false;

    if (!(var = hlsl_new_synthetic_var(ctx, "matrix-swizzle", instr->data_type, &instr->loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    for (i = 0; i < instr->data_type->e.numeric.dimx; ++i)
    {
        struct hlsl_ir_node *load;

        k = swizzle->u.matrix.components[i].y * matrix_type->e.numeric.dimx + swizzle->u.matrix.components[i].x;

        load = hlsl_add_load_component(ctx, block, swizzle->val.node, k, &instr->loc);
        hlsl_block_add_store_component(ctx, block, &var_deref, i, load);
    }

    hlsl_block_add_simple_load(ctx, block, var, &instr->loc);
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
    struct hlsl_deref var_deref;
    struct hlsl_ir_index *index;
    struct hlsl_ir_load *load;
    struct hlsl_ir_node *val;
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

        VKD3D_ASSERT(coords->data_type->class == HLSL_CLASS_VECTOR);
        VKD3D_ASSERT(coords->data_type->e.numeric.type == HLSL_TYPE_UINT);
        VKD3D_ASSERT(coords->data_type->e.numeric.dimx == dim_count);

        if (!(coords = add_zero_mipmap_level(ctx, block, coords, &instr->loc)))
            return false;

        params.type = HLSL_RESOURCE_LOAD;
        params.resource = val;
        params.coords = coords;
        params.format = val->data_type->e.resource.format;
        hlsl_block_add_resource_load(ctx, block, &params, &instr->loc);
        return true;
    }

    if (!(var = hlsl_new_synthetic_var(ctx, "index-val", val->data_type, &instr->loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    hlsl_block_add_simple_store(ctx, block, var, val);

    if (hlsl_index_is_noncontiguous(index))
    {
        struct hlsl_ir_node *mat = index->val.node;
        struct hlsl_deref row_deref;
        unsigned int i;

        VKD3D_ASSERT(!hlsl_type_is_row_major(mat->data_type));

        if (!(var = hlsl_new_synthetic_var(ctx, "row", instr->data_type, &instr->loc)))
            return false;
        hlsl_init_simple_deref_from_var(&row_deref, var);

        for (i = 0; i < mat->data_type->e.numeric.dimx; ++i)
        {
            struct hlsl_ir_node *c;

            c = hlsl_block_add_uint_constant(ctx, block, i, &instr->loc);

            if (!(load = hlsl_new_load_index(ctx, &var_deref, c, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, &load->node);

            if (!(load = hlsl_new_load_index(ctx, &load->src, index->idx.node, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, &load->node);

            hlsl_block_add_store_index(ctx, block, &row_deref, c, &load->node, 0, &instr->loc);
        }

        hlsl_block_add_simple_load(ctx, block, var, &instr->loc);
    }
    else
    {
        hlsl_block_add_load_index(ctx, block, &var_deref, index->idx.node, &instr->loc);
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

    if (src_type->class <= HLSL_CLASS_VECTOR && dst_type->class <= HLSL_CLASS_VECTOR && src_type->e.numeric.dimx == 1)
    {
        struct hlsl_ir_node *new_cast;

        dst_scalar_type = hlsl_get_scalar_type(ctx, dst_type->e.numeric.type);
        /* We need to preserve the cast since it might be doing more than just
         * turning the scalar into a vector. */
        new_cast = hlsl_block_add_cast(ctx, block, cast->operands[0].node, dst_scalar_type, &cast->node.loc);

        if (dst_type->e.numeric.dimx != 1)
            hlsl_block_add_swizzle(ctx, block, HLSL_SWIZZLE(X, X, X, X),
                    dst_type->e.numeric.dimx, new_cast, &cast->node.loc);

        return true;
    }

    return false;
}

/* Allocate a unique, ordered index to each instruction, which will be used for
 * copy propagation and computing liveness ranges.
 * Index 0 means unused, so start at 1. */
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
 *
 * Moreover, we implement a transformation that propagates loads with a single
 * non-constant index in its deref path. Consider a load of the form
 * var[[a0][a1]...[i]...[an]], where ak are integral constants, and i is an
 * arbitrary non-constant node. If, for all j, the following holds:
 *
 *   var[[a0][a1]...[j]...[an]] = x[[c0*j + d0][c1*j + d1]...[cm*j + dm]],
 *
 * where ck, dk are constants, then we can replace the load with
 * x[[c0*i + d0]...[cm*i + dm]]. This pass is implemented by
 * copy_propagation_replace_with_deref().
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
    struct rb_tree *scope_var_defs;
    size_t scope_count, scopes_capacity;
    struct hlsl_ir_node *stop;
    bool stopped;
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

static size_t copy_propagation_push_scope(struct copy_propagation_state *state, struct hlsl_ctx *ctx)
{
    if (!(hlsl_array_reserve(ctx, (void **)&state->scope_var_defs, &state->scopes_capacity,
            state->scope_count + 1, sizeof(*state->scope_var_defs))))
        return false;

    rb_init(&state->scope_var_defs[state->scope_count++], copy_propagation_var_def_compare);

    return state->scope_count;
}

static size_t copy_propagation_pop_scope(struct copy_propagation_state *state)
{
    rb_destroy(&state->scope_var_defs[--state->scope_count], copy_propagation_var_def_destroy, NULL);

    return state->scope_count;
}

static bool copy_propagation_state_init(struct copy_propagation_state *state, struct hlsl_ctx *ctx)
{
    memset(state, 0, sizeof(*state));

    return copy_propagation_push_scope(state, ctx);
}

static void copy_propagation_state_destroy(struct copy_propagation_state *state)
{
    while (copy_propagation_pop_scope(state));

    vkd3d_free(state->scope_var_defs);
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
    for (size_t i = state->scope_count - 1; i < state->scope_count; i--)
    {
        struct rb_tree *tree = &state->scope_var_defs[i];
        struct rb_entry *entry = rb_get(tree, var);
        if (entry)
        {
            struct copy_propagation_var_def *var_def = RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);
            unsigned int component_count = hlsl_type_component_count(var->data_type);
            struct copy_propagation_value *value;

            VKD3D_ASSERT(component < component_count);
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
    struct rb_tree *tree = &state->scope_var_defs[state->scope_count - 1];
    struct rb_entry *entry = rb_get(tree, var);
    struct copy_propagation_var_def *var_def;
    unsigned int component_count = hlsl_type_component_count(var->data_type);
    int res;

    if (entry)
        return RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);

    if (!(var_def = hlsl_alloc(ctx, offsetof(struct copy_propagation_var_def, traces[component_count]))))
        return NULL;

    var_def->var = var;

    res = rb_put(tree, var, &var_def->entry);
    VKD3D_ASSERT(!res);

    return var_def;
}

static void copy_propagation_trace_record_value(struct hlsl_ctx *ctx,
        struct copy_propagation_component_trace *trace, struct hlsl_ir_node *node,
        unsigned int component, unsigned int time)
{
    VKD3D_ASSERT(!trace->record_count || trace->records[trace->record_count - 1].timestamp < time);

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
                VKD3D_ASSERT(!trace->records[trace->record_count - 1].node);
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
            uint32_t index = hlsl_ir_constant(path_node)->value.u[0].u;

            /* Don't bother invalidating anything if the index is constant but
             * out-of-range.
             * Such indices are illegal in HLSL, but only if the code is not
             * dead, and we can't always know if code is dead without copy-prop
             * itself. */
            if (index >= hlsl_type_element_count(type))
                return;

            copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, subtype,
                    depth + 1, comp_start + index * subtype_comp_count, writemask, time);
        }
        else
        {
            for (i = 0; i < hlsl_type_element_count(type); ++i)
            {
                copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, subtype,
                        depth + 1, comp_start + i * subtype_comp_count, writemask, time);
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
        uint32_t swizzle, struct hlsl_ir_node *instr)
{
    const unsigned int instr_component_count = hlsl_type_component_count(instr->data_type);
    const struct hlsl_deref *deref = &load->src;
    const struct hlsl_ir_var *var = deref->var;
    struct hlsl_ir_node *new_instr = NULL;
    unsigned int time = load->node.index;
    unsigned int start, count, i;
    uint32_t ret_swizzle = 0;

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
        hlsl_swizzle_set_component(&ret_swizzle, i, value->component);
    }

    TRACE("Load from %s[%u-%u]%s propagated as instruction %p%s.\n",
            var->name, start, start + count, debug_hlsl_swizzle(swizzle, instr_component_count),
            new_instr, debug_hlsl_swizzle(ret_swizzle, instr_component_count));

    if (new_instr->data_type->class == HLSL_CLASS_SCALAR || new_instr->data_type->class == HLSL_CLASS_VECTOR)
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
        uint32_t swizzle, struct hlsl_ir_node *instr)
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

static bool component_index_from_deref_path_node(struct hlsl_ir_node *path_node,
        struct hlsl_type *type, unsigned int *index)
{
    unsigned int idx, i;

    if (path_node->type != HLSL_IR_CONSTANT)
        return false;

    idx = hlsl_ir_constant(path_node)->value.u[0].u;
    *index = 0;

    switch (type->class)
    {
        case HLSL_CLASS_VECTOR:
            if (idx >= type->e.numeric.dimx)
                return false;
            *index = idx;
            break;

        case HLSL_CLASS_MATRIX:
            if (idx >= hlsl_type_major_size(type))
                return false;
            if (hlsl_type_is_row_major(type))
                *index = idx * type->e.numeric.dimx;
            else
                *index = idx * type->e.numeric.dimy;
            break;

        case HLSL_CLASS_ARRAY:
            if (idx >= type->e.array.elements_count)
                return false;
            *index = idx * hlsl_type_component_count(type->e.array.type);
            break;

        case HLSL_CLASS_STRUCT:
            for (i = 0; i < idx; ++i)
                *index += hlsl_type_component_count(type->e.record.fields[i].type);
            break;

        default:
            vkd3d_unreachable();
    }

    return true;
}

static bool nonconst_index_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        unsigned int *idx, unsigned int *base, unsigned int *scale, unsigned int *count)
{
    struct hlsl_type *type = deref->var->data_type;
    bool found = false;
    unsigned int i;

    *base = 0;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        struct hlsl_type *next_type;

        VKD3D_ASSERT(path_node);

        /* We should always have generated a cast to UINT. */
        VKD3D_ASSERT(hlsl_is_vec1(path_node->data_type) && path_node->data_type->e.numeric.type == HLSL_TYPE_UINT);

        next_type = hlsl_get_element_type_from_path_index(ctx, type, path_node);

        if (path_node->type != HLSL_IR_CONSTANT)
        {
            if (found)
                return false;
            found = true;
            *idx = i;
            *scale = hlsl_type_component_count(next_type);
            *count = hlsl_type_element_count(type);
        }
        else
        {
            unsigned int index;

            if (!component_index_from_deref_path_node(path_node, type, &index))
                return false;
            *base += index;
        }

        type = next_type;
    }

    return found;
}

static struct hlsl_ir_node *new_affine_path_index(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc,
        struct hlsl_block *block, struct hlsl_ir_node *index, int c, int d)
{
    struct hlsl_ir_node *c_node, *d_node, *ic, *idx;
    bool use_uint = c >= 0 && d >= 0;

    if (!c)
    {
        VKD3D_ASSERT(d >= 0);

        return hlsl_block_add_uint_constant(ctx, block, d, loc);
    }

    if (use_uint)
    {
        c_node = hlsl_block_add_uint_constant(ctx, block, c, loc);
        d_node = hlsl_block_add_uint_constant(ctx, block, d, loc);
    }
    else
    {
        c_node = hlsl_block_add_int_constant(ctx, block, c, loc);
        d_node = hlsl_block_add_int_constant(ctx, block, d, loc);
        index = hlsl_block_add_cast(ctx, block, index, hlsl_get_scalar_type(ctx, HLSL_TYPE_INT), loc);
    }

    ic = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, index, c_node);
    idx = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, ic, d_node);
    if (!use_uint)
        idx = hlsl_block_add_cast(ctx, block, idx, hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT), loc);

    return idx;
}

static bool copy_propagation_replace_with_deref(struct hlsl_ctx *ctx,
        const struct copy_propagation_state *state, const struct hlsl_ir_load *load,
        uint32_t swizzle, struct hlsl_ir_node *instr)
{
    const unsigned int instr_component_count = hlsl_type_component_count(instr->data_type);
    unsigned int nonconst_i = 0, base, scale, count;
    struct hlsl_ir_node *index, *new_instr = NULL;
    const struct hlsl_deref *deref = &load->src;
    const struct hlsl_ir_var *var = deref->var;
    unsigned int time = load->node.index;
    struct hlsl_deref tmp_deref = {0};
    struct hlsl_ir_load *new_load;
    struct hlsl_ir_var *x = NULL;
    int *c = NULL, *d = NULL;
    uint32_t ret_swizzle = 0;
    struct hlsl_block block;
    unsigned int path_len;
    bool success = false;
    int i, j, k;

    if (!nonconst_index_from_deref(ctx, deref, &nonconst_i, &base, &scale, &count))
        return false;

    VKD3D_ASSERT(count);

    hlsl_block_init(&block);

    index = deref->path[nonconst_i].node;

    /* Iterate over the nonconst index, and check if their values all have the form
     * x[[c0*i + d0][c1*i + d1]...[cm*i + dm]], and determine the constants c, d. */
    for (i = 0; i < count; ++i)
    {
        unsigned int start = base + scale * i;
        struct copy_propagation_value *value;
        struct hlsl_ir_load *idx;
        uint32_t cur_swizzle = 0;

        if (!(value = copy_propagation_get_value(state, var,
                start + hlsl_swizzle_get_component(swizzle, 0), time)))
            goto done;

        if (value->node->type != HLSL_IR_LOAD)
            goto done;
        idx = hlsl_ir_load(value->node);

        if (!x)
            x = idx->src.var;
        else if (x != idx->src.var)
            goto done;

        if (hlsl_version_lt(ctx, 4, 0) && x->is_uniform && ctx->profile->type != VKD3D_SHADER_TYPE_VERTEX)
        {
            TRACE("Skipping propagating non-constant deref to SM1 uniform %s.\n", var->name);
            goto done;
        }

        if (i == 0)
        {
            path_len = idx->src.path_len;

            if (path_len)
            {
                if (!(c = hlsl_calloc(ctx, path_len, sizeof(c[0])))
                        || !(d = hlsl_alloc(ctx, path_len * sizeof(d[0]))))
                    goto done;
            }

            for (k = 0; k < path_len; ++k)
            {
                if (idx->src.path[k].node->type != HLSL_IR_CONSTANT)
                    goto done;
                d[k] = hlsl_ir_constant(idx->src.path[k].node)->value.u[0].u;
            }

        }
        else if (i == 1)
        {
            struct hlsl_type *type = idx->src.var->data_type;

            if (idx->src.path_len != path_len)
                goto done;

            /* Calculate constants c and d based on the first two path indices. */
            for (k = 0; k < path_len; ++k)
            {
                int ix;

                if (idx->src.path[k].node->type != HLSL_IR_CONSTANT)
                    goto done;
                ix = hlsl_ir_constant(idx->src.path[k].node)->value.u[0].u;
                c[k] = ix - d[k];
                d[k] = ix - c[k] * i;

                if (c[k] && type->class == HLSL_CLASS_STRUCT)
                    goto done;

                type = hlsl_get_element_type_from_path_index(ctx, type, idx->src.path[k].node);
            }
        }
        else
        {
            if (idx->src.path_len != path_len)
                goto done;

            /* Check that this load has the form x[[c0*i +d0][c1*i + d1]...[cm*i + dm]]. */
            for (k = 0; k < path_len; ++k)
            {
                if (idx->src.path[k].node->type != HLSL_IR_CONSTANT)
                    goto done;
                if (hlsl_ir_constant(idx->src.path[k].node)->value.u[0].u != c[k] * i + d[k])
                    goto done;
            }
        }

        hlsl_swizzle_set_component(&cur_swizzle, 0, value->component);

        for (j = 1; j < instr_component_count; ++j)
        {
            struct copy_propagation_value *val;

            if (!(val = copy_propagation_get_value(state, var,
                    start + hlsl_swizzle_get_component(swizzle, j), time)))
                goto done;
            if (val->node != &idx->node)
                goto done;

            hlsl_swizzle_set_component(&cur_swizzle, j, val->component);
        }

        if (i == 0)
            ret_swizzle = cur_swizzle;
        else if (ret_swizzle != cur_swizzle)
            goto done;
    }

    if (!hlsl_init_deref(ctx, &tmp_deref, x, path_len))
        goto done;

    for (k = 0; k < path_len; ++k)
    {
        hlsl_src_from_node(&tmp_deref.path[k],
                new_affine_path_index(ctx, &load->node.loc, &block, index, c[k], d[k]));
    }

    if (!(new_load = hlsl_new_load_index(ctx, &tmp_deref, NULL, &load->node.loc)))
        goto done;
    new_instr = &new_load->node;
    hlsl_block_add_instr(&block, new_instr);

    if (new_instr->data_type->class == HLSL_CLASS_SCALAR || new_instr->data_type->class == HLSL_CLASS_VECTOR)
        new_instr = hlsl_block_add_swizzle(ctx, &block, ret_swizzle, instr_component_count, new_instr, &instr->loc);

    if (TRACE_ON())
    {
        struct vkd3d_string_buffer buffer;

        vkd3d_string_buffer_init(&buffer);

        vkd3d_string_buffer_printf(&buffer, "Load from %s[", var->name);
        for (j = 0; j < deref->path_len; ++j)
        {
            if (j == nonconst_i)
                vkd3d_string_buffer_printf(&buffer, "[i]");
            else
                vkd3d_string_buffer_printf(&buffer, "[%u]", hlsl_ir_constant(deref->path[j].node)->value.u[0].u);
        }
        vkd3d_string_buffer_printf(&buffer, "]%s propagated as %s[",
                debug_hlsl_swizzle(swizzle, instr_component_count), tmp_deref.var->name);
        for (k = 0; k < path_len; ++k)
        {
            if (c[k])
                vkd3d_string_buffer_printf(&buffer, "[i*%d + %d]", c[k], d[k]);
            else
                vkd3d_string_buffer_printf(&buffer, "[%d]", d[k]);
        }
        vkd3d_string_buffer_printf(&buffer, "]%s (i = %p).\n",
                debug_hlsl_swizzle(ret_swizzle, instr_component_count), index);

        vkd3d_string_buffer_trace(&buffer);
        vkd3d_string_buffer_cleanup(&buffer);
    }

    list_move_before(&instr->entry, &block.instrs);
    hlsl_replace_node(instr, new_instr);
    success = true;

done:
    hlsl_cleanup_deref(&tmp_deref);
    hlsl_block_cleanup(&block);
    vkd3d_free(c);
    vkd3d_free(d);
    return success;
}

static bool copy_propagation_transform_load(struct hlsl_ctx *ctx,
        struct hlsl_ir_load *load, struct copy_propagation_state *state)
{
    struct hlsl_type *type = load->node.data_type;

    switch (type->class)
    {
        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_STREAM_OUTPUT:
        case HLSL_CLASS_NULL:
            break;

        case HLSL_CLASS_MATRIX:
        case HLSL_CLASS_ARRAY:
        case HLSL_CLASS_STRUCT:
            /* We can't handle complex types here.
             * They should have been already split anyway by earlier passes,
             * but they may not have been deleted yet. We can't rely on DCE to
             * solve that problem for us, since we may be called on a partial
             * block, but DCE deletes dead stores, so it needs to be able to
             * see the whole program. */
        case HLSL_CLASS_ERROR:
            return false;

        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_VOID:
            vkd3d_unreachable();
    }

    if (copy_propagation_replace_with_constant_vector(ctx, state, load, HLSL_SWIZZLE(X, Y, Z, W), &load->node))
        return true;

    if (copy_propagation_replace_with_single_instr(ctx, state, load, HLSL_SWIZZLE(X, Y, Z, W), &load->node))
        return true;

    if (copy_propagation_replace_with_deref(ctx, state, load, HLSL_SWIZZLE(X, Y, Z, W), &load->node))
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

    if (copy_propagation_replace_with_constant_vector(ctx, state, load, swizzle->u.vector, &swizzle->node))
        return true;

    if (copy_propagation_replace_with_single_instr(ctx, state, load, swizzle->u.vector, &swizzle->node))
        return true;

    if (copy_propagation_replace_with_deref(ctx, state, load, swizzle->u.vector, &swizzle->node))
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
    VKD3D_ASSERT(count == 1);

    if (!(value = copy_propagation_get_value(state, deref->var, start, time)))
        return false;
    VKD3D_ASSERT(value->component == 0);

    /* A uniform object should have never been written to. */
    VKD3D_ASSERT(!deref->var->is_uniform);

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

static bool copy_propagation_transform_interlocked(struct hlsl_ctx *ctx,
        struct hlsl_ir_interlocked *interlocked, struct copy_propagation_state *state)
{
    bool progress = false;

    progress |= copy_propagation_transform_object_load(ctx, &interlocked->dst, state, interlocked->node.index);
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

        if (!hlsl_is_numeric_type(store->rhs.node->data_type))
            writemask = VKD3DSP_WRITEMASK_0;
        copy_propagation_set_value(ctx, var_def, start, writemask, store->rhs.node, store->node.index);
    }
    else
    {
        copy_propagation_invalidate_variable_from_deref(ctx, var_def, lhs, store->writemask,
                store->node.index);
    }
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
    bool progress = false;

    copy_propagation_push_scope(state, ctx);
    progress |= copy_propagation_transform_block(ctx, &iff->then_block, state);
    if (state->stopped)
        return progress;
    copy_propagation_pop_scope(state);

    copy_propagation_push_scope(state, ctx);
    progress |= copy_propagation_transform_block(ctx, &iff->else_block, state);
    if (state->stopped)
        return progress;
    copy_propagation_pop_scope(state);

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
    bool progress = false;

    copy_propagation_invalidate_from_block(ctx, state, &loop->body, loop->node.index);
    copy_propagation_invalidate_from_block(ctx, state, &loop->iter, loop->node.index);

    copy_propagation_push_scope(state, ctx);
    progress |= copy_propagation_transform_block(ctx, &loop->body, state);
    if (state->stopped)
        return progress;
    copy_propagation_pop_scope(state);

    return progress;
}

static bool copy_propagation_process_switch(struct hlsl_ctx *ctx, struct hlsl_ir_switch *s,
        struct copy_propagation_state *state)
{
    struct hlsl_ir_switch_case *c;
    bool progress = false;

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        copy_propagation_push_scope(state, ctx);
        progress |= copy_propagation_transform_block(ctx, &c->body, state);
        if (state->stopped)
            return progress;
        copy_propagation_pop_scope(state);
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
        if (instr == state->stop)
        {
            state->stopped = true;
            return progress;
        }

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

            case HLSL_IR_INTERLOCKED:
                progress |= copy_propagation_transform_interlocked(ctx, hlsl_ir_interlocked(instr), state);
                break;

            default:
                break;
        }

        if (state->stopped)
            return progress;
    }

    return progress;
}

bool hlsl_copy_propagation_execute(struct hlsl_ctx *ctx, struct hlsl_block *block)
{
    struct copy_propagation_state state;
    bool progress;

    if (ctx->result)
        return false;

    index_instructions(block, 1);

    copy_propagation_state_init(&state, ctx);

    progress = copy_propagation_transform_block(ctx, block, &state);

    copy_propagation_state_destroy(&state);

    return progress;
}

enum validation_result
{
    DEREF_VALIDATION_OK,
    DEREF_VALIDATION_OUT_OF_BOUNDS,
    DEREF_VALIDATION_NOT_CONSTANT,
};

struct vectorize_exprs_state
{
    struct vectorizable_exprs_group
    {
        struct hlsl_block *block;
        struct hlsl_ir_expr *exprs[4];
        uint8_t expr_count, component_count;
    } *groups;
    size_t count, capacity;
};

static bool is_same_vectorizable_source(struct hlsl_ir_node *a, struct hlsl_ir_node *b)
{
    /* TODO: We can also vectorize different constants. */

    if (a->type == HLSL_IR_SWIZZLE)
        a = hlsl_ir_swizzle(a)->val.node;
    if (b->type == HLSL_IR_SWIZZLE)
        b = hlsl_ir_swizzle(b)->val.node;

    return a == b;
}

static bool is_same_vectorizable_expr(struct hlsl_ir_expr *a, struct hlsl_ir_expr *b)
{
    if (a->op != b->op)
        return false;

    for (size_t j = 0; j < HLSL_MAX_OPERANDS; ++j)
    {
        if (!a->operands[j].node)
            break;
        if (!is_same_vectorizable_source(a->operands[j].node, b->operands[j].node))
            return false;
    }

    return true;
}

static void record_vectorizable_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_expr *expr, struct vectorize_exprs_state *state)
{
    if (expr->node.data_type->class > HLSL_CLASS_VECTOR)
        return;

    /* These are the only current ops that are not per-component. */
    if (expr->op == HLSL_OP1_COS_REDUCED || expr->op == HLSL_OP1_SIN_REDUCED
            || expr->op == HLSL_OP2_DOT || expr->op == HLSL_OP3_DP2ADD)
        return;

    for (size_t i = 0; i < state->count; ++i)
    {
        struct vectorizable_exprs_group *group = &state->groups[i];
        struct hlsl_ir_expr *other = group->exprs[0];

        /* These are SSA instructions, which means they have the same value
         * regardless of what block they're in. However, being in different
         * blocks may mean that one expression or the other is not always
         * executed. */

        if (expr->node.data_type->e.numeric.dimx + group->component_count <= 4
                && group->block == block
                && is_same_vectorizable_expr(expr, other))
        {
            group->exprs[group->expr_count++] = expr;
            group->component_count += expr->node.data_type->e.numeric.dimx;
            return;
        }
    }

    if (!hlsl_array_reserve(ctx, (void **)&state->groups,
            &state->capacity, state->count + 1, sizeof(*state->groups)))
        return;
    state->groups[state->count].block = block;
    state->groups[state->count].exprs[0] = expr;
    state->groups[state->count].expr_count = 1;
    state->groups[state->count].component_count = expr->node.data_type->e.numeric.dimx;
    ++state->count;
}

static void find_vectorizable_expr_groups(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct vectorize_exprs_state *state)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_EXPR)
        {
            record_vectorizable_expr(ctx, block, hlsl_ir_expr(instr), state);
        }
        else if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            find_vectorizable_expr_groups(ctx, &iff->then_block, state);
            find_vectorizable_expr_groups(ctx, &iff->else_block, state);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            find_vectorizable_expr_groups(ctx, &hlsl_ir_loop(instr)->body, state);
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                find_vectorizable_expr_groups(ctx, &c->body, state);
        }
    }
}

/* Combine sequences like
 *
 * 3: @1.x
 * 4: @2.x
 * 5: @3 * @4
 * 6: @1.y
 * 7: @2.x
 * 8: @6 * @7
 *
 * into
 *
 * 5_1: @1.xy
 * 5_2: @2.xx
 * 5_3: @5_1 * @5_2
 * 5:   @5_3.x
 * 8:   @5_3.y
 *
 * Each operand to an expression needs to refer to the same ultimate source
 * (in this case @1 and @2 respectively), but can be a swizzle thereof.
 *
 * In practice the swizzles @5 and @8 can generally then be vectorized again,
 * either as part of another expression, or as part of a store.
 */
static bool vectorize_exprs(struct hlsl_ctx *ctx, struct hlsl_block *block)
{
    struct vectorize_exprs_state state = {0};
    bool progress = false;

    find_vectorizable_expr_groups(ctx, block, &state);

    for (unsigned int i = 0; i < state.count; ++i)
    {
        struct vectorizable_exprs_group *group = &state.groups[i];
        struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};
        uint32_t swizzles[HLSL_MAX_OPERANDS] = {0};
        struct hlsl_ir_node *arg, *combined;
        unsigned int component_count = 0;
        struct hlsl_type *combined_type;
        struct hlsl_block new_block;
        struct hlsl_ir_expr *expr;

        if (group->expr_count == 1)
            continue;

        hlsl_block_init(&new_block);

        for (unsigned int j = 0; j < group->expr_count; ++j)
        {
            expr = group->exprs[j];

            for (unsigned int a = 0; a < HLSL_MAX_OPERANDS; ++a)
            {
                uint32_t arg_swizzle;

                if (!(arg = expr->operands[a].node))
                    break;

                if (arg->type == HLSL_IR_SWIZZLE)
                    arg_swizzle = hlsl_ir_swizzle(arg)->u.vector;
                else
                    arg_swizzle = HLSL_SWIZZLE(X, Y, Z, W);

                /* Mask out the invalid components. */
                arg_swizzle &= (1u << VKD3D_SHADER_SWIZZLE_SHIFT(arg->data_type->e.numeric.dimx)) - 1;
                swizzles[a] |= arg_swizzle << VKD3D_SHADER_SWIZZLE_SHIFT(component_count);
            }

            component_count += expr->node.data_type->e.numeric.dimx;
        }

        expr = group->exprs[0];
        for (unsigned int a = 0; a < HLSL_MAX_OPERANDS; ++a)
        {
            if (!(arg = expr->operands[a].node))
                break;
            if (arg->type == HLSL_IR_SWIZZLE)
                arg = hlsl_ir_swizzle(arg)->val.node;
            args[a] = hlsl_block_add_swizzle(ctx, &new_block, swizzles[a], component_count, arg, &arg->loc);
        }

        combined_type = hlsl_get_vector_type(ctx, expr->node.data_type->e.numeric.type, component_count);
        combined = hlsl_block_add_expr(ctx, &new_block, expr->op, args, combined_type, &expr->node.loc);

        list_move_before(&expr->node.entry, &new_block.instrs);

        TRACE("Combining %u %s instructions into %p.\n", group->expr_count,
                debug_hlsl_expr_op(group->exprs[0]->op), combined);

        component_count = 0;
        for (unsigned int j = 0; j < group->expr_count; ++j)
        {
            struct hlsl_ir_node *replacement;

            expr = group->exprs[j];

            if (!(replacement = hlsl_new_swizzle(ctx,
                    HLSL_SWIZZLE(X, Y, Z, W) >> VKD3D_SHADER_SWIZZLE_SHIFT(component_count),
                    expr->node.data_type->e.numeric.dimx, combined, &expr->node.loc)))
                goto out;
            component_count += expr->node.data_type->e.numeric.dimx;
            list_add_before(&expr->node.entry, &replacement->entry);
            hlsl_replace_node(&expr->node, replacement);
        }

        progress = true;
    }

out:
    vkd3d_free(state.groups);
    return progress;
}

struct vectorize_stores_state
{
    struct vectorizable_stores_group
    {
        struct hlsl_block *block;
        /* We handle overlapping stores, because it's not really easier not to.
         * In theory, then, we could collect an arbitrary number of stores here.
         *
         * In practice, overlapping stores are unlikely, and of course at most
         * 4 stores can appear without overlap. Therefore, for simplicity, we
         * just use a fixed array of 4.
         *
         * Since computing the writemask requires traversing the deref, and we
         * need to do that anyway, we store it here for convenience. */
        struct hlsl_ir_store *stores[4];
        unsigned int path_len;
        uint8_t writemasks[4];
        uint8_t store_count;
        bool dirty;
    } *groups;
    size_t count, capacity;
};

/* This must be a store to a subsection of a vector.
 * In theory we can also vectorize stores to packed struct fields,
 * but this requires target-specific knowledge and is probably best left
 * to a VSIR pass. */
static bool can_vectorize_store(struct hlsl_ctx *ctx, struct hlsl_ir_store *store,
        unsigned int *path_len, uint8_t *writemask)
{
    struct hlsl_type *type = store->lhs.var->data_type;
    unsigned int i;

    if (store->rhs.node->data_type->class > HLSL_CLASS_VECTOR)
        return false;

    if (type->class == HLSL_CLASS_SCALAR)
        return false;

    for (i = 0; type->class != HLSL_CLASS_VECTOR && i < store->lhs.path_len; ++i)
        type = hlsl_get_element_type_from_path_index(ctx, type, store->lhs.path[i].node);

    if (type->class != HLSL_CLASS_VECTOR)
        return false;

    *path_len = i;

    if (i < store->lhs.path_len)
    {
        struct hlsl_ir_constant *c;

        /* This is a store to a scalar component of a vector, achieved via
         * indexing. */

        if (store->lhs.path[i].node->type != HLSL_IR_CONSTANT)
            return false;
        c = hlsl_ir_constant(store->lhs.path[i].node);
        *writemask = (1u << c->value.u[0].u);
    }
    else
    {
        *writemask = store->writemask;
    }

    return true;
}

static bool derefs_are_same_vector(struct hlsl_ctx *ctx, const struct hlsl_deref *a, const struct hlsl_deref *b)
{
    struct hlsl_type *type = a->var->data_type;

    if (a->var != b->var)
        return false;

    for (unsigned int i = 0; type->class != HLSL_CLASS_VECTOR && i < a->path_len && i < b->path_len; ++i)
    {
        if (a->path[i].node != b->path[i].node)
            return false;
        type = hlsl_get_element_type_from_path_index(ctx, type, a->path[i].node);
    }

    return true;
}

static void record_vectorizable_store(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_store *store, struct vectorize_stores_state *state)
{
    unsigned int path_len;
    uint8_t writemask;

    if (!can_vectorize_store(ctx, store, &path_len, &writemask))
    {
        /* In the case of a dynamically indexed vector, we must invalidate
         * any groups that statically index the same vector.
         * For the sake of expediency, we go one step further and invalidate
         * any groups that store to the same variable.
         * (We also don't check that that was the reason why this store isn't
         * vectorizable.)
         * We could be more granular, but we'll defer that until it comes
         * up in practice. */
        for (size_t i = 0; i < state->count; ++i)
        {
            if (state->groups[i].stores[0]->lhs.var == store->lhs.var)
                state->groups[i].dirty = true;
        }
        return;
    }

    for (size_t i = 0; i < state->count; ++i)
    {
        struct vectorizable_stores_group *group = &state->groups[i];
        struct hlsl_ir_store *other = group->stores[0];

        if (group->dirty)
            continue;

        if (derefs_are_same_vector(ctx, &store->lhs, &other->lhs))
        {
            /* Stores must be in the same CFG block. If they're not,
             * they're not executed in exactly the same flow, and
             * therefore can't be vectorized. */
            if (group->block == block
                    && is_same_vectorizable_source(store->rhs.node, other->rhs.node))
            {
                if (group->store_count < ARRAY_SIZE(group->stores))
                {
                    group->stores[group->store_count] = store;
                    group->writemasks[group->store_count] = writemask;
                    ++group->store_count;
                    return;
                }
            }
            else
            {
                /* A store to the same vector with a different source, or in
                 * a different CFG block, invalidates any earlier store.
                 *
                 * A store to a component which *contains* the vector in
                 * question would also invalidate, but we should have split all
                 * of those by the time we get here. */
                group->dirty = true;

                /* Note that we do exit this loop early if we find a store A we
                 * can vectorize with, but that's fine. If there was a store B
                 * also in the state that we can't vectorize with, it would
                 * already have invalidated A. */
            }
        }
        else
        {
            /* This could still be a store to the same vector, if e.g. the
             * vector is part of a dynamically indexed array, or the path has
             * two equivalent instructions which refer to the same component.
             * [CSE may help with the latter, but we don't have it yet,
             * and we shouldn't depend on it anyway.]
             * For the sake of expediency, we just invalidate it if it refers
             * to the same variable at all.
             * As above, we could be more granular, but we'll defer that until
             * it comes up in practice. */
            if (store->lhs.var == other->lhs.var)
                group->dirty = true;

            /* As above, we don't need to worry about exiting the loop early. */
        }
    }

    if (!hlsl_array_reserve(ctx, (void **)&state->groups,
            &state->capacity, state->count + 1, sizeof(*state->groups)))
        return;
    state->groups[state->count].block = block;
    state->groups[state->count].stores[0] = store;
    state->groups[state->count].path_len = path_len;
    state->groups[state->count].writemasks[0] = writemask;
    state->groups[state->count].store_count = 1;
    state->groups[state->count].dirty = false;
    ++state->count;
}

static void mark_store_groups_dirty(struct hlsl_ctx *ctx,
        struct vectorize_stores_state *state, struct hlsl_ir_var *var)
{
    for (unsigned int i = 0; i < state->count; ++i)
    {
        if (state->groups[i].stores[0]->lhs.var == var)
            state->groups[i].dirty = true;
    }
}

static void find_vectorizable_store_groups(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct vectorize_stores_state *state)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_STORE)
        {
            record_vectorizable_store(ctx, block, hlsl_ir_store(instr), state);
        }
        else if (instr->type == HLSL_IR_LOAD)
        {
            /* By vectorizing store A with store B, we are effectively moving
             * store A down to happen at the same time as store B.
             * If there was a load of the same variable between the two, this
             * would be incorrect.
             * Therefore invalidate all stores to this variable. As above, we
             * could be more granular if necessary. */
            mark_store_groups_dirty(ctx, state, hlsl_ir_load(instr)->src.var);
        }
        else if (instr->type == HLSL_IR_INTERLOCKED)
        {
            /* An interlocked operation can be used on shared memory variables,
             * and it is at the same time both a store and a load, thus, we
             * should also mark all stores to this variable as dirty once we
             * find one.*/
            mark_store_groups_dirty(ctx, state, hlsl_ir_interlocked(instr)->dst.var);
        }
        else if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            find_vectorizable_store_groups(ctx, &iff->then_block, state);
            find_vectorizable_store_groups(ctx, &iff->else_block, state);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            find_vectorizable_store_groups(ctx, &hlsl_ir_loop(instr)->body, state);
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                find_vectorizable_store_groups(ctx, &c->body, state);
        }
    }
}

/* Combine sequences like
 *
 * 2: @1.yw
 * 3: @1.zy
 * 4: var.xy = @2
 * 5: var.yw = @3
 *
 * to
 *
 * 2: @1.yzy
 * 5: var.xyw = @2
 *
 * There are a lot of gotchas here. We need to make sure the two stores are to
 * the same vector (which may be embedded in a complex variable), that they're
 * always executed in the same control flow, and that there aren't any other
 * stores or loads on the same vector in the middle. */
static bool vectorize_stores(struct hlsl_ctx *ctx, struct hlsl_block *block)
{
    struct vectorize_stores_state state = {0};
    bool progress = false;

    find_vectorizable_store_groups(ctx, block, &state);

    for (unsigned int i = 0; i < state.count; ++i)
    {
        struct vectorizable_stores_group *group = &state.groups[i];
        uint32_t new_swizzle = 0, new_writemask = 0;
        struct hlsl_ir_node *new_rhs, *value;
        uint32_t swizzle_components[4];
        unsigned int component_count;
        struct hlsl_ir_store *store;
        struct hlsl_block new_block;

        if (group->store_count == 1)
            continue;

        hlsl_block_init(&new_block);

        /* Compute the swizzle components. */
        for (unsigned int j = 0; j < group->store_count; ++j)
        {
            unsigned int writemask = group->writemasks[j];
            uint32_t rhs_swizzle;

            store = group->stores[j];

            if (store->rhs.node->type == HLSL_IR_SWIZZLE)
                rhs_swizzle = hlsl_ir_swizzle(store->rhs.node)->u.vector;
            else
                rhs_swizzle = HLSL_SWIZZLE(X, Y, Z, W);

            component_count = 0;
            for (unsigned int k = 0; k < 4; ++k)
            {
                if (writemask & (1u << k))
                    swizzle_components[k] = hlsl_swizzle_get_component(rhs_swizzle, component_count++);
            }

            new_writemask |= writemask;
        }

        /* Construct the new swizzle. */
        component_count = 0;
        for (unsigned int k = 0; k < 4; ++k)
        {
            if (new_writemask & (1u << k))
                hlsl_swizzle_set_component(&new_swizzle, component_count++, swizzle_components[k]);
        }

        store = group->stores[0];
        value = store->rhs.node;
        if (value->type == HLSL_IR_SWIZZLE)
            value = hlsl_ir_swizzle(value)->val.node;

        new_rhs = hlsl_block_add_swizzle(ctx, &new_block, new_swizzle, component_count, value, &value->loc);
        hlsl_block_add_store_parent(ctx, &new_block, &store->lhs,
                group->path_len, new_rhs, new_writemask, &store->node.loc);

        TRACE("Combining %u stores to %s.\n", group->store_count, store->lhs.var->name);

        list_move_before(&group->stores[group->store_count - 1]->node.entry, &new_block.instrs);

        for (unsigned int j = 0; j < group->store_count; ++j)
        {
            list_remove(&group->stores[j]->node.entry);
            hlsl_free_instr(&group->stores[j]->node);
        }

        progress = true;
    }

    vkd3d_free(state.groups);
    return progress;
}

static enum validation_result validate_component_index_range_from_deref(struct hlsl_ctx *ctx,
        const struct hlsl_deref *deref)
{
    struct hlsl_type *type = deref->var->data_type;
    unsigned int i;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        unsigned int idx = 0;

        VKD3D_ASSERT(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            return DEREF_VALIDATION_NOT_CONSTANT;

        /* We should always have generated a cast to UINT. */
        VKD3D_ASSERT(hlsl_is_vec1(path_node->data_type) && path_node->data_type->e.numeric.type == HLSL_TYPE_UINT);

        idx = hlsl_ir_constant(path_node)->value.u[0].u;

        switch (type->class)
        {
            case HLSL_CLASS_VECTOR:
                if (idx >= type->e.numeric.dimx)
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Vector index is out of bounds. %u/%u", idx, type->e.numeric.dimx);
                    return DEREF_VALIDATION_OUT_OF_BOUNDS;
                }
                break;

            case HLSL_CLASS_MATRIX:
                if (idx >= hlsl_type_major_size(type))
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Matrix index is out of bounds. %u/%u", idx, hlsl_type_major_size(type));
                    return DEREF_VALIDATION_OUT_OF_BOUNDS;
                }
                break;

            case HLSL_CLASS_ARRAY:
                if (idx >= type->e.array.elements_count)
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Array index is out of bounds. %u/%u", idx, type->e.array.elements_count);
                    return DEREF_VALIDATION_OUT_OF_BOUNDS;
                }
                break;

            case HLSL_CLASS_STRUCT:
                break;

            default:
                vkd3d_unreachable();
        }

        type = hlsl_get_element_type_from_path_index(ctx, type, path_node);
    }

    return DEREF_VALIDATION_OK;
}

static void note_non_static_deref_expressions(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        const char *usage)
{
    unsigned int i;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;

        VKD3D_ASSERT(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            hlsl_note(ctx, &path_node->loc, VKD3D_SHADER_LOG_ERROR,
                    "Expression for %s within \"%s\" cannot be resolved statically.",
                    usage, deref->var->name);
    }
}

static bool validate_dereferences(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr,
        void *context)
{
    switch (instr->type)
    {
        case HLSL_IR_RESOURCE_LOAD:
        {
            struct hlsl_ir_resource_load *load = hlsl_ir_resource_load(instr);

            if (!load->resource.var->is_uniform)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Loaded resource must have a single uniform source.");
            }
            else if (validate_component_index_range_from_deref(ctx, &load->resource) == DEREF_VALIDATION_NOT_CONSTANT)
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
                else if (validate_component_index_range_from_deref(ctx, &load->sampler) == DEREF_VALIDATION_NOT_CONSTANT)
                {
                    hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                            "Resource load sampler from \"%s\" must be determinable at compile time.",
                            load->sampler.var->name);
                    note_non_static_deref_expressions(ctx, &load->sampler, "resource load sampler");
                }
            }
            break;
        }
        case HLSL_IR_RESOURCE_STORE:
        {
            struct hlsl_ir_resource_store *store = hlsl_ir_resource_store(instr);

            if (!store->resource.var->is_uniform)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Accessed resource must have a single uniform source.");
            }
            else if (validate_component_index_range_from_deref(ctx, &store->resource) == DEREF_VALIDATION_NOT_CONSTANT)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Accessed resource from \"%s\" must be determinable at compile time.",
                        store->resource.var->name);
                note_non_static_deref_expressions(ctx, &store->resource, "accessed resource");
            }
            break;
        }
        case HLSL_IR_LOAD:
        {
            struct hlsl_ir_load *load = hlsl_ir_load(instr);
            validate_component_index_range_from_deref(ctx, &load->src);
            break;
        }
        case HLSL_IR_STORE:
        {
            struct hlsl_ir_store *store = hlsl_ir_store(instr);
            validate_component_index_range_from_deref(ctx, &store->lhs);
            break;
        }
        case HLSL_IR_INTERLOCKED:
        {
            struct hlsl_ir_interlocked *interlocked = hlsl_ir_interlocked(instr);

            if (!interlocked->dst.var->is_uniform)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Accessed resource must have a single uniform source.");
            }
            else if (validate_component_index_range_from_deref(ctx, &interlocked->dst) == DEREF_VALIDATION_NOT_CONSTANT)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Accessed resource from \"%s\" must be determinable at compile time.",
                        interlocked->dst.var->name);
                note_non_static_deref_expressions(ctx, &interlocked->dst, "accessed resource");
            }
            break;
        }
        default:
            break;
    }

    return false;
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
                || (src_type->e.numeric.type == dst_type->e.numeric.type
                && hlsl_is_vec1(src_type) && hlsl_is_vec1(dst_type)))
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

struct stream_append_ctx
{
    struct hlsl_ir_function_decl *func;
    bool created;
};

static bool lower_stream_appends(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct stream_append_ctx *append_ctx = context;
    struct hlsl_ir_resource_store *store;
    const struct hlsl_ir_node *rhs;
    const struct hlsl_type *type;
    struct hlsl_ir_var *var;
    struct hlsl_block block;

    if (instr->type != HLSL_IR_RESOURCE_STORE)
        return false;

    store = hlsl_ir_resource_store(instr);
    if (store->store_type != HLSL_RESOURCE_STREAM_APPEND)
        return false;

    rhs = store->value.node;
    var = store->resource.var;
    type = hlsl_get_stream_output_type(var->data_type);

    if (rhs->type != HLSL_IR_LOAD)
    {
        hlsl_fixme(ctx, &instr->loc, "Stream append rhs is not HLSL_IR_LOAD. Broadcast may be missing.");
        return false;
    }

    VKD3D_ASSERT(var->regs[HLSL_REGSET_STREAM_OUTPUTS].allocated);

    if (var->regs[HLSL_REGSET_STREAM_OUTPUTS].index)
    {
        hlsl_fixme(ctx, &instr->loc, "Append to an output stream with a nonzero stream index.");
        return false;
    }

    hlsl_block_init(&block);

    append_output_copy_recurse(ctx, &block, append_ctx->func, type->e.so.type, hlsl_ir_load(rhs), var->storage_modifiers,
            &var->semantic, var->semantic.index, false, !append_ctx->created);
    append_ctx->created = true;

    list_move_before(&instr->entry, &block.instrs);
    hlsl_src_remove(&store->value);

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
    element_type = hlsl_get_vector_type(ctx, type->e.numeric.type, hlsl_type_minor_size(type));

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

    if (src_type->class <= HLSL_CLASS_VECTOR && dst_type->class <= HLSL_CLASS_VECTOR
            && dst_type->e.numeric.dimx < src_type->e.numeric.dimx)
    {
        struct hlsl_ir_node *new_cast;

        dst_vector_type = hlsl_get_vector_type(ctx, dst_type->e.numeric.type, src_type->e.numeric.dimx);
        /* We need to preserve the cast since it might be doing more than just
         * narrowing the vector. */
        new_cast = hlsl_block_add_cast(ctx, block, cast->operands[0].node, dst_vector_type, &cast->node.loc);
        hlsl_block_add_swizzle(ctx, block, HLSL_SWIZZLE(X, Y, Z, W),
                dst_type->e.numeric.dimx, new_cast, &cast->node.loc);
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
        uint32_t combined_swizzle;

        combined_swizzle = hlsl_combine_swizzles(hlsl_ir_swizzle(next_instr)->u.vector,
                swizzle->u.vector, instr->data_type->e.numeric.dimx);
        next_instr = hlsl_ir_swizzle(next_instr)->val.node;

        if (!(new_swizzle = hlsl_new_swizzle(ctx, combined_swizzle,
                instr->data_type->e.numeric.dimx, next_instr, &instr->loc)))
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

    if (instr->data_type->e.numeric.dimx != swizzle->val.node->data_type->e.numeric.dimx)
        return false;

    for (i = 0; i < instr->data_type->e.numeric.dimx; ++i)
        if (hlsl_swizzle_get_component(swizzle->u.vector, i) != i)
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
                terminal_break = (hlsl_ir_jump(node)->type == HLSL_IR_JUMP_BREAK);
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
        if (!(def = hlsl_new_switch_case(ctx, 0, true, NULL, &s->node.loc)))
            return true;
        hlsl_block_add_jump(ctx, &def->body, HLSL_IR_JUMP_BREAK, NULL, &s->node.loc);
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
    VKD3D_ASSERT(deref->var);

    if (deref->path_len == 0)
        return false;

    type = deref->var->data_type;
    for (i = 0; i < deref->path_len - 1; ++i)
        type = hlsl_get_element_type_from_path_index(ctx, type, deref->path[i].node);

    idx = deref->path[deref->path_len - 1].node;

    if (type->class == HLSL_CLASS_VECTOR && idx->type != HLSL_IR_CONSTANT)
    {
        struct hlsl_ir_node *eq, *swizzle, *c, *operands[HLSL_MAX_OPERANDS] = {0};
        unsigned int width = type->e.numeric.dimx;
        struct hlsl_constant_value value;
        struct hlsl_ir_load *vector_load;
        enum hlsl_ir_expr_op op;

        if (!(vector_load = hlsl_new_load_parent(ctx, deref, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, &vector_load->node);

        swizzle = hlsl_block_add_swizzle(ctx, block, HLSL_SWIZZLE(X, X, X, X), width, idx, &instr->loc);

        value.u[0].u = 0;
        value.u[1].u = 1;
        value.u[2].u = 2;
        value.u[3].u = 3;
        if (!(c = hlsl_new_constant(ctx, hlsl_get_vector_type(ctx, HLSL_TYPE_UINT, width), &value, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, c);

        operands[0] = swizzle;
        operands[1] = c;
        eq = hlsl_block_add_expr(ctx, block, HLSL_OP2_EQUAL, operands,
                hlsl_get_vector_type(ctx, HLSL_TYPE_BOOL, width), &instr->loc);
        eq = hlsl_block_add_cast(ctx, block, eq, type, &instr->loc);

        op = HLSL_OP2_DOT;
        if (width == 1)
            op = type->e.numeric.type == HLSL_TYPE_BOOL ? HLSL_OP2_LOGIC_AND : HLSL_OP2_MUL;

        /* Note: We may be creating a DOT for bool vectors here, which we need to lower to
         * LOGIC_OR + LOGIC_AND. */
        operands[0] = &vector_load->node;
        operands[1] = eq;
        hlsl_block_add_expr(ctx, block, op, operands, instr->data_type, &instr->loc);
        return true;
    }

    return false;
}

static bool validate_nonconstant_vector_store_derefs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *idx;
    struct hlsl_deref *deref;
    struct hlsl_type *type;
    unsigned int i;

    if (instr->type != HLSL_IR_STORE)
        return false;

    deref = &hlsl_ir_store(instr)->lhs;
    VKD3D_ASSERT(deref->var);

    if (deref->path_len == 0)
        return false;

    type = deref->var->data_type;
    for (i = 0; i < deref->path_len - 1; ++i)
        type = hlsl_get_element_type_from_path_index(ctx, type, deref->path[i].node);

    idx = deref->path[deref->path_len - 1].node;

    if (type->class == HLSL_CLASS_VECTOR && idx->type != HLSL_IR_CONSTANT)
    {
        /* We should turn this into an hlsl_error after we implement unrolling, because if we get
         * here after that, it means that the HLSL is invalid. */
        hlsl_fixme(ctx, &instr->loc, "Non-constant vector addressing on store. Unrolling may be missing.");
    }

    return false;
}

static bool deref_supports_sm1_indirect_addressing(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    return ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX && deref->var->is_uniform;
}

/* This pass flattens array (and row_major matrix) loads that include the indexing of a non-constant
 * index into multiple constant loads, where the value of only one of them ends up in the resulting
 * node.
 * This is achieved through a synthetic variable. The non-constant index is compared for equality
 * with every possible value it can have within the array bounds, and the ternary operator is used
 * to update the value of the synthetic var when the equality check passes. */
static bool lower_nonconstant_array_loads(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr,
        struct hlsl_block *block)
{
    struct hlsl_constant_value zero_value = {0};
    struct hlsl_ir_node *cut_index, *zero;
    unsigned int i, i_cut, element_count;
    const struct hlsl_deref *deref;
    struct hlsl_type *cut_type;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;
    bool row_major;

    if (instr->type != HLSL_IR_LOAD)
        return false;
    load = hlsl_ir_load(instr);
    deref = &load->src;

    if (deref->path_len == 0)
        return false;

    if (deref_supports_sm1_indirect_addressing(ctx, deref))
        return false;

    for (i = deref->path_len - 1; ; --i)
    {
        if (deref->path[i].node->type != HLSL_IR_CONSTANT)
        {
            i_cut = i;
            break;
        }

        if (i == 0)
            return false;
    }

    cut_index = deref->path[i_cut].node;
    cut_type = deref->var->data_type;
    for (i = 0; i < i_cut; ++i)
        cut_type = hlsl_get_element_type_from_path_index(ctx, cut_type, deref->path[i].node);

    row_major = hlsl_type_is_row_major(cut_type);
    VKD3D_ASSERT(cut_type->class == HLSL_CLASS_ARRAY || row_major);

    if (!(var = hlsl_new_synthetic_var(ctx, row_major ? "row_major-load" : "array-load", instr->data_type, &instr->loc)))
        return false;

    if (!(zero = hlsl_new_constant(ctx, instr->data_type, &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, zero);

    hlsl_block_add_simple_store(ctx, block, var, zero);

    TRACE("Lowering non-constant %s load on variable '%s'.\n", row_major ? "row_major" : "array", deref->var->name);

    element_count = hlsl_type_element_count(cut_type);
    for (i = 0; i < element_count; ++i)
    {
        struct hlsl_ir_node *const_i, *equals, *ternary, *specific_load, *var_load;
        struct hlsl_type *btype = hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL);
        struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {0};
        struct hlsl_deref deref_copy = {0};

        const_i = hlsl_block_add_uint_constant(ctx, block, i, &cut_index->loc);

        operands[0] = cut_index;
        operands[1] = const_i;
        equals = hlsl_block_add_expr(ctx, block, HLSL_OP2_EQUAL, operands, btype, &cut_index->loc);
        equals = hlsl_block_add_swizzle(ctx, block, HLSL_SWIZZLE(X, X, X, X),
                var->data_type->e.numeric.dimx, equals, &cut_index->loc);

        var_load = hlsl_block_add_simple_load(ctx, block, var, &cut_index->loc);

        if (!hlsl_copy_deref(ctx, &deref_copy, deref))
            return false;
        hlsl_src_remove(&deref_copy.path[i_cut]);
        hlsl_src_from_node(&deref_copy.path[i_cut], const_i);
        specific_load = hlsl_block_add_load_index(ctx, block, &deref_copy, NULL, &cut_index->loc);
        hlsl_cleanup_deref(&deref_copy);

        operands[0] = equals;
        operands[1] = specific_load;
        operands[2] = var_load;
        ternary = hlsl_block_add_expr(ctx, block, HLSL_OP3_TERNARY, operands, instr->data_type, &cut_index->loc);

        hlsl_block_add_simple_store(ctx, block, var, ternary);
    }

    hlsl_block_add_simple_load(ctx, block, var, &instr->loc);
    return true;
}

static struct hlsl_type *clone_texture_array_as_combined_sampler_array(struct hlsl_ctx *ctx, struct hlsl_type *type)
{
    struct hlsl_type *sampler_type;

    if (type->class == HLSL_CLASS_ARRAY)
    {
        if (!(sampler_type = clone_texture_array_as_combined_sampler_array(ctx, type->e.array.type)))
            return NULL;

        return hlsl_new_array_type(ctx, sampler_type, type->e.array.elements_count, HLSL_ARRAY_GENERIC);
    }

    return ctx->builtin_types.sampler[type->sampler_dim];
}

static bool deref_offset_is_zero(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);
    unsigned int index;

    if (!hlsl_regset_index_from_deref(ctx, deref, regset, &index))
        return false;
    return index == 0;
}

/* Lower samples from separate texture and sampler variables to samples from
 * synthetized combined samplers. That is, translate SM4-style samples in the
 * source to SM1-style samples in the bytecode. */
static bool lower_separate_samples(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_var *var, *resource, *sampler;
    struct hlsl_ir_resource_load *load;
    struct vkd3d_string_buffer *name;
    struct hlsl_type *sampler_type;

    if (instr->type != HLSL_IR_RESOURCE_LOAD)
        return false;
    load = hlsl_ir_resource_load(instr);

    if (load->load_type != HLSL_RESOURCE_SAMPLE
            && load->load_type != HLSL_RESOURCE_SAMPLE_GRAD
            && load->load_type != HLSL_RESOURCE_SAMPLE_LOD
            && load->load_type != HLSL_RESOURCE_SAMPLE_LOD_BIAS)
        return false;

    if (!load->sampler.var)
        return false;
    resource = load->resource.var;
    sampler = load->sampler.var;

    VKD3D_ASSERT(hlsl_type_is_resource(resource->data_type));
    VKD3D_ASSERT(hlsl_type_is_resource(sampler->data_type));
    if (sampler->data_type->class == HLSL_CLASS_ARRAY && !deref_offset_is_zero(ctx, &load->sampler))
    {
        /* Not supported by d3dcompiler. */
        hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NOT_IMPLEMENTED,
                "Lower separated samples with sampler arrays.");
        return false;
    }
    if (!resource->is_uniform)
        return false;
    if(!sampler->is_uniform)
        return false;

    if (!(name = hlsl_get_string_buffer(ctx)))
        return false;
    vkd3d_string_buffer_printf(name, "%s+%s", sampler->name, resource->name);

    if (load->texel_offset.node)
    {
        hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                "Texel offsets are not supported on profiles lower than 4.0.\n");
        return false;
    }

    TRACE("Lowering to combined sampler %s.\n", debugstr_a(name->buffer));

    if (!(var = hlsl_get_var(ctx->globals, name->buffer)))
    {
        if (!(sampler_type = clone_texture_array_as_combined_sampler_array(ctx, resource->data_type)))
        {
            hlsl_release_string_buffer(ctx, name);
            return false;
        }

        if (!(var = hlsl_new_synthetic_var_named(ctx, name->buffer, sampler_type, &instr->loc, false)))
        {
            hlsl_release_string_buffer(ctx, name);
            return false;
        }
        var->storage_modifiers |= HLSL_STORAGE_UNIFORM;
        var->is_combined_sampler = true;
        var->is_uniform = 1;

        list_remove(&var->scope_entry);
        list_add_after(&sampler->scope_entry, &var->scope_entry);

        list_add_after(&sampler->extern_entry, &var->extern_entry);
    }
    hlsl_release_string_buffer(ctx, name);

    /* Only change the deref's var, keep the path. */
    load->resource.var = var;
    hlsl_cleanup_deref(&load->sampler);
    load->sampler.var = NULL;

    return true;
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
        case HLSL_RESOURCE_GATHER_CMP_RED:
        case HLSL_RESOURCE_GATHER_CMP_GREEN:
        case HLSL_RESOURCE_GATHER_CMP_BLUE:
        case HLSL_RESOURCE_GATHER_CMP_ALPHA:
        case HLSL_RESOURCE_RESINFO:
        case HLSL_RESOURCE_SAMPLE_CMP:
        case HLSL_RESOURCE_SAMPLE_CMP_LZ:
        case HLSL_RESOURCE_SAMPLE_INFO:
            return false;

        case HLSL_RESOURCE_SAMPLE:
        case HLSL_RESOURCE_SAMPLE_GRAD:
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

    VKD3D_ASSERT(hlsl_deref_get_regset(ctx, &load->resource) == HLSL_REGSET_SAMPLERS);

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
            VKD3D_ASSERT(arr_type->class == HLSL_CLASS_ARRAY);
            texture_array_type = hlsl_new_array_type(ctx, texture_array_type,
                    arr_type->e.array.elements_count, HLSL_ARRAY_GENERIC);
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
    VKD3D_ASSERT(hlsl_deref_get_type(ctx, &load->resource)->class == HLSL_CLASS_TEXTURE);
    VKD3D_ASSERT(hlsl_deref_get_type(ctx, &load->sampler)->class == HLSL_CLASS_SAMPLER);

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

static bool sort_synthetic_combined_samplers_first(struct hlsl_ctx *ctx)
{
    struct list separated_resources;
    struct hlsl_ir_var *var, *next;

    list_init(&separated_resources);

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_combined_sampler)
        {
            list_remove(&var->extern_entry);
            insert_ensuring_decreasing_bind_count(&separated_resources, var, HLSL_REGSET_SAMPLERS);
        }
    }

    list_move_head(&ctx->extern_vars, &separated_resources);

    return false;
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

/* Turn CAST to int or uint into TRUNC + REINTERPRET */
static bool lower_casts_to_int(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 };
    struct hlsl_ir_node *arg, *trunc;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_CAST)
        return false;

    arg = expr->operands[0].node;
    if (!hlsl_type_is_integer(instr->data_type) || instr->data_type->e.numeric.type == HLSL_TYPE_BOOL)
        return false;
    if (!hlsl_type_is_floating_point(arg->data_type))
        return false;

    trunc = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_TRUNC, arg, &instr->loc);

    memset(operands, 0, sizeof(operands));
    operands[0] = trunc;
    hlsl_block_add_expr(ctx, block, HLSL_OP1_REINTERPRET, operands, instr->data_type, &instr->loc);

    return true;
}

/* Turn TRUNC into:
 *
 *     TRUNC(x) = x - FRACT(x) + extra
 *
 * where
 *
 *      extra = FRACT(x) > 0 && x < 0
 *
 * where the comparisons in the extra term are performed using CMP or SLT
 * depending on whether this is a pixel or vertex shader, respectively.
 */
static bool lower_trunc(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *res;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_TRUNC)
        return false;

    arg = expr->operands[0].node;

    if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        struct hlsl_ir_node *fract, *neg_fract, *has_fract, *floor, *extra, *zero, *one;
        struct hlsl_constant_value zero_value, one_value;

        memset(&zero_value, 0, sizeof(zero_value));
        if (!(zero = hlsl_new_constant(ctx, arg->data_type, &zero_value, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, zero);

        one_value.u[0].f = 1.0;
        one_value.u[1].f = 1.0;
        one_value.u[2].f = 1.0;
        one_value.u[3].f = 1.0;
        if (!(one = hlsl_new_constant(ctx, arg->data_type, &one_value, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, one);

        fract = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_FRACT, arg, &instr->loc);
        neg_fract = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, fract, &instr->loc);

        if (!(has_fract = hlsl_new_ternary_expr(ctx, HLSL_OP3_CMP, neg_fract, zero, one)))
            return false;
        hlsl_block_add_instr(block, has_fract);

        if (!(extra = hlsl_new_ternary_expr(ctx, HLSL_OP3_CMP, arg, zero, has_fract)))
            return false;
        hlsl_block_add_instr(block, extra);

        floor = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, arg, neg_fract);
        res = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, floor, extra);
    }
    else
    {
        struct hlsl_ir_node *neg_arg, *is_neg, *fract, *neg_fract, *has_fract, *floor;

        neg_arg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, arg, &instr->loc);
        is_neg = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_SLT, arg, neg_arg);
        fract = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_FRACT, arg, &instr->loc);
        neg_fract = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, fract, &instr->loc);
        has_fract = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_SLT, neg_fract, fract);
        floor = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, arg, neg_fract);

        if (!(res = hlsl_new_ternary_expr(ctx, HLSL_OP3_MAD, is_neg, has_fract, floor)))
            return false;
        hlsl_block_add_instr(block, res);
    }

    return true;
}

/* Lower modulus using:
 *
 *     mod(x, y) = x - trunc(x / y) * y;
 *
 */
static bool lower_int_modulus_sm1(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *div, *trunc, *mul, *neg, *operands[2], *ret;
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;
    bool is_float;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP2_MOD)
        return false;

    is_float = instr->data_type->e.numeric.type == HLSL_TYPE_FLOAT
            || instr->data_type->e.numeric.type == HLSL_TYPE_HALF;
    if (is_float)
        return false;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->e.numeric.dimx);

    for (unsigned int i = 0; i < 2; ++i)
    {
        operands[i] = hlsl_block_add_cast(ctx, block, expr->operands[i].node, float_type, &instr->loc);
    }

    div = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_DIV, operands[0], operands[1]);
    trunc = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_TRUNC, div, &instr->loc);
    mul = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, trunc, operands[1]);
    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, mul, &instr->loc);
    ret = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, operands[0], neg);
    hlsl_block_add_cast(ctx, block, ret, instr->data_type, &instr->loc);

    return true;
}

/* Lower DIV to RCP + MUL. */
static bool lower_division(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *rcp, *ret, *operands[2];
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;
    bool is_float;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP2_DIV)
        return false;

    is_float = instr->data_type->e.numeric.type == HLSL_TYPE_FLOAT
            || instr->data_type->e.numeric.type == HLSL_TYPE_HALF;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->e.numeric.dimx);

    for (unsigned int i = 0; i < 2; ++i)
    {
        operands[i] = expr->operands[i].node;
        if (!is_float)
            operands[i] = hlsl_block_add_cast(ctx, block, operands[i], float_type, &instr->loc);
    }

    rcp = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_RCP, operands[1], &instr->loc);
    ret = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, operands[0], rcp);
    if (!is_float)
        ret = hlsl_block_add_cast(ctx, block, ret, instr->data_type, &instr->loc);

    return true;
}

/* Lower SQRT to RSQ + RCP. */
static bool lower_sqrt(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_node *rsq;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_SQRT)
        return false;

    rsq = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_RSQ, expr->operands[0].node, &instr->loc);
    hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_RCP, rsq, &instr->loc);
    return true;
}

/* Lower DP2 to MUL + ADD */
static bool lower_dot(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *mul, *add_x, *add_y;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_DOT)
        return false;
    if (arg1->data_type->e.numeric.dimx != 2)
        return false;

    if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 };

        operands[0] = arg1;
        operands[1] = arg2;
        operands[2] = hlsl_block_add_float_constant(ctx, block, 0.0f, &expr->node.loc);

        hlsl_block_add_expr(ctx, block, HLSL_OP3_DP2ADD, operands, instr->data_type, &expr->node.loc);
    }
    else
    {
        mul = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, expr->operands[0].node, expr->operands[1].node);

        add_x = hlsl_block_add_swizzle(ctx, block, HLSL_SWIZZLE(X, X, X, X),
                instr->data_type->e.numeric.dimx, mul, &expr->node.loc);
        add_y = hlsl_block_add_swizzle(ctx, block, HLSL_SWIZZLE(Y, Y, Y, Y),
                instr->data_type->e.numeric.dimx, mul, &expr->node.loc);
        hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, add_x, add_y);
    }

    return true;
}

/* Lower ABS to MAX */
static bool lower_abs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_ABS)
        return false;

    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, arg, &instr->loc);
    hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MAX, neg, arg);
    return true;
}

/* Lower ROUND using FRC, ROUND(x) -> ((x + 0.5) - FRC(x + 0.5)). */
static bool lower_round(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *sum, *frc, *half;
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

    sum = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, arg, half);
    frc = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_FRACT, sum, &instr->loc);
    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, frc, &instr->loc);
    hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, sum, neg);
    return true;
}

/* Lower CEIL to FRC */
static bool lower_ceil(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *frc;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_CEIL)
        return false;

    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, arg, &instr->loc);
    frc = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_FRACT, neg, &instr->loc);
    hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, frc, arg);
    return true;
}

/* Lower FLOOR to FRC */
static bool lower_floor(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *frc;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_FLOOR)
        return false;

    frc = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_FRACT, arg, &instr->loc);
    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, frc, &instr->loc);
    hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, neg, arg);
    return true;
}

/* Lower SIN/COS to SINCOS for SM1.  */
static bool lower_trig(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *half, *two_pi, *reciprocal_two_pi, *neg_pi;
    struct hlsl_constant_value half_value, two_pi_value, reciprocal_two_pi_value, neg_pi_value;
    struct hlsl_ir_node *mad, *frc, *reduced;
    struct hlsl_type *type;
    struct hlsl_ir_expr *expr;
    enum hlsl_ir_expr_op op;
    struct hlsl_ir_node *sincos;
    int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);

    if (expr->op == HLSL_OP1_SIN)
        op = HLSL_OP1_SIN_REDUCED;
    else if (expr->op == HLSL_OP1_COS)
        op = HLSL_OP1_COS_REDUCED;
    else
        return false;

    arg = expr->operands[0].node;
    type = arg->data_type;

    /* Reduce the range of the input angles to [-pi, pi]. */
    for (i = 0; i < type->e.numeric.dimx; ++i)
    {
        half_value.u[i].f = 0.5;
        two_pi_value.u[i].f = 2.0 * M_PI;
        reciprocal_two_pi_value.u[i].f = 1.0 / (2.0 * M_PI);
        neg_pi_value.u[i].f = -M_PI;
    }

    if (!(half = hlsl_new_constant(ctx, type, &half_value, &instr->loc))
            || !(two_pi = hlsl_new_constant(ctx, type, &two_pi_value, &instr->loc))
            || !(reciprocal_two_pi = hlsl_new_constant(ctx, type, &reciprocal_two_pi_value, &instr->loc))
            || !(neg_pi = hlsl_new_constant(ctx, type, &neg_pi_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, half);
    hlsl_block_add_instr(block, two_pi);
    hlsl_block_add_instr(block, reciprocal_two_pi);
    hlsl_block_add_instr(block, neg_pi);

    if (!(mad = hlsl_new_ternary_expr(ctx, HLSL_OP3_MAD, arg, reciprocal_two_pi, half)))
        return false;
    hlsl_block_add_instr(block, mad);
    frc = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_FRACT, mad, &instr->loc);
    if (!(reduced = hlsl_new_ternary_expr(ctx, HLSL_OP3_MAD, frc, two_pi, neg_pi)))
        return false;
    hlsl_block_add_instr(block, reduced);

    if (type->e.numeric.dimx == 1)
    {
        sincos = hlsl_block_add_unary_expr(ctx, block, op, reduced, &instr->loc);
    }
    else
    {
        struct hlsl_ir_node *comps[4] = {0};
        struct hlsl_ir_var *var;
        struct hlsl_deref var_deref;

        for (i = 0; i < type->e.numeric.dimx; ++i)
        {
            uint32_t s = hlsl_swizzle_from_writemask(1 << i);

            comps[i] = hlsl_block_add_swizzle(ctx, block, s, 1, reduced, &instr->loc);
        }

        if (!(var = hlsl_new_synthetic_var(ctx, "sincos", type, &instr->loc)))
            return false;
        hlsl_init_simple_deref_from_var(&var_deref, var);

        for (i = 0; i < type->e.numeric.dimx; ++i)
        {
            sincos = hlsl_block_add_unary_expr(ctx, block, op, comps[i], &instr->loc);
            hlsl_block_add_store_component(ctx, block, &var_deref, i, sincos);
        }

        hlsl_block_add_load_index(ctx, block, &var_deref, NULL, &instr->loc);
    }

    return true;
}

static bool lower_logic_not(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *arg_cast, *neg, *one, *sub;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS];
    struct hlsl_constant_value one_value;
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_LOGIC_NOT)
        return false;

    arg = expr->operands[0].node;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, arg->data_type->e.numeric.dimx);

    /* If this is happens, it means we failed to cast the argument to boolean somewhere. */
    VKD3D_ASSERT(arg->data_type->e.numeric.type == HLSL_TYPE_BOOL);

    arg_cast = hlsl_block_add_cast(ctx, block, arg, float_type, &arg->loc);

    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, arg_cast, &instr->loc);

    one_value.u[0].f = 1.0;
    one_value.u[1].f = 1.0;
    one_value.u[2].f = 1.0;
    one_value.u[3].f = 1.0;
    if (!(one = hlsl_new_constant(ctx, float_type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    sub = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, one, neg);

    memset(operands, 0, sizeof(operands));
    operands[0] = sub;
    hlsl_block_add_expr(ctx, block, HLSL_OP1_REINTERPRET, operands, instr->data_type, &instr->loc);
    return true;
}

/* Lower TERNARY to CMP for SM1. */
static bool lower_ternary(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *cond, *first, *second, *float_cond, *neg;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {0};
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

    if (cond->data_type->class > HLSL_CLASS_VECTOR || instr->data_type->class > HLSL_CLASS_VECTOR)
    {
        hlsl_fixme(ctx, &instr->loc, "Lower ternary of type other than scalar or vector.");
        return false;
    }

    VKD3D_ASSERT(cond->data_type->e.numeric.type == HLSL_TYPE_BOOL);

    type = hlsl_get_numeric_type(ctx, instr->data_type->class, HLSL_TYPE_FLOAT,
            instr->data_type->e.numeric.dimx, instr->data_type->e.numeric.dimy);
    float_cond = hlsl_block_add_cast(ctx, block, cond, type, &instr->loc);
    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, float_cond, &instr->loc);

    memset(operands, 0, sizeof(operands));
    operands[0] = neg;
    operands[1] = second;
    operands[2] = first;
    hlsl_block_add_expr(ctx, block, HLSL_OP3_CMP, operands, first->data_type, &instr->loc);
    return true;
}

static bool lower_resource_load_bias(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_node *swizzle, *store;
    struct hlsl_ir_resource_load *load;
    struct hlsl_ir_load *tmp_load;
    struct hlsl_ir_var *tmp_var;
    struct hlsl_deref deref;

    if (instr->type != HLSL_IR_RESOURCE_LOAD)
        return false;
    load = hlsl_ir_resource_load(instr);
    if (load->load_type != HLSL_RESOURCE_SAMPLE_LOD
            && load->load_type != HLSL_RESOURCE_SAMPLE_LOD_BIAS)
        return false;

    if (!load->lod.node)
        return false;

    if (!(tmp_var = hlsl_new_synthetic_var(ctx, "coords-with-lod",
            hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4), &instr->loc)))
        return false;

    if (!(swizzle = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, X, X, X), 4, load->lod.node, &load->lod.node->loc)))
        return false;
    list_add_before(&instr->entry, &swizzle->entry);

    if (!(store = hlsl_new_simple_store(ctx, tmp_var, swizzle)))
        return false;
    list_add_before(&instr->entry, &store->entry);

    hlsl_init_simple_deref_from_var(&deref, tmp_var);
    if (!(store = hlsl_new_store_index(ctx, &deref, NULL, load->coords.node, 0, &instr->loc)))
        return false;
    list_add_before(&instr->entry, &store->entry);

    if (!(tmp_load = hlsl_new_var_load(ctx, tmp_var, &instr->loc)))
        return false;
    list_add_before(&instr->entry, &tmp_load->node.entry);

    hlsl_src_remove(&load->coords);
    hlsl_src_from_node(&load->coords, &tmp_load->node);
    hlsl_src_remove(&load->lod);
    return true;
}

static bool lower_comparison_operators(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr,
        struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg1_cast, *arg2, *arg2_cast, *slt, *res;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS];
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;
    bool negate = false;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP2_EQUAL && expr->op != HLSL_OP2_NEQUAL && expr->op != HLSL_OP2_LESS
            && expr->op != HLSL_OP2_GEQUAL)
        return false;

    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->e.numeric.dimx);

    arg1_cast = hlsl_block_add_cast(ctx, block, arg1, float_type, &instr->loc);
    arg2_cast = hlsl_block_add_cast(ctx, block, arg2, float_type, &instr->loc);

    switch (expr->op)
    {
        case HLSL_OP2_EQUAL:
        case HLSL_OP2_NEQUAL:
        {
            struct hlsl_ir_node *neg, *sub, *abs, *abs_neg;

            neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, arg2_cast, &instr->loc);
            sub = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, arg1_cast, neg);

            if (ctx->profile->major_version >= 3)
            {
                abs = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_ABS, sub, &instr->loc);
            }
            else
            {
                /* Use MUL as a precarious ABS. */
                abs = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, sub, sub);
            }

            abs_neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, abs, &instr->loc);
            slt = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_SLT, abs_neg, abs);
            negate = (expr->op == HLSL_OP2_EQUAL);
            break;
        }

        case HLSL_OP2_GEQUAL:
        case HLSL_OP2_LESS:
        {
            slt = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_SLT, arg1_cast, arg2_cast);
            negate = (expr->op == HLSL_OP2_GEQUAL);
            break;
        }

        default:
            vkd3d_unreachable();
    }

    if (negate)
    {
        struct hlsl_constant_value one_value;
        struct hlsl_ir_node *one, *slt_neg;

        one_value.u[0].f = 1.0;
        one_value.u[1].f = 1.0;
        one_value.u[2].f = 1.0;
        one_value.u[3].f = 1.0;
        if (!(one = hlsl_new_constant(ctx, float_type, &one_value, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, one);

        slt_neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, slt, &instr->loc);
        res = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, one, slt_neg);
    }
    else
    {
        res = slt;
    }

    /* We need a REINTERPRET so that the HLSL IR code is valid. SLT and its arguments must be FLOAT,
     * and casts to BOOL have already been lowered to "!= 0". */
    memset(operands, 0, sizeof(operands));
    operands[0] = res;
    hlsl_block_add_expr(ctx, block, HLSL_OP1_REINTERPRET, operands, instr->data_type, &instr->loc);
    return true;
}

/* Intended to be used for SM1-SM3, lowers SLT instructions (only available in vertex shaders) to
 * CMP instructions (only available in pixel shaders).
 * Based on the following equivalence:
 *     SLT(x, y)
 *     = (x < y) ? 1.0 : 0.0
 *     = ((x - y) >= 0) ? 0.0 : 1.0
 *     = CMP(x - y, 0.0, 1.0)
 */
static bool lower_slt(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *arg1_cast, *arg2_cast, *neg, *sub, *zero, *one, *cmp;
    struct hlsl_constant_value zero_value, one_value;
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP2_SLT)
        return false;

    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->e.numeric.dimx);

    arg1_cast = hlsl_block_add_cast(ctx, block, arg1, float_type, &instr->loc);
    arg2_cast = hlsl_block_add_cast(ctx, block, arg2, float_type, &instr->loc);
    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, arg2_cast, &instr->loc);
    sub = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, arg1_cast, neg);

    memset(&zero_value, 0, sizeof(zero_value));
    if (!(zero = hlsl_new_constant(ctx, float_type, &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, zero);

    one_value.u[0].f = 1.0;
    one_value.u[1].f = 1.0;
    one_value.u[2].f = 1.0;
    one_value.u[3].f = 1.0;
    if (!(one = hlsl_new_constant(ctx, float_type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    if (!(cmp = hlsl_new_ternary_expr(ctx, HLSL_OP3_CMP, sub, zero, one)))
        return false;
    hlsl_block_add_instr(block, cmp);

    return true;
}

/* Intended to be used for SM1-SM3, lowers CMP instructions (only available in pixel shaders) to
 * SLT instructions (only available in vertex shaders).
 * Based on the following equivalence:
 *     CMP(x, y, z)
 *     = (x >= 0) ? y : z
 *     = z * ((x < 0) ? 1.0 : 0.0) + y * ((x < 0) ? 0.0 : 1.0)
 *     = z * SLT(x, 0.0) + y * (1 - SLT(x, 0.0))
 */
static bool lower_cmp(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *args[3], *args_cast[3], *slt, *neg_slt, *sub, *zero, *one, *mul1, *mul2;
    struct hlsl_constant_value zero_value, one_value;
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP3_CMP)
        return false;

    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->e.numeric.dimx);

    for (i = 0; i < 3; ++i)
    {
        args[i] = expr->operands[i].node;
        args_cast[i] = hlsl_block_add_cast(ctx, block, args[i], float_type, &instr->loc);
    }

    memset(&zero_value, 0, sizeof(zero_value));
    if (!(zero = hlsl_new_constant(ctx, float_type, &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, zero);

    one_value.u[0].f = 1.0;
    one_value.u[1].f = 1.0;
    one_value.u[2].f = 1.0;
    one_value.u[3].f = 1.0;
    if (!(one = hlsl_new_constant(ctx, float_type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    slt = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_SLT, args_cast[0], zero);
    mul1 = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, args_cast[2], slt);
    neg_slt = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, slt, &instr->loc);
    sub = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, one, neg_slt);
    mul2 = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, args_cast[1], sub);
    hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_ADD, mul1, mul2);
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
    if (type->e.numeric.type != HLSL_TYPE_BOOL)
        return false;

    /* Narrowing casts should have already been lowered. */
    VKD3D_ASSERT(type->e.numeric.dimx == arg_type->e.numeric.dimx);

    zero = hlsl_new_constant(ctx, arg_type, &zero_value, &instr->loc);
    if (!zero)
        return false;
    hlsl_block_add_instr(block, zero);

    neq = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_NEQUAL, expr->operands[0].node, zero);
    neq->data_type = expr->node.data_type;

    return true;
}

struct hlsl_ir_node *hlsl_add_conditional(struct hlsl_ctx *ctx, struct hlsl_block *instrs,
        struct hlsl_ir_node *condition, struct hlsl_ir_node *if_true, struct hlsl_ir_node *if_false)
{
    struct hlsl_type *cond_type = condition->data_type;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS];

    VKD3D_ASSERT(hlsl_types_are_equal(if_true->data_type, if_false->data_type));

    if (cond_type->e.numeric.type != HLSL_TYPE_BOOL)
    {
        cond_type = hlsl_get_numeric_type(ctx, cond_type->class, HLSL_TYPE_BOOL,
                cond_type->e.numeric.dimx, cond_type->e.numeric.dimy);
        condition = hlsl_block_add_cast(ctx, instrs, condition, cond_type, &condition->loc);
    }

    operands[0] = condition;
    operands[1] = if_true;
    operands[2] = if_false;
    return hlsl_block_add_expr(ctx, instrs, HLSL_OP3_TERNARY, operands, if_true->data_type, &condition->loc);
}

static bool lower_int_division_sm4(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
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
    if (type->e.numeric.type != HLSL_TYPE_INT)
        return false;
    utype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_UINT, type->e.numeric.dimx, type->e.numeric.dimy);

    xor = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_BIT_XOR, arg1, arg2);

    for (i = 0; i < type->e.numeric.dimx; ++i)
        high_bit_value.u[i].u = 0x80000000;
    if (!(high_bit = hlsl_new_constant(ctx, type, &high_bit_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, high_bit);

    and = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_BIT_AND, xor, high_bit);
    abs1 = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_ABS, arg1, &instr->loc);
    cast1 = hlsl_block_add_cast(ctx, block, abs1, utype, &instr->loc);
    abs2 = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_ABS, arg2, &instr->loc);
    cast2 = hlsl_block_add_cast(ctx, block, abs2, utype, &instr->loc);
    div = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_DIV, cast1, cast2);
    cast3 = hlsl_block_add_cast(ctx, block, div, type, &instr->loc);
    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, cast3, &instr->loc);
    return hlsl_add_conditional(ctx, block, and, neg, cast3);
}

static bool lower_int_modulus_sm4(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
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
    if (type->e.numeric.type != HLSL_TYPE_INT)
        return false;
    utype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_UINT, type->e.numeric.dimx, type->e.numeric.dimy);

    for (i = 0; i < type->e.numeric.dimx; ++i)
        high_bit_value.u[i].u = 0x80000000;
    if (!(high_bit = hlsl_new_constant(ctx, type, &high_bit_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, high_bit);

    and = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_BIT_AND, arg1, high_bit);
    abs1 = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_ABS, arg1, &instr->loc);
    cast1 = hlsl_block_add_cast(ctx, block, abs1, utype, &instr->loc);
    abs2 = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_ABS, arg2, &instr->loc);
    cast2 = hlsl_block_add_cast(ctx, block, abs2, utype, &instr->loc);
    div = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MOD, cast1, cast2);
    cast3 = hlsl_block_add_cast(ctx, block, div, type, &instr->loc);
    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, cast3, &instr->loc);
    return hlsl_add_conditional(ctx, block, and, neg, cast3);
}

static bool lower_int_abs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_type *type = instr->data_type;
    struct hlsl_ir_node *arg, *neg;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);

    if (expr->op != HLSL_OP1_ABS)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->e.numeric.type != HLSL_TYPE_INT)
        return false;

    arg = expr->operands[0].node;

    neg = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, arg, &instr->loc);
    hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MAX, arg, neg);
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

    if (hlsl_type_is_integer(type))
    {
        arg1 = expr->operands[0].node;
        arg2 = expr->operands[1].node;
        VKD3D_ASSERT(arg1->data_type->e.numeric.dimx == arg2->data_type->e.numeric.dimx);
        dimx = arg1->data_type->e.numeric.dimx;
        is_bool = type->e.numeric.type == HLSL_TYPE_BOOL;

        mult = hlsl_block_add_binary_expr(ctx, block, is_bool ? HLSL_OP2_LOGIC_AND : HLSL_OP2_MUL, arg1, arg2);

        for (i = 0; i < dimx; ++i)
        {
            uint32_t s = hlsl_swizzle_from_writemask(1 << i);

            comps[i] = hlsl_block_add_swizzle(ctx, block, s, 1, mult, &instr->loc);
        }

        res = comps[0];
        for (i = 1; i < dimx; ++i)
            res = hlsl_block_add_binary_expr(ctx, block, is_bool ? HLSL_OP2_LOGIC_OR : HLSL_OP2_ADD, res, comps[i]);

        return true;
    }

    return false;
}

static bool lower_float_modulus(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *mul1, *neg1, *ge, *neg2, *div, *mul2, *frc, *cond, *one;
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
    if (type->e.numeric.type != HLSL_TYPE_FLOAT)
        return false;
    btype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_BOOL, type->e.numeric.dimx, type->e.numeric.dimy);

    mul1 = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, arg2, arg1);
    neg1 = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, mul1, &instr->loc);

    ge = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_GEQUAL, mul1, neg1);
    ge->data_type = btype;

    neg2 = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_NEG, arg2, &instr->loc);
    cond = hlsl_add_conditional(ctx, block, ge, arg2, neg2);

    for (i = 0; i < type->e.numeric.dimx; ++i)
        one_value.u[i].f = 1.0f;
    if (!(one = hlsl_new_constant(ctx, type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    div = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_DIV, one, cond);
    mul2 = hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, div, arg1);
    frc = hlsl_block_add_unary_expr(ctx, block, HLSL_OP1_FRACT, mul2, &instr->loc);
    hlsl_block_add_binary_expr(ctx, block, HLSL_OP2_MUL, frc, cond);
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
    cmp_type = hlsl_get_numeric_type(ctx, arg_type->class, HLSL_TYPE_BOOL,
            arg_type->e.numeric.dimx, arg_type->e.numeric.dimy);
    cmp = hlsl_block_add_expr(ctx, &block, HLSL_OP2_LESS, operands, cmp_type, &instr->loc);

    if (!(bool_false = hlsl_new_constant(ctx, hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL), &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, bool_false);

    or = bool_false;

    count = hlsl_type_component_count(cmp_type);
    for (i = 0; i < count; ++i)
    {
        load = hlsl_add_load_component(ctx, &block, cmp, i, &instr->loc);
        or = hlsl_block_add_binary_expr(ctx, &block, HLSL_OP2_LOGIC_OR, or, load);
    }

    list_move_tail(&instr->entry, &block.instrs);
    hlsl_src_remove(&jump->condition);
    hlsl_src_from_node(&jump->condition, or);
    jump->type = HLSL_IR_JUMP_DISCARD_NZ;

    return true;
}

static bool lower_discard_nz(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_node *cond, *cond_cast, *abs, *neg;
    struct hlsl_type *float_type;
    struct hlsl_ir_jump *jump;
    struct hlsl_block block;

    if (instr->type != HLSL_IR_JUMP)
        return false;
    jump = hlsl_ir_jump(instr);
    if (jump->type != HLSL_IR_JUMP_DISCARD_NZ)
        return false;

    cond = jump->condition.node;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, cond->data_type->e.numeric.dimx);

    hlsl_block_init(&block);

    cond_cast = hlsl_block_add_cast(ctx, &block, cond, float_type, &instr->loc);
    abs = hlsl_block_add_unary_expr(ctx, &block, HLSL_OP1_ABS, cond_cast, &instr->loc);
    neg = hlsl_block_add_unary_expr(ctx, &block, HLSL_OP1_NEG, abs, &instr->loc);

    list_move_tail(&instr->entry, &block.instrs);
    hlsl_src_remove(&jump->condition);
    hlsl_src_from_node(&jump->condition, neg);
    jump->type = HLSL_IR_JUMP_DISCARD_NEG;

    return true;
}

static bool dce(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    switch (instr->type)
    {
        case HLSL_IR_CONSTANT:
        case HLSL_IR_COMPILE:
        case HLSL_IR_EXPR:
        case HLSL_IR_INDEX:
        case HLSL_IR_LOAD:
        case HLSL_IR_RESOURCE_LOAD:
        case HLSL_IR_STRING_CONSTANT:
        case HLSL_IR_SWIZZLE:
        case HLSL_IR_SAMPLER_STATE:
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

            if (var->is_output_semantic)
                break;

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
        case HLSL_IR_INTERLOCKED:
        case HLSL_IR_JUMP:
        case HLSL_IR_LOOP:
        case HLSL_IR_RESOURCE_STORE:
        case HLSL_IR_SWITCH:
        case HLSL_IR_SYNC:
            break;
        case HLSL_IR_STATEBLOCK_CONSTANT:
            /* Stateblock constants should not appear in the shader program. */
            vkd3d_unreachable();
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

static bool mark_indexable_var(struct hlsl_ctx *ctx, struct hlsl_deref *deref,
        struct hlsl_ir_node *instr)
{
    if (!deref->rel_offset.node)
        return false;

    VKD3D_ASSERT(deref->var);
    VKD3D_ASSERT(deref->rel_offset.node->type != HLSL_IR_CONSTANT);
    deref->var->indexable = true;

    return true;
}

static void mark_indexable_vars(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
            var->indexable = false;
    }

    transform_derefs(ctx, mark_indexable_var, &entry_func->body);
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
        case HLSL_REGSET_STREAM_OUTPUTS:
            return 'm';
        case HLSL_REGSET_NUMERIC:
            vkd3d_unreachable();
    }
    vkd3d_unreachable();
}

static void allocate_register_reservations(struct hlsl_ctx *ctx, struct list *extern_vars)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, extern_vars, struct hlsl_ir_var, extern_entry)
    {
        const struct hlsl_reg_reservation *reservation = &var->reg_reservation;
        unsigned int r;

        if (reservation->reg_type)
        {
            for (r = 0; r <= HLSL_REGSET_LAST_OBJECT; ++r)
            {
                if (var->regs[r].allocation_size > 0)
                {
                    if (reservation->reg_type != get_regset_name(r))
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
                        var->regs[r].space = reservation->reg_space;
                        var->regs[r].index = reservation->reg_index;
                    }
                }
            }
        }
    }
}

static void deref_mark_last_read(struct hlsl_deref *deref, unsigned int last_read)
{
    unsigned int i;

    if (hlsl_deref_is_lowered(deref))
    {
        if (deref->rel_offset.node)
            deref->rel_offset.node->last_read = last_read;
    }
    else
    {
        for (i = 0; i < deref->path_len; ++i)
            deref->path[i].node->last_read = last_read;
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
        case HLSL_IR_STATEBLOCK_CONSTANT:
            /* Stateblock constants should not appear in the shader program. */
            vkd3d_unreachable();

        case HLSL_IR_STORE:
        {
            struct hlsl_ir_store *store = hlsl_ir_store(instr);

            var = store->lhs.var;
            if (!var->first_write)
                var->first_write = loop_first ? min(instr->index, loop_first) : instr->index;
            store->rhs.node->last_read = last_read;
            deref_mark_last_read(&store->lhs, last_read);
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
            deref_mark_last_read(&load->src, last_read);
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
            deref_mark_last_read(&load->resource, last_read);

            if ((var = load->sampler.var))
            {
                var->last_read = max(var->last_read, last_read);
                deref_mark_last_read(&load->sampler, last_read);
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
            deref_mark_last_read(&store->resource, last_read);
            if (store->coords.node)
                store->coords.node->last_read = last_read;
            if (store->value.node)
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
        case HLSL_IR_INTERLOCKED:
        {
            struct hlsl_ir_interlocked *interlocked = hlsl_ir_interlocked(instr);

            var = interlocked->dst.var;
            var->last_read = max(var->last_read, last_read);
            deref_mark_last_read(&interlocked->dst, last_read);
            interlocked->coords.node->last_read = last_read;
            interlocked->value.node->last_read = last_read;
            if (interlocked->cmp_value.node)
                interlocked->cmp_value.node->last_read = last_read;
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
        case HLSL_IR_STRING_CONSTANT:
        case HLSL_IR_SYNC:
            break;
        case HLSL_IR_COMPILE:
        case HLSL_IR_SAMPLER_STATE:
            /* These types are skipped as they are only relevant to effects. */
            break;
        }
    }
}

static void compute_liveness(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    if (ctx->result)
        return;

    index_instructions(&entry_func->body, 1);

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
            var->first_write = var->last_read = 0;
    }

    compute_liveness_recurse(&entry_func->body, 0, 0);
}

static void mark_vars_usage(struct hlsl_ctx *ctx)
{
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            if (var->last_read)
                var->is_read = true;
        }
    }
}

struct register_allocator
{
    struct allocation
    {
        uint32_t reg;
        unsigned int writemask;
        unsigned int first_write, last_read;

        /* Two allocations with different mode can't share the same register. */
        int mode;
        /* If an allocation is VIP, no new allocations can be made in the
         * register unless they are VIP as well. */
        bool vip;
    } *allocations;
    size_t count, capacity;

    /* Indexable temps are allocated separately and always keep their index regardless of their
     * lifetime. */
    uint32_t indexable_count;

    /* Total number of registers allocated so far. Used to declare sm4 temp count. */
    uint32_t reg_count;

    /* Special flag so allocations that can share registers prioritize those
     * that will result in smaller writemasks.
     * For instance, a single-register allocation would prefer to share a register
     * whose .xy components are already allocated (becoming .z) instead of a
     * register whose .xyz components are already allocated (becoming .w). */
    bool prioritize_smaller_writemasks;
};

static unsigned int get_available_writemask(const struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, uint32_t reg_idx, int mode, bool vip)
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
        {
            writemask &= ~allocation->writemask;
            if (allocation->mode != mode)
                writemask = 0;
            if (allocation->vip && !vip)
                writemask = 0;
        }

        if (!writemask)
            break;
    }

    return writemask;
}

static void record_allocation(struct hlsl_ctx *ctx, struct register_allocator *allocator, uint32_t reg_idx,
        unsigned int writemask, unsigned int first_write, unsigned int last_read, int mode, bool vip)
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
    allocation->mode = mode;
    allocation->vip = vip;

    allocator->reg_count = max(allocator->reg_count, reg_idx + 1);
}

/* Allocates a register (or some components of it) within the register allocator.
 * 'reg_size' is the number of register components to be reserved.
 * 'component_count' is the number of components for the hlsl_reg's
 *      writemask, which can be smaller than 'reg_size'. For instance, sm1
 *      floats and vectors allocate the whole register even if they are not
 *      using all components.
 * 'mode' can be provided to avoid allocating on a register that already has an
 *      allocation with a different mode.
 * 'force_align' can be used so that the allocation always start in '.x'.
 * 'vip' can be used so that no new allocations can be made in the given register
 *      unless they are 'vip' as well. */
static struct hlsl_reg allocate_register(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, unsigned int reg_size,
        unsigned int component_count, int mode, bool force_align, bool vip)
{
    struct hlsl_reg ret = {.allocation_size = 1, .allocated = true};
    unsigned int required_size = force_align ? 4 : reg_size;
    unsigned int pref;

    VKD3D_ASSERT(component_count <= reg_size);

    pref = allocator->prioritize_smaller_writemasks ? 4 : required_size;
    for (; pref >= required_size; --pref)
    {
        for (uint32_t reg_idx = 0; reg_idx < allocator->reg_count; ++reg_idx)
        {
            unsigned int available_writemask = get_available_writemask(allocator,
                    first_write, last_read, reg_idx, mode, vip);

            if (vkd3d_popcount(available_writemask) >= pref)
            {
                unsigned int writemask = hlsl_combine_writemasks(available_writemask,
                        vkd3d_write_mask_from_component_count(reg_size));

                ret.id = reg_idx;
                ret.writemask = hlsl_combine_writemasks(writemask,
                        vkd3d_write_mask_from_component_count(component_count));

                record_allocation(ctx, allocator, reg_idx, writemask, first_write, last_read, mode, vip);
                return ret;
            }
        }
    }

    ret.id = allocator->reg_count;
    ret.writemask = vkd3d_write_mask_from_component_count(component_count);
    record_allocation(ctx, allocator, allocator->reg_count,
            vkd3d_write_mask_from_component_count(reg_size), first_write, last_read, mode, vip);
    return ret;
}

/* Allocate a register with writemask, while reserving reg_writemask. */
static struct hlsl_reg allocate_register_with_masks(struct hlsl_ctx *ctx,
        struct register_allocator *allocator, unsigned int first_write, unsigned int last_read,
        uint32_t reg_writemask, uint32_t writemask, int mode, bool vip)
{
    struct hlsl_reg ret = {0};
    uint32_t reg_idx;

    VKD3D_ASSERT((reg_writemask & writemask) == writemask);

    for (reg_idx = 0;; ++reg_idx)
    {
        if ((get_available_writemask(allocator, first_write, last_read,
                reg_idx, mode, vip) & reg_writemask) == reg_writemask)
            break;
    }

    record_allocation(ctx, allocator, reg_idx, reg_writemask, first_write, last_read, mode, vip);

    ret.id = reg_idx;
    ret.allocation_size = 1;
    ret.writemask = writemask;
    ret.allocated = true;
    return ret;
}

static bool is_range_available(const struct register_allocator *allocator, unsigned int first_write,
        unsigned int last_read, uint32_t reg_idx, unsigned int reg_size, int mode, bool vip)
{
    unsigned int last_reg_mask = (1u << (reg_size % 4)) - 1;
    unsigned int writemask;
    uint32_t i;

    for (i = 0; i < (reg_size / 4); ++i)
    {
        writemask = get_available_writemask(allocator, first_write, last_read, reg_idx + i, mode, vip);
        if (writemask != VKD3DSP_WRITEMASK_ALL)
            return false;
    }
    writemask = get_available_writemask(allocator, first_write, last_read, reg_idx + (reg_size / 4), mode, vip);
    if ((writemask & last_reg_mask) != last_reg_mask)
        return false;
    return true;
}

static struct hlsl_reg allocate_range(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, unsigned int reg_size, int mode, bool vip)
{
    struct hlsl_reg ret = {0};
    uint32_t reg_idx;
    unsigned int i;

    for (reg_idx = 0;; ++reg_idx)
    {
        if (is_range_available(allocator, first_write, last_read, reg_idx, reg_size, mode, vip))
            break;
    }

    for (i = 0; i < reg_size / 4; ++i)
        record_allocation(ctx, allocator, reg_idx + i, VKD3DSP_WRITEMASK_ALL, first_write, last_read, mode, vip);
    if (reg_size % 4)
        record_allocation(ctx, allocator, reg_idx + (reg_size / 4),
                (1u << (reg_size % 4)) - 1, first_write, last_read, mode, vip);

    ret.id = reg_idx;
    ret.allocation_size = align(reg_size, 4) / 4;
    ret.allocated = true;
    return ret;
}

static struct hlsl_reg allocate_numeric_registers_for_type(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, const struct hlsl_type *type)
{
    unsigned int reg_size = type->reg_size[HLSL_REGSET_NUMERIC];

    /* FIXME: We could potentially pack structs or arrays more efficiently... */

    if (type->class <= HLSL_CLASS_VECTOR)
        return allocate_register(ctx, allocator, first_write, last_read,
                type->e.numeric.dimx, type->e.numeric.dimx, 0, false, false);
    else
        return allocate_range(ctx, allocator, first_write, last_read, reg_size, 0, false);
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

        VKD3D_ASSERT(!load->sampler.var);

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

static void register_deref_usage(struct hlsl_ctx *ctx, struct hlsl_deref *deref)
{
    struct hlsl_ir_var *var = deref->var;
    enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);
    uint32_t required_bind_count;
    struct hlsl_type *type;
    unsigned int index;

    hlsl_regset_index_from_deref(ctx, deref, regset, &index);

    if (regset <= HLSL_REGSET_LAST_OBJECT)
    {
        var->objects_usage[regset][index].used = true;
        var->bind_count[regset] = max(var->bind_count[regset], index + 1);
    }
    else if (regset == HLSL_REGSET_NUMERIC)
    {
        type = hlsl_deref_get_type(ctx, deref);

        required_bind_count = align(index + type->reg_size[regset], 4) / 4;
        var->bind_count[regset] = max(var->bind_count[regset], required_bind_count);
    }
    else
    {
        vkd3d_unreachable();
    }
}

static bool track_components_usage(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    switch (instr->type)
    {
        case HLSL_IR_LOAD:
        {
            struct hlsl_ir_load *load = hlsl_ir_load(instr);

            if (!load->src.var->is_uniform)
                return false;

            /* These will are handled by validate_static_object_references(). */
            if (hlsl_deref_get_regset(ctx, &load->src) != HLSL_REGSET_NUMERIC)
                return false;

            register_deref_usage(ctx, &load->src);
            break;
        }

        case HLSL_IR_RESOURCE_LOAD:
            register_deref_usage(ctx, &hlsl_ir_resource_load(instr)->resource);
            if (hlsl_ir_resource_load(instr)->sampler.var)
                register_deref_usage(ctx, &hlsl_ir_resource_load(instr)->sampler);
            break;

        case HLSL_IR_RESOURCE_STORE:
            register_deref_usage(ctx, &hlsl_ir_resource_store(instr)->resource);
            break;

        case HLSL_IR_INTERLOCKED:
            register_deref_usage(ctx, &hlsl_ir_interlocked(instr)->dst);
            break;

        default:
            break;
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

static void allocate_instr_temp_register(struct hlsl_ctx *ctx,
        struct hlsl_ir_node *instr, struct register_allocator *allocator)
{
    unsigned int reg_writemask = 0, dst_writemask = 0;

    if (instr->reg.allocated || !instr->last_read)
        return;

    if (instr->type == HLSL_IR_EXPR)
    {
        switch (hlsl_ir_expr(instr)->op)
        {
            case HLSL_OP1_COS_REDUCED:
                dst_writemask = VKD3DSP_WRITEMASK_0;
                reg_writemask = ctx->profile->major_version < 3 ? (1 << 3) - 1 : VKD3DSP_WRITEMASK_0;
                break;

            case HLSL_OP1_SIN_REDUCED:
                dst_writemask = VKD3DSP_WRITEMASK_1;
                reg_writemask = ctx->profile->major_version < 3 ? (1 << 3) - 1 : VKD3DSP_WRITEMASK_1;
                break;

            default:
                break;
        }
    }

    if (reg_writemask)
        instr->reg = allocate_register_with_masks(ctx, allocator, instr->index,
                instr->last_read, reg_writemask, dst_writemask, 0, false);
    else
        instr->reg = allocate_numeric_registers_for_type(ctx, allocator,
                instr->index, instr->last_read, instr->data_type);

    TRACE("Allocated anonymous expression @%u to %s (liveness %u-%u).\n", instr->index,
            debug_register('r', instr->reg, instr->data_type), instr->index, instr->last_read);
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

        allocate_instr_temp_register(ctx, instr, allocator);

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

static bool find_constant(struct hlsl_ctx *ctx, const float *f, unsigned int count, struct hlsl_reg *ret)
{
    struct hlsl_constant_defs *defs = &ctx->constant_defs;

    for (size_t i = 0; i < defs->count; ++i)
    {
        const struct hlsl_constant_register *reg = &defs->regs[i];

        for (size_t j = 0; j <= 4 - count; ++j)
        {
            unsigned int writemask = ((1u << count) - 1) << j;

            if ((reg->allocated_mask & writemask) == writemask
                    && !memcmp(f, &reg->value.f[j], count * sizeof(float)))
            {
                ret->id = reg->index;
                ret->allocation_size = 1;
                ret->writemask = writemask;
                ret->allocated = true;
                return true;
            }
        }
    }

    return false;
}

static void record_constant(struct hlsl_ctx *ctx, unsigned int component_index, float f,
        const struct vkd3d_shader_location *loc)
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
            reg->allocated_mask |= (1u << (component_index % 4));
            return;
        }
    }

    if (!hlsl_array_reserve(ctx, (void **)&defs->regs, &defs->size, defs->count + 1, sizeof(*defs->regs)))
        return;
    reg = &defs->regs[defs->count++];
    memset(reg, 0, sizeof(*reg));
    reg->index = component_index / 4;
    reg->value.f[component_index % 4] = f;
    reg->allocated_mask = (1u << (component_index % 4));
    reg->loc = *loc;
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
                float f[4] = {0};

                VKD3D_ASSERT(hlsl_is_numeric_type(type));
                VKD3D_ASSERT(type->e.numeric.dimy == 1);

                for (unsigned int i = 0; i < type->e.numeric.dimx; ++i)
                {
                    const union hlsl_constant_value_component *value;

                    value = &constant->value.u[i];

                    switch (type->e.numeric.type)
                    {
                        case HLSL_TYPE_BOOL:
                            f[i] = !!value->u;
                            break;

                        case HLSL_TYPE_FLOAT:
                        case HLSL_TYPE_HALF:
                            f[i] = value->f;
                            break;

                        case HLSL_TYPE_INT:
                            f[i] = value->i;
                            break;

                        case HLSL_TYPE_MIN16UINT:
                        case HLSL_TYPE_UINT:
                            f[i] = value->u;
                            break;

                        case HLSL_TYPE_DOUBLE:
                            FIXME("Double constant.\n");
                            return;
                    }
                }

                if (find_constant(ctx, f, type->e.numeric.dimx, &constant->reg))
                {
                    TRACE("Reusing already allocated constant %s for @%u.\n",
                            debug_register('c', constant->reg, type), instr->index);
                    break;
                }

                constant->reg = allocate_numeric_registers_for_type(ctx, allocator, 1, UINT_MAX, type);
                TRACE("Allocated constant @%u to %s.\n", instr->index, debug_register('c', constant->reg, type));

                for (unsigned int x = 0, i = 0; x < 4; ++x)
                {
                    if ((constant->reg.writemask & (1u << x)))
                        record_constant(ctx, constant->reg.id * 4 + x, f[i++], &constant->node.loc);
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

static void sort_uniform_by_bind_count(struct list *sorted, struct hlsl_ir_var *to_sort, enum hlsl_regset regset)
{
    struct hlsl_ir_var *var;

    list_remove(&to_sort->extern_entry);

    LIST_FOR_EACH_ENTRY(var, sorted, struct hlsl_ir_var, extern_entry)
    {
        uint32_t to_sort_size = to_sort->bind_count[regset];
        uint32_t var_size = var->bind_count[regset];

        if (to_sort_size > var_size)
        {
            list_add_before(&var->extern_entry, &to_sort->extern_entry);
            return;
        }
    }

    list_add_tail(sorted, &to_sort->extern_entry);
}

static void sort_uniforms_by_bind_count(struct hlsl_ctx *ctx, enum hlsl_regset regset)
{
    struct list sorted = LIST_INIT(sorted);
    struct hlsl_ir_var *var, *next;

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_uniform)
            sort_uniform_by_bind_count(&sorted, var, regset);
    }
    list_move_tail(&ctx->extern_vars, &sorted);
}

/* In SM2, 'sincos' expects specific constants as src1 and src2 arguments.
 * These have to be referenced directly, i.e. as 'c' not 'r'. */
static void allocate_sincos_const_registers(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct register_allocator *allocator)
{
    const struct hlsl_ir_node *instr;
    struct hlsl_type *type;

    if (ctx->profile->major_version >= 3)
        return;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_EXPR && (hlsl_ir_expr(instr)->op == HLSL_OP1_SIN_REDUCED
                || hlsl_ir_expr(instr)->op == HLSL_OP1_COS_REDUCED))
        {
            type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4);

            ctx->d3dsincosconst1 = allocate_numeric_registers_for_type(ctx, allocator, 1, UINT_MAX, type);
            TRACE("Allocated D3DSINCOSCONST1 to %s.\n", debug_register('c', ctx->d3dsincosconst1, type));
            record_constant(ctx, ctx->d3dsincosconst1.id * 4 + 0, -1.55009923e-06f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst1.id * 4 + 1, -2.17013894e-05f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst1.id * 4 + 2,  2.60416674e-03f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst1.id * 4 + 3,  2.60416680e-04f, &instr->loc);

            ctx->d3dsincosconst2 = allocate_numeric_registers_for_type(ctx, allocator, 1, UINT_MAX, type);
            TRACE("Allocated D3DSINCOSCONST2 to %s.\n", debug_register('c', ctx->d3dsincosconst2, type));
            record_constant(ctx, ctx->d3dsincosconst2.id * 4 + 0, -2.08333340e-02f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst2.id * 4 + 1, -1.25000000e-01f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst2.id * 4 + 2,  1.00000000e+00f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst2.id * 4 + 3,  5.00000000e-01f, &instr->loc);

            return;
        }
    }
}

static void allocate_const_registers(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct register_allocator allocator_used = {0};
    struct register_allocator allocator = {0};
    struct hlsl_ir_var *var;

    sort_uniforms_by_bind_count(ctx, HLSL_REGSET_NUMERIC);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];
        unsigned int bind_count = var->bind_count[HLSL_REGSET_NUMERIC];

        if (!var->is_uniform || reg_size == 0)
            continue;

        if (var->reg_reservation.reg_type == 'c')
        {
            unsigned int reg_idx = var->reg_reservation.reg_index;
            unsigned int i;

            VKD3D_ASSERT(reg_size % 4 == 0);
            for (i = 0; i < reg_size / 4; ++i)
            {
                if (i < bind_count)
                {
                    if (get_available_writemask(&allocator_used, 1, UINT_MAX,
                            reg_idx + i, 0, false) != VKD3DSP_WRITEMASK_ALL)
                    {
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                                "Overlapping register() reservations on 'c%u'.", reg_idx + i);
                    }
                    record_allocation(ctx, &allocator_used, reg_idx + i, VKD3DSP_WRITEMASK_ALL, 1, UINT_MAX, 0, false);
                }
                record_allocation(ctx, &allocator, reg_idx + i, VKD3DSP_WRITEMASK_ALL, 1, UINT_MAX, 0, false);
            }

            var->regs[HLSL_REGSET_NUMERIC].id = reg_idx;
            var->regs[HLSL_REGSET_NUMERIC].allocation_size = reg_size / 4;
            var->regs[HLSL_REGSET_NUMERIC].writemask = VKD3DSP_WRITEMASK_ALL;
            var->regs[HLSL_REGSET_NUMERIC].allocated = true;
            TRACE("Allocated reserved %s to %s.\n", var->name,
                    debug_register('c', var->regs[HLSL_REGSET_NUMERIC], var->data_type));
        }
    }

    vkd3d_free(allocator_used.allocations);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int alloc_size = 4 * var->bind_count[HLSL_REGSET_NUMERIC];

        if (!var->is_uniform || alloc_size == 0)
            continue;

        if (!var->regs[HLSL_REGSET_NUMERIC].allocated)
        {
            var->regs[HLSL_REGSET_NUMERIC] = allocate_range(ctx, &allocator, 1, UINT_MAX, alloc_size, 0, false);
            TRACE("Allocated %s to %s.\n", var->name,
                    debug_register('c', var->regs[HLSL_REGSET_NUMERIC], var->data_type));
        }
    }

    allocate_const_registers_recurse(ctx, &entry_func->body, &allocator);

    allocate_sincos_const_registers(ctx, &entry_func->body, &allocator);

    vkd3d_free(allocator.allocations);
}

/* Simple greedy temporary register allocation pass that just assigns a unique
 * index to all (simultaneously live) variables or intermediate values. Agnostic
 * as to how many registers are actually available for the current backend, and
 * does not handle constants. */
static uint32_t allocate_temp_registers(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct register_allocator allocator = {0};
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    /* Reset variable temp register allocations. */
    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            if (!(var->is_input_semantic || var->is_output_semantic || var->is_uniform))
                memset(var->regs, 0, sizeof(var->regs));
        }
    }

    /* ps_1_* outputs are special and go in temp register 0. */
    if (ctx->profile->major_version == 1 && ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        LIST_FOR_EACH_ENTRY(var, &entry_func->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            if (var->is_output_semantic)
            {
                record_allocation(ctx, &allocator, 0, VKD3DSP_WRITEMASK_ALL,
                        var->first_write, UINT_MAX, 0, false);
                break;
            }
        }
    }

    allocate_temp_registers_recurse(ctx, &entry_func->body, &allocator);
    vkd3d_free(allocator.allocations);

    if (allocator.indexable_count)
        TRACE("Declaration of function \"%s\" required %u temp registers, and %u indexable temps.\n",
                entry_func->func->name, allocator.reg_count, allocator.indexable_count);
    else
        TRACE("Declaration of function \"%s\" required %u temp registers.\n",
                entry_func->func->name, allocator.reg_count);

    return allocator.reg_count;
}

static enum vkd3d_shader_interpolation_mode sm4_get_interpolation_mode(struct hlsl_type *type,
        unsigned int storage_modifiers)
{
    unsigned int i;

    static const struct
    {
        unsigned int modifiers;
        enum vkd3d_shader_interpolation_mode mode;
    }
    modes[] =
    {
        {HLSL_STORAGE_CENTROID | HLSL_STORAGE_NOPERSPECTIVE, VKD3DSIM_LINEAR_NOPERSPECTIVE_CENTROID},
        {HLSL_STORAGE_NOPERSPECTIVE, VKD3DSIM_LINEAR_NOPERSPECTIVE},
        {HLSL_STORAGE_CENTROID, VKD3DSIM_LINEAR_CENTROID},
        {HLSL_STORAGE_CENTROID | HLSL_STORAGE_LINEAR, VKD3DSIM_LINEAR_CENTROID},
    };

    if (hlsl_type_is_primitive_array(type))
        type = type->e.array.type;

    VKD3D_ASSERT(hlsl_is_numeric_type(type));

    if ((storage_modifiers & HLSL_STORAGE_NOINTERPOLATION)
            || base_type_get_semantic_equivalent(type->e.numeric.type) == HLSL_TYPE_UINT)
        return VKD3DSIM_CONSTANT;

    for (i = 0; i < ARRAY_SIZE(modes); ++i)
    {
        if ((storage_modifiers & modes[i].modifiers) == modes[i].modifiers)
            return modes[i].mode;
    }

    return VKD3DSIM_LINEAR;
}

static void allocate_semantic_register(struct hlsl_ctx *ctx, struct hlsl_ir_var *var,
        struct register_allocator *allocator, bool output, bool optimize)
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

    bool is_primitive = hlsl_type_is_primitive_array(var->data_type);
    enum vkd3d_shader_register_type type;
    struct vkd3d_shader_version version;
    bool special_interpolation = false;
    bool vip_allocation = false;
    uint32_t reg;
    bool builtin;

    VKD3D_ASSERT(var->semantic.name);

    version.major = ctx->profile->major_version;
    version.minor = ctx->profile->minor_version;
    version.type = ctx->profile->type;

    if (version.major < 4)
    {
        enum vkd3d_decl_usage usage;
        uint32_t usage_idx;

        /* ps_1_* outputs are special and go in temp register 0. */
        if (version.major == 1 && output && version.type == VKD3D_SHADER_TYPE_PIXEL)
            return;

        builtin = sm1_register_from_semantic_name(&version,
                var->semantic.name, var->semantic.index, output, NULL, &type, &reg);
        if (!builtin && !sm1_usage_from_semantic_name(var->semantic.name, var->semantic.index, &usage, &usage_idx))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "Invalid semantic '%s'.", var->semantic.name);
            return;
        }

        if ((!output && !var->last_read) || (output && !var->first_write))
            return;
    }
    else
    {
        enum vkd3d_shader_sysval_semantic semantic;
        bool has_idx;

        if (!sm4_sysval_semantic_from_semantic_name(&semantic, &version, ctx->semantic_compat_mapping, ctx->domain,
                var->semantic.name, var->semantic.index, output, ctx->is_patch_constant_func, is_primitive))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "Invalid semantic '%s'.", var->semantic.name);
            return;
        }

        if ((builtin = sm4_register_from_semantic_name(&version, var->semantic.name, output, &type, &has_idx)))
            reg = has_idx ? var->semantic.index : 0;

        if (semantic == VKD3D_SHADER_SV_TESS_FACTOR_TRIINT)
        {
            /* While SV_InsideTessFactor can be declared as 'float' for "tri"
             * domains, it is allocated as if it was 'float[1]'. */
            var->force_align = true;
        }

        if (semantic == VKD3D_SHADER_SV_RENDER_TARGET_ARRAY_INDEX
                || semantic == VKD3D_SHADER_SV_VIEWPORT_ARRAY_INDEX
                || semantic == VKD3D_SHADER_SV_PRIMITIVE_ID)
            vip_allocation = true;

        if (semantic == VKD3D_SHADER_SV_IS_FRONT_FACE || semantic == VKD3D_SHADER_SV_SAMPLE_INDEX
                || (version.type == VKD3D_SHADER_TYPE_DOMAIN && !output && !is_primitive)
                || (ctx->is_patch_constant_func && output))
            special_interpolation = true;
    }

    if (builtin)
    {
        TRACE("%s %s semantic %s[%u] matches predefined register %#x[%u].\n", shader_names[version.type],
                output ? "output" : "input", var->semantic.name, var->semantic.index, type, reg);
    }
    else
    {
        unsigned int component_count = is_primitive
                ? var->data_type->e.array.type->e.numeric.dimx : var->data_type->e.numeric.dimx;
        int mode = (ctx->profile->major_version < 4)
                ? 0 : sm4_get_interpolation_mode(var->data_type, var->storage_modifiers);
        unsigned int reg_size = optimize ? component_count : 4;

        if (special_interpolation)
            mode = VKD3DSIM_NONE;

        var->regs[HLSL_REGSET_NUMERIC] = allocate_register(ctx, allocator, 1, UINT_MAX,
                reg_size, component_count, mode, var->force_align, vip_allocation);

        TRACE("Allocated %s to %s (mode %d).\n", var->name, debug_register(output ? 'o' : 'v',
                var->regs[HLSL_REGSET_NUMERIC], var->data_type), mode);
    }
}

static void allocate_semantic_registers(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func,
        uint32_t *output_reg_count)
{
    struct register_allocator in_prim_allocator = {0}, patch_constant_out_patch_allocator = {0};
    struct register_allocator input_allocator = {0}, output_allocator = {0};
    bool is_vertex_shader = ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX;
    bool is_pixel_shader = ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL;
    struct hlsl_ir_var *var;

    in_prim_allocator.prioritize_smaller_writemasks = true;
    patch_constant_out_patch_allocator.prioritize_smaller_writemasks = true;
    input_allocator.prioritize_smaller_writemasks = true;
    output_allocator.prioritize_smaller_writemasks = true;

    LIST_FOR_EACH_ENTRY(var, &entry_func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_input_semantic)
        {
            if (hlsl_type_is_primitive_array(var->data_type))
            {
                bool is_patch_constant_output_patch = ctx->is_patch_constant_func &&
                        var->data_type->e.array.array_type == HLSL_ARRAY_PATCH_OUTPUT;

                if (is_patch_constant_output_patch)
                    allocate_semantic_register(ctx, var, &patch_constant_out_patch_allocator, false,
                            !is_vertex_shader);
                else
                    allocate_semantic_register(ctx, var, &in_prim_allocator, false,
                            !is_vertex_shader);
            }
            else
                allocate_semantic_register(ctx, var, &input_allocator, false, !is_vertex_shader);
        }

        if (var->is_output_semantic)
            allocate_semantic_register(ctx, var, &output_allocator, true, !is_pixel_shader);
    }

    *output_reg_count = output_allocator.reg_count;

    vkd3d_free(in_prim_allocator.allocations);
    vkd3d_free(patch_constant_out_patch_allocator.allocations);
    vkd3d_free(input_allocator.allocations);
    vkd3d_free(output_allocator.allocations);
}

static const struct hlsl_buffer *get_reserved_buffer(struct hlsl_ctx *ctx,
        uint32_t space, uint32_t index, bool allocated_only)
{
    const struct hlsl_buffer *buffer;

    LIST_FOR_EACH_ENTRY(buffer, &ctx->buffers, const struct hlsl_buffer, entry)
    {
        if (buffer->reservation.reg_type == 'b'
                && buffer->reservation.reg_space == space && buffer->reservation.reg_index == index)
        {
            if (allocated_only && !buffer->reg.allocated)
                continue;

            return buffer;
        }
    }
    return NULL;
}

static void hlsl_calculate_buffer_offset(struct hlsl_ctx *ctx, struct hlsl_ir_var *var, bool register_reservation)
{
    unsigned int var_reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];
    enum hlsl_type_class var_class = var->data_type->class;
    struct hlsl_buffer *buffer = var->buffer;

    if (register_reservation)
    {
        var->buffer_offset = 4 * var->reg_reservation.reg_index;
        var->has_explicit_bind_point = 1;
    }
    else
    {
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
            var->has_explicit_bind_point = 1;
        }
        else
        {
            var->buffer_offset = hlsl_type_get_sm4_offset(var->data_type, buffer->size);
        }
    }

    TRACE("Allocated buffer offset %u to %s.\n", var->buffer_offset, var->name);
    buffer->size = max(buffer->size, var->buffer_offset + var_reg_size);
    if (var->is_read)
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
                || var1->reg_reservation.reg_type == 's'
                || var1->reg_reservation.reg_type == 't'
                || var1->reg_reservation.reg_type == 'u')
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

void hlsl_calculate_buffer_offsets(struct hlsl_ctx *ctx)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->is_uniform || hlsl_type_is_resource(var->data_type))
            continue;

        if (hlsl_var_has_buffer_offset_register_reservation(ctx, var))
            hlsl_calculate_buffer_offset(ctx, var, true);
    }

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->is_uniform || hlsl_type_is_resource(var->data_type))
            continue;

        if (!hlsl_var_has_buffer_offset_register_reservation(ctx, var))
            hlsl_calculate_buffer_offset(ctx, var, false);
    }
}

static unsigned int get_max_cbuffer_reg_index(struct hlsl_ctx *ctx)
{
    if (hlsl_version_ge(ctx, 5, 1))
        return UINT_MAX;

    return 13;
}

static void allocate_buffers(struct hlsl_ctx *ctx)
{
    struct hlsl_buffer *buffer;
    uint32_t index = 0, id = 0;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->is_uniform || hlsl_type_is_resource(var->data_type))
            continue;

        if (var->is_param)
            var->buffer = ctx->params_buffer;
    }

    hlsl_calculate_buffer_offsets(ctx);
    validate_buffer_offsets(ctx);

    LIST_FOR_EACH_ENTRY(buffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (!buffer->used_size)
            continue;

        if (buffer->type == HLSL_BUFFER_CONSTANT)
        {
            const struct hlsl_reg_reservation *reservation = &buffer->reservation;

            if (reservation->reg_type == 'b')
            {
                const struct hlsl_buffer *allocated_buffer = get_reserved_buffer(ctx,
                        reservation->reg_space, reservation->reg_index, true);
                unsigned int max_index = get_max_cbuffer_reg_index(ctx);

                if (buffer->reservation.reg_index > max_index)
                    hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                            "Buffer reservation cb%u exceeds target's maximum (cb%u).",
                            buffer->reservation.reg_index, max_index);

                if (allocated_buffer && allocated_buffer != buffer)
                {
                    hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                            "Multiple buffers bound to space %u, index %u.",
                            reservation->reg_space, reservation->reg_index);
                    hlsl_note(ctx, &allocated_buffer->loc, VKD3D_SHADER_LOG_ERROR,
                            "Buffer %s is already bound to space %u, index %u.",
                            allocated_buffer->name, reservation->reg_space, reservation->reg_index);
                }

                buffer->reg.space = reservation->reg_space;
                buffer->reg.index = reservation->reg_index;
                if (hlsl_version_ge(ctx, 5, 1))
                    buffer->reg.id = id++;
                else
                    buffer->reg.id = buffer->reg.index;
                buffer->reg.allocation_size = 1;
                buffer->reg.allocated = true;
                TRACE("Allocated reserved %s to space %u, index %u, id %u.\n",
                        buffer->name, buffer->reg.space, buffer->reg.index, buffer->reg.id);
            }
            else if (!reservation->reg_type)
            {
                unsigned int max_index = get_max_cbuffer_reg_index(ctx);
                while (get_reserved_buffer(ctx, 0, index, false))
                    ++index;

                if (index > max_index)
                    hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "Too many buffers reserved, target's maximum is %u.", max_index);

                buffer->reg.space = 0;
                buffer->reg.index = index;
                if (hlsl_version_ge(ctx, 5, 1))
                    buffer->reg.id = id++;
                else
                    buffer->reg.id = buffer->reg.index;
                buffer->reg.allocation_size = 1;
                buffer->reg.allocated = true;
                TRACE("Allocated %s to space 0, index %u, id %u.\n", buffer->name, buffer->reg.index, buffer->reg.id);
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
        uint32_t space, uint32_t index, bool allocated_only)
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

            if (var->reg_reservation.reg_space != space)
                continue;

            if (!var->regs[regset].allocated && allocated_only)
                continue;
        }
        else if (var->regs[regset].allocated)
        {
            if (var->regs[regset].space != space)
                continue;

            start = var->regs[regset].index;
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

static void allocate_objects(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func, enum hlsl_regset regset)
{
    char regset_name = get_regset_name(regset);
    uint32_t min_index = 0, id = 0;
    struct hlsl_ir_var *var;

    if (regset == HLSL_REGSET_UAVS && ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        LIST_FOR_EACH_ENTRY(var, &func->extern_vars, struct hlsl_ir_var, extern_entry)
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
            unsigned int i;

            if (var->regs[regset].index < min_index)
            {
                VKD3D_ASSERT(regset == HLSL_REGSET_UAVS);
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                        "UAV index (%u) must be higher than the maximum render target index (%u).",
                        var->regs[regset].index, min_index - 1);
                continue;
            }

            for (i = 0; i < count; ++i)
            {
                unsigned int space = var->regs[regset].space;
                unsigned int index = var->regs[regset].index + i;

                /* get_allocated_object() may return "var" itself, but we
                 * actually want that, otherwise we'll end up reporting the
                 * same conflict between the same two variables twice. */
                reserved_object = get_allocated_object(ctx, regset, space, index, true);
                if (reserved_object && reserved_object != var && reserved_object != last_reported)
                {
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                            "Multiple variables bound to space %u, %c%u.", regset_name, space, index);
                    hlsl_note(ctx, &reserved_object->loc, VKD3D_SHADER_LOG_ERROR,
                            "Variable '%s' is already bound to space %u, %c%u.",
                            reserved_object->name, regset_name, space, index);
                    last_reported = reserved_object;
                }
            }

            if (hlsl_version_ge(ctx, 5, 1))
                var->regs[regset].id = id++;
            else
                var->regs[regset].id = var->regs[regset].index;
            TRACE("Allocated reserved variable %s to space %u, indices %c%u-%c%u, id %u.\n",
                    var->name, var->regs[regset].space, regset_name, var->regs[regset].index,
                    regset_name, var->regs[regset].index + count, var->regs[regset].id);
        }
        else
        {
            unsigned int index = min_index;
            unsigned int available = 0;

            while (available < count)
            {
                if (get_allocated_object(ctx, regset, 0, index, false))
                    available = 0;
                else
                    ++available;
                ++index;
            }
            index -= count;

            var->regs[regset].space = 0;
            var->regs[regset].index = index;
            if (hlsl_version_ge(ctx, 5, 1))
                var->regs[regset].id = id++;
            else
                var->regs[regset].id = var->regs[regset].index;
            var->regs[regset].allocated = true;
            TRACE("Allocated variable %s to space 0, indices %c%u-%c%u, id %u.\n", var->name,
                    regset_name, index, regset_name, index + count, var->regs[regset].id);
            ++index;
        }
    }
}

static void allocate_stream_outputs(struct hlsl_ctx *ctx)
{
    struct hlsl_ir_var *var;
    uint32_t index = 0;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->data_type->reg_size[HLSL_REGSET_STREAM_OUTPUTS])
            continue;

        /* We should have ensured that all stream output objects are single-element. */
        VKD3D_ASSERT(var->data_type->reg_size[HLSL_REGSET_STREAM_OUTPUTS] == 1);

        var->regs[HLSL_REGSET_STREAM_OUTPUTS].space = 0;
        var->regs[HLSL_REGSET_STREAM_OUTPUTS].index = index;
        var->regs[HLSL_REGSET_STREAM_OUTPUTS].id = index;
        var->regs[HLSL_REGSET_STREAM_OUTPUTS].allocated = true;

        ++index;
    }
}

bool hlsl_component_index_range_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        unsigned int *start, unsigned int *count)
{
    struct hlsl_type *type = deref->var->data_type;
    unsigned int i;

    *start = 0;
    *count = 0;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        unsigned int index;

        VKD3D_ASSERT(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            return false;

        /* We should always have generated a cast to UINT. */
        VKD3D_ASSERT(hlsl_is_vec1(path_node->data_type) && path_node->data_type->e.numeric.type == HLSL_TYPE_UINT);

        if (!component_index_from_deref_path_node(path_node, type, &index))
            return false;
        *start += index;

        type = hlsl_get_element_type_from_path_index(ctx, type, path_node);
    }

    *count = hlsl_type_component_count(type);
    return true;
}

/* Retrieves true if the index is constant, and false otherwise. In the latter case, the maximum
 * possible index is retrieved, assuming there is not out-of-bounds access. */
bool hlsl_regset_index_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        enum hlsl_regset regset, unsigned int *index)
{
    struct hlsl_type *type = deref->var->data_type;
    bool index_is_constant = true;
    unsigned int i;

    *index = 0;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        unsigned int idx = 0;

        VKD3D_ASSERT(path_node);
        if (path_node->type == HLSL_IR_CONSTANT)
        {
            /* We should always have generated a cast to UINT. */
            VKD3D_ASSERT(hlsl_is_vec1(path_node->data_type) && path_node->data_type->e.numeric.type == HLSL_TYPE_UINT);

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

                case HLSL_CLASS_MATRIX:
                    *index += 4 * idx;
                    break;

                default:
                    vkd3d_unreachable();
            }
        }
        else
        {
            index_is_constant = false;

            switch (type->class)
            {
                case HLSL_CLASS_ARRAY:
                    idx = type->e.array.elements_count - 1;
                    *index += idx * type->e.array.type->reg_size[regset];
                    break;

                case HLSL_CLASS_MATRIX:
                    idx = hlsl_type_major_size(type) - 1;
                    *index += idx * 4;
                    break;

                default:
                    vkd3d_unreachable();
            }
        }

        type = hlsl_get_element_type_from_path_index(ctx, type, path_node);
    }

    VKD3D_ASSERT(!(regset <= HLSL_REGSET_LAST_OBJECT) || (type->reg_size[regset] == 1));
    VKD3D_ASSERT(!(regset == HLSL_REGSET_NUMERIC) || type->reg_size[regset] <= 4);
    return index_is_constant;
}

bool hlsl_offset_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref, unsigned int *offset)
{
    enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);
    struct hlsl_ir_node *offset_node = deref->rel_offset.node;
    unsigned int size;

    *offset = deref->const_offset;

    if (hlsl_type_is_primitive_array(deref->var->data_type))
        return false;

    if (offset_node)
    {
        /* We should always have generated a cast to UINT. */
        VKD3D_ASSERT(hlsl_is_vec1(offset_node->data_type) && offset_node->data_type->e.numeric.type == HLSL_TYPE_UINT);
        VKD3D_ASSERT(offset_node->type != HLSL_IR_CONSTANT);
        return false;
    }

    size = deref->var->data_type->reg_size[regset];
    if (*offset >= size)
    {
        /* FIXME: Report a more specific location for the constant deref. */
        hlsl_error(ctx, &deref->var->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
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

    if (deref->rel_offset.node)
        hlsl_fixme(ctx, &deref->rel_offset.node->loc, "Dereference with non-constant offset of type %s.",
                hlsl_node_type_to_string(deref->rel_offset.node->type));

    return 0;
}

struct hlsl_reg hlsl_reg_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    const struct hlsl_ir_var *var = deref->var;
    struct hlsl_reg ret = var->regs[HLSL_REGSET_NUMERIC];
    unsigned int offset = 0;

    VKD3D_ASSERT(deref->data_type);
    VKD3D_ASSERT(hlsl_is_numeric_type(deref->data_type));

    if (!hlsl_type_is_primitive_array(deref->var->data_type))
        offset = hlsl_offset_from_deref_safe(ctx, deref);

    ret.index += offset / 4;
    ret.id += offset / 4;

    ret.writemask = 0xf & (0xf << (offset % 4));
    if (var->regs[HLSL_REGSET_NUMERIC].writemask)
        ret.writemask = hlsl_combine_writemasks(var->regs[HLSL_REGSET_NUMERIC].writemask, ret.writemask);

    return ret;
}

static bool get_integral_argument_value(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr,
        unsigned int i, int *value)
{
    const struct hlsl_ir_node *instr = attr->args[i].node;
    const struct hlsl_type *type = instr->data_type;

    if (type->class != HLSL_CLASS_SCALAR
            || (type->e.numeric.type != HLSL_TYPE_INT && type->e.numeric.type != HLSL_TYPE_UINT))
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, type)))
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Unexpected type for argument %u of [%s]: expected int or uint, but got %s.",
                    i, attr->name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (instr->type != HLSL_IR_CONSTANT)
    {
        hlsl_fixme(ctx, &instr->loc, "Non-constant expression in [%s] initializer.", attr->name);
        return false;
    }

    *value = hlsl_ir_constant(instr)->value.u[0].i;
    return true;
}

static const char *get_string_argument_value(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr, unsigned int i)
{
    const struct hlsl_ir_node *instr = attr->args[i].node;
    const struct hlsl_type *type = instr->data_type;

    if (type->class != HLSL_CLASS_STRING)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, type)))
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for the argument %u of [%s]: expected string, but got %s.",
                    i, attr->name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return NULL;
    }

    return hlsl_ir_string_constant(instr)->string;
}

static void parse_numthreads_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    static const unsigned int limits[3] = {1024, 1024, 64};
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
        int value;

        if (!get_integral_argument_value(ctx, attr, i, &value))
            return;

        if (value < 1 || value > limits[i])
            hlsl_error(ctx, &attr->args[i].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_THREAD_COUNT,
                    "Dimension %u of the thread count must be between 1 and %u.", i, limits[i]);

        ctx->thread_count[i] = value;
    }

    if (ctx->thread_count[0] * ctx->thread_count[1] * ctx->thread_count[2] > 1024)
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_THREAD_COUNT,
                "Product of thread count parameters cannot exceed 1024.");
}

static void parse_domain_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const char *value;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [domain] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!(value = get_string_argument_value(ctx, attr, 0)))
        return;

    if (!strcmp(value, "isoline"))
        ctx->domain = VKD3D_TESSELLATOR_DOMAIN_LINE;
    else if (!strcmp(value, "tri"))
        ctx->domain = VKD3D_TESSELLATOR_DOMAIN_TRIANGLE;
    else if (!strcmp(value, "quad"))
        ctx->domain = VKD3D_TESSELLATOR_DOMAIN_QUAD;
    else
        hlsl_error(ctx, &attr->args[0].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_DOMAIN,
                "Invalid tessellator domain \"%s\": expected \"isoline\", \"tri\", or \"quad\".",
                value);
}

static void parse_outputcontrolpoints_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    int value;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [outputcontrolpoints] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!get_integral_argument_value(ctx, attr, 0, &value))
        return;

    if (value < 0 || value > 32)
        hlsl_error(ctx, &attr->args[0].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_CONTROL_POINT_COUNT,
                "Output control point count must be between 0 and 32.");

    ctx->output_control_point_count = value;
}

static void parse_outputtopology_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const char *value;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [outputtopology] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!(value = get_string_argument_value(ctx, attr, 0)))
        return;

    if (!strcmp(value, "point"))
        ctx->output_primitive = VKD3D_SHADER_TESSELLATOR_OUTPUT_POINT;
    else if (!strcmp(value, "line"))
        ctx->output_primitive = VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE;
    else if (!strcmp(value, "triangle_cw"))
        ctx->output_primitive = VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CW;
    else if (!strcmp(value, "triangle_ccw"))
        ctx->output_primitive = VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW;
    else
        hlsl_error(ctx, &attr->args[0].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_OUTPUT_PRIMITIVE,
                "Invalid tessellator output topology \"%s\": "
                "expected \"point\", \"line\", \"triangle_cw\", or \"triangle_ccw\".", value);
}

static void parse_partitioning_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const char *value;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [partitioning] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!(value = get_string_argument_value(ctx, attr, 0)))
        return;

    if (!strcmp(value, "integer"))
        ctx->partitioning = VKD3D_SHADER_TESSELLATOR_PARTITIONING_INTEGER;
    else if (!strcmp(value, "pow2"))
        ctx->partitioning = VKD3D_SHADER_TESSELLATOR_PARTITIONING_POW2;
    else if (!strcmp(value, "fractional_even"))
        ctx->partitioning = VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN;
    else if (!strcmp(value, "fractional_odd"))
        ctx->partitioning = VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD;
    else
        hlsl_error(ctx, &attr->args[0].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_PARTITIONING,
                "Invalid tessellator partitioning \"%s\": "
                "expected \"integer\", \"pow2\", \"fractional_even\", or \"fractional_odd\".", value);
}

static void parse_patchconstantfunc_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const char *name;
    struct hlsl_ir_function *func;
    struct hlsl_ir_function_decl *decl;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [patchconstantfunc] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!(name = get_string_argument_value(ctx, attr, 0)))
        return;

    ctx->patch_constant_func = NULL;
    if ((func = hlsl_get_function(ctx, name)))
    {
        /* Pick the last overload with a body. */
        LIST_FOR_EACH_ENTRY_REV(decl, &func->overloads, struct hlsl_ir_function_decl, entry)
        {
            if (decl->has_body)
            {
                ctx->patch_constant_func = decl;
                break;
            }
        }
    }

    if (!ctx->patch_constant_func)
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED,
                "Patch constant function \"%s\" is not defined.", name);
}

static void parse_maxvertexcount_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    int value;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [maxvertexcount] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!get_integral_argument_value(ctx, attr, 0, &value))
        return;

    if (value < 1 || value > 1024)
        hlsl_error(ctx, &attr->args[0].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MAX_VERTEX_COUNT,
                "Max vertex count must be between 1 and 1024.");

    ctx->max_vertex_count = value;
}

static void parse_entry_function_attributes(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    const struct hlsl_profile_info *profile = ctx->profile;
    unsigned int i;

    for (i = 0; i < entry_func->attr_count; ++i)
    {
        const struct hlsl_attribute *attr = entry_func->attrs[i];

        if (!strcmp(attr->name, "numthreads") && profile->type == VKD3D_SHADER_TYPE_COMPUTE)
            parse_numthreads_attribute(ctx, attr);
        else if (!strcmp(attr->name, "domain")
                    && (profile->type == VKD3D_SHADER_TYPE_HULL || profile->type == VKD3D_SHADER_TYPE_DOMAIN))
            parse_domain_attribute(ctx, attr);
        else if (!strcmp(attr->name, "outputcontrolpoints") && profile->type == VKD3D_SHADER_TYPE_HULL)
            parse_outputcontrolpoints_attribute(ctx, attr);
        else if (!strcmp(attr->name, "outputtopology") && profile->type == VKD3D_SHADER_TYPE_HULL)
            parse_outputtopology_attribute(ctx, attr);
        else if (!strcmp(attr->name, "partitioning") && profile->type == VKD3D_SHADER_TYPE_HULL)
            parse_partitioning_attribute(ctx, attr);
        else if (!strcmp(attr->name, "patchconstantfunc") && profile->type == VKD3D_SHADER_TYPE_HULL)
            parse_patchconstantfunc_attribute(ctx, attr);
        else if (!strcmp(attr->name, "earlydepthstencil") && profile->type == VKD3D_SHADER_TYPE_PIXEL)
            entry_func->early_depth_test = true;
        else if (!strcmp(attr->name, "maxvertexcount") && profile->type == VKD3D_SHADER_TYPE_GEOMETRY)
            parse_maxvertexcount_attribute(ctx, attr);
        else
            hlsl_warning(ctx, &entry_func->attrs[i]->loc, VKD3D_SHADER_WARNING_HLSL_UNKNOWN_ATTRIBUTE,
                    "Ignoring unknown attribute \"%s\".", entry_func->attrs[i]->name);
    }
}

static void validate_hull_shader_attributes(struct hlsl_ctx *ctx, const struct hlsl_ir_function_decl *entry_func)
{
    if (ctx->domain == VKD3D_TESSELLATOR_DOMAIN_INVALID)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [domain] attribute.", entry_func->func->name);
    }

    if (ctx->output_control_point_count == UINT_MAX)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [outputcontrolpoints] attribute.", entry_func->func->name);
    }

    if (!ctx->output_primitive)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [outputtopology] attribute.", entry_func->func->name);
    }

    if (!ctx->partitioning)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [partitioning] attribute.", entry_func->func->name);
    }

    if (!ctx->patch_constant_func)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [patchconstantfunc] attribute.", entry_func->func->name);
    }
    else if (ctx->patch_constant_func == entry_func)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_RECURSIVE_CALL,
                "Patch constant function cannot be the entry point function.");
        /* Native returns E_NOTIMPL instead of E_FAIL here. */
        ctx->result = VKD3D_ERROR_NOT_IMPLEMENTED;
        return;
    }

    switch (ctx->domain)
    {
        case VKD3D_TESSELLATOR_DOMAIN_LINE:
            if (ctx->output_primitive == VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CW
                    || ctx->output_primitive == VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW)
                hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_OUTPUT_PRIMITIVE,
                        "Triangle output topologies are not available for isoline domains.");
            break;

        case VKD3D_TESSELLATOR_DOMAIN_TRIANGLE:
            if (ctx->output_primitive == VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE)
                hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_OUTPUT_PRIMITIVE,
                        "Line output topologies are not available for triangle domains.");
            break;

        case VKD3D_TESSELLATOR_DOMAIN_QUAD:
            if (ctx->output_primitive == VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE)
                hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_OUTPUT_PRIMITIVE,
                        "Line output topologies are not available for quad domains.");
            break;

        default:
            break;
    }
}

static enum vkd3d_primitive_type get_primitive_type(struct hlsl_ctx *ctx, struct hlsl_ir_var *var)
{
    uint32_t prim_modifier = var->data_type->modifiers & HLSL_PRIMITIVE_MODIFIERS_MASK;
    enum vkd3d_primitive_type prim_type = VKD3D_PT_UNDEFINED;

    if (prim_modifier)
    {
        unsigned int count = var->data_type->e.array.elements_count;
        unsigned int expected_count;

        VKD3D_ASSERT(!(prim_modifier & (prim_modifier - 1)));

        switch (prim_modifier)
        {
            case HLSL_PRIMITIVE_POINT:
                prim_type = VKD3D_PT_POINTLIST;
                expected_count = 1;
                break;

            case HLSL_PRIMITIVE_LINE:
                prim_type = VKD3D_PT_LINELIST;
                expected_count = 2;
                break;

            case HLSL_PRIMITIVE_TRIANGLE:
                prim_type = VKD3D_PT_TRIANGLELIST;
                expected_count = 3;
                break;

            case HLSL_PRIMITIVE_LINEADJ:
                prim_type = VKD3D_PT_LINELIST_ADJ;
                expected_count = 4;
                break;

            case HLSL_PRIMITIVE_TRIANGLEADJ:
                prim_type = VKD3D_PT_TRIANGLELIST_ADJ;
                expected_count = 6;
                break;

            default:
                vkd3d_unreachable();
        }

        if (count != expected_count)
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_modifiers_to_string(ctx, prim_modifier)))
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_CONTROL_POINT_COUNT,
                        "Control point count %u does not match the expect count %u for the %s input primitive type.",
                        count, expected_count, string->buffer);
            hlsl_release_string_buffer(ctx, string);
        }
    }

    /* Patch types take precedence over primitive modifiers. */
    if (hlsl_type_is_patch_array(var->data_type))
        prim_type = VKD3D_PT_PATCH;

    VKD3D_ASSERT(prim_type != VKD3D_PT_UNDEFINED);
    return prim_type;
}


static void validate_and_record_prim_type(struct hlsl_ctx *ctx, struct hlsl_ir_var *var)
{
    unsigned int control_point_count = var->data_type->e.array.elements_count;
    enum hlsl_array_type array_type = var->data_type->e.array.array_type;
    struct hlsl_type *control_point_type = var->data_type->e.array.type;
    const struct hlsl_profile_info *profile = ctx->profile;

    if (array_type == HLSL_ARRAY_PATCH_INPUT)
    {
        if (profile->type != VKD3D_SHADER_TYPE_HULL
                && !(profile->type == VKD3D_SHADER_TYPE_GEOMETRY && hlsl_version_ge(ctx, 5, 0)))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                    "InputPatch parameters can only be used in hull shaders, "
                    "and geometry shaders with shader model 5.0 or higher.");
            return;
        }
    }
    else if (array_type == HLSL_ARRAY_PATCH_OUTPUT)
    {
        if (!ctx->is_patch_constant_func && profile->type != VKD3D_SHADER_TYPE_DOMAIN)
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                    "OutputPatch parameters can only be used in "
                    "hull shader patch constant functions and domain shaders.");
            return;
        }
    }

    if ((var->data_type->modifiers & HLSL_PRIMITIVE_MODIFIERS_MASK) && profile->type != VKD3D_SHADER_TYPE_GEOMETRY)
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                "Input primitive parameters can only be used in geometry shaders.");
        return;
    }

    if (profile->type == VKD3D_SHADER_TYPE_GEOMETRY)
    {
        enum vkd3d_primitive_type prim_type = get_primitive_type(ctx, var);

        if (ctx->input_primitive_type == VKD3D_PT_UNDEFINED)
        {
            ctx->input_primitive_type = prim_type;
        }
        else if (ctx->input_primitive_type != prim_type)
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Input primitive type does not match the previously declared type.");
            hlsl_note(ctx, &ctx->input_primitive_param->loc, VKD3D_SHADER_LOG_ERROR,
                    "The input primitive was previously declared here.");
        }
    }

    if (control_point_count > 32)
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_CONTROL_POINT_COUNT,
                "Control point count %u exceeds 32.", control_point_count);
        return;
    }
    VKD3D_ASSERT(control_point_count > 0);

    if (ctx->is_patch_constant_func && array_type == HLSL_ARRAY_PATCH_OUTPUT)
    {
        if (control_point_count != ctx->output_control_point_count)
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_CONTROL_POINT_COUNT,
                    "Output control point count %u does not match the count %u declared in the control point function.",
                    control_point_count, ctx->output_control_point_count);

        if (!hlsl_types_are_equal(control_point_type, ctx->output_control_point_type))
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Output control point type does not match the output type of the control point function.");

        return;
    }

    if (ctx->input_control_point_count != UINT_MAX)
    {
        VKD3D_ASSERT(profile->type == VKD3D_SHADER_TYPE_GEOMETRY || ctx->is_patch_constant_func);

        if (control_point_count != ctx->input_control_point_count)
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_CONTROL_POINT_COUNT,
                    "Input control point count %u does not match the count %u declared previously.",
                    control_point_count, ctx->input_control_point_count);
            hlsl_note(ctx, &ctx->input_primitive_param->loc, VKD3D_SHADER_LOG_ERROR,
                    "The input primitive was previously declared here.");
        }

        if (profile->type != VKD3D_SHADER_TYPE_GEOMETRY
                && !hlsl_types_are_equal(control_point_type, ctx->input_control_point_type))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Input control point type does not match the input type declared previously.");
            hlsl_note(ctx, &ctx->input_primitive_param->loc, VKD3D_SHADER_LOG_ERROR,
                    "The input primitive was previously declared here.");
        }

        return;
    }

    ctx->input_control_point_count = control_point_count;
    ctx->input_control_point_type = control_point_type;
    ctx->input_primitive_param = var;
}

static void validate_and_record_stream_outputs(struct hlsl_ctx *ctx)
{
    static const enum vkd3d_primitive_type prim_types[] =
    {
        [HLSL_STREAM_OUTPUT_POINT_STREAM]    = VKD3D_PT_POINTLIST,
        [HLSL_STREAM_OUTPUT_LINE_STREAM]     = VKD3D_PT_LINESTRIP,
        [HLSL_STREAM_OUTPUT_TRIANGLE_STREAM] = VKD3D_PT_TRIANGLESTRIP,
    };

    bool reported_non_point_multistream = false, reported_nonzero_index = false, reported_invalid_index = false;
    enum hlsl_so_object_type so_type;
    const struct hlsl_type *type;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->bind_count[HLSL_REGSET_STREAM_OUTPUTS])
            continue;

        type = hlsl_get_stream_output_type(var->data_type);
        so_type = type->e.so.so_type;

        VKD3D_ASSERT(so_type < ARRAY_SIZE(prim_types));

        if (ctx->output_topology_type == VKD3D_PT_UNDEFINED)
        {
            ctx->output_topology_type = prim_types[so_type];
        }
        else
        {
            if ((so_type != HLSL_STREAM_OUTPUT_POINT_STREAM || ctx->output_topology_type != VKD3D_PT_POINTLIST)
                    && !reported_non_point_multistream)
            {
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Multiple output streams are only allowed with PointStream objects.");
                reported_non_point_multistream = true;
            }
        }

        if (var->regs[HLSL_REGSET_STREAM_OUTPUTS].index && hlsl_version_lt(ctx, 5, 0) && !reported_nonzero_index)
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                    "Multiple output streams are only supported in shader model 5.0 or higher.");
            reported_nonzero_index = true;
        }

        if (var->regs[HLSL_REGSET_STREAM_OUTPUTS].index >= VKD3D_MAX_STREAM_COUNT && !reported_invalid_index)
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SIZE,
                    "Output stream index %u exceeds the maximum index %u.",
                    var->regs[HLSL_REGSET_STREAM_OUTPUTS].index, VKD3D_MAX_STREAM_COUNT - 1);
            reported_invalid_index = true;
        }
    }

    /* TODO: check that maxvertexcount * outputdatasize <= 1024. */
}

static void validate_max_output_size(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func,
        uint32_t output_reg_count)
{
    unsigned int max_output_size, comp_count = 0;
    unsigned int *reg_comp_count;
    struct hlsl_ir_var *var;
    uint32_t id;

    if (ctx->result)
        return;

    if (!(reg_comp_count = hlsl_calloc(ctx, output_reg_count, sizeof(*reg_comp_count))))
        return;

    LIST_FOR_EACH_ENTRY(var, &entry_func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->is_output_semantic)
            continue;

        VKD3D_ASSERT(var->regs[HLSL_REGSET_NUMERIC].allocated);
        id = var->regs[HLSL_REGSET_NUMERIC].id;
        reg_comp_count[id] = max(reg_comp_count[id], vkd3d_log2i(var->regs[HLSL_REGSET_NUMERIC].writemask) + 1);
    }

    for (id = 0; id < output_reg_count; ++id)
        comp_count += reg_comp_count[id];

    max_output_size = ctx->max_vertex_count * comp_count;
    if (max_output_size > 1024)
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MAX_VERTEX_COUNT,
                "Max vertex count (%u) * output data component count (%u) = %u, which is greater than 1024.",
                ctx->max_vertex_count, comp_count, max_output_size);

    vkd3d_free(reg_comp_count);
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

void hlsl_lower_index_loads(struct hlsl_ctx *ctx, struct hlsl_block *body)
{
    lower_ir(ctx, lower_index_loads, body);
}


static bool simplify_exprs(struct hlsl_ctx *ctx, struct hlsl_block *block)
{
    bool progress, any_progress = false;

    do
    {
        progress = hlsl_transform_ir(ctx, hlsl_fold_constant_exprs, block, NULL);
        progress |= hlsl_transform_ir(ctx, hlsl_normalize_binary_exprs, block, NULL);
        progress |= hlsl_transform_ir(ctx, hlsl_fold_constant_identities, block, NULL);
        progress |= hlsl_transform_ir(ctx, hlsl_fold_constant_swizzles, block, NULL);

        any_progress |= progress;
    } while (progress);

    return any_progress;
}

static void hlsl_run_folding_passes(struct hlsl_ctx *ctx, struct hlsl_block *body)
{
    bool progress;

    hlsl_transform_ir(ctx, fold_redundant_casts, body, NULL);
    do
    {
        progress = simplify_exprs(ctx, body);
        progress |= hlsl_copy_propagation_execute(ctx, body);
        progress |= hlsl_transform_ir(ctx, fold_swizzle_chains, body, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_swizzles, body, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_conditional_branches, body, NULL);
    } while (progress);
    hlsl_transform_ir(ctx, fold_redundant_casts, body, NULL);
}

void hlsl_run_const_passes(struct hlsl_ctx *ctx, struct hlsl_block *body)
{
    bool progress;

    lower_ir(ctx, lower_complex_casts, body);
    lower_ir(ctx, lower_matrix_swizzles, body);

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
    lower_ir(ctx, lower_int_dot, body);
    if (hlsl_version_ge(ctx, 4, 0))
    {
        lower_ir(ctx, lower_int_modulus_sm4, body);
        lower_ir(ctx, lower_int_division_sm4, body);
    }
    lower_ir(ctx, lower_int_abs, body);
    lower_ir(ctx, lower_casts_to_bool, body);
    lower_ir(ctx, lower_float_modulus, body);

    hlsl_run_folding_passes(ctx, body);
}

static void generate_vsir_signature_entry(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct shader_signature *signature, bool output, struct hlsl_ir_var *var)
{
    enum vkd3d_shader_component_type component_type = VKD3D_SHADER_COMPONENT_VOID;
    bool is_primitive = hlsl_type_is_primitive_array(var->data_type);
    enum vkd3d_shader_sysval_semantic sysval = VKD3D_SHADER_SV_NONE;
    unsigned int register_index, mask, use_mask;
    const char *name = var->semantic.name;
    enum vkd3d_shader_register_type type;
    struct signature_element *element;

    if (hlsl_version_ge(ctx, 4, 0))
    {
        struct vkd3d_string_buffer *string;
        enum hlsl_base_type numeric_type;
        bool has_idx, ret;

        ret = sm4_sysval_semantic_from_semantic_name(&sysval, &program->shader_version, ctx->semantic_compat_mapping,
                ctx->domain, var->semantic.name, var->semantic.index, output, ctx->is_patch_constant_func, is_primitive);
        VKD3D_ASSERT(ret);
        if (sysval == ~0u)
            return;

        if (sm4_register_from_semantic_name(&program->shader_version, var->semantic.name, output, &type, &has_idx))
        {
            register_index = has_idx ? var->semantic.index : ~0u;
            mask = (1u << var->data_type->e.numeric.dimx) - 1;
        }
        else
        {
            VKD3D_ASSERT(var->regs[HLSL_REGSET_NUMERIC].allocated);
            register_index = var->regs[HLSL_REGSET_NUMERIC].id;
            mask = var->regs[HLSL_REGSET_NUMERIC].writemask;
        }

        use_mask = mask; /* FIXME: retrieve use mask accurately. */

        if (var->data_type->class == HLSL_CLASS_ARRAY)
            numeric_type = var->data_type->e.array.type->e.numeric.type;
        else
            numeric_type = var->data_type->e.numeric.type;

        switch (numeric_type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                component_type = VKD3D_SHADER_COMPONENT_FLOAT;
                break;

            case HLSL_TYPE_INT:
                component_type = VKD3D_SHADER_COMPONENT_INT;
                break;

            case HLSL_TYPE_BOOL:
            case HLSL_TYPE_MIN16UINT:
            case HLSL_TYPE_UINT:
                component_type = VKD3D_SHADER_COMPONENT_UINT;
                break;

            case HLSL_TYPE_DOUBLE:
                if ((string = hlsl_type_to_string(ctx, var->data_type)))
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Invalid data type %s for semantic variable %s.", string->buffer, var->name);
                hlsl_release_string_buffer(ctx, string);
                break;
        }

        if (sysval == VKD3D_SHADER_SV_TARGET && !ascii_strcasecmp(name, "color"))
            name = "SV_Target";
        else if (sysval == VKD3D_SHADER_SV_DEPTH && !ascii_strcasecmp(name, "depth"))
            name ="SV_Depth";
        else if (sysval == VKD3D_SHADER_SV_POSITION && !ascii_strcasecmp(name, "position"))
            name = "SV_Position";
    }
    else
    {
        if ((!output && !var->last_read) || (output && !var->first_write))
            return;

        if (!sm1_register_from_semantic_name(&program->shader_version,
                var->semantic.name, var->semantic.index, output, &sysval, &type, &register_index))
        {
            enum vkd3d_decl_usage usage;
            unsigned int usage_idx;
            bool ret;

            register_index = var->regs[HLSL_REGSET_NUMERIC].id;

            ret = sm1_usage_from_semantic_name(var->semantic.name, var->semantic.index, &usage, &usage_idx);
            VKD3D_ASSERT(ret);
            /* With the exception of vertex POSITION output, none of these are
             * system values. Pixel POSITION input is not equivalent to
             * SV_Position; the closer equivalent is VPOS, which is not declared
             * as a semantic. */
            if (program->shader_version.type == VKD3D_SHADER_TYPE_VERTEX
                    && output && usage == VKD3D_DECL_USAGE_POSITION)
                sysval = VKD3D_SHADER_SV_POSITION;
            else
                sysval = VKD3D_SHADER_SV_NONE;
        }

        mask = (1 << var->data_type->e.numeric.dimx) - 1;

        if (!ascii_strcasecmp(var->semantic.name, "PSIZE") && output
                && program->shader_version.type == VKD3D_SHADER_TYPE_VERTEX)
        {
            if (var->data_type->e.numeric.dimx > 1)
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                        "PSIZE output must have only 1 component in this shader model.");
            /* For some reason the writemask has all components set. */
            mask = VKD3DSP_WRITEMASK_ALL;
        }
        if (!ascii_strcasecmp(var->semantic.name, "FOG") && output && program->shader_version.major < 3
                && program->shader_version.type == VKD3D_SHADER_TYPE_VERTEX && var->data_type->e.numeric.dimx > 1)
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "FOG output must have only 1 component in this shader model.");

        use_mask = mask; /* FIXME: retrieve use mask accurately. */
        component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    }

    if (!vkd3d_array_reserve((void **)&signature->elements, &signature->elements_capacity,
            signature->element_count + 1, sizeof(*signature->elements)))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }
    element = &signature->elements[signature->element_count++];
    memset(element, 0, sizeof(*element));

    if (!(element->semantic_name = vkd3d_strdup(name)))
    {
        --signature->element_count;
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }
    element->semantic_index = var->semantic.index;
    element->sysval_semantic = sysval;
    element->component_type = component_type;
    element->register_index = register_index;
    element->target_location = register_index;
    element->register_count = 1;
    element->mask = mask;
    element->used_mask = use_mask;
    if (program->shader_version.type == VKD3D_SHADER_TYPE_PIXEL && !output)
    {
        if (program->shader_version.major >= 4)
            element->interpolation_mode = sm4_get_interpolation_mode(var->data_type, var->storage_modifiers);
        else
            element->interpolation_mode = VKD3DSIM_LINEAR;
    }

    switch (var->data_type->e.numeric.type)
    {
        case HLSL_TYPE_BOOL:
        case HLSL_TYPE_DOUBLE:
        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_HALF:
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
            element->min_precision = VKD3D_SHADER_MINIMUM_PRECISION_NONE;
            break;

        case HLSL_TYPE_MIN16UINT:
            element->min_precision = VKD3D_SHADER_MINIMUM_PRECISION_UINT_16;
            break;
    }
}

static void generate_vsir_signature(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_function_decl *func)
{
    bool is_domain = program->shader_version.type == VKD3D_SHADER_TYPE_DOMAIN;
    struct hlsl_ir_var *var;

    ctx->is_patch_constant_func = func == ctx->patch_constant_func;

    LIST_FOR_EACH_ENTRY(var, &func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_input_semantic)
        {
            bool is_patch = hlsl_type_is_patch_array(var->data_type);

            if (ctx->is_patch_constant_func)
            {
                if (!is_patch)
                    generate_vsir_signature_entry(ctx, program, &program->patch_constant_signature, false, var);
            }
            else if (is_domain)
            {
                if (is_patch)
                    generate_vsir_signature_entry(ctx, program, &program->input_signature, false, var);
                else
                    generate_vsir_signature_entry(ctx, program, &program->patch_constant_signature, false, var);
            }
            else
            {
                generate_vsir_signature_entry(ctx, program, &program->input_signature, false, var);
            }
        }

        if (var->is_output_semantic)
        {
            if (ctx->is_patch_constant_func)
                generate_vsir_signature_entry(ctx, program, &program->patch_constant_signature, true, var);
            else
                generate_vsir_signature_entry(ctx, program, &program->output_signature, true, var);
        }
    }
}

static enum vkd3d_data_type vsir_data_type_from_hlsl_type(struct hlsl_ctx *ctx, const struct hlsl_type *type)
{
    if (hlsl_version_lt(ctx, 4, 0))
        return VKD3D_DATA_FLOAT;

    if (type->class == HLSL_CLASS_ARRAY)
        return vsir_data_type_from_hlsl_type(ctx, type->e.array.type);
    if (type->class == HLSL_CLASS_STRUCT)
        return VKD3D_DATA_MIXED;
    if (type->class <= HLSL_CLASS_LAST_NUMERIC)
    {
        switch (type->e.numeric.type)
        {
            case HLSL_TYPE_DOUBLE:
                return VKD3D_DATA_DOUBLE;
            case HLSL_TYPE_FLOAT:
                return VKD3D_DATA_FLOAT;
            case HLSL_TYPE_HALF:
                return VKD3D_DATA_HALF;
            case HLSL_TYPE_INT:
                return VKD3D_DATA_INT;
            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
            case HLSL_TYPE_MIN16UINT:
                return VKD3D_DATA_UINT;
        }
    }

    vkd3d_unreachable();
}

static enum vkd3d_data_type vsir_data_type_from_hlsl_instruction(struct hlsl_ctx *ctx,
        const struct hlsl_ir_node *instr)
{
    return vsir_data_type_from_hlsl_type(ctx, instr->data_type);
}

static uint32_t generate_vsir_get_src_swizzle(uint32_t src_writemask, uint32_t dst_writemask)
{
    uint32_t swizzle;

    swizzle = hlsl_swizzle_from_writemask(src_writemask);
    swizzle = hlsl_map_swizzle(swizzle, dst_writemask);
    return swizzle;
}

static void sm1_generate_vsir_constant_defs(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_block *block)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    unsigned int i, x;

    for (i = 0; i < ctx->constant_defs.count; ++i)
    {
        const struct hlsl_constant_register *constant_reg = &ctx->constant_defs.regs[i];

        if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
        {
            ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
            return;
        }

        ins = &instructions->elements[instructions->count];
        if (!vsir_instruction_init_with_params(program, ins, &constant_reg->loc, VKD3DSIH_DEF, 1, 1))
        {
            ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
            return;
        }
        ++instructions->count;

        dst_param = &ins->dst[0];
        vsir_register_init(&dst_param->reg, VKD3DSPR_CONST, VKD3D_DATA_FLOAT, 1);
        ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->dst[0].reg.idx[0].offset = constant_reg->index;
        ins->dst[0].write_mask = VKD3DSP_WRITEMASK_ALL;

        src_param = &ins->src[0];
        vsir_register_init(&src_param->reg, VKD3DSPR_IMMCONST, VKD3D_DATA_FLOAT, 0);
        src_param->reg.type = VKD3DSPR_IMMCONST;
        src_param->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
        src_param->reg.non_uniform = false;
        src_param->reg.data_type = VKD3D_DATA_FLOAT;
        src_param->reg.dimension = VSIR_DIMENSION_VEC4;
        for (x = 0; x < 4; ++x)
            src_param->reg.u.immconst_f32[x] = constant_reg->value.f[x];
        src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    }
}

static void sm1_generate_vsir_sampler_dcls(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_block *block)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    enum vkd3d_shader_resource_type resource_type;
    struct vkd3d_shader_register_range *range;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_semantic *semantic;
    struct vkd3d_shader_instruction *ins;
    enum hlsl_sampler_dim sampler_dim;
    struct hlsl_ir_var *var;
    unsigned int i, count;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->regs[HLSL_REGSET_SAMPLERS].allocated)
            continue;

        count = var->bind_count[HLSL_REGSET_SAMPLERS];
        for (i = 0; i < count; ++i)
        {
            if (var->objects_usage[HLSL_REGSET_SAMPLERS][i].used)
            {
                sampler_dim = var->objects_usage[HLSL_REGSET_SAMPLERS][i].sampler_dim;

                switch (sampler_dim)
                {
                    case HLSL_SAMPLER_DIM_2D:
                        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
                        break;

                    case HLSL_SAMPLER_DIM_CUBE:
                        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_CUBE;
                        break;

                    case HLSL_SAMPLER_DIM_3D:
                        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_3D;
                        break;

                    case HLSL_SAMPLER_DIM_GENERIC:
                        /* These can appear in sm4-style separate sample
                         * instructions that haven't been lowered. */
                        hlsl_fixme(ctx, &var->loc, "Generic samplers need to be lowered.");
                        continue;

                    default:
                        vkd3d_unreachable();
                        break;
                }

                if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
                {
                    ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
                    return;
                }

                ins = &instructions->elements[instructions->count];
                if (!vsir_instruction_init_with_params(program, ins, &var->loc, VKD3DSIH_DCL, 0, 0))
                {
                    ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
                    return;
                }
                ++instructions->count;

                semantic = &ins->declaration.semantic;
                semantic->resource_type = resource_type;

                dst_param = &semantic->resource.reg;
                vsir_register_init(&dst_param->reg, VKD3DSPR_SAMPLER, VKD3D_DATA_FLOAT, 1);
                dst_param->reg.dimension = VSIR_DIMENSION_NONE;
                dst_param->reg.idx[0].offset = var->regs[HLSL_REGSET_SAMPLERS].index + i;
                dst_param->write_mask = 0;
                range = &semantic->resource.range;
                range->space = 0;
                range->first = range->last = dst_param->reg.idx[0].offset;
            }
        }
    }
}

static enum vkd3d_shader_register_type sm4_get_semantic_register_type(enum vkd3d_shader_type shader_type,
        bool is_patch_constant_func, const struct hlsl_ir_var *var)
{
    if (hlsl_type_is_primitive_array(var->data_type))
    {
        VKD3D_ASSERT(var->is_input_semantic);

        switch (shader_type)
        {
            case VKD3D_SHADER_TYPE_HULL:
                if (is_patch_constant_func)
                {
                    bool is_inputpatch = var->data_type->e.array.array_type == HLSL_ARRAY_PATCH_INPUT;

                    return is_inputpatch ? VKD3DSPR_INCONTROLPOINT : VKD3DSPR_OUTCONTROLPOINT;
                }
                return VKD3DSPR_INPUT;

            case VKD3D_SHADER_TYPE_DOMAIN:
                return VKD3DSPR_INCONTROLPOINT;

            default:
                return VKD3DSPR_INPUT;
        }
    }

    if (var->is_output_semantic)
        return VKD3DSPR_OUTPUT;
    if (shader_type == VKD3D_SHADER_TYPE_DOMAIN)
        return VKD3DSPR_PATCHCONST;
    return VKD3DSPR_INPUT;
}

static struct vkd3d_shader_instruction *generate_vsir_add_program_instruction(
        struct hlsl_ctx *ctx, struct vsir_program *program,
        const struct vkd3d_shader_location *loc, enum vkd3d_shader_opcode opcode,
        unsigned int dst_count, unsigned int src_count)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_instruction *ins;

    if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    ins = &instructions->elements[instructions->count];
    if (!vsir_instruction_init_with_params(program, ins, loc, opcode, dst_count, src_count))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    ++instructions->count;
    return ins;
}

static void vsir_src_from_hlsl_constant_value(struct vkd3d_shader_src_param *src,
        struct hlsl_ctx *ctx, const struct hlsl_constant_value *value,
        enum vkd3d_data_type type, unsigned int width, unsigned int map_writemask)
{
    unsigned int i, j;

    vsir_src_param_init(src, VKD3DSPR_IMMCONST, type, 0);
    if (width == 1)
    {
        src->reg.u.immconst_u32[0] = value->u[0].u;
        return;
    }

    src->reg.dimension = VSIR_DIMENSION_VEC4;
    for (i = 0, j = 0; i < 4; ++i)
    {
        if ((map_writemask & (1u << i)) && (j < width))
            src->reg.u.immconst_u32[i] = value->u[j++].u;
        else
            src->reg.u.immconst_u32[i] = 0;
    }
}

static void vsir_src_from_hlsl_node(struct vkd3d_shader_src_param *src,
        struct hlsl_ctx *ctx, const struct hlsl_ir_node *instr, uint32_t map_writemask)
{
    struct hlsl_ir_constant *constant;

    if (hlsl_version_ge(ctx, 4, 0) && instr->type == HLSL_IR_CONSTANT)
    {
        /* In SM4 constants are inlined */
        constant = hlsl_ir_constant(instr);
        vsir_src_from_hlsl_constant_value(src, ctx, &constant->value,
                vsir_data_type_from_hlsl_instruction(ctx, instr), instr->data_type->e.numeric.dimx, map_writemask);
    }
    else
    {
        vsir_register_init(&src->reg, VKD3DSPR_TEMP, vsir_data_type_from_hlsl_instruction(ctx, instr), 1);
        src->reg.idx[0].offset = instr->reg.id;
        src->reg.dimension = VSIR_DIMENSION_VEC4;
        src->swizzle = generate_vsir_get_src_swizzle(instr->reg.writemask, map_writemask);
    }
}

static struct vkd3d_shader_src_param *sm4_generate_vsir_new_idx_src(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_node *rel_offset)
{
    struct vkd3d_shader_src_param *idx_src;

    if (!(idx_src = vsir_program_get_src_params(program, 1)))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    memset(idx_src, 0, sizeof(*idx_src));
    vsir_src_from_hlsl_node(idx_src, ctx, rel_offset, VKD3DSP_WRITEMASK_ALL);
    return idx_src;
}

static bool sm4_generate_vsir_numeric_reg_from_deref(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct vkd3d_shader_register *reg, uint32_t *writemask, const struct hlsl_deref *deref)
{
    const struct hlsl_ir_var *var = deref->var;
    unsigned int offset_const_deref;

    reg->type = var->indexable ? VKD3DSPR_IDXTEMP : VKD3DSPR_TEMP;
    reg->idx[0].offset = var->regs[HLSL_REGSET_NUMERIC].id;
    reg->dimension = VSIR_DIMENSION_VEC4;

    VKD3D_ASSERT(var->regs[HLSL_REGSET_NUMERIC].allocated);

    if (!var->indexable)
    {
        offset_const_deref = hlsl_offset_from_deref_safe(ctx, deref);
        reg->idx[0].offset += offset_const_deref / 4;
        reg->idx_count = 1;
    }
    else
    {
        offset_const_deref = deref->const_offset;
        reg->idx[1].offset = offset_const_deref / 4;
        reg->idx_count = 2;

        if (deref->rel_offset.node)
        {
            if (!(reg->idx[1].rel_addr = sm4_generate_vsir_new_idx_src(ctx, program, deref->rel_offset.node)))
                return false;
        }
    }

    *writemask = 0xf & (0xf << (offset_const_deref % 4));
    if (var->regs[HLSL_REGSET_NUMERIC].writemask)
        *writemask = hlsl_combine_writemasks(var->regs[HLSL_REGSET_NUMERIC].writemask, *writemask);
    return true;
}

static bool sm4_generate_vsir_reg_from_deref(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct vkd3d_shader_register *reg, uint32_t *writemask, const struct hlsl_deref *deref)
{
    const struct vkd3d_shader_version *version = &program->shader_version;
    const struct hlsl_type *data_type = hlsl_deref_get_type(ctx, deref);
    const struct hlsl_ir_var *var = deref->var;

    if (var->is_uniform)
    {
        enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);

        if (regset == HLSL_REGSET_TEXTURES)
        {
            reg->type = VKD3DSPR_RESOURCE;
            reg->dimension = VSIR_DIMENSION_VEC4;
            if (vkd3d_shader_ver_ge(version, 5, 1))
            {
                reg->idx[0].offset = var->regs[HLSL_REGSET_TEXTURES].id;
                reg->idx[1].offset = var->regs[HLSL_REGSET_TEXTURES].index; /* FIXME: array index */
                reg->idx_count = 2;
            }
            else
            {
                reg->idx[0].offset = var->regs[HLSL_REGSET_TEXTURES].index;
                reg->idx[0].offset += hlsl_offset_from_deref_safe(ctx, deref);
                reg->idx_count = 1;
            }
            VKD3D_ASSERT(regset == HLSL_REGSET_TEXTURES);
            *writemask = VKD3DSP_WRITEMASK_ALL;
        }
        else if (regset == HLSL_REGSET_UAVS)
        {
            reg->type = VKD3DSPR_UAV;
            reg->dimension = VSIR_DIMENSION_VEC4;
            if (vkd3d_shader_ver_ge(version, 5, 1))
            {
                reg->idx[0].offset = var->regs[HLSL_REGSET_UAVS].id;
                reg->idx[1].offset = var->regs[HLSL_REGSET_UAVS].index; /* FIXME: array index */
                reg->idx_count = 2;
            }
            else
            {
                reg->idx[0].offset = var->regs[HLSL_REGSET_UAVS].index;
                reg->idx[0].offset += hlsl_offset_from_deref_safe(ctx, deref);
                reg->idx_count = 1;
            }
            VKD3D_ASSERT(regset == HLSL_REGSET_UAVS);
            *writemask = VKD3DSP_WRITEMASK_ALL;
        }
        else if (regset == HLSL_REGSET_SAMPLERS)
        {
            reg->type = VKD3DSPR_SAMPLER;
            reg->dimension = VSIR_DIMENSION_NONE;
            if (vkd3d_shader_ver_ge(version, 5, 1))
            {
                reg->idx[0].offset = var->regs[HLSL_REGSET_SAMPLERS].id;
                reg->idx[1].offset = var->regs[HLSL_REGSET_SAMPLERS].index; /* FIXME: array index */
                reg->idx_count = 2;
            }
            else
            {
                reg->idx[0].offset = var->regs[HLSL_REGSET_SAMPLERS].index;
                reg->idx[0].offset += hlsl_offset_from_deref_safe(ctx, deref);
                reg->idx_count = 1;
            }
            VKD3D_ASSERT(regset == HLSL_REGSET_SAMPLERS);
            *writemask = VKD3DSP_WRITEMASK_ALL;
        }
        else
        {
            unsigned int offset = deref->const_offset + var->buffer_offset;

            VKD3D_ASSERT(data_type->class <= HLSL_CLASS_VECTOR);
            reg->type = VKD3DSPR_CONSTBUFFER;
            reg->dimension = VSIR_DIMENSION_VEC4;
            if (vkd3d_shader_ver_ge(version, 5, 1))
            {
                reg->idx[0].offset = var->buffer->reg.id;
                reg->idx[1].offset = var->buffer->reg.index; /* FIXME: array index */
                reg->idx[2].offset = offset / 4;
                reg->idx_count = 3;
            }
            else
            {
                reg->idx[0].offset = var->buffer->reg.index;
                reg->idx[1].offset = offset / 4;
                reg->idx_count = 2;
            }

            if (deref->rel_offset.node)
            {
                if (!(reg->idx[reg->idx_count - 1].rel_addr = sm4_generate_vsir_new_idx_src(ctx,
                        program, deref->rel_offset.node)))
                    return false;
            }

            *writemask = ((1u << data_type->e.numeric.dimx) - 1) << (offset & 3);
        }
    }
    else if (var->is_input_semantic)
    {
        bool is_primitive = hlsl_type_is_primitive_array(var->data_type);
        bool has_idx;

        if (sm4_register_from_semantic_name(version, var->semantic.name, false, &reg->type, &has_idx))
        {
            unsigned int offset = hlsl_offset_from_deref_safe(ctx, deref);

            VKD3D_ASSERT(!is_primitive);

            if (has_idx)
            {
                reg->idx[0].offset = var->semantic.index + offset / 4;
                reg->idx_count = 1;
            }

            if (shader_sm4_is_scalar_register(reg))
                reg->dimension = VSIR_DIMENSION_SCALAR;
            else
                reg->dimension = VSIR_DIMENSION_VEC4;
            *writemask = ((1u << data_type->e.numeric.dimx) - 1) << (offset % 4);
        }
        else
        {
            struct hlsl_reg hlsl_reg = hlsl_reg_from_deref(ctx, deref);

            VKD3D_ASSERT(hlsl_reg.allocated);

            reg->type = sm4_get_semantic_register_type(version->type, ctx->is_patch_constant_func, var);
            reg->dimension = VSIR_DIMENSION_VEC4;
            reg->idx[is_primitive ? 1 : 0].offset = hlsl_reg.id;
            reg->idx_count = is_primitive ? 2 : 1;
            *writemask = hlsl_reg.writemask;
        }

        if (is_primitive)
        {
            reg->idx[0].offset = deref->const_offset / 4;
            if (deref->rel_offset.node)
            {
                if (!(reg->idx[0].rel_addr = sm4_generate_vsir_new_idx_src(ctx, program, deref->rel_offset.node)))
                    return false;
            }
        }
    }
    else if (var->is_output_semantic)
    {
        bool has_idx;

        if (sm4_register_from_semantic_name(version, var->semantic.name, true, &reg->type, &has_idx))
        {
            unsigned int offset = hlsl_offset_from_deref_safe(ctx, deref);

            if (has_idx)
            {
                reg->idx[0].offset = var->semantic.index + offset / 4;
                reg->idx_count = 1;
            }

            if (shader_sm4_is_scalar_register(reg))
                reg->dimension = VSIR_DIMENSION_SCALAR;
            else
                reg->dimension = VSIR_DIMENSION_VEC4;
            *writemask = ((1u << data_type->e.numeric.dimx) - 1) << (offset % 4);
        }
        else
        {
            struct hlsl_reg hlsl_reg = hlsl_reg_from_deref(ctx, deref);

            VKD3D_ASSERT(hlsl_reg.allocated);
            reg->type = VKD3DSPR_OUTPUT;
            reg->dimension = VSIR_DIMENSION_VEC4;
            reg->idx[0].offset = hlsl_reg.id;
            reg->idx_count = 1;
            *writemask = hlsl_reg.writemask;
        }
    }
    else
    {
        return sm4_generate_vsir_numeric_reg_from_deref(ctx, program, reg, writemask, deref);
    }
    return true;
}

static bool sm4_generate_vsir_init_src_param_from_deref(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct vkd3d_shader_src_param *src_param, const struct hlsl_deref *deref,
        unsigned int dst_writemask, const struct vkd3d_shader_location *loc)
{
    uint32_t writemask;

    if (!sm4_generate_vsir_reg_from_deref(ctx, program, &src_param->reg, &writemask, deref))
        return false;
    if (src_param->reg.dimension != VSIR_DIMENSION_NONE)
        src_param->swizzle = generate_vsir_get_src_swizzle(writemask, dst_writemask);
    return true;
}

static bool sm4_generate_vsir_init_dst_param_from_deref(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct vkd3d_shader_dst_param *dst_param, const struct hlsl_deref *deref,
        const struct vkd3d_shader_location *loc, unsigned int writemask)
{
    uint32_t reg_writemask;

    if (!sm4_generate_vsir_reg_from_deref(ctx, program, &dst_param->reg, &reg_writemask, deref))
        return false;
    dst_param->write_mask = hlsl_combine_writemasks(reg_writemask, writemask);
    return true;
}

static void vsir_dst_from_hlsl_node(struct vkd3d_shader_dst_param *dst,
        struct hlsl_ctx *ctx, const struct hlsl_ir_node *instr)
{
    VKD3D_ASSERT(instr->reg.allocated);
    vsir_dst_param_init(dst, VKD3DSPR_TEMP, vsir_data_type_from_hlsl_instruction(ctx, instr), 1);
    dst->reg.idx[0].offset = instr->reg.id;
    dst->reg.dimension = VSIR_DIMENSION_VEC4;
    dst->write_mask = instr->reg.writemask;
}

static void sm1_generate_vsir_instr_constant(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_constant *constant)
{
    struct hlsl_ir_node *instr = &constant->node;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(instr->reg.allocated);
    VKD3D_ASSERT(constant->reg.allocated);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return;

    src_param = &ins->src[0];
    vsir_register_init(&src_param->reg, VKD3DSPR_CONST, VKD3D_DATA_FLOAT, 1);
    src_param->reg.dimension = VSIR_DIMENSION_VEC4;
    src_param->reg.idx[0].offset = constant->reg.id;
    src_param->swizzle = generate_vsir_get_src_swizzle(constant->reg.writemask, instr->reg.writemask);

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);
}

static void sm4_generate_vsir_rasterizer_sample_count(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr)
{
    struct vkd3d_shader_src_param *src_param;
    struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_SAMPLE_INFO, 1, 1)))
        return;
    ins->flags = VKD3DSI_SAMPLE_INFO_UINT;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    src_param = &ins->src[0];
    vsir_src_param_init(src_param, VKD3DSPR_RASTERIZER, VKD3D_DATA_UNUSED, 0);
    src_param->reg.dimension = VSIR_DIMENSION_VEC4;
    src_param->swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
}

/* Translate ops that can be mapped to a single vsir instruction with only one dst register. */
static void generate_vsir_instr_expr_single_instr_op(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr, enum vkd3d_shader_opcode opcode,
        uint32_t src_mod, uint32_t dst_mod, bool map_src_swizzles)
{
    struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    unsigned int i, src_count = 0;

    VKD3D_ASSERT(instr->reg.allocated);

    for (i = 0; i < HLSL_MAX_OPERANDS; ++i)
    {
        if (expr->operands[i].node)
            src_count = i + 1;
    }
    VKD3D_ASSERT(!src_mod || src_count == 1);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, src_count)))
        return;

    dst_param = &ins->dst[0];
    vsir_dst_from_hlsl_node(dst_param, ctx, instr);
    dst_param->modifiers = dst_mod;

    for (i = 0; i < src_count; ++i)
    {
        struct hlsl_ir_node *operand = expr->operands[i].node;

        src_param = &ins->src[i];
        vsir_src_from_hlsl_node(src_param, ctx, operand,
                map_src_swizzles ? dst_param->write_mask : VKD3DSP_WRITEMASK_ALL);
        src_param->modifiers = src_mod;
    }
}

/* Translate ops that have 1 src and need one instruction for each component in
 * the d3dbc backend. */
static void sm1_generate_vsir_instr_expr_per_component_instr_op(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr, enum vkd3d_shader_opcode opcode)
{
    struct hlsl_ir_node *operand = expr->operands[0].node;
    struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    uint32_t src_swizzle;
    unsigned int i, c;

    VKD3D_ASSERT(instr->reg.allocated);
    VKD3D_ASSERT(operand);

    src_swizzle = generate_vsir_get_src_swizzle(operand->reg.writemask, instr->reg.writemask);
    for (i = 0; i < 4; ++i)
    {
        if (instr->reg.writemask & (1u << i))
        {
            if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, 1)))
                return;

            dst_param = &ins->dst[0];
            vsir_register_init(&dst_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
            dst_param->reg.idx[0].offset = instr->reg.id;
            dst_param->reg.dimension = VSIR_DIMENSION_VEC4;
            dst_param->write_mask = 1u << i;

            src_param = &ins->src[0];
            vsir_register_init(&src_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
            src_param->reg.idx[0].offset = operand->reg.id;
            src_param->reg.dimension = VSIR_DIMENSION_VEC4;
            c = vsir_swizzle_get_component(src_swizzle, i);
            src_param->swizzle = vsir_swizzle_from_writemask(1u << c);
        }
    }
}

static void sm1_generate_vsir_instr_expr_sincos(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_ir_expr *expr)
{
    struct hlsl_ir_node *operand = expr->operands[0].node;
    struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    unsigned int src_count = 0;

    VKD3D_ASSERT(instr->reg.allocated);
    src_count = (ctx->profile->major_version < 3) ? 3 : 1;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_SINCOS, 1, src_count)))
        return;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);
    vsir_src_from_hlsl_node(&ins->src[0], ctx, operand, VKD3DSP_WRITEMASK_ALL);

    if (ctx->profile->major_version < 3)
    {
        src_param = &ins->src[1];
        vsir_register_init(&src_param->reg, VKD3DSPR_CONST, VKD3D_DATA_FLOAT, 1);
        src_param->reg.dimension = VSIR_DIMENSION_VEC4;
        src_param->reg.idx[0].offset = ctx->d3dsincosconst1.id;
        src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;

        src_param = &ins->src[2];
        vsir_register_init(&src_param->reg, VKD3DSPR_CONST, VKD3D_DATA_FLOAT, 1);
        src_param->reg.dimension = VSIR_DIMENSION_VEC4;
        src_param->reg.idx[0].offset = ctx->d3dsincosconst2.id;
        src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    }
}

static bool sm1_generate_vsir_instr_expr_cast(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr)
{
    const struct hlsl_type *src_type, *dst_type;
    const struct hlsl_ir_node *arg1, *instr;

    arg1 = expr->operands[0].node;
    src_type = arg1->data_type;
    instr = &expr->node;
    dst_type = instr->data_type;

    /* Narrowing casts were already lowered. */
    VKD3D_ASSERT(src_type->e.numeric.dimx == dst_type->e.numeric.dimx);

    switch (dst_type->e.numeric.type)
    {
        case HLSL_TYPE_HALF:
        case HLSL_TYPE_FLOAT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT:
                case HLSL_TYPE_UINT:
                case HLSL_TYPE_BOOL:
                    /* Integrals are internally represented as floats, so no change is necessary.*/
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    if (ctx->double_as_float_alias)
                    {
                        generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                        return true;
                    }
                    hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "The 'double' type is not supported for the %s profile.", ctx->profile->name);
                    break;
            }
            break;

        case HLSL_TYPE_INT:
        case HLSL_TYPE_MIN16UINT:
        case HLSL_TYPE_UINT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    /* A compilation pass turns these into FLOOR+REINTERPRET, so we should not
                     * reach this case unless we are missing something. */
                    hlsl_fixme(ctx, &instr->loc, "Unlowered SM1 cast from float to integer.");
                    break;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT:
                case HLSL_TYPE_UINT:
                case HLSL_TYPE_BOOL:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &instr->loc, "SM1 cast from double to integer.");
                    break;
            }
            break;

        case HLSL_TYPE_DOUBLE:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;
                    break;

                default:
                    hlsl_fixme(ctx, &instr->loc, "SM1 cast to double.");
                    break;
            }
            break;

        case HLSL_TYPE_BOOL:
            /* Casts to bool should have already been lowered. */
            hlsl_fixme(ctx, &expr->node.loc, "SM1 cast from %s to %s.",
                debug_hlsl_type(ctx, src_type), debug_hlsl_type(ctx, dst_type));
            break;
    }

    return false;
}

static bool sm1_generate_vsir_instr_expr(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_ir_expr *expr)
{
    struct hlsl_ir_node *instr = &expr->node;
    struct hlsl_type *type = instr->data_type;

    if (!hlsl_is_numeric_type(type))
        goto err;

    if (type->e.numeric.type == HLSL_TYPE_DOUBLE && !ctx->double_as_float_alias)
    {
        hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                "The 'double' type is not supported for the %s profile.", ctx->profile->name);
        return false;
    }

    switch (expr->op)
    {
        case HLSL_OP1_ABS:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ABS, 0, 0, true);
            break;

        case HLSL_OP1_CAST:
            return sm1_generate_vsir_instr_expr_cast(ctx, program, expr);

        case HLSL_OP1_COS_REDUCED:
            VKD3D_ASSERT(expr->node.reg.writemask == VKD3DSP_WRITEMASK_0);
            if (!hlsl_type_is_floating_point(type))
                goto err;
            sm1_generate_vsir_instr_expr_sincos(ctx, program, expr);
            break;

        case HLSL_OP1_DSX:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSX, 0, 0, true);
            break;

        case HLSL_OP1_DSY:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSY, 0, 0, true);
            break;

        case HLSL_OP1_EXP2:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            sm1_generate_vsir_instr_expr_per_component_instr_op(ctx, program, expr, VKD3DSIH_EXP);
            break;

        case HLSL_OP1_LOG2:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            sm1_generate_vsir_instr_expr_per_component_instr_op(ctx, program, expr, VKD3DSIH_LOG);
            break;

        case HLSL_OP1_NEG:
            if (type->e.numeric.type == HLSL_TYPE_BOOL)
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, VKD3DSPSM_NEG, 0, true);
            break;

        case HLSL_OP1_RCP:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            sm1_generate_vsir_instr_expr_per_component_instr_op(ctx, program, expr, VKD3DSIH_RCP);
            break;

        case HLSL_OP1_REINTERPRET:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
            break;

        case HLSL_OP1_RSQ:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            sm1_generate_vsir_instr_expr_per_component_instr_op(ctx, program, expr, VKD3DSIH_RSQ);
            break;

        case HLSL_OP1_SAT:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, VKD3DSPDM_SATURATE, true);
            break;

        case HLSL_OP1_SIN_REDUCED:
            VKD3D_ASSERT(expr->node.reg.writemask == VKD3DSP_WRITEMASK_1);
            if (!hlsl_type_is_floating_point(type))
                goto err;
            sm1_generate_vsir_instr_expr_sincos(ctx, program, expr);
            break;

        case HLSL_OP2_ADD:
            if (type->e.numeric.type == HLSL_TYPE_BOOL)
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ADD, 0, 0, true);
            break;

        case HLSL_OP2_DOT:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            switch (expr->operands[0].node->data_type->e.numeric.dimx)
            {
                case 3:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP3, 0, 0, false);
                    break;

                case 4:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP4, 0, 0, false);
                    break;

                default:
                    vkd3d_unreachable();
                    return false;
            }
            break;

        case HLSL_OP2_MAX:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAX, 0, 0, true);
            break;

        case HLSL_OP2_MIN:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MIN, 0, 0, true);
            break;

        case HLSL_OP2_MUL:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MUL, 0, 0, true);
            break;

        case HLSL_OP1_FRACT:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_FRC, 0, 0, true);
            break;

        case HLSL_OP2_LOGIC_AND:
            if (type->e.numeric.type != HLSL_TYPE_BOOL)
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MIN, 0, 0, true);
            break;

        case HLSL_OP2_LOGIC_OR:
            if (type->e.numeric.type != HLSL_TYPE_BOOL)
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAX, 0, 0, true);
            break;

        case HLSL_OP2_SLT:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_SLT, 0, 0, true);
            break;

        case HLSL_OP3_CMP:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_CMP, 0, 0, true);
            break;

        case HLSL_OP3_DP2ADD:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP2ADD, 0, 0, false);
            break;

        case HLSL_OP3_MAD:
            if (!hlsl_type_is_floating_point(type))
                goto err;
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAD, 0, 0, true);
            break;

        default:
            goto err;
    }
    return true;

err:
    hlsl_fixme(ctx, &instr->loc, "SM1 %s expression of type %s.", debug_hlsl_expr_op(expr->op), instr->data_type->name);
    return false;
}

static void sm1_generate_vsir_init_dst_param_from_deref(struct hlsl_ctx *ctx,
        struct vkd3d_shader_dst_param *dst_param, struct hlsl_deref *deref,
        const struct vkd3d_shader_location *loc, unsigned int writemask)
{
    enum vkd3d_shader_register_type type = VKD3DSPR_TEMP;
    struct vkd3d_shader_version version;
    uint32_t register_index;
    struct hlsl_reg reg;

    reg = hlsl_reg_from_deref(ctx, deref);
    register_index = reg.id;
    writemask = hlsl_combine_writemasks(reg.writemask, writemask);

    if (deref->var->is_output_semantic)
    {
        const char *semantic_name = deref->var->semantic.name;

        version.major = ctx->profile->major_version;
        version.minor = ctx->profile->minor_version;
        version.type = ctx->profile->type;

        if (version.type == VKD3D_SHADER_TYPE_PIXEL && version.major == 1)
        {
            type = VKD3DSPR_TEMP;
            register_index = 0;
        }
        else if (!sm1_register_from_semantic_name(&version, semantic_name,
                deref->var->semantic.index, true, NULL, &type, &register_index))
        {
            VKD3D_ASSERT(reg.allocated);
            type = VKD3DSPR_OUTPUT;
            register_index = reg.id;
        }
        else
            writemask = (1u << deref->var->data_type->e.numeric.dimx) - 1;

        if (version.type == VKD3D_SHADER_TYPE_PIXEL && (!ascii_strcasecmp(semantic_name, "PSIZE")
                || (!ascii_strcasecmp(semantic_name, "FOG") && version.major < 3)))
        {
            /* These are always 1-component, but for some reason are written
             * with a writemask containing all components. */
            writemask = VKD3DSP_WRITEMASK_ALL;
        }
    }
    else
        VKD3D_ASSERT(reg.allocated);

    if (type == VKD3DSPR_DEPTHOUT)
    {
        vsir_register_init(&dst_param->reg, type, VKD3D_DATA_FLOAT, 0);
        dst_param->reg.dimension = VSIR_DIMENSION_SCALAR;
    }
    else
    {
        vsir_register_init(&dst_param->reg, type, VKD3D_DATA_FLOAT, 1);
        dst_param->reg.idx[0].offset = register_index;
        dst_param->reg.dimension = VSIR_DIMENSION_VEC4;
    }
    dst_param->write_mask = writemask;

    if (deref->rel_offset.node)
        hlsl_fixme(ctx, loc, "Translate relative addressing on dst register for vsir.");
}

static void sm1_generate_vsir_instr_mova(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_node *instr)
{
    enum vkd3d_shader_opcode opcode = hlsl_version_ge(ctx, 2, 0) ? VKD3DSIH_MOVA : VKD3DSIH_MOV;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(instr->reg.allocated);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, 1)))
        return;

    dst_param = &ins->dst[0];
    vsir_register_init(&dst_param->reg, VKD3DSPR_ADDR, VKD3D_DATA_FLOAT, 0);
    dst_param->write_mask = VKD3DSP_WRITEMASK_0;

    VKD3D_ASSERT(instr->data_type->class <= HLSL_CLASS_VECTOR);
    VKD3D_ASSERT(instr->data_type->e.numeric.dimx == 1);
    vsir_src_from_hlsl_node(&ins->src[0], ctx, instr, VKD3DSP_WRITEMASK_ALL);
}

static struct vkd3d_shader_src_param *sm1_generate_vsir_new_address_src(struct hlsl_ctx *ctx,
        struct vsir_program *program)
{
    struct vkd3d_shader_src_param *idx_src;

    if (!(idx_src = vsir_program_get_src_params(program, 1)))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    memset(idx_src, 0, sizeof(*idx_src));
    vsir_register_init(&idx_src->reg, VKD3DSPR_ADDR, VKD3D_DATA_FLOAT, 0);
    idx_src->reg.dimension = VSIR_DIMENSION_VEC4;
    idx_src->swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
    return idx_src;
}

static void sm1_generate_vsir_init_src_param_from_deref(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct vkd3d_shader_src_param *src_param,
        struct hlsl_deref *deref, uint32_t dst_writemask, const struct vkd3d_shader_location *loc)
{
    enum vkd3d_shader_register_type type = VKD3DSPR_TEMP;
    struct vkd3d_shader_src_param *src_rel_addr = NULL;
    struct vkd3d_shader_version version;
    uint32_t register_index;
    unsigned int writemask;
    struct hlsl_reg reg;

    if (hlsl_type_is_resource(deref->var->data_type))
    {
        unsigned int sampler_offset;

        type = VKD3DSPR_COMBINED_SAMPLER;

        sampler_offset = hlsl_offset_from_deref_safe(ctx, deref);
        register_index = deref->var->regs[HLSL_REGSET_SAMPLERS].index + sampler_offset;
        writemask = VKD3DSP_WRITEMASK_ALL;
    }
    else if (deref->var->is_uniform)
    {
        unsigned int offset = deref->const_offset;

        type = VKD3DSPR_CONST;
        register_index = deref->var->regs[HLSL_REGSET_NUMERIC].id + offset / 4;

        writemask = 0xf & (0xf << (offset % 4));
        if (deref->var->regs[HLSL_REGSET_NUMERIC].writemask)
            writemask = hlsl_combine_writemasks(deref->var->regs[HLSL_REGSET_NUMERIC].writemask, writemask);

        if (deref->rel_offset.node)
        {
            VKD3D_ASSERT(deref_supports_sm1_indirect_addressing(ctx, deref));

            if (!(src_rel_addr = sm1_generate_vsir_new_address_src(ctx, program)))
            {
                ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
                return;
            }
        }
        VKD3D_ASSERT(deref->var->regs[HLSL_REGSET_NUMERIC].allocated);
    }
    else if (deref->var->is_input_semantic)
    {
        version.major = ctx->profile->major_version;
        version.minor = ctx->profile->minor_version;
        version.type = ctx->profile->type;
        if (sm1_register_from_semantic_name(&version, deref->var->semantic.name,
                deref->var->semantic.index, false, NULL, &type, &register_index))
        {
            writemask = (1 << deref->var->data_type->e.numeric.dimx) - 1;
        }
        else
        {
            type = VKD3DSPR_INPUT;

            reg = hlsl_reg_from_deref(ctx, deref);
            register_index = reg.id;
            writemask = reg.writemask;
            VKD3D_ASSERT(reg.allocated);
        }
    }
    else
    {
        type = VKD3DSPR_TEMP;

        reg = hlsl_reg_from_deref(ctx, deref);
        register_index = reg.id;
        writemask = reg.writemask;
    }

    vsir_register_init(&src_param->reg, type, VKD3D_DATA_FLOAT, 1);
    src_param->reg.dimension = VSIR_DIMENSION_VEC4;
    src_param->reg.idx[0].offset = register_index;
    src_param->reg.idx[0].rel_addr = src_rel_addr;
    src_param->swizzle = generate_vsir_get_src_swizzle(writemask, dst_writemask);
}

static void sm1_generate_vsir_instr_load(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_ir_load *load)
{
    struct hlsl_ir_node *instr = &load->node;
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(instr->reg.allocated);

    if (load->src.rel_offset.node)
        sm1_generate_vsir_instr_mova(ctx, program, load->src.rel_offset.node);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    sm1_generate_vsir_init_src_param_from_deref(ctx, program, &ins->src[0],
            &load->src, ins->dst[0].write_mask, &ins->location);
}

static void sm1_generate_vsir_instr_resource_load(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_resource_load *load)
{
    struct hlsl_ir_node *coords = load->coords.node;
    struct hlsl_ir_node *ddx = load->ddx.node;
    struct hlsl_ir_node *ddy = load->ddy.node;
    struct hlsl_ir_node *instr = &load->node;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_shader_opcode opcode;
    unsigned int src_count = 2;
    uint32_t flags = 0;

    VKD3D_ASSERT(instr->reg.allocated);

    switch (load->load_type)
    {
        case HLSL_RESOURCE_SAMPLE:
            opcode = VKD3DSIH_TEX;
            break;

        case HLSL_RESOURCE_SAMPLE_PROJ:
            opcode = VKD3DSIH_TEX;
            flags |= VKD3DSI_TEXLD_PROJECT;
            break;

        case HLSL_RESOURCE_SAMPLE_LOD_BIAS:
            opcode = VKD3DSIH_TEX;
            flags |= VKD3DSI_TEXLD_BIAS;
            break;

        case HLSL_RESOURCE_SAMPLE_GRAD:
            opcode = VKD3DSIH_TEXLDD;
            src_count += 2;
            break;

        default:
            hlsl_fixme(ctx, &instr->loc, "Resource load type %u.", load->load_type);
            return;
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, src_count)))
        return;
    ins->flags = flags;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    src_param = &ins->src[0];
    vsir_src_from_hlsl_node(src_param, ctx, coords, VKD3DSP_WRITEMASK_ALL);

    sm1_generate_vsir_init_src_param_from_deref(ctx, program, &ins->src[1], &load->resource,
            VKD3DSP_WRITEMASK_ALL, &ins->location);

    if (load->load_type == HLSL_RESOURCE_SAMPLE_GRAD)
    {
        src_param = &ins->src[2];
        vsir_src_from_hlsl_node(src_param, ctx, ddx, VKD3DSP_WRITEMASK_ALL);

        src_param = &ins->src[3];
        vsir_src_from_hlsl_node(src_param, ctx, ddy, VKD3DSP_WRITEMASK_ALL);
    }
}

static void generate_vsir_instr_swizzle(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_swizzle *swizzle_instr)
{
    struct hlsl_ir_node *instr = &swizzle_instr->node, *val = swizzle_instr->val.node;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    uint32_t swizzle;

    VKD3D_ASSERT(instr->reg.allocated);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    swizzle = hlsl_swizzle_from_writemask(val->reg.writemask);
    swizzle = hlsl_combine_swizzles(swizzle, swizzle_instr->u.vector, instr->data_type->e.numeric.dimx);
    swizzle = hlsl_map_swizzle(swizzle, ins->dst[0].write_mask);

    src_param = &ins->src[0];
    VKD3D_ASSERT(val->type != HLSL_IR_CONSTANT);
    vsir_register_init(&src_param->reg, VKD3DSPR_TEMP, vsir_data_type_from_hlsl_instruction(ctx, val), 1);
    src_param->reg.idx[0].offset = val->reg.id;
    src_param->reg.dimension = VSIR_DIMENSION_VEC4;
    src_param->swizzle = swizzle;
}

static void sm1_generate_vsir_instr_store(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_ir_store *store)
{
    struct hlsl_ir_node *rhs = store->rhs.node;
    struct hlsl_ir_node *instr = &store->node;
    struct vkd3d_shader_instruction *ins;
    struct vkd3d_shader_src_param *src_param;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return;

    sm1_generate_vsir_init_dst_param_from_deref(ctx, &ins->dst[0], &store->lhs, &ins->location, store->writemask);

    src_param = &ins->src[0];
    vsir_src_from_hlsl_node(src_param, ctx, rhs, ins->dst[0].write_mask);
}

static void sm1_generate_vsir_instr_jump(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_jump *jump)
{
    struct hlsl_ir_node *condition = jump->condition.node;
    struct hlsl_ir_node *instr = &jump->node;
    struct vkd3d_shader_instruction *ins;

    if (jump->type == HLSL_IR_JUMP_DISCARD_NEG)
    {
        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_TEXKILL, 0, 1)))
            return;

        vsir_src_from_hlsl_node(&ins->src[0], ctx, condition, VKD3DSP_WRITEMASK_ALL);
    }
    else
    {
        hlsl_fixme(ctx, &instr->loc, "Jump type %s.", hlsl_jump_type_to_string(jump->type));
    }
}

static void sm1_generate_vsir_block(struct hlsl_ctx *ctx, struct hlsl_block *block, struct vsir_program *program);

static void sm1_generate_vsir_instr_if(struct hlsl_ctx *ctx, struct vsir_program *program, struct hlsl_ir_if *iff)
{
    struct hlsl_ir_node *condition = iff->condition.node;
    struct vkd3d_shader_src_param *src_param;
    struct hlsl_ir_node *instr = &iff->node;
    struct vkd3d_shader_instruction *ins;

    if (hlsl_version_lt(ctx, 2, 1))
    {
        hlsl_fixme(ctx, &instr->loc, "Flatten \"if\" conditionals branches.");
        return;
    }
    VKD3D_ASSERT(condition->data_type->e.numeric.dimx == 1 && condition->data_type->e.numeric.dimy == 1);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_IFC, 0, 2)))
        return;
    ins->flags = VKD3D_SHADER_REL_OP_NE;

    src_param = &ins->src[0];
    vsir_src_from_hlsl_node(src_param, ctx, condition, VKD3DSP_WRITEMASK_ALL);
    src_param->modifiers = 0;

    src_param = &ins->src[1];
    vsir_src_from_hlsl_node(src_param, ctx, condition, VKD3DSP_WRITEMASK_ALL);
    src_param->modifiers = VKD3DSPSM_NEG;

    sm1_generate_vsir_block(ctx, &iff->then_block, program);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_ELSE, 0, 0)))
        return;

    sm1_generate_vsir_block(ctx, &iff->else_block, program);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_ENDIF, 0, 0)))
        return;
}

static void sm1_generate_vsir_block(struct hlsl_ctx *ctx, struct hlsl_block *block, struct vsir_program *program)
{
    struct hlsl_ir_node *instr, *next;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->data_type)
        {
            if (instr->data_type->class != HLSL_CLASS_SCALAR && instr->data_type->class != HLSL_CLASS_VECTOR)
            {
                hlsl_fixme(ctx, &instr->loc, "Class %#x should have been lowered or removed.", instr->data_type->class);
                break;
            }
        }

        switch (instr->type)
        {
            case HLSL_IR_CALL:
                vkd3d_unreachable();

            case HLSL_IR_CONSTANT:
                sm1_generate_vsir_instr_constant(ctx, program, hlsl_ir_constant(instr));
                break;

            case HLSL_IR_EXPR:
                sm1_generate_vsir_instr_expr(ctx, program, hlsl_ir_expr(instr));
                break;

            case HLSL_IR_IF:
                sm1_generate_vsir_instr_if(ctx, program, hlsl_ir_if(instr));
                break;

            case HLSL_IR_JUMP:
                sm1_generate_vsir_instr_jump(ctx, program, hlsl_ir_jump(instr));
                break;

            case HLSL_IR_LOAD:
                sm1_generate_vsir_instr_load(ctx, program, hlsl_ir_load(instr));
                break;

            case HLSL_IR_RESOURCE_LOAD:
                sm1_generate_vsir_instr_resource_load(ctx, program, hlsl_ir_resource_load(instr));
                break;

            case HLSL_IR_STORE:
                sm1_generate_vsir_instr_store(ctx, program, hlsl_ir_store(instr));
                break;

            case HLSL_IR_SWIZZLE:
                generate_vsir_instr_swizzle(ctx, program, hlsl_ir_swizzle(instr));
                break;

            default:
                hlsl_fixme(ctx, &instr->loc, "Instruction type %s.", hlsl_node_type_to_string(instr->type));
                break;
        }
    }
}

static void sm1_generate_vsir(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func,
        uint64_t config_flags, struct vsir_program *program)
{
    struct vkd3d_shader_version version = {0};
    struct hlsl_block block;

    version.major = ctx->profile->major_version;
    version.minor = ctx->profile->minor_version;
    version.type = ctx->profile->type;
    if (!vsir_program_init(program, NULL, &version, 0, VSIR_CF_STRUCTURED, VSIR_NORMALISED_SM4))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }

    program->temp_count = allocate_temp_registers(ctx, entry_func);
    if (ctx->result)
        return;

    generate_vsir_signature(ctx, program, entry_func);

    hlsl_block_init(&block);
    sm1_generate_vsir_constant_defs(ctx, program, &block);
    sm1_generate_vsir_sampler_dcls(ctx, program, &block);
    list_move_head(&entry_func->body.instrs, &block.instrs);

    sm1_generate_vsir_block(ctx, &entry_func->body, program);
}

D3DXPARAMETER_CLASS hlsl_sm1_class(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_ARRAY:
            return hlsl_sm1_class(type->e.array.type);
        case HLSL_CLASS_MATRIX:
            VKD3D_ASSERT(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);
            if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
                return D3DXPC_MATRIX_COLUMNS;
            else
                return D3DXPC_MATRIX_ROWS;
        case HLSL_CLASS_SCALAR:
            return D3DXPC_SCALAR;
        case HLSL_CLASS_STRUCT:
            return D3DXPC_STRUCT;
        case HLSL_CLASS_VECTOR:
            return D3DXPC_VECTOR;
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_VERTEX_SHADER:
            return D3DXPC_OBJECT;
        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_STREAM_OUTPUT:
        case HLSL_CLASS_NULL:
            break;
    }

    vkd3d_unreachable();
}

D3DXPARAMETER_TYPE hlsl_sm1_base_type(const struct hlsl_type *type, bool is_combined_sampler)
{
    enum hlsl_type_class class = type->class;

    if (is_combined_sampler)
        class = HLSL_CLASS_TEXTURE;

    switch (class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            switch (type->e.numeric.type)
            {
                case HLSL_TYPE_BOOL:
                    return D3DXPT_BOOL;
                /* Actually double behaves differently depending on DLL version:
                 * For <= 36, it maps to D3DXPT_FLOAT.
                 * For 37-40, it maps to zero (D3DXPT_VOID).
                 * For >= 41, it maps to 39, which is D3D_SVT_DOUBLE (note D3D_SVT_*
                 * values are mostly compatible with D3DXPT_*).
                 * However, the latter two cases look like bugs, and a reasonable
                 * application certainly wouldn't know what to do with them.
                 * For fx_2_0 it's always D3DXPT_FLOAT regardless of DLL version. */
                case HLSL_TYPE_DOUBLE:
                case HLSL_TYPE_FLOAT:
                case HLSL_TYPE_HALF:
                    return D3DXPT_FLOAT;
                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    return D3DXPT_INT;
                /* Minimum-precision types are not supported until 46, but at
                 * that point they do the same thing, and return sm4 types. */
                case HLSL_TYPE_MIN16UINT:
                    return 0x39;
            }
            break;

        case HLSL_CLASS_SAMPLER:
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
                    vkd3d_unreachable();
            }
            break;

        case HLSL_CLASS_TEXTURE:
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
                    vkd3d_unreachable();
            }
            break;

        case HLSL_CLASS_ARRAY:
            return hlsl_sm1_base_type(type->e.array.type, is_combined_sampler);

        case HLSL_CLASS_STRUCT:
            return D3DXPT_VOID;

        case HLSL_CLASS_STRING:
            return D3DXPT_STRING;

        case HLSL_CLASS_PIXEL_SHADER:
            return D3DXPT_PIXELSHADER;

        case HLSL_CLASS_VERTEX_SHADER:
            return D3DXPT_VERTEXSHADER;

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_STREAM_OUTPUT:
        case HLSL_CLASS_NULL:
            break;
    }

    vkd3d_unreachable();
}

static void write_sm1_type(struct vkd3d_bytecode_buffer *buffer,
        struct hlsl_type *type, bool is_combined_sampler, unsigned int ctab_start)
{
    const struct hlsl_type *array_type = hlsl_get_multiarray_element_type(type);
    unsigned int array_size = hlsl_get_multiarray_size(type);
    struct hlsl_struct_field *field;
    size_t i;

    if (type->bytecode_offset)
        return;

    if (array_type->class == HLSL_CLASS_STRUCT)
    {
        unsigned int field_count = array_type->e.record.field_count;
        size_t fields_offset;

        for (i = 0; i < field_count; ++i)
        {
            field = &array_type->e.record.fields[i];
            field->name_bytecode_offset = put_string(buffer, field->name);
            write_sm1_type(buffer, field->type, false, ctab_start);
        }

        fields_offset = bytecode_align(buffer) - ctab_start;

        for (i = 0; i < field_count; ++i)
        {
            field = &array_type->e.record.fields[i];
            put_u32(buffer, field->name_bytecode_offset - ctab_start);
            put_u32(buffer, field->type->bytecode_offset - ctab_start);
        }

        type->bytecode_offset = put_u32(buffer, vkd3d_make_u32(D3DXPC_STRUCT, D3DXPT_VOID));
        put_u32(buffer, vkd3d_make_u32(1, hlsl_type_component_count(array_type)));
        put_u32(buffer, vkd3d_make_u32(array_size, field_count));
        put_u32(buffer, fields_offset);
    }
    else
    {
        type->bytecode_offset = put_u32(buffer,
                vkd3d_make_u32(hlsl_sm1_class(type), hlsl_sm1_base_type(array_type, is_combined_sampler)));
        if (hlsl_is_numeric_type(array_type))
            put_u32(buffer, vkd3d_make_u32(array_type->e.numeric.dimy, array_type->e.numeric.dimx));
        else
            put_u32(buffer, vkd3d_make_u32(1, 1));
        put_u32(buffer, vkd3d_make_u32(array_size, 0));
        put_u32(buffer, 1);
    }
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
    {
        if (var->is_uniform)
            sm1_sort_extern(&sorted, var);
    }
    list_move_tail(&ctx->extern_vars, &sorted);
}

static void write_sm1_uniforms(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer)
{
    size_t ctab_start, vars_offset, vars_start, creator_offset, offset;
    unsigned int uniform_count = 0, r;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            if (var->semantic.name || !var->regs[r].allocated || !var->last_read)
                continue;

            ++uniform_count;

            if (var->is_param && var->is_uniform)
            {
                char *new_name;

                if (!(new_name = hlsl_sprintf_alloc(ctx, "$%s", var->name)))
                    return;
                vkd3d_free((char *)var->name);
                var->name = new_name;
            }
        }
    }

    sm1_sort_externs(ctx);

    ctab_start = put_u32(buffer, 7 * sizeof(uint32_t)); /* CTAB header size. */
    creator_offset = put_u32(buffer, 0);
    if (ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX)
        put_u32(buffer, D3DVS_VERSION(ctx->profile->major_version, ctx->profile->minor_version));
    else
        put_u32(buffer, D3DPS_VERSION(ctx->profile->major_version, ctx->profile->minor_version));
    put_u32(buffer, uniform_count);
    vars_offset = put_u32(buffer, 0);
    put_u32(buffer, 0); /* FIXME: flags */
    put_u32(buffer, 0); /* FIXME: target string */

    vars_start = bytecode_align(buffer);
    set_u32(buffer, vars_offset, vars_start - ctab_start);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            if (var->semantic.name || !var->regs[r].allocated || !var->last_read)
                continue;

            put_u32(buffer, 0); /* name */
            if (r == HLSL_REGSET_NUMERIC)
            {
                put_u32(buffer, vkd3d_make_u32(D3DXRS_FLOAT4, var->regs[r].id));
                put_u32(buffer, var->bind_count[r]);
            }
            else
            {
                put_u32(buffer, vkd3d_make_u32(D3DXRS_SAMPLER, var->regs[r].index));
                put_u32(buffer, var->bind_count[r]);
            }
            put_u32(buffer, 0); /* type */
            put_u32(buffer, 0); /* default value */
        }
    }

    uniform_count = 0;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            size_t var_offset, name_offset;

            if (var->semantic.name || !var->regs[r].allocated || !var->last_read)
                continue;

            var_offset = vars_start + (uniform_count * 5 * sizeof(uint32_t));

            name_offset = put_string(buffer, var->name);
            set_u32(buffer, var_offset, name_offset - ctab_start);

            write_sm1_type(buffer, var->data_type, var->is_combined_sampler, ctab_start);
            set_u32(buffer, var_offset + 3 * sizeof(uint32_t), var->data_type->bytecode_offset - ctab_start);

            if (var->default_values)
            {
                unsigned int reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];
                unsigned int comp_count = hlsl_type_component_count(var->data_type);
                unsigned int default_value_offset;
                unsigned int k;

                default_value_offset = bytecode_reserve_bytes(buffer, reg_size * sizeof(uint32_t));
                set_u32(buffer, var_offset + 4 * sizeof(uint32_t), default_value_offset - ctab_start);

                for (k = 0; k < comp_count; ++k)
                {
                    struct hlsl_type *comp_type = hlsl_type_get_component_type(ctx, var->data_type, k);
                    unsigned int comp_offset;
                    enum hlsl_regset regset;

                    comp_offset = hlsl_type_get_component_offset(ctx, var->data_type, k, &regset);
                    if (regset == HLSL_REGSET_NUMERIC)
                    {
                        union
                        {
                            uint32_t u;
                            float f;
                        } uni = {0};

                        switch (comp_type->e.numeric.type)
                        {
                            case HLSL_TYPE_DOUBLE:
                                if (ctx->double_as_float_alias)
                                    uni.u = var->default_values[k].number.u;
                                else
                                    uni.u = 0;
                                break;

                            case HLSL_TYPE_INT:
                                uni.f = var->default_values[k].number.i;
                                break;

                            case HLSL_TYPE_MIN16UINT:
                            case HLSL_TYPE_UINT:
                            case HLSL_TYPE_BOOL:
                                uni.f = var->default_values[k].number.u;
                                break;

                            case HLSL_TYPE_HALF:
                            case HLSL_TYPE_FLOAT:
                                uni.u = var->default_values[k].number.u;
                                break;
                        }

                        set_u32(buffer, default_value_offset + comp_offset * sizeof(uint32_t), uni.u);
                    }
                }
            }

            ++uniform_count;
        }
    }

    offset = put_string(buffer, vkd3d_shader_get_version(NULL, NULL));
    set_u32(buffer, creator_offset, offset - ctab_start);
}

static void sm1_generate_ctab(struct hlsl_ctx *ctx, struct vkd3d_shader_code *ctab)
{
    struct vkd3d_bytecode_buffer buffer = {0};

    write_sm1_uniforms(ctx, &buffer);
    if (buffer.status)
    {
        vkd3d_free(buffer.data);
        ctx->result = buffer.status;
        return;
    }
    ctab->code = buffer.data;
    ctab->size = buffer.size;
}

static void sm4_generate_vsir_instr_dcl_semantic(struct hlsl_ctx *ctx, struct vsir_program *program,
        const struct hlsl_ir_var *var, struct hlsl_block *block, const struct vkd3d_shader_location *loc)
{
    const struct vkd3d_shader_version *version = &program->shader_version;
    const bool is_primitive = hlsl_type_is_primitive_array(var->data_type);
    const bool output = var->is_output_semantic;
    enum vkd3d_shader_sysval_semantic semantic;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_shader_register_type type;
    enum vkd3d_shader_opcode opcode;
    unsigned int idx = 0;
    uint32_t write_mask;
    bool has_idx;

    sm4_sysval_semantic_from_semantic_name(&semantic, version, ctx->semantic_compat_mapping, ctx->domain,
            var->semantic.name, var->semantic.index, output, ctx->is_patch_constant_func, is_primitive);
    if (semantic == ~0u)
        semantic = VKD3D_SHADER_SV_NONE;

    if (var->is_input_semantic)
    {
        switch (semantic)
        {
            case VKD3D_SHADER_SV_NONE:
                opcode = (version->type == VKD3D_SHADER_TYPE_PIXEL)
                        ? VKD3DSIH_DCL_INPUT_PS : VKD3DSIH_DCL_INPUT;
                break;

            case VKD3D_SHADER_SV_PRIMITIVE_ID:
                if (version->type == VKD3D_SHADER_TYPE_PIXEL)
                    opcode = VKD3DSIH_DCL_INPUT_PS_SGV;
                else if (version->type == VKD3D_SHADER_TYPE_GEOMETRY)
                    opcode = VKD3DSIH_DCL_INPUT;
                else
                    opcode = VKD3DSIH_DCL_INPUT_SGV;
                break;

            case VKD3D_SHADER_SV_INSTANCE_ID:
            case VKD3D_SHADER_SV_IS_FRONT_FACE:
            case VKD3D_SHADER_SV_SAMPLE_INDEX:
            case VKD3D_SHADER_SV_VERTEX_ID:
                opcode = (version->type == VKD3D_SHADER_TYPE_PIXEL)
                        ? VKD3DSIH_DCL_INPUT_PS_SGV : VKD3DSIH_DCL_INPUT_SGV;
                break;

            default:
                if (version->type == VKD3D_SHADER_TYPE_PIXEL)
                    opcode = VKD3DSIH_DCL_INPUT_PS_SIV;
                else if (is_primitive && version->type != VKD3D_SHADER_TYPE_GEOMETRY)
                    opcode = VKD3DSIH_DCL_INPUT;
                else
                    opcode = VKD3DSIH_DCL_INPUT_SIV;
                break;
        }
    }
    else
    {
        if (semantic == VKD3D_SHADER_SV_NONE || version->type == VKD3D_SHADER_TYPE_PIXEL
                || (version->type == VKD3D_SHADER_TYPE_HULL && !ctx->is_patch_constant_func))
            opcode = VKD3DSIH_DCL_OUTPUT;
        else
            opcode = VKD3DSIH_DCL_OUTPUT_SIV;
    }

    if (sm4_register_from_semantic_name(version, var->semantic.name, output, &type, &has_idx))
    {
        if (has_idx)
            idx = var->semantic.index;
        write_mask = (1u << var->data_type->e.numeric.dimx) - 1;
    }
    else
    {
        type = sm4_get_semantic_register_type(version->type, ctx->is_patch_constant_func, var);
        has_idx = true;
        idx = var->regs[HLSL_REGSET_NUMERIC].id;
        write_mask = var->regs[HLSL_REGSET_NUMERIC].writemask;
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, loc, opcode, 0, 0)))
        return;

    if (opcode == VKD3DSIH_DCL_OUTPUT)
    {
        VKD3D_ASSERT(semantic == VKD3D_SHADER_SV_NONE || semantic == VKD3D_SHADER_SV_TARGET
                || version->type == VKD3D_SHADER_TYPE_HULL || type != VKD3DSPR_OUTPUT);
        dst_param = &ins->declaration.dst;
    }
    else if (opcode == VKD3DSIH_DCL_INPUT || opcode == VKD3DSIH_DCL_INPUT_PS)
    {
        VKD3D_ASSERT(semantic == VKD3D_SHADER_SV_NONE || is_primitive || version->type == VKD3D_SHADER_TYPE_GEOMETRY);
        dst_param = &ins->declaration.dst;
    }
    else
    {
        VKD3D_ASSERT(semantic != VKD3D_SHADER_SV_NONE);
        ins->declaration.register_semantic.sysval_semantic = vkd3d_siv_from_sysval_indexed(semantic,
                var->semantic.index);
        dst_param = &ins->declaration.register_semantic.reg;
    }

    if (is_primitive)
    {
        VKD3D_ASSERT(has_idx);
        vsir_register_init(&dst_param->reg, type, VKD3D_DATA_FLOAT, 2);
        dst_param->reg.idx[0].offset = var->data_type->e.array.elements_count;
        dst_param->reg.idx[1].offset = idx;
    }
    else if (has_idx)
    {
        vsir_register_init(&dst_param->reg, type, VKD3D_DATA_FLOAT, 1);
        dst_param->reg.idx[0].offset = idx;
    }
    else
    {
        vsir_register_init(&dst_param->reg, type, VKD3D_DATA_FLOAT, 0);
    }

    if (shader_sm4_is_scalar_register(&dst_param->reg))
        dst_param->reg.dimension = VSIR_DIMENSION_SCALAR;
    else
        dst_param->reg.dimension = VSIR_DIMENSION_VEC4;

    dst_param->write_mask = write_mask;

    if (var->is_input_semantic && version->type == VKD3D_SHADER_TYPE_PIXEL)
        ins->flags = sm4_get_interpolation_mode(var->data_type, var->storage_modifiers);
}

static void sm4_generate_vsir_instr_dcl_temps(struct hlsl_ctx *ctx, struct vsir_program *program,
        uint32_t temp_count, struct hlsl_block *block, const struct vkd3d_shader_location *loc)
{
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, loc, VKD3DSIH_DCL_TEMPS, 0, 0)))
        return;

    ins->declaration.count = temp_count;
}

static void sm4_generate_vsir_instr_dcl_indexable_temp(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_block *block, uint32_t idx,
        uint32_t size, uint32_t comp_count, const struct vkd3d_shader_location *loc)
{
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, loc, VKD3DSIH_DCL_INDEXABLE_TEMP, 0, 0)))
        return;

    ins->declaration.indexable_temp.register_idx = idx;
    ins->declaration.indexable_temp.register_size = size;
    ins->declaration.indexable_temp.alignment = 0;
    ins->declaration.indexable_temp.data_type = VKD3D_DATA_FLOAT;
    ins->declaration.indexable_temp.component_count = comp_count;
    ins->declaration.indexable_temp.has_function_scope = false;
}

static bool type_is_float(const struct hlsl_type *type)
{
    return type->e.numeric.type == HLSL_TYPE_FLOAT || type->e.numeric.type == HLSL_TYPE_HALF;
}

static void sm4_generate_vsir_cast_from_bool(struct hlsl_ctx *ctx, struct vsir_program *program,
        const struct hlsl_ir_expr *expr, uint32_t bits)
{
    struct hlsl_ir_node *operand = expr->operands[0].node;
    const struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct hlsl_constant_value value = {0};
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(instr->reg.allocated);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_AND, 1, 2)))
        return;

    dst_param = &ins->dst[0];
    vsir_dst_from_hlsl_node(dst_param, ctx, instr);

    vsir_src_from_hlsl_node(&ins->src[0], ctx, operand, dst_param->write_mask);

    value.u[0].u = bits;
    vsir_src_from_hlsl_constant_value(&ins->src[1], ctx, &value, VKD3D_DATA_UINT, 1, 0);
}

static bool sm4_generate_vsir_instr_expr_cast(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr)
{
    const struct hlsl_ir_node *arg1 = expr->operands[0].node;
    const struct hlsl_type *dst_type = expr->node.data_type;
    const struct hlsl_type *src_type = arg1->data_type;

    static const union
    {
        uint32_t u;
        float f;
    } one = { .f = 1.0 };

    /* Narrowing casts were already lowered. */
    VKD3D_ASSERT(src_type->e.numeric.dimx == dst_type->e.numeric.dimx);

    switch (dst_type->e.numeric.type)
    {
        case HLSL_TYPE_HALF:
        case HLSL_TYPE_FLOAT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ITOF, 0, 0, true);
                    return true;

                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_UTOF, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                    sm4_generate_vsir_cast_from_bool(ctx, program, expr, one.u);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to float.");
                    return false;
            }
            break;

        case HLSL_TYPE_INT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_FTOI, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                    sm4_generate_vsir_cast_from_bool(ctx, program, expr, 1u);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to int.");
                    return false;
            }
            break;

        case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
        case HLSL_TYPE_UINT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_FTOU, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                    sm4_generate_vsir_cast_from_bool(ctx, program, expr, 1u);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to uint.");
                    return false;
            }
            break;

        case HLSL_TYPE_DOUBLE:
            hlsl_fixme(ctx, &expr->node.loc, "SM4 cast to double.");
            return false;

        case HLSL_TYPE_BOOL:
            /* Casts to bool should have already been lowered. */
            break;
    }

    vkd3d_unreachable();
}

static void sm4_generate_vsir_expr_with_two_destinations(struct hlsl_ctx *ctx, struct vsir_program *program,
        enum vkd3d_shader_opcode opcode, const struct hlsl_ir_expr *expr, unsigned int dst_idx)
{
    const struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_instruction *ins;
    unsigned int i, src_count;

    VKD3D_ASSERT(instr->reg.allocated);

    for (i = 0; i < HLSL_MAX_OPERANDS; ++i)
    {
        if (expr->operands[i].node)
            src_count = i + 1;
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 2, src_count)))
        return;

    dst_param = &ins->dst[dst_idx];
    vsir_dst_from_hlsl_node(dst_param, ctx, instr);

    vsir_dst_param_init_null(&ins->dst[1 - dst_idx]);

    for (i = 0; i < src_count; ++i)
        vsir_src_from_hlsl_node(&ins->src[i], ctx, expr->operands[i].node, dst_param->write_mask);
}

static void sm4_generate_vsir_rcp_using_div(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_expr *expr)
{
    struct hlsl_ir_node *operand = expr->operands[0].node;
    const struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct hlsl_constant_value value = {0};
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(type_is_float(expr->node.data_type));

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_DIV, 1, 2)))
        return;

    dst_param = &ins->dst[0];
    vsir_dst_from_hlsl_node(dst_param, ctx, instr);

    value.u[0].f = 1.0f;
    value.u[1].f = 1.0f;
    value.u[2].f = 1.0f;
    value.u[3].f = 1.0f;
    vsir_src_from_hlsl_constant_value(&ins->src[0], ctx, &value,
            VKD3D_DATA_FLOAT, instr->data_type->e.numeric.dimx, dst_param->write_mask);

    vsir_src_from_hlsl_node(&ins->src[1], ctx, operand, dst_param->write_mask);
}

static bool sm4_generate_vsir_instr_expr(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr, const char *dst_type_name)
{
    const struct hlsl_type *dst_type = expr->node.data_type;
    const struct hlsl_type *src_type = NULL;

    VKD3D_ASSERT(expr->node.reg.allocated);
    if (expr->operands[0].node)
        src_type = expr->operands[0].node->data_type;

    switch (expr->op)
    {
        case HLSL_OP0_RASTERIZER_SAMPLE_COUNT:
            sm4_generate_vsir_rasterizer_sample_count(ctx, program, expr);
            return true;

        case HLSL_OP1_ABS:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, VKD3DSPSM_ABS, 0, true);
            return true;

        case HLSL_OP1_BIT_NOT:
            VKD3D_ASSERT(hlsl_type_is_integer(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_NOT, 0, 0, true);
            return true;

        case HLSL_OP1_CAST:
            return sm4_generate_vsir_instr_expr_cast(ctx, program, expr);

        case HLSL_OP1_CEIL:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ROUND_PI, 0, 0, true);
            return true;

        case HLSL_OP1_COS:
            VKD3D_ASSERT(type_is_float(dst_type));
            sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_SINCOS, expr, 1);
            return true;

        case HLSL_OP1_DSX:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSX, 0, 0, true);
            return true;

        case HLSL_OP1_DSX_COARSE:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSX_COARSE, 0, 0, true);
            return true;

        case HLSL_OP1_DSX_FINE:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSX_FINE, 0, 0, true);
            return true;

        case HLSL_OP1_DSY:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSY, 0, 0, true);
            return true;

        case HLSL_OP1_DSY_COARSE:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSY_COARSE, 0, 0, true);
            return true;

        case HLSL_OP1_DSY_FINE:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSY_FINE, 0, 0, true);
            return true;

        case HLSL_OP1_EXP2:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_EXP, 0, 0, true);
            return true;

        case HLSL_OP1_F16TOF32:
            VKD3D_ASSERT(type_is_float(dst_type));
            VKD3D_ASSERT(hlsl_version_ge(ctx, 5, 0));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_F16TOF32, 0, 0, true);
            return true;

        case HLSL_OP1_F32TOF16:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_UINT);
            VKD3D_ASSERT(hlsl_version_ge(ctx, 5, 0));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_F32TOF16, 0, 0, true);
            return true;

        case HLSL_OP1_FLOOR:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ROUND_NI, 0, 0, true);
            return true;

        case HLSL_OP1_FRACT:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_FRC, 0, 0, true);
            return true;

        case HLSL_OP1_LOG2:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_LOG, 0, 0, true);
            return true;

        case HLSL_OP1_LOGIC_NOT:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_NOT, 0, 0, true);
            return true;

        case HLSL_OP1_NEG:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, VKD3DSPSM_NEG, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_INEG, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s negation expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP1_RCP:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    /* SM5 comes with a RCP opcode */
                    if (hlsl_version_ge(ctx, 5, 0))
                        generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_RCP, 0, 0, true);
                    else
                        sm4_generate_vsir_rcp_using_div(ctx, program, expr);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s rcp expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP1_REINTERPRET:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
            return true;

        case HLSL_OP1_ROUND:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ROUND_NE, 0, 0, true);
            return true;

        case HLSL_OP1_RSQ:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_RSQ, 0, 0, true);
            return true;

        case HLSL_OP1_SAT:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, VKD3DSPDM_SATURATE, true);
            return true;

        case HLSL_OP1_SIN:
            VKD3D_ASSERT(type_is_float(dst_type));
            sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_SINCOS, expr, 0);
            return true;

        case HLSL_OP1_SQRT:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_SQRT, 0, 0, true);
            return true;

        case HLSL_OP1_TRUNC:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ROUND_Z, 0, 0, true);
            return true;

        case HLSL_OP2_ADD:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ADD, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IADD, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s addition expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_BIT_AND:
            VKD3D_ASSERT(hlsl_type_is_integer(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_AND, 0, 0, true);
            return true;

        case HLSL_OP2_BIT_OR:
            VKD3D_ASSERT(hlsl_type_is_integer(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_OR, 0, 0, true);
            return true;

        case HLSL_OP2_BIT_XOR:
            VKD3D_ASSERT(hlsl_type_is_integer(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_XOR, 0, 0, true);
            return true;

        case HLSL_OP2_DIV:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DIV, 0, 0, true);
                    return true;

                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_UDIV, expr, 0);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s division expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_DOT:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    switch (expr->operands[0].node->data_type->e.numeric.dimx)
                    {
                        case 4:
                            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP4, 0, 0, false);
                            return true;

                        case 3:
                            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP3, 0, 0, false);
                            return true;

                        case 2:
                            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP2, 0, 0, false);
                            return true;

                        case 1:
                        default:
                            vkd3d_unreachable();
                    }

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s dot expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_EQUAL:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);

            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_EQO, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IEQ, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 equality between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    return false;
            }

        case HLSL_OP2_GEQUAL:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);

            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_GEO, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IGE, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_UGE, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 greater-than-or-equal between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    return false;
            }

        case HLSL_OP2_LESS:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);

            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_LTO, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ILT, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ULT, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 less-than between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    return false;
            }

        case HLSL_OP2_LOGIC_AND:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_AND, 0, 0, true);
            return true;

        case HLSL_OP2_LOGIC_OR:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_OR, 0, 0, true);
            return true;

        case HLSL_OP2_LSHIFT:
            VKD3D_ASSERT(hlsl_type_is_integer(dst_type));
            VKD3D_ASSERT(dst_type->e.numeric.type != HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ISHL, 0, 0, true);
            return true;

        case HLSL_OP3_MAD:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAD, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IMAD, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s MAD expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_MAX:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAX, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IMAX, 0, 0, true);
                    return true;

                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_UMAX, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s maximum expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_MIN:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MIN, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IMIN, 0, 0, true);
                    return true;

                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_UMIN, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s minimum expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_MOD:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_UDIV, expr, 1);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s modulus expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_MUL:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MUL, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    /* Using IMUL instead of UMUL because we're taking the low
                     * bits, and the native compiler generates IMUL. */
                    sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_IMUL, expr, 1);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s multiplication expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_NEQUAL:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);

            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_NEU, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_INT:
                case HLSL_TYPE_MIN16UINT: /* FIXME: Needs minimum-precision annotations. */
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_INE, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 inequality between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    return false;
            }

        case HLSL_OP2_RSHIFT:
            VKD3D_ASSERT(hlsl_type_is_integer(dst_type));
            VKD3D_ASSERT(dst_type->e.numeric.type != HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr,
                    dst_type->e.numeric.type == HLSL_TYPE_INT ? VKD3DSIH_ISHR : VKD3DSIH_USHR, 0, 0, true);
            return true;

        case HLSL_OP3_TERNARY:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOVC, 0, 0, true);
            return true;

        default:
            hlsl_fixme(ctx, &expr->node.loc, "SM4 %s expression.", debug_hlsl_expr_op(expr->op));
            return false;
    }
}

static bool sm4_generate_vsir_instr_store(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_store *store)
{
    struct hlsl_ir_node *instr = &store->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return false;

    dst_param = &ins->dst[0];
    if (!sm4_generate_vsir_init_dst_param_from_deref(ctx, program,
            dst_param, &store->lhs, &instr->loc, store->writemask))
        return false;

    src_param = &ins->src[0];
    vsir_src_from_hlsl_node(src_param, ctx, store->rhs.node, dst_param->write_mask);

    return true;
}

/* Does this variable's data come directly from the API user, rather than
 * being temporary or from a previous shader stage? I.e. is it a uniform or
 * VS input? */
static bool var_is_user_input(const struct vkd3d_shader_version *version, const struct hlsl_ir_var *var)
{
    if (var->is_uniform)
        return true;

    return var->is_input_semantic && version->type == VKD3D_SHADER_TYPE_VERTEX;
}

static bool sm4_generate_vsir_instr_load(struct hlsl_ctx *ctx, struct vsir_program *program, struct hlsl_ir_load *load)
{
    const struct vkd3d_shader_version *version = &program->shader_version;
    const struct hlsl_type *type = load->node.data_type;
    struct vkd3d_shader_dst_param *dst_param;
    struct hlsl_ir_node *instr = &load->node;
    struct vkd3d_shader_instruction *ins;
    struct hlsl_constant_value value;

    VKD3D_ASSERT(hlsl_is_numeric_type(type));
    if (type->e.numeric.type == HLSL_TYPE_BOOL && var_is_user_input(version, load->src.var))
    {
        /* Uniform bools can be specified as anything, but internal bools
         * always have 0 for false and ~0 for true. Normalise that here. */

        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOVC, 1, 3)))
            return false;

        dst_param = &ins->dst[0];
        vsir_dst_from_hlsl_node(dst_param, ctx, instr);

        if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program,
                &ins->src[0], &load->src, dst_param->write_mask, &instr->loc))
            return false;

        memset(&value, 0xff, sizeof(value));
        vsir_src_from_hlsl_constant_value(&ins->src[1], ctx, &value,
                VKD3D_DATA_UINT, type->e.numeric.dimx, dst_param->write_mask);
        memset(&value, 0x00, sizeof(value));
        vsir_src_from_hlsl_constant_value(&ins->src[2], ctx, &value,
                VKD3D_DATA_UINT, type->e.numeric.dimx, dst_param->write_mask);
    }
    else
    {
        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
            return false;

        dst_param = &ins->dst[0];
        vsir_dst_from_hlsl_node(dst_param, ctx, instr);

        if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program,
                &ins->src[0], &load->src, dst_param->write_mask, &instr->loc))
            return false;
    }
    return true;
}

static bool sm4_generate_vsir_instr_resource_store(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_resource_store *store)
{
    struct hlsl_type *resource_type = hlsl_deref_get_type(ctx, &store->resource);
    struct hlsl_ir_node *coords = store->coords.node, *value = store->value.node;
    struct hlsl_ir_node *instr = &store->node;
    struct vkd3d_shader_instruction *ins;
    unsigned int writemask;

    if (store->store_type != HLSL_RESOURCE_STORE)
    {
        enum vkd3d_shader_opcode opcode = store->store_type == HLSL_RESOURCE_STREAM_APPEND
                ? VKD3DSIH_EMIT : VKD3DSIH_CUT;

        VKD3D_ASSERT(!store->value.node && !store->coords.node);
        VKD3D_ASSERT(store->resource.var->regs[HLSL_REGSET_STREAM_OUTPUTS].allocated);

        if (store->resource.var->regs[HLSL_REGSET_STREAM_OUTPUTS].index)
        {
            hlsl_fixme(ctx, &instr->loc, "Stream output operation with a nonzero stream index.");
            return false;
        }

        ins = generate_vsir_add_program_instruction(ctx, program, &store->node.loc, opcode, 0, 0);
        return !!ins;
    }

    if (!store->resource.var->is_uniform)
    {
        hlsl_fixme(ctx, &store->node.loc, "Store to non-uniform resource variable.");
        return false;
    }

    if (resource_type->sampler_dim == HLSL_SAMPLER_DIM_STRUCTURED_BUFFER)
    {
        hlsl_fixme(ctx, &store->node.loc, "Structured buffers store is not implemented.");
        return false;
    }

    if (resource_type->sampler_dim == HLSL_SAMPLER_DIM_RAW_BUFFER)
    {
        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_STORE_RAW, 1, 2)))
            return false;

        writemask = vkd3d_write_mask_from_component_count(value->data_type->e.numeric.dimx);
        if (!sm4_generate_vsir_init_dst_param_from_deref(ctx, program,
                &ins->dst[0], &store->resource, &instr->loc, writemask))
            return false;
    }
    else
    {
        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_STORE_UAV_TYPED, 1, 2)))
            return false;

        if (!sm4_generate_vsir_init_dst_param_from_deref(ctx, program,
                &ins->dst[0], &store->resource, &instr->loc, VKD3DSP_WRITEMASK_ALL))
            return false;
    }

    vsir_src_from_hlsl_node(&ins->src[0], ctx, coords, VKD3DSP_WRITEMASK_ALL);
    vsir_src_from_hlsl_node(&ins->src[1], ctx, value, VKD3DSP_WRITEMASK_ALL);

    return true;
}

static bool sm4_generate_vsir_validate_texel_offset_aoffimmi(const struct hlsl_ir_node *texel_offset)
{
    struct hlsl_ir_constant *offset;

    VKD3D_ASSERT(texel_offset);
    if (texel_offset->type != HLSL_IR_CONSTANT)
        return false;
    offset = hlsl_ir_constant(texel_offset);

    if (offset->value.u[0].i < -8 || offset->value.u[0].i > 7)
        return false;
    if (offset->node.data_type->e.numeric.dimx > 1 && (offset->value.u[1].i < -8 || offset->value.u[1].i > 7))
        return false;
    if (offset->node.data_type->e.numeric.dimx > 2 && (offset->value.u[2].i < -8 || offset->value.u[2].i > 7))
        return false;
    return true;
}

static void sm4_generate_vsir_encode_texel_offset_as_aoffimmi(
        struct vkd3d_shader_instruction *ins, const struct hlsl_ir_node *texel_offset)
{
    struct hlsl_ir_constant *offset;

    if (!texel_offset)
        return;
    offset = hlsl_ir_constant(texel_offset);

    ins->texel_offset.u = offset->value.u[0].i;
    ins->texel_offset.v = 0;
    ins->texel_offset.w = 0;
    if (offset->node.data_type->e.numeric.dimx > 1)
        ins->texel_offset.v = offset->value.u[1].i;
    if (offset->node.data_type->e.numeric.dimx > 2)
        ins->texel_offset.w = offset->value.u[2].i;
}

static bool sm4_generate_vsir_instr_ld(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_resource_load *load)
{
    const struct hlsl_type *resource_type = hlsl_deref_get_type(ctx, &load->resource);
    bool uav = (hlsl_deref_get_regset(ctx, &load->resource) == HLSL_REGSET_UAVS);
    const struct vkd3d_shader_version *version = &program->shader_version;
    bool raw = resource_type->sampler_dim == HLSL_SAMPLER_DIM_RAW_BUFFER;
    const struct hlsl_ir_node *sample_index = load->sample_index.node;
    const struct hlsl_ir_node *texel_offset = load->texel_offset.node;
    const struct hlsl_ir_node *coords = load->coords.node;
    unsigned int coords_writemask = VKD3DSP_WRITEMASK_ALL;
    const struct hlsl_deref *resource = &load->resource;
    const struct hlsl_ir_node *instr = &load->node;
    enum hlsl_sampler_dim dim = load->sampling_dim;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_shader_opcode opcode;
    bool multisampled;

    VKD3D_ASSERT(load->load_type == HLSL_RESOURCE_LOAD);

    multisampled = resource_type->class == HLSL_CLASS_TEXTURE
            && (resource_type->sampler_dim == HLSL_SAMPLER_DIM_2DMS
            || resource_type->sampler_dim == HLSL_SAMPLER_DIM_2DMSARRAY);

    if (uav)
        opcode = VKD3DSIH_LD_UAV_TYPED;
    else if (raw)
        opcode = VKD3DSIH_LD_RAW;
    else
        opcode = multisampled ? VKD3DSIH_LD2DMS : VKD3DSIH_LD;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, 2 + multisampled)))
        return false;

    if (texel_offset && !sm4_generate_vsir_validate_texel_offset_aoffimmi(texel_offset))
    {
        hlsl_error(ctx, &texel_offset->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TEXEL_OFFSET,
                "Offset must resolve to integer literal in the range -8 to 7.");
        return false;
    }
    sm4_generate_vsir_encode_texel_offset_as_aoffimmi(ins, texel_offset);

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    if (!uav)
    {
        /* Mipmap level is in the last component in the IR, but needs to be in
         * the W component in the instruction. */
        unsigned int dim_count = hlsl_sampler_dim_count(dim);

        if (dim_count == 1)
            coords_writemask = VKD3DSP_WRITEMASK_0 | VKD3DSP_WRITEMASK_3;
        if (dim_count == 2)
            coords_writemask = VKD3DSP_WRITEMASK_0 | VKD3DSP_WRITEMASK_1 | VKD3DSP_WRITEMASK_3;
    }

    vsir_src_from_hlsl_node(&ins->src[0], ctx, coords, coords_writemask);

    if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program,
            &ins->src[1], resource, ins->dst[0].write_mask, &instr->loc))
        return false;

    if (multisampled)
    {
        if (sample_index->type == HLSL_IR_CONSTANT)
            vsir_src_from_hlsl_constant_value(&ins->src[2], ctx,
                    &hlsl_ir_constant(sample_index)->value, VKD3D_DATA_INT, 1, 0);
        else if (version->major == 4 && version->minor == 0)
            hlsl_error(ctx, &sample_index->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Expected literal sample index.");
        else
            vsir_src_from_hlsl_node(&ins->src[2], ctx, sample_index, VKD3DSP_WRITEMASK_ALL);
    }
    return true;
}

static bool sm4_generate_vsir_instr_sample(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_resource_load *load)
{
    const struct hlsl_ir_node *texel_offset = load->texel_offset.node;
    const struct hlsl_ir_node *coords = load->coords.node;
    const struct hlsl_deref *resource = &load->resource;
    const struct hlsl_deref *sampler = &load->sampler;
    const struct hlsl_ir_node *instr = &load->node;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_shader_opcode opcode;
    unsigned int src_count;

    switch (load->load_type)
    {
        case HLSL_RESOURCE_SAMPLE:
            opcode = VKD3DSIH_SAMPLE;
            src_count = 3;
            break;

        case HLSL_RESOURCE_SAMPLE_CMP:
            opcode = VKD3DSIH_SAMPLE_C;
            src_count = 4;
            break;

        case HLSL_RESOURCE_SAMPLE_CMP_LZ:
            opcode = VKD3DSIH_SAMPLE_C_LZ;
            src_count = 4;
            break;

        case HLSL_RESOURCE_SAMPLE_LOD:
            opcode = VKD3DSIH_SAMPLE_LOD;
            src_count = 4;
            break;

        case HLSL_RESOURCE_SAMPLE_LOD_BIAS:
            opcode = VKD3DSIH_SAMPLE_B;
            src_count = 4;
            break;

        case HLSL_RESOURCE_SAMPLE_GRAD:
            opcode = VKD3DSIH_SAMPLE_GRAD;
            src_count = 5;
            break;

        default:
            vkd3d_unreachable();
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, src_count)))
        return false;

    if (texel_offset && !sm4_generate_vsir_validate_texel_offset_aoffimmi(texel_offset))
    {
        hlsl_error(ctx, &texel_offset->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TEXEL_OFFSET,
                "Offset must resolve to integer literal in the range -8 to 7.");
        return false;
    }
    sm4_generate_vsir_encode_texel_offset_as_aoffimmi(ins, texel_offset);

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    vsir_src_from_hlsl_node(&ins->src[0], ctx, coords, VKD3DSP_WRITEMASK_ALL);

    if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program, &ins->src[1],
            resource, ins->dst[0].write_mask, &instr->loc))
        return false;

    if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program, &ins->src[2],
            sampler, VKD3DSP_WRITEMASK_ALL, &instr->loc))
        return false;

    if (opcode == VKD3DSIH_SAMPLE_LOD || opcode == VKD3DSIH_SAMPLE_B)
    {
        vsir_src_from_hlsl_node(&ins->src[3], ctx, load->lod.node, VKD3DSP_WRITEMASK_ALL);
    }
    else if (opcode == VKD3DSIH_SAMPLE_C || opcode == VKD3DSIH_SAMPLE_C_LZ)
    {
        vsir_src_from_hlsl_node(&ins->src[3], ctx, load->cmp.node, VKD3DSP_WRITEMASK_ALL);
    }
    else if (opcode == VKD3DSIH_SAMPLE_GRAD)
    {
        vsir_src_from_hlsl_node(&ins->src[3], ctx, load->ddx.node, VKD3DSP_WRITEMASK_ALL);
        vsir_src_from_hlsl_node(&ins->src[4], ctx, load->ddy.node, VKD3DSP_WRITEMASK_ALL);
    }
    return true;
}

static bool sm4_generate_vsir_instr_gather(struct hlsl_ctx *ctx, struct vsir_program *program,
        const struct hlsl_ir_resource_load *load, uint32_t swizzle, bool compare)
{
    const struct vkd3d_shader_version *version = &program->shader_version;
    const struct hlsl_ir_node *texel_offset = load->texel_offset.node;
    const struct hlsl_ir_node *coords = load->coords.node;
    const struct hlsl_deref *resource = &load->resource;
    enum vkd3d_shader_opcode opcode = VKD3DSIH_GATHER4;
    const struct hlsl_deref *sampler = &load->sampler;
    const struct hlsl_ir_node *instr = &load->node;
    unsigned int src_count = 3, current_arg = 0;
    struct vkd3d_shader_instruction *ins;

    if (texel_offset && !sm4_generate_vsir_validate_texel_offset_aoffimmi(texel_offset))
    {
        if (!vkd3d_shader_ver_ge(version, 5, 0))
        {
            hlsl_error(ctx, &texel_offset->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TEXEL_OFFSET,
                "Offset must resolve to integer literal in the range -8 to 7 for profiles < 5.");
            return false;
        }
        opcode = VKD3DSIH_GATHER4_PO;
        ++src_count;
    }

    if (compare)
    {
        opcode = opcode == VKD3DSIH_GATHER4 ? VKD3DSIH_GATHER4_C : VKD3DSIH_GATHER4_PO_C;
        ++src_count;
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, src_count)))
        return false;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);
    vsir_src_from_hlsl_node(&ins->src[current_arg++], ctx, coords, VKD3DSP_WRITEMASK_ALL);

    if (opcode == VKD3DSIH_GATHER4_PO || opcode == VKD3DSIH_GATHER4_PO_C)
        vsir_src_from_hlsl_node(&ins->src[current_arg++], ctx, texel_offset, VKD3DSP_WRITEMASK_ALL);
    else
        sm4_generate_vsir_encode_texel_offset_as_aoffimmi(ins, texel_offset);

    if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program,
            &ins->src[current_arg++], resource, ins->dst[0].write_mask, &instr->loc))
        return false;

    if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program,
            &ins->src[current_arg], sampler, VKD3DSP_WRITEMASK_ALL, &instr->loc))
        return false;
    ins->src[current_arg].reg.dimension = VSIR_DIMENSION_VEC4;
    ins->src[current_arg].swizzle = swizzle;
    current_arg++;

    if (compare)
        vsir_src_from_hlsl_node(&ins->src[current_arg++], ctx, load->cmp.node, VKD3DSP_WRITEMASK_0);

    return true;
}

static bool sm4_generate_vsir_instr_sample_info(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_resource_load *load)
{
    const struct hlsl_deref *resource = &load->resource;
    const struct hlsl_ir_node *instr = &load->node;
    struct hlsl_type *type = instr->data_type;
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(type->e.numeric.type == HLSL_TYPE_UINT || type->e.numeric.type == HLSL_TYPE_FLOAT);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_SAMPLE_INFO, 1, 1)))
        return false;

    if (type->e.numeric.type == HLSL_TYPE_UINT)
        ins->flags = VKD3DSI_SAMPLE_INFO_UINT;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program,
            &ins->src[0], resource, ins->dst[0].write_mask, &instr->loc))
        return false;

    return true;
}

static bool sm4_generate_vsir_instr_resinfo(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_resource_load *load)
{
    const struct hlsl_deref *resource = &load->resource;
    const struct hlsl_ir_node *instr = &load->node;
    struct hlsl_type *type = instr->data_type;
    struct vkd3d_shader_instruction *ins;

    if (resource->data_type->sampler_dim == HLSL_SAMPLER_DIM_BUFFER
            || resource->data_type->sampler_dim == HLSL_SAMPLER_DIM_STRUCTURED_BUFFER)
    {
        hlsl_fixme(ctx, &load->node.loc, "resinfo for buffers.");
        return false;
    }

    VKD3D_ASSERT(type->e.numeric.type == HLSL_TYPE_UINT || type->e.numeric.type == HLSL_TYPE_FLOAT);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_RESINFO, 1, 2)))
        return false;

    if (type->e.numeric.type == HLSL_TYPE_UINT)
        ins->flags = VKD3DSI_RESINFO_UINT;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    vsir_src_from_hlsl_node(&ins->src[0], ctx, load->lod.node, VKD3DSP_WRITEMASK_ALL);

    if (!sm4_generate_vsir_init_src_param_from_deref(ctx, program,
            &ins->src[1], resource, ins->dst[0].write_mask, &instr->loc))
        return false;

    return true;
}

static uint32_t get_gather_swizzle(enum hlsl_resource_load_type type)
{
    switch (type)
    {
        case HLSL_RESOURCE_GATHER_RED:
        case HLSL_RESOURCE_GATHER_CMP_RED:
            return VKD3D_SHADER_SWIZZLE(X, X, X, X);

        case HLSL_RESOURCE_GATHER_GREEN:
        case HLSL_RESOURCE_GATHER_CMP_GREEN:
            return VKD3D_SHADER_SWIZZLE(Y, Y, Y, Y);

        case HLSL_RESOURCE_GATHER_BLUE:
        case HLSL_RESOURCE_GATHER_CMP_BLUE:
            return VKD3D_SHADER_SWIZZLE(Z, Z, Z, Z);

        case HLSL_RESOURCE_GATHER_ALPHA:
        case HLSL_RESOURCE_GATHER_CMP_ALPHA:
            return VKD3D_SHADER_SWIZZLE(W, W, W, W);
        default:
            return 0;
    }

    return 0;
}

static bool sm4_generate_vsir_instr_resource_load(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_resource_load *load)
{
    if (load->sampler.var && !load->sampler.var->is_uniform)
    {
        hlsl_fixme(ctx, &load->node.loc, "Sample using non-uniform sampler variable.");
        return false;
    }

    if (!load->resource.var->is_uniform)
    {
        hlsl_fixme(ctx, &load->node.loc, "Load from non-uniform resource variable.");
        return false;
    }

    switch (load->load_type)
    {
        case HLSL_RESOURCE_LOAD:
            return sm4_generate_vsir_instr_ld(ctx, program, load);

        case HLSL_RESOURCE_SAMPLE:
        case HLSL_RESOURCE_SAMPLE_CMP:
        case HLSL_RESOURCE_SAMPLE_CMP_LZ:
        case HLSL_RESOURCE_SAMPLE_LOD:
        case HLSL_RESOURCE_SAMPLE_LOD_BIAS:
        case HLSL_RESOURCE_SAMPLE_GRAD:
            /* Combined sample expressions were lowered. */
            VKD3D_ASSERT(load->sampler.var);
            return sm4_generate_vsir_instr_sample(ctx, program, load);

        case HLSL_RESOURCE_GATHER_RED:
        case HLSL_RESOURCE_GATHER_GREEN:
        case HLSL_RESOURCE_GATHER_BLUE:
        case HLSL_RESOURCE_GATHER_ALPHA:
            return sm4_generate_vsir_instr_gather(ctx, program, load, get_gather_swizzle(load->load_type), false);

        case HLSL_RESOURCE_GATHER_CMP_RED:
        case HLSL_RESOURCE_GATHER_CMP_GREEN:
        case HLSL_RESOURCE_GATHER_CMP_BLUE:
        case HLSL_RESOURCE_GATHER_CMP_ALPHA:
            return sm4_generate_vsir_instr_gather(ctx, program, load, get_gather_swizzle(load->load_type), true);

        case HLSL_RESOURCE_SAMPLE_INFO:
            return sm4_generate_vsir_instr_sample_info(ctx, program, load);

        case HLSL_RESOURCE_RESINFO:
            return sm4_generate_vsir_instr_resinfo(ctx, program, load);

        case HLSL_RESOURCE_SAMPLE_PROJ:
            vkd3d_unreachable();

        default:
            return false;
    }
}

static bool sm4_generate_vsir_instr_interlocked(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_interlocked *interlocked)
{

    static const enum vkd3d_shader_opcode opcodes[] =
    {
        [HLSL_INTERLOCKED_ADD] = VKD3DSIH_ATOMIC_IADD,
        [HLSL_INTERLOCKED_AND] = VKD3DSIH_ATOMIC_AND,
        [HLSL_INTERLOCKED_CMP_EXCH] = VKD3DSIH_ATOMIC_CMP_STORE,
        [HLSL_INTERLOCKED_MAX] = VKD3DSIH_ATOMIC_UMAX,
        [HLSL_INTERLOCKED_MIN] = VKD3DSIH_ATOMIC_UMIN,
        [HLSL_INTERLOCKED_OR] = VKD3DSIH_ATOMIC_OR,
        [HLSL_INTERLOCKED_XOR] = VKD3DSIH_ATOMIC_XOR,
    };

    static const enum vkd3d_shader_opcode imm_opcodes[] =
    {
        [HLSL_INTERLOCKED_ADD] = VKD3DSIH_IMM_ATOMIC_IADD,
        [HLSL_INTERLOCKED_AND] = VKD3DSIH_IMM_ATOMIC_AND,
        [HLSL_INTERLOCKED_CMP_EXCH] = VKD3DSIH_IMM_ATOMIC_CMP_EXCH,
        [HLSL_INTERLOCKED_EXCH] = VKD3DSIH_IMM_ATOMIC_EXCH,
        [HLSL_INTERLOCKED_MAX] = VKD3DSIH_IMM_ATOMIC_UMAX,
        [HLSL_INTERLOCKED_MIN] = VKD3DSIH_IMM_ATOMIC_UMIN,
        [HLSL_INTERLOCKED_OR] = VKD3DSIH_IMM_ATOMIC_OR,
        [HLSL_INTERLOCKED_XOR] = VKD3DSIH_IMM_ATOMIC_XOR,
    };

    struct hlsl_ir_node *cmp_value = interlocked->cmp_value.node, *value = interlocked->value.node;
    struct hlsl_ir_node *coords = interlocked->coords.node;
    struct hlsl_ir_node *instr = &interlocked->node;
    bool is_imm = interlocked->node.reg.allocated;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_shader_opcode opcode;

    opcode = is_imm ? imm_opcodes[interlocked->op] : opcodes[interlocked->op];

    if (value->data_type->e.numeric.type == HLSL_TYPE_INT)
    {
        if (opcode == VKD3DSIH_ATOMIC_UMAX)
            opcode = VKD3DSIH_ATOMIC_IMAX;
        else if (opcode == VKD3DSIH_ATOMIC_UMIN)
            opcode = VKD3DSIH_ATOMIC_IMIN;
        else if (opcode == VKD3DSIH_IMM_ATOMIC_UMAX)
            opcode = VKD3DSIH_IMM_ATOMIC_IMAX;
        else if (opcode == VKD3DSIH_IMM_ATOMIC_UMIN)
            opcode = VKD3DSIH_IMM_ATOMIC_IMIN;
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode,
            is_imm ? 2 : 1, cmp_value ? 3 : 2)))
        return false;

    if (is_imm)
        vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    dst_param = is_imm ? &ins->dst[1] : &ins->dst[0];
    if (!sm4_generate_vsir_init_dst_param_from_deref(ctx, program, dst_param, &interlocked->dst, &instr->loc, 0))
        return false;
    dst_param->reg.dimension = VSIR_DIMENSION_NONE;

    vsir_src_from_hlsl_node(&ins->src[0], ctx, coords, VKD3DSP_WRITEMASK_ALL);
    if (cmp_value)
    {
        vsir_src_from_hlsl_node(&ins->src[1], ctx, cmp_value, VKD3DSP_WRITEMASK_ALL);
        vsir_src_from_hlsl_node(&ins->src[2], ctx, value, VKD3DSP_WRITEMASK_ALL);
    }
    else
    {
        vsir_src_from_hlsl_node(&ins->src[1], ctx, value, VKD3DSP_WRITEMASK_ALL);
    }

    return true;
}

static bool sm4_generate_vsir_instr_jump(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_jump *jump)
{
    const struct hlsl_ir_node *instr = &jump->node;
    struct vkd3d_shader_instruction *ins;

    switch (jump->type)
    {
        case HLSL_IR_JUMP_BREAK:
            return generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_BREAK, 0, 0);

        case HLSL_IR_JUMP_CONTINUE:
            return generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_CONTINUE, 0, 0);

        case HLSL_IR_JUMP_DISCARD_NZ:
            if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_DISCARD, 0, 1)))
                return false;
            ins->flags = VKD3D_SHADER_CONDITIONAL_OP_NZ;

            vsir_src_from_hlsl_node(&ins->src[0], ctx, jump->condition.node, VKD3DSP_WRITEMASK_ALL);
            return true;

        case HLSL_IR_JUMP_RETURN:
            vkd3d_unreachable();

        default:
            hlsl_fixme(ctx, &jump->node.loc, "Jump type %s.", hlsl_jump_type_to_string(jump->type));
            return false;
    }
}

static bool sm4_generate_vsir_instr_sync(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_sync *sync)
{
    const struct hlsl_ir_node *instr = &sync->node;
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_SYNC, 0, 0)))
        return false;
    ins->flags = sync->sync_flags;

    return true;
}

static void sm4_generate_vsir_block(struct hlsl_ctx *ctx, struct hlsl_block *block, struct vsir_program *program);

static void sm4_generate_vsir_instr_if(struct hlsl_ctx *ctx, struct vsir_program *program, struct hlsl_ir_if *iff)
{
    struct hlsl_ir_node *instr = &iff->node;
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(iff->condition.node->data_type->e.numeric.dimx == 1);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_IF, 0, 1)))
        return;
    ins->flags = VKD3D_SHADER_CONDITIONAL_OP_NZ;

    vsir_src_from_hlsl_node(&ins->src[0], ctx, iff->condition.node, VKD3DSP_WRITEMASK_ALL);

    sm4_generate_vsir_block(ctx, &iff->then_block, program);

    if (!list_empty(&iff->else_block.instrs))
    {
        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_ELSE, 0, 0)))
            return;
        sm4_generate_vsir_block(ctx, &iff->else_block, program);
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_ENDIF, 0, 0)))
        return;
}

static void sm4_generate_vsir_instr_loop(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_loop *loop)
{
    struct hlsl_ir_node *instr = &loop->node;
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_LOOP, 0, 0)))
        return;

    sm4_generate_vsir_block(ctx, &loop->body, program);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_ENDLOOP, 0, 0)))
        return;
}

static void sm4_generate_vsir_instr_switch(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_switch *swi)
{
    const struct hlsl_ir_node *selector = swi->selector.node;
    struct hlsl_ir_node *instr = &swi->node;
    struct vkd3d_shader_instruction *ins;
    struct hlsl_ir_switch_case *cas;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_SWITCH, 0, 1)))
        return;
    vsir_src_from_hlsl_node(&ins->src[0], ctx, selector, VKD3DSP_WRITEMASK_ALL);

    LIST_FOR_EACH_ENTRY(cas, &swi->cases, struct hlsl_ir_switch_case, entry)
    {
        if (cas->is_default)
        {
            if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_DEFAULT, 0, 0)))
                return;
        }
        else
        {
            struct hlsl_constant_value value = {.u[0].u = cas->value};

            if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_CASE, 0, 1)))
                return;
            vsir_src_from_hlsl_constant_value(&ins->src[0], ctx, &value, VKD3D_DATA_UINT, 1, VKD3DSP_WRITEMASK_ALL);
        }

        sm4_generate_vsir_block(ctx, &cas->body, program);
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_ENDSWITCH, 0, 0)))
        return;
}

static void sm4_generate_vsir_block(struct hlsl_ctx *ctx, struct hlsl_block *block, struct vsir_program *program)
{
    struct vkd3d_string_buffer *dst_type_string;
    struct hlsl_ir_node *instr, *next;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->data_type)
        {
            if (instr->data_type->class != HLSL_CLASS_SCALAR && instr->data_type->class != HLSL_CLASS_VECTOR)
            {
                hlsl_fixme(ctx, &instr->loc, "Class %#x should have been lowered or removed.", instr->data_type->class);
                break;
            }
        }

        switch (instr->type)
        {
            case HLSL_IR_CALL:
                vkd3d_unreachable();

            case HLSL_IR_CONSTANT:
                /* In SM4 all constants are inlined. */
                break;

            case HLSL_IR_EXPR:
                if (!(dst_type_string = hlsl_type_to_string(ctx, instr->data_type)))
                    break;
                sm4_generate_vsir_instr_expr(ctx, program, hlsl_ir_expr(instr), dst_type_string->buffer);
                hlsl_release_string_buffer(ctx, dst_type_string);
                break;

            case HLSL_IR_IF:
                sm4_generate_vsir_instr_if(ctx, program, hlsl_ir_if(instr));
                break;

            case HLSL_IR_LOAD:
                sm4_generate_vsir_instr_load(ctx, program, hlsl_ir_load(instr));
                break;

            case HLSL_IR_LOOP:
                sm4_generate_vsir_instr_loop(ctx, program, hlsl_ir_loop(instr));
                break;

            case HLSL_IR_RESOURCE_LOAD:
                sm4_generate_vsir_instr_resource_load(ctx, program, hlsl_ir_resource_load(instr));
                break;

            case HLSL_IR_RESOURCE_STORE:
                sm4_generate_vsir_instr_resource_store(ctx, program, hlsl_ir_resource_store(instr));
                break;

            case HLSL_IR_JUMP:
                sm4_generate_vsir_instr_jump(ctx, program, hlsl_ir_jump(instr));
                break;

            case HLSL_IR_STORE:
                sm4_generate_vsir_instr_store(ctx, program, hlsl_ir_store(instr));
                break;

            case HLSL_IR_SWITCH:
                sm4_generate_vsir_instr_switch(ctx, program, hlsl_ir_switch(instr));
                break;

            case HLSL_IR_SWIZZLE:
                generate_vsir_instr_swizzle(ctx, program, hlsl_ir_swizzle(instr));
                break;

            case HLSL_IR_INTERLOCKED:
                sm4_generate_vsir_instr_interlocked(ctx, program, hlsl_ir_interlocked(instr));
                break;

            case HLSL_IR_SYNC:
                sm4_generate_vsir_instr_sync(ctx, program, hlsl_ir_sync(instr));
                break;

            default:
                break;
        }
    }
}

static void sm4_generate_vsir_add_function(struct hlsl_ctx *ctx,
        struct hlsl_ir_function_decl *func, uint64_t config_flags, struct vsir_program *program)
{
    struct hlsl_block block = {0};
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;
    uint32_t temp_count;

    ctx->is_patch_constant_func = func == ctx->patch_constant_func;

    compute_liveness(ctx, func);
    mark_indexable_vars(ctx, func);
    temp_count = allocate_temp_registers(ctx, func);
    if (ctx->result)
        return;
    program->temp_count = max(program->temp_count, temp_count);

    hlsl_block_init(&block);

    LIST_FOR_EACH_ENTRY(var, &func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if ((var->is_input_semantic && var->last_read)
                || (var->is_output_semantic && var->first_write))
            sm4_generate_vsir_instr_dcl_semantic(ctx, program, var, &block, &var->loc);
    }

    if (temp_count)
        sm4_generate_vsir_instr_dcl_temps(ctx, program, temp_count, &block, &func->loc);

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            if (var->is_uniform || var->is_input_semantic || var->is_output_semantic)
                continue;
            if (!var->regs[HLSL_REGSET_NUMERIC].allocated)
                continue;

            if (var->indexable)
            {
                unsigned int id = var->regs[HLSL_REGSET_NUMERIC].id;
                unsigned int size = align(var->data_type->reg_size[HLSL_REGSET_NUMERIC], 4) / 4;

                sm4_generate_vsir_instr_dcl_indexable_temp(ctx, program, &block, id, size, 4, &var->loc);
            }
        }
    }

    list_move_head(&func->body.instrs, &block.instrs);

    hlsl_block_cleanup(&block);

    sm4_generate_vsir_block(ctx, &func->body, program);

    generate_vsir_add_program_instruction(ctx, program, &func->loc, VKD3DSIH_RET, 0, 0);
}

static int sm4_compare_extern_resources(const void *a, const void *b)
{
    const struct extern_resource *aa = a;
    const struct extern_resource *bb = b;
    int r;

    if ((r = vkd3d_u32_compare(aa->regset, bb->regset)))
        return r;

    if ((r = vkd3d_u32_compare(aa->space, bb->space)))
        return r;

    return vkd3d_u32_compare(aa->index, bb->index);
}

static const char *string_skip_tag(const char *string)
{
    if (!strncmp(string, "<resource>", strlen("<resource>")))
        return string + strlen("<resource>");
    return string;
}

static void sm4_free_extern_resources(struct extern_resource *extern_resources, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        vkd3d_free(extern_resources[i].name);
    }
    vkd3d_free(extern_resources);
}

static struct extern_resource *sm4_get_extern_resources(struct hlsl_ctx *ctx, unsigned int *count)
{
    bool separate_components = ctx->profile->major_version == 5 && ctx->profile->minor_version == 0;
    struct extern_resource *extern_resources = NULL;
    const struct hlsl_ir_var *var;
    struct hlsl_buffer *buffer;
    enum hlsl_regset regset;
    size_t capacity = 0;
    char *name;

    *count = 0;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (separate_components)
        {
            unsigned int component_count = hlsl_type_component_count(var->data_type);
            unsigned int k, regset_offset;

            for (k = 0; k < component_count; ++k)
            {
                struct hlsl_type *component_type = hlsl_type_get_component_type(ctx, var->data_type, k);
                struct vkd3d_string_buffer *name_buffer;

                if (!hlsl_type_is_resource(component_type))
                    continue;

                regset_offset = hlsl_type_get_component_offset(ctx, var->data_type, k, &regset);
                if (regset_offset > var->regs[regset].allocation_size)
                    continue;

                if (!var->objects_usage[regset][regset_offset].used)
                    continue;

                if (!(hlsl_array_reserve(ctx, (void **)&extern_resources,
                        &capacity, *count + 1, sizeof(*extern_resources))))
                {
                    sm4_free_extern_resources(extern_resources, *count);
                    *count = 0;
                    return NULL;
                }

                if (!(name_buffer = hlsl_component_to_string(ctx, var, k)))
                {
                    sm4_free_extern_resources(extern_resources, *count);
                    *count = 0;
                    return NULL;
                }
                if (!(name = hlsl_strdup(ctx, string_skip_tag(name_buffer->buffer))))
                {
                    sm4_free_extern_resources(extern_resources, *count);
                    *count = 0;
                    hlsl_release_string_buffer(ctx, name_buffer);
                    return NULL;
                }
                hlsl_release_string_buffer(ctx, name_buffer);

                extern_resources[*count].var = NULL;
                extern_resources[*count].buffer = NULL;

                extern_resources[*count].name = name;
                extern_resources[*count].is_user_packed = !!var->reg_reservation.reg_type;

                extern_resources[*count].component_type = component_type;

                extern_resources[*count].regset = regset;
                extern_resources[*count].id = var->regs[regset].id;
                extern_resources[*count].space = var->regs[regset].space;
                extern_resources[*count].index = var->regs[regset].index + regset_offset;
                extern_resources[*count].bind_count = 1;
                extern_resources[*count].loc = var->loc;

                ++*count;
            }
        }
        else
        {
            unsigned int r;

            if (!hlsl_type_is_resource(var->data_type))
                continue;

            for (r = 0; r <= HLSL_REGSET_LAST; ++r)
            {
                if (!var->regs[r].allocated)
                    continue;

                if (!(hlsl_array_reserve(ctx, (void **)&extern_resources,
                        &capacity, *count + 1, sizeof(*extern_resources))))
                {
                    sm4_free_extern_resources(extern_resources, *count);
                    *count = 0;
                    return NULL;
                }

                if (!(name = hlsl_strdup(ctx, string_skip_tag(var->name))))
                {
                    sm4_free_extern_resources(extern_resources, *count);
                    *count = 0;
                    return NULL;
                }

                extern_resources[*count].var = var;
                extern_resources[*count].buffer = NULL;

                extern_resources[*count].name = name;
                /* For some reason 5.1 resources aren't marked as
                 * user-packed, but cbuffers still are. */
                extern_resources[*count].is_user_packed = hlsl_version_lt(ctx, 5, 1)
                        && !!var->reg_reservation.reg_type;

                extern_resources[*count].component_type = hlsl_type_get_component_type(ctx, var->data_type, 0);

                extern_resources[*count].regset = r;
                extern_resources[*count].id = var->regs[r].id;
                extern_resources[*count].space = var->regs[r].space;
                extern_resources[*count].index = var->regs[r].index;
                extern_resources[*count].bind_count = var->bind_count[r];
                extern_resources[*count].loc = var->loc;

                ++*count;
            }
        }
    }

    LIST_FOR_EACH_ENTRY(buffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (!buffer->reg.allocated)
            continue;

        if (!(hlsl_array_reserve(ctx, (void **)&extern_resources,
                &capacity, *count + 1, sizeof(*extern_resources))))
        {
            sm4_free_extern_resources(extern_resources, *count);
            *count = 0;
            return NULL;
        }

        if (!(name = hlsl_strdup(ctx, buffer->name)))
        {
            sm4_free_extern_resources(extern_resources, *count);
            *count = 0;
            return NULL;
        }

        extern_resources[*count].var = NULL;
        extern_resources[*count].buffer = buffer;

        extern_resources[*count].name = name;
        extern_resources[*count].is_user_packed = !!buffer->reservation.reg_type;

        extern_resources[*count].component_type = NULL;

        extern_resources[*count].regset = HLSL_REGSET_NUMERIC;
        extern_resources[*count].id = buffer->reg.id;
        extern_resources[*count].space = buffer->reg.space;
        extern_resources[*count].index = buffer->reg.index;
        extern_resources[*count].bind_count = 1;
        extern_resources[*count].loc = buffer->loc;

        ++*count;
    }

    if (extern_resources)
        qsort(extern_resources, *count, sizeof(*extern_resources), sm4_compare_extern_resources);

    return extern_resources;
}

static void generate_vsir_scan_required_features(struct hlsl_ctx *ctx, struct vsir_program *program)
{
    struct extern_resource *extern_resources;
    unsigned int extern_resources_count;

    extern_resources = sm4_get_extern_resources(ctx, &extern_resources_count);
    for (unsigned int i = 0; i < extern_resources_count; ++i)
    {
        if (extern_resources[i].component_type && extern_resources[i].component_type->e.resource.rasteriser_ordered)
            program->features.rovs = true;
    }
    sm4_free_extern_resources(extern_resources, extern_resources_count);

    /* FIXME: We also emit code that should require UAVS_AT_EVERY_STAGE,
     * STENCIL_REF, and TYPED_UAV_LOAD_ADDITIONAL_FORMATS. */
}

static void generate_vsir_scan_global_flags(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_function_decl *entry_func)
{
    const struct vkd3d_shader_version *version = &program->shader_version;
    struct extern_resource *extern_resources;
    unsigned int extern_resources_count, i;
    struct hlsl_ir_var *var;

    extern_resources = sm4_get_extern_resources(ctx, &extern_resources_count);

    if (version->major == 4)
    {
        for (i = 0; i < extern_resources_count; ++i)
        {
            const struct extern_resource *resource = &extern_resources[i];
            const struct hlsl_type *type = resource->component_type;

            if (type && type->class == HLSL_CLASS_TEXTURE && type->sampler_dim == HLSL_SAMPLER_DIM_RAW_BUFFER)
            {
                program->global_flags |= VKD3DSGF_ENABLE_RAW_AND_STRUCTURED_BUFFERS;
                break;
            }
        }
    }

    sm4_free_extern_resources(extern_resources, extern_resources_count);

    LIST_FOR_EACH_ENTRY(var, &entry_func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        const struct hlsl_type *type = var->data_type;

        if (hlsl_type_is_primitive_array(type))
            type = var->data_type->e.array.type;

        /* Note that it doesn't matter if the semantic is unused or doesn't
         * generate a signature element (e.g. SV_DispatchThreadID). */
        if ((var->is_input_semantic || var->is_output_semantic)
                && (type->is_minimum_precision || hlsl_type_is_minimum_precision(type)))
        {
            program->global_flags |= VKD3DSGF_ENABLE_MINIMUM_PRECISION;
            break;
        }
    }
    /* FIXME: We also need to check for minimum-precision uniforms and local
     * variable arithmetic. */

    if (entry_func->early_depth_test && vkd3d_shader_ver_ge(version, 5, 0))
        program->global_flags |= VKD3DSGF_FORCE_EARLY_DEPTH_STENCIL;
}

static void sm4_generate_vsir_add_dcl_constant_buffer(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_buffer *cbuffer)
{
    unsigned int array_first = cbuffer->reg.index;
    unsigned int array_last = cbuffer->reg.index; /* FIXME: array end. */
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &cbuffer->loc, VKD3DSIH_DCL_CONSTANT_BUFFER, 0, 0)))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }

    ins->declaration.cb.size = cbuffer->size;

    src_param = &ins->declaration.cb.src;
    vsir_src_param_init(src_param, VKD3DSPR_CONSTBUFFER, VKD3D_DATA_FLOAT, 0);
    src_param->reg.dimension = VSIR_DIMENSION_VEC4;
    src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;

    ins->declaration.cb.range.space = cbuffer->reg.space;
    ins->declaration.cb.range.first = array_first;
    ins->declaration.cb.range.last = array_last;

    src_param->reg.idx[0].offset = cbuffer->reg.id;
    src_param->reg.idx[1].offset = array_first;
    src_param->reg.idx[2].offset = array_last;
    src_param->reg.idx_count = 3;
}

static void sm4_generate_vsir_add_dcl_sampler(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct extern_resource *resource)
{
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    unsigned int i;

    VKD3D_ASSERT(resource->regset == HLSL_REGSET_SAMPLERS);
    VKD3D_ASSERT(hlsl_version_lt(ctx, 5, 1) || resource->bind_count == 1);

    for (i = 0; i < resource->bind_count; ++i)
    {
        unsigned int array_first = resource->index + i;
        unsigned int array_last = resource->index + i; /* FIXME: array end. */

        if (resource->var && !resource->var->objects_usage[HLSL_REGSET_SAMPLERS][i].used)
            continue;

        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &resource->loc, VKD3DSIH_DCL_SAMPLER, 0, 0)))
        {
            ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
            return;
        }

        if (resource->component_type->sampler_dim == HLSL_SAMPLER_DIM_COMPARISON)
            ins->flags |= VKD3DSI_SAMPLER_COMPARISON_MODE;

        src_param = &ins->declaration.sampler.src;
        vsir_src_param_init(src_param, VKD3DSPR_SAMPLER, VKD3D_DATA_UNUSED, 0);

        ins->declaration.sampler.range.first = array_first;
        ins->declaration.sampler.range.last = array_last;
        ins->declaration.sampler.range.space = resource->space;

        src_param->reg.idx[0].offset = resource->id;
        src_param->reg.idx[1].offset = array_first;
        src_param->reg.idx[2].offset = array_last;
        src_param->reg.idx_count = 3;
    }
}

static enum vkd3d_shader_resource_type sm4_generate_vsir_get_resource_type(const struct hlsl_type *type)
{
    switch (type->sampler_dim)
    {
        case HLSL_SAMPLER_DIM_1D:
            return VKD3D_SHADER_RESOURCE_TEXTURE_1D;
        case HLSL_SAMPLER_DIM_2D:
            return VKD3D_SHADER_RESOURCE_TEXTURE_2D;
        case HLSL_SAMPLER_DIM_3D:
            return VKD3D_SHADER_RESOURCE_TEXTURE_3D;
        case HLSL_SAMPLER_DIM_CUBE:
            return VKD3D_SHADER_RESOURCE_TEXTURE_CUBE;
        case HLSL_SAMPLER_DIM_1DARRAY:
            return VKD3D_SHADER_RESOURCE_TEXTURE_1DARRAY;
        case HLSL_SAMPLER_DIM_2DARRAY:
            return VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY;
        case HLSL_SAMPLER_DIM_2DMS:
            return VKD3D_SHADER_RESOURCE_TEXTURE_2DMS;
        case HLSL_SAMPLER_DIM_2DMSARRAY:
            return VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY;
        case HLSL_SAMPLER_DIM_CUBEARRAY:
            return VKD3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY;
        case HLSL_SAMPLER_DIM_BUFFER:
        case HLSL_SAMPLER_DIM_RAW_BUFFER:
        case HLSL_SAMPLER_DIM_STRUCTURED_BUFFER:
            return VKD3D_SHADER_RESOURCE_BUFFER;
        default:
            vkd3d_unreachable();
    }
}

static enum vkd3d_data_type sm4_generate_vsir_get_format_type(const struct hlsl_type *type)
{
    const struct hlsl_type *format = type->e.resource.format;

    switch (format->e.numeric.type)
    {
        case HLSL_TYPE_DOUBLE:
            return VKD3D_DATA_DOUBLE;

        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_HALF:
            if (format->modifiers & HLSL_MODIFIER_UNORM)
                return VKD3D_DATA_UNORM;
            if (format->modifiers & HLSL_MODIFIER_SNORM)
                return VKD3D_DATA_SNORM;
            return VKD3D_DATA_FLOAT;

        case HLSL_TYPE_INT:
            return VKD3D_DATA_INT;

        case HLSL_TYPE_BOOL:
        case HLSL_TYPE_MIN16UINT:
        case HLSL_TYPE_UINT:
            return VKD3D_DATA_UINT;
    }

    vkd3d_unreachable();
}

static void sm4_generate_vsir_add_dcl_texture(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct extern_resource *resource,
        bool uav)
{
    enum hlsl_regset regset = uav ? HLSL_REGSET_UAVS : HLSL_REGSET_TEXTURES;
    struct vkd3d_shader_structured_resource *structured_resource;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_semantic *semantic;
    struct vkd3d_shader_instruction *ins;
    struct hlsl_type *component_type;
    enum vkd3d_shader_opcode opcode;
    bool multisampled;
    unsigned int i, j;

    VKD3D_ASSERT(resource->regset == regset);
    VKD3D_ASSERT(hlsl_version_lt(ctx, 5, 1) || resource->bind_count == 1);

    component_type = resource->component_type;

    for (i = 0; i < resource->bind_count; ++i)
    {
        unsigned int array_first = resource->index + i;
        unsigned int array_last = resource->index + i; /* FIXME: array end. */

        if (resource->var && !resource->var->objects_usage[regset][i].used)
            continue;

        if (uav)
        {
            switch (component_type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_STRUCTURED_BUFFER:
                    opcode = VKD3DSIH_DCL_UAV_STRUCTURED;
                    break;
                case HLSL_SAMPLER_DIM_RAW_BUFFER:
                    opcode = VKD3DSIH_DCL_UAV_RAW;
                    break;
                default:
                    opcode = VKD3DSIH_DCL_UAV_TYPED;
                    break;
            }
        }
        else
        {
            switch (component_type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_RAW_BUFFER:
                    opcode = VKD3DSIH_DCL_RESOURCE_RAW;
                    break;
                default:
                    opcode = VKD3DSIH_DCL;
                    break;
            }
        }

        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &resource->loc, opcode, 0, 0)))
        {
            ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
            return;
        }
        semantic = &ins->declaration.semantic;
        structured_resource = &ins->declaration.structured_resource;
        dst_param = &semantic->resource.reg;
        vsir_dst_param_init(dst_param, uav ? VKD3DSPR_UAV : VKD3DSPR_RESOURCE, VKD3D_DATA_UNUSED, 0);

        if (uav && component_type->sampler_dim == HLSL_SAMPLER_DIM_STRUCTURED_BUFFER)
            structured_resource->byte_stride = 4 * component_type->e.resource.format->reg_size[HLSL_REGSET_NUMERIC];
        if (uav && component_type->e.resource.rasteriser_ordered)
            ins->flags = VKD3DSUF_RASTERISER_ORDERED_VIEW;

        multisampled = component_type->sampler_dim == HLSL_SAMPLER_DIM_2DMS
                || component_type->sampler_dim == HLSL_SAMPLER_DIM_2DMSARRAY;

        if (!hlsl_version_ge(ctx, 4, 1) && multisampled && !component_type->sample_count)
        {
            hlsl_error(ctx, &resource->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Multisampled texture object declaration needs sample count for profile %u.%u.",
                    ctx->profile->major_version, ctx->profile->minor_version);
        }

        for (j = 0; j < 4; ++j)
            semantic->resource_data_type[j] = sm4_generate_vsir_get_format_type(component_type);

        semantic->resource.range.first = array_first;
        semantic->resource.range.last = array_last;
        semantic->resource.range.space = resource->space;

        dst_param->reg.idx[0].offset = resource->id;
        dst_param->reg.idx[1].offset = array_first;
        dst_param->reg.idx[2].offset = array_last;
        dst_param->reg.idx_count = 3;

        ins->resource_type = sm4_generate_vsir_get_resource_type(resource->component_type);
        if (resource->component_type->sampler_dim == HLSL_SAMPLER_DIM_RAW_BUFFER)
            ins->raw = true;
        if (resource->component_type->sampler_dim == HLSL_SAMPLER_DIM_STRUCTURED_BUFFER)
        {
            ins->structured = true;
            ins->resource_stride = 4 * component_type->e.resource.format->reg_size[HLSL_REGSET_NUMERIC];
        }

        if (multisampled)
            semantic->sample_count = component_type->sample_count;
    }
}

/* OBJECTIVE: Translate all the information from ctx and entry_func to the
 * vsir_program, so it can be used as input to tpf_compile() without relying
 * on ctx and entry_func. */
static void sm4_generate_vsir(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        uint64_t config_flags, struct vsir_program *program)
{
    struct vkd3d_shader_version version = {0};
    struct extern_resource *extern_resources;
    unsigned int extern_resources_count;
    const struct hlsl_buffer *cbuffer;

    version.major = ctx->profile->major_version;
    version.minor = ctx->profile->minor_version;
    version.type = ctx->profile->type;

    if (!vsir_program_init(program, NULL, &version, 0, VSIR_CF_STRUCTURED, VSIR_NORMALISED_SM4))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }

    generate_vsir_signature(ctx, program, func);
    if (version.type == VKD3D_SHADER_TYPE_HULL)
        generate_vsir_signature(ctx, program, ctx->patch_constant_func);

    if (version.type == VKD3D_SHADER_TYPE_COMPUTE)
    {
        program->thread_group_size.x = ctx->thread_count[0];
        program->thread_group_size.y = ctx->thread_count[1];
        program->thread_group_size.z = ctx->thread_count[2];
    }
    else if (version.type == VKD3D_SHADER_TYPE_HULL)
    {
        program->input_control_point_count = ctx->input_control_point_count == UINT_MAX
                ? 1 : ctx->input_control_point_count;
        program->output_control_point_count = ctx->output_control_point_count;
        program->tess_domain = ctx->domain;
        program->tess_partitioning = ctx->partitioning;
        program->tess_output_primitive = ctx->output_primitive;
    }
    else if (version.type == VKD3D_SHADER_TYPE_DOMAIN)
    {
        program->input_control_point_count = ctx->input_control_point_count == UINT_MAX
                ? 0 : ctx->input_control_point_count;
        program->tess_domain = ctx->domain;
    }
    else if (version.type == VKD3D_SHADER_TYPE_GEOMETRY)
    {
        program->input_control_point_count = ctx->input_control_point_count;
        program->input_primitive = ctx->input_primitive_type;
        program->output_topology = ctx->output_topology_type;
        program->vertices_out_count = ctx->max_vertex_count;
    }

    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (cbuffer->reg.allocated)
            sm4_generate_vsir_add_dcl_constant_buffer(ctx, program, cbuffer);
    }

    extern_resources = sm4_get_extern_resources(ctx, &extern_resources_count);
    for (unsigned int i = 0; i < extern_resources_count; ++i)
    {
        const struct extern_resource *resource = &extern_resources[i];

        if (resource->regset == HLSL_REGSET_SAMPLERS)
            sm4_generate_vsir_add_dcl_sampler(ctx, program, resource);
        else if (resource->regset == HLSL_REGSET_TEXTURES)
            sm4_generate_vsir_add_dcl_texture(ctx, program, resource, false);
        else if (resource->regset == HLSL_REGSET_UAVS)
            sm4_generate_vsir_add_dcl_texture(ctx, program, resource, true);
    }
    sm4_free_extern_resources(extern_resources, extern_resources_count);

    if (version.type == VKD3D_SHADER_TYPE_HULL)
        generate_vsir_add_program_instruction(ctx, program,
                &ctx->patch_constant_func->loc, VKD3DSIH_HS_CONTROL_POINT_PHASE, 0, 0);
    sm4_generate_vsir_add_function(ctx, func, config_flags, program);
    if (version.type == VKD3D_SHADER_TYPE_HULL)
    {
        generate_vsir_add_program_instruction(ctx, program,
                &ctx->patch_constant_func->loc, VKD3DSIH_HS_FORK_PHASE, 0, 0);
        sm4_generate_vsir_add_function(ctx, ctx->patch_constant_func, config_flags, program);
    }

    generate_vsir_scan_required_features(ctx, program);
    generate_vsir_scan_global_flags(ctx, program, func);
}

/* For some reason, for matrices, values from default value initializers end
 * up in different components than from regular initializers. Default value
 * initializers fill the matrix in vertical reading order
 * (left-to-right top-to-bottom) instead of regular reading order
 * (top-to-bottom left-to-right), so they have to be adjusted. An exception is
 * that the order of matrix initializers for function parameters are row-major
 * (top-to-bottom left-to-right). */
static unsigned int get_component_index_from_default_initializer_index(struct hlsl_type *type, unsigned int index)
{
    unsigned int element_comp_count, element, x, y, i;
    unsigned int base = 0;

    switch (type->class)
    {
        case HLSL_CLASS_MATRIX:
            x = index / type->e.numeric.dimy;
            y = index % type->e.numeric.dimy;
            return y * type->e.numeric.dimx + x;

        case HLSL_CLASS_ARRAY:
            element_comp_count = hlsl_type_component_count(type->e.array.type);
            element = index / element_comp_count;
            base = element * element_comp_count;
            return base + get_component_index_from_default_initializer_index(type->e.array.type, index - base);

        case HLSL_CLASS_STRUCT:
            for (i = 0; i < type->e.record.field_count; ++i)
            {
                struct hlsl_type *field_type = type->e.record.fields[i].type;

                element_comp_count = hlsl_type_component_count(field_type);
                if (index - base < element_comp_count)
                    return base + get_component_index_from_default_initializer_index(field_type, index - base);
                base += element_comp_count;
            }
            break;

        default:
            return index;
    }

    vkd3d_unreachable();
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
        case HLSL_SAMPLER_DIM_BUFFER:
        case HLSL_SAMPLER_DIM_RAW_BUFFER:
        case HLSL_SAMPLER_DIM_STRUCTURED_BUFFER:
            return D3D_SRV_DIMENSION_BUFFER;
        default:
            break;
    }

    vkd3d_unreachable();
}

static enum D3D_RESOURCE_RETURN_TYPE sm4_data_type(const struct hlsl_type *type)
{
    const struct hlsl_type *format = type->e.resource.format;

    switch (format->e.numeric.type)
    {
        case HLSL_TYPE_DOUBLE:
            return D3D_RETURN_TYPE_DOUBLE;

        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_HALF:
            if (format->modifiers & HLSL_MODIFIER_UNORM)
                return D3D_RETURN_TYPE_UNORM;
            if (format->modifiers & HLSL_MODIFIER_SNORM)
                return D3D_RETURN_TYPE_SNORM;
            return D3D_RETURN_TYPE_FLOAT;

        case HLSL_TYPE_INT:
            return D3D_RETURN_TYPE_SINT;
            break;

        case HLSL_TYPE_BOOL:
        case HLSL_TYPE_MIN16UINT:
        case HLSL_TYPE_UINT:
            return D3D_RETURN_TYPE_UINT;
    }

    vkd3d_unreachable();
}

static D3D_SHADER_INPUT_TYPE sm4_resource_type(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_SAMPLER:
            return D3D_SIT_SAMPLER;
        case HLSL_CLASS_TEXTURE:
            return D3D_SIT_TEXTURE;
        case HLSL_CLASS_UAV:
            return D3D_SIT_UAV_RWTYPED;
        default:
            break;
    }

    vkd3d_unreachable();
}

static D3D_SHADER_VARIABLE_CLASS sm4_class(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_MATRIX:
            VKD3D_ASSERT(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);
            if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
                return D3D_SVC_MATRIX_COLUMNS;
            else
                return D3D_SVC_MATRIX_ROWS;
        case HLSL_CLASS_SCALAR:
            return D3D_SVC_SCALAR;
        case HLSL_CLASS_VECTOR:
            return D3D_SVC_VECTOR;

        case HLSL_CLASS_ARRAY:
        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_STRUCT:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_STREAM_OUTPUT:
        case HLSL_CLASS_NULL:
            break;
    }

    vkd3d_unreachable();
}

static D3D_SHADER_VARIABLE_TYPE sm4_base_type(const struct hlsl_type *type)
{
    switch (type->e.numeric.type)
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
        case HLSL_TYPE_UINT:
            return D3D_SVT_UINT;
        case HLSL_TYPE_MIN16UINT:
            return D3D_SVT_MIN16UINT;
    }

    vkd3d_unreachable();
}

static void write_sm4_type(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, struct hlsl_type *type)
{
    const struct hlsl_type *array_type = hlsl_get_multiarray_element_type(type);
    const char *name = array_type->name ? array_type->name : "<unnamed>";
    const struct hlsl_profile_info *profile = ctx->profile;
    unsigned int array_size = 0;
    size_t name_offset = 0;
    size_t i;

    if (type->bytecode_offset)
        return;

    if (profile->major_version >= 5)
        name_offset = put_string(buffer, name);

    if (type->class == HLSL_CLASS_ARRAY)
        array_size = hlsl_get_multiarray_size(type);

    if (array_type->class == HLSL_CLASS_STRUCT)
    {
        unsigned int field_count = 0;
        size_t fields_offset = 0;

        for (i = 0; i < array_type->e.record.field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            if (!field->type->reg_size[HLSL_REGSET_NUMERIC])
                continue;

            field->name_bytecode_offset = put_string(buffer, field->name);
            write_sm4_type(ctx, buffer, field->type);
            ++field_count;
        }

        fields_offset = bytecode_align(buffer);

        for (i = 0; i < array_type->e.record.field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            if (!field->type->reg_size[HLSL_REGSET_NUMERIC])
                continue;

            put_u32(buffer, field->name_bytecode_offset);
            put_u32(buffer, field->type->bytecode_offset);
            put_u32(buffer, field->reg_offset[HLSL_REGSET_NUMERIC] * sizeof(float));
        }
        type->bytecode_offset = put_u32(buffer, vkd3d_make_u32(D3D_SVC_STRUCT, D3D_SVT_VOID));
        put_u32(buffer, vkd3d_make_u32(1, hlsl_type_component_count(array_type)));
        put_u32(buffer, vkd3d_make_u32(array_size, field_count));
        put_u32(buffer, fields_offset);
    }
    else
    {
        VKD3D_ASSERT(array_type->class <= HLSL_CLASS_LAST_NUMERIC);
        type->bytecode_offset = put_u32(buffer, vkd3d_make_u32(sm4_class(array_type), sm4_base_type(array_type)));
        put_u32(buffer, vkd3d_make_u32(array_type->e.numeric.dimy, array_type->e.numeric.dimx));
        put_u32(buffer, vkd3d_make_u32(array_size, 0));
        put_u32(buffer, 1);
    }

    if (profile->major_version >= 5)
    {
        put_u32(buffer, 0); /* FIXME: unknown */
        put_u32(buffer, 0); /* FIXME: unknown */
        put_u32(buffer, 0); /* FIXME: unknown */
        put_u32(buffer, 0); /* FIXME: unknown */
        put_u32(buffer, name_offset);
    }
}

static void sm4_generate_rdef(struct hlsl_ctx *ctx, struct vkd3d_shader_code *rdef)
{
    uint32_t binding_desc_size = (hlsl_version_ge(ctx, 5, 1) ? 10 : 8) * sizeof(uint32_t);
    size_t cbuffers_offset, resources_offset, creator_offset, string_offset;
    unsigned int cbuffer_count = 0, extern_resources_count, i, j;
    size_t cbuffer_position, resource_position, creator_position;
    const struct hlsl_profile_info *profile = ctx->profile;
    struct vkd3d_bytecode_buffer buffer = {0};
    struct extern_resource *extern_resources;
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

    extern_resources = sm4_get_extern_resources(ctx, &extern_resources_count);

    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (cbuffer->reg.allocated)
            ++cbuffer_count;
    }

    put_u32(&buffer, cbuffer_count);
    cbuffer_position = put_u32(&buffer, 0);
    put_u32(&buffer, extern_resources_count);
    resource_position = put_u32(&buffer, 0);
    put_u32(&buffer, vkd3d_make_u32(vkd3d_make_u16(profile->minor_version, profile->major_version),
            target_types[profile->type]));
    put_u32(&buffer, 0); /* FIXME: compilation flags */
    creator_position = put_u32(&buffer, 0);

    if (profile->major_version >= 5)
    {
        put_u32(&buffer, hlsl_version_ge(ctx, 5, 1) ? TAG_RD11_REVERSE : TAG_RD11);
        put_u32(&buffer, 15 * sizeof(uint32_t)); /* size of RDEF header including this header */
        put_u32(&buffer, 6 * sizeof(uint32_t)); /* size of buffer desc */
        put_u32(&buffer, binding_desc_size); /* size of binding desc */
        put_u32(&buffer, 10 * sizeof(uint32_t)); /* size of variable desc */
        put_u32(&buffer, 9 * sizeof(uint32_t)); /* size of type desc */
        put_u32(&buffer, 3 * sizeof(uint32_t)); /* size of member desc */
        put_u32(&buffer, 0); /* unknown; possibly a null terminator */
    }

    /* Bound resources. */

    resources_offset = bytecode_align(&buffer);
    set_u32(&buffer, resource_position, resources_offset);

    for (i = 0; i < extern_resources_count; ++i)
    {
        const struct extern_resource *resource = &extern_resources[i];
        uint32_t flags = 0;

        if (resource->is_user_packed)
            flags |= D3D_SIF_USERPACKED;

        put_u32(&buffer, 0); /* name */
        if (resource->buffer)
            put_u32(&buffer, resource->buffer->type == HLSL_BUFFER_CONSTANT ? D3D_SIT_CBUFFER : D3D_SIT_TBUFFER);
        else
            put_u32(&buffer, sm4_resource_type(resource->component_type));
        if (resource->regset == HLSL_REGSET_TEXTURES || resource->regset == HLSL_REGSET_UAVS)
        {
            unsigned int dimx = resource->component_type->e.resource.format->e.numeric.dimx;

            put_u32(&buffer, sm4_data_type(resource->component_type));
            put_u32(&buffer, sm4_rdef_resource_dimension(resource->component_type));
            put_u32(&buffer, ~0u); /* FIXME: multisample count */
            flags |= (dimx - 1) << VKD3D_SM4_SIF_TEXTURE_COMPONENTS_SHIFT;
        }
        else
        {
            put_u32(&buffer, 0);
            put_u32(&buffer, 0);
            put_u32(&buffer, 0);
        }
        put_u32(&buffer, resource->index);
        put_u32(&buffer, resource->bind_count);
        put_u32(&buffer, flags);

        if (hlsl_version_ge(ctx, 5, 1))
        {
            put_u32(&buffer, resource->space);
            put_u32(&buffer, resource->id);
        }
    }

    for (i = 0; i < extern_resources_count; ++i)
    {
        const struct extern_resource *resource = &extern_resources[i];

        string_offset = put_string(&buffer, resource->name);
        set_u32(&buffer, resources_offset + i * binding_desc_size, string_offset);
    }

    /* Buffers. */

    cbuffers_offset = bytecode_align(&buffer);
    set_u32(&buffer, cbuffer_position, cbuffers_offset);
    LIST_FOR_EACH_ENTRY(cbuffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        unsigned int var_count = 0;

        if (!cbuffer->reg.allocated)
            continue;

        LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            if (var->is_uniform && var->buffer == cbuffer && var->data_type->reg_size[HLSL_REGSET_NUMERIC])
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
        size_t vars_start = bytecode_align(&buffer);

        if (!cbuffer->reg.allocated)
            continue;

        set_u32(&buffer, cbuffers_offset + (i++ * 6 + 2) * sizeof(uint32_t), vars_start);

        LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            uint32_t flags = 0;

            if (!var->is_uniform || var->buffer != cbuffer || !var->data_type->reg_size[HLSL_REGSET_NUMERIC])
                continue;

            if (var->is_read)
                flags |= D3D_SVF_USED;

            put_u32(&buffer, 0); /* name */
            put_u32(&buffer, var->buffer_offset * sizeof(float));
            put_u32(&buffer, var->data_type->reg_size[HLSL_REGSET_NUMERIC] * sizeof(float));
            put_u32(&buffer, flags);
            put_u32(&buffer, 0); /* type */
            put_u32(&buffer, 0); /* default value */

            if (profile->major_version >= 5)
            {
                put_u32(&buffer, 0); /* texture start */
                put_u32(&buffer, 0); /* texture count */
                put_u32(&buffer, 0); /* sampler start */
                put_u32(&buffer, 0); /* sampler count */
            }
        }

        j = 0;
        LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            const unsigned int var_size = (profile->major_version >= 5 ? 10 : 6);
            size_t var_offset = vars_start + j * var_size * sizeof(uint32_t);

            if (!var->is_uniform || var->buffer != cbuffer || !var->data_type->reg_size[HLSL_REGSET_NUMERIC])
                continue;

            string_offset = put_string(&buffer, var->name);
            set_u32(&buffer, var_offset, string_offset);
            write_sm4_type(ctx, &buffer, var->data_type);
            set_u32(&buffer, var_offset + 4 * sizeof(uint32_t), var->data_type->bytecode_offset);

            if (var->default_values)
            {
                unsigned int reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];
                unsigned int comp_count = hlsl_type_component_count(var->data_type);
                unsigned int default_value_offset;
                unsigned int k;

                default_value_offset = bytecode_reserve_bytes(&buffer, reg_size * sizeof(uint32_t));
                set_u32(&buffer, var_offset + 5 * sizeof(uint32_t), default_value_offset);

                for (k = 0; k < comp_count; ++k)
                {
                    struct hlsl_type *comp_type = hlsl_type_get_component_type(ctx, var->data_type, k);
                    unsigned int comp_offset, comp_index;
                    enum hlsl_regset regset;

                    if (comp_type->class == HLSL_CLASS_STRING)
                    {
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                                "Cannot write string default value.");
                        continue;
                    }

                    comp_index = get_component_index_from_default_initializer_index(var->data_type, k);
                    comp_offset = hlsl_type_get_component_offset(ctx, var->data_type, comp_index, &regset);
                    if (regset == HLSL_REGSET_NUMERIC)
                    {
                        if (comp_type->e.numeric.type == HLSL_TYPE_DOUBLE)
                            hlsl_fixme(ctx, &var->loc, "Write double default values.");

                        set_u32(&buffer, default_value_offset + comp_offset * sizeof(uint32_t),
                                var->default_values[k].number.u);
                    }
                }
            }

            ++j;
        }
    }

    creator_offset = put_string(&buffer, vkd3d_shader_get_version(NULL, NULL));
    set_u32(&buffer, creator_position, creator_offset);

    sm4_free_extern_resources(extern_resources, extern_resources_count);

    if (buffer.status)
    {
        vkd3d_free(buffer.data);
        ctx->result = buffer.status;
        return;
    }
    rdef->code = buffer.data;
    rdef->size = buffer.size;
}

static bool loop_unrolling_generate_const_bool_store(struct hlsl_ctx *ctx, struct hlsl_ir_var *var,
        bool val, struct hlsl_block *block, struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *const_node;

    if (!(const_node = hlsl_new_bool_constant(ctx, val, loc)))
        return false;
    hlsl_block_add_instr(block, const_node);

    hlsl_block_add_simple_store(ctx, block, var, const_node);
    return true;
}

static bool loop_unrolling_remove_jumps_recurse(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_var *loop_broken, struct hlsl_ir_var *loop_continued);

static bool loop_unrolling_remove_jumps_visit(struct hlsl_ctx *ctx, struct hlsl_ir_node *node,
        struct hlsl_ir_var *loop_broken, struct hlsl_ir_var *loop_continued)
{
    struct hlsl_ir_jump *jump;
    struct hlsl_ir_var *var;
    struct hlsl_block draft;
    struct hlsl_ir_if *iff;

    if (node->type == HLSL_IR_IF)
    {
        iff = hlsl_ir_if(node);
        if (loop_unrolling_remove_jumps_recurse(ctx, &iff->then_block, loop_broken, loop_continued))
            return true;
        if (loop_unrolling_remove_jumps_recurse(ctx, &iff->else_block, loop_broken, loop_continued))
            return true;
        return false;
    }

    if (node->type == HLSL_IR_JUMP)
    {
        jump = hlsl_ir_jump(node);
        if (jump->type != HLSL_IR_JUMP_UNRESOLVED_CONTINUE && jump->type != HLSL_IR_JUMP_BREAK)
            return false;

        hlsl_block_init(&draft);

        if (jump->type == HLSL_IR_JUMP_UNRESOLVED_CONTINUE)
            var = loop_continued;
        else
            var = loop_broken;

        if (!loop_unrolling_generate_const_bool_store(ctx, var, true, &draft, &jump->node.loc))
            return false;

        list_move_before(&jump->node.entry, &draft.instrs);
        list_remove(&jump->node.entry);
        hlsl_free_instr(&jump->node);

        return true;
    }

    return false;
}

static struct hlsl_ir_if *loop_unrolling_generate_var_check(struct hlsl_ctx *ctx,
        struct hlsl_block *dst, struct hlsl_ir_var *var, struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *cond, *load, *iff;
    struct hlsl_block then_block;

    hlsl_block_init(&then_block);

    load = hlsl_block_add_simple_load(ctx, dst, var, loc);
    cond = hlsl_block_add_unary_expr(ctx, dst, HLSL_OP1_LOGIC_NOT, load, loc);

    if (!(iff = hlsl_new_if(ctx, cond, &then_block, NULL, loc)))
        return NULL;
    hlsl_block_add_instr(dst, iff);

    return hlsl_ir_if(iff);
}

static bool loop_unrolling_remove_jumps_recurse(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_var *loop_broken, struct hlsl_ir_var *loop_continued)
{
    struct hlsl_ir_node *node, *next;

    LIST_FOR_EACH_ENTRY_SAFE(node, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        struct hlsl_ir_if *broken_check, *continued_check;
        struct hlsl_block draft;

        if (!loop_unrolling_remove_jumps_visit(ctx, node, loop_broken, loop_continued))
            continue;

        if (&next->entry == &block->instrs)
            return true;

        hlsl_block_init(&draft);

        broken_check = loop_unrolling_generate_var_check(ctx, &draft, loop_broken, &next->loc);
        continued_check = loop_unrolling_generate_var_check(ctx,
                &broken_check->then_block, loop_continued, &next->loc);

        list_move_before(&next->entry, &draft.instrs);

        list_move_slice_tail(&continued_check->then_block.instrs, &next->entry, list_tail(&block->instrs));

        return true;
    }

    return false;
}

static void loop_unrolling_remove_jumps(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_var *loop_broken, struct hlsl_ir_var *loop_continued)
{
    while (loop_unrolling_remove_jumps_recurse(ctx, block, loop_broken, loop_continued));
}

static unsigned int loop_unrolling_get_max_iterations(struct hlsl_ctx *ctx, struct hlsl_ir_loop *loop)
{
    /* Always use the explicit limit if it has been passed. */
    if (loop->unroll_limit)
        return loop->unroll_limit;

    /* All SMs will default to 1024 if [unroll] has been specified without an explicit limit. */
    if (loop->unroll_type == HLSL_LOOP_FORCE_UNROLL)
        return 1024;

    /* SM4 limits implicit unrolling to 254 iterations. */
    if (hlsl_version_ge(ctx, 4, 0))
        return 254;

    /* SM<3 implicitly unrolls up to 1024 iterations. */
    return 1024;
}

static void loop_unrolling_simplify(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct copy_propagation_state *state, unsigned int *index)
{
    size_t scopes_depth = state->scope_count - 1;
    unsigned int current_index;
    bool progress;

    do
    {
        state->stopped = false;
        for (size_t i = state->scope_count; scopes_depth < i; --i)
            copy_propagation_pop_scope(state);
        copy_propagation_push_scope(state, ctx);

        progress = simplify_exprs(ctx, block);

        current_index = index_instructions(block, *index);
        progress |= copy_propagation_transform_block(ctx, block, state);

        progress |= hlsl_transform_ir(ctx, fold_swizzle_chains, block, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_swizzles, block, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_conditional_branches, block, NULL);
    } while (progress);

    *index = current_index;
}

static bool loop_unrolling_check_val(struct copy_propagation_state *state, struct hlsl_ir_var *var)
{
    struct copy_propagation_value *v;

    if (!(v = copy_propagation_get_value(state, var, 0, UINT_MAX))
            || v->node->type != HLSL_IR_CONSTANT)
        return false;

    return hlsl_ir_constant(v->node)->value.u[0].u;
}

static bool loop_unrolling_unroll_loop(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_loop *loop)
{
    struct hlsl_block draft, tmp_dst, loop_body;
    struct hlsl_ir_var *broken, *continued;
    unsigned int max_iterations, i, index;
    struct copy_propagation_state state;
    struct hlsl_ir_if *target_if;

    if (!(broken = hlsl_new_synthetic_var(ctx, "broken",
            hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL), &loop->node.loc)))
        goto fail;

    if (!(continued = hlsl_new_synthetic_var(ctx, "continued",
            hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL), &loop->node.loc)))
        goto fail;

    hlsl_block_init(&draft);
    hlsl_block_init(&tmp_dst);

    max_iterations = loop_unrolling_get_max_iterations(ctx, loop);
    copy_propagation_state_init(&state, ctx);
    index = 2;
    state.stop = &loop->node;
    loop_unrolling_simplify(ctx, block, &state, &index);
    state.stopped = false;
    index = loop->node.index;

    if (!loop_unrolling_generate_const_bool_store(ctx, broken, false, &tmp_dst, &loop->node.loc))
        goto fail;
    hlsl_block_add_block(&draft, &tmp_dst);

    if (!loop_unrolling_generate_const_bool_store(ctx, continued, false, &tmp_dst, &loop->node.loc))
        goto fail;
    hlsl_block_add_block(&draft, &tmp_dst);

    if (!(target_if = loop_unrolling_generate_var_check(ctx, &tmp_dst, broken, &loop->node.loc)))
        goto fail;
    state.stop = LIST_ENTRY(list_head(&tmp_dst.instrs), struct hlsl_ir_node, entry);
    hlsl_block_add_block(&draft, &tmp_dst);

    copy_propagation_push_scope(&state, ctx);
    loop_unrolling_simplify(ctx, &draft, &state, &index);

    /* As an optimization, we only remove jumps from the loop's body once. */
    if (!hlsl_clone_block(ctx, &loop_body, &loop->body))
        goto fail;
    loop_unrolling_remove_jumps(ctx, &loop_body, broken, continued);

    for (i = 0; i < max_iterations; ++i)
    {
        copy_propagation_push_scope(&state, ctx);

        if (!loop_unrolling_generate_const_bool_store(ctx, continued, false, &tmp_dst, &loop->node.loc))
            goto fail;
        hlsl_block_add_block(&target_if->then_block, &tmp_dst);

        if (!hlsl_clone_block(ctx, &tmp_dst, &loop_body))
            goto fail;
        hlsl_block_add_block(&target_if->then_block, &tmp_dst);

        loop_unrolling_simplify(ctx, &target_if->then_block, &state, &index);

        if (loop_unrolling_check_val(&state, broken))
            break;

        if (!(target_if = loop_unrolling_generate_var_check(ctx, &tmp_dst, broken, &loop->node.loc)))
            goto fail;
        hlsl_block_add_block(&draft, &tmp_dst);

        if (!hlsl_clone_block(ctx, &tmp_dst, &loop->iter))
            goto fail;
        hlsl_block_add_block(&target_if->then_block, &tmp_dst);
   }

    /* Native will not emit an error if max_iterations has been reached with an
     * explicit limit. It also will not insert a loop if there are iterations left
     * i.e [unroll(4)] for (i = 0; i < 8; ++i)) */
    if (!loop->unroll_limit && i == max_iterations)
    {
        if (loop->unroll_type == HLSL_LOOP_FORCE_UNROLL)
            hlsl_error(ctx, &loop->node.loc, VKD3D_SHADER_ERROR_HLSL_FAILED_FORCED_UNROLL,
                "Unable to unroll loop, maximum iterations reached (%u).", max_iterations);
        goto fail;
    }

    hlsl_block_cleanup(&loop_body);
    copy_propagation_state_destroy(&state);

    list_move_before(&loop->node.entry, &draft.instrs);
    hlsl_block_cleanup(&draft);
    list_remove(&loop->node.entry);
    hlsl_free_instr(&loop->node);

    return true;

fail:
    hlsl_block_cleanup(&loop_body);
    copy_propagation_state_destroy(&state);
    hlsl_block_cleanup(&draft);

    return false;
}

static bool unroll_loops(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, void *context)
{
    struct hlsl_block *program = context;
    struct hlsl_ir_loop *loop;

    if (node->type != HLSL_IR_LOOP)
        return true;

    loop = hlsl_ir_loop(node);

    if (loop->unroll_type != HLSL_LOOP_UNROLL && loop->unroll_type != HLSL_LOOP_FORCE_UNROLL)
        return true;

    if (!loop_unrolling_unroll_loop(ctx, program, loop))
        loop->unroll_type = HLSL_LOOP_FORCE_LOOP;

    return true;
}

/* We could handle this at parse time. However, loop unrolling often needs to
 * know the value of variables modified in the "iter" block. It is possible to
 * detect that all exit paths of a loop body modify such variables in the same
 * way, but difficult, and d3dcompiler does not attempt to do so.
 * In fact, d3dcompiler is capable of unrolling the following loop:
 * for (int i = 0; i < 10; ++i)
 * {
 *     if (some_uniform > 4)
 *         continue;
 * }
 * but cannot unroll the same loop with "++i" moved to each exit path:
 * for (int i = 0; i < 10;)
 * {
 *     if (some_uniform > 4)
 *     {
 *         ++i;
 *         continue;
 *     }
 *     ++i;
 * }
 */
static bool resolve_loops(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, void *context)
{
    struct hlsl_ir_loop *loop;

    if (node->type != HLSL_IR_LOOP)
        return true;

    loop = hlsl_ir_loop(node);

    hlsl_block_add_block(&loop->body, &loop->iter);
    return true;
}

static void resolve_continues(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_loop *last_loop)
{
    struct hlsl_ir_node *node;

    LIST_FOR_EACH_ENTRY(node, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (node->type)
        {
            case HLSL_IR_LOOP:
            {
                struct hlsl_ir_loop *loop = hlsl_ir_loop(node);

                resolve_continues(ctx, &loop->body, loop);
                break;
            }
            case HLSL_IR_IF:
            {
                struct hlsl_ir_if *iff = hlsl_ir_if(node);
                resolve_continues(ctx, &iff->then_block, last_loop);
                resolve_continues(ctx, &iff->else_block, last_loop);
                break;
            }
            case HLSL_IR_SWITCH:
            {
                struct hlsl_ir_switch *s = hlsl_ir_switch(node);
                struct hlsl_ir_switch_case *c;

                LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                {
                    resolve_continues(ctx, &c->body, last_loop);
                }

                break;
            }
            case HLSL_IR_JUMP:
            {
                struct hlsl_ir_jump *jump = hlsl_ir_jump(node);

                if (jump->type != HLSL_IR_JUMP_UNRESOLVED_CONTINUE)
                    break;

                if (last_loop->type == HLSL_LOOP_FOR)
                {
                    struct hlsl_block draft;

                    if (!hlsl_clone_block(ctx, &draft, &last_loop->iter))
                        return;

                    list_move_before(&node->entry, &draft.instrs);
                    hlsl_block_cleanup(&draft);
                }

                jump->type = HLSL_IR_JUMP_CONTINUE;
                break;
            }
            default:
                break;
        }
    }
}

static void loop_unrolling_execute(struct hlsl_ctx *ctx, struct hlsl_block *block)
{
    bool progress;

    /* These are required by copy propagation, which in turn is required for
     * unrolling. */
    do
    {
        progress = hlsl_transform_ir(ctx, split_array_copies, block, NULL);
        progress |= hlsl_transform_ir(ctx, split_struct_copies, block, NULL);
    } while (progress);
    hlsl_transform_ir(ctx, split_matrix_copies, block, NULL);

    hlsl_transform_ir(ctx, unroll_loops, block, block);
    resolve_continues(ctx, block, NULL);
    hlsl_transform_ir(ctx, resolve_loops, block, NULL);
}

static bool lower_f16tof32(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, struct hlsl_block *block)
{
    struct hlsl_ir_function_decl *func;
    struct hlsl_ir_node *call, *rhs;
    unsigned int component_count;
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_var *lhs;
    char *body;

    static const char template[] =
    "typedef uint%u uintX;\n"
    "float%u soft_f16tof32(uintX x)\n"
    "{\n"
    "    uintX mantissa = x & 0x3ff;\n"
    "    uintX high2 = mantissa >> 8;\n"
    "    uintX high2_check = high2 ? high2 : mantissa;\n"
    "    uintX high6 = high2_check >> 4;\n"
    "    uintX high6_check = high6 ? high6 : high2_check;\n"
    "\n"
    "    uintX high8 = high6_check >> 2;\n"
    "    uintX high8_check = (high8 ? high8 : high6_check) >> 1;\n"
    "    uintX shift = high6 ? (high2 ? 12 : 4) : (high2 ? 8 : 0);\n"
    "    shift = high8 ? shift + 2 : shift;\n"
    "    shift = high8_check ? shift + 1 : shift;\n"
    "    shift = -shift + 10;\n"
    "    shift = mantissa ? shift : 11;\n"
    "    uintX subnormal_mantissa = ((mantissa << shift) << 23) & 0x7fe000;\n"
    "    uintX subnormal_exp = -(shift << 23) + 0x38800000;\n"
    "    uintX subnormal_val = subnormal_exp + subnormal_mantissa;\n"
    "    uintX subnormal_or_zero = mantissa ? subnormal_val : 0;\n"
    "\n"
    "    uintX exponent = (((x >> 10) << 23) & 0xf800000) + 0x38000000;\n"
    "\n"
    "    uintX low_3 = (x << 13) & 0x7fe000;\n"
    "    uintX normalized_val = exponent + low_3;\n"
    "    uintX inf_nan_val = low_3 + 0x7f800000;\n"
    "\n"
    "    uintX exp_mask = 0x7c00;\n"
    "    uintX is_inf_nan = (x & exp_mask) == exp_mask;\n"
    "    uintX is_normalized = x & exp_mask;\n"
    "\n"
    "    uintX check = is_inf_nan ? inf_nan_val : normalized_val;\n"
    "    uintX exp_mantissa = (is_normalized ? check : subnormal_or_zero) & 0x7fffe000;\n"
    "    uintX sign_bit = (x << 16) & 0x80000000;\n"
    "\n"
    "    return asfloat(exp_mantissa + sign_bit);\n"
    "}\n";


    if (node->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(node);

    if (expr->op != HLSL_OP1_F16TOF32)
        return false;

    rhs = expr->operands[0].node;
    component_count = hlsl_type_component_count(rhs->data_type);

    if (!(body = hlsl_sprintf_alloc(ctx, template, component_count, component_count)))
        return false;

    if (!(func = hlsl_compile_internal_function(ctx, "soft_f16tof32", body)))
        return false;

    lhs = func->parameters.vars[0];
    hlsl_block_add_simple_store(ctx, block, lhs, rhs);

    if (!(call = hlsl_new_call(ctx, func, &node->loc)))
        return false;
    hlsl_block_add_instr(block, call);

    hlsl_block_add_simple_load(ctx, block, func->return_var, &node->loc);
    return true;
}

static bool lower_f32tof16(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, struct hlsl_block *block)
{
    struct hlsl_ir_function_decl *func;
    struct hlsl_ir_node *call, *rhs;
    unsigned int component_count;
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_var *lhs;
    char *body;

    static const char template[] =
    "typedef uint%u uintX;\n"
    "uintX soft_f32tof16(float%u x)\n"
    "{\n"
    "    uintX v = asuint(x);\n"
    "    uintX v_abs = v & 0x7fffffff;\n"
    "    uintX sign_bit = (v >> 16) & 0x8000;\n"
    "    uintX exp = (v >> 23) & 0xff;\n"
    "    uintX mantissa = v & 0x7fffff;\n"
    "    uintX nan16;\n"
    "    uintX nan = (v & 0x7f800000) == 0x7f800000;\n"
    "    uintX val;\n"
    "\n"
    "    val = 113 - exp;\n"
    "    val = (mantissa + 0x800000) >> val;\n"
    "    val >>= 13;\n"
    "\n"
    "    val = (exp - 127) < -38 ? 0 : val;\n"
    "\n"
    "    val = v_abs < 0x38800000 ? val : (v_abs + 0xc8000000) >> 13;\n"
    "    val = v_abs > 0x47ffe000 ? 0x7bff : val;\n"
    "\n"
    "    nan16 = (((v >> 13) | (v >> 3) | v) & 0x3ff) + 0x7c00;\n"
    "    val = nan ? nan16 : val;\n"
    "\n"
    "    return (val & 0x7fff) + sign_bit;\n"
    "}\n";

    if (node->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(node);

    if (expr->op != HLSL_OP1_F32TOF16)
        return false;

    rhs = expr->operands[0].node;
    component_count = hlsl_type_component_count(rhs->data_type);

    if (!(body = hlsl_sprintf_alloc(ctx, template, component_count, component_count)))
        return false;

    if (!(func = hlsl_compile_internal_function(ctx, "soft_f32tof16", body)))
        return false;

    lhs = func->parameters.vars[0];
    hlsl_block_add_simple_store(ctx, block, lhs, rhs);

    if (!(call = hlsl_new_call(ctx, func, &node->loc)))
        return false;
    hlsl_block_add_instr(block, call);

    hlsl_block_add_simple_load(ctx, block, func->return_var, &node->loc);
    return true;
}

static bool lower_isinf(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, struct hlsl_block *block)
{
    struct hlsl_ir_function_decl *func;
    struct hlsl_ir_node *call, *rhs;
    unsigned int component_count;
    struct hlsl_ir_expr *expr;
    const char *template;
    char *body;

    static const char template_sm2[] =
        "typedef bool%u boolX;\n"
        "typedef float%u floatX;\n"
        "boolX isinf(floatX x)\n"
        "{\n"
        "    floatX v = 1 / x;\n"
        "    v = v * v;\n"
        "    return v <= 0;\n"
        "}\n";

    static const char template_sm3[] =
        "typedef bool%u boolX;\n"
        "typedef float%u floatX;\n"
        "boolX isinf(floatX x)\n"
        "{\n"
        "    floatX v = 1 / x;\n"
        "    return v <= 0;\n"
        "}\n";

    static const char template_sm4[] =
        "typedef bool%u boolX;\n"
        "typedef float%u floatX;\n"
        "boolX isinf(floatX x)\n"
        "{\n"
        "    return (asuint(x) & 0x7fffffff) == 0x7f800000;\n"
        "}\n";

    static const char template_int[] =
        "typedef bool%u boolX;\n"
        "typedef float%u floatX;\n"
        "boolX isinf(floatX x)\n"
        "{\n"
        "    return false;\n"
        "}";

    if (node->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(node);

    if (expr->op != HLSL_OP1_ISINF)
        return false;

    rhs = expr->operands[0].node;

    if (hlsl_version_lt(ctx, 3, 0))
        template = template_sm2;
    else if (hlsl_version_lt(ctx, 4, 0))
        template = template_sm3;
    else if (hlsl_type_is_integer(rhs->data_type))
        template = template_int;
    else
        template = template_sm4;

    component_count = hlsl_type_component_count(rhs->data_type);
    if (!(body = hlsl_sprintf_alloc(ctx, template, component_count, component_count)))
        return false;

    if (!(func = hlsl_compile_internal_function(ctx, "isinf", body)))
        return false;

    hlsl_block_add_simple_store(ctx, block, func->parameters.vars[0], rhs);

    if (!(call = hlsl_new_call(ctx, func, &node->loc)))
        return false;
    hlsl_block_add_instr(block, call);

    hlsl_block_add_simple_load(ctx, block, func->return_var, &node->loc);
    return true;
}

static void process_entry_function(struct hlsl_ctx *ctx,
        const struct hlsl_block *global_uniform_block, struct hlsl_ir_function_decl *entry_func)
{
    const struct hlsl_ir_var *input_patch = NULL, *output_patch = NULL;
    const struct hlsl_profile_info *profile = ctx->profile;
    struct hlsl_block static_initializers, global_uniforms;
    struct hlsl_block *const body = &entry_func->body;
    struct recursive_call_ctx recursive_call_ctx;
    struct stream_append_ctx stream_append_ctx;
    uint32_t output_reg_count;
    struct hlsl_ir_var *var;
    unsigned int i;
    bool progress;

    ctx->is_patch_constant_func = entry_func == ctx->patch_constant_func;

    if (!hlsl_clone_block(ctx, &static_initializers, &ctx->static_initializers))
        return;
    list_move_head(&body->instrs, &static_initializers.instrs);

    if (!hlsl_clone_block(ctx, &global_uniforms, global_uniform_block))
        return;
    list_move_head(&body->instrs, &global_uniforms.instrs);

    memset(&recursive_call_ctx, 0, sizeof(recursive_call_ctx));
    hlsl_transform_ir(ctx, find_recursive_calls, body, &recursive_call_ctx);
    vkd3d_free(recursive_call_ctx.backtrace);

    /* Avoid going into an infinite loop when processing call instructions.
     * lower_return() recurses into inferior calls. */
    if (ctx->result)
        return;

    if (hlsl_version_ge(ctx, 4, 0) && hlsl_version_lt(ctx, 5, 0))
    {
        lower_ir(ctx, lower_f16tof32, body);
        lower_ir(ctx, lower_f32tof16, body);
    }

    lower_ir(ctx, lower_isinf, body);

    lower_return(ctx, entry_func, body, false);

    while (hlsl_transform_ir(ctx, lower_calls, body, NULL));

    lower_ir(ctx, lower_complex_casts, body);
    lower_ir(ctx, lower_matrix_swizzles, body);
    lower_ir(ctx, lower_index_loads, body);

    for (i = 0; i < entry_func->parameters.count; ++i)
    {
        var = entry_func->parameters.vars[i];

        if (hlsl_type_is_resource(var->data_type))
        {
            prepend_uniform_copy(ctx, body, var);
        }
        else if ((var->storage_modifiers & HLSL_STORAGE_UNIFORM))
        {
            if (ctx->profile->type == VKD3D_SHADER_TYPE_HULL && ctx->is_patch_constant_func)
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Patch constant function parameter \"%s\" cannot be uniform.", var->name);
            else
                prepend_uniform_copy(ctx, body, var);
        }
        else if (hlsl_type_is_primitive_array(var->data_type))
        {
            if (var->storage_modifiers & HLSL_STORAGE_OUT)
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Input primitive parameter \"%s\" is declared as \"out\".", var->name);

            if (profile->type != VKD3D_SHADER_TYPE_GEOMETRY)
            {
                enum hlsl_array_type array_type = var->data_type->e.array.array_type;

                if (array_type == HLSL_ARRAY_PATCH_INPUT)
                {
                    if (input_patch)
                    {
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_DUPLICATE_PATCH,
                                "Found multiple InputPatch parameters.");
                        hlsl_note(ctx, &input_patch->loc, VKD3D_SHADER_LOG_ERROR,
                                "The InputPatch parameter was previously declared here.");
                        continue;
                    }
                    input_patch = var;
                }
                else if (array_type == HLSL_ARRAY_PATCH_OUTPUT)
                {
                    if (output_patch)
                    {
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_DUPLICATE_PATCH,
                                "Found multiple OutputPatch parameters.");
                        hlsl_note(ctx, &output_patch->loc, VKD3D_SHADER_LOG_ERROR,
                                "The OutputPatch parameter was previously declared here.");
                        continue;
                    }
                    output_patch = var;
                }
            }

            validate_and_record_prim_type(ctx, var);
            prepend_input_var_copy(ctx, entry_func, var);
        }
        else if (var->data_type->reg_size[HLSL_REGSET_STREAM_OUTPUTS])
        {
            if (profile->type != VKD3D_SHADER_TYPE_GEOMETRY)
            {
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                        "Stream output parameters can only be used in geometry shaders.");
                continue;
            }

            if (!(var->storage_modifiers & HLSL_STORAGE_IN) || !(var->storage_modifiers & HLSL_STORAGE_OUT))
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Stream output parameter \"%s\" must be declared as \"inout\".", var->name);

            prepend_uniform_copy(ctx, body, var);
        }
        else
        {
            if (hlsl_get_multiarray_element_type(var->data_type)->class != HLSL_CLASS_STRUCT
                    && !var->semantic.name)
            {
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_SEMANTIC,
                        "Parameter \"%s\" is missing a semantic.", var->name);
                var->semantic.reported_missing = true;
            }

            if (var->storage_modifiers & HLSL_STORAGE_IN)
            {
                if (profile->type == VKD3D_SHADER_TYPE_GEOMETRY && !var->semantic.name)
                {
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_PRIMITIVE_TYPE,
                            "Input parameter \"%s\" is missing a primitive type.", var->name);
                    continue;
                }

                prepend_input_var_copy(ctx, entry_func, var);
            }
            if (var->storage_modifiers & HLSL_STORAGE_OUT)
            {
                if (profile->type == VKD3D_SHADER_TYPE_HULL && !ctx->is_patch_constant_func)
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                            "Output parameters are not supported in hull shader control point functions.");
                else if (profile->type == VKD3D_SHADER_TYPE_GEOMETRY)
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                            "Output parameters are not allowed in geometry shaders.");
                else
                    append_output_var_copy(ctx, entry_func, var);
            }
        }
    }
    if (entry_func->return_var)
    {
        if (profile->type == VKD3D_SHADER_TYPE_GEOMETRY)
            hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                    "Geometry shaders cannot return values.");
        else if (entry_func->return_var->data_type->class != HLSL_CLASS_STRUCT
                && !entry_func->return_var->semantic.name)
            hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_SEMANTIC,
                    "Entry point \"%s\" is missing a return value semantic.", entry_func->func->name);

        append_output_var_copy(ctx, entry_func, entry_func->return_var);

        if (profile->type == VKD3D_SHADER_TYPE_HULL && !ctx->is_patch_constant_func)
            ctx->output_control_point_type = entry_func->return_var->data_type;
    }
    else
    {
        if (profile->type == VKD3D_SHADER_TYPE_HULL && !ctx->is_patch_constant_func)
            hlsl_fixme(ctx, &entry_func->loc, "Passthrough hull shader control point function.");
    }

    if (profile->type == VKD3D_SHADER_TYPE_GEOMETRY && ctx->input_primitive_type == VKD3D_PT_UNDEFINED)
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_PRIMITIVE_TYPE,
                "Entry point \"%s\" is missing an input primitive parameter.", entry_func->func->name);

    if (hlsl_version_ge(ctx, 4, 0))
    {
        hlsl_transform_ir(ctx, lower_discard_neg, body, NULL);
    }
    else
    {
        hlsl_transform_ir(ctx, lower_discard_nz, body, NULL);
        hlsl_transform_ir(ctx, lower_resource_load_bias, body, NULL);
    }

    compute_liveness(ctx, entry_func);
    transform_derefs(ctx, divert_written_uniform_derefs_to_temp, &entry_func->body);

    loop_unrolling_execute(ctx, body);
    hlsl_run_const_passes(ctx, body);

    remove_unreachable_code(ctx, body);
    hlsl_transform_ir(ctx, normalize_switch_cases, body, NULL);

    lower_ir(ctx, lower_nonconstant_vector_derefs, body);
    lower_ir(ctx, lower_casts_to_bool, body);
    lower_ir(ctx, lower_int_dot, body);

    if (hlsl_version_lt(ctx, 4, 0))
        hlsl_transform_ir(ctx, lower_separate_samples, body, NULL);

    hlsl_transform_ir(ctx, validate_dereferences, body, NULL);

    do
    {
        progress = vectorize_exprs(ctx, body);
        compute_liveness(ctx, entry_func);
        progress |= hlsl_transform_ir(ctx, dce, body, NULL);
        progress |= hlsl_transform_ir(ctx, fold_swizzle_chains, body, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_swizzles, body, NULL);
        progress |= vectorize_stores(ctx, body);
    } while (progress);

    hlsl_transform_ir(ctx, track_object_components_sampler_dim, body, NULL);

    if (hlsl_version_ge(ctx, 4, 0))
        hlsl_transform_ir(ctx, lower_combined_samples, body, NULL);

    do
        compute_liveness(ctx, entry_func);
    while (hlsl_transform_ir(ctx, dce, body, NULL));

    hlsl_transform_ir(ctx, track_components_usage, body, NULL);
    if (hlsl_version_lt(ctx, 4, 0))
        sort_synthetic_combined_samplers_first(ctx);
    else
        sort_synthetic_separated_samplers_first(ctx);

    if (profile->type == VKD3D_SHADER_TYPE_GEOMETRY)
    {
        allocate_stream_outputs(ctx);
        validate_and_record_stream_outputs(ctx);

        memset(&stream_append_ctx, 0, sizeof(stream_append_ctx));
        stream_append_ctx.func = entry_func;
        hlsl_transform_ir(ctx, lower_stream_appends, body, &stream_append_ctx);
    }

    if (profile->major_version < 4)
    {
        while (lower_ir(ctx, lower_nonconstant_array_loads, body));

        lower_ir(ctx, lower_ternary, body);
        lower_ir(ctx, lower_int_modulus_sm1, body);
        lower_ir(ctx, lower_division, body);
        /* Constants casted to float must be folded, and new casts to bool also need to be lowered. */
        hlsl_transform_ir(ctx, hlsl_fold_constant_exprs, body, NULL);
        lower_ir(ctx, lower_casts_to_bool, body);

        lower_ir(ctx, lower_casts_to_int, body);
        lower_ir(ctx, lower_trunc, body);
        lower_ir(ctx, lower_sqrt, body);
        lower_ir(ctx, lower_dot, body);
        lower_ir(ctx, lower_round, body);
        lower_ir(ctx, lower_ceil, body);
        lower_ir(ctx, lower_floor, body);
        lower_ir(ctx, lower_trig, body);
        lower_ir(ctx, lower_comparison_operators, body);
        lower_ir(ctx, lower_logic_not, body);
        if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
            lower_ir(ctx, lower_slt, body);
        else
            lower_ir(ctx, lower_cmp, body);
    }

    if (profile->major_version < 2)
    {
        lower_ir(ctx, lower_abs, body);
    }

    lower_ir(ctx, validate_nonconstant_vector_store_derefs, body);

    hlsl_run_folding_passes(ctx, body);

    do
        compute_liveness(ctx, entry_func);
    while (hlsl_transform_ir(ctx, dce, body, NULL));

    /* TODO: move forward, remove when no longer needed */
    transform_derefs(ctx, replace_deref_path_with_offset, body);
    simplify_exprs(ctx, body);
    transform_derefs(ctx, clean_constant_deref_offset_srcs, body);

    do
        compute_liveness(ctx, entry_func);
    while (hlsl_transform_ir(ctx, dce, body, NULL));

    compute_liveness(ctx, entry_func);
    mark_vars_usage(ctx);

    calculate_resource_register_counts(ctx);

    allocate_register_reservations(ctx, &ctx->extern_vars);
    allocate_register_reservations(ctx, &entry_func->extern_vars);
    allocate_semantic_registers(ctx, entry_func, &output_reg_count);

    if (profile->type == VKD3D_SHADER_TYPE_GEOMETRY)
        validate_max_output_size(ctx, entry_func, output_reg_count);
}

int hlsl_emit_bytecode(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func,
        enum vkd3d_shader_target_type target_type, struct vkd3d_shader_code *out)
{
    const struct hlsl_profile_info *profile = ctx->profile;
    struct hlsl_block global_uniform_block;
    struct hlsl_ir_var *var;

    parse_entry_function_attributes(ctx, entry_func);
    if (ctx->result)
        return ctx->result;

    if (profile->type == VKD3D_SHADER_TYPE_HULL)
        validate_hull_shader_attributes(ctx, entry_func);
    else if (profile->type == VKD3D_SHADER_TYPE_COMPUTE && !ctx->found_numthreads)
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [numthreads] attribute.", entry_func->func->name);
    else if (profile->type == VKD3D_SHADER_TYPE_DOMAIN && ctx->domain == VKD3D_TESSELLATOR_DOMAIN_INVALID)
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [domain] attribute.", entry_func->func->name);
    else if (profile->type == VKD3D_SHADER_TYPE_GEOMETRY && !ctx->max_vertex_count)
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [maxvertexcount] attribute.", entry_func->func->name);

    hlsl_block_init(&global_uniform_block);

    LIST_FOR_EACH_ENTRY(var, &ctx->globals->vars, struct hlsl_ir_var, scope_entry)
    {
        if (var->storage_modifiers & HLSL_STORAGE_UNIFORM)
            prepend_uniform_copy(ctx, &global_uniform_block, var);
    }

    process_entry_function(ctx, &global_uniform_block, entry_func);
    if (ctx->result)
        return ctx->result;

    if (profile->type == VKD3D_SHADER_TYPE_HULL)
    {
        process_entry_function(ctx, &global_uniform_block, ctx->patch_constant_func);
        if (ctx->result)
            return ctx->result;
    }

    hlsl_block_cleanup(&global_uniform_block);

    if (profile->major_version < 4)
    {
        mark_indexable_vars(ctx, entry_func);
        allocate_const_registers(ctx, entry_func);
        sort_uniforms_by_bind_count(ctx, HLSL_REGSET_SAMPLERS);
        allocate_objects(ctx, entry_func, HLSL_REGSET_SAMPLERS);
    }
    else
    {
        allocate_buffers(ctx);
        allocate_objects(ctx, entry_func, HLSL_REGSET_TEXTURES);
        allocate_objects(ctx, entry_func, HLSL_REGSET_UAVS);
        allocate_objects(ctx, entry_func, HLSL_REGSET_SAMPLERS);
    }

    if (TRACE_ON())
        rb_for_each_entry(&ctx->functions, dump_function, ctx);

    if (ctx->result)
        return ctx->result;

    switch (target_type)
    {
        case VKD3D_SHADER_TARGET_D3D_BYTECODE:
        {
            uint32_t config_flags = vkd3d_shader_init_config_flags();
            struct vkd3d_shader_code ctab = {0};
            struct vsir_program program;
            int result;

            sm1_generate_ctab(ctx, &ctab);
            if (ctx->result)
                return ctx->result;

            sm1_generate_vsir(ctx, entry_func, config_flags, &program);
            if (ctx->result)
            {
                vsir_program_cleanup(&program);
                vkd3d_shader_free_shader_code(&ctab);
                return ctx->result;
            }

            result = d3dbc_compile(&program, config_flags, NULL, &ctab, out, ctx->message_context);
            vsir_program_cleanup(&program);
            vkd3d_shader_free_shader_code(&ctab);
            return result;
        }

        case VKD3D_SHADER_TARGET_DXBC_TPF:
        {
            uint32_t config_flags = vkd3d_shader_init_config_flags();
            struct vkd3d_shader_code rdef = {0};
            struct vsir_program program;
            int result;

            sm4_generate_rdef(ctx, &rdef);
            if (ctx->result)
                return ctx->result;

            sm4_generate_vsir(ctx, entry_func, config_flags, &program);
            if (ctx->result)
            {
                vsir_program_cleanup(&program);
                vkd3d_shader_free_shader_code(&rdef);
                return ctx->result;
            }

            result = tpf_compile(&program, config_flags, &rdef, out, ctx->message_context);
            vsir_program_cleanup(&program);
            vkd3d_shader_free_shader_code(&rdef);
            return result;
        }

        default:
            ERR("Unsupported shader target type %#x.\n", target_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
    }
}
