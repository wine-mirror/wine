/*
 * shaders implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
    DWORD version = This->baseShader.version;
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
    FIXME("Unsupported opcode %lx(%ld) masked %lx version %ld\n", 
       code, code, code & D3DSI_OPCODE_MASK, version);
    return NULL;
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

/* Note: For vertex shaders,
 * texUsed = addrUsed, and 
 * D3DSPR_TEXTURE = D3DSPR_ADDR. 
 *
 * Also note that this does not count the loop register
 * as an address register. */   

void shader_get_registers_used(
    IWineD3DBaseShader *iface,
    CONST DWORD* pToken) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    DWORD* tempsUsed = &This->baseShader.temps_used;
    DWORD* texUsed = &This->baseShader.textures_used;

    if (pToken == NULL)
        return;

    *tempsUsed = 0;
    *texUsed = 0;

    while (D3DVS_END() != *pToken) {
        CONST SHADER_OPCODE* curOpcode;

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
        curOpcode = shader_get_opcode(iface, *pToken);
        ++pToken;

        /* Unhandled opcode, and its parameters */
        if (NULL == curOpcode) {
           while (*pToken & 0x80000000)
               ++pToken;
           continue;

        /* Skip declarations (for now) */
        } else if (D3DSIO_DCL == curOpcode->opcode) {
            pToken += curOpcode->num_params;
            continue;

        /* Skip definitions (for now) */
        } else if (D3DSIO_DEF == curOpcode->opcode) {
            pToken += curOpcode->num_params;
            continue;

        /* Set texture registers, and temporary registers */
        } else {
            int i;

            for (i = 0; i < curOpcode->num_params; ++i) {
                DWORD regtype = (((*pToken) & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT);
                DWORD reg = (*pToken) & D3DSP_REGNUM_MASK;
                if (D3DSPR_TEXTURE == regtype)
                    *texUsed |= (1 << reg);
                if (D3DSPR_TEMP == regtype)
                    *tempsUsed |= (1 << reg);
                ++pToken;
             }
        }
    }
}

