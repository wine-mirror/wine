/*
 * Direct3D asm shader parser
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
 *
 */

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(asmshader);
WINE_DECLARE_DEBUG_CHANNEL(parsed_shader);


/* How to map vs 1.0 and 2.0 varyings to 3.0 ones
 * oTx is mapped to ox, which happens to be an
 * identical mapping since BWRITERSPR_TEXCRDOUT == BWRITERSPR_OUTPUT
 * oPos, oFog and point size are mapped to general output regs as well.
 * the vs 1.x and 2.x parser functions add varying declarations
 * to the shader, and the 1.x and 2.x output functions check those varyings
 */
#define OT0_REG         0
#define OT1_REG         1
#define OT2_REG         2
#define OT3_REG         3
#define OT4_REG         4
#define OT5_REG         5
#define OT6_REG         6
#define OT7_REG         7
#define OPOS_REG        8
#define OFOG_REG        9
#define OFOG_WRITEMASK  BWRITERSP_WRITEMASK_0
#define OPTS_REG        9
#define OPTS_WRITEMASK  BWRITERSP_WRITEMASK_1
#define OD0_REG         10
#define OD1_REG         11


/****************************************************************
 * Common(non-version specific) shader parser control code      *
 ****************************************************************/

static void asmparser_end(struct asm_parser *This) {
    TRACE("Finalizing shader\n");
}

static void asmparser_constF(struct asm_parser *This, DWORD reg, float x, float y, float z, float w) {
    if(!This->shader) return;
    TRACE("Adding float constant %u at pos %u\n", reg, This->shader->num_cf);
    TRACE_(parsed_shader)("def c%u, %f, %f, %f, %f\n", reg, x, y, z, w);
    if(!add_constF(This->shader, reg, x, y, z, w)) {
        ERR("Out of memory\n");
        set_parse_status(This, PARSE_ERR);
    }
}

static void asmparser_constB(struct asm_parser *This, DWORD reg, BOOL x) {
    if(!This->shader) return;
    TRACE("Adding boolean constant %u at pos %u\n", reg, This->shader->num_cb);
    TRACE_(parsed_shader)("def b%u, %s\n", reg, x ? "true" : "false");
    if(!add_constB(This->shader, reg, x)) {
        ERR("Out of memory\n");
        set_parse_status(This, PARSE_ERR);
    }
}

static void asmparser_constI(struct asm_parser *This, DWORD reg, INT x, INT y, INT z, INT w) {
    if(!This->shader) return;
    TRACE("Adding integer constant %u at pos %u\n", reg, This->shader->num_ci);
    TRACE_(parsed_shader)("def i%u, %d, %d, %d, %d\n", reg, x, y, z, w);
    if(!add_constI(This->shader, reg, x, y, z, w)) {
        ERR("Out of memory\n");
        set_parse_status(This, PARSE_ERR);
    }
}

static void asmparser_dcl_output(struct asm_parser *This, DWORD usage, DWORD num,
                                 const struct shader_reg *reg) {
    if(!This->shader) return;
    if(This->shader->type == ST_PIXEL) {
        asmparser_message(This, "Line %u: Output register declared in a pixel shader\n", This->line_no);
        set_parse_status(This, PARSE_ERR);
    }
    if(!record_declaration(This->shader, usage, num, TRUE, reg->regnum, reg->writemask)) {
        ERR("Out of memory\n");
        set_parse_status(This, PARSE_ERR);
    }
}

static void asmparser_dcl_input(struct asm_parser *This, DWORD usage, DWORD num,
                                const struct shader_reg *reg) {
    if(!This->shader) return;
    if(!record_declaration(This->shader, usage, num, FALSE, reg->regnum, reg->writemask)) {
        ERR("Out of memory\n");
        set_parse_status(This, PARSE_ERR);
    }
}

