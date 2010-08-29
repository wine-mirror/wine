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

#include "d3d9types.h"
#include "d3dcompiler_private.h"

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

DWORD d3d9_srcmod(DWORD bwriter_srcmod) {
    switch(bwriter_srcmod) {
        case BWRITERSPSM_NONE:       return D3DSPSM_NONE;
        case BWRITERSPSM_NEG:        return D3DSPSM_NEG;
        case BWRITERSPSM_BIAS:       return D3DSPSM_BIAS;
        case BWRITERSPSM_BIASNEG:    return D3DSPSM_BIASNEG;
        case BWRITERSPSM_SIGN:       return D3DSPSM_SIGN;
        case BWRITERSPSM_SIGNNEG:    return D3DSPSM_SIGNNEG;
        case BWRITERSPSM_COMP:       return D3DSPSM_COMP;
        case BWRITERSPSM_X2:         return D3DSPSM_X2;
        case BWRITERSPSM_X2NEG:      return D3DSPSM_X2NEG;
        case BWRITERSPSM_DZ:         return D3DSPSM_DZ;
        case BWRITERSPSM_DW:         return D3DSPSM_DW;
        case BWRITERSPSM_ABS:        return D3DSPSM_ABS;
        case BWRITERSPSM_ABSNEG:     return D3DSPSM_ABSNEG;
        case BWRITERSPSM_NOT:        return D3DSPSM_NOT;
        default:
            FIXME("Unhandled BWRITERSPSM token %u\n", bwriter_srcmod);
            return 0;
    }
}

DWORD d3d9_dstmod(DWORD bwriter_mod) {
    DWORD ret = 0;

    if(bwriter_mod & BWRITERSPDM_SATURATE)          ret |= D3DSPDM_SATURATE;
    if(bwriter_mod & BWRITERSPDM_PARTIALPRECISION)  ret |= D3DSPDM_PARTIALPRECISION;
    if(bwriter_mod & BWRITERSPDM_MSAMPCENTROID)     ret |= D3DSPDM_MSAMPCENTROID;

    return ret;
}

DWORD d3d9_comparetype(DWORD asmshader_comparetype) {
    switch(asmshader_comparetype) {
        case BWRITER_COMPARISON_GT:     return D3DSPC_GT;
        case BWRITER_COMPARISON_EQ:     return D3DSPC_EQ;
        case BWRITER_COMPARISON_GE:     return D3DSPC_GE;
        case BWRITER_COMPARISON_LT:     return D3DSPC_LT;
        case BWRITER_COMPARISON_NE:     return D3DSPC_NE;
        case BWRITER_COMPARISON_LE:     return D3DSPC_LE;
        default:
            FIXME("Unexpected BWRITER_COMPARISON type %u\n", asmshader_comparetype);
            return 0;
    }
}

DWORD d3d9_sampler(DWORD bwriter_sampler) {
    if(bwriter_sampler == BWRITERSTT_UNKNOWN)   return D3DSTT_UNKNOWN;
    if(bwriter_sampler == BWRITERSTT_1D)        return D3DSTT_1D;
    if(bwriter_sampler == BWRITERSTT_2D)        return D3DSTT_2D;
    if(bwriter_sampler == BWRITERSTT_CUBE)      return D3DSTT_CUBE;
    if(bwriter_sampler == BWRITERSTT_VOLUME)    return D3DSTT_VOLUME;
    FIXME("Unexpected BWRITERSAMPLER_TEXTURE_TYPE type %u\n", bwriter_sampler);

    return 0;
}

