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
    struct list *instrs;
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
};

struct parse_function
{
    char *name;
    struct hlsl_ir_function_decl *decl;
};

struct parse_if_body
{
    struct list *then_instrs;
    struct list *else_instrs;
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

static struct hlsl_ir_node *node_from_list(struct list *list)
{
    return LIST_ENTRY(list_tail(list), struct hlsl_ir_node, entry);
}

static struct list *make_empty_list(struct hlsl_ctx *ctx)
{
    struct list *list;

    if ((list = hlsl_alloc(ctx, sizeof(*list))))
        list_init(list);
    return list;
}

static void destroy_instr_list(struct list *list)
{
    hlsl_free_instr_list(list);
    vkd3d_free(list);
}

static void check_invalid_matrix_modifiers(struct hlsl_ctx *ctx, DWORD modifiers, struct vkd3d_shader_location loc)
{
    if (modifiers & HLSL_MODIFIERS_MAJORITY_MASK)
        hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                "'row_major' and 'column_major' modifiers are only allowed for matrices.");
}

static bool convertible_data_type(struct hlsl_type *type)
{
    return type->type != HLSL_CLASS_OBJECT;
}

static bool compatible_data_types(struct hlsl_type *t1, struct hlsl_type *t2)
{
   if (!convertible_data_type(t1) || !convertible_data_type(t2))
        return false;

    if (t1->type <= HLSL_CLASS_LAST_NUMERIC)
    {
        /* Scalar vars can be cast to pretty much everything */
        if (t1->dimx == 1 && t1->dimy == 1)
            return true;

        if (t1->type == HLSL_CLASS_VECTOR && t2->type == HLSL_CLASS_VECTOR)
            return t1->dimx >= t2->dimx;
    }

    /* The other way around is true too i.e. whatever to scalar */
    if (t2->type <= HLSL_CLASS_LAST_NUMERIC && t2->dimx == 1 && t2->dimy == 1)
        return true;

    if (t1->type == HLSL_CLASS_ARRAY)
    {
        if (hlsl_types_are_equal(t1->e.array.type, t2))
            /* e.g. float4[3] to float4 is allowed */
            return true;

        if (t2->type == HLSL_CLASS_ARRAY || t2->type == HLSL_CLASS_STRUCT)
            return hlsl_type_component_count(t1) >= hlsl_type_component_count(t2);
        else
            return hlsl_type_component_count(t1) == hlsl_type_component_count(t2);
    }

    if (t1->type == HLSL_CLASS_STRUCT)
        return hlsl_type_component_count(t1) >= hlsl_type_component_count(t2);

    if (t2->type == HLSL_CLASS_ARRAY || t2->type == HLSL_CLASS_STRUCT)
        return hlsl_type_component_count(t1) == hlsl_type_component_count(t2);

    if (t1->type == HLSL_CLASS_MATRIX || t2->type == HLSL_CLASS_MATRIX)
    {
        if (t1->type == HLSL_CLASS_MATRIX && t2->type == HLSL_CLASS_MATRIX && t1->dimx >= t2->dimx && t1->dimy >= t2->dimy)
            return true;

        /* Matrix-vector conversion is apparently allowed if they have the same components count */
        if ((t1->type == HLSL_CLASS_VECTOR || t2->type == HLSL_CLASS_VECTOR)
                && hlsl_type_component_count(t1) == hlsl_type_component_count(t2))
            return true;
        return false;
    }

    if (hlsl_type_component_count(t1) >= hlsl_type_component_count(t2))
        return true;
    return false;
}

static bool implicit_compatible_data_types(struct hlsl_type *t1, struct hlsl_type *t2)
{
    if (!convertible_data_type(t1) || !convertible_data_type(t2))
        return false;

    if (t1->type <= HLSL_CLASS_LAST_NUMERIC)
    {
        /* Scalar vars can be converted to any other numeric data type */
        if (t1->dimx == 1 && t1->dimy == 1 && t2->type <= HLSL_CLASS_LAST_NUMERIC)
            return true;
        /* The other way around is true too */
        if (t2->dimx == 1 && t2->dimy == 1 && t2->type <= HLSL_CLASS_LAST_NUMERIC)
            return true;
    }

    if (t1->type == HLSL_CLASS_ARRAY && t2->type == HLSL_CLASS_ARRAY)
    {
        return hlsl_type_component_count(t1) == hlsl_type_component_count(t2);
    }

    if ((t1->type == HLSL_CLASS_ARRAY && t2->type <= HLSL_CLASS_LAST_NUMERIC)
            || (t1->type <= HLSL_CLASS_LAST_NUMERIC && t2->type == HLSL_CLASS_ARRAY))
    {
        /* e.g. float4[3] to float4 is allowed */
        if (t1->type == HLSL_CLASS_ARRAY && hlsl_types_are_equal(t1->e.array.type, t2))
            return true;
        if (hlsl_type_component_count(t1) == hlsl_type_component_count(t2))
            return true;
        return false;
    }

    if (t1->type <= HLSL_CLASS_VECTOR && t2->type <= HLSL_CLASS_VECTOR)
    {
        if (t1->dimx >= t2->dimx)
            return true;
        return false;
    }

    if (t1->type == HLSL_CLASS_MATRIX || t2->type == HLSL_CLASS_MATRIX)
    {
        if (t1->type == HLSL_CLASS_MATRIX && t2->type == HLSL_CLASS_MATRIX)
            return t1->dimx >= t2->dimx && t1->dimy >= t2->dimy;

        /* Matrix-vector conversion is apparently allowed if they have
         * the same components count, or if the matrix is 1xN or Nx1
         * and we are reducing the component count */
        if (t1->type == HLSL_CLASS_VECTOR || t2->type == HLSL_CLASS_VECTOR)
        {
            if (hlsl_type_component_count(t1) == hlsl_type_component_count(t2))
                return true;

            if ((t1->type == HLSL_CLASS_VECTOR || t1->dimx == 1 || t1->dimy == 1) &&
                    (t2->type == HLSL_CLASS_VECTOR || t2->dimx == 1 || t2->dimy == 1))
                return hlsl_type_component_count(t1) >= hlsl_type_component_count(t2);
        }

        return false;
    }

    if (t1->type == HLSL_CLASS_STRUCT && t2->type == HLSL_CLASS_STRUCT)
        return hlsl_types_are_equal(t1, t2);

    return false;
}

static struct hlsl_ir_load *add_load_component(struct hlsl_ctx *ctx, struct list *instrs, struct hlsl_ir_node *var_instr,
        unsigned int comp, const struct vkd3d_shader_location *loc);

static struct hlsl_ir_node *add_cast(struct hlsl_ctx *ctx, struct list *instrs,
        struct hlsl_ir_node *node, struct hlsl_type *dst_type, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *src_type = node->data_type;
    struct hlsl_ir_expr *cast;

    if (hlsl_types_are_equal(src_type, dst_type))
        return node;

    if ((src_type->type == HLSL_CLASS_MATRIX || dst_type->type == HLSL_CLASS_MATRIX)
        && src_type->type <= HLSL_CLASS_LAST_NUMERIC && dst_type->type <= HLSL_CLASS_LAST_NUMERIC)
    {
        struct vkd3d_string_buffer *name;
        static unsigned int counter = 0;
        struct hlsl_deref var_deref;
        struct hlsl_ir_load *load;
        struct hlsl_ir_var *var;
        unsigned int dst_idx;
        bool broadcast;

        broadcast = src_type->dimx == 1 && src_type->dimy == 1;
        assert(dst_type->dimx * dst_type->dimy <= src_type->dimx * src_type->dimy || broadcast);
        if (src_type->type == HLSL_CLASS_MATRIX && dst_type->type == HLSL_CLASS_MATRIX && !broadcast)
        {
            assert(dst_type->dimx <= src_type->dimx);
            assert(dst_type->dimy <= src_type->dimy);
        }

        name = vkd3d_string_buffer_get(&ctx->string_buffers);
        vkd3d_string_buffer_printf(name, "<cast-%u>", counter++);
        var = hlsl_new_synthetic_var(ctx, name->buffer, dst_type, *loc);
        vkd3d_string_buffer_release(&ctx->string_buffers, name);
        if (!var)
            return NULL;
        hlsl_init_simple_deref_from_var(&var_deref, var);

        for (dst_idx = 0; dst_idx < dst_type->dimx * dst_type->dimy; ++dst_idx)
        {
            struct hlsl_type *dst_scalar_type;
            struct hlsl_ir_store *store;
            struct hlsl_block block;
            unsigned int src_idx;

            if (broadcast)
            {
                src_idx = 0;
            }
            else
            {
                if (src_type->type == HLSL_CLASS_MATRIX && dst_type->type == HLSL_CLASS_MATRIX)
                {
                    unsigned int x = dst_idx % dst_type->dimx, y = dst_idx / dst_type->dimx;

                    src_idx = y * src_type->dimx + x;
                }
                else
                {
                    src_idx = dst_idx;
                }
            }

            dst_scalar_type = hlsl_type_get_component_type(ctx, dst_type, dst_idx);

            if (!(load = add_load_component(ctx, instrs, node, src_idx, loc)))
                return NULL;

            if (!(cast = hlsl_new_cast(ctx, &load->node, dst_scalar_type, loc)))
                return NULL;
            list_add_tail(instrs, &cast->node.entry);

            if (!(store = hlsl_new_store_component(ctx, &block, &var_deref, dst_idx, &cast->node)))
                return NULL;
            list_move_tail(instrs, &block.instrs);
        }

        if (!(load = hlsl_new_var_load(ctx, var, *loc)))
            return NULL;
        list_add_tail(instrs, &load->node.entry);

        return &load->node;
    }
    else
    {
        if (!(cast = hlsl_new_cast(ctx, node, dst_type, loc)))
            return NULL;
        list_add_tail(instrs, &cast->node.entry);
        return &cast->node;
    }
}

static struct hlsl_ir_node *add_implicit_conversion(struct hlsl_ctx *ctx, struct list *instrs,
        struct hlsl_ir_node *node, struct hlsl_type *dst_type, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *src_type = node->data_type;

    if (hlsl_types_are_equal(src_type, dst_type))
        return node;

    if (!implicit_compatible_data_types(src_type, dst_type))
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
                src_type->type == HLSL_CLASS_VECTOR ? "vector" : "matrix");

    return add_cast(ctx, instrs, node, dst_type, loc);
}

static DWORD add_modifiers(struct hlsl_ctx *ctx, DWORD modifiers, DWORD mod, const struct vkd3d_shader_location loc)
{
    if (modifiers & mod)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_modifiers_to_string(ctx, mod)))
            hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                    "Modifier '%s' was already specified.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return modifiers;
    }
    if ((mod & HLSL_MODIFIERS_MAJORITY_MASK) && (modifiers & HLSL_MODIFIERS_MAJORITY_MASK))
    {
        hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                "'row_major' and 'column_major' modifiers are mutually exclusive.");
        return modifiers;
    }
    return modifiers | mod;
}

static bool append_conditional_break(struct hlsl_ctx *ctx, struct list *cond_list)
{
    struct hlsl_ir_node *condition, *not;
    struct hlsl_ir_jump *jump;
    struct hlsl_ir_if *iff;

    /* E.g. "for (i = 0; ; ++i)". */
    if (list_empty(cond_list))
        return true;

    condition = node_from_list(cond_list);
    if (!(not = hlsl_new_unary_expr(ctx, HLSL_OP1_LOGIC_NOT, condition, condition->loc)))
        return false;
    list_add_tail(cond_list, &not->entry);

    if (!(iff = hlsl_new_if(ctx, not, condition->loc)))
        return false;
    list_add_tail(cond_list, &iff->node.entry);

    if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_BREAK, condition->loc)))
        return false;
    list_add_head(&iff->then_instrs.instrs, &jump->node.entry);
    return true;
}

enum loop_type
{
    LOOP_FOR,
    LOOP_WHILE,
    LOOP_DO_WHILE
};

