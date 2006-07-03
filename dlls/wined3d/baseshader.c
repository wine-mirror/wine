/*
 * shaders implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
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

#include "config.h"
#include <string.h>
#include <stdio.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

#define GLNAME_REQUIRE_GLSL  ((const char *)1)

inline static BOOL shader_is_version_token(DWORD token) {
    return shader_is_pshader_version(token) ||
           shader_is_vshader_version(token);
}

int shader_addline(
    SHADER_BUFFER* buffer,  
    const char *format, ...) {

    char* base = buffer->buffer + buffer->bsize;
    int rc;

    va_list args;
    va_start(args, format);
    rc = vsnprintf(base, SHADER_PGMSIZE - 1 - buffer->bsize, format, args);
    va_end(args);

    if (rc < 0 ||                                   /* C89 */ 
        rc > SHADER_PGMSIZE - 1 - buffer->bsize) {  /* C99 */

        ERR("The buffer allocated for the shader program string "
            "is too small at %d bytes.\n", SHADER_PGMSIZE);
        buffer->bsize = SHADER_PGMSIZE - 1;
        return -1;
    }

    buffer->bsize += rc;
    buffer->lineNo++;
    TRACE("GL HW (%u, %u) : %s", buffer->lineNo, buffer->bsize, base); 
    return 0;
}

const SHADER_OPCODE* shader_get_opcode(
    IWineD3DBaseShader *iface, const DWORD code) {

    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl*) iface;

    DWORD i = 0;
    DWORD hex_version = This->baseShader.hex_version;
    const SHADER_OPCODE *shader_ins = This->baseShader.shader_ins;

    /** TODO: use dichotomic search */
    while (NULL != shader_ins[i].name) {
        if (((code & D3DSI_OPCODE_MASK) == shader_ins[i].opcode) &&
            (((hex_version >= shader_ins[i].min_version) && (hex_version <= shader_ins[i].max_version)) ||
            ((shader_ins[i].min_version == 0) && (shader_ins[i].max_version == 0)))) {
            return &shader_ins[i];
        }
        ++i;
    }
    FIXME("Unsupported opcode %#lx(%ld) masked %#lx, shader version %#lx\n", 
       code, code, code & D3DSI_OPCODE_MASK, hex_version);
    return NULL;
}

/* Read a parameter opcode from the input stream,
 * and possibly a relative addressing token.
 * Return the number of tokens read */
int shader_get_param(
    IWineD3DBaseShader* iface,
    const DWORD* pToken,
    DWORD* param,
    DWORD* addr_token) {

    /* PS >= 3.0 have relative addressing (with token)
     * VS >= 2.0 have relative addressing (with token)
     * VS >= 1.0 < 2.0 have relative addressing (without token)
     * The version check below should work in general */

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    char rel_token = D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 2 &&
        ((*pToken & D3DSHADER_ADDRESSMODE_MASK) == D3DSHADER_ADDRMODE_RELATIVE);

    *param = *pToken;
    *addr_token = rel_token? *(pToken + 1): 0;
    return rel_token? 2:1;
}

