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

#include <stdio.h>

WINE_DEFAULT_DEBUG_CHANNEL(asmshader);

struct asm_parser asm_ctx;

/* Needed lexer functions declarations */
void asmshader_error(const char *s);
int asmshader_lex(void);

void set_rel_reg(struct shader_reg *reg, struct rel_reg *rel) {
    reg->rel_reg = NULL;
}

%}

%union {
    unsigned int        regnum;
    struct shader_reg   reg;
    struct {
        DWORD           swizzle;
        DWORD           writemask;
    } swizzle_wmask;
    DWORD               writemask;
    DWORD               swizzle;
    struct {
        DWORD           mod;
        DWORD           shift;
    } modshift;
    struct rel_reg      rel_reg;
    struct src_regs     sregs;
}

/* Common instructions between vertex and pixel shaders */
%token INSTR_MOV

/* Registers */
%token <regnum> REG_TEMP
%token <regnum> REG_CONSTFLOAT

/* Version tokens */
%token VER_VS10
%token VER_VS11
%token VER_VS20
%token VER_VS2X
%token VER_VS30

%token VER_PS10
%token VER_PS11
%token VER_PS12
%token VER_PS13
%token VER_PS14
%token VER_PS20
%token VER_PS2X
%token VER_PS30


%type <reg> dreg_name
%type <reg> dreg
%type <reg> sreg_name
%type <reg> sreg
%type <swizzle> swizzle
%type <modshift> omods
%type <rel_reg> rel_reg
%type <sregs> sregs

%%

shader:               version_marker instructions
                        {
                            asm_ctx.funcs->end(&asm_ctx);
                        }