static struct list *create_loop(struct hlsl_ctx *ctx, enum loop_type type, struct list *init, struct list *cond,
        struct list *iter, struct list *body, struct vkd3d_shader_location loc)
{
    struct list *list = NULL;
    struct hlsl_ir_loop *loop = NULL;
    struct hlsl_ir_if *cond_jump = NULL;

    if (!(list = make_empty_list(ctx)))
        goto oom;

    if (init)
        list_move_head(list, init);

    if (!(loop = hlsl_new_loop(ctx, loc)))
        goto oom;
    list_add_tail(list, &loop->node.entry);

    if (!append_conditional_break(ctx, cond))
        goto oom;

    if (type != LOOP_DO_WHILE)
        list_move_tail(&loop->body.instrs, cond);

    list_move_tail(&loop->body.instrs, body);

    if (iter)
        list_move_tail(&loop->body.instrs, iter);

    if (type == LOOP_DO_WHILE)
        list_move_tail(&loop->body.instrs, cond);

    vkd3d_free(init);
    vkd3d_free(cond);
    vkd3d_free(body);
    return list;

oom:
    vkd3d_free(loop);
    vkd3d_free(cond_jump);
    vkd3d_free(list);
    destroy_instr_list(init);
    destroy_instr_list(cond);
    destroy_instr_list(iter);
    destroy_instr_list(body);
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
    destroy_instr_list(initializer->instrs);
    vkd3d_free(initializer->args);
}

static struct hlsl_ir_swizzle *get_swizzle(struct hlsl_ctx *ctx, struct hlsl_ir_node *value, const char *swizzle,
        struct vkd3d_shader_location *loc)
{
    unsigned int len = strlen(swizzle), component = 0;
    unsigned int i, set, swiz = 0;
    bool valid;

    if (value->data_type->type == HLSL_CLASS_MATRIX)
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

static struct hlsl_ir_jump *add_return(struct hlsl_ctx *ctx, struct list *instrs,
        struct hlsl_ir_node *return_value, struct vkd3d_shader_location loc)
{
    struct hlsl_type *return_type = ctx->cur_function->return_type;
    struct hlsl_ir_jump *jump;

    if (return_value)
    {
        struct hlsl_ir_store *store;

        if (!(return_value = add_implicit_conversion(ctx, instrs, return_value, return_type, &loc)))
            return NULL;

        if (!(store = hlsl_new_simple_store(ctx, ctx->cur_function->return_var, return_value)))
            return NULL;
        list_add_after(&return_value->entry, &store->node.entry);
    }
    else if (ctx->cur_function->return_var)
    {
        hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RETURN, "Non-void function must return a value.");
        return NULL;
    }

    if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_RETURN, loc)))
        return NULL;
    list_add_tail(instrs, &jump->node.entry);

    return jump;
}

static struct hlsl_ir_load *add_load_index(struct hlsl_ctx *ctx, struct list *instrs, struct hlsl_ir_node *var_instr,
        struct hlsl_ir_node *idx, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_deref *src;
    struct hlsl_ir_load *load;

    if (var_instr->type == HLSL_IR_LOAD)
    {
        src = &hlsl_ir_load(var_instr)->src;
    }
    else
    {
        struct vkd3d_string_buffer *name;
        struct hlsl_ir_store *store;
        struct hlsl_ir_var *var;

        if (!(name = hlsl_get_string_buffer(ctx)))
            return NULL;
        vkd3d_string_buffer_printf(name, "<deref-%p>", var_instr);
        var = hlsl_new_synthetic_var(ctx, name->buffer, var_instr->data_type, var_instr->loc);
        hlsl_release_string_buffer(ctx, name);
        if (!var)
            return NULL;

        if (!(store = hlsl_new_simple_store(ctx, var, var_instr)))
            return NULL;
        list_add_tail(instrs, &store->node.entry);

        src = &store->lhs;
    }

    if (!(load = hlsl_new_load_index(ctx, src, idx, loc)))
        return NULL;
    list_add_tail(instrs, &load->node.entry);

    return load;
}

static struct hlsl_ir_load *add_load_component(struct hlsl_ctx *ctx, struct list *instrs, struct hlsl_ir_node *var_instr,
        unsigned int comp, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_deref *src;
    struct hlsl_ir_load *load;
    struct hlsl_block block;

    if (var_instr->type == HLSL_IR_LOAD)
    {
        src = &hlsl_ir_load(var_instr)->src;
    }
    else
    {
        struct vkd3d_string_buffer *name;
        struct hlsl_ir_store *store;
        struct hlsl_ir_var *var;

        if (!(name = hlsl_get_string_buffer(ctx)))
            return NULL;
        vkd3d_string_buffer_printf(name, "<deref-%p>", var_instr);
        var = hlsl_new_synthetic_var(ctx, name->buffer, var_instr->data_type, var_instr->loc);
        hlsl_release_string_buffer(ctx, name);
        if (!var)
            return NULL;

        if (!(store = hlsl_new_simple_store(ctx, var, var_instr)))
            return NULL;
        list_add_tail(instrs, &store->node.entry);

        src = &store->lhs;
    }

    if (!(load = hlsl_new_load_component(ctx, &block, src, comp, loc)))
        return NULL;
    list_move_tail(instrs, &block.instrs);

    return load;
}

static bool add_record_load(struct hlsl_ctx *ctx, struct list *instrs, struct hlsl_ir_node *record,
        unsigned int idx, const struct vkd3d_shader_location loc)
{
    struct hlsl_ir_constant *c;

    assert(idx < record->data_type->e.record.field_count);

    if (!(c = hlsl_new_uint_constant(ctx, idx, &loc)))
        return false;
    list_add_tail(instrs, &c->node.entry);

    return !!add_load_index(ctx, instrs, record, &c->node, &loc);
}

static struct hlsl_ir_node *add_binary_arithmetic_expr(struct hlsl_ctx *ctx, struct list *instrs,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc);

static bool add_matrix_index(struct hlsl_ctx *ctx, struct list *instrs,
        struct hlsl_ir_node *matrix, struct hlsl_ir_node *index, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *mat_type = matrix->data_type, *ret_type;
    struct vkd3d_string_buffer *name;
    static unsigned int counter = 0;
    struct hlsl_deref var_deref;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;
    unsigned int i;

    if (hlsl_type_is_row_major(mat_type))
        return add_load_index(ctx, instrs, matrix, index, loc);

    ret_type = hlsl_get_vector_type(ctx, mat_type->base_type, mat_type->dimx);

    name = vkd3d_string_buffer_get(&ctx->string_buffers);
    vkd3d_string_buffer_printf(name, "<index-%u>", counter++);
    var = hlsl_new_synthetic_var(ctx, name->buffer, ret_type, *loc);
    vkd3d_string_buffer_release(&ctx->string_buffers, name);
    if (!var)
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    for (i = 0; i < mat_type->dimx; ++i)
    {
        struct hlsl_ir_load *column, *value;
        struct hlsl_ir_store *store;
        struct hlsl_ir_constant *c;
        struct hlsl_block block;

        if (!(c = hlsl_new_uint_constant(ctx, i, loc)))
            return false;
        list_add_tail(instrs, &c->node.entry);

        if (!(column = add_load_index(ctx, instrs, matrix, &c->node, loc)))
            return false;

        if (!(value = add_load_index(ctx, instrs, &column->node, index, loc)))
            return false;

        if (!(store = hlsl_new_store_component(ctx, &block, &var_deref, i, &value->node)))
            return false;
        list_move_tail(instrs, &block.instrs);
    }

    if (!(load = hlsl_new_var_load(ctx, var, *loc)))
        return false;
    list_add_tail(instrs, &load->node.entry);

    return true;
}

static bool add_array_load(struct hlsl_ctx *ctx, struct list *instrs, struct hlsl_ir_node *array,
        struct hlsl_ir_node *index, const struct vkd3d_shader_location loc)
{
    const struct hlsl_type *expr_type = array->data_type;

    if (expr_type->type == HLSL_CLASS_MATRIX)
        return add_matrix_index(ctx, instrs, array, index, &loc);

    if (expr_type->type != HLSL_CLASS_ARRAY && expr_type->type != HLSL_CLASS_VECTOR)
    {
        if (expr_type->type == HLSL_CLASS_SCALAR)
            hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_INDEX, "Scalar expressions cannot be array-indexed.");
        else
            hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_INDEX, "Expression cannot be array-indexed.");
        return false;
    }

    if (!add_load_index(ctx, instrs, array, index, &loc))
        return false;

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
        unsigned int *modifiers, struct vkd3d_shader_location loc)
{
    unsigned int default_majority = 0;
    struct hlsl_type *new_type;

    /* This function is only used for declarations (i.e. variables and struct
     * fields), which should inherit the matrix majority. We only explicitly set
     * the default majority for declarations—typedefs depend on this—but we
     * want to always set it, so that an hlsl_type object is never used to
     * represent two different majorities (and thus can be used to store its
     * register size, etc.) */
    if (!(*modifiers & HLSL_MODIFIERS_MAJORITY_MASK)
            && !(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK)
            && type->type == HLSL_CLASS_MATRIX)
    {
        if (ctx->matrix_majority == HLSL_COLUMN_MAJOR)
            default_majority = HLSL_MODIFIER_COLUMN_MAJOR;
        else
            default_majority = HLSL_MODIFIER_ROW_MAJOR;
    }

    if (!default_majority && !(*modifiers & HLSL_TYPE_MODIFIERS_MASK))
        return type;

    if (!(new_type = hlsl_type_clone(ctx, type, default_majority, *modifiers & HLSL_TYPE_MODIFIERS_MASK)))
        return NULL;

    *modifiers &= ~HLSL_TYPE_MODIFIERS_MASK;

    if ((new_type->modifiers & HLSL_MODIFIER_ROW_MAJOR) && (new_type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR))
        hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                "'row_major' and 'column_major' modifiers are mutually exclusive.");

    return new_type;
}

static void free_parse_variable_def(struct parse_variable_def *v)
{
    free_parse_initializer(&v->initializer);
    vkd3d_free(v->arrays.sizes);
    vkd3d_free(v->name);
    vkd3d_free((void *)v->semantic.name);
    vkd3d_free(v);
}

static bool shader_is_sm_5_1(const struct hlsl_ctx *ctx)
{
    return ctx->profile->major_version == 5 && ctx->profile->minor_version >= 1;
}

static bool gen_struct_fields(struct hlsl_ctx *ctx, struct parse_fields *fields,
        struct hlsl_type *type, unsigned int modifiers, struct list *defs)
{
    struct parse_variable_def *v, *v_next;
    size_t i = 0;

    if (type->type == HLSL_CLASS_MATRIX)
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

        if (shader_is_sm_5_1(ctx) && type->type == HLSL_CLASS_OBJECT)
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
        field->modifiers = modifiers;
        if (v->initializer.args_count)
        {
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Illegal initializer on a struct field.");
            free_parse_initializer(&v->initializer);
        }
        vkd3d_free(v);
    }
    vkd3d_free(defs);
    return true;
}