DWORD d3d9_register(DWORD bwriter_register) {
    if(bwriter_register == BWRITERSPR_TEMP)         return D3DSPR_TEMP;
    if(bwriter_register == BWRITERSPR_INPUT)        return D3DSPR_INPUT;
    if(bwriter_register == BWRITERSPR_CONST)        return D3DSPR_CONST;
    if(bwriter_register == BWRITERSPR_ADDR)         return D3DSPR_ADDR;
    if(bwriter_register == BWRITERSPR_TEXTURE)      return D3DSPR_TEXTURE;
    if(bwriter_register == BWRITERSPR_RASTOUT)      return D3DSPR_RASTOUT;
    if(bwriter_register == BWRITERSPR_ATTROUT)      return D3DSPR_ATTROUT;
    if(bwriter_register == BWRITERSPR_TEXCRDOUT)    return D3DSPR_TEXCRDOUT;
    if(bwriter_register == BWRITERSPR_OUTPUT)       return D3DSPR_OUTPUT;
    if(bwriter_register == BWRITERSPR_CONSTINT)     return D3DSPR_CONSTINT;
    if(bwriter_register == BWRITERSPR_COLOROUT)     return D3DSPR_COLOROUT;
    if(bwriter_register == BWRITERSPR_DEPTHOUT)     return D3DSPR_DEPTHOUT;
    if(bwriter_register == BWRITERSPR_SAMPLER)      return D3DSPR_SAMPLER;
    if(bwriter_register == BWRITERSPR_CONSTBOOL)    return D3DSPR_CONSTBOOL;
    if(bwriter_register == BWRITERSPR_LOOP)         return D3DSPR_LOOP;
    if(bwriter_register == BWRITERSPR_MISCTYPE)     return D3DSPR_MISCTYPE;
    if(bwriter_register == BWRITERSPR_LABEL)        return D3DSPR_LABEL;
    if(bwriter_register == BWRITERSPR_PREDICATE)    return D3DSPR_PREDICATE;

    FIXME("Unexpected BWRITERSPR %u\n", bwriter_register);
    return -1;
}

