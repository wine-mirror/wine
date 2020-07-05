/*
 * HLSL parser
 *
 * Copyright 2008 Stefan Dösinger
 * Copyright 2012 Matteo Bruni for CodeWeavers
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
%{
#include "wine/debug.h"

#include <limits.h>
#include <stdio.h>

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlsl_parser);

int hlsl_lex(void);

struct hlsl_parse_ctx hlsl_ctx;

struct YYLTYPE;
static struct source_location get_location(const struct YYLTYPE *l);

void WINAPIV hlsl_message(const char *fmt, ...)
{
    __ms_va_list args;

    __ms_va_start(args, fmt);
    compilation_message(&hlsl_ctx.messages, fmt, args);
    __ms_va_end(args);
}

static const char *hlsl_get_error_level_name(enum hlsl_error_level level)
{
    static const char * const names[] =
    {
        "error",
        "warning",
        "note",
    };
    return names[level];
}

void WINAPIV hlsl_report_message(const struct source_location loc,
        enum hlsl_error_level level, const char *fmt, ...)
{
    __ms_va_list args;
    char *string = NULL;
    int rc, size = 0;

    while (1)
    {
        __ms_va_start(args, fmt);
        rc = vsnprintf(string, size, fmt, args);
        __ms_va_end(args);

        if (rc >= 0 && rc < size)
            break;

        if (rc >= size)
            size = rc + 1;
        else
            size = size ? size * 2 : 32;

        if (!string)
            string = d3dcompiler_alloc(size);
        else
            string = d3dcompiler_realloc(string, size);
        if (!string)
        {
            ERR("Error reallocating memory for a string.\n");
            return;
        }
    }

    hlsl_message("%s:%u:%u: %s: %s\n", loc.file, loc.line, loc.col,
            hlsl_get_error_level_name(level), string);
    d3dcompiler_free(string);

    if (level == HLSL_LEVEL_ERROR)
        set_parse_status(&hlsl_ctx.status, PARSE_ERR);
    else if (level == HLSL_LEVEL_WARNING)
        set_parse_status(&hlsl_ctx.status, PARSE_WARN);
}

static void hlsl_error(const char *s)
{
    const struct source_location loc =
    {
        .file = hlsl_ctx.source_file,
        .line = hlsl_ctx.line_no,
        .col = hlsl_ctx.column,
    };
    hlsl_report_message(loc, HLSL_LEVEL_ERROR, "%s", s);
}

static void debug_dump_decl(struct hlsl_type *type, DWORD modifiers, const char *declname, unsigned int line_no)
{
    TRACE("Line %u: ", line_no);
    if (modifiers)
        TRACE("%s ", debug_modifiers(modifiers));
    TRACE("%s %s;\n", debug_hlsl_type(type), declname);
}

static void check_invalid_matrix_modifiers(DWORD modifiers, struct source_location loc)
{
    if (modifiers & HLSL_MODIFIERS_MAJORITY_MASK)
    {
        hlsl_report_message(loc, HLSL_LEVEL_ERROR,
                "'row_major' or 'column_major' modifiers are only allowed for matrices");
    }
}

static BOOL type_is_single_reg(const struct hlsl_type *type)
{
    return type->type == HLSL_CLASS_SCALAR || type->type == HLSL_CLASS_VECTOR;
}

static BOOL declare_variable(struct hlsl_ir_var *decl, BOOL local)
{
    BOOL ret;

    TRACE("Declaring variable %s.\n", decl->name);
    if (decl->data_type->type != HLSL_CLASS_MATRIX)
        check_invalid_matrix_modifiers(decl->modifiers, decl->loc);

    if (local)
    {
        DWORD invalid = decl->modifiers & (HLSL_STORAGE_EXTERN | HLSL_STORAGE_SHARED
                | HLSL_STORAGE_GROUPSHARED | HLSL_STORAGE_UNIFORM);
        if (invalid)
        {
            hlsl_report_message(decl->loc, HLSL_LEVEL_ERROR,
                    "modifier '%s' invalid for local variables", debug_modifiers(invalid));
        }
        if (decl->semantic)
        {
            hlsl_report_message(decl->loc, HLSL_LEVEL_ERROR,
                    "semantics are not allowed on local variables");
            return FALSE;
        }
    }
    else
    {
        if (find_function(decl->name))
        {
            hlsl_report_message(decl->loc, HLSL_LEVEL_ERROR, "redefinition of '%s'", decl->name);
            return FALSE;
        }
    }
    ret = add_declaration(hlsl_ctx.cur_scope, decl, local);
    if (!ret)
    {
        struct hlsl_ir_var *old = get_variable(hlsl_ctx.cur_scope, decl->name);

        hlsl_report_message(decl->loc, HLSL_LEVEL_ERROR, "\"%s\" already declared", decl->name);
        hlsl_report_message(old->loc, HLSL_LEVEL_NOTE, "\"%s\" was previously declared here", old->name);
        return FALSE;
    }
    return TRUE;
}

static DWORD add_modifiers(DWORD modifiers, DWORD mod, const struct source_location loc)
{
    if (modifiers & mod)
    {
        hlsl_report_message(loc, HLSL_LEVEL_ERROR, "modifier '%s' already specified", debug_modifiers(mod));
        return modifiers;
    }
    if ((mod & HLSL_MODIFIERS_MAJORITY_MASK) && (modifiers & HLSL_MODIFIERS_MAJORITY_MASK))
    {
        hlsl_report_message(loc, HLSL_LEVEL_ERROR, "more than one matrix majority keyword");
        return modifiers;
    }
    return modifiers | mod;
}

static BOOL add_type_to_scope(struct hlsl_scope *scope, struct hlsl_type *def)
{
    if (get_type(scope, def->name, FALSE))
        return FALSE;

    wine_rb_put(&scope->types, def->name, &def->scope_entry);
    return TRUE;
}

static void declare_predefined_types(struct hlsl_scope *scope)
{
    struct hlsl_type *type;
    unsigned int x, y, bt;
    static const char * const names[] =
    {
        "float",
        "half",
        "double",
        "int",
        "uint",
        "bool",
    };
    char name[10];

    static const char *const sampler_names[] =
    {
        "sampler",
        "sampler1D",
        "sampler2D",
        "sampler3D",
        "samplerCUBE"
    };

    for (bt = 0; bt <= HLSL_TYPE_LAST_SCALAR; ++bt)
    {
        for (y = 1; y <= 4; ++y)
        {
            for (x = 1; x <= 4; ++x)
            {
                sprintf(name, "%s%ux%u", names[bt], y, x);
                type = new_hlsl_type(d3dcompiler_strdup(name), HLSL_CLASS_MATRIX, bt, x, y);
                add_type_to_scope(scope, type);

                if (y == 1)
                {
                    sprintf(name, "%s%u", names[bt], x);
                    type = new_hlsl_type(d3dcompiler_strdup(name), HLSL_CLASS_VECTOR, bt, x, y);
                    add_type_to_scope(scope, type);
                    hlsl_ctx.builtin_types.vector[bt][x - 1] = type;

                    if (x == 1)
                    {
                        sprintf(name, "%s", names[bt]);
                        type = new_hlsl_type(d3dcompiler_strdup(name), HLSL_CLASS_SCALAR, bt, x, y);
                        add_type_to_scope(scope, type);
                        hlsl_ctx.builtin_types.scalar[bt] = type;
                    }
                }
            }
        }
    }

    for (bt = 0; bt <= HLSL_SAMPLER_DIM_MAX; ++bt)
    {
        type = new_hlsl_type(d3dcompiler_strdup(sampler_names[bt]), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
        type->sampler_dim = bt;
        hlsl_ctx.builtin_types.sampler[bt] = type;
    }

    hlsl_ctx.builtin_types.Void = new_hlsl_type(d3dcompiler_strdup("void"), HLSL_CLASS_OBJECT, HLSL_TYPE_VOID, 1, 1);

    /* DX8 effects predefined types */
    type = new_hlsl_type(d3dcompiler_strdup("DWORD"), HLSL_CLASS_SCALAR, HLSL_TYPE_INT, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("FLOAT"), HLSL_CLASS_SCALAR, HLSL_TYPE_FLOAT, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("VECTOR"), HLSL_CLASS_VECTOR, HLSL_TYPE_FLOAT, 4, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("MATRIX"), HLSL_CLASS_MATRIX, HLSL_TYPE_FLOAT, 4, 4);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("STRING"), HLSL_CLASS_OBJECT, HLSL_TYPE_STRING, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("TEXTURE"), HLSL_CLASS_OBJECT, HLSL_TYPE_TEXTURE, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("PIXELSHADER"), HLSL_CLASS_OBJECT, HLSL_TYPE_PIXELSHADER, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("VERTEXSHADER"), HLSL_CLASS_OBJECT, HLSL_TYPE_VERTEXSHADER, 1, 1);
    add_type_to_scope(scope, type);
}

static BOOL type_is_void(const struct hlsl_type *type)
{
    return type->type == HLSL_CLASS_OBJECT && type->base_type == HLSL_TYPE_VOID;
}

static struct hlsl_ir_if *new_if(struct hlsl_ir_node *condition, struct source_location loc)
{
    struct hlsl_ir_if *iff;

    if (!(iff = d3dcompiler_alloc(sizeof(*iff))))
        return NULL;
    init_node(&iff->node, HLSL_IR_IF, NULL, loc);
    hlsl_src_from_node(&iff->condition, condition);
    list_init(&iff->then_instrs);
    list_init(&iff->else_instrs);
    return iff;
}

static BOOL append_conditional_break(struct list *cond_list)
{
    struct hlsl_ir_node *condition, *not;
    struct hlsl_ir_jump *jump;
    struct hlsl_ir_if *iff;

    /* E.g. "for (i = 0; ; ++i)". */
    if (!list_count(cond_list))
        return TRUE;

    condition = node_from_list(cond_list);
    if (!(not = new_unary_expr(HLSL_IR_UNOP_LOGIC_NOT, condition, condition->loc)))
    {
        ERR("Out of memory.\n");
        return FALSE;
    }
    list_add_tail(cond_list, &not->entry);

    if (!(iff = new_if(not, condition->loc)))
    {
        ERR("Out of memory.\n");
        return FALSE;
    }
    list_add_tail(cond_list, &iff->node.entry);

    if (!(jump = d3dcompiler_alloc(sizeof(*jump))))
    {
        ERR("Out of memory.\n");
        return FALSE;
    }
    init_node(&jump->node, HLSL_IR_JUMP, NULL, condition->loc);
    jump->type = HLSL_IR_JUMP_BREAK;
    list_add_head(&iff->then_instrs, &jump->node.entry);
    return TRUE;
}

