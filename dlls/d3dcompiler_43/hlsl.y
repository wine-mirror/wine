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

void hlsl_message(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    compilation_message(&hlsl_ctx.messages, fmt, args);
    va_end(args);
}

static void hlsl_error(const char *s)
{
    hlsl_message("Line %u: %s\n", hlsl_ctx.line_no, s);
    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
}

static void debug_dump_decl(struct hlsl_type *type, DWORD modifiers, const char *declname, unsigned int line_no)
{
    TRACE("Line %u: ", line_no);
    TRACE("%s %s;\n", debug_hlsl_type(type), declname);
}

static BOOL declare_variable(struct hlsl_ir_var *decl, BOOL local)
{
    BOOL ret;

    TRACE("Declaring variable %s.\n", decl->name);
    ret = add_declaration(hlsl_ctx.cur_scope, decl, local);
    if (ret == FALSE)
    {
        struct hlsl_ir_var *old = get_variable(hlsl_ctx.cur_scope, decl->name);

        hlsl_message("Line %u: \"%s\" already declared.\n", hlsl_ctx.line_no, decl->name);
        hlsl_message("Line %u: \"%s\" was previously declared here.\n", old->node.line, decl->name);
        set_parse_status(&hlsl_ctx.status, PARSE_ERR);
        return FALSE;
    }
    return TRUE;
}

%}

%error-verbose

%union
{
    struct hlsl_type *type;
    INT intval;
    FLOAT floatval;
    char *name;
    DWORD modifiers;
    struct list *list;
    struct parse_variable_def *variable_def;
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
%token OP_LT
%token OP_GT
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
%type <name> any_identifier
%token <name> STRING
%token <floatval> C_FLOAT
%token <intval> C_INTEGER
%type <type> base_type
%type <type> type
%type <modifiers> var_modifiers
%type <intval> array
%type <name> semantic
%type <variable_def> variable_def
%type <list> variables_def
%%

hlsl_prog:                /* empty */
                            {
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
                                hlsl_ctx.line_no = $1 - 1;
                                d3dcompiler_free(hlsl_ctx.source_file);
                                hlsl_ctx.source_file = $2;
                            }

any_identifier:           VAR_IDENTIFIER
                        | TYPE_IDENTIFIER
                        | NEW_IDENTIFIER

semantic:                 /* Empty */
                            {
                                $$ = NULL;
                            }
                        | ':' any_identifier
                            {
                                $$ = $2;
                            }

type:                     base_type
                            {
                                $$ = $1;
                            }

base_type:                KW_VOID
                            {
                                $$ = new_hlsl_type("void", HLSL_TYPE_VOID, 1, 1);
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
                                if (type->base_type != HLSL_TYPE_STRUCT)
                                {
                                    hlsl_message("Line %u: Redefining %s as a structure.\n",
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
                                    var->name = v->name;
                                    var->modifiers = $1;
                                    var->semantic = v->semantic;
                                    var->node.line = hlsl_ctx.line_no;
                                    if (v->initializer)
                                    {
                                        FIXME("Variable with an initializer.\n");
                                    }

                                    ret = declare_variable(var, local);
                                    if (ret == FALSE)
                                        free_declaration(var);
                                    else
                                        TRACE("Declared variable %s.\n", var->name);
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

                          /* FIXME: Local variables can't have semantics. */
variable_def:             any_identifier array semantic
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                $$->name = $1;
                                $$->array_size = $2;
                                $$->semantic = $3;
                            }

array:                    /* Empty */
                            {
                                $$ = 0;
                            }

var_modifiers:            /* Empty */
                            {
                                $$ = 0;
                            }

%%

struct bwriter_shader *parse_hlsl(enum shader_type type, DWORD version, const char *entrypoint, char **messages)
{
    struct hlsl_scope *scope, *next_scope;
    struct hlsl_type *hlsl_type, *next_type;
    struct hlsl_ir_var *var, *next_var;

    hlsl_ctx.line_no = 1;
    hlsl_ctx.source_file = d3dcompiler_strdup("");
    hlsl_ctx.cur_scope = NULL;
    hlsl_ctx.matrix_majority = HLSL_COLUMN_MAJOR;
    list_init(&hlsl_ctx.scopes);
    list_init(&hlsl_ctx.types);
    list_init(&hlsl_ctx.functions);

    push_scope(&hlsl_ctx);
    hlsl_ctx.globals = hlsl_ctx.cur_scope;

    hlsl_parse();

    d3dcompiler_free(hlsl_ctx.source_file);

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