DWORD d3d9_opcode(DWORD bwriter_opcode) {
    switch(bwriter_opcode) {
        case BWRITERSIO_NOP:         return D3DSIO_NOP;
        case BWRITERSIO_MOV:         return D3DSIO_MOV;
        case BWRITERSIO_ADD:         return D3DSIO_ADD;
        case BWRITERSIO_SUB:         return D3DSIO_SUB;
        case BWRITERSIO_MAD:         return D3DSIO_MAD;
        case BWRITERSIO_MUL:         return D3DSIO_MUL;
        case BWRITERSIO_RCP:         return D3DSIO_RCP;
        case BWRITERSIO_RSQ:         return D3DSIO_RSQ;
        case BWRITERSIO_DP3:         return D3DSIO_DP3;
        case BWRITERSIO_DP4:         return D3DSIO_DP4;
        case BWRITERSIO_MIN:         return D3DSIO_MIN;
        case BWRITERSIO_MAX:         return D3DSIO_MAX;
        case BWRITERSIO_SLT:         return D3DSIO_SLT;
        case BWRITERSIO_SGE:         return D3DSIO_SGE;
        case BWRITERSIO_EXP:         return D3DSIO_EXP;
        case BWRITERSIO_LOG:         return D3DSIO_LOG;
        case BWRITERSIO_LIT:         return D3DSIO_LIT;
        case BWRITERSIO_DST:         return D3DSIO_DST;
        case BWRITERSIO_LRP:         return D3DSIO_LRP;
        case BWRITERSIO_FRC:         return D3DSIO_FRC;
        case BWRITERSIO_M4x4:        return D3DSIO_M4x4;
        case BWRITERSIO_M4x3:        return D3DSIO_M4x3;
        case BWRITERSIO_M3x4:        return D3DSIO_M3x4;
        case BWRITERSIO_M3x3:        return D3DSIO_M3x3;
        case BWRITERSIO_M3x2:        return D3DSIO_M3x2;
        case BWRITERSIO_CALL:        return D3DSIO_CALL;
        case BWRITERSIO_CALLNZ:      return D3DSIO_CALLNZ;
        case BWRITERSIO_LOOP:        return D3DSIO_LOOP;
        case BWRITERSIO_RET:         return D3DSIO_RET;
        case BWRITERSIO_ENDLOOP:     return D3DSIO_ENDLOOP;
        case BWRITERSIO_LABEL:       return D3DSIO_LABEL;
        case BWRITERSIO_DCL:         return D3DSIO_DCL;
        case BWRITERSIO_POW:         return D3DSIO_POW;
        case BWRITERSIO_CRS:         return D3DSIO_CRS;
        case BWRITERSIO_SGN:         return D3DSIO_SGN;
        case BWRITERSIO_ABS:         return D3DSIO_ABS;
        case BWRITERSIO_NRM:         return D3DSIO_NRM;
        case BWRITERSIO_SINCOS:      return D3DSIO_SINCOS;
        case BWRITERSIO_REP:         return D3DSIO_REP;
        case BWRITERSIO_ENDREP:      return D3DSIO_ENDREP;
        case BWRITERSIO_IF:          return D3DSIO_IF;
        case BWRITERSIO_IFC:         return D3DSIO_IFC;
        case BWRITERSIO_ELSE:        return D3DSIO_ELSE;
        case BWRITERSIO_ENDIF:       return D3DSIO_ENDIF;
        case BWRITERSIO_BREAK:       return D3DSIO_BREAK;
        case BWRITERSIO_BREAKC:      return D3DSIO_BREAKC;
        case BWRITERSIO_MOVA:        return D3DSIO_MOVA;
        case BWRITERSIO_DEFB:        return D3DSIO_DEFB;
        case BWRITERSIO_DEFI:        return D3DSIO_DEFI;

        case BWRITERSIO_TEXCOORD:    return D3DSIO_TEXCOORD;
        case BWRITERSIO_TEXKILL:     return D3DSIO_TEXKILL;
        case BWRITERSIO_TEX:         return D3DSIO_TEX;
        case BWRITERSIO_TEXBEM:      return D3DSIO_TEXBEM;
        case BWRITERSIO_TEXBEML:     return D3DSIO_TEXBEML;
        case BWRITERSIO_TEXREG2AR:   return D3DSIO_TEXREG2AR;
        case BWRITERSIO_TEXREG2GB:   return D3DSIO_TEXREG2GB;
        case BWRITERSIO_TEXM3x2PAD:  return D3DSIO_TEXM3x2PAD;
        case BWRITERSIO_TEXM3x2TEX:  return D3DSIO_TEXM3x2TEX;
        case BWRITERSIO_TEXM3x3PAD:  return D3DSIO_TEXM3x3PAD;
        case BWRITERSIO_TEXM3x3TEX:  return D3DSIO_TEXM3x3TEX;
        case BWRITERSIO_TEXM3x3SPEC: return D3DSIO_TEXM3x3SPEC;
        case BWRITERSIO_TEXM3x3VSPEC:return D3DSIO_TEXM3x3VSPEC;
        case BWRITERSIO_EXPP:        return D3DSIO_EXPP;
        case BWRITERSIO_LOGP:        return D3DSIO_LOGP;
        case BWRITERSIO_CND:         return D3DSIO_CND;
        case BWRITERSIO_DEF:         return D3DSIO_DEF;
        case BWRITERSIO_TEXREG2RGB:  return D3DSIO_TEXREG2RGB;
        case BWRITERSIO_TEXDP3TEX:   return D3DSIO_TEXDP3TEX;
        case BWRITERSIO_TEXM3x2DEPTH:return D3DSIO_TEXM3x2DEPTH;
        case BWRITERSIO_TEXDP3:      return D3DSIO_TEXDP3;
        case BWRITERSIO_TEXM3x3:     return D3DSIO_TEXM3x3;
        case BWRITERSIO_TEXDEPTH:    return D3DSIO_TEXDEPTH;
        case BWRITERSIO_CMP:         return D3DSIO_CMP;
        case BWRITERSIO_BEM:         return D3DSIO_BEM;
        case BWRITERSIO_DP2ADD:      return D3DSIO_DP2ADD;
        case BWRITERSIO_DSX:         return D3DSIO_DSX;
        case BWRITERSIO_DSY:         return D3DSIO_DSY;
        case BWRITERSIO_TEXLDD:      return D3DSIO_TEXLDD;
        case BWRITERSIO_SETP:        return D3DSIO_SETP;
        case BWRITERSIO_TEXLDL:      return D3DSIO_TEXLDL;
        case BWRITERSIO_BREAKP:      return D3DSIO_BREAKP;

        case BWRITERSIO_PHASE:       return D3DSIO_PHASE;
        case BWRITERSIO_COMMENT:     return D3DSIO_COMMENT;
        case BWRITERSIO_END:         return D3DSIO_END;

        case BWRITERSIO_TEXLDP:      return D3DSIO_TEX | D3DSI_TEXLD_PROJECT;
        case BWRITERSIO_TEXLDB:      return D3DSIO_TEX | D3DSI_TEXLD_BIAS;

        default:
            FIXME("Unhandled BWRITERSIO token %u\n", bwriter_opcode);
            return -1;
    }
}

