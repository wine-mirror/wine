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
#include "config.h"
#include "wine/debug.h"

#include <stdio.h>

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlsl_parser);

int hlsl_lex(void);

struct hlsl_parse_ctx hlsl_ctx;

struct YYLTYPE;
static void set_location(struct source_location *loc, const struct YYLTYPE *l);

void hlsl_message(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    compilation_message(&hlsl_ctx.messages, fmt, args);
    va_end(args);
}

static const char *hlsl_get_error_level_name(enum hlsl_error_level level)
{
    const char *names[] =
    {
        "error",
        "warning",
        "note",
    };
    return names[level];
}

void hlsl_report_message(const char *filename, DWORD line, DWORD column,
        enum hlsl_error_level level, const char *fmt, ...)
{
    va_list args;
    char *string = NULL;
    int rc, size = 0;

    while (1)
    {
        va_start(args, fmt);
        rc = vsnprintf(string, size, fmt, args);
        va_end(args);

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

    hlsl_message("%s:%u:%u: %s: %s\n", filename, line, column, hlsl_get_error_level_name(level), string);
    d3dcompiler_free(string);

    if (level == HLSL_LEVEL_ERROR)
        set_parse_status(&hlsl_ctx.status, PARSE_ERR);
    else if (level == HLSL_LEVEL_WARNING)
        set_parse_status(&hlsl_ctx.status, PARSE_WARN);
}

static void hlsl_error(const char *s)
{
    hlsl_report_message(hlsl_ctx.source_file, hlsl_ctx.line_no, hlsl_ctx.column, HLSL_LEVEL_ERROR, "%s", s);
}

static void debug_dump_decl(struct hlsl_type *type, DWORD modifiers, const char *declname, unsigned int line_no)
{
    TRACE("Line %u: ", line_no);
    if (modifiers)
        TRACE("%s ", debug_modifiers(modifiers));
    TRACE("%s %s;\n", debug_hlsl_type(type), declname);
}

static void check_invalid_matrix_modifiers(DWORD modifiers, struct source_location *loc)
{
    if (modifiers & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR))
    {
        hlsl_report_message(loc->file, loc->line, loc->col, HLSL_LEVEL_ERROR,
                "'row_major' or 'column_major' modifiers are only allowed for matrices");
    }
}

static BOOL declare_variable(struct hlsl_ir_var *decl, BOOL local)
{
    BOOL ret;

    TRACE("Declaring variable %s.\n", decl->name);
    if (decl->node.data_type->type == HLSL_CLASS_MATRIX)
    {
        if (!(decl->modifiers & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR)))
        {
            decl->modifiers |= hlsl_ctx.matrix_majority == HLSL_ROW_MAJOR
                    ? HLSL_MODIFIER_ROW_MAJOR : HLSL_MODIFIER_COLUMN_MAJOR;
        }
    }
    else
        check_invalid_matrix_modifiers(decl->modifiers, &decl->node.loc);

    if (local)
    {
        DWORD invalid = decl->modifiers & (HLSL_STORAGE_EXTERN | HLSL_STORAGE_SHARED
                | HLSL_STORAGE_GROUPSHARED | HLSL_STORAGE_UNIFORM);
        if (invalid)
        {
            hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_ERROR,
                    "modifier '%s' invalid for local variables", debug_modifiers(invalid));
        }
        if (decl->semantic)
        {
            hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_ERROR,
                    "semantics are not allowed on local variables");
            return FALSE;
        }
    }
    else
    {
        if (find_function(decl->name))
        {
            hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_ERROR,
                    "redefinition of '%s'", decl->name);
            return FALSE;
        }
    }
    ret = add_declaration(hlsl_ctx.cur_scope, decl, local);
    if (!ret)
    {
        struct hlsl_ir_var *old = get_variable(hlsl_ctx.cur_scope, decl->name);

        hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_ERROR,
                "\"%s\" already declared", decl->name);
        hlsl_report_message(old->node.loc.file, old->node.loc.line, old->node.loc.col, HLSL_LEVEL_NOTE,
                "\"%s\" was previously declared here", old->name);
        return FALSE;
    }
    return TRUE;
}