/* Return the number of parameters to skip for an opcode */
static inline int shader_skip_opcode(
    IWineD3DBaseShaderImpl* This,
    const SHADER_OPCODE* curOpcode,
    DWORD opcode_token) {

   /* Shaders >= 2.0 may contain address tokens, but fortunately they
    * have a useful legnth mask - use it here. Shaders 1.0 contain no such tokens */

    return (D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 2)?
        ((opcode_token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT):
        curOpcode->num_params;
}

/* Read the parameters of an unrecognized opcode from the input stream
 * Return the number of tokens read. 
 * 
 * Note: This function assumes source or destination token format.
 * It will not work with specially-formatted tokens like DEF or DCL, 
 * but hopefully those would be recognized */

int shader_skip_unrecognized(
    IWineD3DBaseShader* iface,
    const DWORD* pToken) {

    int tokens_read = 0;
    int i = 0;

    /* TODO: Think of a good name for 0x80000000 and replace it with a constant */
    while (*pToken & 0x80000000) {

        DWORD param, addr_token;
        tokens_read += shader_get_param(iface, pToken, &param, &addr_token);
        pToken += tokens_read;

        FIXME("Unrecognized opcode param: token=%08lX "
            "addr_token=%08lX name=", param, addr_token);
        shader_dump_param(iface, param, addr_token, i);
        FIXME("\n");
        ++i;
    }
    return tokens_read;
}

/* Convert floating point offset relative
 * to a register file to an absolute offset for float constants */

unsigned int shader_get_float_offset(const DWORD reg) {

     unsigned int regnum = reg & D3DSP_REGNUM_MASK;
     int regtype = shader_get_regtype(reg);

     switch (regtype) {
        case D3DSPR_CONST: return regnum;
        case D3DSPR_CONST2: return 2048 + regnum;
        case D3DSPR_CONST3: return 4096 + regnum;
        case D3DSPR_CONST4: return 6144 + regnum;
        default:
            FIXME("Unsupported register type: %d\n", regtype);
            return regnum;
     }
}

static void shader_parse_decl_usage(
    DWORD *semantics_map,
    DWORD usage_token, DWORD param) {

    unsigned int usage = (usage_token & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT;
    unsigned int usage_idx = (usage_token & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;
    unsigned int regnum = param & D3DSP_REGNUM_MASK;

    switch(usage) {
        case D3DDECLUSAGE_POSITION:
            if (usage_idx == 0) { /* tween data */
                TRACE("Setting position to %d\n", regnum);
                semantics_map[WINED3DSHADERDECLUSAGE_POSITION] = param;
            } else {
                /* TODO: position indexes go from 0-8!!*/
                TRACE("Setting position 2 to %d because usage_idx = %d\n", regnum, usage_idx);
                /* robots uses positions up to 8, the position arrays are just packed.*/
                if (usage_idx > 1) {
                    TRACE("Loaded for position %d (greater than 2)\n", usage_idx);
                }
                semantics_map[WINED3DSHADERDECLUSAGE_POSITION2 + usage_idx-1] = param;
            }
            break;

        case D3DDECLUSAGE_BLENDINDICES:
            TRACE("Setting BLENDINDICES to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_BLENDINDICES] = param;
            if (usage_idx != 0) FIXME("Extended BLENDINDICES\n");
            break;

        case D3DDECLUSAGE_BLENDWEIGHT:
            TRACE("Setting BLENDWEIGHT to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_BLENDWEIGHT] = param;
            if (usage_idx != 0) FIXME("Extended blend weights\n");
            break;

        case D3DDECLUSAGE_NORMAL:
            if (usage_idx == 0) { /* tween data */
                TRACE("Setting normal to %d\n", regnum);
                semantics_map[WINED3DSHADERDECLUSAGE_NORMAL] = param;
            } else {
                TRACE("Setting normal 2 to %d because usage = %d\n", regnum, usage_idx);
                semantics_map[WINED3DSHADERDECLUSAGE_NORMAL2] = param;
            }
            break;

        case D3DDECLUSAGE_PSIZE:
            TRACE("Setting PSIZE to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_PSIZE] = param;
            if (usage_idx != 0) FIXME("Extended PSIZE\n");
            break;

        case D3DDECLUSAGE_COLOR:
            if (usage_idx == 0)  {
                TRACE("Setting DIFFUSE to %d\n", regnum);
                semantics_map[WINED3DSHADERDECLUSAGE_DIFFUSE] = param;
            } else {
                TRACE("Setting SPECULAR to %d\n", regnum);
                semantics_map[WINED3DSHADERDECLUSAGE_SPECULAR] = param;
            }
            break;

        case D3DDECLUSAGE_TEXCOORD:
            if (usage_idx > 7) {
                FIXME("Program uses texture coordinate %d but only 0-7 have been "
                    "implemented\n", usage_idx);
            } else {
                TRACE("Setting TEXCOORD %d  to %d\n", usage_idx, regnum);
                semantics_map[WINED3DSHADERDECLUSAGE_TEXCOORD0 + usage_idx] = param;
            }
            break;

        case D3DDECLUSAGE_TANGENT:
            TRACE("Setting TANGENT to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_TANGENT] = param;
            break;

        case D3DDECLUSAGE_BINORMAL:
            TRACE("Setting BINORMAL to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_BINORMAL] = param;
            break;

        case D3DDECLUSAGE_TESSFACTOR:
            TRACE("Setting TESSFACTOR to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_TESSFACTOR] = param;
            break;

        case D3DDECLUSAGE_POSITIONT:
            if (usage_idx == 0) { /* tween data */
                FIXME("Setting positiont to %d\n", regnum);
                semantics_map[WINED3DSHADERDECLUSAGE_POSITIONT] = param;
            } else {
                FIXME("Setting positiont 2 to %d because usage = %d\n", regnum, usage_idx);
                semantics_map[WINED3DSHADERDECLUSAGE_POSITIONT2] = param;
                if (usage_idx != 0) FIXME("Extended positiont\n");
            }
            break;

        case D3DDECLUSAGE_FOG:
            TRACE("Setting FOG to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_FOG] = param;
            break;

        case D3DDECLUSAGE_DEPTH:
            TRACE("Setting DEPTH to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_DEPTH] = param;
            break;

        case D3DDECLUSAGE_SAMPLE:
            TRACE("Setting SAMPLE to %d\n", regnum);
            semantics_map[WINED3DSHADERDECLUSAGE_SAMPLE] = param;
            break;

        default:
            FIXME("Unrecognised dcl %#x", usage);
    }
}

/* Note that this does not count the loop register
 * as an address register. */

void shader_get_registers_used(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    CONST DWORD* pToken) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;

    /* There are some minor differences between pixel and vertex shaders */
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);

    if (pToken == NULL)
        return;

    while (D3DVS_END() != *pToken) {
        CONST SHADER_OPCODE* curOpcode;
        DWORD opcode_token;

        /* Skip version */
        if (shader_is_version_token(*pToken)) {
             ++pToken;
             continue;

        /* Skip comments */
        } else if (shader_is_comment(*pToken)) {
             DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
             ++pToken;
             pToken += comment_len;
             continue;
        }

        /* Fetch opcode */
        opcode_token = *pToken++;
        curOpcode = shader_get_opcode(iface, opcode_token);

        /* Unhandled opcode, and its parameters */
        if (NULL == curOpcode) {
           while (*pToken & 0x80000000)
               ++pToken;

        /* Handle declarations */
        } else if (D3DSIO_DCL == curOpcode->opcode) {

            DWORD usage = *pToken++;
            DWORD param = *pToken++;
            DWORD regtype = shader_get_regtype(param);
            unsigned int regnum = param & D3DSP_REGNUM_MASK;

            /* Vshader: mark attributes used
               Pshader: mark 3.0 input registers used, save token */
            if (D3DSPR_INPUT == regtype) {

                if (!pshader)
                    reg_maps->attributes[regnum] = 1;
                else
                    reg_maps->packed_input[regnum] = 1;

                shader_parse_decl_usage(reg_maps->semantics_in, usage, param);

            /* Vshader: mark 3.0 output registers used, save token */
            } else if (D3DSPR_OUTPUT == regtype) {
                reg_maps->packed_output[regnum] = 1;
                shader_parse_decl_usage(reg_maps->semantics_out, usage, param);

            /* Save sampler usage token */
            } else if (D3DSPR_SAMPLER == regtype)
                reg_maps->samplers[regnum] = usage;

        /* Skip definitions (for now) */
        } else if (D3DSIO_DEF == curOpcode->opcode) {
            pToken += curOpcode->num_params;

        /* If there's a loop in the shader */
        } else if (D3DSIO_LOOP == curOpcode->opcode) {
            reg_maps->loop = 1;
            pToken += curOpcode->num_params;
        
        /* Set texture, address, temporary registers */
        } else {
            int i, limit;

            /* Declare 1.X samplers implicitly, based on the destination reg. number */
            if (D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) == 1 && 
                (D3DSIO_TEX == curOpcode->opcode ||
                 D3DSIO_TEXBEM == curOpcode->opcode ||
                 D3DSIO_TEXM3x2TEX == curOpcode->opcode ||
                 D3DSIO_TEXM3x3TEX == curOpcode->opcode)) {

                /* Fake sampler usage, only set reserved bit and ttype */
                DWORD sampler_code = *pToken & D3DSP_REGNUM_MASK;
                reg_maps->samplers[sampler_code] = (0x1 << 31) | D3DSTT_2D;
                
            } else if (D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) == 1 &&
                (D3DSIO_TEXM3x3SPEC == curOpcode->opcode ||
                 D3DSIO_TEXM3x3VSPEC == curOpcode->opcode)) {

                /* 3D sampler usage, only set reserved bit and ttype
                 * FIXME: This could be either Cube or Volume, but we wouldn't know unless
                 * we waited to generate the shader until the textures were all bound.
                 * For now, use Cube textures because they are more common. */
                DWORD sampler_code = *pToken & D3DSP_REGNUM_MASK;
                reg_maps->samplers[sampler_code] = (0x1 << 31) | D3DSTT_CUBE;
            }

            /* This will loop over all the registers and try to
             * make a bitmask of the ones we're interested in. 
             *
             * Relative addressing tokens are ignored, but that's 
             * okay, since we'll catch any address registers when 
             * they are initialized (required by spec) */

            limit = (opcode_token & D3DSHADER_INSTRUCTION_PREDICATED)?
                curOpcode->num_params + 1: curOpcode->num_params;

            for (i = 0; i < limit; ++i) {

                DWORD param, addr_token, reg, regtype;
                pToken += shader_get_param(iface, pToken, &param, &addr_token);

                regtype = shader_get_regtype(param);
                reg = param & D3DSP_REGNUM_MASK;

                if (D3DSPR_TEXTURE == regtype) { /* vs: D3DSPR_ADDR */

                    if (pshader)
                        reg_maps->texcoord[reg] = 1;
                    else
                        reg_maps->address[reg] = 1;
                }

                else if (D3DSPR_TEMP == regtype)
                    reg_maps->temporary[reg] = 1;

                else if (D3DSPR_INPUT == regtype && !pshader)
                    reg_maps->attributes[reg] = 1;
             }
        }
    }
}

static void shader_dump_decl_usage(
    IWineD3DBaseShaderImpl* This,
    DWORD decl, 
    DWORD param) {

    DWORD regtype = shader_get_regtype(param);

    TRACE("dcl");

    if (regtype == D3DSPR_SAMPLER) {
        DWORD ttype = decl & D3DSP_TEXTURETYPE_MASK;

        switch (ttype) {
            case D3DSTT_2D: TRACE("_2d"); break;
            case D3DSTT_CUBE: TRACE("_cube"); break;
            case D3DSTT_VOLUME: TRACE("_volume"); break;
            default: TRACE("_unknown_ttype(%08lx)", ttype); 
       }

    } else { 

        DWORD usage = decl & D3DSP_DCL_USAGE_MASK;
        DWORD idx = (decl & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;

        /* Pixel shaders 3.0 don't have usage semantics */
        char pshader = shader_is_pshader_version(This->baseShader.hex_version);
        if (pshader && This->baseShader.hex_version < D3DPS_VERSION(3,0))
            return;
        else
            TRACE("_");

        switch(usage) {
        case D3DDECLUSAGE_POSITION:
            TRACE("%s%ld", "position", idx);
            break;
        case D3DDECLUSAGE_BLENDINDICES:
            TRACE("%s", "blend");
            break;
        case D3DDECLUSAGE_BLENDWEIGHT:
            TRACE("%s", "weight");
            break;
        case D3DDECLUSAGE_NORMAL:
            TRACE("%s%ld", "normal", idx);
            break;
        case D3DDECLUSAGE_PSIZE:
            TRACE("%s", "psize");
            break;
        case D3DDECLUSAGE_COLOR:
            if(idx == 0)  {
                TRACE("%s", "color");
            } else {
                TRACE("%s%ld", "specular", (idx - 1));
            }
            break;
        case D3DDECLUSAGE_TEXCOORD:
            TRACE("%s%ld", "texture", idx);
            break;
        case D3DDECLUSAGE_TANGENT:
            TRACE("%s", "tangent");
            break;
        case D3DDECLUSAGE_BINORMAL:
            TRACE("%s", "binormal");
            break;
        case D3DDECLUSAGE_TESSFACTOR:
            TRACE("%s", "tessfactor");
            break;
        case D3DDECLUSAGE_POSITIONT:
            TRACE("%s%ld", "positionT", idx);
            break;
        case D3DDECLUSAGE_FOG:
            TRACE("%s", "fog");
            break;
        case D3DDECLUSAGE_DEPTH:
            TRACE("%s", "depth");
            break;
        case D3DDECLUSAGE_SAMPLE:
            TRACE("%s", "sample");
            break;
        default:
            FIXME("unknown_semantics(%08lx)", usage);
        }
    }
}

static void shader_dump_arr_entry(
    IWineD3DBaseShader *iface,
    const DWORD param,
    const DWORD addr_token,
    unsigned int reg,
    int input) {

    char relative =
        ((param & D3DSHADER_ADDRESSMODE_MASK) == D3DSHADER_ADDRMODE_RELATIVE);

    if (relative) {
        TRACE("[");
        if (addr_token)
            shader_dump_param(iface, addr_token, 0, input);
        else
            TRACE("a0.x");
        TRACE(" + ");
     }
     TRACE("%u", reg);
     if (relative)
         TRACE("]");
}

void shader_dump_param(
    IWineD3DBaseShader *iface,
    const DWORD param, 
    const DWORD addr_token,
    int input) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    static const char* rastout_reg_names[] = { "oPos", "oFog", "oPts" };
    char swizzle_reg_chars[4];

    DWORD reg = param & D3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);

    /* There are some minor differences between pixel and vertex shaders */
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);

    /* For one, we'd prefer color components to be shown for pshaders.
     * FIXME: use the swizzle function for this */

    swizzle_reg_chars[0] = pshader? 'r': 'x';
    swizzle_reg_chars[1] = pshader? 'g': 'y';
    swizzle_reg_chars[2] = pshader? 'b': 'z';
    swizzle_reg_chars[3] = pshader? 'a': 'w';

    if (input) {
        if ( ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) ||
             ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_BIASNEG) ||
             ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_SIGNNEG) ||
             ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_X2NEG) )
            TRACE("-");
        else if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_COMP)
            TRACE("1-");
        else if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NOT)
            TRACE("!");
    }

    switch (regtype) {
        case D3DSPR_TEMP:
            TRACE("r%lu", reg);
            break;
        case D3DSPR_INPUT:
            TRACE("v");
            shader_dump_arr_entry(iface, param, addr_token, reg, input);
            break;
        case D3DSPR_CONST:
        case D3DSPR_CONST2:
        case D3DSPR_CONST3:
        case D3DSPR_CONST4:
            TRACE("c");
            shader_dump_arr_entry(iface, param, addr_token, shader_get_float_offset(param), input);
            break;
        case D3DSPR_TEXTURE: /* vs: case D3DSPR_ADDR */
            TRACE("%c%lu", (pshader? 't':'a'), reg);
            break;        
        case D3DSPR_RASTOUT:
            TRACE("%s", rastout_reg_names[reg]);
            break;
        case D3DSPR_COLOROUT:
            TRACE("oC%lu", reg);
            break;
        case D3DSPR_DEPTHOUT:
            TRACE("oDepth");
            break;
        case D3DSPR_ATTROUT:
            TRACE("oD%lu", reg);
            break;
        case D3DSPR_TEXCRDOUT: 

            /* Vertex shaders >= 3.0 use general purpose output registers
             * (D3DSPR_OUTPUT), which can include an address token */

            if (D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 3) {
                TRACE("o");
                shader_dump_arr_entry(iface, param, addr_token, reg, input);
            }
            else 
               TRACE("oT%lu", reg);
            break;
        case D3DSPR_CONSTINT:
            TRACE("i");
            shader_dump_arr_entry(iface, param, addr_token, reg, input);
            break;
        case D3DSPR_CONSTBOOL:
            TRACE("b");
            shader_dump_arr_entry(iface, param, addr_token, reg, input);
            break;
        case D3DSPR_LABEL:
            TRACE("l%lu", reg);
            break;
        case D3DSPR_LOOP:
            TRACE("aL");
            break;
        case D3DSPR_SAMPLER:
            TRACE("s%lu", reg);
            break;
        case D3DSPR_PREDICATE:
            TRACE("p%lu", reg);
            break;
        default:
            TRACE("unhandled_rtype(%#lx)", regtype);
            break;
   }

   if (!input) {
       /* operand output (for modifiers and shift, see dump_ins_modifiers) */

       if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
           TRACE(".");
           if (param & D3DSP_WRITEMASK_0) TRACE("%c", swizzle_reg_chars[0]);
           if (param & D3DSP_WRITEMASK_1) TRACE("%c", swizzle_reg_chars[1]);
           if (param & D3DSP_WRITEMASK_2) TRACE("%c", swizzle_reg_chars[2]);
           if (param & D3DSP_WRITEMASK_3) TRACE("%c", swizzle_reg_chars[3]);
       }

   } else {
        /** operand input */
        DWORD swizzle = (param & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
        DWORD swizzle_r = swizzle & 0x03;
        DWORD swizzle_g = (swizzle >> 2) & 0x03;
        DWORD swizzle_b = (swizzle >> 4) & 0x03;
        DWORD swizzle_a = (swizzle >> 6) & 0x03;

        if (0 != (param & D3DSP_SRCMOD_MASK)) {
            DWORD mask = param & D3DSP_SRCMOD_MASK;
            switch (mask) {
                case D3DSPSM_NONE:    break;
                case D3DSPSM_NEG:     break;
                case D3DSPSM_NOT:     break;
                case D3DSPSM_BIAS:    TRACE("_bias"); break;
                case D3DSPSM_BIASNEG: TRACE("_bias"); break;
                case D3DSPSM_SIGN:    TRACE("_bx2"); break;
                case D3DSPSM_SIGNNEG: TRACE("_bx2"); break;
                case D3DSPSM_COMP:    break;
                case D3DSPSM_X2:      TRACE("_x2"); break;
                case D3DSPSM_X2NEG:   TRACE("_x2"); break;
                case D3DSPSM_DZ:      TRACE("_dz"); break;
                case D3DSPSM_DW:      TRACE("_dw"); break;
                default:
                    TRACE("_unknown_modifier(%#lx)", mask >> D3DSP_SRCMOD_SHIFT);
            }
        }

        /**
        * swizzle bits fields:
        *  RRGGBBAA
        */
        if ((D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) != swizzle) { /* ! D3DVS_NOSWIZZLE == 0xE4 << D3DVS_SWIZZLE_SHIFT */
            if (swizzle_r == swizzle_g &&
                swizzle_r == swizzle_b &&
                swizzle_r == swizzle_a) {
                    TRACE(".%c", swizzle_reg_chars[swizzle_r]);
            } else {
                TRACE(".%c%c%c%c",
                swizzle_reg_chars[swizzle_r],
                swizzle_reg_chars[swizzle_g],
                swizzle_reg_chars[swizzle_b],
                swizzle_reg_chars[swizzle_a]);
            }
        }
    }
}

