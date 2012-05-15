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

int hlsl_lex(void)
{
    FIXME("Lexer.\n");
    return 0;
}

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

%}

%error-verbose

%union
{
    char *name;
    INT intval;
}

%token <intval> PRE_LINE

%token <name> STRING
%%

hlsl_prog:                /* empty */
                            {
                            }
                        | hlsl_prog preproc_directive
                            {
                            }

preproc_directive:        PRE_LINE STRING
                            {
                                TRACE("Updating line informations to file %s, line %u\n", debugstr_a($2), $1);
                                hlsl_ctx.line_no = $1 - 1;
                                d3dcompiler_free(hlsl_ctx.source_file);
                                hlsl_ctx.source_file = $2;
                            }

%%

struct bwriter_shader *parse_hlsl_shader(const char *text, enum shader_type type, DWORD version, const char *entrypoint, char **messages)
{
    hlsl_ctx.line_no = 1;
    hlsl_ctx.matrix_majority = HLSL_COLUMN_MAJOR;

    hlsl_parse();

    return NULL;
}