static void asmparser_dcl_sampler(struct asm_parser *This, DWORD samptype, DWORD regnum, unsigned int line_no) {
    if(!This->shader) return;
    if(!record_sampler(This->shader, samptype, regnum)) {
        ERR("Out of memory\n");
        set_parse_status(This, PARSE_ERR);
    }
}

static void asmparser_sincos(struct asm_parser *This, DWORD mod, DWORD shift,
                             const struct shader_reg *dst,
                             const struct src_regs *srcs) {
    struct instruction *instr;

    if(!srcs || srcs->count != 3) {
        asmparser_message(This, "Line %u: sincos (vs 2) has an incorrect number of source registers\n", This->line_no);
        set_parse_status(This, PARSE_ERR);
        return;
    }

    instr = alloc_instr(3);
    if(!instr) {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(This, PARSE_ERR);
        return;
    }

    instr->opcode = BWRITERSIO_SINCOS;
    instr->dstmod = mod;
    instr->shift = shift;
    instr->comptype = 0;

    This->funcs->dstreg(This, instr, dst);
    This->funcs->srcreg(This, instr, 0, &srcs->reg[0]);
    This->funcs->srcreg(This, instr, 1, &srcs->reg[1]);
    This->funcs->srcreg(This, instr, 2, &srcs->reg[2]);

    if(!add_instruction(This->shader, instr)) {
        ERR("Out of memory\n");
        set_parse_status(This, PARSE_ERR);
    }
}

static void asmparser_instr(struct asm_parser *This, DWORD opcode,
                            DWORD mod, DWORD shift,
                            BWRITER_COMPARISON_TYPE comp,
                            const struct shader_reg *dst,
                            const struct src_regs *srcs, int expectednsrcs) {
    struct instruction *instr;
    unsigned int i;
    BOOL firstreg = TRUE;
    unsigned int src_count = srcs ? srcs->count : 0;

    if(!This->shader) return;

    TRACE_(parsed_shader)("%s%s%s ", debug_print_opcode(opcode),
                          debug_print_dstmod(mod),
                          debug_print_comp(comp));
    if(dst) {
        TRACE_(parsed_shader)("%s", debug_print_dstreg(dst, This->shader->type));
        firstreg = FALSE;
    }
    for(i = 0; i < src_count; i++) {
        if(!firstreg) TRACE_(parsed_shader)(", ");
        else firstreg = FALSE;
        TRACE_(parsed_shader)("%s", debug_print_srcreg(&srcs->reg[i],
                                                       This->shader->type));
    }
    TRACE_(parsed_shader)("\n");

 /* Check for instructions with different syntaxes in different shader versio
ns */
    switch(opcode) {
        case BWRITERSIO_SINCOS:
            /* The syntax changes between vs 2 and the other shader versions */
            if(This->shader->version == BWRITERVS_VERSION(2, 0) ||
               This->shader->version == BWRITERVS_VERSION(2, 1)) {
                asmparser_sincos(This, mod, shift, dst, srcs);
                return;
            }
            /* Use the default handling */
            break;
    }

    if(src_count != expectednsrcs) {
        asmparser_message(This, "Line %u: Wrong number of source registers\n", This->line_no);
        set_parse_status(This, PARSE_ERR);
        return;
    }

    instr = alloc_instr(src_count);
    if(!instr) {
        ERR("Error allocating memory for the instruction\n");
        set_parse_status(This, PARSE_ERR);
        return;
    }

    instr->opcode = opcode;
    instr->dstmod = mod;
    instr->shift = shift;
    instr->comptype = comp;
    if(dst) This->funcs->dstreg(This, instr, dst);
    for(i = 0; i < src_count; i++) {
        This->funcs->srcreg(This, instr, i, &srcs->reg[i]);
    }

    if(!add_instruction(This->shader, instr)) {
        ERR("Out of memory\n");
        set_parse_status(This, PARSE_ERR);
    }
}