static bool add_typedef(struct hlsl_ctx *ctx, DWORD modifiers, struct hlsl_type *orig_type, struct list *list)
{
    struct parse_variable_def *v, *v_next;
    struct hlsl_type *type;
    unsigned int i;
    bool ret;

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, list, struct parse_variable_def, entry)
    {
        if (!v->arrays.count)
        {
            if (!(type = hlsl_type_clone(ctx, orig_type, 0, modifiers)))
            {
                free_parse_variable_def(v);
                continue;
            }
        }
        else
        {
            type = orig_type;
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

        if (type->type != HLSL_CLASS_MATRIX)
            check_invalid_matrix_modifiers(ctx, type->modifiers, v->loc);

        if ((type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
                && (type->modifiers & HLSL_MODIFIER_ROW_MAJOR))
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                    "'row_major' and 'column_major' modifiers are mutually exclusive.");

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

static bool add_func_parameter(struct hlsl_ctx *ctx, struct list *list,
        struct parse_parameter *param, const struct vkd3d_shader_location loc)
{
    struct hlsl_ir_var *var;

    if (param->type->type == HLSL_CLASS_MATRIX)
        assert(param->type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);

    if ((param->modifiers & HLSL_STORAGE_OUT) && (param->modifiers & HLSL_STORAGE_UNIFORM))
        hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                "Parameter '%s' is declared as both \"out\" and \"uniform\".", param->name);

    if (!(var = hlsl_new_var(ctx, param->name, param->type, loc, &param->semantic, param->modifiers, &param->reg_reservation)))
        return false;
    var->is_param = 1;

    if (!hlsl_add_var(ctx, var, false))
    {
        hlsl_free_var(var);
        return false;
    }
    list_add_tail(list, &var->param_entry);
    return true;
}

static struct hlsl_reg_reservation parse_reg_reservation(const char *reg_string)
{
    struct hlsl_reg_reservation reservation = {0};

    if (!sscanf(reg_string + 1, "%u", &reservation.index))
    {
        FIXME("Unsupported register reservation syntax.\n");
        return reservation;
    }
    reservation.type = reg_string[0];
    return reservation;
}

static const struct hlsl_ir_function_decl *get_func_decl(struct rb_tree *funcs, char *name, struct list *params)
{
    struct hlsl_ir_function *func;
    struct rb_entry *entry;

    if ((entry = rb_get(funcs, name)))
    {
        func = RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);

        if ((entry = rb_get(&func->overloads, params)))
            return RB_ENTRY_VALUE(entry, struct hlsl_ir_function_decl, entry);
    }
    return NULL;
}

static struct list *make_list(struct hlsl_ctx *ctx, struct hlsl_ir_node *node)
{
    struct list *list;

    if (!(list = make_empty_list(ctx)))
    {
        hlsl_free_instr(node);
        return NULL;
    }
    list_add_tail(list, &node->entry);
    return list;
}

static unsigned int evaluate_array_dimension(struct hlsl_ir_node *node)
{
    if (node->data_type->type != HLSL_CLASS_SCALAR)
        return 0;

    switch (node->type)
    {
        case HLSL_IR_CONSTANT:
        {
            struct hlsl_ir_constant *constant = hlsl_ir_constant(node);
            const union hlsl_constant_value *value = &constant->value[0];

            switch (constant->node.data_type->base_type)
            {
                case HLSL_TYPE_UINT:
                    return value->u;
                case HLSL_TYPE_INT:
                    return value->i;
                case HLSL_TYPE_FLOAT:
                case HLSL_TYPE_HALF:
                    return value->f;
                case HLSL_TYPE_DOUBLE:
                    return value->d;
                case HLSL_TYPE_BOOL:
                    return !!value->u;
                default:
                    assert(0);
                    return 0;
            }
        }

        case HLSL_IR_EXPR:
        case HLSL_IR_LOAD:
        case HLSL_IR_RESOURCE_LOAD:
        case HLSL_IR_SWIZZLE:
            FIXME("Unhandled type %s.\n", hlsl_node_type_to_string(node->type));
            return 0;

        case HLSL_IR_IF:
        case HLSL_IR_JUMP:
        case HLSL_IR_LOOP:
        case HLSL_IR_STORE:
            WARN("Invalid node type %s.\n", hlsl_node_type_to_string(node->type));
            return 0;
    }

    assert(0);
    return 0;
}

static bool expr_compatible_data_types(struct hlsl_type *t1, struct hlsl_type *t2)
{
    if (t1->base_type > HLSL_TYPE_LAST_SCALAR || t2->base_type > HLSL_TYPE_LAST_SCALAR)
        return false;

    /* Scalar vars can be converted to pretty much everything */
    if ((t1->dimx == 1 && t1->dimy == 1) || (t2->dimx == 1 && t2->dimy == 1))
        return true;

    if (t1->type == HLSL_CLASS_VECTOR && t2->type == HLSL_CLASS_VECTOR)
        return true;

    if (t1->type == HLSL_CLASS_MATRIX || t2->type == HLSL_CLASS_MATRIX)
    {
        /* Matrix-vector conversion is apparently allowed if either they have the same components
           count or the matrix is nx1 or 1xn */
        if (t1->type == HLSL_CLASS_VECTOR || t2->type == HLSL_CLASS_VECTOR)
        {
            if (hlsl_type_component_count(t1) == hlsl_type_component_count(t2))
                return true;

            return (t1->type == HLSL_CLASS_MATRIX && (t1->dimx == 1 || t1->dimy == 1))
                    || (t2->type == HLSL_CLASS_MATRIX && (t2->dimx == 1 || t2->dimy == 1));
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
    if (t1->type > HLSL_CLASS_LAST_NUMERIC)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, t1)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Expression of type \"%s\" cannot be used in a numeric expression.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    if (t2->type > HLSL_CLASS_LAST_NUMERIC)
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
        *type = t2->type;
        *dimx = t2->dimx;
        *dimy = t2->dimy;
    }
    else if (t2->dimx == 1 && t2->dimy == 1)
    {
        *type = t1->type;
        *dimx = t1->dimx;
        *dimy = t1->dimy;
    }
    else if (t1->type == HLSL_CLASS_MATRIX && t2->type == HLSL_CLASS_MATRIX)
    {
        *type = HLSL_CLASS_MATRIX;
        *dimx = min(t1->dimx, t2->dimx);
        *dimy = min(t1->dimy, t2->dimy);
    }
    else
    {
        if (t1->dimx * t1->dimy <= t2->dimx * t2->dimy)
        {
            *type = t1->type;
            *dimx = t1->dimx;
            *dimy = t1->dimy;
        }
        else
        {
            *type = t2->type;
            *dimx = t2->dimx;
            *dimy = t2->dimy;
        }
    }

    return true;
}

static struct hlsl_ir_node *add_expr(struct hlsl_ctx *ctx, struct list *instrs,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS],
        struct hlsl_type *type, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (type->type == HLSL_CLASS_MATRIX)
    {
        struct vkd3d_string_buffer *name;
        static unsigned int counter = 0;
        struct hlsl_type *vector_type;
        struct hlsl_deref var_deref;
        struct hlsl_ir_load *load;
        struct hlsl_ir_var *var;

        vector_type = hlsl_get_vector_type(ctx, type->base_type, hlsl_type_minor_size(type));

        name = vkd3d_string_buffer_get(&ctx->string_buffers);
        vkd3d_string_buffer_printf(name, "<split_op-%u>", counter++);
        var = hlsl_new_synthetic_var(ctx, name->buffer, type, *loc);
        vkd3d_string_buffer_release(&ctx->string_buffers, name);
        if (!var)
            return NULL;
        hlsl_init_simple_deref_from_var(&var_deref, var);

        for (i = 0; i < hlsl_type_major_size(type); ++i)
        {
            struct hlsl_ir_node *value, *vector_operands[HLSL_MAX_OPERANDS] = { NULL };
            struct hlsl_ir_store *store;
            struct hlsl_ir_constant *c;
            unsigned int j;

            if (!(c = hlsl_new_uint_constant(ctx, i, loc)))
                return NULL;
            list_add_tail(instrs, &c->node.entry);

            for (j = 0; j < HLSL_MAX_OPERANDS; j++)
            {
                if (operands[j])
                {
                    struct hlsl_ir_load *load;

                    if (!(load = add_load_index(ctx, instrs, operands[j], &c->node, loc)))
                        return NULL;
                    vector_operands[j] = &load->node;
                }
            }

            if (!(value = add_expr(ctx, instrs, op, vector_operands, vector_type, loc)))
                return NULL;

            if (!(store = hlsl_new_store_index(ctx, &var_deref, &c->node, value, 0, loc)))
                return NULL;
            list_add_tail(instrs, &store->node.entry);
        }

        if (!(load = hlsl_new_var_load(ctx, var, *loc)))
            return NULL;
        list_add_tail(instrs, &load->node.entry);

        return &load->node;
    }

    if (!(expr = hlsl_alloc(ctx, sizeof(*expr))))
        return NULL;
    init_node(&expr->node, HLSL_IR_EXPR, type, *loc);
    expr->op = op;
    for (i = 0; i < HLSL_MAX_OPERANDS; ++i)
        hlsl_src_from_node(&expr->operands[i], operands[i]);
    list_add_tail(instrs, &expr->node.entry);

    return &expr->node;
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

static struct hlsl_ir_node *add_unary_arithmetic_expr(struct hlsl_ctx *ctx, struct list *instrs,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {arg};

    return add_expr(ctx, instrs, op, args, arg->data_type, loc);
}

static struct hlsl_ir_node *add_unary_bitwise_expr(struct hlsl_ctx *ctx, struct list *instrs,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    check_integer_type(ctx, arg);

    return add_unary_arithmetic_expr(ctx, instrs, op, arg, loc);
}

static struct hlsl_ir_node *add_unary_logical_expr(struct hlsl_ctx *ctx, struct list *instrs,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *bool_type;

    bool_type = hlsl_get_numeric_type(ctx, arg->data_type->type, HLSL_TYPE_BOOL,
            arg->data_type->dimx, arg->data_type->dimy);

    if (!(args[0] = add_implicit_conversion(ctx, instrs, arg, bool_type, loc)))
        return NULL;

    return add_expr(ctx, instrs, op, args, bool_type, loc);
}

static struct hlsl_ir_node *add_binary_arithmetic_expr(struct hlsl_ctx *ctx, struct list *instrs,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *common_type;
    enum hlsl_base_type base = expr_common_base_type(arg1->data_type->base_type, arg2->data_type->base_type);
    enum hlsl_type_class type;
    unsigned int dimx, dimy;
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};

    if (!expr_common_shape(ctx, arg1->data_type, arg2->data_type, loc, &type, &dimx, &dimy))
        return NULL;

    common_type = hlsl_get_numeric_type(ctx, type, base, dimx, dimy);

    if (!(args[0] = add_implicit_conversion(ctx, instrs, arg1, common_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, instrs, arg2, common_type, loc)))
        return NULL;

    return add_expr(ctx, instrs, op, args, common_type, loc);
}

static struct list *add_binary_arithmetic_expr_merge(struct hlsl_ctx *ctx, struct list *list1, struct list *list2,
        enum hlsl_ir_expr_op op, struct vkd3d_shader_location loc)
{
    struct hlsl_ir_node *arg1 = node_from_list(list1), *arg2 = node_from_list(list2);

    list_move_tail(list1, list2);
    vkd3d_free(list2);
    add_binary_arithmetic_expr(ctx, list1, op, arg1, arg2, &loc);
    return list1;
}

static struct hlsl_ir_node *add_binary_bitwise_expr(struct hlsl_ctx *ctx, struct list *instrs,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        const struct vkd3d_shader_location *loc)
{
    check_integer_type(ctx, arg1);
    check_integer_type(ctx, arg2);

    return add_binary_arithmetic_expr(ctx, instrs, op, arg1, arg2, loc);
}

static struct list *add_binary_bitwise_expr_merge(struct hlsl_ctx *ctx, struct list *list1, struct list *list2,
        enum hlsl_ir_expr_op op, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg1 = node_from_list(list1), *arg2 = node_from_list(list2);

    list_move_tail(list1, list2);
    vkd3d_free(list2);
    add_binary_bitwise_expr(ctx, list1, op, arg1, arg2, loc);

    return list1;
}

static struct hlsl_ir_node *add_binary_comparison_expr(struct hlsl_ctx *ctx, struct list *instrs,
        enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2,
        struct vkd3d_shader_location *loc)
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

    if (!(args[0] = add_implicit_conversion(ctx, instrs, arg1, common_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, instrs, arg2, common_type, loc)))
        return NULL;

    return add_expr(ctx, instrs, op, args, return_type, loc);
}

static struct list *add_binary_comparison_expr_merge(struct hlsl_ctx *ctx, struct list *list1, struct list *list2,
        enum hlsl_ir_expr_op op, struct vkd3d_shader_location loc)
{
    struct hlsl_ir_node *arg1 = node_from_list(list1), *arg2 = node_from_list(list2);

    list_move_tail(list1, list2);
    vkd3d_free(list2);
    add_binary_comparison_expr(ctx, list1, op, arg1, arg2, &loc);
    return list1;
}

