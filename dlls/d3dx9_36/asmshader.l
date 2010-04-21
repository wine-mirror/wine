/*
 * Direct3D shader assembler
 *
 * Copyright 2008 Stefan Dösinger
 * Copyright 2009 Matteo Bruni
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

#include "d3dx9_36_private.h"
#include "asmshader.tab.h"

WINE_DEFAULT_DEBUG_CHANNEL(asmshader);
%}

%option noyywrap
%option prefix="asmshader_"
%option noinput nounput

/* Swizzles and writemasks consist of a dot and up to 4 x, y, z or w characters,
 * or up to 4 a, r, g, b characters. There are different rules for swizzles and
 * writemasks wrt repetition, those are handled in the grammar.
 */
DOT                     \.
COMPONENT               [xyzw]|[rgba]

/* Registers */
REG_TEMP                r[0-9]+
/* for relative addressing in the form o[x], v[x] and c[x] */
REG_CONSTFLOAT          c[0-9]*

PREPROCESSORDIRECTIVE   #[^\n]*\n

/* Comments */
DOUBLESLASHCOMMENT      "//"[^\n]*
SEMICOLONCOMMENT        ";"[^\n]*

/* Whitespaces are spaces, tabs and newlines */
WHITESPACE              [ \t]+
NEWLINE                 (\n)|(\r\n)

COMMA                   ","

IMMVAL                  \-?(([0-9]+)|([0-9]*\.[0-9]+))(f)?

ANY                     (.)

%%

    /* Common instructions(vertex and pixel shaders) */
mov                     {return INSTR_MOV;          }

{REG_TEMP}              {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_TEMP;
                        }
{REG_CONSTFLOAT}        {
                            asmshader_lval.regnum = atoi(yytext + 1);
                            return REG_CONSTFLOAT;
                        }

    /* Shader versions. These are important to select the correct
     * parser profile.
     */
vs\.1\.0|vs_1_0         {return VER_VS10;       }
vs\.1\.1|vs_1_1         {return VER_VS11;       }

vs_2_0                  {return VER_VS20;       }
vs_2_x                  {return VER_VS2X;       }
vs_3_0                  {return VER_VS30;       }

ps\.1\.0|ps_1_0         {return VER_PS10;       }
ps\.1\.1|ps_1_1         {return VER_PS11;       }
ps\.1\.2|ps_1_2         {return VER_PS12;       }
ps\.1\.3|ps_1_3         {return VER_PS13;       }
ps\.1\.4|ps_1_4         {return VER_PS14;       }

ps_2_0                  {return VER_PS20;       }
ps_2_x                  {return VER_PS2X;       }
ps_3_0                  {return VER_PS30;       }

{DOT}                   {return yytext[0];      }
{COMPONENT}             {
                            switch(yytext[0]) {
                                case 'x':
                                case 'r':
                                    asmshader_lval.component = 0;
                                    break;
                                case 'y':
                                case 'g':
                                    asmshader_lval.component = 1;
                                    break;
                                case 'z':
                                case 'b':
                                    asmshader_lval.component = 2;
                                    break;
                                case 'w':
                                case 'a':
                                    asmshader_lval.component = 3;
                                    break;
                            }
                            return COMPONENT;
                        }

{COMMA}                 {return yytext[0];          }
-                       {return yytext[0];          }
\(                      {return yytext[0];          }
\)                      {return yytext[0];          }

{PREPROCESSORDIRECTIVE} {
                            /* TODO: update current line information */
                            TRACE("line info update: %s", yytext);
                        }

    /* Skip comments */
{DOUBLESLASHCOMMENT}    {                           }
{SEMICOLONCOMMENT}      {                           }

{WHITESPACE}            { /* Do nothing */          }
{NEWLINE}               {
                            asm_ctx.line_no++;
                        }

{ANY}                   {
                            asmparser_message(&asm_ctx, "Line %u: Unexpected input %s\n", asm_ctx.line_no, yytext);
                            set_parse_status(&asm_ctx, PARSE_ERR);
                        }

%%

struct bwriter_shader *SlAssembleShader(const char *text, char **messages) {
    struct bwriter_shader *ret = NULL;
    YY_BUFFER_STATE buffer;
    TRACE("%p, %p\n", text, messages);

    buffer = asmshader__scan_string(text);
    asmshader__switch_to_buffer(buffer);

    ret = parse_asm_shader(messages);

    asmshader__delete_buffer(buffer);

    return ret;
}
