/*
 * Direct3D bytecode output functions
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

/****************************************************************
 * General assembler shader construction helper routines follow *
 ****************************************************************/
/* struct instruction *alloc_instr
 *
 * Allocates a new instruction structure with srcs registers
 *
 * Parameters:
 *  srcs: Number of source registers to allocate
 *
 * Returns:
 *  A pointer to the allocated instruction structure
 *  NULL in case of an allocation failure
 */
struct instruction *alloc_instr(unsigned int srcs) {
    struct instruction *ret = asm_alloc(sizeof(*ret));
    if(!ret) {
        ERR("Failed to allocate memory for an instruction structure\n");
        return NULL;
    }

    if(srcs) {
        ret->src = asm_alloc(srcs * sizeof(*ret->src));
        if(!ret->src) {
            ERR("Failed to allocate memory for instruction registers\n");
            asm_free(ret);
            return NULL;
        }
        ret->num_srcs = srcs;
    }
    return ret;
}

/* void add_instruction
 *
 * Adds a new instruction to the shader's instructions array and grows the instruction array
 * if needed.
 *
 * The function does NOT copy the instruction structure. Make sure not to release the
 * instruction or any of its substructures like registers.
 *
 * Parameters:
 *  shader: Shader to add the instruction to
 *  instr: Instruction to add to the shader
 */
BOOL add_instruction(struct bwriter_shader *shader, struct instruction *instr) {
    struct instruction      **new_instructions;

    if(!shader) return FALSE;

    if(shader->instr_alloc_size == 0) {
        shader->instr = asm_alloc(sizeof(*shader->instr) * INSTRARRAY_INITIAL_SIZE);
        if(!shader->instr) {
            ERR("Failed to allocate the shader instruction array\n");
            return FALSE;
        }
        shader->instr_alloc_size = INSTRARRAY_INITIAL_SIZE;
    } else if(shader->instr_alloc_size == shader->num_instrs) {
        new_instructions = asm_realloc(shader->instr,
                                       sizeof(*shader->instr) * (shader->instr_alloc_size) * 2);
        if(!new_instructions) {
            ERR("Failed to grow the shader instruction array\n");
            return FALSE;
        }
        shader->instr = new_instructions;
        shader->instr_alloc_size = shader->instr_alloc_size * 2;
    } else if(shader->num_instrs > shader->instr_alloc_size) {
        ERR("More instructions than allocated. This should not happen\n");
        return FALSE;
    }

    shader->instr[shader->num_instrs] = instr;
    shader->num_instrs++;
    return TRUE;
}

void SlDeleteShader(struct bwriter_shader *shader) {
    unsigned int i, j;

    TRACE("Deleting shader %p\n", shader);

    for(i = 0; i < shader->num_cf; i++) {
        asm_free(shader->constF[i]);
    }
    asm_free(shader->constF);
    for(i = 0; i < shader->num_ci; i++) {
        asm_free(shader->constI[i]);
    }
    asm_free(shader->constI);
    for(i = 0; i < shader->num_cb; i++) {
        asm_free(shader->constB[i]);
    }
    asm_free(shader->constB);

    asm_free(shader->inputs);
    asm_free(shader->outputs);
    asm_free(shader->samplers);

    for(i = 0; i < shader->num_instrs; i++) {
        for(j = 0; j < shader->instr[i]->num_srcs; j++) {
            asm_free(shader->instr[i]->src[j].rel_reg);
        }
        asm_free(shader->instr[i]->src);
        asm_free(shader->instr[i]);
    }
    asm_free(shader->instr);

    asm_free(shader);
}
