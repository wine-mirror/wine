/*
 * HLSL parser
 *
 * Copyright 2008 Stefan Dösinger
 * Copyright 2012 Matteo Bruni for CodeWeavers
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

%code requires
{

#include "hlsl.h"
#include <stdio.h>

#define HLSL_YYLTYPE struct vkd3d_shader_location

struct parse_fields
{
    struct hlsl_struct_field *fields;
    size_t count, capacity;
};

struct parse_parameter
{
    struct hlsl_type *type;
    const char *name;
    struct hlsl_semantic semantic;
    struct hlsl_reg_reservation reg_reservation;
    unsigned int modifiers;
};

struct parse_colon_attribute
{
    struct hlsl_semantic semantic;
    struct hlsl_reg_reservation reg_reservation;
};

struct parse_initializer
{
    struct hlsl_ir_node **args;
    unsigned int args_count;
    struct hlsl_block *instrs;
    bool braces;
};

struct parse_array_sizes
{
    uint32_t *sizes; /* innermost first */
    unsigned int count;
};

struct parse_variable_def
{
    struct list entry;
    struct vkd3d_shader_location loc;

    char *name;
    struct parse_array_sizes arrays;
    struct hlsl_semantic semantic;
    struct hlsl_reg_reservation reg_reservation;
    struct parse_initializer initializer;

    struct hlsl_type *basic_type;
    unsigned int modifiers;
    struct vkd3d_shader_location modifiers_loc;
};

struct parse_function
{
    struct hlsl_ir_function_decl *decl;
    struct hlsl_func_parameters parameters;
    struct hlsl_semantic return_semantic;
    bool first;
};

struct parse_if_body
{
    struct hlsl_block *then_block;
    struct hlsl_block *else_block;
};

enum parse_assign_op
{
    ASSIGN_OP_ASSIGN,
    ASSIGN_OP_ADD,
    ASSIGN_OP_SUB,
    ASSIGN_OP_MUL,
    ASSIGN_OP_DIV,
    ASSIGN_OP_MOD,
    ASSIGN_OP_LSHIFT,
    ASSIGN_OP_RSHIFT,
    ASSIGN_OP_AND,
    ASSIGN_OP_OR,
    ASSIGN_OP_XOR,
};

struct parse_attribute_list
{
    unsigned int count;
    const struct hlsl_attribute **attrs;
};

}

%code provides
{

int yylex(HLSL_YYSTYPE *yylval_param, HLSL_YYLTYPE *yylloc_param, void *yyscanner);

}

%code
{

#define YYLLOC_DEFAULT(cur, rhs, n) (cur) = YYRHSLOC(rhs, !!n)

static void yyerror(YYLTYPE *loc, void *scanner, struct hlsl_ctx *ctx, const char *s)
{
    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "%s", s);
}

static struct hlsl_ir_node *node_from_block(struct hlsl_block *block)
{
    return LIST_ENTRY(list_tail(&block->instrs), struct hlsl_ir_node, entry);
}

static struct hlsl_block *make_empty_block(struct hlsl_ctx *ctx)
{
    struct hlsl_block *block;

    if ((block = hlsl_alloc(ctx, sizeof(*block))))
        hlsl_block_init(block);
    return block;
}

static struct list *make_empty_list(struct hlsl_ctx *ctx)
{
    struct list *list;

    if ((list = hlsl_alloc(ctx, sizeof(*list))))
        list_init(list);
    return list;
}

static void destroy_block(struct hlsl_block *block)
{
    hlsl_block_cleanup(block);
    vkd3d_free(block);
}

static void destroy_switch_cases(struct list *cases)
{
    hlsl_cleanup_ir_switch_cases(cases);
    vkd3d_free(cases);
}

static bool hlsl_types_are_componentwise_compatible(struct hlsl_ctx *ctx, struct hlsl_type *src,
        struct hlsl_type *dst)
{
    unsigned int k, count = hlsl_type_component_count(dst);

    if (count > hlsl_type_component_count(src))
        return false;

    for (k = 0; k < count; ++k)
    {
        struct hlsl_type *src_comp_type, *dst_comp_type;

        src_comp_type = hlsl_type_get_component_type(ctx, src, k);
        dst_comp_type = hlsl_type_get_component_type(ctx, dst, k);

        if ((src_comp_type->class != HLSL_CLASS_SCALAR || dst_comp_type->class != HLSL_CLASS_SCALAR)
                && !hlsl_types_are_equal(src_comp_type, dst_comp_type))
            return false;
    }
    return true;
}

static bool hlsl_types_are_componentwise_equal(struct hlsl_ctx *ctx, struct hlsl_type *src,
        struct hlsl_type *dst)
{
    unsigned int k, count = hlsl_type_component_count(src);

    if (count != hlsl_type_component_count(dst))
        return false;

    for (k = 0; k < count; ++k)
    {
        struct hlsl_type *src_comp_type, *dst_comp_type;

        src_comp_type = hlsl_type_get_component_type(ctx, src, k);
        dst_comp_type = hlsl_type_get_component_type(ctx, dst, k);

        if (!hlsl_types_are_equal(src_comp_type, dst_comp_type))
            return false;
    }
    return true;
}

static bool type_contains_only_numerics(const struct hlsl_type *type)
{
    unsigned int i;

    if (type->class == HLSL_CLASS_ARRAY)
        return type_contains_only_numerics(type->e.array.type);
    if (type->class == HLSL_CLASS_STRUCT)
    {
        for (i = 0; i < type->e.record.field_count; ++i)
        {
            if (!type_contains_only_numerics(type->e.record.fields[i].type))
                return false;
        }
        return true;
    }
    return hlsl_is_numeric_type(type);
}

static bool explicit_compatible_data_types(struct hlsl_ctx *ctx, struct hlsl_type *src, struct hlsl_type *dst)
{
    if (hlsl_is_numeric_type(src) && src->dimx == 1 && src->dimy == 1 && type_contains_only_numerics(dst))
        return true;

    if (src->class == HLSL_CLASS_MATRIX && dst->class == HLSL_CLASS_MATRIX
            && src->dimx >= dst->dimx && src->dimy >= dst->dimy)
        return true;

    if ((src->class == HLSL_CLASS_MATRIX && src->dimx > 1 && src->dimy > 1)
            && hlsl_type_component_count(src) != hlsl_type_component_count(dst))
        return false;

    if ((dst->class == HLSL_CLASS_MATRIX && dst->dimy > 1)
            && hlsl_type_component_count(src) != hlsl_type_component_count(dst))
        return false;

    return hlsl_types_are_componentwise_compatible(ctx, src, dst);
}

static bool implicit_compatible_data_types(struct hlsl_ctx *ctx, struct hlsl_type *src, struct hlsl_type *dst)
{
    if (hlsl_is_numeric_type(src) != hlsl_is_numeric_type(dst))
        return false;

    if (hlsl_is_numeric_type(src))
    {
        /* Scalar vars can be converted to any other numeric data type */
        if (src->dimx == 1 && src->dimy == 1)
            return true;
        /* The other way around is true too */
        if (dst->dimx == 1 && dst->dimy == 1)
            return true;

        if (src->class == HLSL_CLASS_MATRIX || dst->class == HLSL_CLASS_MATRIX)
        {
            if (src->class == HLSL_CLASS_MATRIX && dst->class == HLSL_CLASS_MATRIX)
                return src->dimx >= dst->dimx && src->dimy >= dst->dimy;

            /* Matrix-vector conversion is apparently allowed if they have
            * the same components count, or if the matrix is 1xN or Nx1
            * and we are reducing the component count */
            if (src->class == HLSL_CLASS_VECTOR || dst->class == HLSL_CLASS_VECTOR)
            {
                if (hlsl_type_component_count(src) == hlsl_type_component_count(dst))
                    return true;

                if ((src->class == HLSL_CLASS_VECTOR || src->dimx == 1 || src->dimy == 1) &&
                        (dst->class == HLSL_CLASS_VECTOR || dst->dimx == 1 || dst->dimy == 1))
                    return hlsl_type_component_count(src) >= hlsl_type_component_count(dst);
            }

            return false;
        }
        else
        {
            return src->dimx >= dst->dimx;
        }
    }

    return hlsl_types_are_componentwise_equal(ctx, src, dst);
}

static struct hlsl_ir_node *add_cast(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_node *node, struct hlsl_type *dst_type, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *src_type = node->data_type;
    struct hlsl_ir_node *cast;

    if (hlsl_types_are_equal(src_type, dst_type))
        return node;

    if (src_type->class > HLSL_CLASS_VECTOR || dst_type->class > HLSL_CLASS_VECTOR)
    {
        unsigned int src_comp_count = hlsl_type_component_count(src_type);
        unsigned int dst_comp_count = hlsl_type_component_count(dst_type);
        struct hlsl_deref var_deref;
        bool broadcast, matrix_cast;
        struct hlsl_ir_load *load;
        struct hlsl_ir_var *var;
        unsigned int dst_idx;

        broadcast = hlsl_is_numeric_type(src_type) && src_type->dimx == 1 && src_type->dimy == 1;
        matrix_cast = !broadcast && dst_comp_count != src_comp_count
                && src_type->class == HLSL_CLASS_MATRIX && dst_type->class == HLSL_CLASS_MATRIX;
        assert(src_comp_count >= dst_comp_count || broadcast);
        if (matrix_cast)
        {
            assert(dst_type->dimx <= src_type->dimx);
            assert(dst_type->dimy <= src_type->dimy);
        }

        if (!(var = hlsl_new_synthetic_var(ctx, "cast", dst_type, loc)))
            return NULL;
        hlsl_init_simple_deref_from_var(&var_deref, var);

        for (dst_idx = 0; dst_idx < dst_comp_count; ++dst_idx)
        {
            struct hlsl_ir_node *component_load;
            struct hlsl_type *dst_comp_type;
            struct hlsl_block store_block;
            unsigned int src_idx;

            if (broadcast)
            {
                src_idx = 0;
            }
            else if (matrix_cast)
            {
                unsigned int x = dst_idx % dst_type->dimx, y = dst_idx / dst_type->dimx;

                src_idx = y * src_type->dimx + x;
            }
            else
            {
                src_idx = dst_idx;
            }

            dst_comp_type = hlsl_type_get_component_type(ctx, dst_type, dst_idx);

            if (!(component_load = hlsl_add_load_component(ctx, block, node, src_idx, loc)))
                return NULL;

            if (!(cast = hlsl_new_cast(ctx, component_load, dst_comp_type, loc)))
                return NULL;
            hlsl_block_add_instr(block, cast);

            if (!hlsl_new_store_component(ctx, &store_block, &var_deref, dst_idx, cast))
                return NULL;
            hlsl_block_add_block(block, &store_block);
        }

        if (!(load = hlsl_new_var_load(ctx, var, loc)))
            return NULL;
        hlsl_block_add_instr(block, &load->node);

        return &load->node;
    }
    else
    {
        if (!(cast = hlsl_new_cast(ctx, node, dst_type, loc)))
            return NULL;
        hlsl_block_add_instr(block, cast);
        return cast;
    }
}

static struct hlsl_ir_node *add_implicit_conversion(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_node *node, struct hlsl_type *dst_type, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *src_type = node->data_type;

    if (hlsl_types_are_equal(src_type, dst_type))
        return node;

    if (!implicit_compatible_data_types(ctx, src_type, dst_type))
    {
        struct vkd3d_string_buffer *src_string, *dst_string;

        src_string = hlsl_type_to_string(ctx, src_type);
        dst_string = hlsl_type_to_string(ctx, dst_type);
        if (src_string && dst_string)
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Can't implicitly convert from %s to %s.", src_string->buffer, dst_string->buffer);
        hlsl_release_string_buffer(ctx, src_string);
        hlsl_release_string_buffer(ctx, dst_string);
        return NULL;
    }

    if (dst_type->dimx * dst_type->dimy < src_type->dimx * src_type->dimy)
        hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_IMPLICIT_TRUNCATION, "Implicit truncation of %s type.",
                src_type->class == HLSL_CLASS_VECTOR ? "vector" : "matrix");

    return add_cast(ctx, block, node, dst_type, loc);
}

static DWORD add_modifiers(struct hlsl_ctx *ctx, DWORD modifiers, DWORD mod,
        const struct vkd3d_shader_location *loc)
{
    if (modifiers & mod)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_modifiers_to_string(ctx, mod)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                    "Modifier '%s' was already specified.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return modifiers;
    }
    return modifiers | mod;
}

static bool append_conditional_break(struct hlsl_ctx *ctx, struct hlsl_block *cond_block)
{
    struct hlsl_ir_node *condition, *not, *iff, *jump;
    struct hlsl_block then_block;

    /* E.g. "for (i = 0; ; ++i)". */
    if (list_empty(&cond_block->instrs))
        return true;

    condition = node_from_block(cond_block);
    if (!(not = hlsl_new_unary_expr(ctx, HLSL_OP1_LOGIC_NOT, condition, &condition->loc)))
        return false;
    hlsl_block_add_instr(cond_block, not);

    hlsl_block_init(&then_block);

    if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_BREAK, NULL, &condition->loc)))
        return false;
    hlsl_block_add_instr(&then_block, jump);

    if (!(iff = hlsl_new_if(ctx, not, &then_block, NULL, &condition->loc)))
        return false;
    hlsl_block_add_instr(cond_block, iff);
    return true;
}

enum loop_type
{
    LOOP_FOR,
    LOOP_WHILE,
    LOOP_DO_WHILE
};

static bool attribute_list_has_duplicates(const struct parse_attribute_list *attrs)
{
    unsigned int i, j;

    for (i = 0; i < attrs->count; ++i)
    {
        for (j = i + 1; j < attrs->count; ++j)
        {
            if (!strcmp(attrs->attrs[i]->name, attrs->attrs[j]->name))
                 return true;
        }
    }

    return false;
}

static void resolve_loop_continue(struct hlsl_ctx *ctx, struct hlsl_block *block, enum loop_type type,
        struct hlsl_block *cond, struct hlsl_block *iter)
{
    struct hlsl_ir_node *instr, *next;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            resolve_loop_continue(ctx, &iff->then_block, type, cond, iter);
            resolve_loop_continue(ctx, &iff->else_block, type, cond, iter);
        }
        else if (instr->type == HLSL_IR_JUMP)
        {
            struct hlsl_ir_jump *jump = hlsl_ir_jump(instr);
            struct hlsl_block block;

            if (jump->type != HLSL_IR_JUMP_UNRESOLVED_CONTINUE)
                continue;

            if (type == LOOP_DO_WHILE)
            {
                if (!hlsl_clone_block(ctx, &block, cond))
                    return;
                if (!append_conditional_break(ctx, &block))
                {
                    hlsl_block_cleanup(&block);
                    return;
                }
                list_move_before(&instr->entry, &block.instrs);
            }
            else if (type == LOOP_FOR)
            {
                if (!hlsl_clone_block(ctx, &block, iter))
                    return;
                list_move_before(&instr->entry, &block.instrs);
            }
            jump->type = HLSL_IR_JUMP_CONTINUE;
        }
    }
}

static void check_loop_attributes(struct hlsl_ctx *ctx, const struct parse_attribute_list *attributes,
        const struct vkd3d_shader_location *loc)
{
    bool has_unroll = false, has_loop = false, has_fastopt = false;
    unsigned int i;

    for (i = 0; i < attributes->count; ++i)
    {
        const char *name = attributes->attrs[i]->name;

        has_loop |= !strcmp(name, "loop");
        has_unroll |= !strcmp(name, "unroll");
        has_fastopt |= !strcmp(name, "fastopt");
    }

    if (has_unroll && has_loop)
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Unroll attribute can't be used with 'loop' attribute.");

    if (has_unroll && has_fastopt)
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Unroll attribute can't be used with 'fastopt' attribute.");
}

static struct hlsl_block *create_loop(struct hlsl_ctx *ctx, enum loop_type type,
        const struct parse_attribute_list *attributes, struct hlsl_block *init, struct hlsl_block *cond,
        struct hlsl_block *iter, struct hlsl_block *body, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *loop;
    unsigned int i;

    if (attribute_list_has_duplicates(attributes))
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Found duplicate attribute.");

    check_loop_attributes(ctx, attributes, loc);

    /* Ignore unroll(0) attribute, and any invalid attribute. */
    for (i = 0; i < attributes->count; ++i)
    {
        const struct hlsl_attribute *attr = attributes->attrs[i];
        if (!strcmp(attr->name, "unroll"))
        {
            if (attr->args_count)
            {
                hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NOT_IMPLEMENTED, "Unroll attribute with iteration count.");
            }
            else
            {
                hlsl_warning(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NOT_IMPLEMENTED, "Loop unrolling is not implemented.");
            }
        }
        else if (!strcmp(attr->name, "loop"))
        {
            /* TODO: this attribute will be used to disable unrolling, once it's implememented. */
        }
        else if (!strcmp(attr->name, "fastopt")
                || !strcmp(attr->name, "allow_uav_condition"))
        {
            hlsl_fixme(ctx, loc, "Unhandled attribute '%s'.", attr->name);
        }
        else
        {
            hlsl_warning(ctx, loc, VKD3D_SHADER_WARNING_HLSL_UNKNOWN_ATTRIBUTE, "Unrecognized attribute '%s'.", attr->name);
        }
    }

    resolve_loop_continue(ctx, body, type, cond, iter);

    if (!init && !(init = make_empty_block(ctx)))
        goto oom;

    if (!append_conditional_break(ctx, cond))
        goto oom;

    if (iter)
        hlsl_block_add_block(body, iter);

    if (type == LOOP_DO_WHILE)
        list_move_tail(&body->instrs, &cond->instrs);
    else
        list_move_head(&body->instrs, &cond->instrs);

    if (!(loop = hlsl_new_loop(ctx, body, loc)))
        goto oom;
    hlsl_block_add_instr(init, loop);

    destroy_block(cond);
    destroy_block(body);
    destroy_block(iter);
    return init;

oom:
    destroy_block(init);
    destroy_block(cond);
    destroy_block(iter);
    destroy_block(body);
    return NULL;
}

static unsigned int initializer_size(const struct parse_initializer *initializer)
{
    unsigned int count = 0, i;

    for (i = 0; i < initializer->args_count; ++i)
    {
        count += hlsl_type_component_count(initializer->args[i]->data_type);
    }
    return count;
}

static void free_parse_initializer(struct parse_initializer *initializer)
{
    destroy_block(initializer->instrs);
    vkd3d_free(initializer->args);
}

static struct hlsl_ir_node *get_swizzle(struct hlsl_ctx *ctx, struct hlsl_ir_node *value, const char *swizzle,
        struct vkd3d_shader_location *loc)
{
    unsigned int len = strlen(swizzle), component = 0;
    unsigned int i, set, swiz = 0;
    bool valid;

    if (value->data_type->class == HLSL_CLASS_MATRIX)
    {
        /* Matrix swizzle */
        bool m_swizzle;
        unsigned int inc, x, y;

        if (len < 3 || swizzle[0] != '_')
            return NULL;
        m_swizzle = swizzle[1] == 'm';
        inc = m_swizzle ? 4 : 3;

        if (len % inc || len > inc * 4)
            return NULL;

        for (i = 0; i < len; i += inc)
        {
            if (swizzle[i] != '_')
                return NULL;
            if (m_swizzle)
            {
                if (swizzle[i + 1] != 'm')
                    return NULL;
                y = swizzle[i + 2] - '0';
                x = swizzle[i + 3] - '0';
            }
            else
            {
                y = swizzle[i + 1] - '1';
                x = swizzle[i + 2] - '1';
            }

            if (x >= value->data_type->dimx || y >= value->data_type->dimy)
                return NULL;
            swiz |= (y << 4 | x) << component * 8;
            component++;
        }
        return hlsl_new_swizzle(ctx, swiz, component, value, loc);
    }

    /* Vector swizzle */
    if (len > 4)
        return NULL;

    for (set = 0; set < 2; ++set)
    {
        valid = true;
        component = 0;
        for (i = 0; i < len; ++i)
        {
            char c[2][4] = {{'x', 'y', 'z', 'w'}, {'r', 'g', 'b', 'a'}};
            unsigned int s = 0;

            for (s = 0; s < 4; ++s)
            {
                if (swizzle[i] == c[set][s])
                    break;
            }
            if (s == 4)
            {
                valid = false;
                break;
            }

            if (s >= value->data_type->dimx)
                return NULL;
            swiz |= s << component * 2;
            component++;
        }
        if (valid)
            return hlsl_new_swizzle(ctx, swiz, component, value, loc);
    }

    return NULL;
}

static bool add_return(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_node *return_value, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *return_type = ctx->cur_function->return_type;
    struct hlsl_ir_node *jump;

    if (ctx->cur_function->return_var)
    {
        if (return_value)
        {
            struct hlsl_ir_node *store;

            if (!(return_value = add_implicit_conversion(ctx, block, return_value, return_type, loc)))
                return false;

            if (!(store = hlsl_new_simple_store(ctx, ctx->cur_function->return_var, return_value)))
                return false;
            list_add_after(&return_value->entry, &store->entry);
        }
        else
        {
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RETURN, "Non-void functions must return a value.");
            return false;
        }
    }
    else
    {
        if (return_value)
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RETURN, "Void functions cannot return a value.");
    }

    if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_RETURN, NULL, loc)))
        return false;
    hlsl_block_add_instr(block, jump);

    return true;
}

struct hlsl_ir_node *hlsl_add_load_component(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_node *var_instr, unsigned int comp, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *load, *store;
    struct hlsl_block load_block;
    struct hlsl_ir_var *var;
    struct hlsl_deref src;

    if (!(var = hlsl_new_synthetic_var(ctx, "deref", var_instr->data_type, &var_instr->loc)))
        return NULL;

    if (!(store = hlsl_new_simple_store(ctx, var, var_instr)))
        return NULL;
    hlsl_block_add_instr(block, store);

    hlsl_init_simple_deref_from_var(&src, var);
    if (!(load = hlsl_new_load_component(ctx, &load_block, &src, comp, loc)))
        return NULL;
    hlsl_block_add_block(block, &load_block);

    return load;
}

static bool add_record_access(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *record,
        unsigned int idx, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *index, *c;

    assert(idx < record->data_type->e.record.field_count);

    if (!(c = hlsl_new_uint_constant(ctx, idx, loc)))
        return false;
    hlsl_block_add_instr(block, c);

    if (!(index = hlsl_new_index(ctx, record, c, loc)))
        return false;
    hlsl_block_add_instr(block, index);

    return true;
}

static struct hlsl_ir_node *add_binary_arithmetic_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc);

static bool add_array_access(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *array,
        struct hlsl_ir_node *index, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *expr_type = array->data_type, *index_type = index->data_type;
    struct hlsl_ir_node *return_index, *cast;

    if (expr_type->class == HLSL_CLASS_OBJECT
            && (expr_type->base_type == HLSL_TYPE_TEXTURE || expr_type->base_type == HLSL_TYPE_UAV)
            && expr_type->sampler_dim != HLSL_SAMPLER_DIM_GENERIC)
    {
        unsigned int dim_count = hlsl_sampler_dim_count(expr_type->sampler_dim);

        if (index_type->class > HLSL_CLASS_VECTOR || index_type->dimx != dim_count)
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_type_to_string(ctx, expr_type)))
                hlsl_error(ctx, &index->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Array index of type '%s' must be of type 'uint%u'.", string->buffer, dim_count);
            hlsl_release_string_buffer(ctx, string);
            return false;
        }

        if (!(index = add_implicit_conversion(ctx, block, index,
                hlsl_get_vector_type(ctx, HLSL_TYPE_UINT, dim_count), &index->loc)))
            return false;

        if (!(return_index = hlsl_new_index(ctx, array, index, loc)))
            return false;
        hlsl_block_add_instr(block, return_index);

        return true;
    }

    if (index_type->class != HLSL_CLASS_SCALAR)
    {
        hlsl_error(ctx, &index->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Array index is not scalar.");
        return false;
    }

    if (!(cast = hlsl_new_cast(ctx, index, hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT), &index->loc)))
        return false;
    hlsl_block_add_instr(block, cast);
    index = cast;

    if (expr_type->class != HLSL_CLASS_ARRAY && expr_type->class != HLSL_CLASS_VECTOR && expr_type->class != HLSL_CLASS_MATRIX)
    {
        if (expr_type->class == HLSL_CLASS_SCALAR)
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_INDEX, "Scalar expressions cannot be array-indexed.");
        else
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_INDEX, "Expression cannot be array-indexed.");
        return false;
    }

    if (!(return_index = hlsl_new_index(ctx, array, index, loc)))
        return false;
    hlsl_block_add_instr(block, return_index);

    return true;
}

static const struct hlsl_struct_field *get_struct_field(const struct hlsl_struct_field *fields,
        size_t count, const char *name)
{
    size_t i;

    for (i = 0; i < count; ++i)
    {
        if (!strcmp(fields[i].name, name))
            return &fields[i];
    }
    return NULL;
}