void shader_program_dump_decl_usage(
    DWORD decl, 
    DWORD param) {

    DWORD regtype = shader_get_regtype(param);
    TRACE("dcl_");

    if (regtype == D3DSPR_SAMPLER) {
        DWORD ttype = decl & D3DSP_TEXTURETYPE_MASK;

        switch (ttype) {
            case D3DSTT_2D: TRACE("2d"); break;
            case D3DSTT_CUBE: TRACE("cube"); break;
            case D3DSTT_VOLUME: TRACE("volume"); break;
            default: TRACE("unknown_ttype(%08lx)", ttype); 
       }

    } else { 

        DWORD usage = decl & D3DSP_DCL_USAGE_MASK;
        DWORD idx = (decl & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;

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

void shader_dump_param(
    IWineD3DBaseShader *iface,
    const DWORD param, 
    int input) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    static const char* rastout_reg_names[] = { "oPos", "oFog", "oPts" };
    char swizzle_reg_chars[4];

    DWORD reg = param & D3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);

    /* There are some minor differences between pixel and vertex shaders */
    BOOL pshader = shader_is_pshader_version(This->baseShader.hex_version);

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
    }

    switch (regtype) {
        case D3DSPR_TEMP:
            TRACE("r%lu", reg);
            break;
        case D3DSPR_INPUT:
            TRACE("v%lu", reg);
            break;
        case D3DSPR_CONST:
            TRACE("c%s%lu", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
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
            TRACE("oT%lu", reg);
            break;
        case D3DSPR_CONSTINT:
            TRACE("i%s%lu", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
            break;
        case D3DSPR_CONSTBOOL:
            TRACE("b%s%lu", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
            break;
        case D3DSPR_LABEL:
            TRACE("l%lu", reg);
            break;
        case D3DSPR_LOOP:
            TRACE("aL%s%lu", (param & D3DVS_ADDRMODE_RELATIVE) ? "a0.x + " : "", reg);
            break;
        case D3DSPR_SAMPLER:
            TRACE("s%lu", reg);
            break;
        default:
            TRACE("unhandled_rtype(%lx)", regtype);
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
            /*TRACE("_modifier(0x%08lx) ", mask);*/
            switch (mask) {
                case D3DSPSM_NONE:    break;
                case D3DSPSM_NEG:     break;
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
                    TRACE("_unknown(0x%08lx)", mask);
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

/** Generate the variable & register declarations for the ARB_vertex_program
    output target */
void generate_arb_declarations(IWineD3DBaseShader *iface, SHADER_BUFFER* buffer) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    DWORD i;

    for(i = 0; i < This->baseShader.limits.temporary; i++) {
        if (This->baseShader.temps_used & (1 << i))
            shader_addline(buffer, "TEMP R%lu;\n", i);
    }

    for (i = 0; i < This->baseShader.limits.address; i++) {
        if (This->baseShader.textures_used & (1 << i))
            shader_addline(buffer, "ADDRESS A%ld;\n", i);
    }

    for(i = 0; i < This->baseShader.limits.texture; i++) {
        if (This->baseShader.textures_used & (1 << i))
            shader_addline(buffer,"TEMP T%lu;\n", i);
    }

    /* Texture coordinate registers must be pre-loaded */
    for (i = 0; i < This->baseShader.limits.texture; i++) {
    if (This->baseShader.textures_used & (1 << i))
        shader_addline(buffer, "MOV T%lu, fragment.texcoord[%lu];\n", i, i);
    }
}

/** Generate the variable & register declarations for the GLSL
    output target */
void generate_glsl_declarations(IWineD3DBaseShader *iface, SHADER_BUFFER* buffer) {

    FIXME("GLSL not fully implemented yet.\n");

}

/** Shared code in order to generate the bulk of the shader string.
    Use the shader_header_fct & shader_footer_fct to add strings
    that are specific to pixel or vertex functions
    NOTE: A description of how to parse tokens can be found at:
          http://msdn.microsoft.com/library/default.asp?url=/library/en-us/graphics/hh/graphics/usermodedisplaydriver_shader_cc8e4e05-f5c3-4ec0-8853-8ce07c1551b2.xml.asp */
void generate_base_shader(
    IWineD3DBaseShader *iface,
    SHADER_BUFFER* buffer,
    CONST DWORD* pFunction) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    const DWORD *pToken = pFunction;
    const SHADER_OPCODE *curOpcode = NULL;
    DWORD opcode_token;
    DWORD i;

    /* Initialize current parsing state */
    This->baseShader.parse_state.current_row = 0;

    /* First pass: figure out which temporary and texture registers are used */
    shader_get_registers_used(iface, pToken);
    TRACE("Texture/Address registers used: %#lx, Temp registers used %#lx\n",
          This->baseShader.textures_used, This->baseShader.temps_used);

    /* TODO: check register usage against GL/Directx limits, and fail if they're exceeded
        nUseAddressRegister < = GL_MAX_PROGRAM_ADDRESS_REGISTERS_AR
        nUseTempRegister    <=  GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB
    */

    /* Pre-declare registers */
    if (USING_GLSL)
        generate_glsl_declarations(iface, buffer);
    else
        generate_arb_declarations(iface, buffer);

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

            /* Unknown opcode and its parameters */
            if (NULL == curOpcode) {
                while (*pToken & 0x80000000) { /* TODO: Think of a sensible name for 0x80000000 */
                    FIXME("unrecognized opcode: %08lx\n", *pToken);
                    ++pToken;
                }

            /* Using GLSL & no generator function exists */
            } else if (USING_GLSL && curOpcode->hw_glsl_fct == NULL) {

                FIXME("Token %s is not yet implemented with GLSL\n", curOpcode->name);
                pToken += shader_skip_opcode(This, curOpcode, opcode_token);

            /* Unhandled opcode in ARB */
            } else if ( !USING_GLSL && GLNAME_REQUIRE_GLSL == curOpcode->glname) {

                FIXME("Token %s requires greater functionality than "
                    "Vertex or Fragment_Program_ARB supports\n", curOpcode->name);
                pToken += shader_skip_opcode(This, curOpcode, opcode_token);

            /* If a generator function is set for current shader target, use it */
            } else if ((!USING_GLSL && curOpcode->hw_fct != NULL) ||
                       (USING_GLSL && curOpcode->hw_glsl_fct != NULL)) {

                SHADER_OPCODE_ARG hw_arg;

                hw_arg.shader = iface;
                hw_arg.opcode = curOpcode;
                hw_arg.buffer = buffer;
                if (curOpcode->num_params > 0) {
                    hw_arg.dst = *pToken;

                    /* FIXME: this does not account for relative address tokens */
                    for (i = 1; i < curOpcode->num_params; i++)
                       hw_arg.src[i-1] = *(pToken + i);
                }

                /* Call appropriate function for output target */
                if (USING_GLSL)
                    curOpcode->hw_glsl_fct(&hw_arg);
                else
                    curOpcode->hw_fct(&hw_arg);

                pToken += curOpcode->num_params;

            } else {

                /* Unless we encounter a no-op command, this opcode is unrecognized */
                if (curOpcode->opcode != D3DSIO_NOP) {
                    FIXME("Can't handle opcode %s in hwShader\n", curOpcode->name);
                    pToken += shader_skip_opcode(This, curOpcode, opcode_token);
                }
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

/* TODO: Move other shared code here */