static struct shader_reg map_oldvs_register(const struct shader_reg *reg) {
    struct shader_reg ret;
    switch(reg->type) {
        case BWRITERSPR_RASTOUT:
            ret = *reg;
            ret.type = BWRITERSPR_OUTPUT;
            switch(reg->regnum) {
                case BWRITERSRO_POSITION:
                    ret.regnum = OPOS_REG;
                    break;
                case BWRITERSRO_FOG:
                    ret.regnum = OFOG_REG;
                    ret.writemask = OFOG_WRITEMASK;
                    break;
                case BWRITERSRO_POINT_SIZE:
                    ret.regnum = OPTS_REG;
                    ret.writemask = OPTS_WRITEMASK;
                    break;
                default:
                    FIXME("Unhandled RASTOUT register %u\n", reg->regnum);
                    return *reg;
            }
            return ret;

        case BWRITERSPR_TEXCRDOUT:
            ret = *reg;
            ret.type = BWRITERSPR_OUTPUT;
            switch(reg->regnum) {
                case 0: ret.regnum = OT0_REG; break;
                case 1: ret.regnum = OT1_REG; break;
                case 2: ret.regnum = OT2_REG; break;
                case 3: ret.regnum = OT3_REG; break;
                case 4: ret.regnum = OT4_REG; break;
                case 5: ret.regnum = OT5_REG; break;
                case 6: ret.regnum = OT6_REG; break;
                case 7: ret.regnum = OT7_REG; break;
                default:
                    FIXME("Unhandled TEXCRDOUT regnum %u\n", reg->regnum);
                    return *reg;
            }
            return ret;

        case BWRITERSPR_ATTROUT:
            ret = *reg;
            ret.type = BWRITERSPR_OUTPUT;
            switch(reg->regnum) {
                case 0: ret.regnum = OD0_REG; break;
                case 1: ret.regnum = OD1_REG; break;
                default:
                    FIXME("Unhandled ATTROUT regnum %u\n", reg->regnum);
                    return *reg;
            }
            return ret;

        default: return *reg;
    }
}

/* Checks for unsupported source modifiers in VS (all versions) or
   PS 2.0 and newer */
static void check_legacy_srcmod(struct asm_parser *This, DWORD srcmod) {
    if(srcmod == BWRITERSPSM_BIAS || srcmod == BWRITERSPSM_BIASNEG ||
       srcmod == BWRITERSPSM_SIGN || srcmod == BWRITERSPSM_SIGNNEG ||
       srcmod == BWRITERSPSM_COMP || srcmod == BWRITERSPSM_X2 ||
       srcmod == BWRITERSPSM_X2NEG || srcmod == BWRITERSPSM_DZ ||
       srcmod == BWRITERSPSM_DW) {
        asmparser_message(This, "Line %u: Source modifier %s not supported in this shader version\n",
                          This->line_no,
                          debug_print_srcmod(srcmod));
        set_parse_status(This, PARSE_ERR);
    }
}

static void check_abs_srcmod(struct asm_parser *This, DWORD srcmod) {
    if(srcmod == BWRITERSPSM_ABS || srcmod == BWRITERSPSM_ABSNEG) {
        asmparser_message(This, "Line %u: Source modifier %s not supported in this shader version\n",
                          This->line_no,
                          debug_print_srcmod(srcmod));
        set_parse_status(This, PARSE_ERR);
    }
}

static void check_loop_swizzle(struct asm_parser *This,
                               const struct shader_reg *src) {
    if((src->type == BWRITERSPR_LOOP && src->swizzle != BWRITERVS_NOSWIZZLE) ||
       (src->rel_reg && src->rel_reg->type == BWRITERSPR_LOOP &&
        src->rel_reg->swizzle != BWRITERVS_NOSWIZZLE)) {
        asmparser_message(This, "Line %u: Swizzle not allowed on aL register\n", This->line_no);
        set_parse_status(This, PARSE_ERR);
    }
}

static void check_shift_dstmod(struct asm_parser *This, DWORD shift) {
    if(shift != 0) {
        asmparser_message(This, "Line %u: Shift modifiers not supported in this shader version\n",
                          This->line_no);
        set_parse_status(This, PARSE_ERR);
    }
}

