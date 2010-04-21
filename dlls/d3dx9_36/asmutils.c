/*
 * Direct3D shader library utility routines
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
#include "wine/debug.h"

#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(asmshader);

/* bwriter -> d3d9 conversion functions */
DWORD d3d9_swizzle(DWORD bwriter_swizzle) {
    /* Currently a NOP, but this allows changing the internal definitions
     * without side effects
     */
    DWORD ret = 0;

    if((bwriter_swizzle & BWRITERVS_X_X) == BWRITERVS_X_X) ret |= D3DVS_X_X;
    if((bwriter_swizzle & BWRITERVS_X_Y) == BWRITERVS_X_Y) ret |= D3DVS_X_Y;
    if((bwriter_swizzle & BWRITERVS_X_Z) == BWRITERVS_X_Z) ret |= D3DVS_X_Z;
    if((bwriter_swizzle & BWRITERVS_X_W) == BWRITERVS_X_W) ret |= D3DVS_X_W;

    if((bwriter_swizzle & BWRITERVS_Y_X) == BWRITERVS_Y_X) ret |= D3DVS_Y_X;
    if((bwriter_swizzle & BWRITERVS_Y_Y) == BWRITERVS_Y_Y) ret |= D3DVS_Y_Y;
    if((bwriter_swizzle & BWRITERVS_Y_Z) == BWRITERVS_Y_Z) ret |= D3DVS_Y_Z;
    if((bwriter_swizzle & BWRITERVS_Y_W) == BWRITERVS_Y_W) ret |= D3DVS_Y_W;

    if((bwriter_swizzle & BWRITERVS_Z_X) == BWRITERVS_Z_X) ret |= D3DVS_Z_X;
    if((bwriter_swizzle & BWRITERVS_Z_Y) == BWRITERVS_Z_Y) ret |= D3DVS_Z_Y;
    if((bwriter_swizzle & BWRITERVS_Z_Z) == BWRITERVS_Z_Z) ret |= D3DVS_Z_Z;
    if((bwriter_swizzle & BWRITERVS_Z_W) == BWRITERVS_Z_W) ret |= D3DVS_Z_W;

    if((bwriter_swizzle & BWRITERVS_W_X) == BWRITERVS_W_X) ret |= D3DVS_W_X;
    if((bwriter_swizzle & BWRITERVS_W_Y) == BWRITERVS_W_Y) ret |= D3DVS_W_Y;
    if((bwriter_swizzle & BWRITERVS_W_Z) == BWRITERVS_W_Z) ret |= D3DVS_W_Z;
    if((bwriter_swizzle & BWRITERVS_W_W) == BWRITERVS_W_W) ret |= D3DVS_W_W;

    return ret;
}

DWORD d3d9_writemask(DWORD bwriter_writemask) {
    DWORD ret = 0;

    if(bwriter_writemask & BWRITERSP_WRITEMASK_0) ret |= D3DSP_WRITEMASK_0;
    if(bwriter_writemask & BWRITERSP_WRITEMASK_1) ret |= D3DSP_WRITEMASK_1;
    if(bwriter_writemask & BWRITERSP_WRITEMASK_2) ret |= D3DSP_WRITEMASK_2;
    if(bwriter_writemask & BWRITERSP_WRITEMASK_3) ret |= D3DSP_WRITEMASK_3;

    return ret;
}

DWORD d3d9_register(DWORD bwriter_register) {
    if(bwriter_register == BWRITERSPR_TEMP)         return D3DSPR_TEMP;
    if(bwriter_register == BWRITERSPR_CONST)        return D3DSPR_CONST;

    FIXME("Unexpected BWRITERSPR %u\n", bwriter_register);
    return -1;
}

DWORD d3d9_opcode(DWORD bwriter_opcode) {
    switch(bwriter_opcode) {
        case BWRITERSIO_MOV:         return D3DSIO_MOV;

        case BWRITERSIO_COMMENT:     return D3DSIO_COMMENT;
        case BWRITERSIO_END:         return D3DSIO_END;

        default:
            FIXME("Unhandled BWRITERSIO token %u\n", bwriter_opcode);
            return -1;
    }
}

static const char *get_regname(const struct shader_reg *reg, shader_type st) {
    switch(reg->type) {
        case BWRITERSPR_TEMP:
            return wine_dbg_sprintf("r%u", reg->regnum);
        case BWRITERSPR_CONST:
            return wine_dbg_sprintf("c%u", reg->regnum);
        default: return "unknown regname";
    }
}

const char *debug_print_writemask(DWORD mask) {
    char ret[6];
    unsigned char pos = 1;

    if(mask == BWRITERSP_WRITEMASK_ALL) return "";
    ret[0] = '.';
    if(mask & BWRITERSP_WRITEMASK_0) ret[pos++] = 'x';
    if(mask & BWRITERSP_WRITEMASK_1) ret[pos++] = 'y';
    if(mask & BWRITERSP_WRITEMASK_2) ret[pos++] = 'z';
    if(mask & BWRITERSP_WRITEMASK_3) ret[pos++] = 'w';
    ret[pos] = 0;
    return wine_dbg_sprintf("%s", ret);
}

const char *debug_print_dstreg(const struct shader_reg *reg, shader_type st) {
    return wine_dbg_sprintf("%s%s", get_regname(reg, st),
                            debug_print_writemask(reg->writemask));
}

const char *debug_print_swizzle(DWORD arg) {
    char ret[6];
    unsigned int i;
    DWORD swizzle[4];

    switch(arg) {
        case BWRITERVS_NOSWIZZLE:
            return "";
        case BWRITERVS_SWIZZLE_X:
            return ".x";
        case BWRITERVS_SWIZZLE_Y:
            return ".y";
        case BWRITERVS_SWIZZLE_Z:
            return ".z";
        case BWRITERVS_SWIZZLE_W:
            return ".w";
    }

    swizzle[0] = (arg >> (BWRITERVS_SWIZZLE_SHIFT + 0)) & 0x03;
    swizzle[1] = (arg >> (BWRITERVS_SWIZZLE_SHIFT + 2)) & 0x03;
    swizzle[2] = (arg >> (BWRITERVS_SWIZZLE_SHIFT + 4)) & 0x03;
    swizzle[3] = (arg >> (BWRITERVS_SWIZZLE_SHIFT + 6)) & 0x03;

    ret[0] = '.';
    for(i = 0; i < 4; i++) {
        switch(swizzle[i]) {
            case 0: ret[1 + i] = 'x'; break;
            case 1: ret[1 + i] = 'y'; break;
            case 2: ret[1 + i] = 'z'; break;
            case 3: ret[1 + i] = 'w'; break;
        }
    }
    ret[5] = '\0';
    return wine_dbg_sprintf("%s", ret);
}

const char *debug_print_srcreg(const struct shader_reg *reg, shader_type st) {
    switch(reg->srcmod) {
        case BWRITERSPSM_NONE:
            return wine_dbg_sprintf("%s%s", get_regname(reg, st),
                                    debug_print_swizzle(reg->swizzle));
    }
    return "Unknown modifier";
}

const char *debug_print_opcode(DWORD opcode) {
    switch(opcode){
        case BWRITERSIO_MOV:          return "mov";

        default:                      return "unknown";
    }
}