/* Debug print functions */
const char *debug_print_srcmod(DWORD mod) {
    switch(mod) {
        case BWRITERSPSM_NEG:       return "D3DSPSM_NEG";
        case BWRITERSPSM_BIAS:      return "D3DSPSM_BIAS";
        case BWRITERSPSM_BIASNEG:   return "D3DSPSM_BIASNEG";
        case BWRITERSPSM_SIGN:      return "D3DSPSM_SIGN";
        case BWRITERSPSM_SIGNNEG:   return "D3DSPSM_SIGNNEG";
        case BWRITERSPSM_COMP:      return "D3DSPSM_COMP";
        case BWRITERSPSM_X2:        return "D3DSPSM_X2";
        case BWRITERSPSM_X2NEG:     return "D3DSPSM_X2NEG";
        case BWRITERSPSM_DZ:        return "D3DSPSM_DZ";
        case BWRITERSPSM_DW:        return "D3DSPSM_DW";
        case BWRITERSPSM_ABS:       return "D3DSPSM_ABS";
        case BWRITERSPSM_ABSNEG:    return "D3DSPSM_ABSNEG";
        case BWRITERSPSM_NOT:       return "D3DSPSM_NOT";
        default:                    return "Unknown source modifier\n";
    }
}

const char *debug_print_dstmod(DWORD mod) {
    switch(mod) {
        case 0:
            return "";

        case BWRITERSPDM_SATURATE:
            return "_sat";
        case BWRITERSPDM_PARTIALPRECISION:
            return "_pp";
        case BWRITERSPDM_MSAMPCENTROID:
            return "_centroid";

        case BWRITERSPDM_SATURATE | BWRITERSPDM_PARTIALPRECISION:
            return "_sat_pp";
        case BWRITERSPDM_SATURATE | BWRITERSPDM_MSAMPCENTROID:
            return "_sat_centroid";
        case BWRITERSPDM_PARTIALPRECISION | BWRITERSPDM_MSAMPCENTROID:
            return "_pp_centroid";

        case BWRITERSPDM_SATURATE | BWRITERSPDM_PARTIALPRECISION | BWRITERSPDM_MSAMPCENTROID:
            return "_sat_pp_centroid";

        default:
            return "Unexpected modifier\n";
    }
}

const char *debug_print_shift(DWORD shift) {
    static const char * const shiftstrings[] = {
        "",
        "_x2",
        "_x4",
        "_x8",
        "_x16",
        "_x32",
        "",
        "",
        "",
        "",
        "",
        "",
        "_d16",
        "_d8",
        "_d4",
        "_d2",
    };
    return shiftstrings[shift];
}

static const char *get_regname(const struct shader_reg *reg) {
    switch(reg->type) {
        case BWRITERSPR_TEMP:
            return wine_dbg_sprintf("r%u", reg->regnum);
        case BWRITERSPR_INPUT:
            return wine_dbg_sprintf("v%u", reg->regnum);
        case BWRITERSPR_CONST:
            return wine_dbg_sprintf("c%u", reg->regnum);
        case BWRITERSPR_ADDR:
            return wine_dbg_sprintf("a%u", reg->regnum);
        case BWRITERSPR_TEXTURE:
            return wine_dbg_sprintf("t%u", reg->regnum);
        case BWRITERSPR_RASTOUT:
            switch(reg->regnum) {
                case BWRITERSRO_POSITION:   return "oPos";
                case BWRITERSRO_FOG:        return "oFog";
                case BWRITERSRO_POINT_SIZE: return "oPts";
                default: return "Unexpected RASTOUT";
            }
        case BWRITERSPR_ATTROUT:
            return wine_dbg_sprintf("oD%u", reg->regnum);
        case BWRITERSPR_TEXCRDOUT:
            return wine_dbg_sprintf("oT%u", reg->regnum);
        case BWRITERSPR_OUTPUT:
            return wine_dbg_sprintf("o%u", reg->regnum);
        case BWRITERSPR_CONSTINT:
            return wine_dbg_sprintf("i%u", reg->regnum);
        case BWRITERSPR_COLOROUT:
            return wine_dbg_sprintf("oC%u", reg->regnum);
        case BWRITERSPR_DEPTHOUT:
            return "oDepth";
        case BWRITERSPR_SAMPLER:
            return wine_dbg_sprintf("s%u", reg->regnum);
        case BWRITERSPR_CONSTBOOL:
            return wine_dbg_sprintf("b%u", reg->regnum);
        case BWRITERSPR_LOOP:
            return "aL";
        case BWRITERSPR_MISCTYPE:
            switch(reg->regnum) {
                case 0: return "vPos";
                case 1: return "vFace";
                case 2: return "unexpected misctype";
            }
        case BWRITERSPR_LABEL:
            return wine_dbg_sprintf("l%u", reg->regnum);
        case BWRITERSPR_PREDICATE:
            return wine_dbg_sprintf("p%u", reg->regnum);
        default:
            return wine_dbg_sprintf("unknown regname %#x", reg->type);
    }
}