static struct hlsl_ir_node *add_binary_logical_expr(struct hlsl_ctx *ctx, struct list *instrs,
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

    if (!(args[0] = add_implicit_conversion(ctx, instrs, arg1, common_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, instrs, arg2, common_type, loc)))
        return NULL;

    return add_expr(ctx, instrs, op, args, common_type, loc);
}

static struct list *add_binary_logical_expr_merge(struct hlsl_ctx *ctx, struct list *list1, struct list *list2,
        enum hlsl_ir_expr_op op, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg1 = node_from_list(list1), *arg2 = node_from_list(list2);

    list_move_tail(list1, list2);
    vkd3d_free(list2);
    add_binary_logical_expr(ctx, list1, op, arg1, arg2, loc);

    return list1;
}

static struct hlsl_ir_node *add_binary_shift_expr(struct hlsl_ctx *ctx, struct list *instrs,
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

    if (!(args[0] = add_implicit_conversion(ctx, instrs, arg1, return_type, loc)))
        return NULL;

    if (!(args[1] = add_implicit_conversion(ctx, instrs, arg2, integer_type, loc)))
        return NULL;

    return add_expr(ctx, instrs, op, args, return_type, loc);
}

static struct list *add_binary_shift_expr_merge(struct hlsl_ctx *ctx, struct list *list1, struct list *list2,
        enum hlsl_ir_expr_op op, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg1 = node_from_list(list1), *arg2 = node_from_list(list2);

    list_move_tail(list1, list2);
    vkd3d_free(list2);
    add_binary_shift_expr(ctx, list1, op, arg1, arg2, loc);

    return list1;
}

static struct hlsl_ir_node *add_binary_dot_expr(struct hlsl_ctx *ctx, struct list *instrs,
        struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2, const struct vkd3d_shader_location *loc)
{
    enum hlsl_base_type base = expr_common_base_type(arg1->data_type->base_type, arg2->data_type->base_type);
    struct hlsl_ir_node *args[HLSL_MAX_OPERANDS] = {0};
    struct hlsl_type *common_type, *ret_type;
    enum hlsl_ir_expr_op op;
    unsigned dim;

    if (arg1->data_type->type == HLSL_CLASS_MATRIX)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, arg1->data_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Invalid type %s.\n", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return NULL;
    }

    if (arg2->data_type->type == HLSL_CLASS_MATRIX)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, arg2->data_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Invalid type %s.\n", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return NULL;
    }

    if (arg1->data_type->type == HLSL_CLASS_SCALAR)
        dim = arg2->data_type->dimx;
    else if (arg1->data_type->type == HLSL_CLASS_SCALAR)
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

static struct hlsl_ir_node *add_assignment(struct hlsl_ctx *ctx, struct list *instrs, struct hlsl_ir_node *lhs,
        enum parse_assign_op assign_op, struct hlsl_ir_node *rhs)
{
    struct hlsl_type *lhs_type = lhs->data_type;
    struct hlsl_ir_store *store;
    struct hlsl_ir_expr *copy;
    unsigned int writemask = 0;

    if (assign_op == ASSIGN_OP_SUB)
    {
        if (!(rhs = add_unary_arithmetic_expr(ctx, instrs, HLSL_OP1_NEG, rhs, &rhs->loc)))
            return NULL;
        assign_op = ASSIGN_OP_ADD;
    }
    if (assign_op != ASSIGN_OP_ASSIGN)
    {
        enum hlsl_ir_expr_op op = op_from_assignment(assign_op);

        assert(op);
        if (!(rhs = add_binary_arithmetic_expr(ctx, instrs, op, lhs, rhs, &rhs->loc)))
            return NULL;
    }

    if (lhs_type->type <= HLSL_CLASS_LAST_NUMERIC)
    {
        writemask = (1 << lhs_type->dimx) - 1;

        if (!(rhs = add_implicit_conversion(ctx, instrs, rhs, lhs_type, &rhs->loc)))
            return NULL;
    }

    while (lhs->type != HLSL_IR_LOAD)
    {
        if (lhs->type == HLSL_IR_EXPR && hlsl_ir_expr(lhs)->op == HLSL_OP1_CAST)
        {
            hlsl_fixme(ctx, &lhs->loc, "Cast on the LHS.");
            return NULL;
        }
        else if (lhs->type == HLSL_IR_SWIZZLE)
        {
            struct hlsl_ir_swizzle *swizzle = hlsl_ir_swizzle(lhs), *new_swizzle;
            unsigned int width, s = swizzle->swizzle;

            if (lhs->data_type->type == HLSL_CLASS_MATRIX)
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
            list_add_tail(instrs, &new_swizzle->node.entry);

            lhs = swizzle->val.node;
            rhs = &new_swizzle->node;
        }
        else
        {
            hlsl_error(ctx, &lhs->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_LVALUE, "Invalid lvalue.");
            return NULL;
        }
    }

    if (!(store = hlsl_new_store_index(ctx, &hlsl_ir_load(lhs)->src, NULL, rhs, writemask, &rhs->loc)))
        return NULL;
    list_add_tail(instrs, &store->node.entry);

    /* Don't use the instruction itself as a source, as this makes structure
     * splitting easier. Instead copy it here. Since we retrieve sources from
     * the last instruction in the list, we do need to copy. */
    if (!(copy = hlsl_new_copy(ctx, rhs)))
        return NULL;
    list_add_tail(instrs, &copy->node.entry);
    return &copy->node;
}

static bool add_increment(struct hlsl_ctx *ctx, struct list *instrs, bool decrement, bool post,
        struct vkd3d_shader_location loc)
{
    struct hlsl_ir_node *lhs = node_from_list(instrs);
    struct hlsl_ir_constant *one;

    if (lhs->data_type->modifiers & HLSL_MODIFIER_CONST)
        hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_MODIFIES_CONST,
                "Argument to %s%screment operator is const.", post ? "post" : "pre", decrement ? "de" : "in");

    if (!(one = hlsl_new_int_constant(ctx, 1, &loc)))
        return false;
    list_add_tail(instrs, &one->node.entry);

    if (!add_assignment(ctx, instrs, lhs, decrement ? ASSIGN_OP_SUB : ASSIGN_OP_ADD, &one->node))
        return false;

    if (post)
    {
        struct hlsl_ir_expr *copy;

        if (!(copy = hlsl_new_copy(ctx, lhs)))
            return false;
        list_add_tail(instrs, &copy->node.entry);

        /* Post increment/decrement expressions are considered const. */
        if (!(copy->node.data_type = hlsl_type_clone(ctx, copy->node.data_type, 0, HLSL_MODIFIER_CONST)))
            return false;
    }

    return true;
}

static void initialize_var_components(struct hlsl_ctx *ctx, struct list *instrs,
        struct hlsl_ir_var *dst, unsigned int *store_index, struct hlsl_ir_node *src)
{
    unsigned int src_comp_count = hlsl_type_component_count(src->data_type);
    struct hlsl_deref dst_deref;
    unsigned int k;

    hlsl_init_simple_deref_from_var(&dst_deref, dst);

    for (k = 0; k < src_comp_count; ++k)
    {
        struct hlsl_type *dst_comp_type;
        struct hlsl_ir_store *store;
        struct hlsl_ir_load *load;
        struct hlsl_ir_node *conv;
        struct hlsl_block block;

        if (!(load = add_load_component(ctx, instrs, src, k, &src->loc)))
            return;

        dst_comp_type = hlsl_type_get_component_type(ctx, dst->data_type, *store_index);

        if (!(conv = add_implicit_conversion(ctx, instrs, &load->node, dst_comp_type, &src->loc)))
            return;

        if (!(store = hlsl_new_store_component(ctx, &block, &dst_deref, *store_index, conv)))
            return;
        list_move_tail(instrs, &block.instrs);

        ++*store_index;
    }
}

static struct list *declare_vars(struct hlsl_ctx *ctx, struct hlsl_type *basic_type,
        DWORD modifiers, struct list *var_list)
{
    struct parse_variable_def *v, *v_next;
    struct hlsl_ir_function_decl *func;
    struct list *statements_list;
    struct hlsl_ir_var *var;
    struct hlsl_type *type;
    bool local = true;

    if (basic_type->type == HLSL_CLASS_MATRIX)
        assert(basic_type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);

    if (!(statements_list = make_empty_list(ctx)))
    {
        LIST_FOR_EACH_ENTRY_SAFE(v, v_next, var_list, struct parse_variable_def, entry)
            free_parse_variable_def(v);
        vkd3d_free(var_list);
        return NULL;
    }

    if (!var_list)
        return statements_list;

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, var_list, struct parse_variable_def, entry)
    {
        bool unbounded_res_array = false;
        unsigned int i;

        type = basic_type;

        if (shader_is_sm_5_1(ctx) && type->type == HLSL_CLASS_OBJECT)
        {
            for (i = 0; i < v->arrays.count; ++i)
                unbounded_res_array |= (v->arrays.sizes[i] == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT);
        }

        if (unbounded_res_array)
        {
            if (v->arrays.count == 1)
            {
                hlsl_fixme(ctx, &v->loc, "Unbounded resource arrays.");
                free_parse_variable_def(v);
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
                        free_parse_initializer(&v->initializer);
                        v->initializer.args_count = 0;
                    }
                    else if (elem_components == 0)
                    {
                        hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                                "Cannot declare an implicit size array of a size 0 type.");
                        free_parse_initializer(&v->initializer);
                        v->initializer.args_count = 0;
                    }
                    else if (size == 0)
                    {
                        hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                                "Implicit size arrays need to be initialized.");
                        free_parse_initializer(&v->initializer);
                        v->initializer.args_count = 0;

                    }
                    else if (size % elem_components != 0)
                    {
                        hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                                "Cannot initialize implicit size array with %u components, expected a multiple of %u.",
                                size, elem_components);
                        free_parse_initializer(&v->initializer);
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
        vkd3d_free(v->arrays.sizes);

        if (type->type != HLSL_CLASS_MATRIX)
            check_invalid_matrix_modifiers(ctx, modifiers, v->loc);

        if (modifiers & (HLSL_STORAGE_IN | HLSL_STORAGE_OUT))
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_modifiers_to_string(ctx, modifiers & (HLSL_STORAGE_IN | HLSL_STORAGE_OUT))))
                hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Modifiers '%s' are not allowed on non-parameter variables.", string->buffer);
            hlsl_release_string_buffer(ctx, string);
        }

        if (!(var = hlsl_new_var(ctx, v->name, type, v->loc, &v->semantic, modifiers, &v->reg_reservation)))
        {
            free_parse_variable_def(v);
            continue;
        }

        var->buffer = ctx->cur_buffer;

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
                var->modifiers |= HLSL_STORAGE_UNIFORM;

            if ((func = hlsl_get_func_decl(ctx, var->name)))
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
        }

        if ((type->modifiers & HLSL_MODIFIER_CONST) && !v->initializer.args_count
                && !(modifiers & (HLSL_STORAGE_STATIC | HLSL_STORAGE_UNIFORM)))
        {
            hlsl_error(ctx, &v->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_INITIALIZER,
                    "Const variable \"%s\" is missing an initializer.", var->name);
            hlsl_free_var(var);
            free_parse_initializer(&v->initializer);
            vkd3d_free(v);
            continue;
        }

        if (!hlsl_add_var(ctx, var, local))
        {
            struct hlsl_ir_var *old = hlsl_get_var(ctx->cur_scope, var->name);

            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                    "Variable \"%s\" was already declared in this scope.", var->name);
            hlsl_note(ctx, &old->loc, VKD3D_SHADER_LOG_ERROR, "\"%s\" was previously declared here.", old->name);
            hlsl_free_var(var);
            free_parse_initializer(&v->initializer);
            vkd3d_free(v);
            continue;
        }

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
                    free_parse_initializer(&v->initializer);
                    vkd3d_free(v);
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
                struct hlsl_ir_load *load = hlsl_new_var_load(ctx, var, var->loc);

                assert(v->initializer.args_count == 1);
                list_add_tail(v->initializer.instrs, &load->node.entry);
                add_assignment(ctx, v->initializer.instrs, &load->node, ASSIGN_OP_ASSIGN, v->initializer.args[0]);
            }

            if (modifiers & HLSL_STORAGE_STATIC)
                list_move_tail(&ctx->static_initializers, v->initializer.instrs);
            else
                list_move_tail(statements_list, v->initializer.instrs);
            vkd3d_free(v->initializer.args);
            vkd3d_free(v->initializer.instrs);
        }
        vkd3d_free(v);
    }
    vkd3d_free(var_list);
    return statements_list;
}

