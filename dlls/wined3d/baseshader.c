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

inline static BOOL shader_is_version_token(DWORD token) {
    return 0xFFFF0000 == (token & 0xFFFF0000) || 
           0xFFFE0000 == (token & 0xFFFF0000);
}

inline static BOOL shader_is_comment_token(DWORD token) {
    return D3DSIO_COMMENT == (token & D3DSI_OPCODE_MASK);
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

/* Note: For vertex shaders,
 * texUsed = addrUsed, and 
 * D3DSPR_TEXTURE = D3DSPR_ADDR. 
 *
 * Also note that this does not count the loop register
 * as an address register. */   

void shader_get_registers_used(
    IWineD3DBaseShader *iface,
    CONST DWORD* pToken, 
    DWORD* tempsUsed, 
    DWORD* texUsed) {

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
        } else if (shader_is_comment_token(*pToken)) {
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

/* TODO: Move other shared code here */