static struct hlsl_type *apply_type_modifiers(struct hlsl_ctx *ctx, struct hlsl_type *type,
        unsigned int *modifiers, bool force_majority, const struct vkd3d_shader_location *loc)
{
    unsigned int default_majority = 0;
    struct hlsl_type *new_type;

    if (!(*modifiers & HLSL_MODIFIERS_MAJORITY_MASK)
            && !(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK)
            && type->class == HLSL_CLASS_MATRIX)
    {
        if (!(default_majority = ctx->matrix_majority) && force_majority)
            default_majority = HLSL_MODIFIER_COLUMN_MAJOR;
    }
    else if (type->class != HLSL_CLASS_MATRIX && (*modifiers & HLSL_MODIFIERS_MAJORITY_MASK))
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                "'row_major' and 'column_major' modifiers are only allowed for matrices.");
    }

    if (!default_majority && !(*modifiers & HLSL_TYPE_MODIFIERS_MASK))
        return type;

    if (!(new_type = hlsl_type_clone(ctx, type, default_majority, *modifiers & HLSL_TYPE_MODIFIERS_MASK)))
        return NULL;

    *modifiers &= ~HLSL_TYPE_MODIFIERS_MASK;

    if ((new_type->modifiers & HLSL_MODIFIER_ROW_MAJOR) && (new_type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR))
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                "'row_major' and 'column_major' modifiers are mutually exclusive.");

    return new_type;
}

static void free_parse_variable_def(struct parse_variable_def *v)
{
    free_parse_initializer(&v->initializer);
    vkd3d_free(v->arrays.sizes);
    vkd3d_free(v->name);
    hlsl_cleanup_semantic(&v->semantic);
    vkd3d_free(v);
}

static bool shader_is_sm_5_1(const struct hlsl_ctx *ctx)
{
    return ctx->profile->major_version == 5 && ctx->profile->minor_version >= 1;
}

static bool shader_profile_version_ge(const struct hlsl_ctx *ctx, unsigned int major, unsigned int minor)
{
    return ctx->profile->major_version > major || (ctx->profile->major_version == major && ctx->profile->minor_version >= minor);
}

static bool shader_profile_version_lt(const struct hlsl_ctx *ctx, unsigned int major, unsigned int minor)
{
    return !shader_profile_version_ge(ctx, major, minor);
}

static bool gen_struct_fields(struct hlsl_ctx *ctx, struct parse_fields *fields,
        struct hlsl_type *type, unsigned int modifiers, struct list *defs)
{
    struct parse_variable_def *v, *v_next;
    size_t i = 0;

    if (type->class == HLSL_CLASS_MATRIX)
        assert(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);

    memset(fields, 0, sizeof(*fields));
    fields->count = list_count(defs);
    if (!hlsl_array_reserve(ctx, (void **)&fields->fields, &fields->capacity, fields->count, sizeof(*fields->fields)))
        return false;

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, defs, struct parse_variable_def, entry)
    {
        struct hlsl_struct_field *field = &fields->fields[i++];
        bool unbounded_res_array = false;
        unsigned int k;

        field->type = type;

        if (shader_is_sm_5_1(ctx) && type->class == HLSL_CLASS_OBJECT)
        {
            for (k = 0; k < v->arrays.count; ++k)
                unbounded_res_array |= (v->arrays.sizes[k] == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT);
        }

        if (unbounded_res_array)
        {
            if (v->arrays.count == 1)
            {
                hlsl_fixme(ctx, &v->loc, "Unbounded resource arrays as struct fields.");
                free_parse_variable_def(v);
                vkd3d_free(field);
                continue;
            }
            else
            {
                hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Unbounded resource arrays cannot be multi-dimensional.");
            }
        }
        else
        {
            for (k = 0; k < v->arrays.count; ++k)
            {
                if (v->arrays.sizes[k] == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT)
                {
                    hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Implicit size arrays not allowed in struct fields.");
                }

                field->type = hlsl_new_array_type(ctx, field->type, v->arrays.sizes[k]);
            }
        }
        vkd3d_free(v->arrays.sizes);
        field->loc = v->loc;
        field->name = v->name;
        field->semantic = v->semantic;
        field->storage_modifiers = modifiers;
        if (v->initializer.args_count)
        {
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Illegal initializer on a struct field.");
            free_parse_initializer(&v->initializer);
        }
        if (v->reg_reservation.offset_type)
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                "packoffset() is not allowed inside struct definitions.");
        vkd3d_free(v);
    }
    vkd3d_free(defs);
    return true;
}

static bool add_typedef(struct hlsl_ctx *ctx, struct hlsl_type *const orig_type, struct list *list)
{
    struct parse_variable_def *v, *v_next;
    struct hlsl_type *type;
    unsigned int i;
    bool ret;

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, list, struct parse_variable_def, entry)
    {
        if (!v->arrays.count)
        {
            if (!(type = hlsl_type_clone(ctx, orig_type, 0, 0)))
            {
                free_parse_variable_def(v);
                continue;
            }
        }
        else
        {
            unsigned int var_modifiers = 0;

            if (!(type = apply_type_modifiers(ctx, orig_type, &var_modifiers, true, &v->loc)))
            {
                free_parse_variable_def(v);
                continue;
            }
        }

        ret = true;
        for (i = 0; i < v->arrays.count; ++i)
        {
            if (v->arrays.sizes[i] == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT)
            {
                hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Implicit size arrays not allowed in typedefs.");
            }

            if (!(type = hlsl_new_array_type(ctx, type, v->arrays.sizes[i])))
            {
                free_parse_variable_def(v);
                ret = false;
                break;
            }
        }
        if (!ret)
            continue;
        vkd3d_free(v->arrays.sizes);

        vkd3d_free((void *)type->name);
        type->name = v->name;

        ret = hlsl_scope_add_type(ctx->cur_scope, type);
        if (!ret)
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                    "Type '%s' is already defined.", v->name);
        free_parse_initializer(&v->initializer);
        vkd3d_free(v);
    }
    vkd3d_free(list);
    return true;
}

static bool add_func_parameter(struct hlsl_ctx *ctx, struct hlsl_func_parameters *parameters,
        struct parse_parameter *param, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_var *var;

    if (param->type->class == HLSL_CLASS_MATRIX)
        assert(param->type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);

    if ((param->modifiers & HLSL_STORAGE_OUT) && (param->modifiers & HLSL_STORAGE_UNIFORM))
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                "Parameter '%s' is declared as both \"out\" and \"uniform\".", param->name);

    if (param->reg_reservation.offset_type)
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                "packoffset() is not allowed on function parameters.");

    if (!(var = hlsl_new_var(ctx, param->name, param->type, loc, &param->semantic, param->modifiers,
            &param->reg_reservation)))
        return false;
    var->is_param = 1;

    if (!hlsl_add_var(ctx, var, false))
    {
        hlsl_free_var(var);
        return false;
    }

    if (!hlsl_array_reserve(ctx, (void **)&parameters->vars, &parameters->capacity,
            parameters->count + 1, sizeof(*parameters->vars)))
        return false;
    parameters->vars[parameters->count++] = var;
    return true;
}

static struct hlsl_reg_reservation parse_reg_reservation(const char *reg_string)
{
    struct hlsl_reg_reservation reservation = {0};

    if (!sscanf(reg_string + 1, "%u", &reservation.reg_index))
    {
        FIXME("Unsupported register reservation syntax.\n");
        return reservation;
    }
    reservation.reg_type = ascii_tolower(reg_string[0]);
    return reservation;
}

static struct hlsl_reg_reservation parse_packoffset(struct hlsl_ctx *ctx, const char *reg_string,
        const char *swizzle, const struct vkd3d_shader_location *loc)
{
    struct hlsl_reg_reservation reservation = {0};
    char *endptr;

    if (shader_profile_version_lt(ctx, 4, 0))
        return reservation;

    reservation.offset_index = strtoul(reg_string + 1, &endptr, 10);
    if (*endptr)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                "Invalid packoffset() syntax.");
        return reservation;
    }

    reservation.offset_type = ascii_tolower(reg_string[0]);
    if (reservation.offset_type != 'c')
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                "Only 'c' registers are allowed in packoffset().");
        return reservation;
    }

    reservation.offset_index *= 4;

    if (swizzle)
    {
        if (strlen(swizzle) != 1)
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                    "Invalid packoffset() component \"%s\".", swizzle);

        if (swizzle[0] == 'x' || swizzle[0] == 'r')
            reservation.offset_index += 0;
        else if (swizzle[0] == 'y' || swizzle[0] == 'g')
            reservation.offset_index += 1;
        else if (swizzle[0] == 'z' || swizzle[0] == 'b')
            reservation.offset_index += 2;
        else if (swizzle[0] == 'w' || swizzle[0] == 'a')
            reservation.offset_index += 3;
        else
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                    "Invalid packoffset() component \"%s\".", swizzle);
    }

    return reservation;
}

static struct hlsl_block *make_block(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr)
{
    struct hlsl_block *block;

    if (!(block = make_empty_block(ctx)))
    {
        hlsl_free_instr(instr);
        return NULL;
    }
    hlsl_block_add_instr(block, instr);
    return block;
}

static unsigned int evaluate_static_expression_as_uint(struct hlsl_ctx *ctx, struct hlsl_block *block,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_constant *constant;
    struct hlsl_ir_node *node;
    struct hlsl_block expr;
    unsigned int ret = 0;
    bool progress;

    LIST_FOR_EACH_ENTRY(node, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (node->type)
        {
            case HLSL_IR_CONSTANT:
            case HLSL_IR_EXPR:
            case HLSL_IR_SWIZZLE:
            case HLSL_IR_LOAD:
            case HLSL_IR_INDEX:
                continue;
            case HLSL_IR_CALL:
            case HLSL_IR_IF:
            case HLSL_IR_LOOP:
            case HLSL_IR_JUMP:
            case HLSL_IR_RESOURCE_LOAD:
            case HLSL_IR_RESOURCE_STORE:
            case HLSL_IR_STORE:
            case HLSL_IR_SWITCH:
                hlsl_error(ctx, &node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                        "Expected literal expression.");
        }
    }

    if (!hlsl_clone_block(ctx, &expr, &ctx->static_initializers))
        return 0;
    hlsl_block_add_block(&expr, block);

    if (!add_implicit_conversion(ctx, &expr, node_from_block(&expr),
            hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT), loc))
    {
        hlsl_block_cleanup(&expr);
        return 0;
    }

    do
    {
        progress = hlsl_transform_ir(ctx, hlsl_fold_constant_exprs, &expr, NULL);
        progress |= hlsl_copy_propagation_execute(ctx, &expr);
    } while (progress);

    node = node_from_block(&expr);
    if (node->type == HLSL_IR_CONSTANT)
    {
        constant = hlsl_ir_constant(node);
        ret = constant->value.u[0].u;
    }
    else
    {
        hlsl_error(ctx, &node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                "Failed to evaluate constant expression.");
    }

    hlsl_block_cleanup(&expr);

    return ret;
}

static bool expr_compatible_data_types(struct hlsl_type *t1, struct hlsl_type *t2)
{
    if (t1->base_type > HLSL_TYPE_LAST_SCALAR || t2->base_type > HLSL_TYPE_LAST_SCALAR)
        return false;

    /* Scalar vars can be converted to pretty much everything */
    if ((t1->dimx == 1 && t1->dimy == 1) || (t2->dimx == 1 && t2->dimy == 1))
        return true;

    if (t1->class == HLSL_CLASS_VECTOR && t2->class == HLSL_CLASS_VECTOR)
        return true;

    if (t1->class == HLSL_CLASS_MATRIX || t2->class == HLSL_CLASS_MATRIX)
    {
        /* Matrix-vector conversion is apparently allowed if either they have the same components
           count or the matrix is nx1 or 1xn */
        if (t1->class == HLSL_CLASS_VECTOR || t2->class == HLSL_CLASS_VECTOR)
        {
            if (hlsl_type_component_count(t1) == hlsl_type_component_count(t2))
                return true;

            return (t1->class == HLSL_CLASS_MATRIX && (t1->dimx == 1 || t1->dimy == 1))
                    || (t2->class == HLSL_CLASS_MATRIX && (t2->dimx == 1 || t2->dimy == 1));
        }

        /* Both matrices */
        if ((t1->dimx >= t2->dimx && t1->dimy >= t2->dimy)
                || (t1->dimx <= t2->dimx && t1->dimy <= t2->dimy))
            return true;
    }

    return false;
}

static enum hlsl_base_type expr_common_base_type(enum hlsl_base_type t1, enum hlsl_base_type t2)
{
    if (t1 > HLSL_TYPE_LAST_SCALAR || t2 > HLSL_TYPE_LAST_SCALAR) {
        FIXME("Unexpected base type.\n");
        return HLSL_TYPE_FLOAT;
    }
    if (t1 == t2)
        return t1 == HLSL_TYPE_BOOL ? HLSL_TYPE_INT : t1;
    if (t1 == HLSL_TYPE_DOUBLE || t2 == HLSL_TYPE_DOUBLE)
        return HLSL_TYPE_DOUBLE;
    if (t1 == HLSL_TYPE_FLOAT || t2 == HLSL_TYPE_FLOAT
            || t1 == HLSL_TYPE_HALF || t2 == HLSL_TYPE_HALF)
        return HLSL_TYPE_FLOAT;
    if (t1 == HLSL_TYPE_UINT || t2 == HLSL_TYPE_UINT)
        return HLSL_TYPE_UINT;
    return HLSL_TYPE_INT;
}

static bool expr_common_shape(struct hlsl_ctx *ctx, struct hlsl_type *t1, struct hlsl_type *t2,
        const struct vkd3d_shader_location *loc, enum hlsl_type_class *type, unsigned int *dimx, unsigned int *dimy)
{
    if (!hlsl_is_numeric_type(t1))
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, t1)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Expression of type \"%s\" cannot be used in a numeric expression.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (!hlsl_is_numeric_type(t2))
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, t2)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Expression of type \"%s\" cannot be used in a numeric expression.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (!expr_compatible_data_types(t1, t2))
    {
        struct vkd3d_string_buffer *t1_string = hlsl_type_to_string(ctx, t1);
        struct vkd3d_string_buffer *t2_string = hlsl_type_to_string(ctx, t2);

        if (t1_string && t2_string)
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Expression data types \"%s\" and \"%s\" are incompatible.",
                    t1_string->buffer, t2_string->buffer);
        hlsl_release_string_buffer(ctx, t1_string);
        hlsl_release_string_buffer(ctx, t2_string);
        return false;
    }

    if (t1->dimx == 1 && t1->dimy == 1)
    {
        *type = t2->class;
        *dimx = t2->dimx;
        *dimy = t2->dimy;
    }
    else if (t2->dimx == 1 && t2->dimy == 1)
    {
        *type = t1->class;
        *dimx = t1->dimx;
        *dimy = t1->dimy;
    }
    else if (t1->class == HLSL_CLASS_MATRIX && t2->class == HLSL_CLASS_MATRIX)
    {
        *type = HLSL_CLASS_MATRIX;
        *dimx = min(t1->dimx, t2->dimx);
        *dimy = min(t1->dimy, t2->dimy);
    }
    else
    {
        if (t1->dimx * t1->dimy <= t2->dimx * t2->dimy)
        {
            *type = t1->class;
            *dimx = t1->dimx;
            *dimy = t1->dimy;
        }
        else
        {
            *type = t2->class;
            *dimx = t2->dimx;
            *dimy = t2->dimy;
        }
    }

    return true;
}

static struct hlsl_ir_node *add_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS],
        struct hlsl_type *type, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *expr;
    unsigned int i;

    if (type->class == HLSL_CLASS_MATRIX)
    {
        struct hlsl_type *scalar_type;
        struct hlsl_ir_load *var_load;
        struct hlsl_deref var_deref;
        struct hlsl_ir_node *load;
        struct hlsl_ir_var *var;

        scalar_type = hlsl_get_scalar_type(ctx, type->base_type);

        if (!(var = hlsl_new_synthetic_var(ctx, "split_op", type, loc)))
            return NULL;
        hlsl_init_simple_deref_from_var(&var_deref, var);

        for (i = 0; i < type->dimy * type->dimx; ++i)
        {
            struct hlsl_ir_node *value, *cell_operands[HLSL_MAX_OPERANDS] = { NULL };
            struct hlsl_block store_block;
            unsigned int j;

            for (j = 0; j < HLSL_MAX_OPERANDS; j++)
            {
                if (operands[j])
                {
                    if (!(load = hlsl_add_load_component(ctx, block, operands[j], i, loc)))
                        return NULL;

                    cell_operands[j] = load;
                }
            }

            if (!(value = add_expr(ctx, block, op, cell_operands, scalar_type, loc)))
                return NULL;

            if (!hlsl_new_store_component(ctx, &store_block, &var_deref, i, value))
                return NULL;
            hlsl_block_add_block(block, &store_block);
        }

        if (!(var_load = hlsl_new_var_load(ctx, var, loc)))
            return NULL;
        hlsl_block_add_instr(block, &var_load->node);

        return &var_load->node;
    }

    if (!(expr = hlsl_new_expr(ctx, op, operands, type, loc)))
        return NULL;
    hlsl_block_add_instr(block, expr);

    return expr;
}

static void check_integer_type(struct hlsl_ctx *ctx, const struct hlsl_ir_node *instr)
{
    const struct hlsl_type *type = instr->data_type;
    struct vkd3d_string_buffer *string;

    switch (type->base_type)
    {
        case HLSL_TYPE_BOOL:
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
            break;

        default:
            if ((string = hlsl_type_to_string(ctx, type)))
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Expression type '%s' is not integer.", string->buffer);
            hlsl_release_string_buffer(ctx, string);
            break;
    }
}

static struct hlsl_ir_node *add_unary_arithmetic_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {arg};

    return add_expr(ctx, block, op, args, arg->data_type, loc);
}

static struct hlsl_ir_node *add_unary_bitwise_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    check_integer_type(ctx, arg);

    return add_unary_arithmetic_expr(ctx, block, op, arg, loc);
}

static struct hlsl_ir_node *add_unary_logical_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *bool_type;

    bool_type = hlsl_get_numeric_type(ctx, arg->data_type->class, HLSL_TYPE_BOOL,
            arg->data_type->dimx, arg->data_type->dimy);

    if (!(args[0] = add_implicit_conversion(ctx, block, arg, bool_type, loc)))
        return NULL;

    return add_expr(ctx, block, op, args, bool_type, loc);
}

static struct hlsl_type *get_common_numeric_type(struct hlsl_ctx *ctx, const struct hlsl_ir_node *arg1,
        const struct hlsl_ir_node *arg2, const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type base = expr_common_base_type(arg1->data_type->base_type, arg2->data_type->base_type);
    enum hlsl_type_class type;
    unsigned int dimx, dimy;

    if (!expr_common_shape(ctx, arg1->data_type, arg2->data_type, loc, &type, &dimx, &dimy))
        return NULL;

    return hlsl_get_numeric_type(ctx, type, base, dimx, dimy);
}

static struct hlsl_ir_node *add_binary_arithmetic_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *common_type;

    common_type = get_common_numeric_type(ctx, arg1, arg2, loc);

    if (!(args[0] = add_implicit_conversion(ctx, block, arg1, common_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, block, arg2, common_type, loc)))
        return NULL;

    return add_expr(ctx, block, op, args, common_type, loc);
}

static struct hlsl_ir_node *add_binary_bitwise_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc)
{
    check_integer_type(ctx, arg1);
    check_integer_type(ctx, arg2);

    return add_binary_arithmetic_expr(ctx, block, op, arg1, arg2, loc);
}

static struct hlsl_ir_node *add_binary_comparison_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *common_type, *return_type;
    enum hlsl_base_type base = expr_common_base_type(arg1->data_type->base_type, arg2->data_type->base_type);
    enum hlsl_type_class type;
    unsigned int dimx, dimy;
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};

    if (!expr_common_shape(ctx, arg1->data_type, arg2->data_type, loc, &type, &dimx, &dimy))
        return NULL;

    common_type = hlsl_get_numeric_type(ctx, type, base, dimx, dimy);
    return_type = hlsl_get_numeric_type(ctx, type, HLSL_TYPE_BOOL, dimx, dimy);

    if (!(args[0] = add_implicit_conversion(ctx, block, arg1, common_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, block, arg2, common_type, loc)))
        return NULL;

    return add_expr(ctx, block, op, args, return_type, loc);
}

static struct hlsl_ir_node *add_binary_logical_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *common_type;
    enum hlsl_type_class type;
    unsigned int dimx, dimy;

    if (!expr_common_shape(ctx, arg1->data_type, arg2->data_type, loc, &type, &dimx, &dimy))
        return NULL;

    common_type = hlsl_get_numeric_type(ctx, type, HLSL_TYPE_BOOL, dimx, dimy);

    if (!(args[0] = add_implicit_conversion(ctx, block, arg1, common_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, block, arg2, common_type, loc)))
        return NULL;

    return add_expr(ctx, block, op, args, common_type, loc);
}

static struct hlsl_ir_node *add_binary_shift_expr(struct hlsl_ctx *ctx, struct hlsl_block *block,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type base = arg1->data_type->base_type;
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *return_type, *integer_type;
    enum hlsl_type_class type;
    unsigned int dimx, dimy;

    check_integer_type(ctx, arg1);
    check_integer_type(ctx, arg2);

    if (base == HLSL_TYPE_BOOL)
        base = HLSL_TYPE_INT;

    if (!expr_common_shape(ctx, arg1->data_type, arg2->data_type, loc, &type, &dimx, &dimy))
        return NULL;

    return_type = hlsl_get_numeric_type(ctx, type, base, dimx, dimy);
    integer_type = hlsl_get_numeric_type(ctx, type, HLSL_TYPE_INT, dimx, dimy);

    if (!(args[0] = add_implicit_conversion(ctx, block, arg1, return_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, block, arg2, integer_type, loc)))
        return NULL;

    return add_expr(ctx, block, op, args, return_type, loc);
}

static struct hlsl_ir_node *add_binary_dot_expr(struct hlsl_ctx *ctx, struct hlsl_block *instrs,
        struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2, const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type base = expr_common_base_type(arg1->data_type->base_type, arg2->data_type->base_type);
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *common_type, *ret_type;
    enum hlsl_ir_expr_op op;
    unsigned dim;

    if (arg1->data_type->class == HLSL_CLASS_MATRIX)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, arg1->data_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Invalid type %s.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return NULL;
    }

    if (arg2->data_type->class == HLSL_CLASS_MATRIX)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, arg2->data_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Invalid type %s.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return NULL;
    }

    if (arg1->data_type->class == HLSL_CLASS_SCALAR)
        dim = arg2->data_type->dimx;
    else if (arg2->data_type->class == HLSL_CLASS_SCALAR)
        dim = arg1->data_type->dimx;
    else
        dim = min(arg1->data_type->dimx, arg2->data_type->dimx);

    if (dim == 1)
        op = HLSL_OP2_MUL;
    else
        op = HLSL_OP2_DOT;

    common_type = hlsl_get_vector_type(ctx, base, dim);
    ret_type = hlsl_get_scalar_type(ctx, base);

    if (!(args[0] = add_implicit_conversion(ctx, instrs, arg1, common_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, instrs, arg2, common_type, loc)))
        return NULL;

    return add_expr(ctx, instrs, op, args, ret_type, loc);
}

static struct hlsl_block *add_binary_expr_merge(struct hlsl_ctx *ctx, struct hlsl_block *block1,
        struct hlsl_block *block2, enum hlsl_ir_expr_op op, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg1 = node_from_block(block1), *arg2 = node_from_block(block2);

    hlsl_block_add_block(block1, block2);
    destroy_block(block2);

    switch (op)
    {
        case HLSL_OP2_ADD:
        case HLSL_OP2_DIV:
        case HLSL_OP2_MOD:
        case HLSL_OP2_MUL:
            add_binary_arithmetic_expr(ctx, block1, op, arg1, arg2, loc);
            break;

        case HLSL_OP2_BIT_AND:
        case HLSL_OP2_BIT_OR:
        case HLSL_OP2_BIT_XOR:
            add_binary_bitwise_expr(ctx, block1, op, arg1, arg2, loc);
            break;

        case HLSL_OP2_LESS:
        case HLSL_OP2_GEQUAL:
        case HLSL_OP2_EQUAL:
        case HLSL_OP2_NEQUAL:
            add_binary_comparison_expr(ctx, block1, op, arg1, arg2, loc);
            break;

        case HLSL_OP2_LOGIC_AND:
        case HLSL_OP2_LOGIC_OR:
            add_binary_logical_expr(ctx, block1, op, arg1, arg2, loc);
            break;

        case HLSL_OP2_LSHIFT:
        case HLSL_OP2_RSHIFT:
            add_binary_shift_expr(ctx, block1, op, arg1, arg2, loc);
            break;

        default:
            vkd3d_unreachable();
    }

    return block1;
}

static enum hlsl_ir_expr_op op_from_assignment(enum parse_assign_op op)
{
    static const enum hlsl_ir_expr_op ops[] =
    {
        0,
        HLSL_OP2_ADD,
        0,
        HLSL_OP2_MUL,
        HLSL_OP2_DIV,
        HLSL_OP2_MOD,
        HLSL_OP2_LSHIFT,
        HLSL_OP2_RSHIFT,
        HLSL_OP2_BIT_AND,
        HLSL_OP2_BIT_OR,
        HLSL_OP2_BIT_XOR,
    };

    return ops[op];
}

