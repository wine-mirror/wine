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
    /* We can have an additional offset without true relative addressing
     * ex. c2[ 4 ] */
    reg->regnum += rel->additional_offset;
    if(!rel->has_rel_reg) {
        reg->rel_reg = NULL;
    } else {
        reg->rel_reg = asm_alloc(sizeof(*reg->rel_reg));
        if(!reg->rel_reg) {
            return;
        }
        reg->rel_reg->type = rel->type;
        reg->rel_reg->swizzle = rel->swizzle;
        reg->rel_reg->regnum = rel->rel_regnum;
    }
}

%}

%union {
    struct {
        float           val;
        BOOL            integer;
    } immval;
    unsigned int        regnum;
    struct shader_reg   reg;
    DWORD               srcmod;
    DWORD               writemask;
    struct {
        DWORD           writemask;
        DWORD           idx;
        DWORD           last;
    } wm_components;
    DWORD               swizzle;
    struct {
        DWORD           swizzle;
        DWORD           idx;
    } sw_components;
    DWORD               component;
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
%token <regnum> REG_OUTPUT
%token <regnum> REG_INPUT
%token <regnum> REG_CONSTFLOAT
%token <regnum> REG_CONSTINT
%token <regnum> REG_CONSTBOOL
%token <regnum> REG_TEXTURE
%token <regnum> REG_SAMPLER
%token <regnum> REG_TEXCRDOUT
%token REG_OPOS
%token REG_OFOG
%token REG_OPTS
%token <regnum> REG_VERTEXCOLOR
%token <regnum> REG_FRAGCOLOR
%token REG_FRAGDEPTH
%token REG_VPOS
%token REG_VFACE
%token REG_ADDRESS
%token REG_LOOP
%token REG_PREDICATE
%token <regnum> REG_LABEL

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

/* Output modifiers */
%token MOD_SAT
%token MOD_PP
%token MOD_CENTROID

/* Source register modifiers */
%token SMOD_ABS

/* Misc stuff */
%token <component> COMPONENT
%token <immval> IMMVAL

%type <reg> dreg_name
%type <reg> dreg
%type <reg> sreg_name
%type <reg> relreg_name
%type <reg> sreg
%type <srcmod> smod
%type <writemask> writemask
%type <wm_components> wm_components
%type <swizzle> swizzle
%type <sw_components> sw_components
%type <modshift> omods
%type <modshift> omodifier
%type <rel_reg> rel_reg
%type <immval> immsum
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
                    | dreg_name writemask
                            {
                                $$.regnum = $1.regnum;
                                $$.type = $1.type;
                                $$.writemask = $2;
                                $$.srcmod = BWRITERSPSM_NONE;
                                $$.rel_reg = NULL;
                            }

dreg_name:            REG_TEMP
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_TEMP;
                        }
                    | REG_OUTPUT
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_OUTPUT;
                        }
                    | REG_INPUT
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register v%u is not a valid destination register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_CONSTFLOAT
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register c%u is not a valid destination register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_CONSTINT
                        {
			  asmparser_message(&asm_ctx, "Line %u: Register i%u is not a valid destination register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_CONSTBOOL
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register b%u is not a valid destination register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_TEXTURE
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_TEXTURE;
                        }
                    | REG_TEXCRDOUT
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_TEXCRDOUT;
                        }
                    | REG_SAMPLER
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register s%u is not a valid destination register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_OPOS
                        {
                            $$.regnum = BWRITERSRO_POSITION; $$.type = BWRITERSPR_RASTOUT;
                        }
                    | REG_OPTS
                        {
                            $$.regnum = BWRITERSRO_POINT_SIZE; $$.type = BWRITERSPR_RASTOUT;
                        }
                    | REG_OFOG
                        {
                            $$.regnum = BWRITERSRO_FOG; $$.type = BWRITERSPR_RASTOUT;
                        }
                    | REG_VERTEXCOLOR
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_ATTROUT;
                        }
                    | REG_FRAGCOLOR
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_COLOROUT;
                        }
                    | REG_FRAGDEPTH
                        {
                            $$.regnum = 0; $$.type = BWRITERSPR_DEPTHOUT;
                        }
                    | REG_PREDICATE
                        {
                            $$.regnum = 0; $$.type = BWRITERSPR_PREDICATE;
                        }
                    | REG_VPOS
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register vPos is not a valid destination register\n",
                                              asm_ctx.line_no);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_VFACE
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register vFace is not a valid destination register\n",
                                              asm_ctx.line_no);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_ADDRESS
                        {
                            /* index 0 is hardcoded for the addr register */
                            $$.regnum = 0; $$.type = BWRITERSPR_ADDR;
                        }
                    | REG_LOOP
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register aL is not a valid destination register\n",
                                              asm_ctx.line_no);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }

