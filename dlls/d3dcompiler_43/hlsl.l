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
#include "wine/port.h"
#include "wine/debug.h"

#define YY_NO_UNISTD_H
#include "d3dcompiler_private.h"
#include "hlsl.tab.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlsl_parser);
%}

%option noyywrap nounput noinput
%option prefix="hlsl_"

%x pp pp_line pp_pragma pp_ignore

RESERVED1               auto|case|catch|char|class|const_cast|default|delete|dynamic_cast|enum
RESERVED2               explicit|friend|goto|long|mutable|new|operator|private|protected|public
RESERVED3               reinterpret_cast|short|signed|sizeof|static_cast|template|this|throw|try
RESERVED4               typename|union|unsigned|using|virtual

WS                      [ \t]
NEWLINE                 (\n)|(\r\n)
STRING                  \"[^\"]*\"

ANY                     (.)

%%
{RESERVED1}             {
                            hlsl_message("Line %u: Reserved keyword \"%s\" used.\n", hlsl_ctx.line_no, yytext);
                            set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                        }
{RESERVED2}             {
                            hlsl_message("Line %u: Reserved keyword \"%s\" used.\n", hlsl_ctx.line_no, yytext);
                            set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                        }
{RESERVED3}             {
                            hlsl_message("Line %u: Reserved keyword \"%s\" used.\n", hlsl_ctx.line_no, yytext);
                            set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                        }
{RESERVED4}             {
                            hlsl_message("Line %u: Reserved keyword \"%s\" used.\n", hlsl_ctx.line_no, yytext);
                            set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                        }

{WS}+                   {}
{NEWLINE}               {
                            hlsl_ctx.line_no++;
                        }

^#                      {
                            BEGIN pp;
                        }

<pp>pragma{WS}+         {
                            TRACE("Got a #pragma.\n");
                            BEGIN pp_pragma;
                        }
<pp_pragma>pack_matrix{WS}*\({WS}*row_major{WS}*\) {
                            TRACE("#pragma setting row_major mode.\n");
                            hlsl_ctx.matrix_majority = HLSL_ROW_MAJOR;
                            BEGIN pp_ignore;
                        }
<pp_pragma>pack_matrix{WS}*\({WS}*column_major{WS}*\) {
                            TRACE("#pragma setting column_major mode.\n");
                            hlsl_ctx.matrix_majority = HLSL_COLUMN_MAJOR;
                            BEGIN pp_ignore;
                        }
<pp_pragma>{NEWLINE}    {
                            FIXME("Unsupported preprocessor #pragma directive at line %u.\n", hlsl_ctx.line_no);
                            BEGIN INITIAL;
                        }
<pp_pragma>{ANY}        {}
<pp>[0-9]+              {
                            TRACE("Preprocessor line info.\n");
                            BEGIN pp_line;
                            hlsl_lval.intval = (atoi(yytext));
                            return PRE_LINE;
                        }
<pp_line>{STRING}       {
                            char *string = d3dcompiler_strdup(yytext + 1);

                            BEGIN pp_ignore;
                            string[strlen(string) - 1] = 0;
                            hlsl_lval.name = string;
                            return STRING;
                        }
<pp_line>{WS}+          {}
<pp_line>{NEWLINE}      {
                            FIXME("Malformed preprocessor line directive?\n");
                            BEGIN INITIAL;
                        }
<pp_ignore>{NEWLINE}    {
                            BEGIN INITIAL;
                        }
<pp_ignore>{ANY}        {}
<pp>{NEWLINE}           {
                            FIXME("Unexpected preprocessor directive.\n");
                            BEGIN INITIAL;
                        }
<pp>{ANY}               {}

{ANY}                   {
                            return yytext[0];
                        }

%%

struct bwriter_shader *parse_hlsl(enum shader_type type, DWORD version, const char *entrypoint, char **messages);

struct bwriter_shader *parse_hlsl_shader(const char *text, enum shader_type type, DWORD version,
        const char *entrypoint, char **messages)
{
    struct bwriter_shader *ret = NULL;
    YY_BUFFER_STATE buffer;

    buffer = hlsl__scan_string(text);
    hlsl__switch_to_buffer(buffer);

    ret = parse_hlsl(type, version, entrypoint, messages);

    hlsl__delete_buffer(buffer);
    return ret;
}