static DWORD add_modifier(DWORD modifiers, DWORD mod, const struct YYLTYPE *loc);

static unsigned int components_count_expr_list(struct list *list)
{
    struct hlsl_ir_node *node;
    unsigned int count = 0;

    LIST_FOR_EACH_ENTRY(node, list, struct hlsl_ir_node, entry)
    {
        count += components_count_type(node->data_type);
    }
    return count;
}

%}

%locations
%error-verbose

%union
{
    struct hlsl_type *type;
    INT intval;
    FLOAT floatval;
    BOOL boolval;
    char *name;
    DWORD modifiers;
    struct hlsl_ir_var *var;
    struct hlsl_ir_node *instr;
    struct list *list;
    struct hlsl_ir_function_decl *function;
    struct parse_parameter parameter;
    struct parse_variable_def *variable_def;
    enum parse_unary_op unary_op;
    enum parse_assign_op assign_op;
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
%type <list> complex_initializer
%type <list> initializer_expr_list
%type <instr> initializer_expr
%type <modifiers> var_modifiers
%type <list> parameters
%type <list> param_list
%type <instr> expr
%type <var> variable
%type <intval> array
%type <list> statement
%type <list> statement_list
%type <list> compound_statement
%type <function> func_declaration
%type <function> func_prototype
%type <parameter> parameter
%type <name> semantic
%type <variable_def> variable_def
%type <list> variables_def
%type <instr> primary_expr
%type <instr> postfix_expr
%type <instr> unary_expr
%type <instr> mul_expr
%type <instr> add_expr
%type <instr> shift_expr
%type <instr> relational_expr
%type <instr> equality_expr
%type <instr> bitand_expr
%type <instr> bitxor_expr
%type <instr> bitor_expr
%type <instr> logicand_expr
%type <instr> logicor_expr
%type <instr> conditional_expr
%type <instr> assignment_expr
%type <list> expr_statement
%type <unary_op> unary_op
%type <assign_op> assign_op
%type <modifiers> input_mod
%%

hlsl_prog:                /* empty */
                            {
                            }
                        | hlsl_prog func_declaration
                            {
                                FIXME("Check that the function doesn't conflict with an already declared one.\n");
                                list_add_tail(&hlsl_ctx.functions, &$2->node.entry);
                            }
                        | hlsl_prog declaration_statement
                            {
                                TRACE("Declaration statement parsed.\n");
                            }
                        | hlsl_prog preproc_directive
                            {
                            }

preproc_directive:        PRE_LINE STRING
                            {
                                TRACE("Updating line information to file %s, line %u\n", debugstr_a($2), $1);
                                hlsl_ctx.line_no = $1;
                                if (strcmp($2, hlsl_ctx.source_file))
                                {
                                    const char **new_array;

                                    hlsl_ctx.source_file = $2;
                                    new_array = d3dcompiler_realloc(hlsl_ctx.source_files,
                                            sizeof(*hlsl_ctx.source_files) * hlsl_ctx.source_files_count + 1);
                                    if (new_array)
                                    {
                                        hlsl_ctx.source_files = new_array;
                                        hlsl_ctx.source_files[hlsl_ctx.source_files_count++] = $2;
                                    }
                                }
                            }

any_identifier:           VAR_IDENTIFIER
                        | TYPE_IDENTIFIER
                        | NEW_IDENTIFIER

func_declaration:         func_prototype compound_statement
                            {
                                TRACE("Function %s parsed.\n", $1->name);
                                $$ = $1;
                                $$->body = $2;
                                pop_scope(&hlsl_ctx);
                            }
                        | func_prototype ';'
                            {
                                TRACE("Function prototype for %s.\n", $1->name);
                                $$ = $1;
                                pop_scope(&hlsl_ctx);
                            }

func_prototype:           var_modifiers type var_identifier '(' parameters ')' semantic
                            {
                                if (get_variable(hlsl_ctx.globals, $3))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, @3.first_line, @3.first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'\n", $3);
                                    return 1;
                                }
                                if ($2->base_type == HLSL_TYPE_VOID && $7)
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, @7.first_line, @7.first_column,
                                            HLSL_LEVEL_ERROR, "void function with a semantic");
                                }

                                $$ = new_func_decl($3, $2, $5);
                                if (!$$)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                $$->semantic = $7;
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