static bool invert_swizzle(unsigned int *swizzle, unsigned int *writemask, unsigned int *ret_width)
{
    unsigned int i, j, bit = 0, inverted = 0, width, new_writemask = 0, new_swizzle = 0;

    /* Apply the writemask to the swizzle to get a new writemask and swizzle. */
    for (i = 0; i < 4; ++i)
    {
        if (*writemask & (1 << i))
        {
            unsigned int s = (*swizzle >> (i * 2)) & 3;
            new_swizzle |= s << (bit++ * 2);
            if (new_writemask & (1 << s))
                return false;
            new_writemask |= 1 << s;
        }
    }
    width = bit;

    /* Invert the swizzle. */
    bit = 0;
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < width; ++j)
        {
            unsigned int s = (new_swizzle >> (j * 2)) & 3;
            if (s == i)
                inverted |= j << (bit++ * 2);
        }
    }

    *swizzle = inverted;
    *writemask = new_writemask;
    *ret_width = width;
    return true;
}

static struct hlsl_ir_node *add_assignment(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *lhs,
        enum parse_assign_op assign_op, struct hlsl_ir_node *rhs)
{
    struct hlsl_type *lhs_type = lhs->data_type;
    struct hlsl_ir_node *copy;
    unsigned int writemask = 0;

    if (assign_op == ASSIGN_OP_SUB)
    {
        if (!(rhs = add_unary_arithmetic_expr(ctx, block, HLSL_OP1_NEG, rhs, &rhs->loc)))
            return NULL;
        assign_op = ASSIGN_OP_ADD;
    }
    if (assign_op != ASSIGN_OP_ASSIGN)
    {
        enum hlsl_ir_expr_op op = op_from_assignment(assign_op);

        assert(op);
        if (!(rhs = add_binary_arithmetic_expr(ctx, block, op, lhs, rhs, &rhs->loc)))
            return NULL;
    }

    if (hlsl_is_numeric_type(lhs_type))
        writemask = (1 << lhs_type->dimx) - 1;

    if (!(rhs = add_implicit_conversion(ctx, block, rhs, lhs_type, &rhs->loc)))
        return NULL;

    while (lhs->type != HLSL_IR_LOAD && lhs->type != HLSL_IR_INDEX)
    {
        if (lhs->type == HLSL_IR_EXPR && hlsl_ir_expr(lhs)->op == HLSL_OP1_CAST)
        {
            hlsl_fixme(ctx, &lhs->loc, "Cast on the LHS.");
            return NULL;
        }
        else if (lhs->type == HLSL_IR_SWIZZLE)
        {
            struct hlsl_ir_swizzle *swizzle = hlsl_ir_swizzle(lhs);
            unsigned int width, s = swizzle->swizzle;
            struct hlsl_ir_node *new_swizzle;

            if (lhs->data_type->class == HLSL_CLASS_MATRIX)
                hlsl_fixme(ctx, &lhs->loc, "Matrix assignment with a writemask.");

            if (!invert_swizzle(&s, &writemask, &width))
            {
                hlsl_error(ctx, &lhs->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_WRITEMASK, "Invalid writemask.");
                return NULL;
            }

            if (!(new_swizzle = hlsl_new_swizzle(ctx, s, width, rhs, &swizzle->node.loc)))
            {
                return NULL;
            }
            hlsl_block_add_instr(block, new_swizzle);

            lhs = swizzle->val.node;
            rhs = new_swizzle;
        }
        else
        {
            hlsl_error(ctx, &lhs->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_LVALUE, "Invalid lvalue.");
            return NULL;
        }
    }

    if (lhs->type == HLSL_IR_INDEX && hlsl_index_is_resource_access(hlsl_ir_index(lhs)))
    {
        struct hlsl_ir_node *coords = hlsl_ir_index(lhs)->idx.node;
        struct hlsl_deref resource_deref;
        struct hlsl_type *resource_type;
        struct hlsl_ir_node *store;
        unsigned int dim_count;

        if (!hlsl_init_deref_from_index_chain(ctx, &resource_deref, hlsl_ir_index(lhs)->val.node))
            return NULL;

        resource_type = hlsl_deref_get_type(ctx, &resource_deref);
        assert(resource_type->class == HLSL_CLASS_OBJECT);
        assert(resource_type->base_type == HLSL_TYPE_TEXTURE || resource_type->base_type == HLSL_TYPE_UAV);

        if (resource_type->base_type != HLSL_TYPE_UAV)
            hlsl_error(ctx, &lhs->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Read-only resources cannot be stored to.");

        dim_count = hlsl_sampler_dim_count(resource_type->sampler_dim);

        if (writemask != ((1u << resource_type->e.resource_format->dimx) - 1))
            hlsl_error(ctx, &lhs->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_WRITEMASK,
                    "Resource store expressions must write to all components.");

        assert(coords->data_type->class == HLSL_CLASS_VECTOR);
        assert(coords->data_type->base_type == HLSL_TYPE_UINT);
        assert(coords->data_type->dimx == dim_count);

        if (!(store = hlsl_new_resource_store(ctx, &resource_deref, coords, rhs, &lhs->loc)))
        {
            hlsl_cleanup_deref(&resource_deref);
            return NULL;
        }
        hlsl_block_add_instr(block, store);
        hlsl_cleanup_deref(&resource_deref);
    }
    else if (lhs->type == HLSL_IR_INDEX && hlsl_index_is_noncontiguous(hlsl_ir_index(lhs)))
    {
        struct hlsl_ir_index *row = hlsl_ir_index(lhs);
        struct hlsl_ir_node *mat = row->val.node;
        unsigned int i, k = 0;

        for (i = 0; i < mat->data_type->dimx; ++i)
        {
            struct hlsl_ir_node *cell, *load, *store, *c;
            struct hlsl_deref deref;

            if (!(writemask & (1 << i)))
                continue;

            if (!(c = hlsl_new_uint_constant(ctx, i, &lhs->loc)))
                return NULL;
            hlsl_block_add_instr(block, c);

            if (!(cell = hlsl_new_index(ctx, &row->node, c, &lhs->loc)))
                return NULL;
            hlsl_block_add_instr(block, cell);

            if (!(load = hlsl_add_load_component(ctx, block, rhs, k++, &rhs->loc)))
                return NULL;

            if (!hlsl_init_deref_from_index_chain(ctx, &deref, cell))
                return NULL;

            if (!(store = hlsl_new_store_index(ctx, &deref, NULL, load, 0, &rhs->loc)))
            {
                hlsl_cleanup_deref(&deref);
                return NULL;
            }
            hlsl_block_add_instr(block, store);
            hlsl_cleanup_deref(&deref);
        }
    }
    else
    {
        struct hlsl_ir_node *store;
        struct hlsl_deref deref;

        if (!hlsl_init_deref_from_index_chain(ctx, &deref, lhs))
            return NULL;

        if (!(store = hlsl_new_store_index(ctx, &deref, NULL, rhs, writemask, &rhs->loc)))
        {
            hlsl_cleanup_deref(&deref);
            return NULL;
        }
        hlsl_block_add_instr(block, store);
        hlsl_cleanup_deref(&deref);
    }

    /* Don't use the instruction itself as a source, as this makes structure
     * splitting easier. Instead copy it here. Since we retrieve sources from
     * the last instruction in the list, we do need to copy. */
    if (!(copy = hlsl_new_copy(ctx, rhs)))
        return NULL;
    hlsl_block_add_instr(block, copy);
    return copy;
}

static bool add_increment(struct hlsl_ctx *ctx, struct hlsl_block *block, bool decrement, bool post,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *lhs = node_from_block(block);
    struct hlsl_ir_node *one;

    if (lhs->data_type->modifiers & HLSL_MODIFIER_CONST)
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_MODIFIES_CONST,
                "Argument to %s%screment operator is const.", post ? "post" : "pre", decrement ? "de" : "in");

    if (!(one = hlsl_new_int_constant(ctx, 1, loc)))
        return false;
    hlsl_block_add_instr(block, one);

    if (!add_assignment(ctx, block, lhs, decrement ? ASSIGN_OP_SUB : ASSIGN_OP_ADD, one))
        return false;

    if (post)
    {
        struct hlsl_ir_node *copy;

        if (!(copy = hlsl_new_copy(ctx, lhs)))
            return false;
        hlsl_block_add_instr(block, copy);

        /* Post increment/decrement expressions are considered const. */
        if (!(copy->data_type = hlsl_type_clone(ctx, copy->data_type, 0, HLSL_MODIFIER_CONST)))
            return false;
    }

    return true;
}

static void initialize_var_components(struct hlsl_ctx *ctx, struct hlsl_block *instrs,
        struct hlsl_ir_var *dst, unsigned int *store_index, struct hlsl_ir_node *src)
{
    unsigned int src_comp_count = hlsl_type_component_count(src->data_type);
    struct hlsl_deref dst_deref;
    unsigned int k;

    hlsl_init_simple_deref_from_var(&dst_deref, dst);

    for (k = 0; k < src_comp_count; ++k)
    {
        struct hlsl_ir_node *conv, *load;
        struct hlsl_type *dst_comp_type;
        struct hlsl_block block;

        if (!(load = hlsl_add_load_component(ctx, instrs, src, k, &src->loc)))
            return;

        dst_comp_type = hlsl_type_get_component_type(ctx, dst->data_type, *store_index);

        if (!(conv = add_implicit_conversion(ctx, instrs, load, dst_comp_type, &src->loc)))
            return;

        if (!hlsl_new_store_component(ctx, &block, &dst_deref, *store_index, conv))
            return;
        hlsl_block_add_block(instrs, &block);

        ++*store_index;
    }
}

static bool type_has_object_components(struct hlsl_type *type, bool must_be_in_struct)
{
    if (type->class == HLSL_CLASS_OBJECT)
        return !must_be_in_struct;
    if (type->class == HLSL_CLASS_ARRAY)
        return type_has_object_components(type->e.array.type, must_be_in_struct);

    if (type->class == HLSL_CLASS_STRUCT)
    {
        unsigned int i;

        for (i = 0; i < type->e.record.field_count; ++i)
        {
            if (type_has_object_components(type->e.record.fields[i].type, false))
                return true;
        }
    }
    return false;
}

static bool type_has_numeric_components(struct hlsl_type *type)
{
    if (hlsl_is_numeric_type(type))
        return true;
    if (type->class == HLSL_CLASS_ARRAY)
        return type_has_numeric_components(type->e.array.type);

    if (type->class == HLSL_CLASS_STRUCT)
    {
        unsigned int i;

        for (i = 0; i < type->e.record.field_count; ++i)
        {
            if (type_has_numeric_components(type->e.record.fields[i].type))
                return true;
        }
    }
    return false;
}

static void check_invalid_in_out_modifiers(struct hlsl_ctx *ctx, unsigned int modifiers,
        const struct vkd3d_shader_location *loc)
{
    modifiers &= (HLSL_STORAGE_IN | HLSL_STORAGE_OUT);
    if (modifiers)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_modifiers_to_string(ctx, modifiers)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                    "Modifiers '%s' are not allowed on non-parameter variables.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
}

static void declare_var(struct hlsl_ctx *ctx, struct parse_variable_def *v)
{
    struct hlsl_type *basic_type = v->basic_type;
    struct hlsl_ir_function_decl *func;
    struct hlsl_semantic new_semantic;
    uint32_t modifiers = v->modifiers;
    bool unbounded_res_array = false;
    struct hlsl_ir_var *var;
    struct hlsl_type *type;
    bool local = true;
    char *var_name;
    unsigned int i;

    assert(basic_type);

    if (basic_type->class == HLSL_CLASS_MATRIX)
        assert(basic_type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);

    type = basic_type;

    if (shader_is_sm_5_1(ctx) && type->class == HLSL_CLASS_OBJECT)
    {
        for (i = 0; i < v->arrays.count; ++i)
            unbounded_res_array |= (v->arrays.sizes[i] == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT);
    }

    if (unbounded_res_array)
    {
        if (v->arrays.count == 1)
        {
            hlsl_fixme(ctx, &v->loc, "Unbounded resource arrays.");
            return;
        }
        else
        {
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Unbounded resource arrays cannot be multi-dimensional.");
        }
    }
    else
    {
        for (i = 0; i < v->arrays.count; ++i)
        {
            if (v->arrays.sizes[i] == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT)
            {
                unsigned int size = initializer_size(&v->initializer);
                unsigned int elem_components = hlsl_type_component_count(type);

                if (i < v->arrays.count - 1)
                {
                    hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Only innermost array size can be implicit.");
                    v->initializer.args_count = 0;
                }
                else if (elem_components == 0)
                {
                    hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Cannot declare an implicit size array of a size 0 type.");
                    v->initializer.args_count = 0;
                }
                else if (size == 0)
                {
                    hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Implicit size arrays need to be initialized.");
                    v->initializer.args_count = 0;
                }
                else if (size % elem_components != 0)
                {
                    hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                            "Cannot initialize implicit size array with %u components, expected a multiple of %u.",
                            size, elem_components);
                    v->initializer.args_count = 0;
                }
                else
                {
                    v->arrays.sizes[i] = size / elem_components;
                }
            }
            type = hlsl_new_array_type(ctx, type, v->arrays.sizes[i]);
        }
    }

    if (!(var_name = vkd3d_strdup(v->name)))
        return;

    new_semantic = v->semantic;
    if (v->semantic.name)
    {
        if (!(new_semantic.name = vkd3d_strdup(v->semantic.name)))
        {
            vkd3d_free(var_name);
            return;
        }
    }

    if (!(var = hlsl_new_var(ctx, var_name, type, &v->loc, &new_semantic, modifiers, &v->reg_reservation)))
    {
        hlsl_cleanup_semantic(&new_semantic);
        vkd3d_free(var_name);
        return;
    }

    var->buffer = ctx->cur_buffer;

    if (var->buffer == ctx->globals_buffer)
    {
        if (var->reg_reservation.offset_type)
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                    "packoffset() is only allowed inside constant buffer declarations.");
    }

    if (ctx->cur_scope == ctx->globals)
    {
        local = false;

        if ((modifiers & HLSL_STORAGE_UNIFORM) && (modifiers & HLSL_STORAGE_STATIC))
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                    "Variable '%s' is declared as both \"uniform\" and \"static\".", var->name);

        /* Mark it as uniform. We need to do this here since synthetic
            * variables also get put in the global scope, but shouldn't be
            * considered uniforms, and we have no way of telling otherwise. */
        if (!(modifiers & HLSL_STORAGE_STATIC))
            var->storage_modifiers |= HLSL_STORAGE_UNIFORM;

        if (ctx->profile->major_version < 5 && (var->storage_modifiers & HLSL_STORAGE_UNIFORM) &&
                type_has_object_components(var->data_type, true))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Target profile doesn't support objects as struct members in uniform variables.");
        }

        if ((func = hlsl_get_first_func_decl(ctx, var->name)))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                    "'%s' is already defined as a function.", var->name);
            hlsl_note(ctx, &func->loc, VKD3D_SHADER_LOG_ERROR,
                    "'%s' was previously defined here.", var->name);
        }
    }
    else
    {
        static const unsigned int invalid = HLSL_STORAGE_EXTERN | HLSL_STORAGE_SHARED
                | HLSL_STORAGE_GROUPSHARED | HLSL_STORAGE_UNIFORM;

        if (modifiers & invalid)
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_modifiers_to_string(ctx, modifiers & invalid)))
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Modifiers '%s' are not allowed on local variables.", string->buffer);
            hlsl_release_string_buffer(ctx, string);
        }
        if (var->semantic.name)
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "Semantics are not allowed on local variables.");

        if ((type->modifiers & HLSL_MODIFIER_CONST) && !v->initializer.args_count && !(modifiers & HLSL_STORAGE_STATIC))
        {
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_INITIALIZER,
                "Const variable \"%s\" is missing an initializer.", var->name);
        }
    }

    if ((var->storage_modifiers & HLSL_STORAGE_STATIC) && type_has_numeric_components(var->data_type)
            && type_has_object_components(var->data_type, false))
    {
        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                "Static variables cannot have both numeric and resource components.");
    }

    if (!hlsl_add_var(ctx, var, local))
    {
        struct hlsl_ir_var *old = hlsl_get_var(ctx->cur_scope, var->name);

        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                "Variable \"%s\" was already declared in this scope.", var->name);
        hlsl_note(ctx, &old->loc, VKD3D_SHADER_LOG_ERROR, "\"%s\" was previously declared here.", old->name);
        hlsl_free_var(var);
        return;
    }
}

static struct hlsl_block *initialize_vars(struct hlsl_ctx *ctx, struct list *var_list)
{
    struct parse_variable_def *v, *v_next;
    struct hlsl_block *initializers;
    struct hlsl_ir_var *var;
    struct hlsl_type *type;

    if (!(initializers = make_empty_block(ctx)))
    {
        LIST_FOR_EACH_ENTRY_SAFE(v, v_next, var_list, struct parse_variable_def, entry)
        {
            free_parse_variable_def(v);
        }
        vkd3d_free(var_list);
        return NULL;
    }

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, var_list, struct parse_variable_def, entry)
    {
        /* If this fails, the variable failed to be declared. */
        if (!(var = hlsl_get_var(ctx->cur_scope, v->name)))
        {
            free_parse_variable_def(v);
            continue;
        }
        type = var->data_type;

        if (v->initializer.args_count)
        {
            if (v->initializer.braces)
            {
                unsigned int size = initializer_size(&v->initializer);
                unsigned int store_index = 0;
                unsigned int k;

                if (hlsl_type_component_count(type) != size)
                {
                    hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                            "Expected %u components in initializer, but got %u.",
                            hlsl_type_component_count(type), size);
                    free_parse_variable_def(v);
                    continue;
                }

                for (k = 0; k < v->initializer.args_count; ++k)
                {
                    initialize_var_components(ctx, v->initializer.instrs, var,
                            &store_index, v->initializer.args[k]);
                }
            }
            else
            {
                struct hlsl_ir_load *load = hlsl_new_var_load(ctx, var, &var->loc);

                assert(v->initializer.args_count == 1);
                hlsl_block_add_instr(v->initializer.instrs, &load->node);
                add_assignment(ctx, v->initializer.instrs, &load->node, ASSIGN_OP_ASSIGN, v->initializer.args[0]);
            }

            if (var->storage_modifiers & HLSL_STORAGE_STATIC)
                hlsl_block_add_block(&ctx->static_initializers, v->initializer.instrs);
            else
                hlsl_block_add_block(initializers, v->initializer.instrs);
        }
        else if (var->storage_modifiers & HLSL_STORAGE_STATIC)
        {
            struct hlsl_ir_node *cast, *store, *zero;

            /* Initialize statics to zero by default. */

            if (type_has_object_components(var->data_type, false))
            {
                free_parse_variable_def(v);
                continue;
            }

            if (!(zero = hlsl_new_uint_constant(ctx, 0, &var->loc)))
            {
                free_parse_variable_def(v);
                continue;
            }
            hlsl_block_add_instr(&ctx->static_initializers, zero);

            if (!(cast = add_cast(ctx, &ctx->static_initializers, zero, var->data_type, &var->loc)))
            {
                free_parse_variable_def(v);
                continue;
            }

            if (!(store = hlsl_new_simple_store(ctx, var, cast)))
            {
                free_parse_variable_def(v);
                continue;
            }
            hlsl_block_add_instr(&ctx->static_initializers, store);
        }
        free_parse_variable_def(v);
    }

    vkd3d_free(var_list);
    return initializers;
}

static bool func_is_compatible_match(struct hlsl_ctx *ctx,
        const struct hlsl_ir_function_decl *decl, const struct parse_initializer *args)
{
    unsigned int i;

    if (decl->parameters.count != args->args_count)
        return false;

    for (i = 0; i < decl->parameters.count; ++i)
    {
        if (!implicit_compatible_data_types(ctx, args->args[i]->data_type, decl->parameters.vars[i]->data_type))
            return false;
    }
    return true;
}

static struct hlsl_ir_function_decl *find_function_call(struct hlsl_ctx *ctx,
        const char *name, const struct parse_initializer *args,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_function_decl *decl, *compatible_match = NULL;
    struct hlsl_ir_function *func;
    struct rb_entry *entry;

    if (!(entry = rb_get(&ctx->functions, name)))
        return NULL;
    func = RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);

    LIST_FOR_EACH_ENTRY(decl, &func->overloads, struct hlsl_ir_function_decl, entry)
    {
        if (func_is_compatible_match(ctx, decl, args))
        {
            if (compatible_match)
            {
                hlsl_fixme(ctx, loc, "Prioritize between multiple compatible function overloads.");
                break;
            }
            compatible_match = decl;
        }
    }

    return compatible_match;
}

static struct hlsl_ir_node *hlsl_new_void_expr(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {0};

    return hlsl_new_expr(ctx, HLSL_OP0_VOID, operands, ctx->builtin_types.Void, loc);
}

static bool add_user_call(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        const struct parse_initializer *args, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *call;
    unsigned int i;

    assert(args->args_count == func->parameters.count);

    for (i = 0; i < func->parameters.count; ++i)
    {
        struct hlsl_ir_var *param = func->parameters.vars[i];
        struct hlsl_ir_node *arg = args->args[i];

        if (!hlsl_types_are_equal(arg->data_type, param->data_type))
        {
            struct hlsl_ir_node *cast;

            if (!(cast = add_cast(ctx, args->instrs, arg, param->data_type, &arg->loc)))
                return false;
            args->args[i] = cast;
            arg = cast;
        }

        if (param->storage_modifiers & HLSL_STORAGE_IN)
        {
            struct hlsl_ir_node *store;

            if (!(store = hlsl_new_simple_store(ctx, param, arg)))
                return false;
            hlsl_block_add_instr(args->instrs, store);
        }
    }

    if (!(call = hlsl_new_call(ctx, func, loc)))
        return false;
    hlsl_block_add_instr(args->instrs, call);

    for (i = 0; i < func->parameters.count; ++i)
    {
        struct hlsl_ir_var *param = func->parameters.vars[i];
        struct hlsl_ir_node *arg = args->args[i];

        if (param->storage_modifiers & HLSL_STORAGE_OUT)
        {
            struct hlsl_ir_load *load;

            if (arg->data_type->modifiers & HLSL_MODIFIER_CONST)
                hlsl_error(ctx, &arg->loc, VKD3D_SHADER_ERROR_HLSL_MODIFIES_CONST,
                        "Output argument to \"%s\" is const.", func->func->name);

            if (!(load = hlsl_new_var_load(ctx, param, &arg->loc)))
                return false;
            hlsl_block_add_instr(args->instrs, &load->node);

            if (!add_assignment(ctx, args->instrs, arg, ASSIGN_OP_ASSIGN, &load->node))
                return false;
        }
    }

    if (func->return_var)
    {
        struct hlsl_ir_load *load;

        if (!(load = hlsl_new_var_load(ctx, func->return_var, loc)))
            return false;
        hlsl_block_add_instr(args->instrs, &load->node);
    }
    else
    {
        struct hlsl_ir_node *expr;

        if (!(expr = hlsl_new_void_expr(ctx, loc)))
            return false;
        hlsl_block_add_instr(args->instrs, expr);
    }

    return true;
}

static struct hlsl_ir_node *intrinsic_float_convert_arg(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *type = arg->data_type;

    if (type->base_type == HLSL_TYPE_FLOAT || type->base_type == HLSL_TYPE_HALF)
        return arg;

    type = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_FLOAT, type->dimx, type->dimy);
    return add_implicit_conversion(ctx, params->instrs, arg, type, loc);
}

static bool convert_args(struct hlsl_ctx *ctx, const struct parse_initializer *params,
        struct hlsl_type *type, const struct vkd3d_shader_location *loc)
{
    unsigned int i;

    for (i = 0; i < params->args_count; ++i)
    {
        struct hlsl_ir_node *new_arg;

        if (!(new_arg = add_implicit_conversion(ctx, params->instrs, params->args[i], type, loc)))
            return false;
        params->args[i] = new_arg;
    }

    return true;
}

static struct hlsl_type *elementwise_intrinsic_get_common_type(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type base = params->args[0]->data_type->base_type;
    bool vectors = false, matrices = false;
    unsigned int dimx = 4, dimy = 4;
    struct hlsl_type *common_type;
    unsigned int i;

    for (i = 0; i < params->args_count; ++i)
    {
        struct hlsl_type *arg_type = params->args[i]->data_type;

        base = expr_common_base_type(base, arg_type->base_type);

        if (arg_type->class == HLSL_CLASS_VECTOR)
        {
            vectors = true;
            dimx = min(dimx, arg_type->dimx);
        }
        else if (arg_type->class == HLSL_CLASS_MATRIX)
        {
            matrices = true;
            dimx = min(dimx, arg_type->dimx);
            dimy = min(dimy, arg_type->dimy);
        }
    }

    if (matrices && vectors)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                "Cannot use both matrices and vectors in an elementwise intrinsic.");
        return NULL;
    }
    else if (matrices)
    {
        common_type = hlsl_get_matrix_type(ctx, base, dimx, dimy);
    }
    else if (vectors)
    {
        common_type = hlsl_get_vector_type(ctx, base, dimx);
    }
    else
    {
        common_type = hlsl_get_scalar_type(ctx, base);
    }

    return common_type;
}

static bool elementwise_intrinsic_convert_args(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *common_type;

    if (!(common_type = elementwise_intrinsic_get_common_type(ctx, params, loc)))
        return false;

    return convert_args(ctx, params, common_type, loc);
}

