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
static void end(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    put_dword(buffer, D3DSIO_END);
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

static void instr_handler(struct bc_writer *This,
                          const struct instruction *instr,
                          struct bytecode_buffer *buffer) {
    DWORD token = d3d9_opcode(instr->opcode);
    TRACE("token: %x\n", token);

    This->funcs->opcode(This, instr, token, buffer);
    if(instr->has_dst) This->funcs->dstreg(This, &instr->dst, buffer, instr->shift, instr->dstmod);
    write_srcregs(This, instr, buffer);
}

static void sm_2_opcode(struct bc_writer *This,
                        const struct instruction *instr,
                        DWORD token, struct bytecode_buffer *buffer) {
    /* From sm 2 onwards instruction length is encoded in the opcode field */
    int dsts = instr->has_dst ? 1 : 0;
    token |= instrlen(instr, instr->num_srcs, dsts) << D3DSI_INSTLENGTH_SHIFT;
    put_dword(buffer,token);
}

static void sm_3_header(struct bc_writer *This, const struct bwriter_shader *shader, struct bytecode_buffer *buffer) {
    /* Declare the shader type and version */
    put_dword(buffer, This->version);
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

    token |= d3d9_swizzle(reg->swizzle) & D3DVS_SWIZZLE_MASK;
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

    token |= d3d9_writemask(reg->writemask);
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
    {BWRITERSIO_ELSE,           instr_handler},
    {BWRITERSIO_ENDIF,          instr_handler},
    {BWRITERSIO_BREAK,          instr_handler},
    {BWRITERSIO_LOOP,           instr_handler},
    {BWRITERSIO_RET,            instr_handler},
    {BWRITERSIO_ENDLOOP,        instr_handler},

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

static void init_vs30_dx9_writer(struct bc_writer *writer) {
    TRACE("Creating DirectX9 vertex shader 3.0 writer\n");
    writer->funcs = &vs_3_backend;
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
            /* TODO: Set the appropriate writer backend */
            break;
        case BWRITERVS_VERSION(1, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 1.1 requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
            break;
        case BWRITERVS_VERSION(2, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 2.0 requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
            break;
        case BWRITERVS_VERSION(2, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for vertex shader 2.x requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
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
            /* TODO: Set the appropriate writer backend */
            break;
        case BWRITERPS_VERSION(1, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.1 requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
            break;
        case BWRITERPS_VERSION(1, 2):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.2 requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
            break;
        case BWRITERPS_VERSION(1, 3):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.3 requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
            break;
        case BWRITERPS_VERSION(1, 4):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 1.4 requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
            break;

        case BWRITERPS_VERSION(2, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 2.0 requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
            break;

        case BWRITERPS_VERSION(2, 1):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 2.x requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
            break;

        case BWRITERPS_VERSION(3, 0):
            if(dxversion != 9) {
                WARN("Unsupported dxversion for pixel shader 3.0 requested: %u\n", dxversion);
                goto fail;
            }
            /* TODO: Set the appropriate writer backend */
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

    FIXME("Unhandled instruction %u\n", instr->opcode);
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