semantic:                 /* Empty */
                            {
                                $$ = NULL;
                            }
                        | ':' any_identifier
                            {
                                $$ = $2;
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
                                struct source_location loc;

                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                set_location(&loc, &@1);
                                if (!add_func_parameter($$, &$1, &loc))
                                {
                                    ERR("Error adding function parameter %s.\n", $1.name);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                            }
                        | param_list ',' parameter
                            {
                                struct source_location loc;

                                $$ = $1;
                                set_location(&loc, &@3);
                                if (!add_func_parameter($$, &$3, &loc))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "duplicate parameter %s", $3.name);
                                    return 1;
                                }
                            }

parameter:                input_mod var_modifiers type any_identifier semantic
                            {
                                $$.modifiers = $1;
                                $$.modifiers |= $2;
                                $$.type = $3;
                                $$.name = $4;
                                $$.semantic = $5;
                            }

input_mod:                /* Empty */
                            {
                                $$ = HLSL_MODIFIER_IN;
                            }
                        | KW_IN
                            {
                                $$ = HLSL_MODIFIER_IN;
                            }
                        | KW_OUT
                            {
                                $$ = HLSL_MODIFIER_OUT;
                            }
                        | KW_INOUT
                            {
                                $$ = HLSL_MODIFIER_IN | HLSL_MODIFIER_OUT;
                            }