enum loop_type
{
    LOOP_FOR,
    LOOP_WHILE,
    LOOP_DO_WHILE
};

static struct list *create_loop(enum loop_type type, struct list *init, struct list *cond,
        struct list *iter, struct list *body, struct source_location loc)
{
    struct list *list = NULL;
    struct hlsl_ir_loop *loop = NULL;
    struct hlsl_ir_if *cond_jump = NULL;

    list = d3dcompiler_alloc(sizeof(*list));
    if (!list)
        goto oom;
    list_init(list);

    if (init)
        list_move_head(list, init);

    loop = d3dcompiler_alloc(sizeof(*loop));
    if (!loop)
        goto oom;
    init_node(&loop->node, HLSL_IR_LOOP, NULL, loc);
    list_add_tail(list, &loop->node.entry);
    list_init(&loop->body);

    if (!append_conditional_break(cond))
        goto oom;

    if (type != LOOP_DO_WHILE)
        list_move_tail(&loop->body, cond);

    list_move_tail(&loop->body, body);

    if (iter)
        list_move_tail(&loop->body, iter);

    if (type == LOOP_DO_WHILE)
        list_move_tail(&loop->body, cond);

    d3dcompiler_free(init);
    d3dcompiler_free(cond);
    d3dcompiler_free(body);
    return list;

oom:
    ERR("Out of memory.\n");
    d3dcompiler_free(loop);
    d3dcompiler_free(cond_jump);
    d3dcompiler_free(list);
    free_instr_list(init);
    free_instr_list(cond);
    free_instr_list(iter);
    free_instr_list(body);
    return NULL;
}

static unsigned int initializer_size(const struct parse_initializer *initializer)
{
    unsigned int count = 0, i;

    for (i = 0; i < initializer->args_count; ++i)
    {
        count += components_count_type(initializer->args[i]->data_type);
    }
    TRACE("Initializer size = %u.\n", count);
    return count;
}

static void free_parse_initializer(struct parse_initializer *initializer)
{
    free_instr_list(initializer->instrs);
    d3dcompiler_free(initializer->args);
}

static struct hlsl_ir_swizzle *new_swizzle(DWORD s, unsigned int components,
        struct hlsl_ir_node *val, struct source_location *loc)
{
    struct hlsl_ir_swizzle *swizzle = d3dcompiler_alloc(sizeof(*swizzle));

    if (!swizzle)
        return NULL;
    init_node(&swizzle->node, HLSL_IR_SWIZZLE,
            new_hlsl_type(NULL, HLSL_CLASS_VECTOR, val->data_type->base_type, components, 1), *loc);
    hlsl_src_from_node(&swizzle->val, val);
    swizzle->swizzle = s;
    return swizzle;
}

static struct hlsl_ir_swizzle *get_swizzle(struct hlsl_ir_node *value, const char *swizzle,
        struct source_location *loc)
{
    unsigned int len = strlen(swizzle), component = 0;
    unsigned int i, set, swiz = 0;
    BOOL valid;

    if (value->data_type->type == HLSL_CLASS_MATRIX)
    {
        /* Matrix swizzle */
        BOOL m_swizzle;
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
        return new_swizzle(swiz, component, value, loc);
    }

    /* Vector swizzle */
    if (len > 4)
        return NULL;

    for (set = 0; set < 2; ++set)
    {
        valid = TRUE;
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
                valid = FALSE;
                break;
            }

            if (s >= value->data_type->dimx)
                return NULL;
            swiz |= s << component * 2;
            component++;
        }
        if (valid)
            return new_swizzle(swiz, component, value, loc);
    }

    return NULL;
}

static struct hlsl_ir_var *new_var(const char *name, struct hlsl_type *type, const struct source_location loc,
        const char *semantic, unsigned int modifiers, const struct reg_reservation *reg_reservation)
{
    struct hlsl_ir_var *var;

    if (!(var = d3dcompiler_alloc(sizeof(*var))))
    {
        hlsl_ctx.status = PARSE_ERR;
        return NULL;
    }

    var->name = name;
    var->data_type = type;
    var->loc = loc;
    var->semantic = semantic;
    var->modifiers = modifiers;
    var->reg_reservation = reg_reservation;
    return var;
}

static struct hlsl_ir_var *new_synthetic_var(const char *name, struct hlsl_type *type,
        const struct source_location loc)
{
    struct hlsl_ir_var *var = new_var(strdup(name), type, loc, NULL, 0, NULL);

    if (var)
        list_add_tail(&hlsl_ctx.globals->vars, &var->scope_entry);
    return var;
}

static struct hlsl_ir_assignment *new_assignment(struct hlsl_ir_var *var, struct hlsl_ir_node *offset,
        struct hlsl_ir_node *rhs, unsigned int writemask, struct source_location loc)
{
    struct hlsl_ir_assignment *assign;

    if (!writemask && type_is_single_reg(rhs->data_type))
        writemask = (1 << rhs->data_type->dimx) - 1;

    if (!(assign = d3dcompiler_alloc(sizeof(*assign))))
        return NULL;

    init_node(&assign->node, HLSL_IR_ASSIGNMENT, NULL, loc);
    assign->lhs.var = var;
    hlsl_src_from_node(&assign->lhs.offset, offset);
    hlsl_src_from_node(&assign->rhs, rhs);
    assign->writemask = writemask;
    return assign;
}

static struct hlsl_ir_assignment *new_simple_assignment(struct hlsl_ir_var *lhs, struct hlsl_ir_node *rhs)
{
    return new_assignment(lhs, NULL, rhs, 0, rhs->loc);
}

static struct hlsl_ir_jump *add_return(struct list *instrs,
        struct hlsl_ir_node *return_value, struct source_location loc)
{
    struct hlsl_type *return_type = hlsl_ctx.cur_function->return_type;
    struct hlsl_ir_jump *jump;

    if (return_value)
    {
        struct hlsl_ir_assignment *assignment;

        if (!(return_value = add_implicit_conversion(instrs, return_value, return_type, &loc)))
            return NULL;

        if (!(assignment = new_simple_assignment(hlsl_ctx.cur_function->return_var, return_value)))
            return NULL;
        list_add_after(&return_value->entry, &assignment->node.entry);
    }
    else if (!type_is_void(return_type))
    {
        hlsl_report_message(loc, HLSL_LEVEL_ERROR, "non-void function must return a value");
        return NULL;
    }

    if (!(jump = d3dcompiler_alloc(sizeof(*jump))))
    {
        ERR("Out of memory\n");
        return NULL;
    }
    init_node(&jump->node, HLSL_IR_JUMP, NULL, loc);
    jump->type = HLSL_IR_JUMP_RETURN;
    list_add_tail(instrs, &jump->node.entry);

    return jump;
}

static struct hlsl_ir_constant *new_uint_constant(unsigned int n, const struct source_location loc)
{
    struct hlsl_ir_constant *c;

    if (!(c = d3dcompiler_alloc(sizeof(*c))))
        return NULL;
    init_node(&c->node, HLSL_IR_CONSTANT, hlsl_ctx.builtin_types.scalar[HLSL_TYPE_UINT], loc);
    c->value.u[0] = n;
    return c;
}

struct hlsl_ir_node *new_unary_expr(enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg, struct source_location loc)
{
    struct hlsl_ir_expr *expr;

    if (!(expr = d3dcompiler_alloc(sizeof(*expr))))
        return NULL;
    init_node(&expr->node, HLSL_IR_EXPR, arg->data_type, loc);
    expr->op = op;
    hlsl_src_from_node(&expr->operands[0], arg);
    return &expr->node;
}

struct hlsl_ir_node *new_binary_expr(enum hlsl_ir_expr_op op,
        struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2)
{
    struct hlsl_ir_expr *expr;

    assert(compare_hlsl_types(arg1->data_type, arg2->data_type));

    if (!(expr = d3dcompiler_alloc(sizeof(*expr))))
        return NULL;
    init_node(&expr->node, HLSL_IR_EXPR, arg1->data_type, arg1->loc);
    expr->op = op;
    hlsl_src_from_node(&expr->operands[0], arg1);
    hlsl_src_from_node(&expr->operands[1], arg2);
    return &expr->node;
}

static struct hlsl_ir_load *new_var_load(struct hlsl_ir_var *var, const struct source_location loc)
{
    struct hlsl_ir_load *load = d3dcompiler_alloc(sizeof(*load));

    if (!load)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    init_node(&load->node, HLSL_IR_LOAD, var->data_type, loc);
    load->src.var = var;
    return load;
}

static struct hlsl_ir_load *add_load(struct list *instrs, struct hlsl_ir_node *var_node, struct hlsl_ir_node *offset,
        struct hlsl_type *data_type, const struct source_location loc)
{
    struct hlsl_ir_node *add = NULL;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;

    if (var_node->type == HLSL_IR_LOAD)
    {
        const struct hlsl_deref *src = &load_from_node(var_node)->src;

        var = src->var;
        if (src->offset.node)
        {
            if (!(add = new_binary_expr(HLSL_IR_BINOP_ADD, src->offset.node, offset)))
                return NULL;
            list_add_tail(instrs, &add->entry);
            offset = add;
        }
    }
    else
    {
        struct hlsl_ir_assignment *assign;
        char name[27];

        sprintf(name, "<deref-%p>", var_node);
        if (!(var = new_synthetic_var(name, var_node->data_type, var_node->loc)))
            return NULL;

        TRACE("Synthesized variable %p for %s node.\n", var, debug_node_type(var_node->type));

        if (!(assign = new_simple_assignment(var, var_node)))
            return NULL;

        list_add_tail(instrs, &assign->node.entry);
    }

    if (!(load = d3dcompiler_alloc(sizeof(*load))))
        return NULL;
    init_node(&load->node, HLSL_IR_LOAD, data_type, loc);
    load->src.var = var;
    hlsl_src_from_node(&load->src.offset, offset);
    list_add_tail(instrs, &load->node.entry);
    return load;
}

static struct hlsl_ir_load *add_record_load(struct list *instrs, struct hlsl_ir_node *record,
        const struct hlsl_struct_field *field, const struct source_location loc)
{
    struct hlsl_ir_constant *c;

    if (!(c = new_uint_constant(field->reg_offset * 4, loc)))
        return NULL;
    list_add_tail(instrs, &c->node.entry);

    return add_load(instrs, record, &c->node, field->type, loc);
}

static struct hlsl_ir_load *add_array_load(struct list *instrs, struct hlsl_ir_node *array,
        struct hlsl_ir_node *index, const struct source_location loc)
{
    const struct hlsl_type *expr_type = array->data_type;
    struct hlsl_type *data_type;
    struct hlsl_ir_constant *c;
    struct hlsl_ir_node *mul;

    TRACE("Array load from type %s.\n", debug_hlsl_type(expr_type));