static bool elementwise_intrinsic_float_convert_args(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *type;

    if (!(type = elementwise_intrinsic_get_common_type(ctx, params, loc)))
        return false;

    type = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_FLOAT, type->dimx, type->dimy);

    return convert_args(ctx, params, type, loc);
}

static bool intrinsic_abs(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_ABS, params->args[0], loc);
}

static bool intrinsic_all(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg = params->args[0], *mul, *one, *zero, *load;
    unsigned int i, count;

    if (!(one = hlsl_new_float_constant(ctx, 1.0f, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, one);

    if (!(zero = hlsl_new_float_constant(ctx, 0.0f, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, zero);

    mul = one;

    count = hlsl_type_component_count(arg->data_type);
    for (i = 0; i < count; ++i)
    {
        if (!(load = hlsl_add_load_component(ctx, params->instrs, arg, i, loc)))
            return false;

        if (!(mul = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, load, mul, loc)))
            return false;
    }

    return !!add_binary_comparison_expr(ctx, params->instrs, HLSL_OP2_NEQUAL, mul, zero, loc);
}

static bool intrinsic_any(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg = params->args[0], *dot, *or, *zero, *bfalse, *load;
    unsigned int i, count;

    if (arg->data_type->class != HLSL_CLASS_VECTOR && arg->data_type->class != HLSL_CLASS_SCALAR)
    {
        hlsl_fixme(ctx, loc, "any() implementation for non-vector, non-scalar");
        return false;
    }

    if (arg->data_type->base_type == HLSL_TYPE_FLOAT)
    {
        if (!(zero = hlsl_new_float_constant(ctx, 0.0f, loc)))
            return false;
        hlsl_block_add_instr(params->instrs, zero);

        if (!(dot = add_binary_dot_expr(ctx, params->instrs, arg, arg, loc)))
            return false;

        return !!add_binary_comparison_expr(ctx, params->instrs, HLSL_OP2_NEQUAL, dot, zero, loc);
    }
    else if (arg->data_type->base_type == HLSL_TYPE_BOOL)
    {
        if (!(bfalse = hlsl_new_bool_constant(ctx, false, loc)))
            return false;
        hlsl_block_add_instr(params->instrs, bfalse);

        or = bfalse;

        count = hlsl_type_component_count(arg->data_type);
        for (i = 0; i < count; ++i)
        {
            if (!(load = hlsl_add_load_component(ctx, params->instrs, arg, i, loc)))
                return false;

            if (!(or = add_binary_bitwise_expr(ctx, params->instrs, HLSL_OP2_BIT_OR, or, load, loc)))
                return false;
        }

        return true;
    }

    hlsl_fixme(ctx, loc, "any() implementation for non-float, non-bool");
    return false;
}

/* Find the type corresponding to the given source type, with the same
 * dimensions but a different base type. */
static struct hlsl_type *convert_numeric_type(const struct hlsl_ctx *ctx,
        const struct hlsl_type *type, enum hlsl_base_type base_type)
{
    return hlsl_get_numeric_type(ctx, type->class, base_type, type->dimx, type->dimy);
}

static bool intrinsic_asfloat(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *data_type;

    data_type = params->args[0]->data_type;
    if (data_type->base_type == HLSL_TYPE_BOOL || data_type->base_type == HLSL_TYPE_DOUBLE)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, data_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong argument type of asfloat(): expected 'int', 'uint', 'float', or 'half', but got '%s'.",
                    string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
    data_type = convert_numeric_type(ctx, data_type, HLSL_TYPE_FLOAT);

    operands[0] = params->args[0];
    return add_expr(ctx, params->instrs, HLSL_OP1_REINTERPRET, operands, data_type, loc);
}

static bool intrinsic_asuint(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *data_type;

    if (params->args_count != 1 && params->args_count != 3)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Wrong number of arguments to function 'asuint': expected 1 or 3, but got %u.", params->args_count);
        return false;
    }

    if (params->args_count == 3)
    {
        hlsl_fixme(ctx, loc, "Double-to-integer conversion.");
        return false;
    }

    data_type = params->args[0]->data_type;
    if (data_type->base_type == HLSL_TYPE_BOOL || data_type->base_type == HLSL_TYPE_DOUBLE)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, data_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for argument 0 of asuint(): expected 'int', 'uint', 'float', or 'half', but got '%s'.",
                    string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
    data_type = convert_numeric_type(ctx, data_type, HLSL_TYPE_UINT);

    operands[0] = params->args[0];
    return add_expr(ctx, params->instrs, HLSL_OP1_REINTERPRET, operands, data_type, loc);
}

static bool intrinsic_ceil(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_CEIL, arg, loc);
}

static bool intrinsic_clamp(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *max;

    if (!elementwise_intrinsic_convert_args(ctx, params, loc))
        return false;

    if (!(max = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MAX, params->args[0], params->args[1], loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MIN, max, params->args[2], loc);
}

static bool intrinsic_clip(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *condition, *jump;

    if (!elementwise_intrinsic_float_convert_args(ctx, params, loc))
        return false;

    condition = params->args[0];

    if (ctx->profile->major_version < 4 && hlsl_type_component_count(condition->data_type) > 4)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, condition->data_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Argument type cannot exceed 4 components, got type \"%s\".", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_DISCARD_NEG, condition, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, jump);

    return true;
}

static bool intrinsic_cos(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_COS, arg, loc);
}

static bool intrinsic_cross(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg1_swzl1, *arg1_swzl2, *arg2_swzl1, *arg2_swzl2;
    struct hlsl_ir_node *arg1 = params->args[0], *arg2 = params->args[1];
    struct hlsl_ir_node *arg1_cast, *arg2_cast, *mul1_neg, *mul1, *mul2;
    struct hlsl_type *cast_type;
    enum hlsl_base_type base;

    if (arg1->data_type->base_type == HLSL_TYPE_HALF && arg2->data_type->base_type == HLSL_TYPE_HALF)
        base = HLSL_TYPE_HALF;
    else
        base = HLSL_TYPE_FLOAT;

    cast_type = hlsl_get_vector_type(ctx, base, 3);

    if (!(arg1_cast = add_implicit_conversion(ctx, params->instrs, arg1, cast_type, loc)))
        return false;

    if (!(arg2_cast = add_implicit_conversion(ctx, params->instrs, arg2, cast_type, loc)))
        return false;

    if (!(arg1_swzl1 = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Z, X, Y, Z), 3, arg1_cast, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, arg1_swzl1);

    if (!(arg2_swzl1 = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Y, Z, X, Y), 3, arg2_cast, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, arg2_swzl1);

    if (!(mul1 = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, arg1_swzl1, arg2_swzl1, loc)))
        return false;

    if (!(mul1_neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, mul1, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, mul1_neg);

    if (!(arg1_swzl2 = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Y, Z, X, Y), 3, arg1_cast, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, arg1_swzl2);

    if (!(arg2_swzl2 = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Z, X, Y, Z), 3, arg2_cast, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, arg2_swzl2);

    if (!(mul2 = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, arg1_swzl2, arg2_swzl2, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, mul2, mul1_neg, loc);
}

static bool intrinsic_ddx(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_DSX, arg, loc);
}

static bool intrinsic_ddx_coarse(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_DSX_COARSE, arg, loc);
}

static bool intrinsic_ddx_fine(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_DSX_FINE, arg, loc);
}

static bool intrinsic_ddy(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_DSY, arg, loc);
}

static bool intrinsic_ddy_coarse(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_DSY_COARSE, arg, loc);
}

static bool intrinsic_degrees(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg, *deg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    /* 1 rad = 180/pi degree = 57.2957795 degree */
    if (!(deg = hlsl_new_float_constant(ctx, 57.2957795f, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, deg);

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, arg, deg, loc);
}

static bool intrinsic_ddy_fine(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_DSY_FINE, arg, loc);
}

static bool intrinsic_distance(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg1, *arg2, *neg, *add, *dot;

    if (!(arg1 = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    if (!(arg2 = intrinsic_float_convert_arg(ctx, params, params->args[1], loc)))
        return false;

    if (!(neg = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_NEG, arg2, loc)))
        return false;

    if (!(add = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, arg1, neg, loc)))
        return false;

    if (!(dot = add_binary_dot_expr(ctx, params->instrs, add, add, loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_SQRT, dot, loc);
}

static bool intrinsic_dot(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return !!add_binary_dot_expr(ctx, params->instrs, params->args[0], params->args[1], loc);
}

static bool intrinsic_exp(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg, *mul, *coeff;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    /* 1/ln(2) */
    if (!(coeff = hlsl_new_float_constant(ctx, 1.442695f, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, coeff);

    if (!(mul = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, coeff, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_EXP2, mul, loc);
}

static bool intrinsic_exp2(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_EXP2, arg, loc);
}

static bool intrinsic_floor(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_FLOOR, arg, loc);
}

static bool intrinsic_fmod(struct hlsl_ctx *ctx, const struct parse_initializer *params,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *x, *y, *div, *abs, *frac, *neg_frac, *ge, *select, *zero;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 };
    static const struct hlsl_constant_value zero_value;

    if (!(x = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    if (!(y = intrinsic_float_convert_arg(ctx, params, params->args[1], loc)))
        return false;

    if (!(div = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_DIV, x, y, loc)))
        return false;

    if (!(zero = hlsl_new_constant(ctx, div->data_type, &zero_value, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, zero);

    if (!(abs = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_ABS, div, loc)))
        return false;

    if (!(frac = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_FRACT, abs, loc)))
        return false;

    if (!(neg_frac = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_NEG, frac, loc)))
        return false;

    if (!(ge = add_binary_comparison_expr(ctx, params->instrs, HLSL_OP2_GEQUAL, div, zero, loc)))
        return false;

    operands[0] = ge;
    operands[1] = frac;
    operands[2] = neg_frac;
    if (!(select = add_expr(ctx, params->instrs, HLSL_OP3_TERNARY, operands, x->data_type, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, select, y, loc);
}

static bool intrinsic_frac(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_FRACT, arg, loc);
}

static bool intrinsic_fwidth(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_function_decl *func;
    struct hlsl_type *type;
    char *body;

    static const char template[] =
            "%s fwidth(%s x)\n"
            "{\n"
            "    return abs(ddx(x)) + abs(ddy(x));\n"
            "}";

    if (!elementwise_intrinsic_float_convert_args(ctx, params, loc))
        return false;
    type = params->args[0]->data_type;

    if (!(body = hlsl_sprintf_alloc(ctx, template, type->name, type->name)))
        return false;
    func = hlsl_compile_internal_function(ctx, "fwidth", body);
    vkd3d_free(body);
    if (!func)
        return false;

    return add_user_call(ctx, func, params, loc);
}

static bool intrinsic_ldexp(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!elementwise_intrinsic_float_convert_args(ctx, params, loc))
        return false;

    if (!(arg = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_EXP2, params->args[1], loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, params->args[0], arg, loc);
}

static bool intrinsic_length(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *type = params->args[0]->data_type;
    struct hlsl_ir_node *arg, *dot;

    if (type->class == HLSL_CLASS_MATRIX)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Invalid type %s.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    if (!(dot = add_binary_dot_expr(ctx, params->instrs, arg, arg, loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_SQRT, dot, loc);
}

static bool intrinsic_lerp(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *neg, *add, *mul;

    if (!elementwise_intrinsic_float_convert_args(ctx, params, loc))
        return false;

    if (!(neg = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_NEG, params->args[0], loc)))
        return false;

    if (!(add = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, params->args[1], neg, loc)))
        return false;

    if (!(mul = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, params->args[2], add, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, params->args[0], mul, loc);
}

static struct hlsl_ir_node * add_pow_expr(struct hlsl_ctx *ctx,
        struct hlsl_block *instrs, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *log, *mul;

    if (!(log = add_unary_arithmetic_expr(ctx, instrs, HLSL_OP1_LOG2, arg1, loc)))
        return NULL;

    if (!(mul = add_binary_arithmetic_expr(ctx, instrs, HLSL_OP2_MUL, arg2, log, loc)))
        return NULL;

    return add_unary_arithmetic_expr(ctx, instrs, HLSL_OP1_EXP2, mul, loc);
}

static bool intrinsic_lit(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_function_decl *func;

    static const char body[] =
            "float4 lit(float n_l, float n_h, float m)\n"
            "{\n"
            "    float4 ret;\n"
            "    ret.xw = 1.0;\n"
            "    ret.y = max(n_l, 0);\n"
            "    ret.z = (n_l < 0 || n_h < 0) ? 0 : pow(n_h, m);\n"
            "    return ret;\n"
            "}";

    if (params->args[0]->data_type->class != HLSL_CLASS_SCALAR
            || params->args[1]->data_type->class != HLSL_CLASS_SCALAR
            || params->args[2]->data_type->class != HLSL_CLASS_SCALAR)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Invalid argument type.");
        return false;
    }

    if (!(func = hlsl_compile_internal_function(ctx, "lit", body)))
        return false;

    return add_user_call(ctx, func, params, loc);
}

static bool intrinsic_log(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *log, *arg, *coeff;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    if (!(log = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_LOG2, arg, loc)))
        return false;

    /* ln(2) */
    if (!(coeff = hlsl_new_float_constant(ctx, 0.69314718055f, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, coeff);

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, log, coeff, loc);
}

static bool intrinsic_log10(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *log, *arg, *coeff;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    if (!(log = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_LOG2, arg, loc)))
        return false;

    /* 1 / log2(10) */
    if (!(coeff = hlsl_new_float_constant(ctx, 0.301029996f, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, coeff);

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, log, coeff, loc);
}

static bool intrinsic_log2(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_LOG2, arg, loc);
}

static bool intrinsic_max(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    if (!elementwise_intrinsic_convert_args(ctx, params, loc))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MAX, params->args[0], params->args[1], loc);
}

static bool intrinsic_min(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    if (!elementwise_intrinsic_convert_args(ctx, params, loc))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MIN, params->args[0], params->args[1], loc);
}

static bool intrinsic_mul(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg1 = params->args[0], *arg2 = params->args[1], *cast1, *cast2;
    enum hlsl_base_type base = expr_common_base_type(arg1->data_type->base_type, arg2->data_type->base_type);
    struct hlsl_type *cast_type1 = arg1->data_type, *cast_type2 = arg2->data_type, *matrix_type, *ret_type;
    unsigned int i, j, k, vect_count = 0;
    struct hlsl_deref var_deref;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;

    if (arg1->data_type->class == HLSL_CLASS_SCALAR || arg2->data_type->class == HLSL_CLASS_SCALAR)
        return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, arg1, arg2, loc);

    if (arg1->data_type->class == HLSL_CLASS_VECTOR)
    {
        vect_count++;
        cast_type1 = hlsl_get_matrix_type(ctx, base, arg1->data_type->dimx, 1);
    }
    if (arg2->data_type->class == HLSL_CLASS_VECTOR)
    {
        vect_count++;
        cast_type2 = hlsl_get_matrix_type(ctx, base, 1, arg2->data_type->dimx);
    }

    matrix_type = hlsl_get_matrix_type(ctx, base, cast_type2->dimx, cast_type1->dimy);

    if (vect_count == 0)
    {
        ret_type = matrix_type;
    }
    else if (vect_count == 1)
    {
        assert(matrix_type->dimx == 1 || matrix_type->dimy == 1);
        ret_type = hlsl_get_vector_type(ctx, base, matrix_type->dimx * matrix_type->dimy);
    }
    else
    {
        assert(matrix_type->dimx == 1 && matrix_type->dimy == 1);
        ret_type = hlsl_get_scalar_type(ctx, base);
    }

    if (!(cast1 = add_implicit_conversion(ctx, params->instrs, arg1, cast_type1, loc)))
        return false;

    if (!(cast2 = add_implicit_conversion(ctx, params->instrs, arg2, cast_type2, loc)))
        return false;

    if (!(var = hlsl_new_synthetic_var(ctx, "mul", matrix_type, loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    for (i = 0; i < matrix_type->dimx; ++i)
    {
        for (j = 0; j < matrix_type->dimy; ++j)
        {
            struct hlsl_ir_node *instr = NULL;
            struct hlsl_block block;

            for (k = 0; k < cast_type1->dimx && k < cast_type2->dimy; ++k)
            {
                struct hlsl_ir_node *value1, *value2, *mul;

                if (!(value1 = hlsl_add_load_component(ctx, params->instrs,
                        cast1, j * cast1->data_type->dimx + k, loc)))
                    return false;

                if (!(value2 = hlsl_add_load_component(ctx, params->instrs,
                        cast2, k * cast2->data_type->dimx + i, loc)))
                    return false;

                if (!(mul = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, value1, value2, loc)))
                    return false;

                if (instr)
                {
                    if (!(instr = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, instr, mul, loc)))
                        return false;
                }
                else
                {
                    instr = mul;
                }
            }

            if (!hlsl_new_store_component(ctx, &block, &var_deref, j * matrix_type->dimx + i, instr))
                return false;
            hlsl_block_add_block(params->instrs, &block);
        }
    }

    if (!(load = hlsl_new_var_load(ctx, var, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, &load->node);

    return !!add_implicit_conversion(ctx, params->instrs, &load->node, ret_type, loc);
}

static bool intrinsic_normalize(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *type = params->args[0]->data_type;
    struct hlsl_ir_node *dot, *rsq, *arg;

    if (type->class == HLSL_CLASS_MATRIX)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Invalid type %s.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    if (!(dot = add_binary_dot_expr(ctx, params->instrs, arg, arg, loc)))
        return false;

    if (!(rsq = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_RSQ, dot, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, rsq, arg, loc);
}

static bool intrinsic_pow(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    if (!elementwise_intrinsic_float_convert_args(ctx, params, loc))
        return false;

    return !!add_pow_expr(ctx, params->instrs, params->args[0], params->args[1], loc);
}

static bool intrinsic_radians(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg, *rad;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    /* 1 degree = pi/180 rad = 0.0174532925f rad */
    if (!(rad = hlsl_new_float_constant(ctx, 0.0174532925f, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, rad);

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, arg, rad, loc);
}

static bool intrinsic_reflect(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *i = params->args[0], *n = params->args[1];
    struct hlsl_ir_node *dot, *mul_n, *two_dot, *neg;

    if (!(dot = add_binary_dot_expr(ctx, params->instrs, i, n, loc)))
        return false;

    if (!(two_dot = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, dot, dot, loc)))
        return false;

    if (!(mul_n = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, n, two_dot, loc)))
        return false;

    if (!(neg = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_NEG, mul_n, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, i, neg, loc);
}

static bool intrinsic_round(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_ROUND, arg, loc);
}

static bool intrinsic_rsqrt(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_RSQ, arg, loc);
}

static bool intrinsic_saturate(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_SAT, arg, loc);
}

static bool intrinsic_sign(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *lt, *neg, *op1, *op2, *zero, *arg = params->args[0];
    static const struct hlsl_constant_value zero_value;

    struct hlsl_type *int_type = hlsl_get_numeric_type(ctx, arg->data_type->class, HLSL_TYPE_INT,
            arg->data_type->dimx, arg->data_type->dimy);

    if (!(zero = hlsl_new_constant(ctx, hlsl_get_scalar_type(ctx, arg->data_type->base_type), &zero_value, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, zero);

    /* Check if 0 < arg, cast bool to int */

    if (!(lt = add_binary_comparison_expr(ctx, params->instrs, HLSL_OP2_LESS, zero, arg, loc)))
        return false;

    if (!(op1 = add_implicit_conversion(ctx, params->instrs, lt, int_type, loc)))
        return false;

    /* Check if arg < 0, cast bool to int and invert (meaning true is -1) */

    if (!(lt = add_binary_comparison_expr(ctx, params->instrs, HLSL_OP2_LESS, arg, zero, loc)))
        return false;

    if (!(op2 = add_implicit_conversion(ctx, params->instrs, lt, int_type, loc)))
        return false;

    if (!(neg = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_NEG, op2, loc)))
        return false;

    /* Adding these two together will make 1 when > 0, -1 when < 0, and 0 when neither */
    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, neg, op1, loc);
}

static bool intrinsic_sin(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_SIN, arg, loc);
}

/* smoothstep(a, b, x) = p^2 (3 - 2p), where p = saturate((x - a)/(b - a)) */
static bool intrinsic_smoothstep(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_function_decl *func;
    struct hlsl_type *type;
    char *body;

    static const char template[] =
            "%s smoothstep(%s low, %s high, %s x)\n"
            "{\n"
            "    %s p = saturate((x - low) / (high - low));\n"
            "    return (p * p) * (3 - 2 * p);\n"
            "}";

    if (!(type = elementwise_intrinsic_get_common_type(ctx, params, loc)))
        return false;
    type = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_FLOAT, type->dimx, type->dimy);

    if (!(body = hlsl_sprintf_alloc(ctx, template, type->name, type->name, type->name, type->name, type->name)))
        return false;
    func = hlsl_compile_internal_function(ctx, "smoothstep", body);
    vkd3d_free(body);
    if (!func)
        return false;

    return add_user_call(ctx, func, params, loc);
}

static bool intrinsic_sqrt(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_SQRT, arg, loc);
}

static bool intrinsic_step(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *ge;
    struct hlsl_type *type;

    if (!elementwise_intrinsic_float_convert_args(ctx, params, loc))
        return false;

    if (!(ge = add_binary_comparison_expr(ctx, params->instrs, HLSL_OP2_GEQUAL,
            params->args[1], params->args[0], loc)))
        return false;

    type = ge->data_type;
    type = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_FLOAT, type->dimx, type->dimy);
    return !!add_implicit_conversion(ctx, params->instrs, ge, type, loc);
}

static bool intrinsic_tan(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg = params->args[0], *sin, *cos;

    if (!(sin = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_SIN, arg, loc)))
        return false;

    if (!(cos = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_COS, arg, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_DIV, sin, cos, loc);
}