struct find_function_call_args
{
    const struct parse_initializer *params;
    const struct hlsl_ir_function_decl *decl;
};

static void find_function_call_exact(struct rb_entry *entry, void *context)
{
    const struct hlsl_ir_function_decl *decl = RB_ENTRY_VALUE(entry, const struct hlsl_ir_function_decl, entry);
    struct find_function_call_args *args = context;
    const struct hlsl_ir_var *param;
    unsigned int i = 0;

    LIST_FOR_EACH_ENTRY(param, decl->parameters, struct hlsl_ir_var, param_entry)
    {
        if (i >= args->params->args_count
                || !hlsl_types_are_equal(param->data_type, args->params->args[i++]->data_type))
            return;
    }
    if (i == args->params->args_count)
        args->decl = decl;
}

static const struct hlsl_ir_function_decl *find_function_call(struct hlsl_ctx *ctx,
        const char *name, const struct parse_initializer *params)
{
    struct find_function_call_args args = {.params = params};
    struct hlsl_ir_function *func;
    struct rb_entry *entry;

    if (!(entry = rb_get(&ctx->functions, name)))
        return NULL;
    func = RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);

    rb_for_each_entry(&func->overloads, find_function_call_exact, &args);
    if (!args.decl)
        FIXME("Search for compatible overloads.\n");
    return args.decl;
}

static struct hlsl_ir_node *intrinsic_float_convert_arg(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *type = arg->data_type;

    if (type->base_type == HLSL_TYPE_FLOAT || type->base_type == HLSL_TYPE_HALF)
        return arg;

    type = hlsl_get_numeric_type(ctx, type->type, HLSL_TYPE_FLOAT, type->dimx, type->dimy);
    return add_implicit_conversion(ctx, params->instrs, arg, type, loc);
}

static bool intrinsic_abs(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_ABS, params->args[0], loc);
}

static bool intrinsic_clamp(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *max;

    if (!(max = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MAX, params->args[0], params->args[1], loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MIN, max, params->args[2], loc);
}

static bool intrinsic_cross(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_swizzle *arg1_swzl1, *arg1_swzl2, *arg2_swzl1, *arg2_swzl2;
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
    list_add_tail(params->instrs, &arg1_swzl1->node.entry);

    if (!(arg2_swzl1 = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Y, Z, X, Y), 3, arg2_cast, loc)))
        return false;
    list_add_tail(params->instrs, &arg2_swzl1->node.entry);

    if (!(mul1 = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL,
            &arg1_swzl1->node, &arg2_swzl1->node, loc)))
        return false;

    if (!(mul1_neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, mul1, *loc)))
        return false;
    list_add_tail(params->instrs, &mul1_neg->entry);

    if (!(arg1_swzl2 = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Y, Z, X, Y), 3, arg1_cast, loc)))
        return false;
    list_add_tail(params->instrs, &arg1_swzl2->node.entry);

    if (!(arg2_swzl2 = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Z, X, Y, Z), 3, arg2_cast, loc)))
        return false;
    list_add_tail(params->instrs, &arg2_swzl2->node.entry);

    if (!(mul2 = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL,
            &arg1_swzl2->node, &arg2_swzl2->node, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, mul2, mul1_neg, loc);
}

static bool intrinsic_dot(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return !!add_binary_dot_expr(ctx, params->instrs, params->args[0], params->args[1], loc);
}

static bool intrinsic_floor(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_FLOOR, arg, loc);
}

static bool intrinsic_ldexp(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[1], loc)))
        return false;

    if (!(arg = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_EXP2, arg, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, params->args[0], arg, loc);
}

static bool intrinsic_lerp(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg, *neg, *add, *mul;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    if (!(neg = add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_NEG, arg, loc)))
        return false;

    if (!(add = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, params->args[1], neg, loc)))
        return false;

    if (!(mul = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, params->args[2], add, loc)))
        return false;

    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_ADD, arg, mul, loc);
}

static bool intrinsic_max(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MAX, params->args[0], params->args[1], loc);
}

static bool intrinsic_min(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MIN, params->args[0], params->args[1], loc);
}

static bool intrinsic_mul(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg1 = params->args[0], *arg2 = params->args[1], *cast1, *cast2;
    enum hlsl_base_type base = expr_common_base_type(arg1->data_type->base_type, arg2->data_type->base_type);
    struct hlsl_type *cast_type1 = arg1->data_type, *cast_type2 = arg2->data_type, *matrix_type, *ret_type;
    unsigned int i, j, k, vect_count = 0;
    struct vkd3d_string_buffer *name;
    static unsigned int counter = 0;
    struct hlsl_deref var_deref;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;

    if (arg1->data_type->type == HLSL_CLASS_SCALAR || arg2->data_type->type == HLSL_CLASS_SCALAR)
        return !!add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, arg1, arg2, loc);

    if (arg1->data_type->type == HLSL_CLASS_VECTOR)
    {
        vect_count++;
        cast_type1 = hlsl_get_matrix_type(ctx, base, arg1->data_type->dimx, 1);
    }
    if (arg2->data_type->type == HLSL_CLASS_VECTOR)
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

    name = vkd3d_string_buffer_get(&ctx->string_buffers);
    vkd3d_string_buffer_printf(name, "<mul-%u>", counter++);
    var = hlsl_new_synthetic_var(ctx, name->buffer, matrix_type, *loc);
    vkd3d_string_buffer_release(&ctx->string_buffers, name);
    if (!var)
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    for (i = 0; i < matrix_type->dimx; ++i)
    {
        for (j = 0; j < matrix_type->dimy; ++j)
        {
            struct hlsl_ir_node *instr = NULL;
            struct hlsl_ir_store *store;
            struct hlsl_block block;

            for (k = 0; k < cast_type1->dimx && k < cast_type2->dimy; ++k)
            {
                struct hlsl_ir_load *value1, *value2;
                struct hlsl_ir_node *mul;

                if (!(value1 = add_load_component(ctx, params->instrs, cast1, j * cast1->data_type->dimx + k, loc)))
                    return false;

                if (!(value2 = add_load_component(ctx, params->instrs, cast2, k * cast2->data_type->dimx + i, loc)))
                    return false;

                if (!(mul = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, &value1->node, &value2->node, loc)))
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

            if (!(store = hlsl_new_store_component(ctx, &block, &var_deref, j * matrix_type->dimx + i, instr)))
                return false;
            list_move_tail(params->instrs, &block.instrs);
        }
    }

    if (!(load = hlsl_new_var_load(ctx, var, *loc)))
        return false;
    list_add_tail(params->instrs, &load->node.entry);

    return !!add_implicit_conversion(ctx, params->instrs, &load->node, ret_type, loc);
}

static bool intrinsic_pow(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *log, *exp, *arg, *mul;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    if (!(log = hlsl_new_unary_expr(ctx, HLSL_OP1_LOG2, arg, *loc)))
        return false;
    list_add_tail(params->instrs, &log->entry);

    if (!(mul = add_binary_arithmetic_expr(ctx, params->instrs, HLSL_OP2_MUL, params->args[1], log, loc)))
        return false;

    if (!(exp = hlsl_new_unary_expr(ctx, HLSL_OP1_EXP2, mul, *loc)))
        return false;
    list_add_tail(params->instrs, &exp->entry);
    return true;
}

static bool intrinsic_round(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_ROUND, arg, loc);
}

static bool intrinsic_saturate(struct hlsl_ctx *ctx,
        const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *arg;

    if (!(arg = intrinsic_float_convert_arg(ctx, params, params->args[0], loc)))
        return false;

    return !!add_unary_arithmetic_expr(ctx, params->instrs, HLSL_OP1_SAT, arg, loc);
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
    {"abs",                                 1, true,  intrinsic_abs},
    {"clamp",                               3, true,  intrinsic_clamp},
    {"cross",                               2, true,  intrinsic_cross},
    {"dot",                                 2, true,  intrinsic_dot},
    {"floor",                               1, true,  intrinsic_floor},
    {"ldexp",                               2, true,  intrinsic_ldexp},
    {"lerp",                                3, true,  intrinsic_lerp},
    {"max",                                 2, true,  intrinsic_max},
    {"min",                                 2, true,  intrinsic_min},
    {"mul",                                 2, true,  intrinsic_mul},
    {"pow",                                 2, true,  intrinsic_pow},
    {"round",                               1, true,  intrinsic_round},
    {"saturate",                            1, true,  intrinsic_saturate},
};

static int intrinsic_function_name_compare(const void *a, const void *b)
{
    const struct intrinsic_function *func = b;

    return strcmp(a, func->name);
}

static struct list *add_call(struct hlsl_ctx *ctx, const char *name,
        struct parse_initializer *params, struct vkd3d_shader_location loc)
{
    const struct hlsl_ir_function_decl *decl;
    struct intrinsic_function *intrinsic;

    if ((decl = find_function_call(ctx, name, params)))
    {
        hlsl_fixme(ctx, &loc, "Call to user-defined function \"%s\".", name);
        free_parse_initializer(params);
        return NULL;
    }
    else if ((intrinsic = bsearch(name, intrinsic_functions, ARRAY_SIZE(intrinsic_functions),
            sizeof(*intrinsic_functions), intrinsic_function_name_compare)))
    {
        if (intrinsic->param_count >= 0 && params->args_count != intrinsic->param_count)
        {
            hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                    "Wrong number of arguments to function '%s': expected %u, but got %u.",
                    name, intrinsic->param_count, params->args_count);
            free_parse_initializer(params);
            return NULL;
        }

        if (intrinsic->check_numeric)
        {
            unsigned int i;

            for (i = 0; i < params->args_count; ++i)
            {
                if (params->args[i]->data_type->type > HLSL_CLASS_LAST_NUMERIC)
                {
                    struct vkd3d_string_buffer *string;

                    if ((string = hlsl_type_to_string(ctx, params->args[i]->data_type)))
                        hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                                "Wrong type for argument %u of '%s': expected a numeric type, but got '%s'.",
                                i + 1, name, string->buffer);
                    hlsl_release_string_buffer(ctx, string);
                    free_parse_initializer(params);
                    return NULL;
                }
            }
        }

        if (!intrinsic->handler(ctx, params, &loc))
        {
            free_parse_initializer(params);
            return NULL;
        }
    }
    else
    {
        hlsl_error(ctx, &loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED, "Function \"%s\" is not defined.", name);
        free_parse_initializer(params);
        return NULL;
    }
    vkd3d_free(params->args);
    return params->instrs;
}

static struct list *add_constructor(struct hlsl_ctx *ctx, struct hlsl_type *type,
        struct parse_initializer *params, struct vkd3d_shader_location loc)
{
    static unsigned int counter;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;
    unsigned int i, idx = 0;
    char name[23];

    sprintf(name, "<constructor-%x>", counter++);
    if (!(var = hlsl_new_synthetic_var(ctx, name, type, loc)))
        return NULL;

    for (i = 0; i < params->args_count; ++i)
    {
        struct hlsl_ir_node *arg = params->args[i];

        if (arg->data_type->type == HLSL_CLASS_OBJECT)
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
    list_add_tail(params->instrs, &load->node.entry);

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
            assert(0);
            return 0;
    }
}