    if (expr_type->type == HLSL_CLASS_ARRAY)
    {
        data_type = expr_type->e.array.type;
    }
    else if (expr_type->type == HLSL_CLASS_MATRIX || expr_type->type == HLSL_CLASS_VECTOR)
    {
        /* This needs to be lowered now, while we still have type information. */
        FIXME("Index of matrix or vector type.\n");
        return NULL;
    }
    else
    {
        if (expr_type->type == HLSL_CLASS_SCALAR)
            hlsl_report_message(loc, HLSL_LEVEL_ERROR, "array-indexed expression is scalar");
        else
            hlsl_report_message(loc, HLSL_LEVEL_ERROR, "expression is not array-indexable");
        return NULL;
    }

    if (!(c = new_uint_constant(data_type->reg_size * 4, loc)))
        return NULL;
    list_add_tail(instrs, &c->node.entry);
    if (!(mul = new_binary_expr(HLSL_IR_BINOP_MUL, index, &c->node)))
        return NULL;
    list_add_tail(instrs, &mul->entry);
    index = mul;

    return add_load(instrs, array, index, data_type, loc);
}

static void struct_var_initializer(struct list *list, struct hlsl_ir_var *var,
        struct parse_initializer *initializer)
{
    struct hlsl_type *type = var->data_type;
    struct hlsl_struct_field *field;
    unsigned int i = 0;

    if (initializer_size(initializer) != components_count_type(type))
    {
        hlsl_report_message(var->loc, HLSL_LEVEL_ERROR, "structure initializer mismatch");
        free_parse_initializer(initializer);
        return;
    }

    list_move_tail(list, initializer->instrs);
    d3dcompiler_free(initializer->instrs);

    LIST_FOR_EACH_ENTRY(field, type->e.elements, struct hlsl_struct_field, entry)
    {
        struct hlsl_ir_node *node = initializer->args[i];
        struct hlsl_ir_assignment *assign;
        struct hlsl_ir_constant *c;

        if (i++ >= initializer->args_count)
            break;

        if (components_count_type(field->type) == components_count_type(node->data_type))
        {
            if (!(c = new_uint_constant(field->reg_offset * 4, node->loc)))
                break;
            list_add_tail(list, &c->node.entry);

            if (!(assign = new_assignment(var, &c->node, node, 0, node->loc)))
                break;
            list_add_tail(list, &assign->node.entry);
        }
        else
            FIXME("Initializing with \"mismatched\" fields is not supported yet.\n");
    }

    d3dcompiler_free(initializer->args);
}

static void free_parse_variable_def(struct parse_variable_def *v)
{
    free_parse_initializer(&v->initializer);
    d3dcompiler_free(v->name);
    d3dcompiler_free((void *)v->semantic);
    d3dcompiler_free(v->reg_reservation);
    d3dcompiler_free(v);
}

static struct list *declare_vars(struct hlsl_type *basic_type, DWORD modifiers, struct list *var_list)
{
    struct hlsl_type *type;
    struct parse_variable_def *v, *v_next;
    struct hlsl_ir_var *var;
    BOOL ret, local = TRUE;
    struct list *statements_list = d3dcompiler_alloc(sizeof(*statements_list));

    if (basic_type->type == HLSL_CLASS_MATRIX)
        assert(basic_type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);

    if (!statements_list)
    {
        ERR("Out of memory.\n");
        LIST_FOR_EACH_ENTRY_SAFE(v, v_next, var_list, struct parse_variable_def, entry)
            free_parse_variable_def(v);
        d3dcompiler_free(var_list);
        return NULL;
    }
    list_init(statements_list);

    if (!var_list)
        return statements_list;

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, var_list, struct parse_variable_def, entry)
    {
        if (v->array_size)
            type = new_array_type(basic_type, v->array_size);
        else
            type = basic_type;

        if (!(var = new_var(v->name, type, v->loc, v->semantic, modifiers, v->reg_reservation)))
        {
            free_parse_variable_def(v);
            continue;
        }
        debug_dump_decl(type, modifiers, v->name, v->loc.line);

        if (hlsl_ctx.cur_scope == hlsl_ctx.globals)
        {
            var->modifiers |= HLSL_STORAGE_UNIFORM;
            local = FALSE;
        }

        if (type->modifiers & HLSL_MODIFIER_CONST && !(var->modifiers & HLSL_STORAGE_UNIFORM) && !v->initializer.args_count)
        {
            hlsl_report_message(v->loc, HLSL_LEVEL_ERROR, "const variable without initializer");
            free_declaration(var);
            d3dcompiler_free(v);
            continue;
        }

        ret = declare_variable(var, local);
        if (!ret)
        {
            free_declaration(var);
            d3dcompiler_free(v);
            continue;
        }
        TRACE("Declared variable %s.\n", var->name);

        if (v->initializer.args_count)
        {
            unsigned int size = initializer_size(&v->initializer);
            struct hlsl_ir_load *load;

            TRACE("Variable with initializer.\n");
            if (type->type <= HLSL_CLASS_LAST_NUMERIC
                    && type->dimx * type->dimy != size && size != 1)
            {
                if (size < type->dimx * type->dimy)
                {
                    hlsl_report_message(v->loc, HLSL_LEVEL_ERROR,
                            "'%s' initializer does not match", v->name);
                    free_parse_initializer(&v->initializer);
                    d3dcompiler_free(v);
                    continue;
                }
            }
            if ((type->type == HLSL_CLASS_STRUCT || type->type == HLSL_CLASS_ARRAY)
                    && components_count_type(type) != size)
            {
                hlsl_report_message(v->loc, HLSL_LEVEL_ERROR,
                        "'%s' initializer does not match", v->name);
                free_parse_initializer(&v->initializer);
                d3dcompiler_free(v);
                continue;
            }

            if (type->type == HLSL_CLASS_STRUCT)
            {
                struct_var_initializer(statements_list, var, &v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (type->type > HLSL_CLASS_LAST_NUMERIC)
            {
                FIXME("Initializers for non scalar/struct variables not supported yet.\n");
                free_parse_initializer(&v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (v->array_size > 0)
            {
                FIXME("Initializing arrays is not supported yet.\n");
                free_parse_initializer(&v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (v->initializer.args_count > 1)
            {
                FIXME("Complex initializers are not supported yet.\n");
                free_parse_initializer(&v->initializer);
                d3dcompiler_free(v);
                continue;
            }

            load = new_var_load(var, var->loc);
            list_add_tail(v->initializer.instrs, &load->node.entry);
            add_assignment(v->initializer.instrs, &load->node, ASSIGN_OP_ASSIGN, v->initializer.args[0]);
            d3dcompiler_free(v->initializer.args);

            if (modifiers & HLSL_STORAGE_STATIC)
                list_move_tail(&hlsl_ctx.static_initializers, v->initializer.instrs);
            else
                list_move_tail(statements_list, v->initializer.instrs);
            d3dcompiler_free(v->initializer.instrs);
        }
        d3dcompiler_free(v);
    }
    d3dcompiler_free(var_list);
    return statements_list;
}

static BOOL add_struct_field(struct list *fields, struct hlsl_struct_field *field)
{
    struct hlsl_struct_field *f;

    LIST_FOR_EACH_ENTRY(f, fields, struct hlsl_struct_field, entry)
    {
        if (!strcmp(f->name, field->name))
            return FALSE;
    }
    list_add_tail(fields, &field->entry);
    return TRUE;
}

BOOL is_row_major(const struct hlsl_type *type)
{
    /* Default to column-major if the majority isn't explicitly set, which can
     * happen for anonymous nodes. */
    return !!(type->modifiers & HLSL_MODIFIER_ROW_MAJOR);
}

static struct hlsl_type *apply_type_modifiers(struct hlsl_type *type,
        unsigned int *modifiers, struct source_location loc)
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
        if (hlsl_ctx.matrix_majority == HLSL_COLUMN_MAJOR)
            default_majority = HLSL_MODIFIER_COLUMN_MAJOR;
        else
            default_majority = HLSL_MODIFIER_ROW_MAJOR;
    }

    if (!default_majority && !(*modifiers & HLSL_TYPE_MODIFIERS_MASK))
        return type;

    if (!(new_type = clone_hlsl_type(type, default_majority)))
        return NULL;

    new_type->modifiers = add_modifiers(new_type->modifiers, *modifiers, loc);
    *modifiers &= ~HLSL_TYPE_MODIFIERS_MASK;

    if (new_type->type == HLSL_CLASS_MATRIX)
        new_type->reg_size = is_row_major(new_type) ? new_type->dimy : new_type->dimx;
    return new_type;
}

static struct list *gen_struct_fields(struct hlsl_type *type, DWORD modifiers, struct list *fields)
{
    struct parse_variable_def *v, *v_next;
    struct hlsl_struct_field *field;
    struct list *list;

    if (type->type == HLSL_CLASS_MATRIX)
        assert(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);

    list = d3dcompiler_alloc(sizeof(*list));
    if (!list)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    list_init(list);
    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, fields, struct parse_variable_def, entry)
    {
        debug_dump_decl(type, 0, v->name, v->loc.line);
        field = d3dcompiler_alloc(sizeof(*field));
        if (!field)
        {
            ERR("Out of memory.\n");
            d3dcompiler_free(v);
            return list;
        }
        if (v->array_size)
            field->type = new_array_type(type, v->array_size);
        else
            field->type = type;
        field->name = v->name;
        field->modifiers = modifiers;
        field->semantic = v->semantic;
        if (v->initializer.args_count)
        {
            hlsl_report_message(v->loc, HLSL_LEVEL_ERROR, "struct field with an initializer.\n");
            free_parse_initializer(&v->initializer);
        }
        list_add_tail(list, &field->entry);
        d3dcompiler_free(v);
    }
    d3dcompiler_free(fields);
    return list;
}

static DWORD get_array_size(const struct hlsl_type *type)
{
    if (type->type == HLSL_CLASS_ARRAY)
        return get_array_size(type->e.array.type) * type->e.array.elements_count;
    return 1;
}

static struct hlsl_type *new_struct_type(const char *name, struct list *fields)
{
    struct hlsl_type *type = d3dcompiler_alloc(sizeof(*type));
    struct hlsl_struct_field *field;
    unsigned int reg_size = 0;

    if (!type)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    type->type = HLSL_CLASS_STRUCT;
    type->base_type = HLSL_TYPE_VOID;
    type->name = name;
    type->dimx = 0;
    type->dimy = 1;
    type->e.elements = fields;

    LIST_FOR_EACH_ENTRY(field, fields, struct hlsl_struct_field, entry)
    {
        field->reg_offset = reg_size;
        reg_size += field->type->reg_size;
        type->dimx += field->type->dimx * field->type->dimy * get_array_size(field->type);
    }
    type->reg_size = reg_size;

    list_add_tail(&hlsl_ctx.types, &type->entry);

    return type;
}

static BOOL add_typedef(DWORD modifiers, struct hlsl_type *orig_type, struct list *list)
{
    BOOL ret;
    struct hlsl_type *type;
    struct parse_variable_def *v, *v_next;

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, list, struct parse_variable_def, entry)
    {
        if (v->array_size)
            type = new_array_type(orig_type, v->array_size);
        else
            type = clone_hlsl_type(orig_type, 0);
        if (!type)
        {
            ERR("Out of memory\n");
            return FALSE;
        }
        d3dcompiler_free((void *)type->name);
        type->name = v->name;
        type->modifiers |= modifiers;

        if (type->type != HLSL_CLASS_MATRIX)
            check_invalid_matrix_modifiers(type->modifiers, v->loc);
        else
            type->reg_size = is_row_major(type) ? type->dimy : type->dimx;

        if ((type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
                && (type->modifiers & HLSL_MODIFIER_ROW_MAJOR))
            hlsl_report_message(v->loc, HLSL_LEVEL_ERROR, "more than one matrix majority keyword");

        ret = add_type_to_scope(hlsl_ctx.cur_scope, type);
        if (!ret)
        {
            hlsl_report_message(v->loc, HLSL_LEVEL_ERROR,
                    "redefinition of custom type '%s'", v->name);
        }
        d3dcompiler_free(v);
    }
    d3dcompiler_free(list);
    return TRUE;
}

static BOOL add_func_parameter(struct list *list, struct parse_parameter *param, const struct source_location loc)
{
    struct hlsl_ir_var *var;

    if (param->type->type == HLSL_CLASS_MATRIX)
        assert(param->type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);

    if (!(var = new_var(param->name, param->type, loc, param->semantic, param->modifiers, param->reg_reservation)))
        return FALSE;

    if (!add_declaration(hlsl_ctx.cur_scope, var, FALSE))
    {
        free_declaration(var);
        return FALSE;
    }
    list_add_tail(list, &var->param_entry);
    return TRUE;
}

static struct reg_reservation *parse_reg_reservation(const char *reg_string)
{
    struct reg_reservation *reg_res;
    enum bwritershader_param_register_type type;
    DWORD regnum = 0;

    switch (reg_string[0])
    {
        case 'c':
            type = BWRITERSPR_CONST;
            break;
        case 'i':
            type = BWRITERSPR_CONSTINT;
            break;
        case 'b':
            type = BWRITERSPR_CONSTBOOL;
            break;
        case 's':
            type = BWRITERSPR_SAMPLER;
            break;
        default:
            FIXME("Unsupported register type.\n");
            return NULL;
     }

    if (!sscanf(reg_string + 1, "%u", &regnum))
    {
        FIXME("Unsupported register reservation syntax.\n");
        return NULL;
    }

    reg_res = d3dcompiler_alloc(sizeof(*reg_res));
    if (!reg_res)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    reg_res->type = type;
    reg_res->regnum = regnum;
    return reg_res;
}

static const struct hlsl_ir_function_decl *get_overloaded_func(struct wine_rb_tree *funcs, char *name,
        struct list *params, BOOL exact_signature)
{
    struct hlsl_ir_function *func;
    struct wine_rb_entry *entry;

    entry = wine_rb_get(funcs, name);
    if (entry)
    {
        func = WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);

        entry = wine_rb_get(&func->overloads, params);
        if (!entry)
        {
            if (!exact_signature)
                FIXME("No exact match, search for a compatible overloaded function (if any).\n");
            return NULL;
        }
        return WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function_decl, entry);
    }
    return NULL;
}