static bool intrinsic_tex(struct hlsl_ctx *ctx, const struct parse_initializer *params,
        const struct vkd3d_shader_location *loc, const char *name, enum hlsl_sampler_dim dim)
{
    struct hlsl_resource_load_params load_params = { 0 };
    const struct hlsl_type *sampler_type;
    struct hlsl_ir_node *coords, *load;

    if (params->args_count != 2 && params->args_count != 4)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Wrong number of arguments to function '%s': expected 2 or 4, but got %u.", name, params->args_count);
        return false;
    }

    if (params->args_count == 4)
    {
        hlsl_fixme(ctx, loc, "Samples with gradients are not implemented.");
    }

    sampler_type = params->args[0]->data_type;
    if (sampler_type->class != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
            || (sampler_type->sampler_dim != dim && sampler_type->sampler_dim != HLSL_SAMPLER_DIM_GENERIC))
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, sampler_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for argument 1 of '%s': expected 'sampler' or '%s', but got '%s'.",
                    name, ctx->builtin_types.sampler[dim]->name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }

    if (!strcmp(name, "tex2Dlod"))
    {
        struct hlsl_ir_node *lod, *c;

        load_params.type = HLSL_RESOURCE_SAMPLE_LOD;

        if (!(c = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, Y, Z, W), hlsl_sampler_dim_count(dim), params->args[1], loc)))
            return false;
        hlsl_block_add_instr(params->instrs, c);

        if (!(coords = add_implicit_conversion(ctx, params->instrs, c, hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT,
                hlsl_sampler_dim_count(dim)), loc)))
        {
            return false;
        }

        if (!(lod = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(W, W, W, W), 1, params->args[1], loc)))
            return false;
        hlsl_block_add_instr(params->instrs, lod);

        if (!(load_params.lod = add_implicit_conversion(ctx, params->instrs, lod,
                hlsl_get_scalar_type(ctx, HLSL_TYPE_FLOAT), loc)))
        {
            return false;
        }
    }
    else if (!strcmp(name, "tex2Dproj")
            || !strcmp(name, "tex3Dproj")
            || !strcmp(name, "texCUBEproj"))
    {
        if (!(coords = add_implicit_conversion(ctx, params->instrs, params->args[1],
                hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4), loc)))
        {
            return false;
        }

        if (shader_profile_version_ge(ctx, 4, 0))
        {
            unsigned int count = hlsl_sampler_dim_count(dim);
            struct hlsl_ir_node *divisor;

            if (!(divisor = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(W, W, W, W), count, coords, loc)))
                return false;
            hlsl_block_add_instr(params->instrs, divisor);

            if (!(coords = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, Y, Z, W), count, coords, loc)))
                return false;
            hlsl_block_add_instr(params->instrs, coords);

            if (!(coords = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_DIV, coords, divisor, loc)))
                return false;

            load_params.type = HLSL_RESOURCE_SAMPLE;
        }
        else
        {
            load_params.type = HLSL_RESOURCE_SAMPLE_PROJ;
        }
    }
    else
    {
        load_params.type = HLSL_RESOURCE_SAMPLE;

        if (!(coords = add_implicit_conversion(ctx, params->instrs, params->args[1],
                hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, hlsl_sampler_dim_count(dim)), loc)))
        {
            return false;
        }
    }

    /* tex1D() functions never produce 1D resource declarations. For newer profiles half offset
       is used for the second coordinate, while older ones appear to replicate first coordinate.*/
    if (dim == HLSL_SAMPLER_DIM_1D)
    {
        struct hlsl_ir_load *load;
        struct hlsl_ir_node *half;
        struct hlsl_ir_var *var;
        unsigned int idx = 0;

        if (!(var = hlsl_new_synthetic_var(ctx, "coords", hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 2), loc)))
            return false;

        initialize_var_components(ctx, params->instrs, var, &idx, coords);
        if (shader_profile_version_ge(ctx, 4, 0))
        {
            if (!(half = hlsl_new_float_constant(ctx, 0.5f, loc)))
                return false;
            hlsl_block_add_instr(params->instrs, half);

            initialize_var_components(ctx, params->instrs, var, &idx, half);
        }
        else
            initialize_var_components(ctx, params->instrs, var, &idx, coords);

        if (!(load = hlsl_new_var_load(ctx, var, loc)))
            return false;
        hlsl_block_add_instr(params->instrs, &load->node);

        coords = &load->node;

        dim = HLSL_SAMPLER_DIM_2D;
    }

    load_params.coords = coords;
    load_params.resource = params->args[0];
    load_params.format = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4);
    load_params.sampling_dim = dim;

    if (!(load = hlsl_new_resource_load(ctx, &load_params, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, load);
    return true;
}

static bool intrinsic_tex1D(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return intrinsic_tex(ctx, params, loc, "tex1D", HLSL_SAMPLER_DIM_1D);
}

static bool intrinsic_tex2D(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return intrinsic_tex(ctx, params, loc, "tex2D", HLSL_SAMPLER_DIM_2D);
}

static bool intrinsic_tex2Dlod(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return intrinsic_tex(ctx, params, loc, "tex2Dlod", HLSL_SAMPLER_DIM_2D);
}

static bool intrinsic_tex2Dproj(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return intrinsic_tex(ctx, params, loc, "tex2Dproj", HLSL_SAMPLER_DIM_2D);
}

static bool intrinsic_tex3D(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return intrinsic_tex(ctx, params, loc, "tex3D", HLSL_SAMPLER_DIM_3D);
}

static bool intrinsic_tex3Dproj(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return intrinsic_tex(ctx, params, loc, "tex3Dproj", HLSL_SAMPLER_DIM_3D);
}

static bool intrinsic_texCUBE(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return intrinsic_tex(ctx, params, loc, "texCUBE", HLSL_SAMPLER_DIM_CUBE);
}

static bool intrinsic_texCUBEproj(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return intrinsic_tex(ctx, params, loc, "texCUBEproj", HLSL_SAMPLER_DIM_CUBE);
}

static bool intrinsic_transpose(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg = params->args[0];
    struct hlsl_type *arg_type = arg->data_type;
    struct hlsl_ir_load *var_load;
    struct hlsl_deref var_deref;
    struct hlsl_type *mat_type;
    struct hlsl_ir_node *load;
    struct hlsl_ir_var *var;
    unsigned int i, j;

    if (arg_type->class != HLSL_CLASS_SCALAR && arg_type->class != HLSL_CLASS_MATRIX)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, arg_type)))
            hlsl_error(ctx, &arg->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                   "Wrong type for argument 1 of transpose(): expected a matrix or scalar type, but got '%s'.",
                   string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (arg_type->class == HLSL_CLASS_SCALAR)
    {
        hlsl_block_add_instr(params->instrs, arg);
        return true;
    }

    mat_type = hlsl_get_matrix_type(ctx, arg_type->base_type, arg_type->dimy, arg_type->dimx);

    if (!(var = hlsl_new_synthetic_var(ctx, "transpose", mat_type, loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    for (i = 0; i < arg_type->dimx; ++i)
    {
        for (j = 0; j < arg_type->dimy; ++j)
        {
            struct hlsl_block block;

            if (!(load = hlsl_add_load_component(ctx, params->instrs, arg, j * arg->data_type->dimx + i, loc)))
                return false;

            if (!hlsl_new_store_component(ctx, &block, &var_deref, i * var->data_type->dimx + j, load))
                return false;
            hlsl_block_add_block(params->instrs, &block);
        }
    }

    if (!(var_load = hlsl_new_var_load(ctx, var, loc)))
        return false;
    hlsl_block_add_instr(params->instrs, &var_load->node);

    return true;
}

static bool intrinsic_trunc(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_TRUNC, arg, loc);
}

static bool intrinsic_d3dcolor_to_ubyte4(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg = params->args[0], *ret, *c, *swizzle;
    struct hlsl_type *arg_type = arg->data_type;

    if (arg_type->class != HLSL_CLASS_SCALAR && !(arg_type->class == HLSL_CLASS_VECTOR && arg_type->dimx == 4))
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, arg_type)))
        {
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Wrong argument type '%s'.", string->buffer);
            hlsl_release_string_buffer(ctx, string);
        }

        return false;
    }

    if (!(arg = intrinsic_float_convert_arg(ctx, params, arg, loc)))
        return false;

    if (!(c = hlsl_new_float_constant(ctx, 255.0f + (0.5f / 256.0f), loc)))
        return false;
    hlsl_block_add_instr(params->instrs, c);

    if (arg_type->class == HLSL_CLASS_VECTOR)
    {
        if (!(swizzle = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Z, Y, X, W), 4, arg, loc)))
            return false;
        hlsl_block_add_instr(params->instrs, swizzle);

        arg = swizzle;
    }

    if (!(ret = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, arg, c, loc)))
        return false;

    if (shader_profile_version_ge(ctx, 4, 0))
        return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_TRUNC, ret, loc);

    return true;
}

static const struct intrinsic_function
{
    const char *name;
    int param_count;
    bool check_numeric;
    bool (*handler)(struct hlsl_ctx *ctx, const struct parse_initializer *params,
            const struct vkd3d_shader_location *loc);
}
intrinsic_functions[] =
{
    /* Note: these entries should be kept in alphabetical order. */
    {"D3DCOLORtoUBYTE4",                    1, true,  intrinsic_d3dcolor_to_ubyte4},
    {"abs",                                 1, true,  intrinsic_abs},
    {"all",                                 1, true,  intrinsic_all},
    {"any",                                 1, true,  intrinsic_any},
    {"asfloat",                             1, true,  intrinsic_asfloat},
    {"asuint",                             -1, true,  intrinsic_asuint},
    {"ceil",                                1, true,  intrinsic_ceil},
    {"clamp",                               3, true,  intrinsic_clamp},
    {"clip",                                1, true,  intrinsic_clip},
    {"cos",                                 1, true,  intrinsic_cos},
    {"cross",                               2, true,  intrinsic_cross},
    {"ddx",                                 1, true,  intrinsic_ddx},
    {"ddx_coarse",                          1, true,  intrinsic_ddx_coarse},
    {"ddx_fine",                            1, true,  intrinsic_ddx_fine},
    {"ddy",                                 1, true,  intrinsic_ddy},
    {"ddy_coarse",                          1, true,  intrinsic_ddy_coarse},
    {"ddy_fine",                            1, true,  intrinsic_ddy_fine},
    {"degrees",                             1, true,  intrinsic_degrees},
    {"distance",                            2, true,  intrinsic_distance},
    {"dot",                                 2, true,  intrinsic_dot},
    {"exp",                                 1, true,  intrinsic_exp},
    {"exp2",                                1, true,  intrinsic_exp2},
    {"floor",                               1, true,  intrinsic_floor},
    {"fmod",                                2, true,  intrinsic_fmod},
    {"frac",                                1, true,  intrinsic_frac},
    {"fwidth",                              1, true,  intrinsic_fwidth},
    {"ldexp",                               2, true,  intrinsic_ldexp},
    {"length",                              1, true,  intrinsic_length},
    {"lerp",                                3, true,  intrinsic_lerp},
    {"lit",                                 3, true,  intrinsic_lit},
    {"log",                                 1, true,  intrinsic_log},
    {"log10",                               1, true,  intrinsic_log10},
    {"log2",                                1, true,  intrinsic_log2},
    {"max",                                 2, true,  intrinsic_max},
    {"min",                                 2, true,  intrinsic_min},
    {"mul",                                 2, true,  intrinsic_mul},
    {"normalize",                           1, true,  intrinsic_normalize},
    {"pow",                                 2, true,  intrinsic_pow},
    {"radians",                             1, true,  intrinsic_radians},
    {"reflect",                             2, true,  intrinsic_reflect},
    {"round",                               1, true,  intrinsic_round},
    {"rsqrt",                               1, true,  intrinsic_rsqrt},
    {"saturate",                            1, true,  intrinsic_saturate},
    {"sign",                                1, true,  intrinsic_sign},
    {"sin",                                 1, true,  intrinsic_sin},
    {"smoothstep",                          3, true,  intrinsic_smoothstep},
    {"sqrt",                                1, true,  intrinsic_sqrt},
    {"step",                                2, true,  intrinsic_step},
    {"tan",                                 1, true,  intrinsic_tan},
    {"tex1D",                              -1, false, intrinsic_tex1D},
    {"tex2D",                              -1, false, intrinsic_tex2D},
    {"tex2Dlod",                            2, false, intrinsic_tex2Dlod},
    {"tex2Dproj",                           2, false, intrinsic_tex2Dproj},
    {"tex3D",                              -1, false, intrinsic_tex3D},
    {"tex3Dproj",                           2, false, intrinsic_tex3Dproj},
    {"texCUBE",                            -1, false, intrinsic_texCUBE},
    {"texCUBEproj",                         2, false, intrinsic_texCUBEproj},
    {"transpose",                           1, true,  intrinsic_transpose},
    {"trunc",                               1, true,  intrinsic_trunc},
};

static int intrinsic_function_name_compare(const void *a, const void *b)
{
    const struct intrinsic_function *func = b;

    return strcmp(a, func->name);
}

static struct hlsl_block *add_call(struct hlsl_ctx *ctx, const char *name,
        struct parse_initializer *args, const struct vkd3d_shader_location *loc)
{
    struct intrinsic_function *intrinsic;
    struct hlsl_ir_function_decl *decl;

    if ((decl = find_function_call(ctx, name, args, loc)))
    {
        if (!add_user_call(ctx, decl, args, loc))
            goto fail;
    }
    else if ((intrinsic = bsearch(name, intrinsic_functions, ARRAY_SIZE(intrinsic_functions),
            sizeof(*intrinsic_functions), intrinsic_function_name_compare)))
    {
        if (intrinsic->param_count >= 0 && args->args_count != intrinsic->param_count)
        {
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                    "Wrong number of arguments to function '%s': expected %u, but got %u.",
                    name, intrinsic->param_count, args->args_count);
            goto fail;
        }

        if (intrinsic->check_numeric)
        {
            unsigned int i;

            for (i = 0; i < args->args_count; ++i)
            {
                if (!hlsl_is_numeric_type(args->args[i]->data_type))
                {
                    struct vkd3d_string_buffer *string;

                    if ((string = hlsl_type_to_string(ctx, args->args[i]->data_type)))
                        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                                "Wrong type for argument %u of '%s': expected a numeric type, but got '%s'.",
                                i + 1, name, string->buffer);
                    hlsl_release_string_buffer(ctx, string);
                    goto fail;
                }
            }
        }

        if (!intrinsic->handler(ctx, args, loc))
            goto fail;
    }
    else if (rb_get(&ctx->functions, name))
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED, "No compatible %u parameter declaration for \"%s\" found.",
                args->args_count, name);
        goto fail;
    }
    else
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED, "Function \"%s\" is not defined.", name);
        goto fail;
    }
    vkd3d_free(args->args);
    return args->instrs;

fail:
    free_parse_initializer(args);
    return NULL;
}

static struct hlsl_block *add_constructor(struct hlsl_ctx *ctx, struct hlsl_type *type,
        struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;
    unsigned int i, idx = 0;

    if (!(var = hlsl_new_synthetic_var(ctx, "constructor", type, loc)))
        return NULL;

    for (i = 0; i < params->args_count; ++i)
    {
        struct hlsl_ir_node *arg = params->args[i];

        if (arg->data_type->class == HLSL_CLASS_OBJECT)
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_type_to_string(ctx, arg->data_type)))
                hlsl_error(ctx, &arg->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Invalid type %s for constructor argument.", string->buffer);
            hlsl_release_string_buffer(ctx, string);
            continue;
        }

        initialize_var_components(ctx, params->instrs, var, &idx, arg);
    }

    if (!(load = hlsl_new_var_load(ctx, var, loc)))
        return NULL;
    hlsl_block_add_instr(params->instrs, &load->node);

    vkd3d_free(params->args);
    return params->instrs;
}

static unsigned int hlsl_offset_dim_count(enum hlsl_sampler_dim dim)
{
    switch (dim)
    {
        case HLSL_SAMPLER_DIM_1D:
        case HLSL_SAMPLER_DIM_1DARRAY:
            return 1;
        case HLSL_SAMPLER_DIM_2D:
        case HLSL_SAMPLER_DIM_2DMS:
        case HLSL_SAMPLER_DIM_2DARRAY:
        case HLSL_SAMPLER_DIM_2DMSARRAY:
            return 2;
        case HLSL_SAMPLER_DIM_3D:
            return 3;
        case HLSL_SAMPLER_DIM_CUBE:
        case HLSL_SAMPLER_DIM_CUBEARRAY:
            /* Offset parameters not supported for these types. */
            return 0;
        default:
            vkd3d_unreachable();
    }
}

static bool raise_invalid_method_object_type(struct hlsl_ctx *ctx, const struct hlsl_type *object_type,
        const char *method, const struct vkd3d_shader_location *loc)
{
    struct vkd3d_string_buffer *string;

    if ((string = hlsl_type_to_string(ctx, object_type)))
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED,
                "Method '%s' is not defined on type '%s'.", method, string->buffer);
    hlsl_release_string_buffer(ctx, string);
    return false;
}