version_marker:       VER_VS10
                        {
                            TRACE("Vertex shader 1.0\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_VS11
                        {
                            TRACE("Vertex shader 1.1\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_VS20
                        {
                            TRACE("Vertex shader 2.0\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_VS2X
                        {
                            TRACE("Vertex shader 2.x\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_VS30
                        {
                            TRACE("Vertex shader 3.0\n");
                            create_vs30_parser(&asm_ctx);
                        }
                    | VER_PS10
                        {
                            TRACE("Pixel  shader 1.0\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_PS11
                        {
                            TRACE("Pixel  shader 1.1\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_PS12
                        {
                            TRACE("Pixel  shader 1.2\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_PS13
                        {
                            TRACE("Pixel  shader 1.3\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_PS14
                        {
                            TRACE("Pixel  shader 1.4\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_PS20
                        {
                            TRACE("Pixel  shader 2.0\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_PS2X
                        {
                            TRACE("Pixel  shader 2.x\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }
                    | VER_PS30
                        {
                            TRACE("Pixel  shader 3.0\n");
                            set_parse_status(&asm_ctx, PARSE_ERR);
                            YYABORT;
                        }

instructions:         /* empty */
                    | instructions complexinstr
                            {
                                /* Nothing to do */
                            }

complexinstr:         instruction
                            {

                            }

instruction:          INSTR_MOV omods dreg ',' sregs
                            {
                                TRACE("MOV\n");
                                asm_ctx.funcs->instr(&asm_ctx, BWRITERSIO_MOV, $2.mod, $2.shift, 0, &$3, &$5, 1);
                            }

dreg:                 dreg_name rel_reg
                            {
                                $$.regnum = $1.regnum;
                                $$.type = $1.type;
                                $$.writemask = BWRITERSP_WRITEMASK_ALL;
                                $$.srcmod = BWRITERSPSM_NONE;
                                set_rel_reg(&$$, &$2);
                            }

dreg_name:            REG_TEMP
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_TEMP;
                        }

swizzle:              /* empty */
                        {
                            $$ = BWRITERVS_NOSWIZZLE;
                            TRACE("Default swizzle: %08x\n", $$);
                        }

omods:                 /* Empty */
                        {
                            $$.mod = 0;
                            $$.shift = 0;
                        }

sregs:                sreg
                        {
                            $$.reg[0] = $1;
                            $$.count = 1;
                        }
                    | sregs ',' sreg
                        {
                            if($$.count == MAX_SRC_REGS){
                                asmparser_message(&asm_ctx, "Line %u: Too many source registers in this instruction\n",
                                                  asm_ctx.line_no);
                                set_parse_status(&asm_ctx, PARSE_ERR);
                            }
                            else
                                $$.reg[$$.count++] = $3;
                        }

sreg:                   sreg_name rel_reg swizzle
                        {
                            $$.type = $1.type;
                            $$.regnum = $1.regnum;
                            $$.swizzle = $3;
                            $$.srcmod = BWRITERSPSM_NONE;
                            set_rel_reg(&$$, &$2);
                        }

rel_reg:               /* empty */
                        {
                            $$.has_rel_reg = FALSE;
                            $$.additional_offset = 0;
                        }

sreg_name:            REG_TEMP
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_TEMP;
                        }
                    | REG_CONSTFLOAT
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_CONST;
                        }

%%

void asmshader_error (char const *s) {
    asmparser_message(&asm_ctx, "Line %u: Error \"%s\" from bison\n", asm_ctx.line_no, s);
    set_parse_status(&asm_ctx, PARSE_ERR);
}

/* Error reporting function */
void asmparser_message(struct asm_parser *ctx, const char *fmt, ...) {
    va_list args;
    char* newbuffer;
    int rc, newsize;

    if(ctx->messagecapacity == 0) {
        ctx->messages = asm_alloc(MESSAGEBUFFER_INITIAL_SIZE);
        if(ctx->messages == NULL) {
            ERR("Error allocating memory for parser messages\n");
            return;
        }
        ctx->messagecapacity = MESSAGEBUFFER_INITIAL_SIZE;
    }

    while(1) {
        va_start(args, fmt);
        rc = vsnprintf(ctx->messages + ctx->messagesize,
                       ctx->messagecapacity - ctx->messagesize, fmt, args);
        va_end(args);

        if (rc < 0 ||                                           /* C89 */
            rc >= ctx->messagecapacity - ctx->messagesize) {    /* C99 */
            /* Resize the buffer */
            newsize = ctx->messagecapacity * 2;
            newbuffer = asm_realloc(ctx->messages, newsize);
            if(newbuffer == NULL){
                ERR("Error reallocating memory for parser messages\n");
                return;
            }
            ctx->messages = newbuffer;
            ctx->messagecapacity = newsize;
        } else {
            ctx->messagesize += rc;
            return;
        }
    }
}

/* New status is the worst between current status and parameter value */
void set_parse_status(struct asm_parser *ctx, enum parse_status status) {
    if(status == PARSE_ERR) ctx->status = PARSE_ERR;
    else if(status == PARSE_WARN && ctx->status == PARSE_SUCCESS) ctx->status = PARSE_WARN;
}

struct bwriter_shader *parse_asm_shader(char **messages) {
    struct bwriter_shader *ret = NULL;

    asm_ctx.shader = NULL;
    asm_ctx.status = PARSE_SUCCESS;
    asm_ctx.messagesize = asm_ctx.messagecapacity = 0;
    asm_ctx.line_no = 1;

    asmshader_parse();

    if(asm_ctx.status != PARSE_ERR) ret = asm_ctx.shader;
    else if(asm_ctx.shader) SlDeleteShader(asm_ctx.shader);

    if(messages) {
        if(asm_ctx.messagesize) {
            /* Shrink the buffer to the used size */
            *messages = asm_realloc(asm_ctx.messages, asm_ctx.messagesize + 1);
            if(!*messages) {
                ERR("Out of memory, no messages reported\n");
                asm_free(asm_ctx.messages);
            }
        } else {
            *messages = NULL;
        }
    } else {
        if(asm_ctx.messagecapacity) asm_free(asm_ctx.messages);
    }

    return ret;
}