static struct hlsl_ir_function_decl *get_func_entry(const char *name)
{
    struct hlsl_ir_function_decl *decl;
    struct hlsl_ir_function *func;
    struct wine_rb_entry *entry;

    if ((entry = wine_rb_get(&hlsl_ctx.functions, name)))
    {
        func = WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);
        WINE_RB_FOR_EACH_ENTRY(decl, &func->overloads, struct hlsl_ir_function_decl, entry)
            return decl;
    }

    return NULL;
}

static struct list *append_unop(struct list *list, struct hlsl_ir_node *node)
{
    list_add_tail(list, &node->entry);
    return list;
}

static struct list *add_binary_expr(struct list *list1, struct list *list2,
        enum hlsl_ir_expr_op op, struct source_location loc)
{
    struct hlsl_ir_node *args[3] = {node_from_list(list1), node_from_list(list2)};
    list_move_tail(list1, list2);
    d3dcompiler_free(list2);
    add_expr(list1, op, args, &loc);
    return list1;
}

static struct list *make_list(struct hlsl_ir_node *node)
{
    struct list *list;

    if (!(list = d3dcompiler_alloc(sizeof(*list))))
    {
        ERR("Out of memory.\n");
        free_instr(node);
        return NULL;
    }
    list_init(list);
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
        struct hlsl_ir_constant *constant = constant_from_node(node);

        switch (constant->node.data_type->base_type)
        {
        case HLSL_TYPE_UINT:
            return constant->value.u[0];
        case HLSL_TYPE_INT:
            return constant->value.i[0];
        case HLSL_TYPE_FLOAT:
            return constant->value.f[0];
        case HLSL_TYPE_DOUBLE:
            return constant->value.d[0];
        case HLSL_TYPE_BOOL:
            return constant->value.b[0];
        default:
            WARN("Invalid type %s.\n", debug_base_type(constant->node.data_type));
            return 0;
        }
    }
    case HLSL_IR_EXPR:
    case HLSL_IR_LOAD:
    case HLSL_IR_SWIZZLE:
        FIXME("Unhandled type %s.\n", debug_node_type(node->type));
        return 0;
    case HLSL_IR_ASSIGNMENT:
    default:
        WARN("Invalid node type %s.\n", debug_node_type(node->type));
        return 0;
    }
}

static struct hlsl_ir_function_decl *new_func_decl(struct hlsl_type *return_type,
        struct list *parameters, const char *semantic, struct source_location loc)
{
    struct hlsl_ir_function_decl *decl;

    if (!(decl = d3dcompiler_alloc(sizeof(*decl))))
        return NULL;
    decl->return_type = return_type;
    decl->parameters = parameters;
    decl->semantic = semantic;
    decl->loc = loc;

    if (!type_is_void(return_type))
    {
        struct hlsl_ir_var *return_var;
        char name[28];

        sprintf(name, "<retval-%p>", decl);
        if (!(return_var = new_synthetic_var(name, return_type, loc)))
        {
            d3dcompiler_free(decl);
            return NULL;
        }
        decl->return_var = return_var;
    }

    return decl;
}

%}

%locations
%define parse.error verbose
%expect 1

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
    struct parse_function function;
    struct parse_parameter parameter;
    struct parse_initializer initializer;
    struct parse_variable_def *variable_def;
    struct parse_if_body if_body;
    enum parse_unary_op unary_op;
    enum parse_assign_op assign_op;
    struct reg_reservation *reg_reservation;
    struct parse_colon_attribute colon_attribute;
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
%token KW_TEXTURE3DARRAY
%token KW_TEXTURECUBE
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

%token <intval> PRE_LINE

%token <name> VAR_IDENTIFIER TYPE_IDENTIFIER NEW_IDENTIFIER
%type <name> any_identifier var_identifier
%token <name> STRING
%token <floatval> C_FLOAT
%token <intval> C_INTEGER
%type <boolval> boolean
%type <type> base_type
%type <type> type
%type <list> declaration_statement
%type <list> declaration
%type <list> struct_declaration
%type <type> struct_spec
%type <type> named_struct_spec
%type <type> unnamed_struct_spec
%type <type> field_type
%type <type> typedef_type
%type <list> type_specs
%type <variable_def> type_spec
%type <initializer> complex_initializer
%type <initializer> initializer_expr_list
%type <list> initializer_expr
%type <modifiers> var_modifiers
%type <list> field
%type <list> parameters
%type <list> param_list
%type <list> expr
%type <intval> array
%type <list> statement
%type <list> statement_list
%type <list> compound_statement
%type <list> jump_statement
%type <list> selection_statement
%type <list> loop_statement
%type <function> func_declaration
%type <function> func_prototype
%type <list> fields_list
%type <parameter> parameter
%type <colon_attribute> colon_attribute
%type <name> semantic
%type <reg_reservation> register_opt
%type <variable_def> variable_def
%type <list> variables_def
%type <list> variables_def_optional
%type <if_body> if_body
%type <list> primary_expr
%type <list> postfix_expr
%type <list> unary_expr
%type <list> mul_expr
%type <list> add_expr
%type <list> shift_expr
%type <list> relational_expr
%type <list> equality_expr
%type <list> bitand_expr
%type <list> bitxor_expr
%type <list> bitor_expr
%type <list> logicand_expr
%type <list> logicor_expr
%type <list> conditional_expr
%type <list> assignment_expr
%type <list> expr_statement
%type <unary_op> unary_op
%type <assign_op> assign_op
%type <modifiers> input_mods
%type <modifiers> input_mod
%%

hlsl_prog:                /* empty */
                            {
                            }
                        | hlsl_prog func_declaration
                            {
                                const struct hlsl_ir_function_decl *decl;

                                decl = get_overloaded_func(&hlsl_ctx.functions, $2.name, $2.decl->parameters, TRUE);
                                if (decl && !decl->func->intrinsic)
                                {
                                    if (decl->body && $2.decl->body)
                                    {
                                        hlsl_report_message($2.decl->loc, HLSL_LEVEL_ERROR,
                                                "redefinition of function %s", debugstr_a($2.name));
                                        YYABORT;
                                    }
                                    else if (!compare_hlsl_types(decl->return_type, $2.decl->return_type))
                                    {
                                        hlsl_report_message($2.decl->loc, HLSL_LEVEL_ERROR,
                                                "redefining function %s with a different return type",
                                                debugstr_a($2.name));
                                        hlsl_report_message(decl->loc, HLSL_LEVEL_NOTE,
                                                "%s previously declared here",
                                                debugstr_a($2.name));
                                        YYABORT;
                                    }
                                }

                                if (type_is_void($2.decl->return_type) && $2.decl->semantic)
                                {
                                    hlsl_report_message($2.decl->loc, HLSL_LEVEL_ERROR,
                                            "void function with a semantic");
                                }

                                TRACE("Adding function '%s' to the function list.\n", $2.name);
                                add_function_decl(&hlsl_ctx.functions, $2.name, $2.decl, FALSE);
                            }
                        | hlsl_prog declaration_statement
                            {
                                TRACE("Declaration statement parsed.\n");

                                if (!list_empty($2))
                                    FIXME("Uniform initializer.\n");
                                free_instr_list($2);
                            }
                        | hlsl_prog preproc_directive
                            {
                            }
                        | hlsl_prog ';'
                            {
                                TRACE("Skipping stray semicolon.\n");
                            }