static bool add_load_method_call(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
    const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
    struct hlsl_resource_load_params load_params = {.type = HLSL_RESOURCE_LOAD};
    struct hlsl_ir_node *load;
    bool multisampled;

    if (object_type->sampler_dim == HLSL_SAMPLER_DIM_CUBE
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_CUBEARRAY)
    {
        return raise_invalid_method_object_type(ctx, object_type, name, loc);
    }

    multisampled = object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMS
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMSARRAY;

    if (params->args_count < 1 + multisampled || params->args_count > 3 + multisampled)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Wrong number of arguments to method 'Load': expected between %u and %u, but got %u.",
                1 + multisampled, 3 + multisampled, params->args_count);
        return false;
    }
    if (multisampled)
    {
        if (!(load_params.sample_index = add_implicit_conversion(ctx, block, params->args[1],
                hlsl_get_scalar_type(ctx, HLSL_TYPE_INT), loc)))
            return false;
    }

    assert(offset_dim);
    if (params->args_count > 1 + multisampled)
    {
        if (!(load_params.texel_offset = add_implicit_conversion(ctx, block, params->args[1 + multisampled],
                hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
            return false;
    }
    if (params->args_count > 2 + multisampled)
    {
        hlsl_fixme(ctx, loc, "Tiled resource status argument.");
    }

    /* +1 for the mipmap level for non-multisampled textures */
    if (!(load_params.coords = add_implicit_conversion(ctx, block, params->args[0],
            hlsl_get_vector_type(ctx, HLSL_TYPE_INT, sampler_dim + !multisampled), loc)))
        return false;

    load_params.format = object_type->e.resource_format;
    load_params.resource = object;

    if (!(load = hlsl_new_resource_load(ctx, &load_params, loc)))
        return false;
    hlsl_block_add_instr(block, load);
    return true;
}

static bool add_sample_method_call(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
    const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
    struct hlsl_resource_load_params load_params = {.type = HLSL_RESOURCE_SAMPLE};
    const struct hlsl_type *sampler_type;
    struct hlsl_ir_node *load;

    if (object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMS
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMSARRAY)
    {
        return raise_invalid_method_object_type(ctx, object_type, name, loc);
    }

    if (params->args_count < 2 || params->args_count > 4 + !!offset_dim)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Wrong number of arguments to method 'Sample': expected from 2 to %u, but got %u.",
                4 + !!offset_dim, params->args_count);
        return false;
    }

    sampler_type = params->args[0]->data_type;
    if (sampler_type->class != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
            || sampler_type->sampler_dim != HLSL_SAMPLER_DIM_GENERIC)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, sampler_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for argument 0 of Sample(): expected 'sampler', but got '%s'.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (!(load_params.coords = add_implicit_conversion(ctx, block, params->args[1],
            hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
        return false;

    if (offset_dim && params->args_count > 2)
    {
        if (!(load_params.texel_offset = add_implicit_conversion(ctx, block, params->args[2],
                hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
            return false;
    }

    if (params->args_count > 2 + !!offset_dim)
        hlsl_fixme(ctx, loc, "Sample() clamp parameter.");
    if (params->args_count > 3 + !!offset_dim)
        hlsl_fixme(ctx, loc, "Tiled resource status argument.");

    load_params.format = object_type->e.resource_format;
    load_params.resource = object;
    load_params.sampler = params->args[0];

    if (!(load = hlsl_new_resource_load(ctx, &load_params, loc)))
        return false;
    hlsl_block_add_instr(block, load);

    return true;
}

static bool add_sample_cmp_method_call(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
    const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
    struct hlsl_resource_load_params load_params = { 0 };
    const struct hlsl_type *sampler_type;
    struct hlsl_ir_node *load;

    if (object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMS
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMSARRAY)
    {
        return raise_invalid_method_object_type(ctx, object_type, name, loc);
    }

    if (!strcmp(name, "SampleCmpLevelZero"))
        load_params.type = HLSL_RESOURCE_SAMPLE_CMP_LZ;
    else
        load_params.type = HLSL_RESOURCE_SAMPLE_CMP;

    if (params->args_count < 3 || params->args_count > 5 + !!offset_dim)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Wrong number of arguments to method '%s': expected from 3 to %u, but got %u.",
                name, 5 + !!offset_dim, params->args_count);
        return false;
    }

    sampler_type = params->args[0]->data_type;
    if (sampler_type->class != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
            || sampler_type->sampler_dim != HLSL_SAMPLER_DIM_COMPARISON)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, sampler_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for argument 0 of %s(): expected 'SamplerComparisonState', but got '%s'.",
                    name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (!(load_params.coords = add_implicit_conversion(ctx, block, params->args[1],
            hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
        return false;

    if (!(load_params.cmp = add_implicit_conversion(ctx, block, params->args[2],
            hlsl_get_scalar_type(ctx, HLSL_TYPE_FLOAT), loc)))
        load_params.cmp = params->args[2];

    if (offset_dim && params->args_count > 3)
    {
        if (!(load_params.texel_offset = add_implicit_conversion(ctx, block, params->args[2],
                hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
            return false;
    }

    if (params->args_count > 3 + !!offset_dim)
        hlsl_fixme(ctx, loc, "%s() clamp parameter.", name);
    if (params->args_count > 4 + !!offset_dim)
        hlsl_fixme(ctx, loc, "Tiled resource status argument.");

    load_params.format = object_type->e.resource_format;
    load_params.resource = object;
    load_params.sampler = params->args[0];

    if (!(load = hlsl_new_resource_load(ctx, &load_params, loc)))
        return false;
    hlsl_block_add_instr(block, load);

    return true;
}

static bool add_gather_method_call(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
    const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
    struct hlsl_resource_load_params load_params = {0};
    const struct hlsl_type *sampler_type;
    struct hlsl_ir_node *load;
    unsigned int read_channel;

    if (object_type->sampler_dim != HLSL_SAMPLER_DIM_2D
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_2DARRAY
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_CUBE
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_CUBEARRAY)
    {
        return raise_invalid_method_object_type(ctx, object_type, name, loc);
    }

    if (!strcmp(name, "GatherGreen"))
    {
        load_params.type = HLSL_RESOURCE_GATHER_GREEN;
        read_channel = 1;
    }
    else if (!strcmp(name, "GatherBlue"))
    {
        load_params.type = HLSL_RESOURCE_GATHER_BLUE;
        read_channel = 2;
    }
    else if (!strcmp(name, "GatherAlpha"))
    {
        load_params.type = HLSL_RESOURCE_GATHER_ALPHA;
        read_channel = 3;
    }
    else
    {
        load_params.type = HLSL_RESOURCE_GATHER_RED;
        read_channel = 0;
    }

    if (!strcmp(name, "Gather") || !offset_dim)
    {
        if (params->args_count < 2 || params->args_count > 3 + !!offset_dim)
        {
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                    "Wrong number of arguments to method '%s': expected from 2 to %u, but got %u.",
                    name, 3 + !!offset_dim, params->args_count);
            return false;
        }
    }
    else if (params->args_count < 2 || params->args_count == 5 || params->args_count > 7)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Wrong number of arguments to method '%s': expected 2, 3, 4, 6 or 7, but got %u.",
                name, params->args_count);
        return false;
    }

    if (params->args_count == 3 + !!offset_dim || params->args_count == 7)
        hlsl_fixme(ctx, loc, "Tiled resource status argument.");

    if (params->args_count == 6 || params->args_count == 7)
    {
        hlsl_fixme(ctx, loc, "Multiple %s() offset parameters.", name);
    }
    else if (offset_dim && params->args_count > 2)
    {
        if (!(load_params.texel_offset = add_implicit_conversion(ctx, block, params->args[2],
                hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
            return false;
    }

    sampler_type = params->args[0]->data_type;
    if (sampler_type->class != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
            || sampler_type->sampler_dim != HLSL_SAMPLER_DIM_GENERIC)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, sampler_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for argument 1 of %s(): expected 'sampler', but got '%s'.", name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (read_channel >= object_type->e.resource_format->dimx)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                "Method %s() requires at least %u channels.", name, read_channel + 1);
        return false;
    }

    if (!(load_params.coords = add_implicit_conversion(ctx, block, params->args[1],
            hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
        return false;

    load_params.format = hlsl_get_vector_type(ctx, object_type->e.resource_format->base_type, 4);
    load_params.resource = object;
    load_params.sampler = params->args[0];

    if (!(load = hlsl_new_resource_load(ctx, &load_params, loc)))
        return false;
    hlsl_block_add_instr(block, load);
    return true;
}

static bool add_assignment_from_component(struct hlsl_ctx *ctx, struct hlsl_block *instrs, struct hlsl_ir_node *dest,
        struct hlsl_ir_node *src, unsigned int component, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *load;

    if (!dest)
        return true;

    if (!(load = hlsl_add_load_component(ctx, instrs, src, component, loc)))
        return false;

    if (!add_assignment(ctx, instrs, dest, ASSIGN_OP_ASSIGN, load))
        return false;

    return true;
}

static bool add_getdimensions_method_call(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    bool uint_resinfo, has_uint_arg, has_float_arg;
    struct hlsl_resource_load_params load_params;
    struct hlsl_ir_node *sample_info, *res_info;
    struct hlsl_ir_node *zero = NULL, *void_ret;
    struct hlsl_type *uint_type, *float_type;
    unsigned int i, j;
    enum func_argument
    {
        ARG_MIP_LEVEL,
        ARG_WIDTH,
        ARG_HEIGHT,
        ARG_ELEMENT_COUNT,
        ARG_LEVEL_COUNT,
        ARG_SAMPLE_COUNT,
        ARG_MAX_ARGS,
    };
    struct hlsl_ir_node *args[ARG_MAX_ARGS] = { 0 };
    static const struct overload
    {
        enum hlsl_sampler_dim sampler_dim;
        unsigned int args_count;
        enum func_argument args[ARG_MAX_ARGS];
    }
    overloads[] =
    {
        { HLSL_SAMPLER_DIM_1D, 1, { ARG_WIDTH } },
        { HLSL_SAMPLER_DIM_1D, 3, { ARG_MIP_LEVEL, ARG_WIDTH, ARG_LEVEL_COUNT } },
        { HLSL_SAMPLER_DIM_1DARRAY, 2, { ARG_WIDTH, ARG_ELEMENT_COUNT } },
        { HLSL_SAMPLER_DIM_1DARRAY, 4, { ARG_MIP_LEVEL, ARG_WIDTH, ARG_ELEMENT_COUNT, ARG_LEVEL_COUNT } },
        { HLSL_SAMPLER_DIM_2D, 2, { ARG_WIDTH, ARG_HEIGHT } },
        { HLSL_SAMPLER_DIM_2D, 4, { ARG_MIP_LEVEL, ARG_WIDTH, ARG_HEIGHT, ARG_LEVEL_COUNT } },
        { HLSL_SAMPLER_DIM_2DARRAY, 3, { ARG_WIDTH, ARG_HEIGHT, ARG_ELEMENT_COUNT } },
        { HLSL_SAMPLER_DIM_2DARRAY, 5, { ARG_MIP_LEVEL, ARG_WIDTH, ARG_HEIGHT, ARG_ELEMENT_COUNT, ARG_LEVEL_COUNT } },
        { HLSL_SAMPLER_DIM_3D, 3, { ARG_WIDTH, ARG_HEIGHT, ARG_ELEMENT_COUNT } },
        { HLSL_SAMPLER_DIM_3D, 5, { ARG_MIP_LEVEL, ARG_WIDTH, ARG_HEIGHT, ARG_ELEMENT_COUNT, ARG_LEVEL_COUNT } },
        { HLSL_SAMPLER_DIM_CUBE, 2, { ARG_WIDTH, ARG_HEIGHT } },
        { HLSL_SAMPLER_DIM_CUBE, 4, { ARG_MIP_LEVEL, ARG_WIDTH, ARG_HEIGHT, ARG_LEVEL_COUNT } },
        { HLSL_SAMPLER_DIM_CUBEARRAY, 3, { ARG_WIDTH, ARG_HEIGHT, ARG_ELEMENT_COUNT } },
        { HLSL_SAMPLER_DIM_CUBEARRAY, 5, { ARG_MIP_LEVEL, ARG_WIDTH, ARG_HEIGHT, ARG_ELEMENT_COUNT, ARG_LEVEL_COUNT } },
        { HLSL_SAMPLER_DIM_2DMS, 3, { ARG_WIDTH, ARG_HEIGHT, ARG_SAMPLE_COUNT } },
        { HLSL_SAMPLER_DIM_2DMSARRAY, 4, { ARG_WIDTH, ARG_HEIGHT, ARG_ELEMENT_COUNT, ARG_SAMPLE_COUNT } },
    };
    const struct overload *o = NULL;

    if (object_type->sampler_dim > HLSL_SAMPLER_DIM_LAST_TEXTURE)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "GetDimensions() is not defined for this type.");
    }

    uint_type = hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT);
    float_type = hlsl_get_scalar_type(ctx, HLSL_TYPE_FLOAT);
    has_uint_arg = has_float_arg = false;
    for (i = 0; i < ARRAY_SIZE(overloads); ++i)
    {
        const struct overload *iter = &overloads[i];

        if (iter->sampler_dim == object_type->sampler_dim && iter->args_count == params->args_count)
        {
            for (j = 0; j < params->args_count; ++j)
            {
                args[iter->args[j]] = params->args[j];

                /* Input parameter. */
                if (iter->args[j] == ARG_MIP_LEVEL)
                {
                    if (!(args[ARG_MIP_LEVEL] = add_implicit_conversion(ctx, block, args[ARG_MIP_LEVEL],
                            hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT), loc)))
                    {
                        return false;
                    }

                    continue;
                }

                has_float_arg |= hlsl_types_are_equal(params->args[j]->data_type, float_type);
                has_uint_arg |= hlsl_types_are_equal(params->args[j]->data_type, uint_type);

                if (params->args[j]->data_type->class != HLSL_CLASS_SCALAR)
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Expected scalar arguments.");
                    break;
                }
            }
            o = iter;
            break;
        }
    }
    uint_resinfo = !has_float_arg && has_uint_arg;

    if (!o)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, object_type)))
        {
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                    "Unexpected number of arguments %u for %s.%s().", params->args_count, string->buffer, name);
            hlsl_release_string_buffer(ctx, string);
        }
    }

    if (!args[ARG_MIP_LEVEL])
    {
        if (!(zero = hlsl_new_uint_constant(ctx, 0, loc)))
            return false;
        hlsl_block_add_instr(block, zero);
        args[ARG_MIP_LEVEL] = zero;
    }

    memset(&load_params, 0, sizeof(load_params));
    load_params.type = HLSL_RESOURCE_RESINFO;
    load_params.resource = object;
    load_params.lod = args[ARG_MIP_LEVEL];
    load_params.format = hlsl_get_vector_type(ctx, uint_resinfo ? HLSL_TYPE_UINT : HLSL_TYPE_FLOAT, 4);

    if (!(res_info = hlsl_new_resource_load(ctx, &load_params, loc)))
        return false;
    hlsl_block_add_instr(block, res_info);

    if (!add_assignment_from_component(ctx, block, args[ARG_WIDTH], res_info, 0, loc))
        return false;

    if (!add_assignment_from_component(ctx, block, args[ARG_HEIGHT], res_info, 1, loc))
        return false;

    if (!add_assignment_from_component(ctx, block, args[ARG_ELEMENT_COUNT], res_info,
            object_type->sampler_dim == HLSL_SAMPLER_DIM_1DARRAY ? 1 : 2, loc))
    {
        return false;
    }

    if (!add_assignment_from_component(ctx, block, args[ARG_LEVEL_COUNT], res_info, 3, loc))
        return false;

    if (args[ARG_SAMPLE_COUNT])
    {
        memset(&load_params, 0, sizeof(load_params));
        load_params.type = HLSL_RESOURCE_SAMPLE_INFO;
        load_params.resource = object;
        load_params.format = args[ARG_SAMPLE_COUNT]->data_type;
        if (!(sample_info = hlsl_new_resource_load(ctx, &load_params, loc)))
            return false;
        hlsl_block_add_instr(block, sample_info);

        if (!add_assignment(ctx, block, args[ARG_SAMPLE_COUNT], ASSIGN_OP_ASSIGN, sample_info))
            return false;
    }

    if (!(void_ret = hlsl_new_void_expr(ctx, loc)))
        return false;
    hlsl_block_add_instr(block, void_ret);

    return true;
}

static bool add_sample_lod_method_call(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    struct hlsl_resource_load_params load_params = { 0 };
    const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
    const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
    const struct hlsl_type *sampler_type;
    struct hlsl_ir_node *load;

    if (object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMS
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMSARRAY)
    {
        return raise_invalid_method_object_type(ctx, object_type, name, loc);
    }

    if (!strcmp(name, "SampleLevel"))
        load_params.type = HLSL_RESOURCE_SAMPLE_LOD;
    else
        load_params.type = HLSL_RESOURCE_SAMPLE_LOD_BIAS;

    if (params->args_count < 3 || params->args_count > 4 + !!offset_dim)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Wrong number of arguments to method '%s': expected from 3 to %u, but got %u.",
                name, 4 + !!offset_dim, params->args_count);
        return false;
    }

    sampler_type = params->args[0]->data_type;
    if (sampler_type->class != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
            || sampler_type->sampler_dim != HLSL_SAMPLER_DIM_GENERIC)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, sampler_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for argument 0 of %s(): expected 'sampler', but got '%s'.", name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (!(load_params.coords = add_implicit_conversion(ctx, block, params->args[1],
            hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
        load_params.coords = params->args[1];

    if (!(load_params.lod = add_implicit_conversion(ctx, block, params->args[2],
            hlsl_get_scalar_type(ctx, HLSL_TYPE_FLOAT), loc)))
        load_params.lod = params->args[2];

    if (offset_dim && params->args_count > 3)
    {
        if (!(load_params.texel_offset = add_implicit_conversion(ctx, block, params->args[3],
                hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
            return false;
    }

    if (params->args_count > 3 + !!offset_dim)
        hlsl_fixme(ctx, loc, "Tiled resource status argument.");

    load_params.format = object_type->e.resource_format;
    load_params.resource = object;
    load_params.sampler = params->args[0];

    if (!(load = hlsl_new_resource_load(ctx, &load_params, loc)))
        return false;
    hlsl_block_add_instr(block, load);
    return true;
}

static bool add_sample_grad_method_call(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    struct hlsl_resource_load_params load_params = { 0 };
    const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
    const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
    const struct hlsl_type *sampler_type;
    struct hlsl_ir_node *load;

    if (object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMS
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_2DMSARRAY)
    {
        return raise_invalid_method_object_type(ctx, object_type, name, loc);
    }

    load_params.type = HLSL_RESOURCE_SAMPLE_GRAD;

    if (params->args_count < 4 || params->args_count > 5 + !!offset_dim)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Wrong number of arguments to method '%s': expected from 4 to %u, but got %u.",
                name, 5 + !!offset_dim, params->args_count);
        return false;
    }

    sampler_type = params->args[0]->data_type;
    if (sampler_type->class != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
            || sampler_type->sampler_dim != HLSL_SAMPLER_DIM_GENERIC)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, sampler_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for argument 0 of %s(): expected 'sampler', but got '%s'.", name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (!(load_params.coords = add_implicit_conversion(ctx, block, params->args[1],
            hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
        load_params.coords = params->args[1];

    if (!(load_params.ddx = add_implicit_conversion(ctx, block, params->args[2],
            hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
        load_params.ddx = params->args[2];

    if (!(load_params.ddy = add_implicit_conversion(ctx, block, params->args[3],
            hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
        load_params.ddy = params->args[3];

    if (offset_dim && params->args_count > 4)
    {
        if (!(load_params.texel_offset = add_implicit_conversion(ctx, block, params->args[4],
                hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
            return false;
    }

    if (params->args_count > 4 + !!offset_dim)
        hlsl_fixme(ctx, loc, "Tiled resource status argument.");

    load_params.format = object_type->e.resource_format;
    load_params.resource = object;
    load_params.sampler = params->args[0];

    if (!(load = hlsl_new_resource_load(ctx, &load_params, loc)))
        return false;
    hlsl_block_add_instr(block, load);
    return true;
}

static const struct method_function
{
    const char *name;
    bool (*handler)(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
            const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc);
}
object_methods[] =
{
    { "Gather",             add_gather_method_call },
    { "GatherAlpha",        add_gather_method_call },
    { "GatherBlue",         add_gather_method_call },
    { "GatherGreen",        add_gather_method_call },
    { "GatherRed",          add_gather_method_call },

    { "GetDimensions",      add_getdimensions_method_call },

    { "Load",               add_load_method_call },

    { "Sample",             add_sample_method_call },
    { "SampleBias",         add_sample_lod_method_call },
    { "SampleCmp",          add_sample_cmp_method_call },
    { "SampleCmpLevelZero", add_sample_cmp_method_call },
    { "SampleGrad",         add_sample_grad_method_call },
    { "SampleLevel",        add_sample_lod_method_call },
};

static int object_method_function_name_compare(const void *a, const void *b)
{
    const struct method_function *func = b;

    return strcmp(a, func->name);
}

static bool add_method_call(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    const struct method_function *method;

    if (object_type->class != HLSL_CLASS_OBJECT || object_type->base_type != HLSL_TYPE_TEXTURE
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_GENERIC)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, object_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Type '%s' does not have methods.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if ((method = bsearch(name, object_methods, ARRAY_SIZE(object_methods),
            sizeof(*method), object_method_function_name_compare)))
    {
        return method->handler(ctx, block, object, name, params, loc);
    }
    else
    {
        return raise_invalid_method_object_type(ctx, object_type, name, loc);
    }
}

static void validate_texture_format_type(struct hlsl_ctx *ctx, struct hlsl_type *format,
        const struct vkd3d_shader_location *loc)
{
    if (format->class > HLSL_CLASS_VECTOR)
    {
        struct vkd3d_string_buffer *string;

        string = hlsl_type_to_string(ctx, format);
        if (string)
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Texture data type %s is not scalar or vector.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
}

static bool check_continue(struct hlsl_ctx *ctx, const struct hlsl_scope *scope, const struct vkd3d_shader_location *loc)
{
    if (scope->_switch)
    {
        hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                "The 'continue' statement is not allowed in 'switch' statements.");
        return false;
    }

    if (scope->loop)
        return true;

    if (scope->upper)
        return check_continue(ctx, scope->upper, loc);

    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "The 'continue' statement is only allowed in loops.");
    return false;
}

static bool is_break_allowed(const struct hlsl_scope *scope)
{
    if (scope->loop || scope->_switch)
        return true;

    return scope->upper ? is_break_allowed(scope->upper) : false;
}

static void check_duplicated_switch_cases(struct hlsl_ctx *ctx, const struct hlsl_ir_switch_case *check, struct list *cases)
{
    struct hlsl_ir_switch_case *c;
    bool found_duplicate = false;

    LIST_FOR_EACH_ENTRY(c, cases, struct hlsl_ir_switch_case, entry)
    {
        if (check->is_default)
        {
            if ((found_duplicate = c->is_default))
            {
                hlsl_error(ctx, &check->loc, VKD3D_SHADER_ERROR_HLSL_DUPLICATE_SWITCH_CASE,
                        "Found multiple 'default' statements.");
                hlsl_note(ctx, &c->loc, VKD3D_SHADER_LOG_ERROR, "The 'default' statement was previously found here.");
            }
        }
        else
        {
            if (c->is_default) continue;
            if ((found_duplicate = (c->value == check->value)))
            {
                hlsl_error(ctx, &check->loc, VKD3D_SHADER_ERROR_HLSL_DUPLICATE_SWITCH_CASE,
                        "Found duplicate 'case' statement.");
                hlsl_note(ctx, &c->loc, VKD3D_SHADER_LOG_ERROR, "The same 'case %d' statement was previously found here.",
                        c->value);
            }
        }

        if (found_duplicate)
            break;
    }
}

}

%locations
%define parse.error verbose
%define api.prefix {hlsl_yy}
%define api.pure full
%expect 1
%lex-param {yyscan_t scanner}
%parse-param {void *scanner}
%parse-param {struct hlsl_ctx *ctx}

%union
{
    struct hlsl_type *type;
    INT intval;
    FLOAT floatval;
    bool boolval;
    char *name;
    DWORD modifiers;
    struct hlsl_ir_node *instr;
    struct hlsl_block *block;
    struct list *list;
    struct parse_fields fields;
    struct parse_function function;
    struct parse_parameter parameter;
    struct hlsl_func_parameters parameters;
    struct parse_initializer initializer;
    struct parse_array_sizes arrays;
    struct parse_variable_def *variable_def;
    struct parse_if_body if_body;
    enum parse_assign_op assign_op;
    struct hlsl_reg_reservation reg_reservation;
    struct parse_colon_attribute colon_attribute;
    struct hlsl_semantic semantic;
    enum hlsl_buffer_type buffer_type;
    enum hlsl_sampler_dim sampler_dim;
    struct hlsl_attribute *attr;
    struct parse_attribute_list attr_list;
    struct hlsl_ir_switch_case *switch_case;
}

%token KW_BLENDSTATE
%token KW_BREAK
%token KW_BUFFER
%token KW_CASE
%token KW_CBUFFER
%token KW_CENTROID
%token KW_COLUMN_MAJOR
%token KW_COMPILE
%token KW_CONST
%token KW_CONTINUE
%token KW_DEFAULT
%token KW_DEPTHSTENCILSTATE
%token KW_DEPTHSTENCILVIEW
%token KW_DISCARD
%token KW_DO
%token KW_DOUBLE
%token KW_ELSE
%token KW_EXTERN
%token KW_FALSE
%token KW_FOR
%token KW_GEOMETRYSHADER
%token KW_GROUPSHARED
%token KW_IF
%token KW_IN
%token KW_INLINE
%token KW_INOUT
%token KW_LINEAR
%token KW_MATRIX
%token KW_NAMESPACE
%token KW_NOINTERPOLATION
%token KW_NOPERSPECTIVE
%token KW_OUT
%token KW_PACKOFFSET
%token KW_PASS
%token KW_PIXELSHADER
%token KW_PRECISE
%token KW_RASTERIZERSTATE
%token KW_RENDERTARGETVIEW
%token KW_RETURN
%token KW_REGISTER
%token KW_ROW_MAJOR
%token KW_RWBUFFER
%token KW_RWSTRUCTUREDBUFFER
%token KW_RWTEXTURE1D
%token KW_RWTEXTURE1DARRAY
%token KW_RWTEXTURE2D
%token KW_RWTEXTURE2DARRAY
%token KW_RWTEXTURE3D
%token KW_SAMPLER
%token KW_SAMPLER1D
%token KW_SAMPLER2D
%token KW_SAMPLER3D
%token KW_SAMPLERCUBE
%token KW_SAMPLER_STATE
%token KW_SAMPLERCOMPARISONSTATE
%token KW_SHARED
%token KW_STATEBLOCK
%token KW_STATEBLOCK_STATE
%token KW_STATIC
%token KW_STRING
%token KW_STRUCT
%token KW_SWITCH
%token KW_TBUFFER
%token KW_TECHNIQUE
%token KW_TECHNIQUE10
%token KW_TECHNIQUE11
%token KW_TEXTURE
%token KW_TEXTURE1D
%token KW_TEXTURE1DARRAY
%token KW_TEXTURE2D
%token KW_TEXTURE2DARRAY
%token KW_TEXTURE2DMS
%token KW_TEXTURE2DMSARRAY
%token KW_TEXTURE3D
%token KW_TEXTURECUBE
%token KW_TEXTURECUBEARRAY
%token KW_TRUE
%token KW_TYPEDEF
%token KW_UNIFORM
%token KW_VECTOR
%token KW_VERTEXSHADER
%token KW_VOID
%token KW_VOLATILE
%token KW_WHILE

%token OP_INC
%token OP_DEC
%token OP_AND
%token OP_OR
%token OP_EQ
%token OP_LEFTSHIFT
%token OP_LEFTSHIFTASSIGN
%token OP_RIGHTSHIFT
%token OP_RIGHTSHIFTASSIGN
%token OP_LE
%token OP_GE
%token OP_NE
%token OP_ADDASSIGN
%token OP_SUBASSIGN
%token OP_MULASSIGN
%token OP_DIVASSIGN
%token OP_MODASSIGN
%token OP_ANDASSIGN
%token OP_ORASSIGN
%token OP_XORASSIGN

%token <floatval> C_FLOAT

%token <intval> C_INTEGER
%token <intval> C_UNSIGNED
%token <intval> PRE_LINE

%type <list> type_specs
%type <list> variables_def
%type <list> variables_def_typed
%type <list> switch_cases

%token <name> VAR_IDENTIFIER
%token <name> NEW_IDENTIFIER
%token <name> STRING
%token <name> TYPE_IDENTIFIER

%type <arrays> arrays

%type <assign_op> assign_op

%type <attr> attribute

%type <attr_list> attribute_list
%type <attr_list> attribute_list_optional

%type <block> add_expr
%type <block> assignment_expr
%type <block> bitand_expr
%type <block> bitor_expr
%type <block> bitxor_expr
%type <block> compound_statement
%type <block> conditional_expr
%type <block> declaration
%type <block> declaration_statement
%type <block> equality_expr
%type <block> expr
%type <block> expr_optional
%type <block> expr_statement
%type <block> initializer_expr
%type <block> jump_statement
%type <block> logicand_expr
%type <block> logicor_expr
%type <block> loop_statement
%type <block> mul_expr
%type <block> postfix_expr
%type <block> primary_expr
%type <block> relational_expr
%type <block> shift_expr
%type <block> selection_statement
%type <block> statement
%type <block> statement_list
%type <block> struct_declaration_without_vars
%type <block> switch_statement
%type <block> unary_expr

%type <boolval> boolean

%type <buffer_type> buffer_type

%type <colon_attribute> colon_attribute

%type <fields> field
%type <fields> fields_list

%type <function> func_prototype
%type <function> func_prototype_no_attrs

%type <initializer> complex_initializer
%type <initializer> complex_initializer_list
%type <initializer> func_arguments
%type <initializer> initializer_expr_list

%type <if_body> if_body

%type <modifiers> var_modifiers

%type <name> any_identifier
%type <name> var_identifier
%type <name> technique_name

%type <parameter> parameter

%type <parameters> param_list
%type <parameters> parameters

%type <reg_reservation> register_opt
%type <reg_reservation> packoffset_opt

%type <sampler_dim> texture_type texture_ms_type uav_type

%type <semantic> semantic

%type <switch_case> switch_case

%type <type> field_type
%type <type> named_struct_spec
%type <type> unnamed_struct_spec
%type <type> struct_spec
%type <type> type
%type <type> type_no_void
%type <type> typedef_type

%type <variable_def> type_spec
%type <variable_def> variable_decl
%type <variable_def> variable_def
%type <variable_def> variable_def_typed

%%

hlsl_prog:
      %empty
    | hlsl_prog func_declaration
    | hlsl_prog buffer_declaration buffer_body
    | hlsl_prog declaration_statement
        {
            if (!list_empty(&$2->instrs))
                hlsl_fixme(ctx, &@2, "Uniform initializer.");
            destroy_block($2);
        }
    | hlsl_prog preproc_directive
    | hlsl_prog technique
    | hlsl_prog ';'

technique_name:
      %empty
        {
            $$ = NULL;
        }
    | any_identifier

pass_list:
      %empty

technique9:
      KW_TECHNIQUE technique_name '{' pass_list '}'
        {
            hlsl_fixme(ctx, &@$, "Unsupported \'technique\' declaration.");
        }

technique10:
      KW_TECHNIQUE10 technique_name '{' pass_list '}'
        {
            if (ctx->profile->type == VKD3D_SHADER_TYPE_EFFECT && ctx->profile->major_version == 2)
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                        "The 'technique10' keyword is invalid for this profile.");

            hlsl_fixme(ctx, &@$, "Unsupported \'technique10\' declaration.");
        }

technique11:
      KW_TECHNIQUE11 technique_name '{' pass_list '}'
        {
            if (ctx->profile->type == VKD3D_SHADER_TYPE_EFFECT && ctx->profile->major_version == 2)
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                        "The 'technique11' keyword is invalid for this profile.");

            hlsl_fixme(ctx, &@$, "Unsupported \'technique11\' declaration.");
        }

technique:
      technique9
    | technique10
    | technique11

buffer_declaration:
      buffer_type any_identifier colon_attribute
        {
            if ($3.semantic.name)
                hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC, "Semantics are not allowed on buffers.");

            if (!(ctx->cur_buffer = hlsl_new_buffer(ctx, $1, $2, &$3.reg_reservation, &@2)))
                YYABORT;
        }

buffer_body:
      '{' declaration_statement_list '}'
        {
            ctx->cur_buffer = ctx->globals_buffer;
        }

buffer_type:
      KW_CBUFFER
        {
            $$ = HLSL_BUFFER_CONSTANT;
        }
    | KW_TBUFFER
        {
            $$ = HLSL_BUFFER_TEXTURE;
        }

declaration_statement_list:
      %empty
    | declaration_statement_list declaration_statement

preproc_directive:
      PRE_LINE STRING
        {
            const char **new_array = NULL;

            ctx->location.line = $1;
            if (strcmp($2, ctx->location.source_name))
                new_array = hlsl_realloc(ctx, ctx->source_files,
                        sizeof(*ctx->source_files) * (ctx->source_files_count + 1));

            if (new_array)
            {
                ctx->source_files = new_array;
                ctx->source_files[ctx->source_files_count++] = $2;
                ctx->location.source_name = $2;
            }
            else
            {
                vkd3d_free($2);
            }
        }

struct_declaration_without_vars:
      var_modifiers struct_spec ';'
        {
            if (!$2->name)
                hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                    "Anonymous struct type must declare a variable.");

            if ($1)
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Modifiers are not allowed on struct type declarations.");

            if (!($$ = make_empty_block(ctx)))
                YYABORT;
        }

struct_spec:
      named_struct_spec
    | unnamed_struct_spec

named_struct_spec:
      KW_STRUCT any_identifier '{' fields_list '}'
        {
            bool ret;

            $$ = hlsl_new_struct_type(ctx, $2, $4.fields, $4.count);

            if (hlsl_get_var(ctx->cur_scope, $2))
            {
                hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_REDEFINED, "\"%s\" is already declared as a variable.", $2);
                YYABORT;
            }

            ret = hlsl_scope_add_type(ctx->cur_scope, $$);
            if (!ret)
            {
                hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_REDEFINED, "Struct \"%s\" is already defined.", $2);
                YYABORT;
            }
        }

unnamed_struct_spec:
      KW_STRUCT '{' fields_list '}'
        {
            $$ = hlsl_new_struct_type(ctx, NULL, $3.fields, $3.count);
        }

any_identifier:
      VAR_IDENTIFIER
    | TYPE_IDENTIFIER
    | NEW_IDENTIFIER

fields_list:
      %empty
        {
            $$.fields = NULL;
            $$.count = 0;
            $$.capacity = 0;
        }
    | fields_list field
        {
            size_t i;

            for (i = 0; i < $2.count; ++i)
            {
                const struct hlsl_struct_field *field = &$2.fields[i];
                const struct hlsl_struct_field *existing;

                if ((existing = get_struct_field($1.fields, $1.count, field->name)))
                {
                    hlsl_error(ctx, &field->loc, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                            "Field \"%s\" is already defined.", field->name);
                    hlsl_note(ctx, &existing->loc, VKD3D_SHADER_LOG_ERROR,
                            "'%s' was previously defined here.", field->name);
                }
            }

            if (!hlsl_array_reserve(ctx, (void **)&$1.fields, &$1.capacity, $1.count + $2.count, sizeof(*$1.fields)))
                YYABORT;
            memcpy($1.fields + $1.count, $2.fields, $2.count * sizeof(*$2.fields));
            $1.count += $2.count;
            vkd3d_free($2.fields);

            $$ = $1;
        }

field_type:
      type
    | unnamed_struct_spec

field:
      var_modifiers field_type variables_def ';'
        {
            struct hlsl_type *type;
            unsigned int modifiers = $1;

            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, true, &@1)))
                YYABORT;
            if (modifiers & ~HLSL_INTERPOLATION_MODIFIERS_MASK)
            {
                struct vkd3d_string_buffer *string;

                if ((string = hlsl_modifiers_to_string(ctx, modifiers)))
                    hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                            "Modifiers '%s' are not allowed on struct fields.", string->buffer);
                hlsl_release_string_buffer(ctx, string);
            }
            if (!gen_struct_fields(ctx, &$$, type, modifiers, $3))
                YYABORT;
        }