static void check_ps_dstmod(struct asm_parser *This, DWORD dstmod) {
    if(dstmod == BWRITERSPDM_PARTIALPRECISION ||
       dstmod == BWRITERSPDM_MSAMPCENTROID) {
        asmparser_message(This, "Line %u: Instruction modifier %s not supported in this shader version\n",
                          This->line_no,
                          debug_print_dstmod(dstmod));
        set_parse_status(This, PARSE_ERR);
    }
}

struct allowed_reg_type {
    DWORD type;
    DWORD count;
};

static BOOL check_reg_type(const struct shader_reg *reg,
                           const struct allowed_reg_type *allowed) {
    unsigned int i = 0;

    while(allowed[i].type != ~0U) {
        if(reg->type == allowed[i].type) {
            if(reg->rel_reg) return TRUE; /* The relative addressing register
                                             can have a negative value, we
                                             can't check the register index */
            if(reg->regnum < allowed[i].count) return TRUE;
            return FALSE;
        }
        i++;
    }
    return FALSE;
}

/* Native assembler doesn't do separate checks for src and dst registers */
static const struct allowed_reg_type vs_2_reg_allowed[] = {
    { BWRITERSPR_TEMP,      12 },
    { BWRITERSPR_INPUT,     16 },
    { BWRITERSPR_CONST,    ~0U },
    { BWRITERSPR_ADDR,       1 },
    { BWRITERSPR_CONSTBOOL, 16 },
    { BWRITERSPR_CONSTINT,  16 },
    { BWRITERSPR_LOOP,       1 },
    { BWRITERSPR_LABEL,   2048 },
    { BWRITERSPR_PREDICATE,  1 },
    { BWRITERSPR_RASTOUT,    3 }, /* oPos, oFog and oPts */
    { BWRITERSPR_ATTROUT,    2 },
    { BWRITERSPR_TEXCRDOUT,  8 },
    { ~0U, 0 } /* End tag */
};

static void asmparser_srcreg_vs_2(struct asm_parser *This,
                                  struct instruction *instr, int num,
                                  const struct shader_reg *src) {
    struct shader_reg reg;

    if(!check_reg_type(src, vs_2_reg_allowed)) {
        asmparser_message(This, "Line %u: Source register %s not supported in VS 2\n",
                          This->line_no,
                          debug_print_srcreg(src, ST_VERTEX));
        set_parse_status(This, PARSE_ERR);
    }
    check_loop_swizzle(This, src);
    check_legacy_srcmod(This, src->srcmod);
    check_abs_srcmod(This, src->srcmod);
    reg = map_oldvs_register(src);
    memcpy(&instr->src[num], &reg, sizeof(reg));
}

static const struct allowed_reg_type vs_3_reg_allowed[] = {
    { BWRITERSPR_TEMP,         32 },
    { BWRITERSPR_INPUT,        16 },
    { BWRITERSPR_CONST,       ~0U },
    { BWRITERSPR_ADDR,          1 },
    { BWRITERSPR_CONSTBOOL,    16 },
    { BWRITERSPR_CONSTINT,     16 },
    { BWRITERSPR_LOOP,          1 },
    { BWRITERSPR_LABEL,      2048 },
    { BWRITERSPR_PREDICATE,     1 },
    { BWRITERSPR_SAMPLER,       4 },
    { BWRITERSPR_OUTPUT,       12 },
    { ~0U, 0 } /* End tag */
};

static void asmparser_srcreg_vs_3(struct asm_parser *This,
                                  struct instruction *instr, int num,
                                  const struct shader_reg *src) {
    if(!check_reg_type(src, vs_3_reg_allowed)) {
        asmparser_message(This, "Line %u: Source register %s not supported in VS 3.0\n",
                          This->line_no,
                          debug_print_srcreg(src, ST_VERTEX));
        set_parse_status(This, PARSE_ERR);
    }
    check_loop_swizzle(This, src);
    check_legacy_srcmod(This, src->srcmod);
    memcpy(&instr->src[num], src, sizeof(*src));
}