preproc_directive:        PRE_LINE STRING
                            {
                                const char **new_array = NULL;

                                TRACE("Updating line information to file %s, line %u\n", debugstr_a($2), $1);
                                hlsl_ctx.line_no = $1;
                                if (strcmp($2, hlsl_ctx.source_file))
                                    new_array = d3dcompiler_realloc(hlsl_ctx.source_files,
                                            sizeof(*hlsl_ctx.source_files) * (hlsl_ctx.source_files_count + 1));

                                if (new_array)
                                {
                                    hlsl_ctx.source_files = new_array;
                                    hlsl_ctx.source_files[hlsl_ctx.source_files_count++] = $2;
                                    hlsl_ctx.source_file = $2;
                                }
                                else
                                {
                                    d3dcompiler_free($2);
                                }
                            }

struct_declaration:       var_modifiers struct_spec variables_def_optional ';'
                            {
                                struct hlsl_type *type;
                                DWORD modifiers = $1;

                                if (!$3)
                                {
                                    if (!$2->name)
                                    {
                                        hlsl_report_message(get_location(&@2), HLSL_LEVEL_ERROR,
                                                "anonymous struct declaration with no variables");
                                    }
                                    if (modifiers)
                                    {
                                        hlsl_report_message(get_location(&@1), HLSL_LEVEL_ERROR,
                                                "modifier not allowed on struct type declaration");
                                    }
                                }

                                if (!(type = apply_type_modifiers($2, &modifiers, get_location(&@1))))
                                    YYABORT;
                                $$ = declare_vars(type, modifiers, $3);
                            }

struct_spec:              named_struct_spec
                        | unnamed_struct_spec

named_struct_spec:        KW_STRUCT any_identifier '{' fields_list '}'
                            {
                                BOOL ret;

                                TRACE("Structure %s declaration.\n", debugstr_a($2));
                                $$ = new_struct_type($2, $4);

                                if (get_variable(hlsl_ctx.cur_scope, $2))
                                {
                                    hlsl_report_message(get_location(&@2),
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'", $2);
                                    YYABORT;
                                }

                                ret = add_type_to_scope(hlsl_ctx.cur_scope, $$);
                                if (!ret)
                                {
                                    hlsl_report_message(get_location(&@2),
                                            HLSL_LEVEL_ERROR, "redefinition of struct '%s'", $2);
                                    YYABORT;
                                }
                            }

unnamed_struct_spec:      KW_STRUCT '{' fields_list '}'
                            {
                                TRACE("Anonymous structure declaration.\n");
                                $$ = new_struct_type(NULL, $3);
                            }

any_identifier:           VAR_IDENTIFIER
                        | TYPE_IDENTIFIER
                        | NEW_IDENTIFIER

fields_list:              /* Empty */
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | fields_list field
                            {
                                BOOL ret;
                                struct hlsl_struct_field *field, *next;

                                $$ = $1;
                                LIST_FOR_EACH_ENTRY_SAFE(field, next, $2, struct hlsl_struct_field, entry)
                                {
                                    ret = add_struct_field($$, field);
                                    if (ret == FALSE)
                                    {
                                        hlsl_report_message(get_location(&@2),
                                                HLSL_LEVEL_ERROR, "redefinition of '%s'", field->name);
                                        d3dcompiler_free(field);
                                    }
                                }
                                d3dcompiler_free($2);
                            }

field_type:               type
                        | unnamed_struct_spec

field:                    var_modifiers field_type variables_def ';'
                            {
                                struct hlsl_type *type;
                                DWORD modifiers = $1;

                                if (!(type = apply_type_modifiers($2, &modifiers, get_location(&@1))))
                                    YYABORT;
                                $$ = gen_struct_fields(type, modifiers, $3);
                            }

func_declaration:         func_prototype compound_statement
                            {
                                TRACE("Function %s parsed.\n", $1.name);
                                $$ = $1;
                                $$.decl->body = $2;
                                pop_scope(&hlsl_ctx);
                            }
                        | func_prototype ';'
                            {
                                TRACE("Function prototype for %s.\n", $1.name);
                                $$ = $1;
                                pop_scope(&hlsl_ctx);
                            }

                        /* var_modifiers is necessary to avoid shift/reduce conflicts. */
func_prototype:           var_modifiers type var_identifier '(' parameters ')' colon_attribute
                            {
                                if ($1)
                                {
                                    hlsl_report_message(get_location(&@1), HLSL_LEVEL_ERROR,
                                            "unexpected modifiers on a function");
                                    YYABORT;
                                }
                                if (get_variable(hlsl_ctx.globals, $3))
                                {
                                    hlsl_report_message(get_location(&@3),
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'\n", $3);
                                    YYABORT;
                                }
                                if (type_is_void($2) && $7.semantic)
                                {
                                    hlsl_report_message(get_location(&@7),
                                            HLSL_LEVEL_ERROR, "void function with a semantic");
                                }

                                if ($7.reg_reservation)
                                {
                                    FIXME("Unexpected register reservation for a function.\n");
                                    d3dcompiler_free($7.reg_reservation);
                                }
                                if (!($$.decl = new_func_decl($2, $5, $7.semantic, get_location(&@3))))
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                $$.name = $3;
                                hlsl_ctx.cur_function = $$.decl;
                            }

compound_statement:       '{' '}'
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | '{' scope_start statement_list '}'
                            {
                                pop_scope(&hlsl_ctx);
                                $$ = $3;
                            }

scope_start:              /* Empty */
                            {
                                push_scope(&hlsl_ctx);
                            }

var_identifier:           VAR_IDENTIFIER
                        | NEW_IDENTIFIER

colon_attribute:          /* Empty */
                            {
                                $$.semantic = NULL;
                                $$.reg_reservation = NULL;
                            }
                        | semantic
                            {
                                $$.semantic = $1;
                                $$.reg_reservation = NULL;
                            }
                        | register_opt
                            {
                                $$.semantic = NULL;
                                $$.reg_reservation = $1;
                            }

semantic:                 ':' any_identifier
                            {
                                $$ = $2;
                            }

                          /* FIXME: Writemasks */
register_opt:             ':' KW_REGISTER '(' any_identifier ')'
                            {
                                $$ = parse_reg_reservation($4);
                                d3dcompiler_free($4);
                            }
                        | ':' KW_REGISTER '(' any_identifier ',' any_identifier ')'
                            {
                                FIXME("Ignoring shader target %s in a register reservation.\n", debugstr_a($4));
                                d3dcompiler_free($4);

                                $$ = parse_reg_reservation($6);
                                d3dcompiler_free($6);
                            }

parameters:               scope_start
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | scope_start param_list
                            {
                                $$ = $2;
                            }

param_list:               parameter
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                if (!add_func_parameter($$, &$1, get_location(&@1)))
                                {
                                    ERR("Error adding function parameter %s.\n", $1.name);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }
                            }
                        | param_list ',' parameter
                            {
                                $$ = $1;
                                if (!add_func_parameter($$, &$3, get_location(&@3)))
                                {
                                    hlsl_report_message(get_location(&@3), HLSL_LEVEL_ERROR,
                                            "duplicate parameter %s", $3.name);
                                    YYABORT;
                                }
                            }

parameter:                input_mods var_modifiers type any_identifier colon_attribute
                            {
                                struct hlsl_type *type;
                                DWORD modifiers = $2;

                                if (!(type = apply_type_modifiers($3, &modifiers, get_location(&@2))))
                                    YYABORT;

                                $$.modifiers = $1 ? $1 : HLSL_STORAGE_IN;
                                $$.modifiers |= modifiers;
                                $$.type = type;
                                $$.name = $4;
                                $$.semantic = $5.semantic;
                                $$.reg_reservation = $5.reg_reservation;
                            }

input_mods:               /* Empty */
                            {
                                $$ = 0;
                            }
                        | input_mods input_mod
                            {
                                if ($1 & $2)
                                {
                                    hlsl_report_message(get_location(&@2), HLSL_LEVEL_ERROR,
                                            "duplicate input-output modifiers");
                                    YYABORT;
                                }
                                $$ = $1 | $2;
                            }

input_mod:                KW_IN
                            {
                                $$ = HLSL_STORAGE_IN;
                            }
                        | KW_OUT
                            {
                                $$ = HLSL_STORAGE_OUT;
                            }
                        | KW_INOUT
                            {
                                $$ = HLSL_STORAGE_IN | HLSL_STORAGE_OUT;
                            }

type:

      base_type
        {
            $$ = $1;
        }
    | KW_VECTOR '<' base_type ',' C_INTEGER '>'
        {
            if ($3->type != HLSL_CLASS_SCALAR)
            {
                hlsl_report_message(get_location(&@3), HLSL_LEVEL_ERROR,
                        "vectors of non-scalar types are not allowed\n");
                YYABORT;
            }
            if ($5 < 1 || $5 > 4)
            {
                hlsl_report_message(get_location(&@5), HLSL_LEVEL_ERROR,
                        "vector size must be between 1 and 4\n");
                YYABORT;
            }

            $$ = new_hlsl_type(NULL, HLSL_CLASS_VECTOR, $3->base_type, $5, 1);
        }
    | KW_MATRIX '<' base_type ',' C_INTEGER ',' C_INTEGER '>'
        {
            if ($3->type != HLSL_CLASS_SCALAR)
            {
                hlsl_report_message(get_location(&@3), HLSL_LEVEL_ERROR,
                        "matrices of non-scalar types are not allowed\n");
                YYABORT;
            }
            if ($5 < 1 || $5 > 4)
            {
                hlsl_report_message(get_location(&@5), HLSL_LEVEL_ERROR,
                        "matrix row count must be between 1 and 4\n");
                YYABORT;
            }
            if ($7 < 1 || $7 > 4)
            {
                hlsl_report_message(get_location(&@7), HLSL_LEVEL_ERROR,
                        "matrix column count must be between 1 and 4\n");
                YYABORT;
            }

            $$ = new_hlsl_type(NULL, HLSL_CLASS_MATRIX, $3->base_type, $7, $5);
        }

base_type:

      KW_VOID
        {
            $$ = hlsl_ctx.builtin_types.Void;
        }
    | KW_SAMPLER
        {
            $$ = hlsl_ctx.builtin_types.sampler[HLSL_SAMPLER_DIM_GENERIC];
        }
    | KW_SAMPLER1D
        {
            $$ = hlsl_ctx.builtin_types.sampler[HLSL_SAMPLER_DIM_1D];
        }
    | KW_SAMPLER2D
        {
            $$ = hlsl_ctx.builtin_types.sampler[HLSL_SAMPLER_DIM_2D];
        }
    | KW_SAMPLER3D
        {
            $$ = hlsl_ctx.builtin_types.sampler[HLSL_SAMPLER_DIM_3D];
        }
    | KW_SAMPLERCUBE
        {
            $$ = hlsl_ctx.builtin_types.sampler[HLSL_SAMPLER_DIM_3D];
        }
    | TYPE_IDENTIFIER
        {
            $$ = get_type(hlsl_ctx.cur_scope, $1, TRUE);
            d3dcompiler_free($1);
        }
    | KW_STRUCT TYPE_IDENTIFIER
        {
            $$ = get_type(hlsl_ctx.cur_scope, $2, TRUE);
            if ($$->type != HLSL_CLASS_STRUCT)
                hlsl_report_message(get_location(&@1), HLSL_LEVEL_ERROR, "'%s' redefined as a structure\n", $2);
            d3dcompiler_free($2);
        }

declaration_statement:    declaration
                        | struct_declaration
                        | typedef
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                if (!$$)
                                {
                                    ERR("Out of memory\n");
                                    YYABORT;
                                }
                                list_init($$);
                            }