attribute:
      '[' any_identifier ']'
        {
            if (!($$ = hlsl_alloc(ctx, offsetof(struct hlsl_attribute, args[0]))))
            {
                vkd3d_free($2);
                YYABORT;
            }
            $$->name = $2;
            hlsl_block_init(&$$->instrs);
            $$->loc = @$;
            $$->args_count = 0;
        }
    | '[' any_identifier '(' initializer_expr_list ')' ']'
        {
            unsigned int i;

            if (!($$ = hlsl_alloc(ctx, offsetof(struct hlsl_attribute, args[$4.args_count]))))
            {
                vkd3d_free($2);
                free_parse_initializer(&$4);
                YYABORT;
            }
            $$->name = $2;
            hlsl_block_init(&$$->instrs);
            hlsl_block_add_block(&$$->instrs, $4.instrs);
            vkd3d_free($4.instrs);
            $$->loc = @$;
            $$->args_count = $4.args_count;
            for (i = 0; i < $4.args_count; ++i)
                hlsl_src_from_node(&$$->args[i], $4.args[i]);
        }

attribute_list:
      attribute
        {
            $$.count = 1;
            if (!($$.attrs = hlsl_alloc(ctx, sizeof(*$$.attrs))))
            {
                hlsl_free_attribute($1);
                YYABORT;
            }
            $$.attrs[0] = $1;
        }
    | attribute_list attribute
        {
            const struct hlsl_attribute **new_array;

            $$ = $1;
            if (!(new_array = vkd3d_realloc($$.attrs, ($$.count + 1) * sizeof(*$$.attrs))))
            {
                unsigned int i;

                for (i = 0; i < $$.count; ++i)
                    hlsl_free_attribute((void *)$$.attrs[i]);
                vkd3d_free($$.attrs);
                YYABORT;
            }
            $$.attrs = new_array;
            $$.attrs[$$.count++] = $2;
        }

attribute_list_optional:
      %empty
        {
            $$.count = 0;
            $$.attrs = NULL;
        }
      | attribute_list

func_declaration:
      func_prototype compound_statement
        {
            struct hlsl_ir_function_decl *decl = $1.decl;

            if (decl->has_body)
            {
                hlsl_error(ctx, &decl->loc, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                         "Function \"%s\" is already defined.", decl->func->name);
                hlsl_note(ctx, &decl->loc, VKD3D_SHADER_LOG_ERROR,
                         "\"%s\" was previously defined here.", decl->func->name);
                destroy_block($2);
            }
            else
            {
                size_t i;

                decl->has_body = true;
                hlsl_block_add_block(&decl->body, $2);
                destroy_block($2);

                /* Semantics are taken from whichever definition has a body.
                 * We can't just replace the hlsl_ir_var pointers, though: if
                 * the function was already declared but not defined, the
                 * callers would have used the old declaration's parameters to
                 * transfer arguments. */

                if (!$1.first)
                {
                    assert(decl->parameters.count == $1.parameters.count);

                    for (i = 0; i < $1.parameters.count; ++i)
                    {
                        struct hlsl_ir_var *dst = decl->parameters.vars[i];
                        struct hlsl_ir_var *src = $1.parameters.vars[i];

                        hlsl_cleanup_semantic(&dst->semantic);
                        dst->semantic = src->semantic;
                        memset(&src->semantic, 0, sizeof(src->semantic));
                    }

                    if (decl->return_var)
                    {
                        hlsl_cleanup_semantic(&decl->return_var->semantic);
                        decl->return_var->semantic = $1.return_semantic;
                        memset(&$1.return_semantic, 0, sizeof($1.return_semantic));
                    }
                }
            }
            hlsl_pop_scope(ctx);

            if (!$1.first)
            {
                vkd3d_free($1.parameters.vars);
                hlsl_cleanup_semantic(&$1.return_semantic);
            }
        }
    | func_prototype ';'
        {
            hlsl_pop_scope(ctx);
        }

func_prototype_no_attrs:
    /* var_modifiers is necessary to avoid shift/reduce conflicts. */
      var_modifiers type var_identifier '(' parameters ')' colon_attribute
        {
            unsigned int modifiers = $1;
            struct hlsl_ir_var *var;
            struct hlsl_type *type;

            /* Functions are unconditionally inlined. */
            modifiers &= ~HLSL_MODIFIER_INLINE;

            if (modifiers & ~HLSL_MODIFIERS_MAJORITY_MASK)
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Only majority modifiers are allowed on functions.");
            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, true, &@1)))
                YYABORT;
            if ((var = hlsl_get_var(ctx->globals, $3)))
            {
                hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                        "\"%s\" is already declared as a variable.", $3);
                hlsl_note(ctx, &var->loc, VKD3D_SHADER_LOG_ERROR,
                        "\"%s\" was previously declared here.", $3);
            }
            if (hlsl_types_are_equal(type, ctx->builtin_types.Void) && $7.semantic.name)
            {
                hlsl_error(ctx, &@7, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                        "Semantics are not allowed on void functions.");
            }

            if ($7.reg_reservation.reg_type)
                FIXME("Unexpected register reservation for a function.\n");
            if ($7.reg_reservation.offset_type)
                hlsl_error(ctx, &@5, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "packoffset() is not allowed on functions.");

            if (($$.decl = hlsl_get_func_decl(ctx, $3, &$5)))
            {
                const struct hlsl_func_parameters *params = &$$.decl->parameters;
                size_t i;

                if (!hlsl_types_are_equal($2, $$.decl->return_type))
                {
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                            "\"%s\" was already declared with a different return type.", $3);
                    hlsl_note(ctx, &$$.decl->loc, VKD3D_SHADER_LOG_ERROR, "\"%s\" was previously declared here.", $3);
                }

                vkd3d_free($3);

                /* We implement function invocation by copying to input
                 * parameters, emitting a HLSL_IR_CALL instruction, then copying
                 * from output parameters. As a result, we need to use the same
                 * parameter variables for every invocation of this function,
                 * which means we use the parameters created by the first
                 * declaration. If we're not the first declaration, the
                 * parameter variables that just got created will end up being
                 * mostly ignored—in particular, they won't be used in actual
                 * IR.
                 *
                 * There is a hitch: if this is the actual definition, the
                 * function body will look up parameter variables by name. We
                 * must return the original parameters, and not the ones we just
                 * created, but we're in the wrong scope, and the parameters
                 * might not even have the same names.
                 *
                 * Therefore we need to shuffle the parameters we just created
                 * into a dummy scope where they'll never be looked up, and
                 * rename the original parameters so they have the expected
                 * names. We actually do this for every prototype: we don't know
                 * whether this is the function definition yet, but it doesn't
                 * really matter. The variables can only be used in the
                 * actual definition, and don't do anything in a declaration.
                 *
                 * This is complex, and it seems tempting to avoid this logic by
                 * putting arguments into the HLSL_IR_CALL instruction, letting
                 * the canonical variables be the ones attached to the function
                 * definition, and resolving the copies when inlining. The
                 * problem with this is output parameters. We would have to use
                 * a lot of parsing logic on already lowered IR, which is
                 * brittle and ugly.
                 */

                assert($5.count == params->count);
                for (i = 0; i < params->count; ++i)
                {
                    struct hlsl_ir_var *orig_param = params->vars[i];
                    struct hlsl_ir_var *new_param = $5.vars[i];
                    char *new_name;

                    list_remove(&orig_param->scope_entry);
                    list_add_tail(&ctx->cur_scope->vars, &orig_param->scope_entry);

                    list_remove(&new_param->scope_entry);
                    list_add_tail(&ctx->dummy_scope->vars, &new_param->scope_entry);

                    if (!(new_name = hlsl_strdup(ctx, new_param->name)))
                        YYABORT;
                    vkd3d_free((void *)orig_param->name);
                    orig_param->name = new_name;
                }

                $$.first = false;
                $$.parameters = $5;
                $$.return_semantic = $7.semantic;
            }
            else
            {
                if (!($$.decl = hlsl_new_func_decl(ctx, type, &$5, &$7.semantic, &@3)))
                    YYABORT;

                hlsl_add_function(ctx, $3, $$.decl);

                $$.first = true;
            }

            ctx->cur_function = $$.decl;
        }

func_prototype:
      func_prototype_no_attrs
    | attribute_list func_prototype_no_attrs
        {
            if ($2.first)
            {
                $2.decl->attr_count = $1.count;
                $2.decl->attrs = $1.attrs;
            }
            else
            {
                free($1.attrs);
            }
            $$ = $2;
        }

compound_statement:
      '{' '}'
        {
            if (!($$ = make_empty_block(ctx)))
                YYABORT;
        }
    | '{' scope_start statement_list '}'
        {
            hlsl_pop_scope(ctx);
            $$ = $3;
        }

scope_start:
      %empty
        {
            hlsl_push_scope(ctx);
        }

loop_scope_start:
      %empty
        {
            hlsl_push_scope(ctx);
            ctx->cur_scope->loop = true;
        }

switch_scope_start:
      %empty
        {
            hlsl_push_scope(ctx);
            ctx->cur_scope->_switch = true;
        }

var_identifier:
      VAR_IDENTIFIER
    | NEW_IDENTIFIER

colon_attribute:
      %empty
        {
            $$.semantic = (struct hlsl_semantic){0};
            $$.reg_reservation.reg_type = 0;
            $$.reg_reservation.offset_type = 0;
        }
    | semantic
        {
            $$.semantic = $1;
            $$.reg_reservation.reg_type = 0;
            $$.reg_reservation.offset_type = 0;
        }
    | register_opt
        {
            $$.semantic = (struct hlsl_semantic){0};
            $$.reg_reservation = $1;
        }
    | packoffset_opt
        {
            $$.semantic = (struct hlsl_semantic){0};
            $$.reg_reservation = $1;
        }

semantic:
      ':' any_identifier
        {
            char *p;

            for (p = $2 + strlen($2); p > $2 && isdigit(p[-1]); --p)
                ;
            $$.name = $2;
            $$.index = atoi(p);
            $$.reported_missing = false;
            $$.reported_duplicated_output_next_index = 0;
            $$.reported_duplicated_input_incompatible_next_index = 0;
            *p = 0;
        }

/* FIXME: Writemasks */
register_opt:
      ':' KW_REGISTER '(' any_identifier ')'
        {
            $$ = parse_reg_reservation($4);
            vkd3d_free($4);
        }
    | ':' KW_REGISTER '(' any_identifier ',' any_identifier ')'
        {
            FIXME("Ignoring shader target %s in a register reservation.\n", debugstr_a($4));
            vkd3d_free($4);

            $$ = parse_reg_reservation($6);
            vkd3d_free($6);
        }

packoffset_opt:
      ':' KW_PACKOFFSET '(' any_identifier ')'
        {
            $$ = parse_packoffset(ctx, $4, NULL, &@$);

            vkd3d_free($4);
        }
    | ':' KW_PACKOFFSET '(' any_identifier '.' any_identifier ')'
        {
            $$ = parse_packoffset(ctx, $4, $6, &@$);

            vkd3d_free($4);
            vkd3d_free($6);
        }

parameters:
      scope_start
        {
            memset(&$$, 0, sizeof($$));
        }
    | scope_start KW_VOID
        {
            memset(&$$, 0, sizeof($$));
        }
    | scope_start param_list
        {
            $$ = $2;
        }

param_list:
      parameter
        {
            memset(&$$, 0, sizeof($$));
            if (!add_func_parameter(ctx, &$$, &$1, &@1))
            {
                ERR("Error adding function parameter %s.\n", $1.name);
                YYABORT;
            }
        }
    | param_list ',' parameter
        {
            $$ = $1;
            if (!add_func_parameter(ctx, &$$, &$3, &@3))
            {
                hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                        "Parameter \"%s\" is already declared.", $3.name);
                YYABORT;
            }
        }

parameter:
      var_modifiers type_no_void any_identifier arrays colon_attribute
        {
            unsigned int modifiers = $1;
            struct hlsl_type *type;
            unsigned int i;

            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, true, &@1)))
                YYABORT;

            $$.modifiers = modifiers;
            if (!($$.modifiers & (HLSL_STORAGE_IN | HLSL_STORAGE_OUT)))
                $$.modifiers |= HLSL_STORAGE_IN;

            for (i = 0; i < $4.count; ++i)
            {
                if ($4.sizes[i] == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT)
                {
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Implicit size arrays not allowed in function parameters.");
                }
                type = hlsl_new_array_type(ctx, type, $4.sizes[i]);
            }
            $$.type = type;

            $$.name = $3;
            $$.semantic = $5.semantic;
            $$.reg_reservation = $5.reg_reservation;
        }

texture_type:
      KW_TEXTURE1D
        {
            $$ = HLSL_SAMPLER_DIM_1D;
        }
    | KW_TEXTURE2D
        {
            $$ = HLSL_SAMPLER_DIM_2D;
        }
    | KW_TEXTURE3D
        {
            $$ = HLSL_SAMPLER_DIM_3D;
        }
    | KW_TEXTURECUBE
        {
            $$ = HLSL_SAMPLER_DIM_CUBE;
        }
    | KW_TEXTURE1DARRAY
        {
            $$ = HLSL_SAMPLER_DIM_1DARRAY;
        }
    | KW_TEXTURE2DARRAY
        {
            $$ = HLSL_SAMPLER_DIM_2DARRAY;
        }
    | KW_TEXTURECUBEARRAY
        {
            $$ = HLSL_SAMPLER_DIM_CUBEARRAY;
        }

texture_ms_type:
      KW_TEXTURE2DMS
        {
            $$ = HLSL_SAMPLER_DIM_2DMS;
        }
    | KW_TEXTURE2DMSARRAY
        {
            $$ = HLSL_SAMPLER_DIM_2DMSARRAY;
        }

uav_type:
      KW_RWBUFFER
        {
            $$ = HLSL_SAMPLER_DIM_BUFFER;
        }
    | KW_RWSTRUCTUREDBUFFER
        {
            $$ = HLSL_SAMPLER_DIM_STRUCTURED_BUFFER;
        }
    | KW_RWTEXTURE1D
        {
            $$ = HLSL_SAMPLER_DIM_1D;
        }
    | KW_RWTEXTURE1DARRAY
        {
            $$ = HLSL_SAMPLER_DIM_1DARRAY;
        }
    | KW_RWTEXTURE2D
        {
            $$ = HLSL_SAMPLER_DIM_2D;
        }
    | KW_RWTEXTURE2DARRAY
        {
            $$ = HLSL_SAMPLER_DIM_2DARRAY;
        }
    | KW_RWTEXTURE3D
        {
            $$ = HLSL_SAMPLER_DIM_3D;
        }

type_no_void:
      KW_VECTOR '<' type ',' C_INTEGER '>'
        {
            if ($3->class != HLSL_CLASS_SCALAR)
            {
                struct vkd3d_string_buffer *string;

                string = hlsl_type_to_string(ctx, $3);
                if (string)
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Vector base type %s is not scalar.", string->buffer);
                hlsl_release_string_buffer(ctx, string);
                YYABORT;
            }
            if ($5 < 1 || $5 > 4)
            {
                hlsl_error(ctx, &@5, VKD3D_SHADER_ERROR_HLSL_INVALID_SIZE,
                        "Vector size %d is not between 1 and 4.", $5);
                YYABORT;
            }

            $$ = hlsl_type_clone(ctx, hlsl_get_vector_type(ctx, $3->base_type, $5), 0, 0);
            $$->is_minimum_precision = $3->is_minimum_precision;
        }
    | KW_VECTOR
        {
            $$ = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4);
        }
    | KW_MATRIX '<' type ',' C_INTEGER ',' C_INTEGER '>'
        {
            if ($3->class != HLSL_CLASS_SCALAR)
            {
                struct vkd3d_string_buffer *string;

                string = hlsl_type_to_string(ctx, $3);
                if (string)
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Matrix base type %s is not scalar.", string->buffer);
                hlsl_release_string_buffer(ctx, string);
                YYABORT;
            }
            if ($5 < 1 || $5 > 4)
            {
                hlsl_error(ctx, &@5, VKD3D_SHADER_ERROR_HLSL_INVALID_SIZE,
                        "Matrix row count %d is not between 1 and 4.", $5);
                YYABORT;
            }
            if ($7 < 1 || $7 > 4)
            {
                hlsl_error(ctx, &@7, VKD3D_SHADER_ERROR_HLSL_INVALID_SIZE,
                        "Matrix column count %d is not between 1 and 4.", $7);
                YYABORT;
            }

            $$ = hlsl_type_clone(ctx, hlsl_get_matrix_type(ctx, $3->base_type, $7, $5), 0, 0);
            $$->is_minimum_precision = $3->is_minimum_precision;
        }
    | KW_MATRIX
        {
            $$ = hlsl_get_matrix_type(ctx, HLSL_TYPE_FLOAT, 4, 4);
        }
    | KW_SAMPLER
        {
            $$ = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_GENERIC];
        }
    | KW_SAMPLERCOMPARISONSTATE
        {
            $$ = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_COMPARISON];
        }
    | KW_SAMPLER1D
        {
            $$ = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_1D];
        }
    | KW_SAMPLER2D
        {
            $$ = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_2D];
        }
    | KW_SAMPLER3D
        {
            $$ = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_3D];
        }
    | KW_SAMPLERCUBE
        {
            $$ = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_CUBE];
        }
    | KW_TEXTURE
        {
            $$ = hlsl_new_texture_type(ctx, HLSL_SAMPLER_DIM_GENERIC, NULL, 0);
        }
    | texture_type
        {
            $$ = hlsl_new_texture_type(ctx, $1, hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4), 0);
        }
    | texture_type '<' type '>'
        {
            validate_texture_format_type(ctx, $3, &@3);
            $$ = hlsl_new_texture_type(ctx, $1, $3, 0);
        }
    | texture_ms_type '<' type '>'
        {
            validate_texture_format_type(ctx, $3, &@3);

            if (shader_profile_version_lt(ctx, 4, 1))
            {
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Multisampled texture object declaration needs sample count for profile %s.", ctx->profile->name);
            }

            $$ = hlsl_new_texture_type(ctx, $1, $3, 0);
        }
    | texture_ms_type '<' type ',' shift_expr '>'
        {
            unsigned int sample_count;
            struct hlsl_block block;

            hlsl_block_init(&block);
            hlsl_block_add_block(&block, $5);

            sample_count = evaluate_static_expression_as_uint(ctx, &block, &@5);

            hlsl_block_cleanup(&block);

            vkd3d_free($5);

            $$ = hlsl_new_texture_type(ctx, $1, $3, sample_count);
        }
    | uav_type '<' type '>'
        {
            struct vkd3d_string_buffer *string = hlsl_type_to_string(ctx, $3);

            if (!type_contains_only_numerics($3))
            {
                if (string)
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "UAV type %s is not numeric.", string->buffer);
            }

            switch ($1)
            {
                case HLSL_SAMPLER_DIM_BUFFER:
                case HLSL_SAMPLER_DIM_1D:
                case HLSL_SAMPLER_DIM_1DARRAY:
                case HLSL_SAMPLER_DIM_2D:
                case HLSL_SAMPLER_DIM_2DARRAY:
                case HLSL_SAMPLER_DIM_3D:
                    if ($3->class == HLSL_CLASS_ARRAY)
                    {
                        if (string)
                            hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                                    "This type of UAV does not support array type.");
                    }
                    else if (hlsl_type_component_count($3) > 4)
                    {
                        if (string)
                            hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                                    "UAV data type %s size exceeds maximum size.", string->buffer);
                    }
                    break;
                case HLSL_SAMPLER_DIM_STRUCTURED_BUFFER:
                    break;
                default:
                    vkd3d_unreachable();
            }

            hlsl_release_string_buffer(ctx, string);

            $$ = hlsl_new_uav_type(ctx, $1, $3);
        }
    | TYPE_IDENTIFIER
        {
            $$ = hlsl_get_type(ctx->cur_scope, $1, true, true);
            if ($$->is_minimum_precision)
            {
                if (shader_profile_version_lt(ctx, 4, 0))
                {
                    hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Target profile doesn't support minimum-precision types.");
                }
                else
                {
                    FIXME("Reinterpreting type %s.\n", $$->name);
                }
            }
            vkd3d_free($1);
        }
    | KW_STRUCT TYPE_IDENTIFIER
        {
            $$ = hlsl_get_type(ctx->cur_scope, $2, true, true);
            if ($$->class != HLSL_CLASS_STRUCT)
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_REDEFINED, "\"%s\" redefined as a structure.", $2);
            vkd3d_free($2);
        }

type:
      type_no_void
    | KW_VOID
        {
            $$ = ctx->builtin_types.Void;
        }

declaration_statement:
      declaration
    | struct_declaration_without_vars
    | typedef
        {
            if (!($$ = make_empty_block(ctx)))
                YYABORT;
        }

typedef_type:
      type
    | struct_spec

typedef:
      KW_TYPEDEF var_modifiers typedef_type type_specs ';'
        {
            struct parse_variable_def *v, *v_next;
            unsigned int modifiers = $2;
            struct hlsl_type *type;

            if (!(type = apply_type_modifiers(ctx, $3, &modifiers, false, &@2)))
            {
                LIST_FOR_EACH_ENTRY_SAFE(v, v_next, $4, struct parse_variable_def, entry)
                    free_parse_variable_def(v);
                vkd3d_free($4);
                YYABORT;
            }

            if (modifiers)
            {
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Storage modifiers are not allowed on typedefs.");
                LIST_FOR_EACH_ENTRY_SAFE(v, v_next, $4, struct parse_variable_def, entry)
                    vkd3d_free(v);
                vkd3d_free($4);
                YYABORT;
            }
            if (!add_typedef(ctx, type, $4))
                YYABORT;
        }

type_specs:
      type_spec
        {
            if (!($$ = make_empty_list(ctx)))
                YYABORT;
            list_add_head($$, &$1->entry);
        }
    | type_specs ',' type_spec
        {
            $$ = $1;
            list_add_tail($$, &$3->entry);
        }

type_spec:
      any_identifier arrays
        {
            $$ = hlsl_alloc(ctx, sizeof(*$$));
            $$->loc = @1;
            $$->name = $1;
            $$->arrays = $2;
        }

declaration:
      variables_def_typed ';'
        {
            if (!($$ = initialize_vars(ctx, $1)))
                YYABORT;
        }

variables_def:
      variable_def
        {
            if (!($$ = make_empty_list(ctx)))
                YYABORT;
            list_add_head($$, &$1->entry);
        }
    | variables_def ',' variable_def
        {
            $$ = $1;
            list_add_tail($$, &$3->entry);
        }

variables_def_typed:
      variable_def_typed
        {
            if (!($$ = make_empty_list(ctx)))
                YYABORT;
            list_add_head($$, &$1->entry);

            declare_var(ctx, $1);
        }
    | variables_def_typed ',' variable_def
        {
            struct parse_variable_def *head_def;

            assert(!list_empty($1));
            head_def = LIST_ENTRY(list_head($1), struct parse_variable_def, entry);

            assert(head_def->basic_type);
            $3->basic_type = head_def->basic_type;
            $3->modifiers = head_def->modifiers;
            $3->modifiers_loc = head_def->modifiers_loc;

            declare_var(ctx, $3);

            $$ = $1;
            list_add_tail($$, &$3->entry);
        }

variable_decl:
      any_identifier arrays colon_attribute
        {
            $$ = hlsl_alloc(ctx, sizeof(*$$));
            $$->loc = @1;
            $$->name = $1;
            $$->arrays = $2;
            $$->semantic = $3.semantic;
            $$->reg_reservation = $3.reg_reservation;
        }

state:
      any_identifier '=' expr ';'
        {
            vkd3d_free($1);
            destroy_block($3);
        }

state_block_start:
      %empty
        {
            ctx->in_state_block = 1;
        }

state_block:
      %empty
    | state_block state

variable_def:
      variable_decl
    | variable_decl '=' complex_initializer
        {
            $$ = $1;
            $$->initializer = $3;
        }
    | variable_decl '{' state_block_start state_block '}'
        {
            $$ = $1;
            ctx->in_state_block = 0;
        }

variable_def_typed:
      var_modifiers struct_spec variable_def
        {
            unsigned int modifiers = $1;
            struct hlsl_type *type;

            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, true, &@1)))
                YYABORT;

            check_invalid_in_out_modifiers(ctx, modifiers, &@1);

            $$ = $3;
            $$->basic_type = type;
            $$->modifiers = modifiers;
            $$->modifiers_loc = @1;
        }
    | var_modifiers type variable_def
        {
            unsigned int modifiers = $1;
            struct hlsl_type *type;

            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, true, &@1)))
                YYABORT;

            check_invalid_in_out_modifiers(ctx, modifiers, &@1);

            $$ = $3;
            $$->basic_type = type;
            $$->modifiers = modifiers;
            $$->modifiers_loc = @1;
        }

arrays:
      %empty
        {
            $$.sizes = NULL;
            $$.count = 0;
        }
    | '[' expr ']' arrays
        {
            uint32_t *new_array;
            unsigned int size;

            size = evaluate_static_expression_as_uint(ctx, $2, &@2);

            destroy_block($2);

            $$ = $4;

            if (!size)
            {
                hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_INVALID_SIZE,
                        "Array size is not a positive integer constant.");
                vkd3d_free($$.sizes);
                YYABORT;
            }

            if (size > 65536)
            {
                hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_INVALID_SIZE,
                        "Array size %u is not between 1 and 65536.", size);
                vkd3d_free($$.sizes);
                YYABORT;
            }

            if (!(new_array = hlsl_realloc(ctx, $$.sizes, ($$.count + 1) * sizeof(*new_array))))
            {
                vkd3d_free($$.sizes);
                YYABORT;
            }
            $$.sizes = new_array;
            $$.sizes[$$.count++] = size;
        }
    | '[' ']' arrays
        {
            uint32_t *new_array;

            $$ = $3;

            if (!(new_array = hlsl_realloc(ctx, $$.sizes, ($$.count + 1) * sizeof(*new_array))))
            {
                vkd3d_free($$.sizes);
                YYABORT;
            }

            $$.sizes = new_array;
            $$.sizes[$$.count++] = HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT;
        }