type:                     base_type
                            {
                                $$ = $1;
                            }
                        | KW_VECTOR '<' base_type ',' C_INTEGER '>'
                            {
                                if ($3->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: vectors of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                if ($5 < 1 || $5 > 4)
                                {
                                    hlsl_message("Line %u: vector size must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }

                                $$ = new_hlsl_type(NULL, HLSL_CLASS_VECTOR, $3->base_type, $5, 1);
                            }
                        | KW_MATRIX '<' base_type ',' C_INTEGER ',' C_INTEGER '>'
                            {
                                if ($3->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: matrices of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                if ($5 < 1 || $5 > 4 || $7 < 1 || $7 > 4)
                                {
                                    hlsl_message("Line %u: matrix dimensions must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }

                                $$ = new_hlsl_type(NULL, HLSL_CLASS_MATRIX, $3->base_type, $5, $7);
                            }

base_type:                KW_VOID
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("void"), HLSL_CLASS_OBJECT, HLSL_TYPE_VOID, 1, 1);
                            }
                        | KW_SAMPLER
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("sampler"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_GENERIC;
                            }
                        | KW_SAMPLER1D
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("sampler1D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_1D;
                            }
                        | KW_SAMPLER2D
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("sampler2D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_2D;
                            }
                        | KW_SAMPLER3D
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("sampler3D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_3D;
                            }
                        | KW_SAMPLERCUBE
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("samplerCUBE"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_CUBE;
                            }
                        | TYPE_IDENTIFIER
                            {
                                struct hlsl_type *type;

                                TRACE("Type %s.\n", $1);
                                type = get_type(hlsl_ctx.cur_scope, $1, TRUE);
                                $$ = type;
                                d3dcompiler_free($1);
                            }
                        | KW_STRUCT TYPE_IDENTIFIER
                            {
                                struct hlsl_type *type;

                                TRACE("Struct type %s.\n", $2);
                                type = get_type(hlsl_ctx.cur_scope, $2, TRUE);
                                if (type->type != HLSL_CLASS_STRUCT)
                                {
                                    hlsl_message("Line %u: redefining %s as a structure.\n",
                                            hlsl_ctx.line_no, $2);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                }
                                else
                                {
                                    $$ = type;
                                }
                                d3dcompiler_free($2);
                            }

declaration_statement:    declaration
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }

declaration:              var_modifiers type variables_def ';'
                            {
                                struct parse_variable_def *v, *v_next;
                                struct hlsl_ir_var *var;
                                BOOL ret, local = TRUE;

                                LIST_FOR_EACH_ENTRY_SAFE(v, v_next, $3, struct parse_variable_def, entry)
                                {
                                    debug_dump_decl($2, $1, v->name, hlsl_ctx.line_no);
                                    var = d3dcompiler_alloc(sizeof(*var));
                                    var->node.type = HLSL_IR_VAR;
                                    if (v->array_size)
                                        var->node.data_type = new_array_type($2, v->array_size);
                                    else
                                        var->node.data_type = $2;
                                    var->node.loc = v->loc;
                                    var->name = v->name;
                                    var->modifiers = $1;
                                    var->semantic = v->semantic;

                                    if (hlsl_ctx.cur_scope == hlsl_ctx.globals)
                                    {
                                        var->modifiers |= HLSL_STORAGE_UNIFORM;
                                        local = FALSE;
                                    }

                                    if (var->modifiers & HLSL_MODIFIER_CONST && !v->initializer)
                                    {
                                        hlsl_report_message(v->loc.file, v->loc.line, v->loc.col,
                                                HLSL_LEVEL_ERROR, "const variable without initializer");
                                        free_declaration(var);
                                        d3dcompiler_free(v);
                                        continue;
                                    }

                                    ret = declare_variable(var, local);
                                    if (!ret)
                                        free_declaration(var);
                                    else
                                        TRACE("Declared variable %s.\n", var->name);

                                    if (v->initializer)
                                    {
                                        FIXME("Variable with an initializer.\n");
                                        free_instr_list(v->initializer);
                                    }

                                    d3dcompiler_free(v);
                                }
                                d3dcompiler_free($3);
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

variable_def:             any_identifier array semantic
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                set_location(&$$->loc, &@1);
                                $$->name = $1;
                                $$->array_size = $2;
                                $$->semantic = $3;
                            }
                        | any_identifier array semantic '=' complex_initializer
                            {
                                TRACE("Declaration with initializer.\n");
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                set_location(&$$->loc, &@1);
                                $$->name = $1;
                                $$->array_size = $2;
                                $$->semantic = $3;
                                $$->initializer = $5;
                            }

array:                    /* Empty */
                            {
                                $$ = 0;
                            }
                        | '[' expr ']'
                            {
                                FIXME("Array.\n");
                                $$ = 0;
                                free_instr($2);
                            }

var_modifiers:            /* Empty */
                            {
                                $$ = 0;
                            }
                        | KW_EXTERN var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_EXTERN, &@1);
                            }
                        | KW_NOINTERPOLATION var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_NOINTERPOLATION, &@1);
                            }
                        | KW_PRECISE var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_MODIFIER_PRECISE, &@1);
                            }
                        | KW_SHARED var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_SHARED, &@1);
                            }
                        | KW_GROUPSHARED var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_GROUPSHARED, &@1);
                            }
                        | KW_STATIC var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_STATIC, &@1);
                            }
                        | KW_UNIFORM var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_UNIFORM, &@1);
                            }
                        | KW_VOLATILE var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_VOLATILE, &@1);
                            }
                        | KW_CONST var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_MODIFIER_CONST, &@1);
                            }
                        | KW_ROW_MAJOR var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_MODIFIER_ROW_MAJOR, &@1);
                            }
                        | KW_COLUMN_MAJOR var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_MODIFIER_COLUMN_MAJOR, &@1);
                            }

complex_initializer:      initializer_expr
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &$1->entry);
                            }
                        | '{' initializer_expr_list '}'
                            {
                                $$ = $2;
                            }

initializer_expr:         assignment_expr
                            {
                                $$ = $1;
                            }

initializer_expr_list:    initializer_expr
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &$1->entry);
                            }
                        | initializer_expr_list ',' initializer_expr
                            {
                                $$ = $1;
                                list_add_tail($$, &$3->entry);
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
                            {
                                $$ = $1;
                            }
                        | expr_statement
                            {
                                $$ = $1;
                            }
                        | compound_statement
                            {
                                $$ = $1;
                            }

expr_statement:           ';'
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | expr ';'
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                if ($1)
                                    list_add_head($$, &$1->entry);
                            }