static bool add_method_call(struct hlsl_ctx *ctx, struct list *instrs, struct hlsl_ir_node *object,
        const char *name, const struct parse_initializer *params, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_type *object_type = object->data_type;
    struct hlsl_ir_load *object_load;

    if (object_type->type != HLSL_CLASS_OBJECT || object_type->base_type != HLSL_TYPE_TEXTURE
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_GENERIC)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, object_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Type '%s' does not have methods.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
    }

    /* Only HLSL_IR_LOAD can return an object. */
    object_load = hlsl_ir_load(object);

    if (!strcmp(name, "Load")
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_CUBE
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_CUBEARRAY)
    {
        const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
        const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
        struct hlsl_ir_resource_load *load;
        struct hlsl_ir_node *offset = NULL;
        struct hlsl_ir_node *coords;
        bool multisampled;

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
            hlsl_fixme(ctx, loc, "Load() sampling index parameter.");
        }

        assert(offset_dim);
        if (params->args_count > 1 + multisampled)
        {
            if (!(offset = add_implicit_conversion(ctx, instrs, params->args[1 + multisampled],
                    hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
                return false;
        }
        if (params->args_count > 2 + multisampled)
        {
            hlsl_fixme(ctx, loc, "Tiled resource status argument.");
        }

        /* +1 for the mipmap level */
        if (!(coords = add_implicit_conversion(ctx, instrs, params->args[0],
                hlsl_get_vector_type(ctx, HLSL_TYPE_INT, sampler_dim + 1), loc)))
            return false;

        if (!(load = hlsl_new_resource_load(ctx, object_type->e.resource_format, HLSL_RESOURCE_LOAD,
                &object_load->src, NULL, coords, offset, loc)))
            return false;
        list_add_tail(instrs, &load->node.entry);
        return true;
    }
    else if (!strcmp(name, "Sample")
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_2DMS
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_2DMSARRAY)
    {
        const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
        const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
        const struct hlsl_type *sampler_type;
        struct hlsl_ir_resource_load *load;
        struct hlsl_ir_node *offset = NULL;
        struct hlsl_ir_load *sampler_load;
        struct hlsl_ir_node *coords;

        if (params->args_count < 2 || params->args_count > 4 + !!offset_dim)
        {
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                    "Wrong number of arguments to method 'Sample': expected from 2 to %u, but got %u.",
                    4 + !!offset_dim, params->args_count);
            return false;
        }

        sampler_type = params->args[0]->data_type;
        if (sampler_type->type != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
                || sampler_type->sampler_dim != HLSL_SAMPLER_DIM_GENERIC)
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_type_to_string(ctx, sampler_type)))
                hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Wrong type for argument 0 of Sample(): expected 'sampler', but got '%s'.", string->buffer);
            hlsl_release_string_buffer(ctx, string);
            return false;
        }

        /* Only HLSL_IR_LOAD can return an object. */
        sampler_load = hlsl_ir_load(params->args[0]);

        if (!(coords = add_implicit_conversion(ctx, instrs, params->args[1],
                hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
            return false;

        if (offset_dim && params->args_count > 2)
        {
            if (!(offset = add_implicit_conversion(ctx, instrs, params->args[2],
                    hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
                return false;
        }

        if (params->args_count > 2 + !!offset_dim)
            hlsl_fixme(ctx, loc, "Sample() clamp parameter.");
        if (params->args_count > 3 + !!offset_dim)
            hlsl_fixme(ctx, loc, "Tiled resource status argument.");

        if (!(load = hlsl_new_resource_load(ctx, object_type->e.resource_format,
                HLSL_RESOURCE_SAMPLE, &object_load->src, &sampler_load->src, coords, offset, loc)))
            return false;
        list_add_tail(instrs, &load->node.entry);

        return true;
    }
    else if ((!strcmp(name, "Gather") || !strcmp(name, "GatherRed") || !strcmp(name, "GatherBlue")
            || !strcmp(name, "GatherGreen") || !strcmp(name, "GatherAlpha"))
            && (object_type->sampler_dim == HLSL_SAMPLER_DIM_2D
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_2DARRAY
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_CUBE
            || object_type->sampler_dim == HLSL_SAMPLER_DIM_CUBEARRAY))
    {
        const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
        const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
        enum hlsl_resource_load_type load_type;
        const struct hlsl_type *sampler_type;
        struct hlsl_ir_resource_load *load;
        struct hlsl_ir_node *offset = NULL;
        struct hlsl_ir_load *sampler_load;
        struct hlsl_type *result_type;
        struct hlsl_ir_node *coords;
        unsigned int read_channel;

        if (!strcmp(name, "GatherGreen"))
        {
            load_type = HLSL_RESOURCE_GATHER_GREEN;
            read_channel = 1;
        }
        else if (!strcmp(name, "GatherBlue"))
        {
            load_type = HLSL_RESOURCE_GATHER_BLUE;
            read_channel = 2;
        }
        else if (!strcmp(name, "GatherAlpha"))
        {
            load_type = HLSL_RESOURCE_GATHER_ALPHA;
            read_channel = 3;
        }
        else
        {
            load_type = HLSL_RESOURCE_GATHER_RED;
            read_channel = 0;
        }

        if (!strcmp(name, "Gather") || !offset_dim)
        {
            if (params->args_count < 2 && params->args_count > 3 + !!offset_dim)
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
            if (!(offset = add_implicit_conversion(ctx, instrs, params->args[2],
                    hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
                return false;
        }

        sampler_type = params->args[0]->data_type;
        if (sampler_type->type != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
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

        result_type = hlsl_get_vector_type(ctx, object_type->e.resource_format->base_type, 4);

        /* Only HLSL_IR_LOAD can return an object. */
        sampler_load = hlsl_ir_load(params->args[0]);

        if (!(coords = add_implicit_conversion(ctx, instrs, params->args[1],
                hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
            return false;

        if (!(load = hlsl_new_resource_load(ctx, result_type, load_type, &object_load->src,
                &sampler_load->src, coords, offset, loc)))
            return false;
        list_add_tail(instrs, &load->node.entry);
        return true;
    }
    else if (!strcmp(name, "SampleLevel")
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_2DMS
            && object_type->sampler_dim != HLSL_SAMPLER_DIM_2DMSARRAY)
    {
        const unsigned int sampler_dim = hlsl_sampler_dim_count(object_type->sampler_dim);
        const unsigned int offset_dim = hlsl_offset_dim_count(object_type->sampler_dim);
        const struct hlsl_type *sampler_type;
        struct hlsl_ir_resource_load *load;
        struct hlsl_ir_node *offset = NULL;
        struct hlsl_ir_load *sampler_load;
        struct hlsl_ir_node *coords, *lod;

        if (params->args_count < 3 || params->args_count > 4 + !!offset_dim)
        {
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                    "Wrong number of arguments to method 'SampleLevel': expected from 3 to %u, but got %u.",
                    4 + !!offset_dim, params->args_count);
            return false;
        }

        sampler_type = params->args[0]->data_type;
        if (sampler_type->type != HLSL_CLASS_OBJECT || sampler_type->base_type != HLSL_TYPE_SAMPLER
                || sampler_type->sampler_dim != HLSL_SAMPLER_DIM_GENERIC)
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_type_to_string(ctx, sampler_type)))
                hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Wrong type for argument 0 of SampleLevel(): expected 'sampler', but got '%s'.", string->buffer);
            hlsl_release_string_buffer(ctx, string);
            return false;
        }

        /* Only HLSL_IR_LOAD can return an object. */
        sampler_load = hlsl_ir_load(params->args[0]);

        if (!(coords = add_implicit_conversion(ctx, instrs, params->args[1],
                hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, sampler_dim), loc)))
            coords = params->args[1];

        if (!(lod = add_implicit_conversion(ctx, instrs, params->args[2],
                hlsl_get_scalar_type(ctx, HLSL_TYPE_FLOAT), loc)))
            lod = params->args[2];

        if (offset_dim && params->args_count > 3)
        {
            if (!(offset = add_implicit_conversion(ctx, instrs, params->args[3],
                    hlsl_get_vector_type(ctx, HLSL_TYPE_INT, offset_dim), loc)))
                return false;
        }

        if (params->args_count > 3 + !!offset_dim)
            hlsl_fixme(ctx, loc, "Tiled resource status argument.");

        if (!(load = hlsl_new_sample_lod(ctx, object_type->e.resource_format,
                &object_load->src, &sampler_load->src, coords, offset, lod, loc)))
            return false;
        list_add_tail(instrs, &load->node.entry);
        return true;
    }
    else
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, object_type)))
            hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED,
                    "Method '%s' is not defined on type '%s'.", name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return false;
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
    BOOL boolval;
    char *name;
    DWORD modifiers;
    struct hlsl_ir_node *instr;
    struct list *list;
    struct parse_fields fields;
    struct parse_function function;
    struct parse_parameter parameter;
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
}

%token KW_BLENDSTATE
%token KW_BREAK
%token KW_BUFFER
%token KW_CBUFFER
%token KW_COLUMN_MAJOR
%token KW_COMPILE
%token KW_CONST
%token KW_CONTINUE
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
%token KW_MATRIX
%token KW_NAMESPACE
%token KW_NOINTERPOLATION
%token KW_OUT
%token KW_PASS
%token KW_PIXELSHADER
%token KW_PRECISE
%token KW_RASTERIZERSTATE
%token KW_RENDERTARGETVIEW
%token KW_RETURN
%token KW_REGISTER
%token KW_ROW_MAJOR
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
%token OP_ELLIPSIS
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
%token OP_UNKNOWN1
%token OP_UNKNOWN2
%token OP_UNKNOWN3
%token OP_UNKNOWN4

%token <floatval> C_FLOAT

%token <intval> C_INTEGER
%token <intval> PRE_LINE

%type <list> add_expr
%type <list> assignment_expr
%type <list> bitand_expr
%type <list> bitor_expr
%type <list> bitxor_expr
%type <list> compound_statement
%type <list> conditional_expr
%type <list> declaration
%type <list> declaration_statement
%type <list> equality_expr
%type <list> expr
%type <list> expr_statement
%type <list> initializer_expr
%type <list> jump_statement
%type <list> logicand_expr
%type <list> logicor_expr
%type <list> loop_statement
%type <list> mul_expr
%type <list> param_list
%type <list> parameters
%type <list> postfix_expr
%type <list> primary_expr
%type <list> relational_expr
%type <list> selection_statement
%type <list> shift_expr
%type <list> statement
%type <list> statement_list
%type <list> struct_declaration
%type <list> type_specs
%type <list> unary_expr
%type <list> variables_def
%type <list> variables_def_optional

%token <name> VAR_IDENTIFIER
%token <name> NEW_IDENTIFIER
%token <name> STRING
%token <name> TYPE_IDENTIFIER

%type <arrays> arrays

%type <assign_op> assign_op

%type <boolval> boolean

%type <buffer_type> buffer_type

%type <colon_attribute> colon_attribute

%type <fields> field
%type <fields> fields_list

%type <function> func_declaration
%type <function> func_prototype

%type <initializer> complex_initializer
%type <initializer> complex_initializer_list
%type <initializer> func_arguments
%type <initializer> initializer_expr_list

%type <if_body> if_body

%type <modifiers> var_modifiers

%type <name> any_identifier
%type <name> var_identifier

%type <parameter> parameter

%type <reg_reservation> register_opt

%type <sampler_dim> texture_type

%type <semantic> semantic

%type <type> field_type
%type <type> named_struct_spec
%type <type> unnamed_struct_spec
%type <type> struct_spec
%type <type> type
%type <type> typedef_type

%type <variable_def> type_spec
%type <variable_def> variable_decl
%type <variable_def> variable_def

%%

hlsl_prog:
      %empty
    | hlsl_prog func_declaration
        {
            const struct hlsl_ir_function_decl *decl;

            decl = get_func_decl(&ctx->functions, $2.name, $2.decl->parameters);
            if (decl && !decl->func->intrinsic)
            {
                if (decl->has_body && $2.decl->has_body)
                {
                    hlsl_error(ctx, &$2.decl->loc, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                            "Function \"%s\" is already defined.", $2.name);
                    hlsl_note(ctx, &decl->loc, VKD3D_SHADER_LOG_ERROR, "\"%s\" was previously defined here.", $2.name);
                    YYABORT;
                }
                else if (!hlsl_types_are_equal(decl->return_type, $2.decl->return_type))
                {
                    hlsl_error(ctx, &$2.decl->loc, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                            "Function \"%s\" was already declared with a different return type.", $2.name);
                    hlsl_note(ctx, &decl->loc, VKD3D_SHADER_LOG_ERROR, "\"%s\" was previously declared here.", $2.name);
                    YYABORT;
                }
            }

            hlsl_add_function(ctx, $2.name, $2.decl, false);
        }
    | hlsl_prog buffer_declaration buffer_body
    | hlsl_prog declaration_statement
        {
            if (!list_empty($2))
                hlsl_fixme(ctx, &@2, "Uniform initializer.");
            destroy_instr_list($2);
        }
    | hlsl_prog preproc_directive
    | hlsl_prog ';'

buffer_declaration:
      buffer_type any_identifier colon_attribute
        {
            if ($3.semantic.name)
                hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC, "Semantics are not allowed on buffers.");

            if (!(ctx->cur_buffer = hlsl_new_buffer(ctx, $1, $2, &$3.reg_reservation, @2)))
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
      declaration_statement
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

struct_declaration:
      var_modifiers struct_spec variables_def_optional ';'
        {
            struct hlsl_type *type;
            unsigned int modifiers = $1;

            if (!$3)
            {
                if (!$2->name)
                    hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                            "Anonymous struct type must declare a variable.");
                if (modifiers)
                    hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                            "Modifiers are not allowed on struct type declarations.");
            }

            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, @1)))
                YYABORT;
            $$ = declare_vars(ctx, type, modifiers, $3);
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

            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, @1)))
                YYABORT;
            if (modifiers & ~HLSL_STORAGE_NOINTERPOLATION)
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

