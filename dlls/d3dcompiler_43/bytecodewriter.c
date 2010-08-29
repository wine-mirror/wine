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

#include "d3d9types.h"
#include "d3dcompiler_private.h"

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

BOOL add_constF(struct bwriter_shader *shader, DWORD reg, float x, float y, float z, float w) {
    struct constant *newconst;

    if(shader->num_cf) {
        struct constant **newarray;
        newarray = asm_realloc(shader->constF,
                               sizeof(*shader->constF) * (shader->num_cf + 1));
        if(!newarray) {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constF = newarray;
    } else {
        shader->constF = asm_alloc(sizeof(*shader->constF));
        if(!shader->constF) {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = asm_alloc(sizeof(*newconst));
    if(!newconst) {
        ERR("Failed to allocate a new constant\n");
        return FALSE;
    }
    newconst->regnum = reg;
    newconst->value[0].f = x;
    newconst->value[1].f = y;
    newconst->value[2].f = z;
    newconst->value[3].f = w;
    shader->constF[shader->num_cf] = newconst;

    shader->num_cf++;
    return TRUE;
}

BOOL add_constI(struct bwriter_shader *shader, DWORD reg, INT x, INT y, INT z, INT w) {
    struct constant *newconst;

    if(shader->num_ci) {
        struct constant **newarray;
        newarray = asm_realloc(shader->constI,
                               sizeof(*shader->constI) * (shader->num_ci + 1));
        if(!newarray) {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constI = newarray;
    } else {
        shader->constI = asm_alloc(sizeof(*shader->constI));
        if(!shader->constI) {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = asm_alloc(sizeof(*newconst));
    if(!newconst) {
        ERR("Failed to allocate a new constant\n");
        return FALSE;
    }
    newconst->regnum = reg;
    newconst->value[0].i = x;
    newconst->value[1].i = y;
    newconst->value[2].i = z;
    newconst->value[3].i = w;
    shader->constI[shader->num_ci] = newconst;

    shader->num_ci++;
    return TRUE;
}

BOOL add_constB(struct bwriter_shader *shader, DWORD reg, BOOL x) {
    struct constant *newconst;

    if(shader->num_cb) {
        struct constant **newarray;
        newarray = asm_realloc(shader->constB,
                               sizeof(*shader->constB) * (shader->num_cb + 1));
        if(!newarray) {
            ERR("Failed to grow the constants array\n");
            return FALSE;
        }
        shader->constB = newarray;
    } else {
        shader->constB = asm_alloc(sizeof(*shader->constB));
        if(!shader->constB) {
            ERR("Failed to allocate the constants array\n");
            return FALSE;
        }
    }

    newconst = asm_alloc(sizeof(*newconst));
    if(!newconst) {
        ERR("Failed to allocate a new constant\n");
        return FALSE;
    }
    newconst->regnum = reg;
    newconst->value[0].b = x;
    shader->constB[shader->num_cb] = newconst;

    shader->num_cb++;
    return TRUE;
}

BOOL record_declaration(struct bwriter_shader *shader, DWORD usage,
                        DWORD usage_idx, DWORD mod, BOOL output,
                        DWORD regnum, DWORD writemask, BOOL builtin) {
    unsigned int *num;
    struct declaration **decl;
    unsigned int i;

    if(!shader) return FALSE;

    if(output) {
        num = &shader->num_outputs;
        decl = &shader->outputs;
    } else {
        num = &shader->num_inputs;
        decl = &shader->inputs;
    }

    if(*num == 0) {
        *decl = asm_alloc(sizeof(**decl));
        if(!*decl) {
            ERR("Error allocating declarations array\n");
            return FALSE;
        }
    } else {
        struct declaration *newdecl;
        for(i = 0; i < *num; i++) {
            if((*decl)[i].regnum == regnum && ((*decl)[i].writemask & writemask)) {
                WARN("Declaration of register %u already exists, writemask match 0x%x\n",
                      regnum, (*decl)[i].writemask & writemask);
            }
        }

        newdecl = asm_realloc(*decl,
                              sizeof(**decl) * ((*num) + 1));
        if(!newdecl) {
            ERR("Error reallocating declarations array\n");
            return FALSE;
        }
        *decl = newdecl;
    }
    (*decl)[*num].usage = usage;
    (*decl)[*num].usage_idx = usage_idx;
    (*decl)[*num].regnum = regnum;
    (*decl)[*num].mod = mod;
    (*decl)[*num].writemask = writemask;
    (*decl)[*num].builtin = builtin;
    (*num)++;

    return TRUE;
}

BOOL record_sampler(struct bwriter_shader *shader, DWORD samptype, DWORD mod, DWORD regnum) {
    unsigned int i;

    if(!shader) return FALSE;

    if(shader->num_samplers == 0) {
        shader->samplers = asm_alloc(sizeof(*shader->samplers));
        if(!shader->samplers) {
            ERR("Error allocating samplers array\n");
            return FALSE;
        }
    } else {
        struct samplerdecl *newarray;

        for(i = 0; i < shader->num_samplers; i++) {
            if(shader->samplers[i].regnum == regnum) {
                WARN("Sampler %u already declared\n", regnum);
                /* This is not an error as far as the assembler is concerned.
                 * Direct3D might refuse to load the compiled shader though
                 */
            }
        }

        newarray = asm_realloc(shader->samplers,
                               sizeof(*shader->samplers) * (shader->num_samplers + 1));
        if(!newarray) {
            ERR("Error reallocating samplers array\n");
            return FALSE;
        }
        shader->samplers = newarray;
    }

    shader->samplers[shader->num_samplers].type = samptype;
    shader->samplers[shader->num_samplers].mod = mod;
    shader->samplers[shader->num_samplers].regnum = regnum;
    shader->num_samplers++;
    return TRUE;
}


/* shader bytecode buffer manipulation functions.
 * allocate_buffer creates a new buffer structure, put_dword adds a new
 * DWORD to the buffer. In the rare case of a memory allocation failure
 * when trying to grow the buffer a flag is set in the buffer to mark it
 * invalid. This avoids return value checking and passing in many places
 */
static struct bytecode_buffer *allocate_buffer(void) {
    struct bytecode_buffer *ret;

    ret = asm_alloc(sizeof(*ret));
    if(!ret) return NULL;

    ret->alloc_size = BYTECODEBUFFER_INITIAL_SIZE;
    ret->data = asm_alloc(sizeof(DWORD) * ret->alloc_size);
    if(!ret->data) {
        asm_free(ret);
        return NULL;
    }
    ret->state = S_OK;
    return ret;
}

static void put_dword(struct bytecode_buffer *buffer, DWORD value) {
    if(FAILED(buffer->state)) return;

    if(buffer->alloc_size == buffer->size) {
        DWORD *newarray;
        buffer->alloc_size *= 2;
        newarray = asm_realloc(buffer->data,
                               sizeof(DWORD) * buffer->alloc_size);
        if(!newarray) {
            ERR("Failed to grow the buffer data memory\n");
            buffer->state = E_OUTOFMEMORY;
            return;
        }
        buffer->data = newarray;
    }
    buffer->data[buffer->size++] = value;
}

/******************************************************
 * Implementation of the writer functions starts here *
 ******************************************************/
static void write_declarations(struct bc_writer *This,
                               struct bytecode_buffer *buffer, BOOL len,
                               const struct declaration *decls, unsigned int num, DWORD type) {
    DWORD i;
    DWORD instr_dcl = D3DSIO_DCL;
    DWORD token;
    struct shader_reg reg;

    ZeroMemory(&reg, sizeof(reg));

    if(len) {
        instr_dcl |= 2 << D3DSI_INSTLENGTH_SHIFT;
    }

    for(i = 0; i < num; i++) {
        if(decls[i].builtin) continue;

        /* Write the DCL instruction */
        put_dword(buffer, instr_dcl);

        /* Write the usage and index */
        token = (1 << 31); /* Bit 31 of non-instruction opcodes is 1 */
        token |= (decls[i].usage << D3DSP_DCL_USAGE_SHIFT) & D3DSP_DCL_USAGE_MASK;
        token |= (decls[i].usage_idx << D3DSP_DCL_USAGEINDEX_SHIFT) & D3DSP_DCL_USAGEINDEX_MASK;
        put_dword(buffer, token);

        /* Write the dest register */
        reg.type = type;
        reg.regnum = decls[i].regnum;
        reg.u.writemask = decls[i].writemask;
        This->funcs->dstreg(This, &reg, buffer, 0, decls[i].mod);
    }
}

static void write_const(struct constant **consts, int num, DWORD opcode, DWORD reg_type, struct bytecode_buffer *buffer, BOOL len) {
    DWORD i;
    DWORD instr_def = opcode;
    const DWORD reg = (1<<31) |
                      ((reg_type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK) |
                      ((reg_type << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2) |
                      D3DSP_WRITEMASK_ALL;

    if(len) {
        if(opcode == D3DSIO_DEFB)
            instr_def |= 2 << D3DSI_INSTLENGTH_SHIFT;
        else
            instr_def |= 5 << D3DSI_INSTLENGTH_SHIFT;
    }

    for(i = 0; i < num; i++) {
        /* Write the DEF instruction */
        put_dword(buffer, instr_def);

        put_dword(buffer, reg | (consts[i]->regnum & D3DSP_REGNUM_MASK));
        put_dword(buffer, consts[i]->value[0].d);
        if(opcode != D3DSIO_DEFB) {
            put_dword(buffer, consts[i]->value[1].d);
            put_dword(buffer, consts[i]->value[2].d);
            put_dword(buffer, consts[i]->value[3].d);
        }
    }
}

static void write_constF(const struct bwriter_shader *shader, struct bytecode_buffer *buffer, BOOL len) {
    write_const(shader->constF, shader->num_cf, D3DSIO_DEF, D3DSPR_CONST, buffer, len);
}

/* This function looks for VS 1/2 registers mapping to VS 3 output registers */
static HRESULT vs_find_builtin_varyings(struct bc_writer *This, const struct bwriter_shader *shader) {
    DWORD i;
    DWORD usage, usage_idx, writemask, regnum;

    for(i = 0; i < shader->num_outputs; i++) {
        if(!shader->outputs[i].builtin) continue;

        usage = shader->outputs[i].usage;
        usage_idx = shader->outputs[i].usage_idx;
        writemask = shader->outputs[i].writemask;
        regnum = shader->outputs[i].regnum;

        switch(usage) {
            case BWRITERDECLUSAGE_POSITION:
            case BWRITERDECLUSAGE_POSITIONT:
                if(usage_idx > 0) {
                    WARN("dcl_position%u not supported in sm 1/2 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                TRACE("o%u is oPos\n", regnum);
                This->oPos_regnum = regnum;
                break;

            case BWRITERDECLUSAGE_COLOR:
                if(usage_idx > 1) {
                    WARN("dcl_color%u not supported in sm 1/2 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != BWRITERSP_WRITEMASK_ALL) {
                    WARN("Only WRITEMASK_ALL is supported on color in sm 1/2\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u is oD%u\n", regnum, usage_idx);
                This->oD_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_TEXCOORD:
                if(usage_idx >= 8) {
                    WARN("dcl_color%u not supported in sm 1/2 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != (BWRITERSP_WRITEMASK_0) &&
                   writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1) &&
                   writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1 | BWRITERSP_WRITEMASK_2) &&
                   writemask != (BWRITERSP_WRITEMASK_ALL)) {
                    WARN("Partial writemasks not supported on texture coordinates in sm 1 and 2\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u is oT%u\n", regnum, usage_idx);
                This->oT_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_PSIZE:
                if(usage_idx > 0) {
                    WARN("dcl_psize%u not supported in sm 1/2 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                TRACE("o%u writemask 0x%08x is oPts\n", regnum, writemask);
                This->oPts_regnum = regnum;
                This->oPts_mask = writemask;
                break;

            case BWRITERDECLUSAGE_FOG:
                if(usage_idx > 0) {
                    WARN("dcl_fog%u not supported in sm 1 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != BWRITERSP_WRITEMASK_0 && writemask != BWRITERSP_WRITEMASK_1 &&
                   writemask != BWRITERSP_WRITEMASK_2 && writemask != BWRITERSP_WRITEMASK_3) {
                    WARN("Unsupported fog writemask\n");
                    return E_INVALIDARG;
                }
                TRACE("o%u writemask 0x%08x is oFog\n", regnum, writemask);
                This->oFog_regnum = regnum;
                This->oFog_mask = writemask;
                break;

            default:
                WARN("Varying type %u is not supported in shader model 1.x\n", usage);
                return E_INVALIDARG;
        }
    }

    return S_OK;
}

static void vs_1_x_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr;

    if(shader->num_ci || shader->num_cb) {
        WARN("Int and bool constants are not supported in shader model 1 shaders\n");
        WARN("Got %u int and %u boolean constants\n", shader->num_ci, shader->num_cb);
        This->state = E_INVALIDARG;
        return;
    }

    hr = vs_find_builtin_varyings(This, shader);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    /* Declare the shader type and version */
    put_dword(buffer, This->version);

    write_declarations(This, buffer, FALSE, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_constF(shader, buffer, FALSE);
    return;
}

static HRESULT find_ps_builtin_semantics(struct bc_writer *This,
                                         const struct bwriter_shader *shader,
                                         DWORD texcoords) {
    DWORD i;
    DWORD usage, usage_idx, writemask, regnum;

    This->v_regnum[0] = -1; This->v_regnum[1] = -1;
    for(i = 0; i < 8; i++) This->t_regnum[i] = -1;

    for(i = 0; i < shader->num_inputs; i++) {
        if(!shader->inputs[i].builtin) continue;

        usage = shader->inputs[i].usage;
        usage_idx = shader->inputs[i].usage_idx;
        writemask = shader->inputs[i].writemask;
        regnum = shader->inputs[i].regnum;

        switch(usage) {
            case BWRITERDECLUSAGE_COLOR:
                if(usage_idx > 1) {
                    WARN("dcl_color%u not supported in sm 1 shaders\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != BWRITERSP_WRITEMASK_ALL) {
                    WARN("Only WRITEMASK_ALL is supported on color in sm 1\n");
                    return E_INVALIDARG;
                }
                TRACE("v%u is v%u\n", regnum, usage_idx);
                This->v_regnum[usage_idx] = regnum;
                break;

            case BWRITERDECLUSAGE_TEXCOORD:
                if(usage_idx > texcoords) {
                    WARN("dcl_texcoord%u not supported in this shader version\n", usage_idx);
                    return E_INVALIDARG;
                }
                if(writemask != (BWRITERSP_WRITEMASK_0) &&
                   writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1) &&
                   writemask != (BWRITERSP_WRITEMASK_0 | BWRITERSP_WRITEMASK_1 | BWRITERSP_WRITEMASK_2) &&
                   writemask != (BWRITERSP_WRITEMASK_ALL)) {
                    WARN("Partial writemasks not supported on texture coordinates in sm 1 and 2\n");
                } else {
                    writemask = BWRITERSP_WRITEMASK_ALL;
                }
                TRACE("v%u is t%u\n", regnum, usage_idx);
                This->t_regnum[usage_idx] = regnum;
                break;

            default:
                WARN("Varying type %u is not supported in shader model 1.x\n", usage);
                return E_INVALIDARG;
        }
    }

    return S_OK;
}

static void ps_1_x_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr;

    /* First check the constants and varyings, and complain if unsupported things are used */
    if(shader->num_ci || shader->num_cb) {
        WARN("Int and bool constants are not supported in shader model 1 shaders\n");
        WARN("Got %u int and %u boolean constants\n", shader->num_ci, shader->num_cb);
        This->state = E_INVALIDARG;
        return;
    }

    hr = find_ps_builtin_semantics(This, shader, 4);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    /* Declare the shader type and version */
    put_dword(buffer, This->version);
    write_constF(shader, buffer, TRUE);
}

static void ps_1_4_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr;

    /* First check the constants and varyings, and complain if unsupported things are used */
    if(shader->num_ci || shader->num_cb) {
        WARN("Int and bool constants are not supported in shader model 1 shaders\n");
        WARN("Got %u int and %u boolean constants\n", shader->num_ci, shader->num_cb);
        This->state = E_INVALIDARG;
        return;
    }
    hr = find_ps_builtin_semantics(This, shader, 6);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    /* Declare the shader type and version */
    put_dword(buffer, This->version);
    write_constF(shader, buffer, TRUE);
}

static void end(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    put_dword(buffer, D3DSIO_END);
}

static DWORD map_vs_output(struct bc_writer *This, DWORD regnum, DWORD mask, DWORD *has_components) {
    DWORD token = 0;
    DWORD i;

    *has_components = TRUE;
    if(regnum == This->oPos_regnum) {
        token |= (D3DSPR_RASTOUT << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
        token |= D3DSRO_POSITION & D3DSP_REGNUM_MASK; /* No shift */
        return token;
    }
    if(regnum == This->oFog_regnum && mask == This->oFog_mask) {
        token |= (D3DSPR_RASTOUT << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
        token |= D3DSRO_FOG & D3DSP_REGNUM_MASK; /* No shift */
        token |= D3DSP_WRITEMASK_ALL;
        *has_components = FALSE;
        return token;
    }
    if(regnum == This->oPts_regnum && mask == This->oPts_mask) {
        token |= (D3DSPR_RASTOUT << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
        token |= D3DSRO_POINT_SIZE & D3DSP_REGNUM_MASK; /* No shift */
        token |= D3DSP_WRITEMASK_ALL;
        *has_components = FALSE;
        return token;
    }
    for(i = 0; i < 2; i++) {
        if(regnum == This->oD_regnum[i]) {
            token |= (D3DSPR_ATTROUT << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= i & D3DSP_REGNUM_MASK; /* No shift */
            return token;
        }
    }
    for(i = 0; i < 8; i++) {
        if(regnum == This->oT_regnum[i]) {
            token |= (D3DSPR_TEXCRDOUT << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= i & D3DSP_REGNUM_MASK; /* No shift */
            return token;
        }
    }

    /* The varying must be undeclared - if an unsupported varying was declared,
     * the vs_find_builtin_varyings function would have caught it and this code
     * would not run */
    WARN("Undeclared varying %u\n", regnum);
    This->state = E_INVALIDARG;
    return -1;
}

static void vs_12_dstreg(struct bc_writer *This, const struct shader_reg *reg,
                         struct bytecode_buffer *buffer,
                         DWORD shift, DWORD mod) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    DWORD has_wmask;

    if(reg->rel_reg) {
        WARN("Relative addressing not supported for destination registers\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_OUTPUT:
            token |= map_vs_output(This, reg->regnum, reg->u.writemask, &has_wmask);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can be encoded in the bytecode,
            * but are unexpected. If we hit this path it might be due to an error.
            */
            FIXME("Unexpected register type %u\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
            token |= (reg->type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            has_wmask = TRUE;
            break;

        case BWRITERSPR_ADDR:
            if(reg->regnum != 0) {
                WARN("Only a0 exists\n");
                This->state = E_INVALIDARG;
                return;
            }
            token |= (D3DSPR_ADDR << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= 0 & D3DSP_REGNUM_MASK; /* No shift */
            has_wmask = TRUE;
            break;

        case BWRITERSPR_PREDICATE:
            if(This->version != BWRITERVS_VERSION(2, 1)){
                WARN("Predicate register is allowed only in vs_2_x\n");
                This->state = E_INVALIDARG;
                return;
            }
            if(reg->regnum != 0) {
                WARN("Only predicate register p0 exists\n");
                This->state = E_INVALIDARG;
                return;
            }
            token |= (D3DSPR_PREDICATE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= (D3DSPR_PREDICATE << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
            token |= 0 & D3DSP_REGNUM_MASK; /* No shift */
            has_wmask = TRUE;
            break;

        default:
            WARN("Invalid register type for 1.x-2.x vertex shader\n");
            This->state = E_INVALIDARG;
            return;
    }

    /* strictly speaking there are no modifiers in vs_2_0 and vs_1_x, but they can be written
     * into the bytecode and since the compiler doesn't do such checks write them
     * (the checks are done by the undocumented shader validator)
     */
    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    if(has_wmask) {
        token |= d3d9_writemask(reg->u.writemask);
    }
    put_dword(buffer, token);
}

static void vs_1_x_srcreg(struct bc_writer *This, const struct shader_reg *reg,
                          struct bytecode_buffer *buffer) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    DWORD has_swizzle;
    DWORD component;

    switch(reg->type) {
        case BWRITERSPR_OUTPUT:
            /* Map the swizzle to a writemask, the format expected
               by map_vs_output
             */
            switch(reg->u.swizzle) {
                case BWRITERVS_SWIZZLE_X:
                    component = BWRITERSP_WRITEMASK_0;
                    break;
                case BWRITERVS_SWIZZLE_Y:
                    component = BWRITERSP_WRITEMASK_1;
                    break;
                case BWRITERVS_SWIZZLE_Z:
                    component = BWRITERSP_WRITEMASK_2;
                    break;
                case BWRITERVS_SWIZZLE_W:
                    component = BWRITERSP_WRITEMASK_3;
                    break;
                default:
                    component = 0;
            }
            token |= map_vs_output(This, reg->regnum, component, &has_swizzle);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can be encoded in the bytecode,
             * but are unexpected. If we hit this path it might be due to an error.
             */
            FIXME("Unexpected register type %u\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
        case BWRITERSPR_ADDR:
            token |= (reg->type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            if(reg->rel_reg) {
                if(reg->rel_reg->type != BWRITERSPR_ADDR ||
                   reg->rel_reg->regnum != 0 ||
                   reg->rel_reg->u.swizzle != BWRITERVS_SWIZZLE_X) {
                    WARN("Relative addressing in vs_1_x is only allowed with a0.x\n");
                    This->state = E_INVALIDARG;
                    return;
                }
                token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
            }
            break;

        default:
            WARN("Invalid register type for 1.x vshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    token |= d3d9_srcmod(reg->srcmod);
    put_dword(buffer, token);
}

static void write_srcregs(struct bc_writer *This, const struct instruction *instr,
                          struct bytecode_buffer *buffer){
    unsigned int i;
    if(instr->has_predicate){
        This->funcs->srcreg(This, &instr->predicate, buffer);
    }
    for(i = 0; i < instr->num_srcs; i++){
        This->funcs->srcreg(This, &instr->src[i], buffer);
    }
}

static DWORD map_ps13_temp(struct bc_writer *This, const struct shader_reg *reg) {
    DWORD token = 0;
    if(reg->regnum == T0_REG) {
        token |= (D3DSPR_TEXTURE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
        token |= 0 & D3DSP_REGNUM_MASK; /* No shift */
    } else if(reg->regnum == T1_REG) {
        token |= (D3DSPR_TEXTURE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
        token |= 1 & D3DSP_REGNUM_MASK; /* No shift */
    } else if(reg->regnum == T2_REG) {
        token |= (D3DSPR_TEXTURE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
        token |= 2 & D3DSP_REGNUM_MASK; /* No shift */
    } else if(reg->regnum == T3_REG) {
        token |= (D3DSPR_TEXTURE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
        token |= 3 & D3DSP_REGNUM_MASK; /* No shift */
    } else {
        token |= (D3DSPR_TEMP << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
        token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
    }
    return token;
}

static DWORD map_ps_input(struct bc_writer *This,
                          const struct shader_reg *reg) {
    DWORD i, token = 0;
    /* Map color interpolators */
    for(i = 0; i < 2; i++) {
        if(reg->regnum == This->v_regnum[i]) {
            token |= (D3DSPR_INPUT << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= i & D3DSP_REGNUM_MASK; /* No shift */
            return token;
        }
    }
    for(i = 0; i < 8; i++) {
        if(reg->regnum == This->t_regnum[i]) {
            token |= (D3DSPR_TEXTURE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= i & D3DSP_REGNUM_MASK; /* No shift */
            return token;
        }
    }

    WARN("Invalid ps 1/2 varying\n");
    This->state = E_INVALIDARG;
    return token;
}

static void ps_1_0123_srcreg(struct bc_writer *This, const struct shader_reg *reg,
                             struct bytecode_buffer *buffer) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    if(reg->rel_reg) {
        WARN("Relative addressing not supported in <= ps_3_0\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

            /* Take care about the texture temporaries. There's a problem: They aren't
             * declared anywhere, so we can only hardcode the values that are used
             * to map ps_1_3 shaders to the common shader structure
             */
        case BWRITERSPR_TEMP:
            token |= map_ps13_temp(This, reg);
            break;

        case BWRITERSPR_CONST: /* Can be mapped 1:1 */
            token |= (reg->type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            break;

        default:
            WARN("Invalid register type for <= ps_1_3 shader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    if(reg->srcmod == BWRITERSPSM_DZ || reg->srcmod == BWRITERSPSM_DW ||
       reg->srcmod == BWRITERSPSM_ABS || reg->srcmod == BWRITERSPSM_ABSNEG ||
       reg->srcmod == BWRITERSPSM_NOT) {
        WARN("Invalid source modifier %u for <= ps_1_3\n", reg->srcmod);
        This->state = E_INVALIDARG;
        return;
    }
    token |= d3d9_srcmod(reg->srcmod);
    put_dword(buffer, token);
}

static void ps_1_0123_dstreg(struct bc_writer *This, const struct shader_reg *reg,
                             struct bytecode_buffer *buffer,
                             DWORD shift, DWORD mod) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */

    if(reg->rel_reg) {
        WARN("Relative addressing not supported for destination registers\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_TEMP:
            token |= map_ps13_temp(This, reg);
            break;

        /* texkill uses the input register as a destination parameter */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

        default:
            WARN("Invalid dest register type for 1.x pshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->u.writemask);
    put_dword(buffer, token);
}

/* The length of an instruction consists of the destination register (if any),
 * the number of source registers, the number of address registers used for
 * indirect addressing, and optionally the predicate register
 */
static DWORD instrlen(const struct instruction *instr, unsigned int srcs, unsigned int dsts) {
    unsigned int i;
    DWORD ret = srcs + dsts + (instr->has_predicate ? 1 : 0);

    if(dsts){
        if(instr->dst.rel_reg) ret++;
    }
    for(i = 0; i < srcs; i++) {
        if(instr->src[i].rel_reg) ret++;
    }
    return ret;
}

static void sm_1_x_opcode(struct bc_writer *This,
                          const struct instruction *instr,
                          DWORD token, struct bytecode_buffer *buffer) {
    /* In sm_1_x instruction length isn't encoded */
    if(instr->coissue){
        token |= D3DSI_COISSUE;
    }
    put_dword(buffer, token);
}

static void instr_handler(struct bc_writer *This,
                          const struct instruction *instr,
                          struct bytecode_buffer *buffer) {
    DWORD token = d3d9_opcode(instr->opcode);

    This->funcs->opcode(This, instr, token, buffer);
    if(instr->has_dst) This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    write_srcregs(This, instr, buffer);
}

static const struct instr_handler_table vs_1_x_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_SLT,            instr_handler},
    {BWRITERSIO_SGE,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_DST,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_LIT,            instr_handler},

    {BWRITERSIO_END,            NULL}, /* Sentinel value, it signals
                                          the end of the list */
};

static const struct bytecode_backend vs_1_x_backend = {
    vs_1_x_header,
    end,
    vs_1_x_srcreg,
    vs_12_dstreg,
    sm_1_x_opcode,
    vs_1_x_handlers
};

static void instr_ps_1_0123_texld(struct bc_writer *This,
                                  const struct instruction *instr,
                                  struct bytecode_buffer *buffer) {
    DWORD idx;
    struct shader_reg reg;
    DWORD swizzlemask;

    if(instr->src[1].type != BWRITERSPR_SAMPLER ||
       instr->src[1].regnum > 3) {
        WARN("Unsupported sampler type %u regnum %u\n",
             instr->src[1].type, instr->src[1].regnum);
        This->state = E_INVALIDARG;
        return;
    } else if(instr->dst.type != BWRITERSPR_TEMP) {
        WARN("Can only sample into a temp register\n");
        This->state = E_INVALIDARG;
        return;
    }

    idx = instr->src[1].regnum;
    if((idx == 0 && instr->dst.regnum != T0_REG) ||
       (idx == 1 && instr->dst.regnum != T1_REG) ||
       (idx == 2 && instr->dst.regnum != T2_REG) ||
       (idx == 3 && instr->dst.regnum != T3_REG)) {
        WARN("Sampling from sampler s%u to register r%u is not possible in ps_1_x\n",
             idx, instr->dst.regnum);
        This->state = E_INVALIDARG;
        return;
    }
    if(instr->src[0].type == BWRITERSPR_INPUT) {
        /* A simple non-dependent read tex instruction */
        if(instr->src[0].regnum != This->t_regnum[idx]) {
            WARN("Cannot sample from s%u with texture address data from interpolator %u\n",
                 idx, instr->src[0].regnum);
            This->state = E_INVALIDARG;
            return;
        }
        This->funcs->opcode(This, instr, D3DSIO_TEX & D3DSI_OPCODE_MASK, buffer);

        /* map the temp dstreg to the ps_1_3 texture temporary register */
        This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    } else if(instr->src[0].type == BWRITERSPR_TEMP) {

        swizzlemask = (3 << BWRITERVS_SWIZZLE_SHIFT) |
            (3 << (BWRITERVS_SWIZZLE_SHIFT + 2)) |
            (3 << (BWRITERVS_SWIZZLE_SHIFT + 4));
        if((instr->src[0].u.swizzle & swizzlemask) == (BWRITERVS_X_X | BWRITERVS_Y_Y | BWRITERVS_Z_Z)) {
            TRACE("writing texreg2rgb\n");
            This->funcs->opcode(This, instr, D3DSIO_TEXREG2RGB & D3DSI_OPCODE_MASK, buffer);
        } else if(instr->src[0].u.swizzle == (BWRITERVS_X_W | BWRITERVS_Y_X | BWRITERVS_Z_X | BWRITERVS_W_X)) {
            TRACE("writing texreg2ar\n");
            This->funcs->opcode(This, instr, D3DSIO_TEXREG2AR & D3DSI_OPCODE_MASK, buffer);
        } else if(instr->src[0].u.swizzle == (BWRITERVS_X_Y | BWRITERVS_Y_Z | BWRITERVS_Z_Z | BWRITERVS_W_Z)) {
            TRACE("writing texreg2gb\n");
            This->funcs->opcode(This, instr, D3DSIO_TEXREG2GB & D3DSI_OPCODE_MASK, buffer);
        } else {
            WARN("Unsupported src addr swizzle in dependent texld: 0x%08x\n", instr->src[0].u.swizzle);
            This->state = E_INVALIDARG;
            return;
        }

        /* Dst and src reg can be mapped normally. Both registers are temporary registers in the
         * source shader and have to be mapped to the temporary form of the texture registers. However,
         * the src reg doesn't have a swizzle
         */
        This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
        reg = instr->src[0];
        reg.u.swizzle = BWRITERVS_NOSWIZZLE;
        This->funcs->srcreg(This, &reg, buffer);
    } else {
        WARN("Invalid address data source register\n");
        This->state = E_INVALIDARG;
        return;
    }
}

static void instr_ps_1_0123_mov(struct bc_writer *This,
                                const struct instruction *instr,
                                struct bytecode_buffer *buffer) {
    DWORD token = D3DSIO_MOV & D3DSI_OPCODE_MASK;

    if(instr->dst.type == BWRITERSPR_TEMP && instr->src[0].type == BWRITERSPR_INPUT) {
        if((instr->dst.regnum == T0_REG && instr->src[0].regnum == This->t_regnum[0]) ||
           (instr->dst.regnum == T1_REG && instr->src[0].regnum == This->t_regnum[1]) ||
           (instr->dst.regnum == T2_REG && instr->src[0].regnum == This->t_regnum[2]) ||
           (instr->dst.regnum == T3_REG && instr->src[0].regnum == This->t_regnum[3])) {
            if(instr->dstmod & BWRITERSPDM_SATURATE) {
                This->funcs->opcode(This, instr, D3DSIO_TEXCOORD & D3DSI_OPCODE_MASK, buffer);
                /* Remove the SATURATE flag, it's implicit to the instruction */
                This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod & (~BWRITERSPDM_SATURATE));
                return;
            } else {
                WARN("A varying -> temp copy is only supported with the SATURATE modifier in <=ps_1_3\n");
                This->state = E_INVALIDARG;
                return;
            }
        } else if(instr->src[0].regnum == This->v_regnum[0] ||
                  instr->src[0].regnum == This->v_regnum[1]) {
            /* Handled by the normal mov below. Just drop out of the if condition */
        } else {
            WARN("Unsupported varying -> temp mov in <= ps_1_3\n");
            This->state = E_INVALIDARG;
            return;
        }
    }

    This->funcs->opcode(This, instr, token, buffer);
    This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    This->funcs->srcreg(This, &instr->src[0], buffer);
}

static const struct instr_handler_table ps_1_0123_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_ps_1_0123_mov},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},

    /* pshader instructions */
    {BWRITERSIO_CND,            instr_handler},
    {BWRITERSIO_CMP,            instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},
    {BWRITERSIO_TEX,            instr_ps_1_0123_texld},
    {BWRITERSIO_TEXBEM,         instr_handler},
    {BWRITERSIO_TEXBEML,        instr_handler},
    {BWRITERSIO_TEXM3x2PAD,     instr_handler},
    {BWRITERSIO_TEXM3x3PAD,     instr_handler},
    {BWRITERSIO_TEXM3x3SPEC,    instr_handler},
    {BWRITERSIO_TEXM3x3VSPEC,   instr_handler},
    {BWRITERSIO_TEXM3x3TEX,     instr_handler},
    {BWRITERSIO_TEXM3x3,        instr_handler},
    {BWRITERSIO_TEXM3x2DEPTH,   instr_handler},
    {BWRITERSIO_TEXM3x2TEX,     instr_handler},
    {BWRITERSIO_TEXDP3,         instr_handler},
    {BWRITERSIO_TEXDP3TEX,      instr_handler},
    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_1_0123_backend = {
    ps_1_x_header,
    end,
    ps_1_0123_srcreg,
    ps_1_0123_dstreg,
    sm_1_x_opcode,
    ps_1_0123_handlers
};

static void ps_1_4_srcreg(struct bc_writer *This, const struct shader_reg *reg,
                          struct bytecode_buffer *buffer) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    if(reg->rel_reg) {
        WARN("Relative addressing not supported in <= ps_3_0\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

        /* Can be mapped 1:1 */
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
            token |= (reg->type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            break;

        default:
            WARN("Invalid register type for ps_1_4 shader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    if(reg->srcmod == BWRITERSPSM_ABS || reg->srcmod == BWRITERSPSM_ABSNEG ||
       reg->srcmod == BWRITERSPSM_NOT) {
        WARN("Invalid source modifier %u for ps_1_4\n", reg->srcmod);
        This->state = E_INVALIDARG;
        return;
    }
    token |= d3d9_srcmod(reg->srcmod);
    put_dword(buffer, token);
}

static void ps_1_4_dstreg(struct bc_writer *This, const struct shader_reg *reg,
                          struct bytecode_buffer *buffer,
                          DWORD shift, DWORD mod) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */

    if(reg->rel_reg) {
        WARN("Relative addressing not supported for destination registers\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_TEMP: /* 1:1 mapping */
            token |= (reg->type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            break;

	/* For texkill */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

        default:
            WARN("Invalid dest register type for 1.x pshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->u.writemask);
    put_dword(buffer, token);
}

static void instr_ps_1_4_mov(struct bc_writer *This,
                             const struct instruction *instr,
                             struct bytecode_buffer *buffer) {
    DWORD token = D3DSIO_MOV & D3DSI_OPCODE_MASK;

    if(instr->dst.type == BWRITERSPR_TEMP && instr->src[0].type == BWRITERSPR_INPUT) {
        if(instr->src[0].regnum == This->t_regnum[0] ||
           instr->src[0].regnum == This->t_regnum[1] ||
           instr->src[0].regnum == This->t_regnum[2] ||
           instr->src[0].regnum == This->t_regnum[3] ||
           instr->src[0].regnum == This->t_regnum[4] ||
           instr->src[0].regnum == This->t_regnum[5]) {
            /* Similar to a regular mov, but a different opcode */
            token = D3DSIO_TEXCOORD & D3DSI_OPCODE_MASK;
        } else if(instr->src[0].regnum == This->v_regnum[0] ||
                  instr->src[0].regnum == This->v_regnum[1]) {
            /* Handled by the normal mov below. Just drop out of the if condition */
        } else {
            WARN("Unsupported varying -> temp mov in ps_1_4\n");
            This->state = E_INVALIDARG;
            return;
        }
    }

    This->funcs->opcode(This, instr, token, buffer);
    This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    This->funcs->srcreg(This, &instr->src[0], buffer);
}

static void instr_ps_1_4_texld(struct bc_writer *This,
                               const struct instruction *instr,
                               struct bytecode_buffer *buffer) {
    if(instr->src[1].type != BWRITERSPR_SAMPLER ||
       instr->src[1].regnum > 5) {
        WARN("Unsupported sampler type %u regnum %u\n",
             instr->src[1].type, instr->src[1].regnum);
        This->state = E_INVALIDARG;
        return;
    } else if(instr->dst.type != BWRITERSPR_TEMP) {
        WARN("Can only sample into a temp register\n");
        This->state = E_INVALIDARG;
        return;
    }

    if(instr->src[1].regnum != instr->dst.regnum) {
        WARN("Sampling from sampler s%u to register r%u is not possible in ps_1_4\n",
             instr->src[1].regnum, instr->dst.regnum);
        This->state = E_INVALIDARG;
        return;
    }

    This->funcs->opcode(This, instr, D3DSIO_TEX & D3DSI_OPCODE_MASK, buffer);
    This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    This->funcs->srcreg(This, &instr->src[0], buffer);
}

static const struct instr_handler_table ps_1_4_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_ps_1_4_mov},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},

    /* pshader instructions */
    {BWRITERSIO_CND,            instr_handler},
    {BWRITERSIO_CMP,            instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},
    {BWRITERSIO_TEX,            instr_ps_1_4_texld},
    {BWRITERSIO_TEXDEPTH,       instr_handler},
    {BWRITERSIO_BEM,            instr_handler},

    {BWRITERSIO_PHASE,          instr_handler},
    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_1_4_backend = {
    ps_1_4_header,
    end,
    ps_1_4_srcreg,
    ps_1_4_dstreg,
    sm_1_x_opcode,
    ps_1_4_handlers
};

static void write_constB(const struct bwriter_shader *shader, struct bytecode_buffer *buffer, BOOL len) {
    write_const(shader->constB, shader->num_cb, D3DSIO_DEFB, D3DSPR_CONSTBOOL, buffer, len);
}

static void write_constI(const struct bwriter_shader *shader, struct bytecode_buffer *buffer, BOOL len) {
    write_const(shader->constI, shader->num_ci, D3DSIO_DEFI, D3DSPR_CONSTINT, buffer, len);
}

static void vs_2_header(struct bc_writer *This,
                        const struct bwriter_shader *shader,
                        struct bytecode_buffer *buffer) {
    HRESULT hr;

    hr = vs_find_builtin_varyings(This, shader);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    /* Declare the shader type and version */
    put_dword(buffer, This->version);

    write_declarations(This, buffer, TRUE, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
    return;
}

static void vs_2_srcreg(struct bc_writer *This,
                        const struct shader_reg *reg,
                        struct bytecode_buffer *buffer) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    DWORD has_swizzle;
    DWORD component;
    DWORD d3d9reg;

    switch(reg->type) {
        case BWRITERSPR_OUTPUT:
            /* Map the swizzle to a writemask, the format expected
               by map_vs_output
             */
            switch(reg->u.swizzle) {
                case BWRITERVS_SWIZZLE_X:
                    component = BWRITERSP_WRITEMASK_0;
                    break;
                case BWRITERVS_SWIZZLE_Y:
                    component = BWRITERSP_WRITEMASK_1;
                    break;
                case BWRITERVS_SWIZZLE_Z:
                    component = BWRITERSP_WRITEMASK_2;
                    break;
                case BWRITERVS_SWIZZLE_W:
                    component = BWRITERSP_WRITEMASK_3;
                    break;
                default:
                    component = 0;
            }
            token |= map_vs_output(This, reg->regnum, component, &has_swizzle);
            break;

        case BWRITERSPR_RASTOUT:
        case BWRITERSPR_ATTROUT:
            /* These registers are mapped to input and output regs. They can be encoded in the bytecode,
             * but are unexpected. If we hit this path it might be due to an error.
             */
            FIXME("Unexpected register type %u\n", reg->type);
            /* drop through */
        case BWRITERSPR_INPUT:
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
        case BWRITERSPR_ADDR:
        case BWRITERSPR_CONSTINT:
        case BWRITERSPR_CONSTBOOL:
        case BWRITERSPR_LABEL:
            d3d9reg = d3d9_register(reg->type);
            token |= (d3d9reg << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= (d3d9reg << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            break;

        case BWRITERSPR_LOOP:
            if(reg->regnum != 0) {
                WARN("Only regnum 0 is supported for the loop index register in vs_2_0\n");
                This->state = E_INVALIDARG;
                return;
            }
            token |= (D3DSPR_LOOP << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= (D3DSPR_LOOP << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
            token |= 0 & D3DSP_REGNUM_MASK; /* No shift */
            break;

        case BWRITERSPR_PREDICATE:
            if(This->version != BWRITERVS_VERSION(2, 1)){
                WARN("Predicate register is allowed only in vs_2_x\n");
                This->state = E_INVALIDARG;
                return;
            }
            if(reg->regnum > 0) {
                WARN("Only predicate register 0 is supported\n");
                This->state = E_INVALIDARG;
                return;
            }
            token |= (D3DSPR_PREDICATE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= (D3DSPR_PREDICATE << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
            token |= 0 & D3DSP_REGNUM_MASK; /* No shift */

            break;

        default:
            WARN("Invalid register type for 2.0 vshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    token |= d3d9_srcmod(reg->srcmod);

    if(reg->rel_reg)
        token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;

    put_dword(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in the
     * binary code
     */
    if(token & D3DVS_ADDRMODE_RELATIVE)
        vs_2_srcreg(This, reg->rel_reg, buffer);
}

static void sm_2_opcode(struct bc_writer *This,
                        const struct instruction *instr,
                        DWORD token, struct bytecode_buffer *buffer) {
    /* From sm 2 onwards instruction length is encoded in the opcode field */
    int dsts = instr->has_dst ? 1 : 0;
    token |= instrlen(instr, instr->num_srcs, dsts) << D3DSI_INSTLENGTH_SHIFT;
    if(instr->comptype)
        token |= (d3d9_comparetype(instr->comptype) << 16) & (0xf << 16);
    if(instr->has_predicate)
        token |= D3DSHADER_INSTRUCTION_PREDICATED;
    put_dword(buffer,token);
}

static const struct instr_handler_table vs_2_0_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_SLT,            instr_handler},
    {BWRITERSIO_SGE,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_DST,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_SGN,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_LIT,            instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_MOVA,           instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend vs_2_0_backend = {
    vs_2_header,
    end,
    vs_2_srcreg,
    vs_12_dstreg,
    sm_2_opcode,
    vs_2_0_handlers
};

static const struct instr_handler_table vs_2_x_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_SLT,            instr_handler},
    {BWRITERSIO_SGE,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_DST,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_SGN,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_LIT,            instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_MOVA,           instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_IFC,            instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_BREAKC,         instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

    {BWRITERSIO_SETP,           instr_handler},
    {BWRITERSIO_BREAKP,         instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend vs_2_x_backend = {
    vs_2_header,
    end,
    vs_2_srcreg,
    vs_12_dstreg,
    sm_2_opcode,
    vs_2_x_handlers
};

static void write_samplers(const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    DWORD i;
    DWORD instr_dcl = D3DSIO_DCL | (2 << D3DSI_INSTLENGTH_SHIFT);
    DWORD token;
    const DWORD reg = (1<<31) |
        ((D3DSPR_SAMPLER << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK) |
        ((D3DSPR_SAMPLER << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2) |
        D3DSP_WRITEMASK_ALL;

    for(i = 0; i < shader->num_samplers; i++) {
        /* Write the DCL instruction */
        put_dword(buffer, instr_dcl);
        token = (1<<31);
        /* Already shifted */
        token |= (d3d9_sampler(shader->samplers[i].type)) & D3DSP_TEXTURETYPE_MASK;
        put_dword(buffer, token);
        token = reg | (shader->samplers[i].regnum & D3DSP_REGNUM_MASK);
        token |= d3d9_dstmod(shader->samplers[i].mod);
        put_dword(buffer, token);
    }
}

static void ps_2_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    HRESULT hr = find_ps_builtin_semantics(This, shader, 8);
    if(FAILED(hr)) {
        This->state = hr;
        return;
    }

    /* Declare the shader type and version */
    put_dword(buffer, This->version);
    write_declarations(This, buffer, TRUE, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_samplers(shader, buffer);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
}

static void ps_2_srcreg(struct bc_writer *This,
                        const struct shader_reg *reg,
                        struct bytecode_buffer *buffer) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    DWORD d3d9reg;
    if(reg->rel_reg) {
        WARN("Relative addressing not supported in <= ps_3_0\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

            /* Can be mapped 1:1 */
        case BWRITERSPR_TEMP:
        case BWRITERSPR_CONST:
        case BWRITERSPR_COLOROUT:
        case BWRITERSPR_CONSTBOOL:
        case BWRITERSPR_CONSTINT:
        case BWRITERSPR_SAMPLER:
        case BWRITERSPR_LABEL:
        case BWRITERSPR_DEPTHOUT:
            d3d9reg = d3d9_register(reg->type);
            token |= (d3d9reg << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= (d3d9reg << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            break;

        case BWRITERSPR_PREDICATE:
            if(This->version != BWRITERPS_VERSION(2, 1)){
                WARN("Predicate register not supported in ps_2_0\n");
                This->state = E_INVALIDARG;
            }
            if(reg->regnum) {
                WARN("Predicate register with regnum %u not supported\n",
                     reg->regnum);
                This->state = E_INVALIDARG;
            }
            token |= (D3DSPR_PREDICATE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= (D3DSPR_PREDICATE << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
            token |= 0 & D3DSP_REGNUM_MASK; /* No shift */
            break;

        default:
            WARN("Invalid register type for ps_2_0 shader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK; /* already shifted */

    token |= d3d9_srcmod(reg->srcmod);
    put_dword(buffer, token);
}

static void ps_2_0_dstreg(struct bc_writer *This,
                          const struct shader_reg *reg,
                          struct bytecode_buffer *buffer,
                          DWORD shift, DWORD mod) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    DWORD d3d9reg;

    if(reg->rel_reg) {
        WARN("Relative addressing not supported for destination registers\n");
        This->state = E_INVALIDARG;
        return;
    }

    switch(reg->type) {
        case BWRITERSPR_TEMP: /* 1:1 mapping */
        case BWRITERSPR_COLOROUT:
        case BWRITERSPR_DEPTHOUT:
            d3d9reg = d3d9_register(reg->type);
            token |= (d3d9reg << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= (d3d9reg << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            break;

        case BWRITERSPR_PREDICATE:
            if(This->version != BWRITERPS_VERSION(2, 1)){
                WARN("Predicate register not supported in ps_2_0\n");
                This->state = E_INVALIDARG;
            }
            token |= (D3DSPR_PREDICATE << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
            token |= (D3DSPR_PREDICATE << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
            token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */
            break;

	/* texkill uses the input register as a destination parameter */
        case BWRITERSPR_INPUT:
            token |= map_ps_input(This, reg);
            break;

        default:
            WARN("Invalid dest register type for 2.x pshader\n");
            This->state = E_INVALIDARG;
            return;
    }

    token |= (shift << D3DSP_DSTSHIFT_SHIFT) & D3DSP_DSTSHIFT_MASK;
    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->u.writemask);
    put_dword(buffer, token);
}

static const struct instr_handler_table ps_2_0_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_DP2ADD,         instr_handler},
    {BWRITERSIO_CMP,            instr_handler},

    {BWRITERSIO_TEX,            instr_handler},
    {BWRITERSIO_TEXLDP,         instr_handler},
    {BWRITERSIO_TEXLDB,         instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_2_0_backend = {
    ps_2_header,
    end,
    ps_2_srcreg,
    ps_2_0_dstreg,
    sm_2_opcode,
    ps_2_0_handlers
};

static const struct instr_handler_table ps_2_x_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_DP2ADD,         instr_handler},
    {BWRITERSIO_CMP,            instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_IFC,            instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_BREAKC,         instr_handler},
    {BWRITERSIO_RET,            instr_handler},

    {BWRITERSIO_TEX,            instr_handler},
    {BWRITERSIO_TEXLDP,         instr_handler},
    {BWRITERSIO_TEXLDB,         instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},
    {BWRITERSIO_DSX,            instr_handler},
    {BWRITERSIO_DSY,            instr_handler},

    {BWRITERSIO_SETP,           instr_handler},
    {BWRITERSIO_BREAKP,         instr_handler},

    {BWRITERSIO_TEXLDD,         instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_2_x_backend = {
    ps_2_header,
    end,
    ps_2_srcreg,
    ps_2_0_dstreg,
    sm_2_opcode,
    ps_2_x_handlers
};

static void sm_3_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    /* Declare the shader type and version */
    put_dword(buffer, This->version);

    write_declarations(This, buffer, TRUE, shader->inputs, shader->num_inputs, BWRITERSPR_INPUT);
    write_declarations(This, buffer, TRUE, shader->outputs, shader->num_outputs, BWRITERSPR_OUTPUT);
    write_constF(shader, buffer, TRUE);
    write_constB(shader, buffer, TRUE);
    write_constI(shader, buffer, TRUE);
    write_samplers(shader, buffer);
    return;
}

static void sm_3_srcreg(struct bc_writer *This,
                        const struct shader_reg *reg,
                        struct bytecode_buffer *buffer) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    DWORD d3d9reg;

    d3d9reg = d3d9_register(reg->type);
    token |= (d3d9reg << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
    token |= (d3d9reg << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
    token |= reg->regnum & D3DSP_REGNUM_MASK;

    token |= d3d9_swizzle(reg->u.swizzle) & D3DVS_SWIZZLE_MASK;
    token |= d3d9_srcmod(reg->srcmod);

    if(reg->rel_reg) {
        if(reg->type == BWRITERSPR_CONST && This->version == BWRITERPS_VERSION(3, 0)) {
            WARN("c%u[...] is unsupported in ps_3_0\n", reg->regnum);
            This->state = E_INVALIDARG;
            return;
        }
        if(((reg->rel_reg->type == BWRITERSPR_ADDR && This->version == BWRITERVS_VERSION(3, 0)) ||
           reg->rel_reg->type == BWRITERSPR_LOOP) &&
           reg->rel_reg->regnum == 0) {
            token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
        } else {
            WARN("Unsupported relative addressing register\n");
            This->state = E_INVALIDARG;
            return;
        }
    }

    put_dword(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in the
     * binary code
     */
    if(token & D3DVS_ADDRMODE_RELATIVE) {
        sm_3_srcreg(This, reg->rel_reg, buffer);
    }
}

static void sm_3_dstreg(struct bc_writer *This,
                        const struct shader_reg *reg,
                        struct bytecode_buffer *buffer,
                        DWORD shift, DWORD mod) {
    DWORD token = (1 << 31); /* Bit 31 of registers is 1 */
    DWORD d3d9reg;

    if(reg->rel_reg) {
        if(This->version == BWRITERVS_VERSION(3, 0) &&
           reg->type == BWRITERSPR_OUTPUT) {
            token |= D3DVS_ADDRMODE_RELATIVE & D3DVS_ADDRESSMODE_MASK;
        } else {
            WARN("Relative addressing not supported for this shader type or register type\n");
            This->state = E_INVALIDARG;
            return;
        }
    }

    d3d9reg = d3d9_register(reg->type);
    token |= (d3d9reg << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK;
    token |= (d3d9reg << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2;
    token |= reg->regnum & D3DSP_REGNUM_MASK; /* No shift */

    token |= d3d9_dstmod(mod);

    token |= d3d9_writemask(reg->u.writemask);
    put_dword(buffer, token);

    /* vs_2_0 and newer write the register containing the index explicitly in the
     * binary code
     */
    if(token & D3DVS_ADDRMODE_RELATIVE) {
        sm_3_srcreg(This, reg->rel_reg, buffer);
    }
}

static const struct instr_handler_table vs_3_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_SLT,            instr_handler},
    {BWRITERSIO_SGE,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_DST,            instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_SGN,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_LIT,            instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_MOVA,           instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_IFC,            instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_BREAKC,         instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

    {BWRITERSIO_SETP,           instr_handler},
    {BWRITERSIO_BREAKP,         instr_handler},
    {BWRITERSIO_TEXLDL,         instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend vs_3_backend = {
    sm_3_header,
    end,
    sm_3_srcreg,
    sm_3_dstreg,
    sm_2_opcode,
    vs_3_handlers
};

static const struct instr_handler_table ps_3_handlers[] = {
    {BWRITERSIO_ADD,            instr_handler},
    {BWRITERSIO_NOP,            instr_handler},
    {BWRITERSIO_MOV,            instr_handler},
    {BWRITERSIO_SUB,            instr_handler},
    {BWRITERSIO_MAD,            instr_handler},
    {BWRITERSIO_MUL,            instr_handler},
    {BWRITERSIO_RCP,            instr_handler},
    {BWRITERSIO_RSQ,            instr_handler},
    {BWRITERSIO_DP3,            instr_handler},
    {BWRITERSIO_DP4,            instr_handler},
    {BWRITERSIO_MIN,            instr_handler},
    {BWRITERSIO_MAX,            instr_handler},
    {BWRITERSIO_ABS,            instr_handler},
    {BWRITERSIO_EXP,            instr_handler},
    {BWRITERSIO_LOG,            instr_handler},
    {BWRITERSIO_EXPP,           instr_handler},
    {BWRITERSIO_LOGP,           instr_handler},
    {BWRITERSIO_LRP,            instr_handler},
    {BWRITERSIO_FRC,            instr_handler},
    {BWRITERSIO_CRS,            instr_handler},
    {BWRITERSIO_NRM,            instr_handler},
    {BWRITERSIO_SINCOS,         instr_handler},
    {BWRITERSIO_M4x4,           instr_handler},
    {BWRITERSIO_M4x3,           instr_handler},
    {BWRITERSIO_M3x4,           instr_handler},
    {BWRITERSIO_M3x3,           instr_handler},
    {BWRITERSIO_M3x2,           instr_handler},
    {BWRITERSIO_POW,            instr_handler},
    {BWRITERSIO_DP2ADD,         instr_handler},
    {BWRITERSIO_CMP,            instr_handler},

    {BWRITERSIO_CALL,           instr_handler},
    {BWRITERSIO_CALLNZ,         instr_handler},
    {BWRITERSIO_REP,            instr_handler},
    {BWRITERSIO_ENDREP,         instr_handler},
    {BWRITERSIO_IF,             instr_handler},
    {BWRITERSIO_LABEL,          instr_handler},
    {BWRITERSIO_IFC,            instr_handler},
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_BREAKC,         instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

    {BWRITERSIO_SETP,           instr_handler},
    {BWRITERSIO_BREAKP,         instr_handler},
    {BWRITERSIO_TEXLDL,         instr_handler},

    {BWRITERSIO_TEX,            instr_handler},
    {BWRITERSIO_TEXLDP,         instr_handler},
    {BWRITERSIO_TEXLDB,         instr_handler},
    {BWRITERSIO_TEXKILL,        instr_handler},
    {BWRITERSIO_DSX,            instr_handler},
    {BWRITERSIO_DSY,            instr_handler},
    {BWRITERSIO_TEXLDD,         instr_handler},

    {BWRITERSIO_END,            NULL},
};

static const struct bytecode_backend ps_3_backend = {
    sm_3_header,
    end,
    sm_3_srcreg,
    sm_3_dstreg,
    sm_2_opcode,
    ps_3_handlers
};

static void init_vs10_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 1.0 writer\n");
    writer->funcs = &vs_1_x_backend;
}

static void init_vs11_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 1.1 writer\n");
    writer->funcs = &vs_1_x_backend;
}

static void init_vs20_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 2.0 writer\n");
    writer->funcs = &vs_2_0_backend;
}

static void init_vs2x_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 2.x writer\n");
    writer->funcs = &vs_2_x_backend;
}

static void init_vs30_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 3.0 writer\n");
    writer->funcs = &vs_3_backend;
}

static void init_ps10_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.0 writer\n");
    writer->funcs = &ps_1_0123_backend;
}

static void init_ps11_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.1 writer\n");
    writer->funcs = &ps_1_0123_backend;
}

static void init_ps12_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.2 writer\n");
    writer->funcs = &ps_1_0123_backend;
}

static void init_ps13_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.3 writer\n");
    writer->funcs = &ps_1_0123_backend;
}

static void init_ps14_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 1.4 writer\n");
    writer->funcs = &ps_1_4_backend;
}

static void init_ps20_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 2.0 writer\n");
    writer->funcs = &ps_2_0_backend;
}

static void init_ps2x_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 2.x writer\n");
    writer->funcs = &ps_2_x_backend;
}

static void init_ps30_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 pixel shader 3.0 writer\n");
    writer->funcs = &ps_3_backend;
}

static struct bc_writer *create_writer(DWORD version, DWORD dxversion) {
    struct bc_writer *ret = asm_alloc(sizeof(*ret));

    if(!ret) {
        WARN("Failed to allocate a bytecode writer instance\n");
        return NULL;
    }

    switch(version) {
        case BWRITERVS_VERSION(1, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 1.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_vs10_dx9_writer(ret);
            break;
        case BWRITERVS_VERSION(1, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 1.1 requested: %u\n", dxversion);
                goto fail;
            }
            init_vs11_dx9_writer(ret);
            break;
        case BWRITERVS_VERSION(2, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 2.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_vs20_dx9_writer(ret);
            break;
        case BWRITERVS_VERSION(2, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 2.x requested: %u\n", dxversion);
                goto fail;
            }
            init_vs2x_dx9_writer(ret);
            break;
        case BWRITERVS_VERSION(3, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 3.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_vs30_dx9_writer(ret);
            break;

        case BWRITERPS_VERSION(1, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps10_dx9_writer(ret);
            break;
        case BWRITERPS_VERSION(1, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.1 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps11_dx9_writer(ret);
            break;
        case BWRITERPS_VERSION(1, 2):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.2 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps12_dx9_writer(ret);
            break;
        case BWRITERPS_VERSION(1, 3):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.3 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps13_dx9_writer(ret);
            break;
        case BWRITERPS_VERSION(1, 4):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.4 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps14_dx9_writer(ret);
            break;

        case BWRITERPS_VERSION(2, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 2.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps20_dx9_writer(ret);
            break;

        case BWRITERPS_VERSION(2, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 2.x requested: %u\n", dxversion);
                goto fail;
            }
            init_ps2x_dx9_writer(ret);
            break;

        case BWRITERPS_VERSION(3, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 3.0 requested: %u\n", dxversion);
                goto fail;
            }
            init_ps30_dx9_writer(ret);
            break;

        default:
            WARN("Unexpected shader version requested: %08x\n", version);
            goto fail;
    }
    ret->version = version;
    return ret;

fail:
    asm_free(ret);
    return NULL;
}

static HRESULT call_instr_handler(struct bc_writer *writer,
                                  const struct instruction *instr,
                                  struct bytecode_buffer *buffer) {
    DWORD i=0;

    while(writer->funcs->instructions[i].opcode != BWRITERSIO_END) {
        if(instr->opcode == writer->funcs->instructions[i].opcode) {
            if(!writer->funcs->instructions[i].func) {
                WARN("Opcode %u not supported by this profile\n", instr->opcode);
                return E_INVALIDARG;
            }
            writer->funcs->instructions[i].func(writer, instr, buffer);
            return S_OK;
        }
        i++;
    }

    FIXME("Unhandled instruction %u - %s\n", instr->opcode,
          debug_print_opcode(instr->opcode));
    return E_INVALIDARG;
}

/* SlWriteBytecode (wineshader.@)
 *
 * Writes shader version specific bytecode from the shader passed in.
 * The returned bytecode can be passed to the Direct3D runtime like
 * IDirect3DDevice9::Create*Shader.
 *
 * Parameters:
 *  shader: Shader to translate into bytecode
 *  version: Shader version to generate(d3d version token)
 *  dxversion: DirectX version the code targets
 *  result: the resulting shader bytecode
 *
 * Return values:
 *  S_OK on success
 */
DWORD SlWriteBytecode(const struct bwriter_shader *shader, int dxversion, DWORD **result) {
    struct bc_writer *writer;
    struct bytecode_buffer *buffer = NULL;
    HRESULT hr;
    unsigned int i;

    if(!shader){
        ERR("NULL shader structure, aborting\n");
        return E_FAIL;
    }
    writer = create_writer(shader->version, dxversion);
    *result = NULL;

    if(!writer) {
        WARN("Could not create a bytecode writer instance. Either unsupported version\n");
        WARN("or out of memory\n");
        hr = E_FAIL;
        goto error;
    }

    buffer = allocate_buffer();
    if(!buffer) {
        WARN("Failed to allocate a buffer for the shader bytecode\n");
        hr = E_FAIL;
        goto error;
    }

    writer->funcs->header(writer, shader, buffer);
    if(FAILED(writer->state)) {
        hr = writer->state;
        goto error;
    }

    for(i = 0; i < shader->num_instrs; i++) {
        hr = call_instr_handler(writer, shader->instr[i], buffer);
        if(FAILED(hr)) {
            goto error;
        }
    }

    if(FAILED(writer->state)) {
        hr = writer->state;
        goto error;
    }

    writer->funcs->end(writer, shader, buffer);

    if(FAILED(buffer->state)) {
        hr = buffer->state;
        goto error;
    }

    /* Cut off unneeded memory from the result buffer */
    *result = asm_realloc(buffer->data,
                         sizeof(DWORD) * buffer->size);
    if(!*result) {
        *result = buffer->data;
    }
    buffer->data = NULL;
    hr = S_OK;

error:
    if(buffer) {
        asm_free(buffer->data);
        asm_free(buffer);
    }
    asm_free(writer);
    return hr;
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