primary_expr:             C_FLOAT
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type("float", HLSL_CLASS_SCALAR, HLSL_TYPE_FLOAT, 1, 1);
                                c->v.value.f[0] = $1;
                                $$ = &c->node;
                            }
                        | C_INTEGER
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type("int", HLSL_CLASS_SCALAR, HLSL_TYPE_INT, 1, 1);
                                c->v.value.i[0] = $1;
                                $$ = &c->node;
                            }
                        | boolean
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type("bool", HLSL_CLASS_SCALAR, HLSL_TYPE_BOOL, 1, 1);
                                c->v.value.b[0] = $1;
                                $$ = &c->node;
                            }
                        | variable
                            {
                                struct hlsl_ir_deref *deref = new_var_deref($1);
                                if (deref)
                                {
                                    $$ = &deref->node;
                                    set_location(&$$->loc, &@1);
                                }
                                else
                                    $$ = NULL;
                            }
                        | '(' expr ')'
                            {
                                $$ = $2;
                            }

variable:                 VAR_IDENTIFIER
                            {
                                struct hlsl_ir_var *var;
                                var = get_variable(hlsl_ctx.cur_scope, $1);
                                if (!var)
                                {
                                    hlsl_message("Line %d: variable '%s' not declared\n",
                                            hlsl_ctx.line_no, $1);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                $$ = var;
                            }

postfix_expr:             primary_expr
                            {
                                $$ = $1;
                            }
                        | postfix_expr OP_INC
                            {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                operands[0] = $1;
                                operands[1] = operands[2] = NULL;
                                set_location(&loc, &@2);
                                $$ = &new_expr(HLSL_IR_BINOP_POSTINC, operands, &loc)->node;
                            }
                        | postfix_expr OP_DEC
                            {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                operands[0] = $1;
                                operands[1] = operands[2] = NULL;
                                set_location(&loc, &@2);
                                $$ = &new_expr(HLSL_IR_BINOP_POSTDEC, operands, &loc)->node;
                            }
                          /* "var_modifiers" doesn't make sense in this case, but it's needed
                             in the grammar to avoid shift/reduce conflicts. */
                        | var_modifiers type '(' initializer_expr_list ')'
                            {
                                struct hlsl_ir_constructor *constructor;

                                TRACE("%s constructor.\n", debug_hlsl_type($2));
                                if ($1)
                                {
                                    hlsl_message("Line %u: unexpected modifier in a constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                                if ($2->type > HLSL_CLASS_LAST_NUMERIC)
                                {
                                    hlsl_message("Line %u: constructors are allowed only for numeric data types.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                                if ($2->dimx * $2->dimy != components_count_expr_list($4))
                                {
                                    hlsl_message("Line %u: wrong number of components in constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }

                                constructor = d3dcompiler_alloc(sizeof(*constructor));
                                constructor->node.type = HLSL_IR_CONSTRUCTOR;
                                set_location(&constructor->node.loc, &@3);
                                constructor->node.data_type = $2;
                                constructor->arguments = $4;

                                $$ = &constructor->node;
                            }

unary_expr:               postfix_expr
                            {
                                $$ = $1;
                            }
                        | OP_INC unary_expr
                            {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                operands[0] = $2;
                                operands[1] = operands[2] = NULL;
                                set_location(&loc, &@1);
                                $$ = &new_expr(HLSL_IR_BINOP_PREINC, operands, &loc)->node;
                            }
                        | OP_DEC unary_expr
                            {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                operands[0] = $2;
                                operands[1] = operands[2] = NULL;
                                set_location(&loc, &@1);
                                $$ = &new_expr(HLSL_IR_BINOP_PREDEC, operands, &loc)->node;
                            }
                        | unary_op unary_expr
                            {
                                enum hlsl_ir_expr_op ops[] = {0, HLSL_IR_UNOP_NEG,
                                        HLSL_IR_UNOP_LOGIC_NOT, HLSL_IR_UNOP_BIT_NOT};
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                if ($1 == UNARY_OP_PLUS)
                                {
                                    $$ = $2;
                                }
                                else
                                {
                                    operands[0] = $2;
                                    operands[1] = operands[2] = NULL;
                                    set_location(&loc, &@1);
                                    $$ = &new_expr(ops[$1], operands, &loc)->node;
                                }
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

mul_expr:                 unary_expr
                            {
                                $$ = $1;
                            }
                        | mul_expr '*' unary_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_mul($1, $3, &loc)->node;
                            }
                        | mul_expr '/' unary_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_div($1, $3, &loc)->node;
                            }
                        | mul_expr '%' unary_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_mod($1, $3, &loc)->node;
                            }

add_expr:                 mul_expr
                            {
                                $$ = $1;
                            }
                        | add_expr '+' mul_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_add($1, $3, &loc)->node;
                            }
                        | add_expr '-' mul_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_sub($1, $3, &loc)->node;
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

relational_expr:          shift_expr
                            {
                                $$ = $1;
                            }
                        | relational_expr '<' shift_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_lt($1, $3, &loc)->node;
                            }
                        | relational_expr '>' shift_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_gt($1, $3, &loc)->node;
                            }
                        | relational_expr OP_LE shift_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_le($1, $3, &loc)->node;
                            }
                        | relational_expr OP_GE shift_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_ge($1, $3, &loc)->node;
                            }