func_declaration:
      func_prototype compound_statement
        {
            $$ = $1;
            $$.decl->has_body = true;
            list_move_tail(&$$.decl->body.instrs, $2);
            vkd3d_free($2);
            hlsl_pop_scope(ctx);
        }
    | func_prototype ';'
        {
            $$ = $1;
            hlsl_pop_scope(ctx);
        }

func_prototype:
    /* var_modifiers is necessary to avoid shift/reduce conflicts. */
      var_modifiers type var_identifier '(' parameters ')' colon_attribute
        {
            unsigned int modifiers = $1;
            struct hlsl_ir_var *var;
            struct hlsl_type *type;

            if (modifiers & ~HLSL_MODIFIERS_MAJORITY_MASK)
            {
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Only majority modifiers are allowed on functions.");
                YYABORT;
            }
            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, @1)))
                YYABORT;
            if ((var = hlsl_get_var(ctx->globals, $3)))
            {
                hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                        "\"%s\" is already declared as a variable.", $3);
                hlsl_note(ctx, &var->loc, VKD3D_SHADER_LOG_ERROR,
                        "\"%s\" was previously declared here.", $3);
                YYABORT;
            }
            if (hlsl_types_are_equal(type, ctx->builtin_types.Void) && $7.semantic.name)
            {
                hlsl_error(ctx, &@7, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                        "Semantics are not allowed on void functions.");
            }

            if ($7.reg_reservation.type)
                FIXME("Unexpected register reservation for a function.\n");

            if (!($$.decl = hlsl_new_func_decl(ctx, type, $5, &$7.semantic, @3)))
                YYABORT;
            $$.name = $3;
            ctx->cur_function = $$.decl;
        }

compound_statement:
      '{' '}'
        {
            if (!($$ = make_empty_list(ctx)))
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

var_identifier:
      VAR_IDENTIFIER
    | NEW_IDENTIFIER

colon_attribute:
      %empty
        {
            $$.semantic.name = NULL;
            $$.reg_reservation.type = 0;
        }
    | semantic
        {
            $$.semantic = $1;
            $$.reg_reservation.type = 0;
        }
    | register_opt
        {
            $$.semantic.name = NULL;
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

parameters:
      scope_start
        {
            if (!($$ = make_empty_list(ctx)))
                YYABORT;
        }
    | scope_start param_list
        {
            $$ = $2;
        }

param_list:
      parameter
        {
            if (!($$ = make_empty_list(ctx)))
                YYABORT;
            if (!add_func_parameter(ctx, $$, &$1, @1))
            {
                ERR("Error adding function parameter %s.\n", $1.name);
                YYABORT;
            }
        }
    | param_list ',' parameter
        {
            $$ = $1;
            if (!add_func_parameter(ctx, $$, &$3, @3))
            {
                hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_REDEFINED,
                        "Parameter \"%s\" is already declared.", $3.name);
                YYABORT;
            }
        }

parameter:
      var_modifiers type any_identifier colon_attribute
        {
            struct hlsl_type *type;
            unsigned int modifiers = $1;

            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, @1)))
                YYABORT;

            $$.modifiers = modifiers;
            if (!($$.modifiers & (HLSL_STORAGE_IN | HLSL_STORAGE_OUT)))
                $$.modifiers |= HLSL_STORAGE_IN;
            $$.type = type;
            $$.name = $3;
            $$.semantic = $4.semantic;
            $$.reg_reservation = $4.reg_reservation;
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

type:
      KW_VECTOR '<' type ',' C_INTEGER '>'
        {
            if ($3->type != HLSL_CLASS_SCALAR)
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

            $$ = hlsl_get_vector_type(ctx, $3->base_type, $5);
        }
    | KW_VECTOR
        {
            $$ = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4);
        }
    | KW_MATRIX '<' type ',' C_INTEGER ',' C_INTEGER '>'
        {
            if ($3->type != HLSL_CLASS_SCALAR)
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

            $$ = hlsl_get_matrix_type(ctx, $3->base_type, $7, $5);
        }
    | KW_MATRIX
        {
            $$ = hlsl_get_matrix_type(ctx, HLSL_TYPE_FLOAT, 4, 4);
        }
    | KW_VOID
        {
            $$ = ctx->builtin_types.Void;
        }
    | KW_SAMPLER
        {
            $$ = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_GENERIC];
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
            $$ = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_3D];
        }
    | KW_TEXTURE
        {
            $$ = hlsl_new_texture_type(ctx, HLSL_SAMPLER_DIM_GENERIC, NULL);
        }
    | texture_type
        {
            $$ = hlsl_new_texture_type(ctx, $1, hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4));
        }
    | texture_type '<' type '>'
        {
            if ($3->type > HLSL_CLASS_VECTOR)
            {
                struct vkd3d_string_buffer *string;

                string = hlsl_type_to_string(ctx, $3);
                if (string)
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Texture data type %s is not scalar or vector.", string->buffer);
                hlsl_release_string_buffer(ctx, string);
            }
            $$ = hlsl_new_texture_type(ctx, $1, $3);
        }
    | TYPE_IDENTIFIER
        {
            $$ = hlsl_get_type(ctx->cur_scope, $1, true);
            vkd3d_free($1);
        }
    | KW_STRUCT TYPE_IDENTIFIER
        {
            $$ = hlsl_get_type(ctx->cur_scope, $2, true);
            if ($$->type != HLSL_CLASS_STRUCT)
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_REDEFINED, "\"%s\" redefined as a structure.", $2);
            vkd3d_free($2);
        }

declaration_statement:
      declaration
    | struct_declaration
    | typedef
        {
            if (!($$ = make_empty_list(ctx)))
                YYABORT;
        }

typedef_type:
      type
    | struct_spec

typedef:
      KW_TYPEDEF var_modifiers typedef_type type_specs ';'
        {
            if ($2 & ~HLSL_TYPE_MODIFIERS_MASK)
            {
                struct parse_variable_def *v, *v_next;
                hlsl_error(ctx, &@1, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Storage modifiers are not allowed on typedefs.");
                LIST_FOR_EACH_ENTRY_SAFE(v, v_next, $4, struct parse_variable_def, entry)
                    vkd3d_free(v);
                vkd3d_free($4);
                YYABORT;
            }
            if (!add_typedef(ctx, $2, $3, $4))
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
      var_modifiers type variables_def ';'
        {
            struct hlsl_type *type;
            unsigned int modifiers = $1;

            if (!(type = apply_type_modifiers(ctx, $2, &modifiers, @1)))
                YYABORT;
            $$ = declare_vars(ctx, type, modifiers, $3);
        }

variables_def_optional:
      %empty
        {
            $$ = NULL;
        }
    | variables_def

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
            hlsl_free_instr_list($3);
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

arrays:
      %empty
        {
            $$.sizes = NULL;
            $$.count = 0;
        }
    | '[' expr ']' arrays
        {
            unsigned int size = evaluate_array_dimension(node_from_list($2));
            uint32_t *new_array;

            destroy_instr_list($2);

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
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_EXTERN, @1);
        }
    | KW_NOINTERPOLATION var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_NOINTERPOLATION, @1);
        }
    | KW_PRECISE var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_PRECISE, @1);
        }
    | KW_SHARED var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_SHARED, @1);
        }
    | KW_GROUPSHARED var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_GROUPSHARED, @1);
        }
    | KW_STATIC var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_STATIC, @1);
        }
    | KW_UNIFORM var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_UNIFORM, @1);
        }
    | KW_VOLATILE var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_VOLATILE, @1);
        }
    | KW_CONST var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_CONST, @1);
        }
    | KW_ROW_MAJOR var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_ROW_MAJOR, @1);
        }
    | KW_COLUMN_MAJOR var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_MODIFIER_COLUMN_MAJOR, @1);
        }
    | KW_IN var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_IN, @1);
        }
    | KW_OUT var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_OUT, @1);
        }
    | KW_INOUT var_modifiers
        {
            $$ = add_modifiers(ctx, $2, HLSL_STORAGE_IN | HLSL_STORAGE_OUT, @1);
        }


complex_initializer:
      initializer_expr
        {
            $$.args_count = 1;
            if (!($$.args = hlsl_alloc(ctx, sizeof(*$$.args))))
            {
                destroy_instr_list($1);
                YYABORT;
            }
            $$.args[0] = node_from_list($1);
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
            list_move_tail($$.instrs, $3.instrs);
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
                destroy_instr_list($1);
                YYABORT;
            }
            $$.args[0] = node_from_list($1);
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
                destroy_instr_list($3);
                YYABORT;
            }
            $$.args = new_args;
            $$.args[$$.args_count++] = node_from_list($3);
            list_move_tail($$.instrs, $3);
            vkd3d_free($3);
        }

boolean:
      KW_TRUE
        {
            $$ = TRUE;
        }
    | KW_FALSE
        {
            $$ = FALSE;
        }

statement_list:
      statement
    | statement_list statement
        {
            $$ = $1;
            list_move_tail($$, $2);
            vkd3d_free($2);
        }

statement:
      declaration_statement
    | expr_statement
    | compound_statement
    | jump_statement
    | selection_statement
    | loop_statement

jump_statement:
      KW_RETURN expr ';'
        {
            if (!add_return(ctx, $2, node_from_list($2), @1))
                YYABORT;
            $$ = $2;
        }
    | KW_RETURN ';'
        {
            if (!($$ = make_empty_list(ctx)))
                YYABORT;
            if (!add_return(ctx, $$, NULL, @1))
                YYABORT;
        }

selection_statement:
      KW_IF '(' expr ')' if_body
        {
            struct hlsl_ir_node *condition = node_from_list($3);
            struct hlsl_ir_if *instr;

            if (!(instr = hlsl_new_if(ctx, condition, @1)))
                YYABORT;
            list_move_tail(&instr->then_instrs.instrs, $5.then_instrs);
            if ($5.else_instrs)
                list_move_tail(&instr->else_instrs.instrs, $5.else_instrs);
            vkd3d_free($5.then_instrs);
            vkd3d_free($5.else_instrs);
            if (condition->data_type->dimx > 1 || condition->data_type->dimy > 1)
            {
                struct vkd3d_string_buffer *string;

                if ((string = hlsl_type_to_string(ctx, condition->data_type)))
                    hlsl_error(ctx, &instr->node.loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "if condition type %s is not scalar.", string->buffer);
                hlsl_release_string_buffer(ctx, string);
            }
            $$ = $3;
            list_add_tail($$, &instr->node.entry);
        }