static const char *debug_print_writemask(DWORD mask) {
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

static const char *debug_print_swizzle(DWORD arg) {
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

static const char *debug_print_relarg(const struct shader_reg *reg) {
    const char *short_swizzle;
    if(!reg->rel_reg) return "";

    short_swizzle = debug_print_swizzle(reg->rel_reg->u.swizzle);

    if(reg->rel_reg->type == BWRITERSPR_ADDR) {
        return wine_dbg_sprintf("[a%u%s]", reg->rel_reg->regnum, short_swizzle);
    } else if(reg->rel_reg->type == BWRITERSPR_LOOP && reg->rel_reg->regnum == 0) {
        return wine_dbg_sprintf("[aL%s]", short_swizzle);
    } else {
        return "Unexpected relative addressing argument";
    }
}

const char *debug_print_dstreg(const struct shader_reg *reg) {
    return wine_dbg_sprintf("%s%s%s", get_regname(reg),
                            debug_print_relarg(reg),
                            debug_print_writemask(reg->u.writemask));
}

const char *debug_print_srcreg(const struct shader_reg *reg) {
    switch(reg->srcmod) {
        case BWRITERSPSM_NONE:
            return wine_dbg_sprintf("%s%s%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_NEG:
            return wine_dbg_sprintf("-%s%s%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_BIAS:
            return wine_dbg_sprintf("%s%s_bias%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_BIASNEG:
            return wine_dbg_sprintf("-%s%s_bias%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_SIGN:
            return wine_dbg_sprintf("%s%s_bx2%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_SIGNNEG:
            return wine_dbg_sprintf("-%s%s_bx2%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_COMP:
            return wine_dbg_sprintf("1 - %s%s%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_X2:
            return wine_dbg_sprintf("%s%s_x2%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_X2NEG:
            return wine_dbg_sprintf("-%s%s_x2%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_DZ:
            return wine_dbg_sprintf("%s%s_dz%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_DW:
            return wine_dbg_sprintf("%s%s_dw%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_ABS:
            return wine_dbg_sprintf("%s%s_abs%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_ABSNEG:
            return wine_dbg_sprintf("-%s%s_abs%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
        case BWRITERSPSM_NOT:
            return wine_dbg_sprintf("!%s%s%s", get_regname(reg),
                                    debug_print_relarg(reg),
                                    debug_print_swizzle(reg->u.swizzle));
    }
    return "Unknown modifier";
}

const char *debug_print_comp(DWORD comp) {
    switch(comp) {
        case BWRITER_COMPARISON_NONE: return "";
        case BWRITER_COMPARISON_GT:   return "_gt";
        case BWRITER_COMPARISON_EQ:   return "_eq";
        case BWRITER_COMPARISON_GE:   return "_ge";
        case BWRITER_COMPARISON_LT:   return "_lt";
        case BWRITER_COMPARISON_NE:   return "_ne";
        case BWRITER_COMPARISON_LE:   return "_le";
        default: return "_unknown";
    }
}

const char *debug_print_opcode(DWORD opcode) {
    switch(opcode){
        case BWRITERSIO_NOP:          return "nop";
        case BWRITERSIO_MOV:          return "mov";
        case BWRITERSIO_ADD:          return "add";
        case BWRITERSIO_SUB:          return "sub";
        case BWRITERSIO_MAD:          return "mad";
        case BWRITERSIO_MUL:          return "mul";
        case BWRITERSIO_RCP:          return "rcp";
        case BWRITERSIO_RSQ:          return "rsq";
        case BWRITERSIO_DP3:          return "dp3";
        case BWRITERSIO_DP4:          return "dp4";
        case BWRITERSIO_MIN:          return "min";
        case BWRITERSIO_MAX:          return "max";
        case BWRITERSIO_SLT:          return "slt";
        case BWRITERSIO_SGE:          return "sge";
        case BWRITERSIO_EXP:          return "exp";
        case BWRITERSIO_LOG:          return "log";
        case BWRITERSIO_LIT:          return "lit";
        case BWRITERSIO_DST:          return "dst";
        case BWRITERSIO_LRP:          return "lrp";
        case BWRITERSIO_FRC:          return "frc";
        case BWRITERSIO_M4x4:         return "m4x4";
        case BWRITERSIO_M4x3:         return "m4x3";
        case BWRITERSIO_M3x4:         return "m3x4";
        case BWRITERSIO_M3x3:         return "m3x3";
        case BWRITERSIO_M3x2:         return "m3x2";
        case BWRITERSIO_CALL:         return "call";
        case BWRITERSIO_CALLNZ:       return "callnz";
        case BWRITERSIO_LOOP:         return "loop";
        case BWRITERSIO_RET:          return "ret";
        case BWRITERSIO_ENDLOOP:      return "endloop";
        case BWRITERSIO_LABEL:        return "label";
        case BWRITERSIO_DCL:          return "dcl";
        case BWRITERSIO_POW:          return "pow";
        case BWRITERSIO_CRS:          return "crs";
        case BWRITERSIO_SGN:          return "sgn";
        case BWRITERSIO_ABS:          return "abs";
        case BWRITERSIO_NRM:          return "nrm";
        case BWRITERSIO_SINCOS:       return "sincos";
        case BWRITERSIO_REP:          return "rep";
        case BWRITERSIO_ENDREP:       return "endrep";
        case BWRITERSIO_IF:           return "if";
        case BWRITERSIO_IFC:          return "ifc";
        case BWRITERSIO_ELSE:         return "else";
        case BWRITERSIO_ENDIF:        return "endif";
        case BWRITERSIO_BREAK:        return "break";
        case BWRITERSIO_BREAKC:       return "breakc";
        case BWRITERSIO_MOVA:         return "mova";
        case BWRITERSIO_DEFB:         return "defb";
        case BWRITERSIO_DEFI:         return "defi";
        case BWRITERSIO_TEXCOORD:     return "texcoord";
        case BWRITERSIO_TEXKILL:      return "texkill";
        case BWRITERSIO_TEX:          return "tex";
        case BWRITERSIO_TEXBEM:       return "texbem";
        case BWRITERSIO_TEXBEML:      return "texbeml";
        case BWRITERSIO_TEXREG2AR:    return "texreg2ar";
        case BWRITERSIO_TEXREG2GB:    return "texreg2gb";
        case BWRITERSIO_TEXM3x2PAD:   return "texm3x2pad";
        case BWRITERSIO_TEXM3x2TEX:   return "texm3x2tex";
        case BWRITERSIO_TEXM3x3PAD:   return "texm3x3pad";
        case BWRITERSIO_TEXM3x3TEX:   return "texm3x3tex";
        case BWRITERSIO_TEXM3x3SPEC:  return "texm3x3vspec";
        case BWRITERSIO_TEXM3x3VSPEC: return "texm3x3vspec";
        case BWRITERSIO_EXPP:         return "expp";
        case BWRITERSIO_LOGP:         return "logp";
        case BWRITERSIO_CND:          return "cnd";
        case BWRITERSIO_DEF:          return "def";
        case BWRITERSIO_TEXREG2RGB:   return "texreg2rgb";
        case BWRITERSIO_TEXDP3TEX:    return "texdp3tex";
        case BWRITERSIO_TEXM3x2DEPTH: return "texm3x2depth";
        case BWRITERSIO_TEXDP3:       return "texdp3";
        case BWRITERSIO_TEXM3x3:      return "texm3x3";
        case BWRITERSIO_TEXDEPTH:     return "texdepth";
        case BWRITERSIO_CMP:          return "cmp";
        case BWRITERSIO_BEM:          return "bem";
        case BWRITERSIO_DP2ADD:       return "dp2add";
        case BWRITERSIO_DSX:          return "dsx";
        case BWRITERSIO_DSY:          return "dsy";
        case BWRITERSIO_TEXLDD:       return "texldd";
        case BWRITERSIO_SETP:         return "setp";
        case BWRITERSIO_TEXLDL:       return "texldl";
        case BWRITERSIO_BREAKP:       return "breakp";
        case BWRITERSIO_PHASE:        return "phase";

        case BWRITERSIO_TEXLDP:       return "texldp";
        case BWRITERSIO_TEXLDB:       return "texldb";

        default:                      return "unknown";
    }
}