/** Shared code in order to generate the bulk of the shader string.
    Use the shader_header_fct & shader_footer_fct to add strings
    that are specific to pixel or vertex functions
    NOTE: A description of how to parse tokens can be found at:
          http://msdn.microsoft.com/library/default.asp?url=/library/en-us/graphics/hh/graphics/usermodedisplaydriver_shader_cc8e4e05-f5c3-4ec0-8853-8ce07c1551b2.xml.asp */
void shader_generate_main(
    IWineD3DBaseShader *iface,
    SHADER_BUFFER* buffer,
    shader_reg_maps* reg_maps,
    CONST DWORD* pFunction) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    const DWORD *pToken = pFunction;
    const SHADER_OPCODE *curOpcode = NULL;
    SHADER_HANDLER hw_fct = NULL;
    DWORD opcode_token;
    DWORD i;
    SHADER_OPCODE_ARG hw_arg;

    /* Initialize current parsing state */
    hw_arg.shader = iface;
    hw_arg.buffer = buffer;
    hw_arg.reg_maps = reg_maps;
    This->baseShader.parse_state.current_row = 0;

    /* Second pass, process opcodes */
    if (NULL != pToken) {
        while (D3DPS_END() != *pToken) {

            /* Skip version token */
            if (shader_is_version_token(*pToken)) {
                ++pToken;
                continue;
            }

            /* Skip comment tokens */
            if (shader_is_comment(*pToken)) {
                DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                ++pToken;
                TRACE("#%s\n", (char*)pToken);
                pToken += comment_len;
                continue;
            }

            /* Read opcode */
            opcode_token = *pToken++;
            curOpcode = shader_get_opcode(iface, opcode_token);
            hw_fct = (curOpcode == NULL)? NULL:
                     (wined3d_settings.shader_mode == SHADER_GLSL)?
                         curOpcode->hw_glsl_fct : curOpcode->hw_fct;

            /* Unknown opcode and its parameters */
            if (NULL == curOpcode) {
                FIXME("Unrecognized opcode: token=%08lX\n", opcode_token);
                pToken += shader_skip_unrecognized(iface, pToken); 

            /* Nothing to do */
            } else if (D3DSIO_DCL == curOpcode->opcode ||
                       D3DSIO_NOP == curOpcode->opcode) {

                pToken += shader_skip_opcode(This, curOpcode, opcode_token);

            /* If a generator function is set for current shader target, use it */
            } else if (hw_fct != NULL) {

                hw_arg.opcode = curOpcode;

                /* Destination token */
                if (curOpcode->dst_token) {

                    DWORD param, addr_token = 0;
                    pToken += shader_get_param(iface, pToken, &param, &addr_token);
                    hw_arg.dst = param;
                    hw_arg.dst_addr = addr_token;
                }

                /* Predication token */
                if (opcode_token & D3DSHADER_INSTRUCTION_PREDICATED) 
                    hw_arg.predicate = *pToken++;

                /* Other source tokens */
                for (i = curOpcode->dst_token; i < curOpcode->num_params; i++) {

                    DWORD param, addr_token = 0; 

                    /* DEF* instructions have constant src parameters, not registers */
                    if (curOpcode->opcode == D3DSIO_DEF || 
                        curOpcode->opcode == D3DSIO_DEFI || 
                        curOpcode->opcode == D3DSIO_DEFB) {
                        param = *pToken++;

                    } else
                        pToken += shader_get_param(iface, pToken, &param, &addr_token);

                    hw_arg.src[i-1] = param;
                    hw_arg.src_addr[i-1] = addr_token;
                }

                /* Call appropriate function for output target */
                hw_fct(&hw_arg);

                /* Process instruction modifiers for GLSL apps ( _sat, etc. ) */
                if (wined3d_settings.shader_mode == SHADER_GLSL)
                    shader_glsl_add_instruction_modifiers(&hw_arg);

            /* Unhandled opcode */
            } else {

                FIXME("Can't handle opcode %s in hwShader\n", curOpcode->name);
                pToken += shader_skip_opcode(This, curOpcode, opcode_token);
            }
        }
        /* TODO: What about result.depth? */

    }
}