writemask:            '.' wm_components
                        {
                            if($2.writemask == SWIZZLE_ERR) {
                                asmparser_message(&asm_ctx, "Line %u: Invalid writemask specified\n",
                                                  asm_ctx.line_no);
                                set_parse_status(&asm_ctx, PARSE_ERR);
                                /* Provide a correct writemask to prevent following complaints */
                                $$ = BWRITERSP_WRITEMASK_ALL;
                            }
                            else {
                                $$ = $2.writemask;
                                TRACE("Writemask: %x\n", $$);
                            }
                        }

wm_components:        COMPONENT
                        {
                            $$.writemask = 1 << $1;
                            $$.last = $1;
                            $$.idx = 1;
                        }
                    | wm_components COMPONENT
                        {
                            if($1.writemask == SWIZZLE_ERR || $1.idx == 4)
                                /* Wrong writemask */
                                $$.writemask = SWIZZLE_ERR;
                            else {
                                if($2 <= $1.last)
                                    $$.writemask = SWIZZLE_ERR;
                                else {
                                    $$.writemask = $1.writemask | (1 << $2);
                                    $$.idx = $1.idx + 1;
                                }
                            }
                        }

swizzle:              /* empty */
                        {
                            $$ = BWRITERVS_NOSWIZZLE;
                            TRACE("Default swizzle: %08x\n", $$);
                        }
                    | '.' sw_components
                        {
                            if($2.swizzle == SWIZZLE_ERR) {
                                asmparser_message(&asm_ctx, "Line %u: Invalid swizzle\n",
                                                  asm_ctx.line_no);
                                set_parse_status(&asm_ctx, PARSE_ERR);
                                /* Provide a correct swizzle to prevent following complaints */
                                $$ = BWRITERVS_NOSWIZZLE;
                            }
                            else {
                                DWORD last, i;

                                $$ = $2.swizzle << BWRITERVS_SWIZZLE_SHIFT;
                                /* Fill the swizzle by extending the last component */
                                last = ($2.swizzle >> 2 * ($2.idx - 1)) & 0x03;
                                for(i = $2.idx; i < 4; i++){
                                    $$ |= last << (BWRITERVS_SWIZZLE_SHIFT + 2 * i);
                                }
                                TRACE("Got a swizzle: %08x\n", $$);
                            }
                        }

sw_components:        COMPONENT
                        {
                            $$.swizzle = $1;
                            $$.idx = 1;
                        }
                    | sw_components COMPONENT
                        {
                            if($1.idx == 4) {
                                /* Too many sw_components */
                                $$.swizzle = SWIZZLE_ERR;
                                $$.idx = 4;
                            }
                            else {
                                $$.swizzle = $1.swizzle | ($2 << 2 * $1.idx);
                                $$.idx = $1.idx + 1;
                            }
                        }

omods:                 /* Empty */
                        {
                            $$.mod = 0;
                            $$.shift = 0;
                        }
                    | omods omodifier
                        {
                            $$.mod = $1.mod | $2.mod;
                            if($1.shift && $2.shift) {
                                asmparser_message(&asm_ctx, "Line %u: More than one shift flag\n",
                                                  asm_ctx.line_no);
                                set_parse_status(&asm_ctx, PARSE_ERR);
                                $$.shift = $1.shift;
                            } else {
                                $$.shift = $1.shift | $2.shift;
                            }
                        }