var_modifiers:
      %empty
        {
            $$ = 0;
        }
    | KW_EXTERN var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_EXTERN, &@1);
        }
    | KW_NOINTERPOLATION var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_NOINTERPOLATION, &@1);
        }
    | KW_CENTROID var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_CENTROID, &@1);
        }
    | KW_LINEAR var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_LINEAR, &@1);
        }
    | KW_NOPERSPECTIVE var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_NOPERSPECTIVE, &@1);
        }
    | KW_PRECISE var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_PRECISE, &@1);
        }
    | KW_SHARED var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_SHARED, &@1);
        }
    | KW_GROUPSHARED var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_GROUPSHARED, &@1);
        }
    | KW_STATIC var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_STATIC, &@1);
        }
    | KW_UNIFORM var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_UNIFORM, &@1);
        }
    | KW_VOLATILE var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_VOLATILE, &@1);
        }
    | KW_CONST var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_CONST, &@1);
        }
    | KW_ROW_MAJOR var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_ROW_MAJOR, &@1);
        }
    | KW_COLUMN_MAJOR var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_COLUMN_MAJOR, &@1);
        }
    | KW_IN var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_IN, &@1);
        }
    | KW_OUT var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_OUT, &@1);
        }
    | KW_INOUT var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_IN | HLSL_STORAGE_OUT, &@1);
        }
    | KW_INLINE var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_INLINE, &@1);
        }


complex_initializer:
      initializer_expr
        {
            $$.args_count = 1;
            if (!($$.args = hlsl_alloc(ctx, sizeof(*$$.args))))
            {
                destroy_block($1);
                YYABORT;
            }
            $$.args[0] = node_from_block($1);
            $$.instrs = $1;
            $$.braces = false;
        }
    | '{' complex_initializer_list '}'
        {
            $$ = $2;
            $$.braces = true;
        }
    | '{' complex_initializer_list ',' '}'
        {
            $$ = $2;
            $$.braces = true;
        }

complex_initializer_list:
      complex_initializer
    | complex_initializer_list ',' complex_initializer
        {
            struct hlsl_ir_node **new_args;
            unsigned int i;

            $$ = $1;
            if (!(new_args = hlsl_realloc(ctx, $$.args, ($$.args_count + $3.args_count) * sizeof(*$$.args))))
            {
                free_parse_initializer(&$$);
                free_parse_initializer(&$3);
                YYABORT;
            }
            $$.args = new_args;
            for (i = 0; i < $3.args_count; ++i)
                $$.args[$$.args_count++] = $3.args[i];
            hlsl_block_add_block($$.instrs, $3.instrs);
            free_parse_initializer(&$3);
        }

initializer_expr:
      assignment_expr

initializer_expr_list:
      initializer_expr
        {
            $$.args_count = 1;
            if (!($$.args = hlsl_alloc(ctx, sizeof(*$$.args))))
            {
                destroy_block($1);
                YYABORT;
            }
            $$.args[0] = node_from_block($1);
            $$.instrs = $1;
            $$.braces = false;
        }
    | initializer_expr_list ',' initializer_expr
        {
            struct hlsl_ir_node **new_args;

            $$ = $1;
            if (!(new_args = hlsl_realloc(ctx, $$.args, ($$.args_count + 1) * sizeof(*$$.args))))
            {
                free_parse_initializer(&$$);
                destroy_block($3);
                YYABORT;
            }
            $$.args = new_args;
            $$.args[$$.args_count++] = node_from_block($3);
            hlsl_block_add_block($$.instrs, $3);
            destroy_block($3);
        }

boolean:
      KW_TRUE
        {
            $$ = true;
        }
    | KW_FALSE
        {
            $$ = false;
        }

statement_list:
      statement
    | statement_list statement
        {
            $$ = $1;
            hlsl_block_add_block($$, $2);
            destroy_block($2);
        }

statement:
      declaration_statement
    | expr_statement
    | compound_statement
    | jump_statement
    | selection_statement
    | loop_statement
    | switch_statement

jump_statement:
      KW_BREAK ';'
        {
            struct hlsl_ir_node *jump;

            if (!is_break_allowed(ctx->cur_scope))
            {
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                        "The 'break' statement must be used inside of a loop or a switch.");
            }

            if (!($$ = make_empty_block(ctx)))
                YYABORT;
            if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_BREAK, NULL, &@1)))
                YYABORT;
            hlsl_block_add_instr($$, jump);
        }
    | KW_CONTINUE ';'
        {
            struct hlsl_ir_node *jump;

            check_continue(ctx, ctx->cur_scope, &@1);

            if (!($$ = make_empty_block(ctx)))
                YYABORT;

            if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_UNRESOLVED_CONTINUE, NULL, &@1)))
                YYABORT;
            hlsl_block_add_instr($$, jump);
        }
    | KW_RETURN expr ';'
        {
            $$ = $2;
            if (!add_return(ctx, $$, node_from_block($$), &@1))
                YYABORT;
        }
    | KW_RETURN ';'
        {
            if (!($$ = make_empty_block(ctx)))
                YYABORT;
            if (!add_return(ctx, $$, NULL, &@1))
                YYABORT;
        }
    | KW_DISCARD ';'
        {
            struct hlsl_ir_node *discard, *c;

            if (!($$ = make_empty_block(ctx)))
                YYABORT;

            if (!(c = hlsl_new_uint_constant(ctx, ~0u, &@1)))
                return false;
            hlsl_block_add_instr($$, c);

            if (!(discard = hlsl_new_jump(ctx, HLSL_IR_JUMP_DISCARD_NZ, c, &@1)))
                return false;
            hlsl_block_add_instr($$, discard);
        }

selection_statement:
      attribute_list_optional KW_IF '(' expr ')' if_body
        {
            struct hlsl_ir_node *condition = node_from_block($4);
            const struct parse_attribute_list *attributes = &$1;
            struct hlsl_ir_node *instr;
            unsigned int i;

            if (attribute_list_has_duplicates(attributes))
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Found duplicate attribute.");

            for (i = 0; i < attributes->count; ++i)
            {
                const struct hlsl_attribute *attr = attributes->attrs[i];

                if (!strcmp(attr->name, "branch")
                        || !strcmp(attr->name, "flatten"))
                {
                    hlsl_warning(ctx, &@1, VKD3D_SHADER_WARNING_HLSL_IGNORED_ATTRIBUTE, "Unhandled attribute '%s'.", attr->name);
                }
                else
                {
                    hlsl_warning(ctx, &@1, VKD3D_SHADER_WARNING_HLSL_UNKNOWN_ATTRIBUTE, "Unrecognized attribute '%s'.", attr->name);
                }
            }

            if (!(instr = hlsl_new_if(ctx, condition, $6.then_block, $6.else_block, &@2)))
            {
                destroy_block($6.then_block);
                destroy_block($6.else_block);
                YYABORT;
            }
            destroy_block($6.then_block);
            destroy_block($6.else_block);
            if (condition->data_type->dimx > 1 || condition->data_type->dimy > 1)
            {
                struct vkd3d_string_buffer *string;

                if ((string = hlsl_type_to_string(ctx, condition->data_type)))
                    hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "if condition type %s is not scalar.", string->buffer);
                hlsl_release_string_buffer(ctx, string);
            }
            $$ = $4;
            hlsl_block_add_instr($$, instr);
        }

if_body:
      statement
        {
            $$.then_block = $1;
            $$.else_block = NULL;
        }
    | statement KW_ELSE statement
        {
            $$.then_block = $1;
            $$.else_block = $3;
        }

loop_statement:
      attribute_list_optional loop_scope_start KW_WHILE '(' expr ')' statement
        {
            $$ = create_loop(ctx, LOOP_WHILE, &$1, NULL, $5, NULL, $7, &@3);
            hlsl_pop_scope(ctx);
        }
    | attribute_list_optional loop_scope_start KW_DO statement KW_WHILE '(' expr ')' ';'
        {
            $$ = create_loop(ctx, LOOP_DO_WHILE, &$1, NULL, $7, NULL, $4, &@3);
            hlsl_pop_scope(ctx);
        }
    | attribute_list_optional loop_scope_start KW_FOR '(' expr_statement expr_statement expr_optional ')' statement
        {
            $$ = create_loop(ctx, LOOP_FOR, &$1, $5, $6, $7, $9, &@3);
            hlsl_pop_scope(ctx);
        }
    | attribute_list_optional loop_scope_start KW_FOR '(' declaration expr_statement expr_optional ')' statement
        {
            $$ = create_loop(ctx, LOOP_FOR, &$1, $5, $6, $7, $9, &@3);
            hlsl_pop_scope(ctx);
        }

switch_statement:
      attribute_list_optional switch_scope_start KW_SWITCH '(' expr ')' '{' switch_cases '}'
        {
            struct hlsl_ir_node *selector = node_from_block($5);
            struct hlsl_ir_node *s;

            if (!(selector = add_implicit_conversion(ctx, $5, selector, hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT), &@5)))
            {
                destroy_switch_cases($8);
                destroy_block($5);
                YYABORT;
            }

            s = hlsl_new_switch(ctx, selector, $8, &@3);

            destroy_switch_cases($8);

            if (!s)
            {
                destroy_block($5);
                YYABORT;
            }

            $$ = $5;
            hlsl_block_add_instr($$, s);

            hlsl_pop_scope(ctx);
        }

switch_case:
      KW_CASE expr ':' statement_list
        {
            struct hlsl_ir_switch_case *c;
            unsigned int value;

            value = evaluate_static_expression_as_uint(ctx, $2, &@2);

            c = hlsl_new_switch_case(ctx, value, false, $4, &@2);

            destroy_block($2);
            destroy_block($4);

            if (!c)
                YYABORT;
            $$ = c;
        }
    | KW_CASE expr ':'
        {
            struct hlsl_ir_switch_case *c;
            unsigned int value;

            value = evaluate_static_expression_as_uint(ctx, $2, &@2);

            c = hlsl_new_switch_case(ctx, value, false, NULL, &@2);

            destroy_block($2);

            if (!c)
                YYABORT;
            $$ = c;
        }
    | KW_DEFAULT ':' statement_list
        {
            struct hlsl_ir_switch_case *c;

            c = hlsl_new_switch_case(ctx, 0, true, $3, &@1);

            destroy_block($3);

            if (!c)
                YYABORT;
            $$ = c;
        }
    | KW_DEFAULT ':'
        {
            struct hlsl_ir_switch_case *c;

            if (!(c = hlsl_new_switch_case(ctx, 0, true, NULL, &@1)))
                YYABORT;
            $$ = c;
        }

switch_cases:
      switch_case
        {
            struct hlsl_ir_switch_case *c = LIST_ENTRY($1, struct hlsl_ir_switch_case, entry);
            if (!($$ = make_empty_list(ctx)))
            {
                hlsl_free_ir_switch_case(c);
                YYABORT;
            }
            list_add_head($$, &$1->entry);
        }
    | switch_cases switch_case
        {
            $$ = $1;
            check_duplicated_switch_cases(ctx, $2, $$);
            list_add_tail($$, &$2->entry);
        }

expr_optional:
      %empty
        {
            if (!($$ = make_empty_block(ctx)))
                YYABORT;
        }
    | expr

expr_statement:
      expr_optional ';'
        {
            $$ = $1;
        }

func_arguments:
      %empty
        {
            $$.args = NULL;
            $$.args_count = 0;
            if (!($$.instrs = make_empty_block(ctx)))
                YYABORT;
            $$.braces = false;
        }
    | initializer_expr_list

primary_expr:
      C_FLOAT
        {
            struct hlsl_ir_node *c;

            if (!(c = hlsl_new_float_constant(ctx, $1, &@1)))
                YYABORT;
            if (!($$ = make_block(ctx, c)))
                YYABORT;
        }
    | C_INTEGER
        {
            struct hlsl_ir_node *c;

            if (!(c = hlsl_new_int_constant(ctx, $1, &@1)))
                YYABORT;
            if (!($$ = make_block(ctx, c)))
                YYABORT;
        }
    | C_UNSIGNED
        {
            struct hlsl_ir_node *c;

            if (!(c = hlsl_new_uint_constant(ctx, $1, &@1)))
                YYABORT;
            if (!($$ = make_block(ctx, c)))
                YYABORT;
        }
    | boolean
        {
            struct hlsl_ir_node *c;

            if (!(c = hlsl_new_bool_constant(ctx, $1, &@1)))
                YYABORT;
            if (!($$ = make_block(ctx, c)))
            {
                hlsl_free_instr(c);
                YYABORT;
            }
        }
    | VAR_IDENTIFIER
        {
            struct hlsl_ir_load *load;
            struct hlsl_ir_var *var;

            if (!(var = hlsl_get_var(ctx->cur_scope, $1)))
            {
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED, "Variable \"%s\" is not defined.", $1);
                YYABORT;
            }
            if (!(load = hlsl_new_var_load(ctx, var, &@1)))
                YYABORT;
            if (!($$ = make_block(ctx, &load->node)))
                YYABORT;
        }
    | '(' expr ')'
        {
            $$ = $2;
        }
    | var_identifier '(' func_arguments ')'
        {
            if (!($$ = add_call(ctx, $1, &$3, &@1)))
            {
                vkd3d_free($1);
                YYABORT;
            }
            vkd3d_free($1);
        }
    | NEW_IDENTIFIER
        {
            if (ctx->in_state_block)
            {
                struct hlsl_ir_load *load;
                struct hlsl_ir_var *var;

                if (!(var = hlsl_new_synthetic_var(ctx, "state_block_expr",
                        hlsl_get_scalar_type(ctx, HLSL_TYPE_INT), &@1)))
                    YYABORT;
                if (!(load = hlsl_new_var_load(ctx, var, &@1)))
                    YYABORT;
                if (!($$ = make_block(ctx, &load->node)))
                    YYABORT;
            }
            else
            {
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED, "Identifier \"%s\" is not declared.", $1);
                YYABORT;
            }
        }

postfix_expr:
      primary_expr
    | postfix_expr OP_INC
        {
            if (!add_increment(ctx, $1, false, true, &@2))
            {
                destroy_block($1);
                YYABORT;
            }
            $$ = $1;
        }
    | postfix_expr OP_DEC
        {
            if (!add_increment(ctx, $1, true, true, &@2))
            {
                destroy_block($1);
                YYABORT;
            }
            $$ = $1;
        }
    | postfix_expr '.' any_identifier
        {
            struct hlsl_ir_node *node = node_from_block($1);

            if (node->data_type->class == HLSL_CLASS_STRUCT)
            {
                struct hlsl_type *type = node->data_type;
                const struct hlsl_struct_field *field;
                unsigned int field_idx = 0;

                if (!(field = get_struct_field(type->e.record.fields, type->e.record.field_count, $3)))
                {
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED, "Field \"%s\" is not defined.", $3);
                    YYABORT;
                }

                field_idx = field - type->e.record.fields;
                if (!add_record_access(ctx, $1, node, field_idx, &@2))
                    YYABORT;
                $$ = $1;
            }
            else if (hlsl_is_numeric_type(node->data_type))
            {
                struct hlsl_ir_node *swizzle;

                if (!(swizzle = get_swizzle(ctx, node, $3, &@3)))
                {
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Invalid swizzle \"%s\".", $3);
                    YYABORT;
                }
                hlsl_block_add_instr($1, swizzle);
                $$ = $1;
            }
            else
            {
                hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Invalid subscript \"%s\".", $3);
                YYABORT;
            }
        }
    | postfix_expr '[' expr ']'
        {
            struct hlsl_ir_node *array = node_from_block($1), *index = node_from_block($3);

            hlsl_block_add_block($3, $1);
            destroy_block($1);

            if (!add_array_access(ctx, $3, array, index, &@2))
            {
                destroy_block($3);
                YYABORT;
            }
            $$ = $3;
        }

    /* var_modifiers is necessary to avoid shift/reduce conflicts. */
    | var_modifiers type '(' initializer_expr_list ')'
        {
            if ($1)
            {
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Modifiers are not allowed on constructors.");
                free_parse_initializer(&$4);
                YYABORT;
            }
            if (!hlsl_is_numeric_type($2))
            {
                struct vkd3d_string_buffer *string;

                if ((string = hlsl_type_to_string(ctx, $2)))
                    hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Constructor data type %s is not numeric.", string->buffer);
                hlsl_release_string_buffer(ctx, string);
                free_parse_initializer(&$4);
                YYABORT;
            }
            if ($2->dimx * $2->dimy != initializer_size(&$4))
            {
                hlsl_error(ctx, &@4, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                        "Expected %u components in constructor, but got %u.",
                        $2->dimx * $2->dimy, initializer_size(&$4));
                free_parse_initializer(&$4);
                YYABORT;
            }

            if (!($$ = add_constructor(ctx, $2, &$4, &@2)))
            {
                free_parse_initializer(&$4);
                YYABORT;
            }
        }
    | postfix_expr '.' any_identifier '(' func_arguments ')'
        {
            struct hlsl_ir_node *object = node_from_block($1);

            hlsl_block_add_block($1, $5.instrs);
            vkd3d_free($5.instrs);

            if (!add_method_call(ctx, $1, object, $3, &$5, &@3))
            {
                destroy_block($1);
                vkd3d_free($5.args);
                YYABORT;
            }
            vkd3d_free($5.args);
            $$ = $1;
        }

unary_expr:
      postfix_expr
    | OP_INC unary_expr
        {
            if (!add_increment(ctx, $2, false, false, &@1))
            {
                destroy_block($2);
                YYABORT;
            }
            $$ = $2;
        }
    | OP_DEC unary_expr
        {
            if (!add_increment(ctx, $2, true, false, &@1))
            {
                destroy_block($2);
                YYABORT;
            }
            $$ = $2;
        }
    | '+' unary_expr
        {
            $$ = $2;
        }
    | '-' unary_expr
        {
            add_unary_arithmetic_expr(ctx, $2, HLSL_OP1_NEG, node_from_block($2), &@1);
            $$ = $2;
        }
    | '~' unary_expr
        {
            add_unary_bitwise_expr(ctx, $2, HLSL_OP1_BIT_NOT, node_from_block($2), &@1);
            $$ = $2;
        }
    | '!' unary_expr
        {
            add_unary_logical_expr(ctx, $2, HLSL_OP1_LOGIC_NOT, node_from_block($2), &@1);
            $$ = $2;
        }
    /* var_modifiers is necessary to avoid shift/reduce conflicts. */
    | '(' var_modifiers type arrays ')' unary_expr
        {
            struct hlsl_type *src_type = node_from_block($6)->data_type;
            struct hlsl_type *dst_type;
            unsigned int i;

            if ($2)
            {
                hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Modifiers are not allowed on casts.");
                YYABORT;
            }

            dst_type = $3;
            for (i = 0; i < $4.count; ++i)
            {
                if ($4.sizes[i] == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT)
                {
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Implicit size arrays not allowed in casts.");
                }
                dst_type = hlsl_new_array_type(ctx, dst_type, $4.sizes[i]);
            }

            if (!explicit_compatible_data_types(ctx, src_type, dst_type))
            {
                struct vkd3d_string_buffer *src_string, *dst_string;

                src_string = hlsl_type_to_string(ctx, src_type);
                dst_string = hlsl_type_to_string(ctx, dst_type);
                if (src_string && dst_string)
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Can't cast from %s to %s.",
                            src_string->buffer, dst_string->buffer);
                hlsl_release_string_buffer(ctx, src_string);
                hlsl_release_string_buffer(ctx, dst_string);
                YYABORT;
            }

            if (!add_cast(ctx, $6, node_from_block($6), dst_type, &@3))
            {
                destroy_block($6);
                YYABORT;
            }
            $$ = $6;
        }

mul_expr:
      unary_expr
    | mul_expr '*' unary_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_MUL, &@2);
        }
    | mul_expr '/' unary_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_DIV, &@2);
        }
    | mul_expr '%' unary_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_MOD, &@2);
        }

add_expr:
      mul_expr
    | add_expr '+' mul_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_ADD, &@2);
        }
    | add_expr '-' mul_expr
        {
            struct hlsl_ir_node *neg;

            if (!(neg = add_unary_arithmetic_expr(ctx, $3, HLSL_OP1_NEG, node_from_block($3), &@2)))
                YYABORT;
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_ADD, &@2);
        }

shift_expr:
      add_expr
    | shift_expr OP_LEFTSHIFT add_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_LSHIFT, &@2);
        }
    | shift_expr OP_RIGHTSHIFT add_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_RSHIFT, &@2);
        }

relational_expr:
      shift_expr
    | relational_expr '<' shift_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_LESS, &@2);
        }
    | relational_expr '>' shift_expr
        {
            $$ = add_binary_expr_merge(ctx, $3, $1, HLSL_OP2_LESS, &@2);
        }
    | relational_expr OP_LE shift_expr
        {
            $$ = add_binary_expr_merge(ctx, $3, $1, HLSL_OP2_GEQUAL, &@2);
        }
    | relational_expr OP_GE shift_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_GEQUAL, &@2);
        }

equality_expr:
      relational_expr
    | equality_expr OP_EQ relational_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_EQUAL, &@2);
        }
    | equality_expr OP_NE relational_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_NEQUAL, &@2);
        }

bitand_expr:
      equality_expr
    | bitand_expr '&' equality_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_BIT_AND, &@2);
        }

bitxor_expr:
      bitand_expr
    | bitxor_expr '^' bitand_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_BIT_XOR, &@2);
        }

bitor_expr:
      bitxor_expr
    | bitor_expr '|' bitxor_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_BIT_OR, &@2);
        }

logicand_expr:
      bitor_expr
    | logicand_expr OP_AND bitor_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_LOGIC_AND, &@2);
        }

logicor_expr:
      logicand_expr
    | logicor_expr OP_OR logicand_expr
        {
            $$ = add_binary_expr_merge(ctx, $1, $3, HLSL_OP2_LOGIC_OR, &@2);
        }

conditional_expr:
      logicor_expr
    | logicor_expr '?' expr ':' assignment_expr
        {
            struct hlsl_ir_node *cond = node_from_block($1);
            struct hlsl_ir_node *first = node_from_block($3);
            struct hlsl_ir_node *second = node_from_block($5);
            struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = { 0 };
            struct hlsl_type *common_type;

            hlsl_block_add_block($1, $3);
            hlsl_block_add_block($1, $5);
            destroy_block($3);
            destroy_block($5);

            if (!(common_type = get_common_numeric_type(ctx, first, second, &@3)))
                YYABORT;

            if (!(first = add_implicit_conversion(ctx, $1, first, common_type, &@3)))
                YYABORT;

            if (!(second = add_implicit_conversion(ctx, $1, second, common_type, &@5)))
                YYABORT;

            args[0] = cond;
            args[1] = first;
            args[2] = second;
            if (!add_expr(ctx, $1, HLSL_OP3_TERNARY, args, common_type, &@1))
                YYABORT;
            $$ = $1;
        }

assignment_expr:

      conditional_expr
    | unary_expr assign_op assignment_expr
        {
            struct hlsl_ir_node *lhs = node_from_block($1), *rhs = node_from_block($3);

            if (lhs->data_type->modifiers & HLSL_MODIFIER_CONST)
            {
                hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_MODIFIES_CONST, "Statement modifies a const expression.");
                YYABORT;
            }
            hlsl_block_add_block($3, $1);
            destroy_block($1);
            if (!add_assignment(ctx, $3, lhs, $2, rhs))
                YYABORT;
            $$ = $3;
        }

assign_op:
      '='
        {
            $$ = ASSIGN_OP_ASSIGN;
        }
    | OP_ADDASSIGN
        {
            $$ = ASSIGN_OP_ADD;
        }
    | OP_SUBASSIGN
        {
            $$ = ASSIGN_OP_SUB;
        }
    | OP_MULASSIGN
        {
            $$ = ASSIGN_OP_MUL;
        }
    | OP_DIVASSIGN
        {
            $$ = ASSIGN_OP_DIV;
        }
    | OP_MODASSIGN
        {
            $$ = ASSIGN_OP_MOD;
        }
    | OP_LEFTSHIFTASSIGN
        {
            $$ = ASSIGN_OP_LSHIFT;
        }
    | OP_RIGHTSHIFTASSIGN
        {
            $$ = ASSIGN_OP_RSHIFT;
        }
    | OP_ANDASSIGN
        {
            $$ = ASSIGN_OP_AND;
        }
    | OP_ORASSIGN
        {
            $$ = ASSIGN_OP_OR;
        }
    | OP_XORASSIGN
        {
            $$ = ASSIGN_OP_XOR;
        }

expr:
      assignment_expr
    | expr ',' assignment_expr
        {
            $$ = $1;
            hlsl_block_add_block($$, $3);
            destroy_block($3);
        }
