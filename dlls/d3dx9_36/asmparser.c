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


/****************************************************************
 * Common(non-version specific) shader parser control code      *
 ****************************************************************/

static void asmparser_end(struct asm_parser *This) {
    TRACE("Finalizing shader\n");
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

static void asmparser_srcreg_vs_3(struct asm_parser *This,
                                  struct instruction *instr, int num,
                                  const struct shader_reg *src) {
    memcpy(&instr->src[num], src, sizeof(*src));
}

static void asmparser_dstreg_vs_3(struct asm_parser *This,
                                  struct instruction *instr,
                                  const struct shader_reg *dst) {
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

static const struct asmparser_backend parser_vs_3 = {
    asmparser_dstreg_vs_3,
    asmparser_srcreg_vs_3,

    asmparser_predicate_supported,
    asmparser_coissue_unsupported,

    asmparser_dcl_output,

    asmparser_end,

    asmparser_instr,
};

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