typedef_type:             type
                        | struct_spec

typedef:                  KW_TYPEDEF var_modifiers typedef_type type_specs ';'
                            {
                                if ($2 & ~HLSL_TYPE_MODIFIERS_MASK)
                                {
                                    struct parse_variable_def *v, *v_next;
                                    hlsl_report_message(get_location(&@1),
                                            HLSL_LEVEL_ERROR, "modifier not allowed on typedefs");
                                    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, $4, struct parse_variable_def, entry)
                                        d3dcompiler_free(v);
                                    d3dcompiler_free($4);
                                    YYABORT;
                                }
                                if (!add_typedef($2, $3, $4))
                                    YYABORT;
                            }

type_specs:               type_spec
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &$1->entry);
                            }
                        | type_specs ',' type_spec
                            {
                                $$ = $1;
                                list_add_tail($$, &$3->entry);
                            }

type_spec:                any_identifier array
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                $$->loc = get_location(&@1);
                                $$->name = $1;
                                $$->array_size = $2;
                            }

declaration:              var_modifiers type variables_def ';'
                            {
                                struct hlsl_type *type;
                                DWORD modifiers = $1;

                                if (!(type = apply_type_modifiers($2, &modifiers, get_location(&@1))))
                                    YYABORT;
                                $$ = declare_vars(type, modifiers, $3);
                            }

variables_def_optional:   /* Empty */
                            {
                                $$ = NULL;
                            }
                        | variables_def
                            {
                                $$ = $1;
                            }

variables_def:            variable_def
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &$1->entry);
                            }
                        | variables_def ',' variable_def
                            {
                                $$ = $1;
                                list_add_tail($$, &$3->entry);
                            }

variable_def:             any_identifier array colon_attribute
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                $$->loc = get_location(&@1);
                                $$->name = $1;
                                $$->array_size = $2;
                                $$->semantic = $3.semantic;
                                $$->reg_reservation = $3.reg_reservation;
                            }
                        | any_identifier array colon_attribute '=' complex_initializer
                            {
                                TRACE("Declaration with initializer.\n");
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                $$->loc = get_location(&@1);
                                $$->name = $1;
                                $$->array_size = $2;
                                $$->semantic = $3.semantic;
                                $$->reg_reservation = $3.reg_reservation;
                                $$->initializer = $5;
                            }

array:                    /* Empty */
                            {
                                $$ = 0;
                            }
                        | '[' expr ']'
                            {
                                unsigned int size = evaluate_array_dimension(node_from_list($2));

                                free_instr_list($2);

                                if (!size)
                                {
                                    hlsl_report_message(get_location(&@2), HLSL_LEVEL_ERROR,
                                            "array size is not a positive integer constant\n");
                                    YYABORT;
                                }
                                TRACE("Array size %u.\n", size);

                                if (size > 65536)
                                {
                                    hlsl_report_message(get_location(&@2), HLSL_LEVEL_ERROR,
                                            "array size must be between 1 and 65536");
                                    YYABORT;
                                }
                                $$ = size;
                            }

var_modifiers:            /* Empty */
                            {
                                $$ = 0;
                            }
                        | KW_EXTERN var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_STORAGE_EXTERN, get_location(&@1));
                            }
                        | KW_NOINTERPOLATION var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_STORAGE_NOINTERPOLATION, get_location(&@1));
                            }
                        | KW_PRECISE var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_MODIFIER_PRECISE, get_location(&@1));
                            }
                        | KW_SHARED var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_STORAGE_SHARED, get_location(&@1));
                            }
                        | KW_GROUPSHARED var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_STORAGE_GROUPSHARED, get_location(&@1));
                            }
                        | KW_STATIC var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_STORAGE_STATIC, get_location(&@1));
                            }
                        | KW_UNIFORM var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_STORAGE_UNIFORM, get_location(&@1));
                            }
                        | KW_VOLATILE var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_STORAGE_VOLATILE, get_location(&@1));
                            }
                        | KW_CONST var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_MODIFIER_CONST, get_location(&@1));
                            }
                        | KW_ROW_MAJOR var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_MODIFIER_ROW_MAJOR, get_location(&@1));
                            }
                        | KW_COLUMN_MAJOR var_modifiers
                            {
                                $$ = add_modifiers($2, HLSL_MODIFIER_COLUMN_MAJOR, get_location(&@1));
                            }

complex_initializer:      initializer_expr
                            {
                                $$.args_count = 1;
                                if (!($$.args = d3dcompiler_alloc(sizeof(*$$.args))))
                                    YYABORT;
                                $$.args[0] = node_from_list($1);
                                $$.instrs = $1;
                            }
                        | '{' initializer_expr_list '}'
                            {
                                $$ = $2;
                            }
                        | '{' initializer_expr_list ',' '}'
                            {
                                $$ = $2;
                            }

initializer_expr:         assignment_expr
                            {
                                $$ = $1;
                            }

initializer_expr_list:    initializer_expr
                            {
                                $$.args_count = 1;
                                if (!($$.args = d3dcompiler_alloc(sizeof(*$$.args))))
                                    YYABORT;
                                $$.args[0] = node_from_list($1);
                                $$.instrs = $1;
                            }
                        | initializer_expr_list ',' initializer_expr
                            {
                                $$ = $1;
                                if (!($$.args = d3dcompiler_realloc($$.args, ($$.args_count + 1) * sizeof(*$$.args))))
                                    YYABORT;
                                $$.args[$$.args_count++] = node_from_list($3);
                                list_move_tail($$.instrs, $3);
                                d3dcompiler_free($3);
                            }

boolean:                  KW_TRUE
                            {
                                $$ = TRUE;
                            }
                        | KW_FALSE
                            {
                                $$ = FALSE;
                            }

statement_list:           statement
                            {
                                $$ = $1;
                            }
                        | statement_list statement
                            {
                                $$ = $1;
                                list_move_tail($$, $2);
                                d3dcompiler_free($2);
                            }

statement:                declaration_statement
                        | expr_statement
                        | compound_statement
                        | jump_statement
                        | selection_statement
                        | loop_statement

jump_statement:

      KW_RETURN expr ';'
        {
            if (!add_return($2, node_from_list($2), get_location(&@1)))
                YYABORT;
            $$ = $2;
        }
    | KW_RETURN ';'
        {
            if (!($$ = d3dcompiler_alloc(sizeof(*$$))))
                YYABORT;
            list_init($$);
            if (!add_return($$, NULL, get_location(&@1)))
                YYABORT;
        }

selection_statement:      KW_IF '(' expr ')' if_body
                            {
                                struct hlsl_ir_node *condition = node_from_list($3);
                                struct hlsl_ir_if *instr;

                                if (!(instr = new_if(condition, get_location(&@1))))
                                    YYABORT;
                                list_move_tail(&instr->then_instrs, $5.then_instrs);
                                list_move_tail(&instr->else_instrs, $5.else_instrs);
                                d3dcompiler_free($5.then_instrs);
                                d3dcompiler_free($5.else_instrs);
                                if (condition->data_type->dimx > 1 || condition->data_type->dimy > 1)
                                {
                                    hlsl_report_message(instr->node.loc, HLSL_LEVEL_ERROR,
                                            "if condition requires a scalar");
                                }
                                $$ = $3;
                                list_add_tail($$, &instr->node.entry);
                            }

if_body:                  statement
                            {
                                $$.then_instrs = $1;
                                $$.else_instrs = NULL;
                            }
                        | statement KW_ELSE statement
                            {
                                $$.then_instrs = $1;
                                $$.else_instrs = $3;
                            }

loop_statement:           KW_WHILE '(' expr ')' statement
                            {
                                $$ = create_loop(LOOP_WHILE, NULL, $3, NULL, $5, get_location(&@1));
                            }
                        | KW_DO statement KW_WHILE '(' expr ')' ';'
                            {
                                $$ = create_loop(LOOP_DO_WHILE, NULL, $5, NULL, $2, get_location(&@1));
                            }
                        | KW_FOR '(' scope_start expr_statement expr_statement expr ')' statement
                            {
                                $$ = create_loop(LOOP_FOR, $4, $5, $6, $8, get_location(&@1));
                                pop_scope(&hlsl_ctx);
                            }
                        | KW_FOR '(' scope_start declaration expr_statement expr ')' statement
                            {
                                if (!$4)
                                    hlsl_report_message(get_location(&@4), HLSL_LEVEL_WARNING,
                                            "no expressions in for loop initializer");
                                $$ = create_loop(LOOP_FOR, $4, $5, $6, $8, get_location(&@1));
                                pop_scope(&hlsl_ctx);
                            }

expr_statement:           ';'
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | expr ';'
                            {
                                $$ = $1;
                            }

primary_expr:             C_FLOAT
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                init_node(&c->node, HLSL_IR_CONSTANT,
                                        hlsl_ctx.builtin_types.scalar[HLSL_TYPE_FLOAT], get_location(&@1));
                                c->value.f[0] = $1;
                                if (!($$ = make_list(&c->node)))
                                    YYABORT;
                            }
                        | C_INTEGER
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                init_node(&c->node, HLSL_IR_CONSTANT,
                                        hlsl_ctx.builtin_types.scalar[HLSL_TYPE_INT], get_location(&@1));
                                c->value.i[0] = $1;
                                if (!($$ = make_list(&c->node)))
                                    YYABORT;
                            }
                        | boolean
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                init_node(&c->node, HLSL_IR_CONSTANT,
                                        hlsl_ctx.builtin_types.scalar[HLSL_TYPE_BOOL], get_location(&@1));
                                c->value.b[0] = $1;
                                if (!($$ = make_list(&c->node)))
                                    YYABORT;
                            }
                        | VAR_IDENTIFIER
                            {
                                struct hlsl_ir_load *load;
                                struct hlsl_ir_var *var;

                                if (!(var = get_variable(hlsl_ctx.cur_scope, $1)))
                                {
                                    hlsl_report_message(get_location(&@1), HLSL_LEVEL_ERROR,
                                            "variable '%s' is not declared\n", $1);
                                    YYABORT;
                                }
                                if ((load = new_var_load(var, get_location(&@1))))
                                {
                                    if (!($$ = make_list(&load->node)))
                                        YYABORT;
                                }
                                else
                                    $$ = NULL;
                            }
                        | '(' expr ')'
                            {
                                $$ = $2;
                            }