if_body:
      statement
        {
            $$.then_instrs = $1;
            $$.else_instrs = NULL;
        }
    | statement KW_ELSE statement
        {
            $$.then_instrs = $1;
            $$.else_instrs = $3;
        }

loop_statement:
      KW_WHILE '(' expr ')' statement
        {
            $$ = create_loop(ctx, LOOP_WHILE, NULL, $3, NULL, $5, @1);
        }
    | KW_DO statement KW_WHILE '(' expr ')' ';'
        {
            $$ = create_loop(ctx, LOOP_DO_WHILE, NULL, $5, NULL, $2, @1);
        }
    | KW_FOR '(' scope_start expr_statement expr_statement expr ')' statement
        {
            $$ = create_loop(ctx, LOOP_FOR, $4, $5, $6, $8, @1);
            hlsl_pop_scope(ctx);
        }
    | KW_FOR '(' scope_start declaration expr_statement expr ')' statement
        {
            $$ = create_loop(ctx, LOOP_FOR, $4, $5, $6, $8, @1);
            hlsl_pop_scope(ctx);
        }

expr_statement:
      ';'
        {
            if (!($$ = make_empty_list(ctx)))
                YYABORT;
        }
    | expr ';'
        {
            $$ = $1;
        }

func_arguments:
      %empty
        {
            $$.args = NULL;
            $$.args_count = 0;
            if (!($$.instrs = make_empty_list(ctx)))
                YYABORT;
            $$.braces = false;
        }
    | initializer_expr_list

primary_expr:
      C_FLOAT
        {
            struct hlsl_ir_constant *c;

            if (!(c = hlsl_alloc(ctx, sizeof(*c))))
                YYABORT;
            init_node(&c->node, HLSL_IR_CONSTANT, hlsl_get_scalar_type(ctx, HLSL_TYPE_FLOAT), @1);
            c->value[0].f = $1;
            if (!($$ = make_list(ctx, &c->node)))
                YYABORT;
        }
    | C_INTEGER
        {
            struct hlsl_ir_constant *c;

            if (!(c = hlsl_alloc(ctx, sizeof(*c))))
                YYABORT;
            init_node(&c->node, HLSL_IR_CONSTANT, hlsl_get_scalar_type(ctx, HLSL_TYPE_INT), @1);
            c->value[0].i = $1;
            if (!($$ = make_list(ctx, &c->node)))
                YYABORT;
        }
    | boolean
        {
            struct hlsl_ir_constant *c;

            if (!(c = hlsl_alloc(ctx, sizeof(*c))))
                YYABORT;
            init_node(&c->node, HLSL_IR_CONSTANT, hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL), @1);
            c->value[0].u = $1 ? ~0u : 0;
            if (!($$ = make_list(ctx, &c->node)))
                YYABORT;
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
            if (!(load = hlsl_new_var_load(ctx, var, @1)))
                YYABORT;
            if (!($$ = make_list(ctx, &load->node)))
                YYABORT;
        }
    | '(' expr ')'
        {
            $$ = $2;
        }
    | var_identifier '(' func_arguments ')'
        {
            if (!($$ = add_call(ctx, $1, &$3, @1)))
                YYABORT;
        }
    | NEW_IDENTIFIER
        {
            if (ctx->in_state_block)
            {
                struct hlsl_ir_load *load;
                struct hlsl_ir_var *var;

                if (!(var = hlsl_new_synthetic_var(ctx, "<state-block-expr>",
                        hlsl_get_scalar_type(ctx, HLSL_TYPE_INT), @1)))
                    YYABORT;
                if (!(load = hlsl_new_var_load(ctx, var, @1)))
                    YYABORT;
                if (!($$ = make_list(ctx, &load->node)))
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
            if (!add_increment(ctx, $1, false, true, @2))
            {
                destroy_instr_list($1);
                YYABORT;
            }
            $$ = $1;
        }
    | postfix_expr OP_DEC
        {
            if (!add_increment(ctx, $1, true, true, @2))
            {
                destroy_instr_list($1);
                YYABORT;
            }
            $$ = $1;
        }
    | postfix_expr '.' any_identifier
        {
            struct hlsl_ir_node *node = node_from_list($1);

            if (node->data_type->type == HLSL_CLASS_STRUCT)
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
                if (!add_record_load(ctx, $1, node, field_idx, @2))
                    YYABORT;
                $$ = $1;
            }
            else if (node->data_type->type <= HLSL_CLASS_LAST_NUMERIC)
            {
                struct hlsl_ir_swizzle *swizzle;

                if (!(swizzle = get_swizzle(ctx, node, $3, &@3)))
                {
                    hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX, "Invalid swizzle \"%s\".", $3);
                    YYABORT;
                }
                list_add_tail($1, &swizzle->node.entry);
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
            struct hlsl_ir_node *array = node_from_list($1), *index = node_from_list($3);
            struct hlsl_ir_expr *cast;

            list_move_tail($1, $3);
            vkd3d_free($3);

            if (index->data_type->type != HLSL_CLASS_SCALAR)
            {
                hlsl_error(ctx, &@3, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE, "Array index is not scalar.");
                destroy_instr_list($1);
                YYABORT;
            }

            if (!(cast = hlsl_new_cast(ctx, index, hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT), &index->loc)))
            {
                destroy_instr_list($1);
                YYABORT;
            }
            list_add_tail($1, &cast->node.entry);

            if (!add_array_load(ctx, $1, array, &cast->node, @2))
            {
                destroy_instr_list($1);
                YYABORT;
            }
            $$ = $1;
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
            if ($2->type > HLSL_CLASS_LAST_NUMERIC)
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

            if (!($$ = add_constructor(ctx, $2, &$4, @2)))
            {
                free_parse_initializer(&$4);
                YYABORT;
            }
        }
    | postfix_expr '.' any_identifier '(' func_arguments ')'
        {
            struct hlsl_ir_node *object = node_from_list($1);

            list_move_tail($1, $5.instrs);
            vkd3d_free($5.instrs);

            if (!add_method_call(ctx, $1, object, $3, &$5, &@3))
            {
                hlsl_free_instr_list($1);
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
            if (!add_increment(ctx, $2, false, false, @1))
            {
                destroy_instr_list($2);
                YYABORT;
            }
            $$ = $2;
        }
    | OP_DEC unary_expr
        {
            if (!add_increment(ctx, $2, true, false, @1))
            {
                destroy_instr_list($2);
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
            add_unary_arithmetic_expr(ctx, $2, HLSL_OP1_NEG, node_from_list($2), &@1);
            $$ = $2;
        }
    | '~' unary_expr
        {
            add_unary_bitwise_expr(ctx, $2, HLSL_OP1_BIT_NOT, node_from_list($2), &@1);
            $$ = $2;
        }
    | '!' unary_expr
        {
            add_unary_logical_expr(ctx, $2, HLSL_OP1_LOGIC_NOT, node_from_list($2), &@1);
            $$ = $2;
        }
    /* var_modifiers is necessary to avoid shift/reduce conflicts. */
    | '(' var_modifiers type arrays ')' unary_expr
        {
            struct hlsl_type *src_type = node_from_list($6)->data_type;
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

            if (!compatible_data_types(src_type, dst_type))
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

            if (!add_cast(ctx, $6, node_from_list($6), dst_type, &@3))
            {
                hlsl_free_instr_list($6);
                YYABORT;
            }
            $$ = $6;
        }

mul_expr:
      unary_expr
    | mul_expr '*' unary_expr
        {
            $$ = add_binary_arithmetic_expr_merge(ctx, $1, $3, HLSL_OP2_MUL, @2);
        }
    | mul_expr '/' unary_expr
        {
            $$ = add_binary_arithmetic_expr_merge(ctx, $1, $3, HLSL_OP2_DIV, @2);
        }
    | mul_expr '%' unary_expr
        {
            $$ = add_binary_arithmetic_expr_merge(ctx, $1, $3, HLSL_OP2_MOD, @2);
        }

add_expr:
      mul_expr
    | add_expr '+' mul_expr
        {
            $$ = add_binary_arithmetic_expr_merge(ctx, $1, $3, HLSL_OP2_ADD, @2);
        }
    | add_expr '-' mul_expr
        {
            struct hlsl_ir_node *neg;

            if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, node_from_list($3), @2)))
                YYABORT;
            list_add_tail($3, &neg->entry);
            $$ = add_binary_arithmetic_expr_merge(ctx, $1, $3, HLSL_OP2_ADD, @2);
        }

shift_expr:
      add_expr
    | shift_expr OP_LEFTSHIFT add_expr
        {
            $$ = add_binary_shift_expr_merge(ctx, $1, $3, HLSL_OP2_LSHIFT, &@2);
        }
    | shift_expr OP_RIGHTSHIFT add_expr
        {
            $$ = add_binary_shift_expr_merge(ctx, $1, $3, HLSL_OP2_RSHIFT, &@2);
        }

relational_expr:
      shift_expr
    | relational_expr '<' shift_expr
        {
            $$ = add_binary_comparison_expr_merge(ctx, $1, $3, HLSL_OP2_LESS, @2);
        }
    | relational_expr '>' shift_expr
        {
            $$ = add_binary_comparison_expr_merge(ctx, $3, $1, HLSL_OP2_LESS, @2);
        }
    | relational_expr OP_LE shift_expr
        {
            $$ = add_binary_comparison_expr_merge(ctx, $3, $1, HLSL_OP2_GEQUAL, @2);
        }
    | relational_expr OP_GE shift_expr
        {
            $$ = add_binary_comparison_expr_merge(ctx, $1, $3, HLSL_OP2_GEQUAL, @2);
        }

equality_expr:
      relational_expr
    | equality_expr OP_EQ relational_expr
        {
            $$ = add_binary_comparison_expr_merge(ctx, $1, $3, HLSL_OP2_EQUAL, @2);
        }
    | equality_expr OP_NE relational_expr
        {
            $$ = add_binary_comparison_expr_merge(ctx, $1, $3, HLSL_OP2_NEQUAL, @2);
        }

bitand_expr:
      equality_expr
    | bitand_expr '&' equality_expr
        {
            $$ = add_binary_bitwise_expr_merge(ctx, $1, $3, HLSL_OP2_BIT_AND, &@2);
        }

bitxor_expr:
      bitand_expr
    | bitxor_expr '^' bitand_expr
        {
            $$ = add_binary_bitwise_expr_merge(ctx, $1, $3, HLSL_OP2_BIT_XOR, &@2);
        }

bitor_expr:
      bitxor_expr
    | bitor_expr '|' bitxor_expr
        {
            $$ = add_binary_bitwise_expr_merge(ctx, $1, $3, HLSL_OP2_BIT_OR, &@2);
        }

logicand_expr:
      bitor_expr
    | logicand_expr OP_AND bitor_expr
        {
            $$ = add_binary_logical_expr_merge(ctx, $1, $3, HLSL_OP2_LOGIC_AND, &@2);
        }

logicor_expr:
      logicand_expr
    | logicor_expr OP_OR logicand_expr
        {
            $$ = add_binary_logical_expr_merge(ctx, $1, $3, HLSL_OP2_LOGIC_OR, &@2);
        }

conditional_expr:
      logicor_expr
    | logicor_expr '?' expr ':' assignment_expr
        {
            hlsl_fixme(ctx, &@$, "Ternary operator.");
        }

assignment_expr:

      conditional_expr
    | unary_expr assign_op assignment_expr
        {
            struct hlsl_ir_node *lhs = node_from_list($1), *rhs = node_from_list($3);

            if (lhs->data_type->modifiers & HLSL_MODIFIER_CONST)
            {
                hlsl_error(ctx, &@2, VKD3D_SHADER_ERROR_HLSL_MODIFIES_CONST, "Statement modifies a const expression.");
                YYABORT;
            }
            list_move_tail($3, $1);
            vkd3d_free($1);
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
            list_move_tail($$, $3);
            vkd3d_free($3);
        }