equality_expr:            relational_expr
                            {
                                $$ = $1;
                            }
                        | equality_expr OP_EQ relational_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_eq($1, $3, &loc)->node;
                            }
                        | equality_expr OP_NE relational_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_ne($1, $3, &loc)->node;
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

assignment_expr:          conditional_expr
                            {
                                $$ = $1;
                            }
                        | unary_expr assign_op assignment_expr
                            {
                                $$ = make_assignment($1, $2, BWRITERSP_WRITEMASK_ALL, $3);
                                if (!$$)
                                    return 1;
                                set_location(&$$->loc, &@2);
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
                                FIXME("Comma expression\n");
                            }

%%

static void set_location(struct source_location *loc, const struct YYLTYPE *l)
{
    loc->file = hlsl_ctx.source_file;
    loc->line = l->first_line;
    loc->col = l->first_column;
}

static DWORD add_modifier(DWORD modifiers, DWORD mod, const struct YYLTYPE *loc)
{
    if (modifiers & mod)
    {
        hlsl_report_message(hlsl_ctx.source_file, loc->first_line, loc->first_column, HLSL_LEVEL_ERROR,
                "modifier '%s' already specified", debug_modifiers(mod));
        return modifiers;
    }
    if (mod & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR)
            && modifiers & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR))
    {
        hlsl_report_message(hlsl_ctx.source_file, loc->first_line, loc->first_column, HLSL_LEVEL_ERROR,
                "more than one matrix majority keyword");
        return modifiers;
    }
    return modifiers | mod;
}

struct bwriter_shader *parse_hlsl(enum shader_type type, DWORD major, DWORD minor,
        const char *entrypoint, char **messages)
{
    struct hlsl_ir_function_decl *function;
    struct hlsl_scope *scope, *next_scope;
    struct hlsl_type *hlsl_type, *next_type;
    struct hlsl_ir_var *var, *next_var;
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
    list_init(&hlsl_ctx.functions);

    push_scope(&hlsl_ctx);
    hlsl_ctx.globals = hlsl_ctx.cur_scope;

    hlsl_parse();

    if (TRACE_ON(hlsl_parser))
    {
        struct hlsl_ir_function_decl *func;

        TRACE("IR dump.\n");
        LIST_FOR_EACH_ENTRY(func, &hlsl_ctx.functions, struct hlsl_ir_function_decl, node.entry)
        {
            if (func->body)
                debug_dump_ir_function(func);
        }
    }

    TRACE("Compilation status = %d\n", hlsl_ctx.status);
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
    LIST_FOR_EACH_ENTRY(function, &hlsl_ctx.functions, struct hlsl_ir_function_decl, node.entry)
        free_function(function);

    TRACE("Freeing variables.\n");
    LIST_FOR_EACH_ENTRY_SAFE(scope, next_scope, &hlsl_ctx.scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY_SAFE(var, next_var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            free_declaration(var);
        }
        d3dcompiler_free(scope);
    }

    TRACE("Freeing types.\n");
    LIST_FOR_EACH_ENTRY_SAFE(hlsl_type, next_type, &hlsl_ctx.types, struct hlsl_type, entry)
    {
        free_hlsl_type(hlsl_type);
    }

    return NULL;
}