postfix_expr:             primary_expr
                            {
                                $$ = $1;
                            }
                        | postfix_expr OP_INC
                            {
                                struct source_location loc;
                                struct hlsl_ir_node *inc;

                                loc = get_location(&@2);
                                if (node_from_list($1)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc, HLSL_LEVEL_ERROR, "modifying a const expression");
                                    YYABORT;
                                }
                                inc = new_unary_expr(HLSL_IR_UNOP_POSTINC, node_from_list($1), loc);
                                /* Post increment/decrement expressions are considered const */
                                inc->data_type = clone_hlsl_type(inc->data_type, 0);
                                inc->data_type->modifiers |= HLSL_MODIFIER_CONST;
                                $$ = append_unop($1, inc);
                            }
                        | postfix_expr OP_DEC
                            {
                                struct source_location loc;
                                struct hlsl_ir_node *inc;

                                loc = get_location(&@2);
                                if (node_from_list($1)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc, HLSL_LEVEL_ERROR, "modifying a const expression");
                                    YYABORT;
                                }
                                inc = new_unary_expr(HLSL_IR_UNOP_POSTDEC, node_from_list($1), loc);
                                /* Post increment/decrement expressions are considered const */
                                inc->data_type = clone_hlsl_type(inc->data_type, 0);
                                inc->data_type->modifiers |= HLSL_MODIFIER_CONST;
                                $$ = append_unop($1, inc);
                            }
                        | postfix_expr '.' any_identifier
                            {
                                struct hlsl_ir_node *node = node_from_list($1);
                                struct source_location loc;

                                loc = get_location(&@2);
                                if (node->data_type->type == HLSL_CLASS_STRUCT)
                                {
                                    struct hlsl_type *type = node->data_type;
                                    struct hlsl_struct_field *field;

                                    $$ = NULL;
                                    LIST_FOR_EACH_ENTRY(field, type->e.elements, struct hlsl_struct_field, entry)
                                    {
                                        if (!strcmp($3, field->name))
                                        {
                                            if (!add_record_load($1, node, field, loc))
                                                YYABORT;
                                            $$ = $1;
                                            break;
                                        }
                                    }
                                    if (!$$)
                                    {
                                        hlsl_report_message(loc, HLSL_LEVEL_ERROR,
                                                "invalid subscript %s", debugstr_a($3));
                                        YYABORT;
                                    }
                                }
                                else if (node->data_type->type <= HLSL_CLASS_LAST_NUMERIC)
                                {
                                    struct hlsl_ir_swizzle *swizzle;

                                    swizzle = get_swizzle(node, $3, &loc);
                                    if (!swizzle)
                                    {
                                        hlsl_report_message(loc, HLSL_LEVEL_ERROR,
                                                "invalid swizzle %s", debugstr_a($3));
                                        YYABORT;
                                    }
                                    $$ = append_unop($1, &swizzle->node);
                                }
                                else
                                {
                                    hlsl_report_message(loc, HLSL_LEVEL_ERROR,
                                            "invalid subscript %s", debugstr_a($3));
                                    YYABORT;
                                }
                            }
    | postfix_expr '[' expr ']'
        {
            struct hlsl_ir_node *array = node_from_list($1), *index = node_from_list($3);

            list_move_tail($1, $3);
            d3dcompiler_free($3);

            if (index->data_type->type != HLSL_CLASS_SCALAR)
            {
                hlsl_report_message(get_location(&@3), HLSL_LEVEL_ERROR, "array index is not scalar");
                free_instr_list($1);
                YYABORT;
            }

            if (!add_array_load($1, array, index, get_location(&@2)))
            {
                free_instr_list($1);
                YYABORT;
            }
            $$ = $1;
        }

      /* "var_modifiers" doesn't make sense in this case, but it's needed
         in the grammar to avoid shift/reduce conflicts. */
    | var_modifiers type '(' initializer_expr_list ')'
        {
            struct hlsl_ir_assignment *assignment;
            unsigned int i, writemask_offset = 0;
            static unsigned int counter;
            struct hlsl_ir_load *load;
            struct hlsl_ir_var *var;
            char name[23];

            if ($1)
            {
                hlsl_report_message(get_location(&@1), HLSL_LEVEL_ERROR,
                        "unexpected modifier on a constructor\n");
                YYABORT;
            }
            if ($2->type > HLSL_CLASS_LAST_NUMERIC)
            {
                hlsl_report_message(get_location(&@2), HLSL_LEVEL_ERROR,
                        "constructors may only be used with numeric data types\n");
                YYABORT;
            }
            if ($2->dimx * $2->dimy != initializer_size(&$4))
            {
                hlsl_report_message(get_location(&@4), HLSL_LEVEL_ERROR,
                        "expected %u components in constructor, but got %u\n",
                        $2->dimx * $2->dimy, initializer_size(&$4));
                YYABORT;
            }

            if ($2->type == HLSL_CLASS_MATRIX)
                FIXME("Matrix constructors are not supported yet.\n");

            sprintf(name, "<constructor-%x>", counter++);
            if (!(var = new_synthetic_var(name, $2, get_location(&@2))))
                YYABORT;
            for (i = 0; i < $4.args_count; ++i)
            {
                struct hlsl_ir_node *arg = $4.args[i];
                unsigned int width;

                if (arg->data_type->type == HLSL_CLASS_OBJECT)
                {
                    hlsl_report_message(arg->loc, HLSL_LEVEL_ERROR,
                            "invalid constructor argument");
                    continue;
                }
                width = components_count_type(arg->data_type);

                if (width > 4)
                {
                    FIXME("Constructor argument with %u components.\n", width);
                    continue;
                }

                if (!(arg = add_implicit_conversion($4.instrs, arg,
                        hlsl_ctx.builtin_types.vector[$2->base_type][width - 1], &arg->loc)))
                    continue;

                if (!(assignment = new_assignment(var, NULL, arg,
                        ((1 << width) - 1) << writemask_offset, arg->loc)))
                    YYABORT;
                writemask_offset += width;
                list_add_tail($4.instrs, &assignment->node.entry);
            }
            d3dcompiler_free($4.args);
            if (!(load = new_var_load(var, get_location(&@2))))
                YYABORT;
            $$ = append_unop($4.instrs, &load->node);
        }

unary_expr:               postfix_expr
                            {
                                $$ = $1;
                            }
                        | OP_INC unary_expr
                            {
                                struct source_location loc;

                                loc = get_location(&@1);
                                if (node_from_list($2)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc, HLSL_LEVEL_ERROR, "modifying a const expression");
                                    YYABORT;
                                }
                                $$ = append_unop($2, new_unary_expr(HLSL_IR_UNOP_PREINC, node_from_list($2), loc));
                            }
                        | OP_DEC unary_expr
                            {
                                struct source_location loc;

                                loc = get_location(&@1);
                                if (node_from_list($2)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc, HLSL_LEVEL_ERROR, "modifying a const expression");
                                    YYABORT;
                                }
                                $$ = append_unop($2, new_unary_expr(HLSL_IR_UNOP_PREDEC, node_from_list($2), loc));
                            }
                        | unary_op unary_expr
                            {
                                enum hlsl_ir_expr_op ops[] = {0, HLSL_IR_UNOP_NEG,
                                        HLSL_IR_UNOP_LOGIC_NOT, HLSL_IR_UNOP_BIT_NOT};

                                if ($1 == UNARY_OP_PLUS)
                                {
                                    $$ = $2;
                                }
                                else
                                {
                                    $$ = append_unop($2, new_unary_expr(ops[$1], node_from_list($2), get_location(&@1)));
                                }
                            }
                          /* var_modifiers just to avoid shift/reduce conflicts */
                        | '(' var_modifiers type array ')' unary_expr
                            {
                                struct hlsl_type *src_type = node_from_list($6)->data_type;
                                struct hlsl_type *dst_type;
                                struct source_location loc;

                                loc = get_location(&@3);
                                if ($2)
                                {
                                    hlsl_report_message(loc, HLSL_LEVEL_ERROR, "unexpected modifier in a cast");
                                    YYABORT;
                                }

                                if ($4)
                                    dst_type = new_array_type($3, $4);
                                else
                                    dst_type = $3;

                                if (!compatible_data_types(src_type, dst_type))
                                {
                                    hlsl_report_message(loc, HLSL_LEVEL_ERROR, "can't cast from %s to %s",
                                            debug_hlsl_type(src_type), debug_hlsl_type(dst_type));
                                    YYABORT;
                                }

                                $$ = append_unop($6, &new_cast(node_from_list($6), dst_type, &loc)->node);
                            }

unary_op:                 '+'
                            {
                                $$ = UNARY_OP_PLUS;
                            }
                        | '-'
                            {
                                $$ = UNARY_OP_MINUS;
                            }
                        | '!'
                            {
                                $$ = UNARY_OP_LOGICNOT;
                            }
                        | '~'
                            {
                                $$ = UNARY_OP_BITNOT;
                            }

mul_expr:

      unary_expr
    | mul_expr '*' unary_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_MUL, get_location(&@2));
        }
    | mul_expr '/' unary_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_DIV, get_location(&@2));
        }
    | mul_expr '%' unary_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_MOD, get_location(&@2));
        }

add_expr:

      mul_expr
    | add_expr '+' mul_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_ADD, get_location(&@2));
        }
    | add_expr '-' mul_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_SUB, get_location(&@2));
        }

shift_expr:               add_expr
                            {
                                $$ = $1;
                            }
                        | shift_expr OP_LEFTSHIFT add_expr
                            {
                                FIXME("Left shift\n");
                            }
                        | shift_expr OP_RIGHTSHIFT add_expr
                            {
                                FIXME("Right shift\n");
                            }

relational_expr:

      shift_expr
    | relational_expr '<' shift_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_LESS, get_location(&@2));
        }
    | relational_expr '>' shift_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_GREATER, get_location(&@2));
        }
    | relational_expr OP_LE shift_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_LEQUAL, get_location(&@2));
        }
    | relational_expr OP_GE shift_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_GEQUAL, get_location(&@2));
        }