omodifier:            MOD_SAT
                        {
                            $$.mod = BWRITERSPDM_SATURATE;
                            $$.shift = 0;
                        }
                    | MOD_PP
                        {
                            $$.mod = BWRITERSPDM_PARTIALPRECISION;
                            $$.shift = 0;
                        }
                    | MOD_CENTROID
                        {
                            $$.mod = BWRITERSPDM_MSAMPCENTROID;
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
                    | sreg_name rel_reg smod swizzle
                        {
                            $$.type = $1.type;
                            $$.regnum = $1.regnum;
                            set_rel_reg(&$$, &$2);
                            $$.srcmod = $3;
                            $$.swizzle = $4;
                        }
                    | '-' sreg_name rel_reg swizzle
                        {
                            $$.type = $2.type;
                            $$.regnum = $2.regnum;
                            $$.srcmod = BWRITERSPSM_NEG;
                            set_rel_reg(&$$, &$3);
                            $$.swizzle = $4;
                        }
                    | '-' sreg_name rel_reg smod swizzle
                        {
                            $$.type = $2.type;
                            $$.regnum = $2.regnum;
                            set_rel_reg(&$$, &$3);
                            switch($4) {
                                case BWRITERSPSM_ABS:  $$.srcmod = BWRITERSPSM_ABSNEG;  break;
                                default:
                                    FIXME("Unhandled combination of NEGATE and %u\n", $4);
                            }
                            $$.swizzle = $5;
                        }

rel_reg:               /* empty */
                        {
                            $$.has_rel_reg = FALSE;
                            $$.additional_offset = 0;
                        }
                    | '[' immsum ']'
                        {
                            $$.has_rel_reg = FALSE;
                            $$.additional_offset = $2.val;
                        }
                    | '[' relreg_name swizzle ']'
                        {
                            $$.has_rel_reg = TRUE;
                            $$.type = $2.type;
                            $$.additional_offset = 0;
                            $$.rel_regnum = $2.regnum;
                            $$.swizzle = $3;
                        }
                    | '[' immsum '+' relreg_name swizzle ']'
                        {
                            $$.has_rel_reg = TRUE;
                            $$.type = $4.type;
                            $$.additional_offset = $2.val;
                            $$.rel_regnum = $4.regnum;
                            $$.swizzle = $5;
                        }
                    | '[' relreg_name swizzle '+' immsum ']'
                        {
                            $$.has_rel_reg = TRUE;
                            $$.type = $2.type;
                            $$.additional_offset = $5.val;
                            $$.rel_regnum = $2.regnum;
                            $$.swizzle = $3;
                        }
                    | '[' immsum '+' relreg_name swizzle '+' immsum ']'
                        {
                            $$.has_rel_reg = TRUE;
                            $$.type = $4.type;
                            $$.additional_offset = $2.val + $7.val;
                            $$.rel_regnum = $4.regnum;
                            $$.swizzle = $5;
                        }

immsum:               IMMVAL
                        {
                            if(!$1.integer) {
                                asmparser_message(&asm_ctx, "Line %u: Unexpected float %f\n",
                                                  asm_ctx.line_no, $1.val);
                                set_parse_status(&asm_ctx, PARSE_ERR);
                            }
                            $$.val = $1.val;
                        }
                    | immsum '+' IMMVAL
                        {
                            if(!$3.integer) {
                                asmparser_message(&asm_ctx, "Line %u: Unexpected float %f\n",
                                                  asm_ctx.line_no, $3.val);
                                set_parse_status(&asm_ctx, PARSE_ERR);
                            }
                            $$.val = $1.val + $3.val;
                        }

smod:                 SMOD_ABS
                        {
                            $$ = BWRITERSPSM_ABS;
                        }

relreg_name:          REG_ADDRESS
                        {
                            $$.regnum = 0; $$.type = BWRITERSPR_ADDR;
                        }
                    | REG_LOOP
                        {
                            $$.regnum = 0; $$.type = BWRITERSPR_LOOP;
                        }

sreg_name:            REG_TEMP
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_TEMP;
                        }
                    | REG_OUTPUT
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register o%u is not a valid source register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_INPUT
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_INPUT;
                        }
                    | REG_CONSTFLOAT
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_CONST;
                        }
                    | REG_CONSTINT
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_CONSTINT;
                        }
                    | REG_CONSTBOOL
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_CONSTBOOL;
                        }
                    | REG_TEXTURE
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_TEXTURE;
                        }
                    | REG_TEXCRDOUT
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register oT%u is not a valid source register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_SAMPLER
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_SAMPLER;
                        }
                    | REG_OPOS
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register oPos is not a valid source register\n",
                                              asm_ctx.line_no);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_OFOG
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register oFog is not a valid source register\n",
                                              asm_ctx.line_no);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_VERTEXCOLOR
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register oD%u is not a valid source register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_FRAGCOLOR
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register oC%u is not a valid source register\n",
                                              asm_ctx.line_no, $1);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_FRAGDEPTH
                        {
                            asmparser_message(&asm_ctx, "Line %u: Register oDepth is not a valid source register\n",
                                              asm_ctx.line_no);
                            set_parse_status(&asm_ctx, PARSE_WARN);
                        }
                    | REG_PREDICATE
                        {
                            $$.regnum = 0; $$.type = BWRITERSPR_PREDICATE;
                        }
                    | REG_VPOS
                        {
                            $$.regnum = 0; $$.type = BWRITERSPR_MISCTYPE;
                        }
                    | REG_VFACE
                        {
                            $$.regnum = 1; $$.type = BWRITERSPR_MISCTYPE;
                        }
                    | REG_ADDRESS
                        {
                            $$.regnum = 0; $$.type = BWRITERSPR_ADDR;
                        }
                    | REG_LOOP
                        {
                            $$.regnum = 0; $$.type = BWRITERSPR_LOOP;
                        }
                    | REG_LABEL
                        {
                            $$.regnum = $1; $$.type = BWRITERSPR_LABEL;
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