static const struct allowed_reg_type ps_3_reg_allowed[] = {
    { BWRITERSPR_INPUT,        10 },
    { BWRITERSPR_TEMP,         32 },
    { BWRITERSPR_CONST,       224 },
    { BWRITERSPR_CONSTINT,     16 },
    { BWRITERSPR_CONSTBOOL,    16 },
    { BWRITERSPR_PREDICATE,     1 },
    { BWRITERSPR_SAMPLER,      16 },
    { BWRITERSPR_MISCTYPE,      2 }, /* vPos and vFace */
    { BWRITERSPR_LOOP,          1 },
    { BWRITERSPR_LABEL,      2048 },
    { BWRITERSPR_COLOROUT,      4 },
    { BWRITERSPR_DEPTHOUT,      1 },
    { ~0U, 0 } /* End tag */
};

static void asmparser_srcreg_ps_3(struct asm_parser *This,
                                  struct instruction *instr, int num,
                                  const struct shader_reg *src) {
    if(!check_reg_type(src, ps_3_reg_allowed)) {
        asmparser_message(This, "Line %u: Source register %s not supported in PS 3.0\n",
                          This->line_no,
                          debug_print_srcreg(src, ST_PIXEL));
        set_parse_status(This, PARSE_ERR);
    }
    check_loop_swizzle(This, src);
    check_legacy_srcmod(This, src->srcmod);
    memcpy(&instr->src[num], src, sizeof(*src));
}

static void asmparser_dstreg_vs_2(struct asm_parser *This,
                                  struct instruction *instr,
                                  const struct shader_reg *dst) {
    struct shader_reg reg;

    if(!check_reg_type(dst, vs_2_reg_allowed)) {
        asmparser_message(This, "Line %u: Destination register %s not supported in VS 2.0\n",
                          This->line_no,
                          debug_print_dstreg(dst, ST_VERTEX));
        set_parse_status(This, PARSE_ERR);
    }
    check_ps_dstmod(This, instr->dstmod);
    check_shift_dstmod(This, instr->shift);
    reg = map_oldvs_register(dst);
    memcpy(&instr->dst, &reg, sizeof(reg));
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_vs_3(struct asm_parser *This,
                                  struct instruction *instr,
                                  const struct shader_reg *dst) {
    if(!check_reg_type(dst, vs_3_reg_allowed)) {
        asmparser_message(This, "Line %u: Destination register %s not supported in VS 3.0\n",
                          This->line_no,
                          debug_print_dstreg(dst, ST_VERTEX));
        set_parse_status(This, PARSE_ERR);
    }
    check_ps_dstmod(This, instr->dstmod);
    check_shift_dstmod(This, instr->shift);
    memcpy(&instr->dst, dst, sizeof(*dst));
    instr->has_dst = TRUE;
}

static void asmparser_dstreg_ps_3(struct asm_parser *This,
                                  struct instruction *instr,
                                  const struct shader_reg *dst) {
    if(!check_reg_type(dst, ps_3_reg_allowed)) {
        asmparser_message(This, "Line %u: Destination register %s not supported in PS 3.0\n",
                          This->line_no,
                          debug_print_dstreg(dst, ST_PIXEL));
        set_parse_status(This, PARSE_ERR);
    }
    check_shift_dstmod(This, instr->shift);
    memcpy(&instr->dst, dst, sizeof(*dst));
    instr->has_dst = TRUE;
}

static void asmparser_predicate_supported(struct asm_parser *This,
                                          const struct shader_reg *predicate) {
    /* this sets the predicate of the last instruction added to the shader */
    if(!This->shader) return;
    if(This->shader->num_instrs == 0) ERR("Predicate without an instruction\n");
    This->shader->instr[This->shader->num_instrs - 1]->has_predicate = TRUE;
    memcpy(&This->shader->instr[This->shader->num_instrs - 1]->predicate, predicate, sizeof(*predicate));
}

#if 0
static void asmparser_predicate_unsupported(struct asm_parser *This,
                                            const struct shader_reg *predicate) {
    asmparser_message(This, "Line %u: Predicate not supported in < VS 2.0 or PS 2.x\n", This->line_no);
    set_parse_status(This, PARSE_ERR);
}
#endif

static void asmparser_coissue_unsupported(struct asm_parser *This) {
    asmparser_message(This, "Line %u: Coissue is only supported in pixel shaders versions <= 1.4\n", This->line_no);
    set_parse_status(This, PARSE_ERR);
}

static const struct asmparser_backend parser_vs_2 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_vs_2,
    asmparser_srcreg_vs_2,

    asmparser_predicate_supported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output,
    asmparser_dcl_input,
    asmparser_dcl_sampler,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_vs_3 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_vs_3,
    asmparser_srcreg_vs_3,

    asmparser_predicate_supported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output,
    asmparser_dcl_input,
    asmparser_dcl_sampler,

    asmparser_end,

    asmparser_instr,
};