void shader_dump_ins_modifiers(const DWORD output) {

    DWORD shift = (output & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
    DWORD mmask = output & D3DSP_DSTMOD_MASK;

    switch (shift) {
        case 0: break;
        case 13: TRACE("_d8"); break;
        case 14: TRACE("_d4"); break;
        case 15: TRACE("_d2"); break;
        case 1: TRACE("_x2"); break;
        case 2: TRACE("_x4"); break;
        case 3: TRACE("_x8"); break;
        default: TRACE("_unhandled_shift(%ld)", shift); break;
    }

    if (mmask & D3DSPDM_SATURATE)         TRACE("_sat");
    if (mmask & D3DSPDM_PARTIALPRECISION) TRACE("_pp");
    if (mmask & D3DSPDM_MSAMPCENTROID)    TRACE("_centroid");

    mmask &= ~(D3DSPDM_SATURATE | D3DSPDM_PARTIALPRECISION | D3DSPDM_MSAMPCENTROID);
    if (mmask)
        FIXME("_unrecognized_modifier(%#lx)", mmask >> D3DSP_DSTMOD_SHIFT);
}

/* First pass: trace shader, initialize length and version */
void shader_trace_init(
    IWineD3DBaseShader *iface,
    const DWORD* pFunction) {

    IWineD3DBaseShaderImpl *This =(IWineD3DBaseShaderImpl *)iface;

    const DWORD* pToken = pFunction;
    const SHADER_OPCODE* curOpcode = NULL;
    DWORD opcode_token;
    unsigned int len = 0;
    DWORD i;

    TRACE("(%p) : Parsing programme\n", This);

    if (NULL != pToken) {
        while (D3DVS_END() != *pToken) {
            if (shader_is_version_token(*pToken)) { /** version */
                This->baseShader.hex_version = *pToken;
                TRACE("%s_%lu_%lu\n", shader_is_pshader_version(This->baseShader.hex_version)? "ps": "vs",
                    D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version),
                    D3DSHADER_VERSION_MINOR(This->baseShader.hex_version));
                ++pToken;
                ++len;
                continue;
            }
            if (shader_is_comment(*pToken)) { /** comment */
                DWORD comment_len = (*pToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                ++pToken;
                TRACE("//%s\n", (char*)pToken);
                pToken += comment_len;
                len += comment_len + 1;
                continue;
            }
            opcode_token = *pToken++;
            curOpcode = shader_get_opcode(iface, opcode_token);
            len++;

            if (NULL == curOpcode) {
                int tokens_read;
                FIXME("Unrecognized opcode: token=%08lX\n", opcode_token);
                tokens_read = shader_skip_unrecognized(iface, pToken);
                pToken += tokens_read;
                len += tokens_read;

            } else {
                if (curOpcode->opcode == D3DSIO_DCL) {

                    DWORD usage = *pToken;
                    DWORD param = *(pToken + 1);

                    shader_dump_decl_usage(This, usage, param);
                    shader_dump_ins_modifiers(param);
                    TRACE(" ");
                    shader_dump_param(iface, param, 0, 0);
                    pToken += 2;
                    len += 2;

                } else if (curOpcode->opcode == D3DSIO_DEF) {

                        unsigned int offset = shader_get_float_offset(*pToken);

                        TRACE("def c%u = %f, %f, %f, %f", offset,
                            *(float *)(pToken + 1),
                            *(float *)(pToken + 2),
                            *(float *)(pToken + 3),
                            *(float *)(pToken + 4));

                        pToken += 5;
                        len += 5;
                } else if (curOpcode->opcode == D3DSIO_DEFI) {

                        TRACE("defi i%lu = %ld, %ld, %ld, %ld", *pToken & D3DSP_REGNUM_MASK,
                            (long) *(pToken + 1),
                            (long) *(pToken + 2),
                            (long) *(pToken + 3),
                            (long) *(pToken + 4));

                        pToken += 5;
                        len += 5;

                } else if (curOpcode->opcode == D3DSIO_DEFB) {

                        TRACE("defb b%lu = %s", *pToken & D3DSP_REGNUM_MASK,
                            *(pToken + 1)? "true": "false");

                        pToken += 2;
                        len += 2;

                } else {

                    DWORD param, addr_token;
                    int tokens_read;

                    /* Print out predication source token first - it follows
                     * the destination token. */
                    if (opcode_token & D3DSHADER_INSTRUCTION_PREDICATED) {
                        TRACE("(");
                        shader_dump_param(iface, *(pToken + 2), 0, 1);
                        TRACE(") ");
                    }

                    TRACE("%s", curOpcode->name);

                    /* Destination token */
                    if (curOpcode->dst_token) {

                        /* Destination token */
                        tokens_read = shader_get_param(iface, pToken, &param, &addr_token);
                        pToken += tokens_read;
                        len += tokens_read;

                        shader_dump_ins_modifiers(param);
                        TRACE(" ");
                        shader_dump_param(iface, param, addr_token, 0);
                    }

                    /* Predication token - already printed out, just skip it */
                    if (opcode_token & D3DSHADER_INSTRUCTION_PREDICATED) {
                        pToken++;
                        len++;
                    }

                    /* Other source tokens */
                    for (i = curOpcode->dst_token; i < curOpcode->num_params; ++i) {

                        tokens_read = shader_get_param(iface, pToken, &param, &addr_token);
                        pToken += tokens_read;
                        len += tokens_read;

                        TRACE((i == 0)? " " : ", ");
                        shader_dump_param(iface, param, addr_token, 1);
                    }
                }
                TRACE("\n");
            }
        }
        This->baseShader.functionLength = (len + 1) * sizeof(DWORD);
    } else {
        This->baseShader.functionLength = 1; /* no Function defined use fixed function vertex processing */
    }
}

/* TODO: Move other shared code here */