equality_expr:

      relational_expr
    | equality_expr OP_EQ relational_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_EQUAL, get_location(&@2));
        }
    | equality_expr OP_NE relational_expr
        {
            $$ = add_binary_expr($1, $3, HLSL_IR_BINOP_NEQUAL, get_location(&@2));
        }

bitand_expr:              equality_expr
                            {
                                $$ = $1;
                            }
                        | bitand_expr '&' equality_expr
                            {
                                FIXME("bitwise AND\n");
                            }

bitxor_expr:              bitand_expr
                            {
                                $$ = $1;
                            }
                        | bitxor_expr '^' bitand_expr
                            {
                                FIXME("bitwise XOR\n");
                            }

bitor_expr:               bitxor_expr
                            {
                                $$ = $1;
                            }
                        | bitor_expr '|' bitxor_expr
                            {
                                FIXME("bitwise OR\n");
                            }

logicand_expr:            bitor_expr
                            {
                                $$ = $1;
                            }
                        | logicand_expr OP_AND bitor_expr
                            {
                                FIXME("logic AND\n");
                            }

logicor_expr:             logicand_expr
                            {
                                $$ = $1;
                            }
                        | logicor_expr OP_OR logicand_expr
                            {
                                FIXME("logic OR\n");
                            }

conditional_expr:         logicor_expr
                            {
                                $$ = $1;
                            }
                        | logicor_expr '?' expr ':' assignment_expr
                            {
                                FIXME("ternary operator\n");
                            }

assignment_expr:

      conditional_expr
    | unary_expr assign_op assignment_expr
        {
            struct hlsl_ir_node *lhs = node_from_list($1), *rhs = node_from_list($3);

            if (lhs->data_type->modifiers & HLSL_MODIFIER_CONST)
            {
                hlsl_report_message(get_location(&@2), HLSL_LEVEL_ERROR, "l-value is const");
                YYABORT;
            }
            list_move_tail($3, $1);
            d3dcompiler_free($1);
            if (!add_assignment($3, lhs, $2, rhs))
                YYABORT;
            $$ = $3;
        }

assign_op:                '='
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

expr:                     assignment_expr
                            {
                                $$ = $1;
                            }
                        | expr ',' assignment_expr
                            {
                                $$ = $1;
                                list_move_tail($$, $3);
                                d3dcompiler_free($3);
                            }

%%

static struct source_location get_location(const struct YYLTYPE *l)
{
    const struct source_location loc =
    {
        .file = hlsl_ctx.source_file,
        .line = l->first_line,
        .col = l->first_column,
    };
    return loc;
}

static void dump_function_decl(struct wine_rb_entry *entry, void *context)
{
    struct hlsl_ir_function_decl *func = WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function_decl, entry);
    if (func->body)
        debug_dump_ir_function_decl(func);
}

static void dump_function(struct wine_rb_entry *entry, void *context)
{
    struct hlsl_ir_function *func = WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);
    wine_rb_for_each_entry(&func->overloads, dump_function_decl, NULL);
}

/* Allocate a unique, ordered index to each instruction, which will be used for
 * computing liveness ranges. */
static unsigned int index_instructions(struct list *instrs, unsigned int index)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, instrs, struct hlsl_ir_node, entry)
    {
        instr->index = index++;

        if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = if_from_node(instr);
            index = index_instructions(&iff->then_instrs, index);
            index = index_instructions(&iff->else_instrs, index);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            index = index_instructions(&loop_from_node(instr)->body, index);
            loop_from_node(instr)->next_index = index;
        }
    }

    return index;
}

/* Compute the earliest and latest liveness for each variable. In the case that
 * a variable is accessed inside of a loop, we promote its liveness to extend
 * to at least the range of the entire loop. Note that we don't need to do this
 * for anonymous nodes, since there's currently no way to use a node which was
 * calculated in an earlier iteration of the loop. */
static void compute_liveness_recurse(struct list *instrs, unsigned int loop_first, unsigned int loop_last)
{
    struct hlsl_ir_node *instr;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(instr, instrs, struct hlsl_ir_node, entry)
    {
        switch (instr->type)
        {
        case HLSL_IR_ASSIGNMENT:
        {
            struct hlsl_ir_assignment *assignment = assignment_from_node(instr);
            var = assignment->lhs.var;
            if (!var->first_write)
                var->first_write = loop_first ? min(instr->index, loop_first) : instr->index;
            assignment->rhs.node->last_read = instr->index;
            if (assignment->lhs.offset.node)
                assignment->lhs.offset.node->last_read = instr->index;
            break;
        }
        case HLSL_IR_EXPR:
        {
            struct hlsl_ir_expr *expr = expr_from_node(instr);
            unsigned int i;

            for (i = 0; i < ARRAY_SIZE(expr->operands) && expr->operands[i].node; ++i)
                expr->operands[i].node->last_read = instr->index;
            break;
        }
        case HLSL_IR_IF:
        {
            struct hlsl_ir_if *iff = if_from_node(instr);
            compute_liveness_recurse(&iff->then_instrs, loop_first, loop_last);
            compute_liveness_recurse(&iff->else_instrs, loop_first, loop_last);
            iff->condition.node->last_read = instr->index;
            break;
        }
        case HLSL_IR_LOAD:
        {
            struct hlsl_ir_load *load = load_from_node(instr);
            var = load->src.var;
            var->last_read = loop_last ? max(instr->index, loop_last) : instr->index;
            if (load->src.offset.node)
                load->src.offset.node->last_read = instr->index;
            break;
        }
        case HLSL_IR_LOOP:
        {
            struct hlsl_ir_loop *loop = loop_from_node(instr);
            compute_liveness_recurse(&loop->body, loop_first ? loop_first : instr->index,
                    loop_last ? loop_last : loop->next_index);
            break;
        }
        case HLSL_IR_SWIZZLE:
        {
            struct hlsl_ir_swizzle *swizzle = swizzle_from_node(instr);
            swizzle->val.node->last_read = instr->index;
            break;
        }
        case HLSL_IR_CONSTANT:
        case HLSL_IR_JUMP:
            break;
        }
    }
}

static void compute_liveness(struct hlsl_ir_function_decl *entry_func)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &hlsl_ctx.globals->vars, struct hlsl_ir_var, scope_entry)
    {
        var->first_write = 1;
    }

    LIST_FOR_EACH_ENTRY(var, entry_func->parameters, struct hlsl_ir_var, param_entry)
    {
        if (var->modifiers & HLSL_STORAGE_IN)
            var->first_write = 1;
        if (var->modifiers & HLSL_STORAGE_OUT)
            var->last_read = UINT_MAX;
    }

    if (entry_func->return_var)
        entry_func->return_var->last_read = UINT_MAX;

    compute_liveness_recurse(entry_func->body, 0, 0);
}

HRESULT parse_hlsl(enum shader_type type, DWORD major, DWORD minor,
        const char *entrypoint, ID3D10Blob **shader_blob, char **messages)
{
    struct hlsl_ir_function_decl *entry_func;
    struct hlsl_scope *scope, *next_scope;
    struct hlsl_type *hlsl_type, *next_type;
    struct hlsl_ir_var *var, *next_var;
    HRESULT hr = E_FAIL;
    unsigned int i;

    hlsl_ctx.status = PARSE_SUCCESS;
    hlsl_ctx.messages.size = hlsl_ctx.messages.capacity = 0;
    hlsl_ctx.line_no = hlsl_ctx.column = 1;
    hlsl_ctx.source_file = d3dcompiler_strdup("");
    hlsl_ctx.source_files = d3dcompiler_alloc(sizeof(*hlsl_ctx.source_files));
    if (hlsl_ctx.source_files)
        hlsl_ctx.source_files[0] = hlsl_ctx.source_file;
    hlsl_ctx.source_files_count = 1;
    hlsl_ctx.cur_scope = NULL;
    hlsl_ctx.matrix_majority = HLSL_COLUMN_MAJOR;
    list_init(&hlsl_ctx.scopes);
    list_init(&hlsl_ctx.types);
    init_functions_tree(&hlsl_ctx.functions);
    list_init(&hlsl_ctx.static_initializers);

    push_scope(&hlsl_ctx);
    hlsl_ctx.globals = hlsl_ctx.cur_scope;
    declare_predefined_types(hlsl_ctx.globals);

    hlsl_parse();

    if (hlsl_ctx.status == PARSE_ERR)
        goto out;

    if (!(entry_func = get_func_entry(entrypoint)))
    {
        hlsl_message("error: entry point %s is not defined\n", debugstr_a(entrypoint));
        goto out;
    }

    if (!type_is_void(entry_func->return_type)
            && entry_func->return_type->type != HLSL_CLASS_STRUCT && !entry_func->semantic)
    {
        hlsl_report_message(entry_func->loc, HLSL_LEVEL_ERROR,
                "entry point \"%s\" is missing a return value semantic", entry_func->func->name);
    }

    list_move_head(entry_func->body, &hlsl_ctx.static_initializers);

    /* Index 0 means unused; index 1 means function entry, so start at 2. */
    index_instructions(entry_func->body, 2);

    if (TRACE_ON(hlsl_parser))
    {
        TRACE("IR dump.\n");
        wine_rb_for_each_entry(&hlsl_ctx.functions, dump_function, NULL);
    }

    compute_liveness(entry_func);

    if (hlsl_ctx.status != PARSE_ERR)
        hr = E_NOTIMPL;

out:
    if (messages)
    {
        if (hlsl_ctx.messages.size)
            *messages = hlsl_ctx.messages.string;
        else
            *messages = NULL;
    }
    else
    {
        if (hlsl_ctx.messages.capacity)
            d3dcompiler_free(hlsl_ctx.messages.string);
    }

    for (i = 0; i < hlsl_ctx.source_files_count; ++i)
        d3dcompiler_free((void *)hlsl_ctx.source_files[i]);
    d3dcompiler_free(hlsl_ctx.source_files);

    TRACE("Freeing functions IR.\n");
    wine_rb_destroy(&hlsl_ctx.functions, free_function_rb, NULL);

    TRACE("Freeing variables.\n");
    LIST_FOR_EACH_ENTRY_SAFE(scope, next_scope, &hlsl_ctx.scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY_SAFE(var, next_var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            free_declaration(var);
        }
        wine_rb_destroy(&scope->types, NULL, NULL);
        d3dcompiler_free(scope);
    }

    TRACE("Freeing types.\n");
    LIST_FOR_EACH_ENTRY_SAFE(hlsl_type, next_type, &hlsl_ctx.types, struct hlsl_type, entry)
    {
        free_hlsl_type(hlsl_type);
    }

    return hr;
}