static const struct asmparser_backend parser_ps_3 = {
    asmparser_constF,
    asmparser_constI,
    asmparser_constB,

    asmparser_dstreg_ps_3,
    asmparser_srcreg_ps_3,

    asmparser_predicate_supported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output,
    asmparser_dcl_input,
    asmparser_dcl_sampler,

    asmparser_end,

    asmparser_instr,
};

static void gen_oldvs_output(struct bwriter_shader *shader) {
    record_declaration(shader, BWRITERDECLUSAGE_POSITION, 0, TRUE, OPOS_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 0, TRUE, OT0_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 1, TRUE, OT1_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 2, TRUE, OT2_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 3, TRUE, OT3_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 4, TRUE, OT4_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 5, TRUE, OT5_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 6, TRUE, OT6_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_TEXCOORD, 7, TRUE, OT7_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_FOG, 0, TRUE, OFOG_REG, OFOG_WRITEMASK);
    record_declaration(shader, BWRITERDECLUSAGE_PSIZE, 0, TRUE, OPTS_REG, OPTS_WRITEMASK);
    record_declaration(shader, BWRITERDECLUSAGE_COLOR, 0, TRUE, OD0_REG, BWRITERSP_WRITEMASK_ALL);
    record_declaration(shader, BWRITERDECLUSAGE_COLOR, 1, TRUE, OD1_REG, BWRITERSP_WRITEMASK_ALL);
}

void create_vs20_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("vs_2_0\n");

    ret->shader = asm_alloc(sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(ret, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_VERTEX;
    ret->shader->version = BWRITERVS_VERSION(2, 0);
    ret->funcs = &parser_vs_2;
    gen_oldvs_output(ret->shader);
}

void create_vs2x_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("vs_2_x\n");

    ret->shader = asm_alloc(sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(ret, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_VERTEX;
    ret->shader->version = BWRITERVS_VERSION(2, 1);
    ret->funcs = &parser_vs_2;
    gen_oldvs_output(ret->shader);
}

void create_vs30_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("vs_3_0\n");

    ret->shader = asm_alloc(sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(ret, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_VERTEX;
    ret->shader->version = BWRITERVS_VERSION(3, 0);
    ret->funcs = &parser_vs_3;
}

void create_ps30_parser(struct asm_parser *ret) {
    TRACE_(parsed_shader)("ps_3_0\n");

    ret->shader = asm_alloc(sizeof(*ret->shader));
    if(!ret->shader) {
        ERR("Failed to allocate memory for the shader\n");
        set_parse_status(ret, PARSE_ERR);
        return;
    }

    ret->shader->type = ST_PIXEL;
    ret->shader->version = BWRITERPS_VERSION(3, 0);
    ret->funcs = &parser_ps_3;
}
