/*
 * Fixed function pipeline replacement using GL_ATI_fragment_shader
 *
 * Copyright 2008 Stefan Dösinger(for CodeWeavers)
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

#include <math.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

/* Some private defines, Constant associations, etc
 * Env bump matrix and per stage constant should be independent,
 * a stage that bumpmaps can't read the per state constant
 */
#define ATI_FFP_CONST_BUMPMAT(i) (GL_CON_0_ATI + i)
#define ATI_FFP_CONST_CONSTANT0 GL_CON_0_ATI
#define ATI_FFP_CONST_CONSTANT1 GL_CON_1_ATI
#define ATI_FFP_CONST_CONSTANT2 GL_CON_2_ATI
#define ATI_FFP_CONST_CONSTANT3 GL_CON_3_ATI
#define ATI_FFP_CONST_CONSTANT4 GL_CON_4_ATI
#define ATI_FFP_CONST_CONSTANT5 GL_CON_5_ATI
#define ATI_FFP_CONST_TFACTOR   GL_CON_6_ATI

/* GL_ATI_fragment_shader specific fixed function pipeline description. "Inherits" from the common one */
struct atifs_ffp_desc
{
    struct ffp_desc parent;
    GLuint shader;
};

struct atifs_private_data
{
    struct shader_arb_priv parent;
    struct list fragment_shaders; /* A linked list to track fragment pipeline replacement shaders */

};

static const char *debug_dstmod(GLuint mod) {
    switch(mod) {
        case GL_NONE:               return "GL_NONE";
        case GL_2X_BIT_ATI:         return "GL_2X_BIT_ATI";
        case GL_4X_BIT_ATI:         return "GL_4X_BIT_ATI";
        case GL_8X_BIT_ATI:         return "GL_8X_BIT_ATI";
        case GL_HALF_BIT_ATI:       return "GL_HALF_BIT_ATI";
        case GL_QUARTER_BIT_ATI:    return "GL_QUARTER_BIT_ATI";
        case GL_EIGHTH_BIT_ATI:     return "GL_EIGHTH_BIT_ATI";
        case GL_SATURATE_BIT_ATI:   return "GL_SATURATE_BIT_ATI";
        default:                    return "Unexpected modifier\n";
    }
}

static const char *debug_argmod(GLuint mod) {
    switch(mod) {
        case GL_NONE:
            return "GL_NONE";

        case GL_2X_BIT_ATI:
            return "GL_2X_BIT_ATI";
        case GL_COMP_BIT_ATI:
            return "GL_COMP_BIT_ATI";
        case GL_NEGATE_BIT_ATI:
            return "GL_NEGATE_BIT_ATI";
        case GL_BIAS_BIT_ATI:
            return "GL_BIAS_BIT_ATI";

        case GL_2X_BIT_ATI | GL_COMP_BIT_ATI:
            return "GL_2X_BIT_ATI | GL_COMP_BIT_ATI";
        case GL_2X_BIT_ATI | GL_NEGATE_BIT_ATI:
            return "GL_2X_BIT_ATI | GL_NEGATE_BIT_ATI";
        case GL_2X_BIT_ATI | GL_BIAS_BIT_ATI:
            return "GL_2X_BIT_ATI | GL_BIAS_BIT_ATI";
        case GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI:
            return "GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI";
        case GL_COMP_BIT_ATI | GL_BIAS_BIT_ATI:
            return "GL_COMP_BIT_ATI | GL_BIAS_BIT_ATI";
        case GL_NEGATE_BIT_ATI | GL_BIAS_BIT_ATI:
            return "GL_NEGATE_BIT_ATI | GL_BIAS_BIT_ATI";

        case GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI | GL_BIAS_BIT_ATI:
            return "GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI | GL_BIAS_BIT_ATI";
        case GL_2X_BIT_ATI | GL_NEGATE_BIT_ATI | GL_BIAS_BIT_ATI:
            return "GL_2X_BIT_ATI | GL_NEGATE_BIT_ATI | GL_BIAS_BIT_ATI";
        case GL_2X_BIT_ATI | GL_COMP_BIT_ATI | GL_BIAS_BIT_ATI:
            return "GL_2X_BIT_ATI | GL_COMP_BIT_ATI | GL_BIAS_BIT_ATI";
        case GL_2X_BIT_ATI | GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI:
            return "GL_2X_BIT_ATI | GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI";

        case GL_2X_BIT_ATI | GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI | GL_BIAS_BIT_ATI:
            return "GL_2X_BIT_ATI | GL_COMP_BIT_ATI | GL_NEGATE_BIT_ATI | GL_BIAS_BIT_ATI";

        default:
            return "Unexpected argmod combination\n";
    }
}
static const char *debug_register(GLuint reg) {
    switch(reg) {
        case GL_REG_0_ATI:                  return "GL_REG_0_ATI";
        case GL_REG_1_ATI:                  return "GL_REG_1_ATI";
        case GL_REG_2_ATI:                  return "GL_REG_2_ATI";
        case GL_REG_3_ATI:                  return "GL_REG_3_ATI";
        case GL_REG_4_ATI:                  return "GL_REG_4_ATI";
        case GL_REG_5_ATI:                  return "GL_REG_5_ATI";

        case GL_CON_0_ATI:                  return "GL_CON_0_ATI";
        case GL_CON_1_ATI:                  return "GL_CON_1_ATI";
        case GL_CON_2_ATI:                  return "GL_CON_2_ATI";
        case GL_CON_3_ATI:                  return "GL_CON_3_ATI";
        case GL_CON_4_ATI:                  return "GL_CON_4_ATI";
        case GL_CON_5_ATI:                  return "GL_CON_5_ATI";
        case GL_CON_6_ATI:                  return "GL_CON_6_ATI";
        case GL_CON_7_ATI:                  return "GL_CON_7_ATI";

        case GL_ZERO:                       return "GL_ZERO";
        case GL_ONE:                        return "GL_ONE";
        case GL_PRIMARY_COLOR:              return "GL_PRIMARY_COLOR";
        case GL_SECONDARY_INTERPOLATOR_ATI: return "GL_SECONDARY_INTERPOLATOR_ATI";

        default:                            return "Unknown register\n";
    }
}

static const char *debug_swizzle(GLuint swizzle) {
    switch(swizzle) {
        case GL_SWIZZLE_STR_ATI:        return "GL_SWIZZLE_STR_ATI";
        case GL_SWIZZLE_STQ_ATI:        return "GL_SWIZZLE_STQ_ATI";
        case GL_SWIZZLE_STR_DR_ATI:     return "GL_SWIZZLE_STR_DR_ATI";
        case GL_SWIZZLE_STQ_DQ_ATI:     return "GL_SWIZZLE_STQ_DQ_ATI";
        default:                        return "unknown swizzle";
    }
}

#define GLINFO_LOCATION (*gl_info)
static GLuint register_for_arg(DWORD arg, WineD3D_GL_Info *gl_info, unsigned int stage, GLuint *mod, GLuint tmparg) {
    GLenum ret;

    if(mod) *mod = GL_NONE;
    if(arg == 0xFFFFFFFF) return -1; /* This is the marker for unused registers */

    switch(arg & WINED3DTA_SELECTMASK) {
        case WINED3DTA_DIFFUSE:
            ret = GL_PRIMARY_COLOR;
            break;

        case WINED3DTA_CURRENT:
            /* Note that using GL_REG_0_ATI for the passed on register is safe because
             * texture0 is read at stage0, so in the worst case it is read in the
             * instruction writing to reg0. Afterwards texture0 is not used any longer.
             * If we're reading from current
             */
            if(stage == 0) {
                ret = GL_PRIMARY_COLOR;
            } else {
                ret = GL_REG_0_ATI;
            }
            break;

        case WINED3DTA_TEXTURE:
            ret = GL_REG_0_ATI + stage;
            break;

        case WINED3DTA_TFACTOR:
            ret = ATI_FFP_CONST_TFACTOR;
            break;

        case WINED3DTA_SPECULAR:
            ret = GL_SECONDARY_INTERPOLATOR_ATI;
            break;

        case WINED3DTA_TEMP:
            ret = tmparg;
            break;

        case WINED3DTA_CONSTANT:
            FIXME("Unhandled source argument WINED3DTA_TEMP\n");
            ret = GL_CON_0_ATI;
            break;

        default:
            FIXME("Unknown source argument %d\n", arg);
            ret = GL_ZERO;
    }

    if(arg & WINED3DTA_COMPLEMENT) {
        if(mod) *mod |= GL_COMP_BIT_ATI;
    }
    if(arg & WINED3DTA_ALPHAREPLICATE) {
        FIXME("Unhandled read modifier WINED3DTA_ALPHAREPLICATE\n");
    }
    return ret;
}

static GLuint find_tmpreg(struct texture_stage_op op[MAX_TEXTURES]) {
    int lowest_read = -1;
    int lowest_write = -1;
    int i;
    BOOL tex_used[MAX_TEXTURES];

    memset(tex_used, 0, sizeof(tex_used));
    for(i = 0; i < MAX_TEXTURES; i++) {
        if(op[i].cop == WINED3DTOP_DISABLE) {
            break;
        }

        if(lowest_read == -1 &&
          (op[i].carg1 == WINED3DTA_TEMP || op[i].carg2 == WINED3DTA_TEMP || op[i].carg0 == WINED3DTA_TEMP ||
           op[i].aarg1 == WINED3DTA_TEMP || op[i].aarg2 == WINED3DTA_TEMP || op[i].aarg0 == WINED3DTA_TEMP)) {
            lowest_read = i;
        }

        if(lowest_write == -1 && op[i].dst == WINED3DTA_TEMP) {
            lowest_write = i;
        }

        if(op[i].carg1 == WINED3DTA_TEXTURE || op[i].carg2 == WINED3DTA_TEXTURE || op[i].carg0 == WINED3DTA_TEXTURE ||
           op[i].aarg1 == WINED3DTA_TEXTURE || op[i].aarg2 == WINED3DTA_TEXTURE || op[i].aarg0 == WINED3DTA_TEXTURE) {
            tex_used[i] = TRUE;
        }
    }

    /* Temp reg not read? We don't need it, return GL_NONE */
    if(lowest_read == -1) return GL_NONE;

    if(lowest_write >= lowest_read) {
        FIXME("Temp register read before beeing written\n");
    }

    if(lowest_write == -1) {
        /* This needs a test. Maybe we are supposed to return 0.0/0.0/0.0/0.0, or fail drawprim, or whatever */
        FIXME("Temp register read without beeing written\n");
        return GL_REG_1_ATI;
    } else if(lowest_write >= 1) {
        /* If we're writing to the temp reg at earliest in stage 1, we can use register 1 for the temp result.
         * there may be texture data stored in reg 1, but we do not need it any longer since stage 1 already
         * read it
         */
        return GL_REG_1_ATI;
    } else {
        /* Search for a free texture register. We have 6 registers available. GL_REG_0_ATI is already used
         * for the regular result
         */
        for(i = 1; i < 6; i++) {
            if(!tex_used[i]) {
                return GL_REG_0_ATI + i;
            }
        }
        /* What to do here? Report it in ValidateDevice? */
        FIXME("Could not find a register for the temporary register\n");
        return 0;
    }
}

static GLuint gen_ati_shader(struct texture_stage_op op[MAX_TEXTURES], WineD3D_GL_Info *gl_info) {
    GLuint ret = GL_EXTCALL(glGenFragmentShadersATI(1));
    unsigned int stage;
    GLuint arg0, arg1, arg2, extrarg;
    GLuint dstmod, argmod0, argmod1, argmod2, argmodextra;
    GLuint swizzle;
    GLuint tmparg = find_tmpreg(op);
    GLuint dstreg;

    if(!ret) {
        ERR("Failed to generate a GL_ATI_fragment_shader shader id\n");
        return 0;
    }
    GL_EXTCALL(glBindFragmentShaderATI(ret));
    checkGLcall("GL_EXTCALL(glBindFragmentShaderATI(ret))");

    TRACE("glBeginFragmentShaderATI()\n");
    GL_EXTCALL(glBeginFragmentShaderATI());
    checkGLcall("GL_EXTCALL(glBeginFragmentShaderATI())");

    /* Pass 1: Generate sampling instructions for perturbation maps */
      for(stage = 0; stage < GL_LIMITS(textures); stage++) {
        if(op[stage].cop == WINED3DTOP_DISABLE) break;
        if(op[stage].cop != WINED3DTOP_BUMPENVMAP &&
           op[stage].cop != WINED3DTOP_BUMPENVMAPLUMINANCE) continue;

        TRACE("glSampleMapATI(GL_REG_%d_ATI, GL_TEXTURE_%d_ARB, GL_SWIZZLE_STR_ATI)\n",
              stage, stage);
        GL_EXTCALL(glSampleMapATI(GL_REG_0_ATI + stage,
                   GL_TEXTURE0_ARB + stage,
                   GL_SWIZZLE_STR_ATI));
        TRACE("glPassTexCoordATI(GL_REG_%d_ATI, GL_TEXTURE_%d_ARB, GL_SWIZZLE_STR_ATI)\n",
              stage + 1, stage + 1);
        GL_EXTCALL(glPassTexCoordATI(GL_REG_0_ATI + stage + 1,
                   GL_TEXTURE0_ARB + stage + 1,
                   GL_SWIZZLE_STR_ATI));

        /* We need GL_REG_5_ATI as a temporary register to swizzle the bump matrix. So we run into
         * issues if we're bump mapping on stage 4 or 5
         */
        if(stage >= 4) {
            FIXME("Bump mapping in stage %d\n", stage);
        }
    }

    /* Pass 2: Generate perturbation calculations */
    for(stage = 0; stage < GL_LIMITS(textures); stage++) {
        if(op[stage].cop == WINED3DTOP_DISABLE) break;
        if(op[stage].cop != WINED3DTOP_BUMPENVMAP &&
           op[stage].cop != WINED3DTOP_BUMPENVMAPLUMINANCE) continue;

        /* Nice thing, we get the color correction for free :-) */
        if(op[stage].color_correction == WINED3DFMT_V8U8) {
            argmodextra = GL_2X_BIT_ATI | GL_BIAS_BIT_ATI;
        } else {
            argmodextra = 0;
        }

        TRACE("glColorFragmentOp3ATI(GL_DOT2_ADD_ATI, GL_REG_%d_ATI, GL_RED_BIT_ATI, GL_NONE, GL_REG_%d_ATI, GL_NONE, %s, ATI_FFP_CONST_BUMPMAT(%d), GL_NONE, GL_NONE, GL_REG_%d_ATI, GL_RED, GL_NONE)\n",
              stage + 1, stage, debug_argmod(argmodextra), stage, stage + 1);
        GL_EXTCALL(glColorFragmentOp3ATI(GL_DOT2_ADD_ATI, GL_REG_0_ATI + stage + 1, GL_RED_BIT_ATI, GL_NONE,
                                         GL_REG_0_ATI + stage, GL_NONE, argmodextra,
                                         ATI_FFP_CONST_BUMPMAT(stage), GL_NONE, GL_2X_BIT_ATI | GL_BIAS_BIT_ATI,
                                         GL_REG_0_ATI + stage + 1, GL_RED, GL_NONE));

        /* FIXME: How can I make GL_DOT2_ADD_ATI read the factors from blue and alpha? It defaults to red and green,
         * and it is fairly easy to make it read GL_BLUE or BL_ALPHA, but I can't get an R * B + G * A. So we're wasting
         * one register and two instructions in this pass for a simple swizzling operation.
         * For starters it might be good enough to merge the two movs into one, but even that isn't possible :-(
         *
         * NOTE: GL_BLUE | GL_ALPHA is not possible. It doesn't throw a compilation error, but an OR operation on the
         * constants doesn't make sense, considering their values.
         */
        TRACE("glColorFragmentOp1ATI(GL_MOV_ATI, GL_REG_5_ATI, GL_RED_BIT_ATI, GL_NONE, ATI_FFP_CONST_BUMPMAT(%d), GL_BLUE, GL_NONE)\n", stage);
        GL_EXTCALL(glColorFragmentOp1ATI(GL_MOV_ATI, GL_REG_5_ATI, GL_RED_BIT_ATI, GL_NONE,
                                         ATI_FFP_CONST_BUMPMAT(stage), GL_BLUE, GL_NONE));
        TRACE("glColorFragmentOp1ATI(GL_MOV_ATI, GL_REG_5_ATI, GL_GREEN_BIT_ATI, GL_NONE, ATI_FFP_CONST_BUMPMAT(%d), GL_ALPHA, GL_NONE)\n", stage);
        GL_EXTCALL(glColorFragmentOp1ATI(GL_MOV_ATI, GL_REG_5_ATI, GL_GREEN_BIT_ATI, GL_NONE,
                                        ATI_FFP_CONST_BUMPMAT(stage), GL_ALPHA, GL_NONE));
        TRACE("glColorFragmentOp3ATI(GL_DOT2_ADD_ATI, GL_REG_%d_ATI, GL_GREEN_BIT_ATI, GL_NONE, GL_REG_%d_ATI, GL_NONE, %s, GL_REG_5_ATI, GL_NONE, GL_NONE, GL_REG_%d_ATI, GL_GREEN, GL_NONE)\n",
              stage + 1, stage, debug_argmod(argmodextra), stage + 1);
        GL_EXTCALL(glColorFragmentOp3ATI(GL_DOT2_ADD_ATI, GL_REG_0_ATI + stage + 1, GL_GREEN_BIT_ATI, GL_NONE,
                                         GL_REG_0_ATI + stage, GL_NONE, argmodextra,
                                         GL_REG_5_ATI, GL_NONE, GL_2X_BIT_ATI | GL_BIAS_BIT_ATI,
                                         GL_REG_0_ATI + stage + 1, GL_GREEN, GL_NONE));
    }

    /* Pass 3: Generate sampling instructions for regular textures */
    for(stage = 0; stage < GL_LIMITS(textures); stage++) {
        if(op[stage].cop == WINED3DTOP_DISABLE) {
            break;
        }

        if(op[stage].projected == proj_none) {
            swizzle = GL_SWIZZLE_STR_ATI;
        } else if(op[stage].projected == proj_count3) {
            /* TODO: D3DTTFF_COUNT3 | D3DTTFF_PROJECTED would be GL_SWIZZLE_STR_DR_ATI.
             * However, the FFP vertex processing texture transform matrix handler does
             * some transformations in the texture matrix which makes the 3rd coordinate
             * arrive in Q, not R in that case. This is needed for opengl fixed function
             * fragment processing which always divides by Q. In this backend we can
             * handle that properly and be compatible with vertex shader output and avoid
             * side effects of the texture matrix games
             */
            swizzle = GL_SWIZZLE_STQ_DQ_ATI;
        } else {
            swizzle = GL_SWIZZLE_STQ_DQ_ATI;
        }

        if((op[stage].carg0 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE ||
           (op[stage].carg1 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE ||
           (op[stage].carg2 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE ||
           (op[stage].aarg0 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE ||
           (op[stage].aarg1 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE ||
           (op[stage].aarg2 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE ||
            op[stage].cop == WINED3DTOP_BLENDTEXTUREALPHA) {

            if(stage > 0 &&
               (op[stage - 1].cop == WINED3DTOP_BUMPENVMAP ||
                op[stage - 1].cop == WINED3DTOP_BUMPENVMAPLUMINANCE)) {
                TRACE("glSampleMapATI(GL_REG_%d_ATI, GL_REG_%d_ATI, GL_SWIZZLE_STR_ATI)\n",
                      stage, stage);
                GL_EXTCALL(glSampleMapATI(GL_REG_0_ATI + stage,
                           GL_REG_0_ATI + stage,
                           GL_SWIZZLE_STR_ATI));
            } else {
                TRACE("glSampleMapATI(GL_REG_%d_ATI, GL_TEXTURE_%d_ARB, %s)\n",
                    stage, stage, debug_swizzle(swizzle));
                GL_EXTCALL(glSampleMapATI(GL_REG_0_ATI + stage,
                                        GL_TEXTURE0_ARB + stage,
                                        swizzle));
            }
        }
    }

    /* Pass 4: Generate the arithmetic instructions */
    for(stage = 0; stage < MAX_TEXTURES; stage++) {
        if(op[stage].cop == WINED3DTOP_DISABLE) {
            if(stage == 0) {
                /* Handle complete texture disabling gracefully */
                TRACE("glColorFragmentOp1ATI(GL_MOV_ATI, GL_REG_0_ATI, GL_NONE, GL_NONE, GL_PRIMARY_COLOR, GL_NONE, GL_NONE)\n");
                GL_EXTCALL(glColorFragmentOp1ATI(GL_MOV_ATI, GL_REG_0_ATI, GL_NONE, GL_NONE,
                                                 GL_PRIMARY_COLOR, GL_NONE, GL_NONE));
                TRACE("glAlphaFragmentOp1ATI(GL_MOV_ATI, GL_REG_0_ATI, GL_NONE, GL_PRIMARY_COLOR, GL_NONE, GL_NONE)\n");
                GL_EXTCALL(glAlphaFragmentOp1ATI(GL_MOV_ATI, GL_REG_0_ATI, GL_NONE,
                                                 GL_PRIMARY_COLOR, GL_NONE, GL_NONE));
            }
            break;
        }

        if(op[stage].dst == WINED3DTA_TEMP) {
            /* If we're writing to D3DTA_TEMP, but never reading from it we don't have to write there in the first place.
             * skip the entire stage, this saves some GPU time
             */
            if(tmparg == GL_NONE) continue;

            dstreg = tmparg;
        } else {
            dstreg = GL_REG_0_ATI;
        }

        arg0 = register_for_arg(op[stage].carg0, gl_info, stage, &argmod0, tmparg);
        arg1 = register_for_arg(op[stage].carg1, gl_info, stage, &argmod1, tmparg);
        arg2 = register_for_arg(op[stage].carg2, gl_info, stage, &argmod2, tmparg);
        dstmod = GL_NONE;
        argmodextra = GL_NONE;
        extrarg = GL_NONE;

        switch(op[stage].cop) {
            case WINED3DTOP_SELECTARG2:
                arg1 = arg2;
                argmod1 = argmod2;
            case WINED3DTOP_SELECTARG1:
                TRACE("glColorFragmentOp1ATI(GL_MOV_ATI, %s, GL_NONE, GL_NONE, %s, GL_NONE, %s)\n",
                      debug_register(dstreg), debug_register(arg1), debug_argmod(argmod1));
                GL_EXTCALL(glColorFragmentOp1ATI(GL_MOV_ATI, dstreg, GL_NONE, GL_NONE,
                                                 arg1, GL_NONE, argmod1));
                break;

            case WINED3DTOP_MODULATE4X:
                if(dstmod == GL_NONE) dstmod = GL_4X_BIT_ATI;
            case WINED3DTOP_MODULATE2X:
                if(dstmod == GL_NONE) dstmod = GL_2X_BIT_ATI;
            case WINED3DTOP_MODULATE:
                TRACE("glColorFragmentOp2ATI(GL_MUL_ATI, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg), debug_dstmod(dstmod),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmod2));
                GL_EXTCALL(glColorFragmentOp2ATI(GL_MUL_ATI, dstreg, GL_NONE, dstmod,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmod2));
                break;

            case WINED3DTOP_ADDSIGNED2X:
                dstmod = GL_2X_BIT_ATI;
            case WINED3DTOP_ADDSIGNED:
                argmodextra = GL_BIAS_BIT_ATI;
            case WINED3DTOP_ADD:
                TRACE("glColorFragmentOp2ATI(GL_ADD_ATI, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg), debug_dstmod(dstmod),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmodextra | argmod2));
                GL_EXTCALL(glColorFragmentOp2ATI(GL_ADD_ATI, GL_REG_0_ATI, GL_NONE, dstmod,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmodextra | argmod2));
                break;

            case WINED3DTOP_SUBTRACT:
                TRACE("glColorFragmentOp2ATI(GL_SUB_ATI, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg), debug_dstmod(dstmod),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmod2));
                GL_EXTCALL(glColorFragmentOp2ATI(GL_SUB_ATI, dstreg, GL_NONE, dstmod,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmod2));
                break;

            case WINED3DTOP_ADDSMOOTH:
                argmodextra = argmod1 & GL_COMP_BIT_ATI ? argmod1 & ~GL_COMP_BIT_ATI : argmod1 | GL_COMP_BIT_ATI;
                TRACE("glColorFragmentOp3ATI(GL_MAD_ATI, %s, GL_NONE, GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg1), debug_argmod(argmodextra),
                      debug_register(arg1), debug_argmod(argmod1));
                /* Dst = arg1 + * arg2(1 -arg 1)
                 *     = arg2 * (1 - arg1) + arg1
                 */
                GL_EXTCALL(glColorFragmentOp3ATI(GL_MAD_ATI, dstreg, GL_NONE, GL_NONE,
                                                 arg2, GL_NONE, argmod2,
                                                 arg1, GL_NONE, argmodextra,
                                                 arg1, GL_NONE, argmod1));
                break;

            case WINED3DTOP_BLENDCURRENTALPHA:
                if(extrarg == GL_NONE) extrarg = register_for_arg(WINED3DTA_CURRENT, gl_info, stage, NULL, -1);
            case WINED3DTOP_BLENDFACTORALPHA:
                if(extrarg == GL_NONE) extrarg = register_for_arg(WINED3DTA_TFACTOR, gl_info, stage, NULL, -1);
            case WINED3DTOP_BLENDTEXTUREALPHA:
                if(extrarg == GL_NONE) extrarg = register_for_arg(WINED3DTA_TEXTURE, gl_info, stage, NULL, -1);
            case WINED3DTOP_BLENDDIFFUSEALPHA:
                if(extrarg == GL_NONE) extrarg = register_for_arg(WINED3DTA_DIFFUSE, gl_info, stage, NULL, -1);
                TRACE("glColorFragmentOp3ATI(GL_LERP_ATI, %s, GL_NONE, GL_NONE, %s, GL_ALPHA, GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(extrarg),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmod2));
                GL_EXTCALL(glColorFragmentOp3ATI(GL_LERP_ATI, dstreg, GL_NONE, GL_NONE,
                                                 extrarg, GL_ALPHA, GL_NONE,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmod2));
                break;

            case WINED3DTOP_BLENDTEXTUREALPHAPM:
                arg0 = register_for_arg(WINED3DTA_TEXTURE, gl_info, stage, NULL, -1);
                TRACE("glColorFragmentOp3ATI(GL_MAD_ATI, %s, GL_NONE, GL_NONE, %s, GL_NONE, %s, %s, GL_ALPHA, GL_COMP_BIT_ATI, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg0),
                      debug_register(arg1), debug_argmod(argmod1));
                GL_EXTCALL(glColorFragmentOp3ATI(GL_MAD_ATI, dstreg, GL_NONE, GL_NONE,
                                                 arg2, GL_NONE,  argmod2,
                                                 arg0, GL_ALPHA, GL_COMP_BIT_ATI,
                                                 arg1, GL_NONE,  argmod1));
                break;

            /* D3DTOP_PREMODULATE ???? */

            case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
                argmodextra = argmod1 & GL_COMP_BIT_ATI ? argmod1 & ~GL_COMP_BIT_ATI : argmod1 | GL_COMP_BIT_ATI;
            case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
                if(!argmodextra) argmodextra = argmod1;
                TRACE("glColorFragmentOp3ATI(GL_MAD_ATI, %s, GL_NONE, GL_NONE, %s, GL_NONE, %s, %s, GL_ALPHA, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg1), debug_argmod(argmodextra), debug_register(arg1), debug_argmod(arg1));
                GL_EXTCALL(glColorFragmentOp3ATI(GL_MAD_ATI, dstreg, GL_NONE, GL_NONE,
                                                 arg2, GL_NONE,  argmod2,
                                                 arg1, GL_ALPHA, argmodextra,
                                                 arg1, GL_NONE,  argmod1));
                break;

            case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
                argmodextra = argmod1 & GL_COMP_BIT_ATI ? argmod1 & ~GL_COMP_BIT_ATI : argmod1 | GL_COMP_BIT_ATI;
            case WINED3DTOP_MODULATECOLOR_ADDALPHA:
                if(!argmodextra) argmodextra = argmod1;
                TRACE("glColorFragmentOp3ATI(GL_MAD_ATI, %s, GL_NONE, GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_ALPHA, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg1), debug_argmod(argmodextra),
                      debug_register(arg1), debug_argmod(argmod1));
                GL_EXTCALL(glColorFragmentOp3ATI(GL_MAD_ATI, dstreg, GL_NONE, GL_NONE,
                                                 arg2, GL_NONE,  argmod2,
                                                 arg1, GL_NONE,  argmodextra,
                                                 arg1, GL_ALPHA, argmod1));
                break;

            case WINED3DTOP_DOTPRODUCT3:
                TRACE("glColorFragmentOp2ATI(GL_DOT3_ATI, %s, GL_NONE, GL_4X_BIT_ATI, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg1), debug_argmod(argmod1 | GL_BIAS_BIT_ATI),
                      debug_register(arg2), debug_argmod(argmod2 | GL_BIAS_BIT_ATI));
                GL_EXTCALL(glColorFragmentOp2ATI(GL_DOT3_ATI, dstreg, GL_NONE, GL_4X_BIT_ATI,
                                                 arg1, GL_NONE, argmod1 | GL_BIAS_BIT_ATI,
                                                 arg2, GL_NONE, argmod2 | GL_BIAS_BIT_ATI));
                break;

            case WINED3DTOP_MULTIPLYADD:
                TRACE("glColorFragmentOp3ATI(GL_MAD_ATI, %s, GL_NONE, GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg0), debug_argmod(argmod0),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg1), debug_argmod(argmod1));
                GL_EXTCALL(glColorFragmentOp3ATI(GL_MAD_ATI, dstreg, GL_NONE, GL_NONE,
                                                 arg0, GL_NONE, argmod0,
                                                 arg2, GL_NONE, argmod2,
                                                 arg1, GL_NONE, argmod1));
                break;

            case WINED3DTOP_LERP:
                TRACE("glColorFragmentOp3ATI(GL_LERP_ATI, %s, GL_NONE, GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg0), debug_argmod(argmod0));
                GL_EXTCALL(glColorFragmentOp3ATI(GL_LERP_ATI, dstreg, GL_NONE, GL_NONE,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmod2,
                                                 arg0, GL_NONE, argmod0));
                break;

            case WINED3DTOP_BUMPENVMAP:
            case WINED3DTOP_BUMPENVMAPLUMINANCE:
                /* Those are handled in the first pass of the shader(generation pass 1 and 2) alraedy */
                break;

            default: FIXME("Unhandled color operation %d on stage %d\n", op[stage].cop, stage);
        }

        arg0 = register_for_arg(op[stage].aarg0, gl_info, stage, &argmod0, tmparg);
        arg1 = register_for_arg(op[stage].aarg1, gl_info, stage, &argmod1, tmparg);
        arg2 = register_for_arg(op[stage].aarg2, gl_info, stage, &argmod2, tmparg);
        dstmod = GL_NONE;
        argmodextra = GL_NONE;
        extrarg = GL_NONE;

        switch(op[stage].aop) {
            case WINED3DTOP_DISABLE:
                /* Get the primary color to the output if on stage 0, otherwise leave register 0 untouched */
                if(stage == 0) {
                    TRACE("glAlphaFragmentOp1ATI(GL_MOV_ATI, GL_REG_0_ATI, GL_NONE, GL_PRIMARY_COLOR, GL_NONE, GL_NONE)\n");
                    GL_EXTCALL(glAlphaFragmentOp1ATI(GL_MOV_ATI, GL_REG_0_ATI, GL_NONE,
                               GL_PRIMARY_COLOR, GL_NONE, GL_NONE));
                }
                break;

            case WINED3DTOP_SELECTARG2:
                arg1 = arg2;
                argmod1 = argmod2;
            case WINED3DTOP_SELECTARG1:
                TRACE("glAlphaFragmentOp1ATI(GL_MOV_ATI, %s,          GL_NONE, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg1), debug_argmod(argmod1));
                GL_EXTCALL(glAlphaFragmentOp1ATI(GL_MOV_ATI, dstreg, GL_NONE,
                                                 arg1, GL_NONE, argmod1));
                break;

            case WINED3DTOP_MODULATE4X:
                if(dstmod == GL_NONE) dstmod = GL_4X_BIT_ATI;
            case WINED3DTOP_MODULATE2X:
                if(dstmod == GL_NONE) dstmod = GL_2X_BIT_ATI;
            case WINED3DTOP_MODULATE:
                TRACE("glAlphaFragmentOp2ATI(GL_MUL_ATI, %s,          %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg), debug_dstmod(dstmod),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmod2));
                GL_EXTCALL(glAlphaFragmentOp2ATI(GL_MUL_ATI, dstreg, dstmod,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmod2));
                break;

            case WINED3DTOP_ADDSIGNED2X:
                dstmod = GL_2X_BIT_ATI;
            case WINED3DTOP_ADDSIGNED:
                argmodextra = GL_BIAS_BIT_ATI;
            case WINED3DTOP_ADD:
                TRACE("glAlphaFragmentOp2ATI(GL_ADD_ATI, %s,          %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg), debug_dstmod(dstmod),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmodextra | argmod2));
                GL_EXTCALL(glAlphaFragmentOp2ATI(GL_ADD_ATI, dstreg, dstmod,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmodextra | argmod2));
                break;

            case WINED3DTOP_SUBTRACT:
                TRACE("glAlphaFragmentOp2ATI(GL_SUB_ATI, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg), debug_dstmod(dstmod),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmod2));
                GL_EXTCALL(glAlphaFragmentOp2ATI(GL_SUB_ATI, dstreg, dstmod,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmod2));
                break;

            case WINED3DTOP_ADDSMOOTH:
                argmodextra = argmod1 & GL_COMP_BIT_ATI ? argmod1 & ~GL_COMP_BIT_ATI : argmod1 | GL_COMP_BIT_ATI;
                TRACE("glAlphaFragmentOp3ATI(GL_MAD_ATI, %s,          GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg1), debug_argmod(argmodextra),
                      debug_register(arg1), debug_argmod(argmod1));
                /* Dst = arg1 + * arg2(1 -arg 1)
                 *     = arg2 * (1 - arg1) + arg1
                 */
                GL_EXTCALL(glAlphaFragmentOp3ATI(GL_MAD_ATI, dstreg, GL_NONE,
                                                 arg2, GL_NONE, argmod2,
                                                 arg1, GL_NONE, argmodextra,
                                                 arg1, GL_NONE, argmod1));
                break;

            case WINED3DTOP_BLENDCURRENTALPHA:
                if(extrarg == GL_NONE) extrarg = register_for_arg(WINED3DTA_CURRENT, gl_info, stage, NULL, -1);
            case WINED3DTOP_BLENDFACTORALPHA:
                if(extrarg == GL_NONE) extrarg = register_for_arg(WINED3DTA_TFACTOR, gl_info, stage, NULL, -1);
            case WINED3DTOP_BLENDTEXTUREALPHA:
                if(extrarg == GL_NONE) extrarg = register_for_arg(WINED3DTA_TEXTURE, gl_info, stage, NULL, -1);
            case WINED3DTOP_BLENDDIFFUSEALPHA:
                if(extrarg == GL_NONE) extrarg = register_for_arg(WINED3DTA_DIFFUSE, gl_info, stage, NULL, -1);
                TRACE("glAlphaFragmentOp3ATI(GL_LERP_ATI, %s,          GL_NONE, %s, GL_ALPHA, GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(extrarg),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmod2));
                GL_EXTCALL(glAlphaFragmentOp3ATI(GL_LERP_ATI, dstreg, GL_NONE,
                                                 extrarg, GL_ALPHA, GL_NONE,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmod2));
                break;

            case WINED3DTOP_BLENDTEXTUREALPHAPM:
                arg0 = register_for_arg(WINED3DTA_TEXTURE, gl_info, stage, NULL, -1);
                TRACE("glAlphaFragmentOp3ATI(GL_MAD_ATI, %s,          GL_NONE, %s, GL_NONE, %s, %s, GL_ALPHA, GL_COMP_BIT_ATI, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg0),
                      debug_register(arg1), debug_argmod(argmod1));
                GL_EXTCALL(glAlphaFragmentOp3ATI(GL_MAD_ATI, dstreg, GL_NONE,
                                                 arg2, GL_NONE,  argmod2,
                                                 arg0, GL_ALPHA, GL_COMP_BIT_ATI,
                                                 arg1, GL_NONE,  argmod1));
                break;

            /* D3DTOP_PREMODULATE ???? */

            case WINED3DTOP_DOTPRODUCT3:
                TRACE("glAlphaFragmentOp2ATI(GL_DOT3_ATI, %s, GL_NONE, GL_4X_BIT_ATI, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg1), debug_argmod(argmod1 | GL_BIAS_BIT_ATI),
                      debug_register(arg2), debug_argmod(argmod2 | GL_BIAS_BIT_ATI));
                GL_EXTCALL(glAlphaFragmentOp2ATI(GL_DOT3_ATI, dstreg, GL_4X_BIT_ATI,
                                                 arg1, GL_NONE, argmod1 | GL_BIAS_BIT_ATI,
                                                 arg2, GL_NONE, argmod2 | GL_BIAS_BIT_ATI));
                break;

            case WINED3DTOP_MULTIPLYADD:
                TRACE("glAlphaFragmentOp3ATI(GL_MAD_ATI, %s,          GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg0), debug_argmod(argmod0),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg1), debug_argmod(argmod1));
                GL_EXTCALL(glAlphaFragmentOp3ATI(GL_MAD_ATI, dstreg,          GL_NONE,
                           arg0, GL_NONE, argmod0,
                           arg2, GL_NONE, argmod2,
                           arg1, GL_NONE, argmod1));
                break;

            case WINED3DTOP_LERP:
                TRACE("glAlphaFragmentOp3ATI(GL_LERP_ATI, %s,          GL_NONE, %s, GL_NONE, %s, %s, GL_NONE, %s, %s, GL_NONE, %s)\n",
                      debug_register(dstreg),
                      debug_register(arg1), debug_argmod(argmod1),
                      debug_register(arg2), debug_argmod(argmod2),
                      debug_register(arg0), debug_argmod(argmod0));
                GL_EXTCALL(glAlphaFragmentOp3ATI(GL_LERP_ATI, dstreg, GL_NONE,
                                                 arg1, GL_NONE, argmod1,
                                                 arg2, GL_NONE, argmod2,
                                                 arg0, GL_NONE, argmod0));
                break;

            case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
            case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
            case WINED3DTOP_MODULATECOLOR_ADDALPHA:
            case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
            case WINED3DTOP_BUMPENVMAP:
            case WINED3DTOP_BUMPENVMAPLUMINANCE:
                ERR("Application uses an invalid alpha operation\n");
                break;

            default: FIXME("Unhandled alpha operation %d on stage %d\n", op[stage].aop, stage);
        }
    }

    TRACE("glEndFragmentShaderATI()\n");
    GL_EXTCALL(glEndFragmentShaderATI());
    checkGLcall("GL_EXTCALL(glEndFragmentShaderATI())");
    return ret;
}
#undef GLINFO_LOCATION

#define GLINFO_LOCATION stateblock->wineD3DDevice->adapter->gl_info
static void set_tex_op_atifs(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    IWineD3DDeviceImpl          *This = stateblock->wineD3DDevice;
    struct atifs_ffp_desc       *desc;
    struct texture_stage_op     op[MAX_TEXTURES];
    struct atifs_private_data   *priv = (struct atifs_private_data *) This->shader_priv;

    gen_ffp_op(stateblock, op);
    desc = (struct atifs_ffp_desc *) find_ffp_shader(&priv->fragment_shaders, op);
    if(!desc) {
        desc = HeapAlloc(GetProcessHeap(), 0, sizeof(*desc));
        if(!desc) {
            ERR("Out of memory\n");
            return;
        }
        memcpy(desc->parent.op, op, sizeof(op));
        desc->shader = gen_ati_shader(op, &GLINFO_LOCATION);
        add_ffp_shader(&priv->fragment_shaders, &desc->parent);
        TRACE("Allocated fixed function replacement shader descriptor %p\n", desc);
    }

    GL_EXTCALL(glBindFragmentShaderATI(desc->shader));
}

static void state_texfactor_atifs(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    float col[4];
    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_TEXTUREFACTOR], col);

    GL_EXTCALL(glSetFragmentShaderConstantATI(ATI_FFP_CONST_TFACTOR, col));
    checkGLcall("glSetFragmentShaderConstantATI(ATI_FFP_CONST_TFACTOR, col)");
}

static void set_bumpmat(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    float mat[2][2];

    mat[0][0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT00]);
    mat[1][0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT01]);
    mat[0][1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT10]);
    mat[1][1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT11]);
    /* GL_ATI_fragment_shader allows only constants from 0.0 to 1.0, but the bumpmat
     * constants can be in any range. While they should stay between [-1.0 and 1.0] because
     * Shader Model 1.x pixel shaders are clamped to that range negative values are used occasionally,
     * for example by our d3d9 test. So to get negative values scale -1;1 to 0;1 and undo that in the
     * shader(it is free). This might potentially reduce precision. However, if the hardware does
     * support proper floats it shouldn't, and if it doesn't we can't get anything better anyway
     */
    mat[0][0] = (mat[0][0] + 1.0) * 0.5;
    mat[1][0] = (mat[1][0] + 1.0) * 0.5;
    mat[0][1] = (mat[0][1] + 1.0) * 0.5;
    mat[1][1] = (mat[1][1] + 1.0) * 0.5;
    GL_EXTCALL(glSetFragmentShaderConstantATI(ATI_FFP_CONST_BUMPMAT(stage), (float *) mat));
    checkGLcall("glSetFragmentShaderConstantATI(ATI_FFP_CONST_BUMPMAT(stage), mat)");

    /* FIXME: This should go away
     * This is currently needed because atifs borrows a pixel shader implementation
     * from somewhere else, but consumes bump map matrix change events. The other pixel
     * shader implementation may need notification about the change to update the texbem
     * constants. Once ATIFS supports real shaders on its own, and GLSL/ARB have a replacement
     * pipeline this call can go away
     *
     * FIXME2: Even considering this workaround calling FFPStateTable directly isn't nice
     * as well. Better would be to call the model's table we inherit from, but currently
     * it is always the FFP table, and as soon as this changes we can remove the call anyway
     */
    FFPStateTable[state].apply(state, stateblock, context);
}
#undef GLINFO_LOCATION

/* our state table. Borrows lots of stuff from the base implementation */
struct StateEntry ATIFSStateTable[STATE_HIGHEST + 1];

static void init_state_table() {
    unsigned int i;
    const DWORD rep = STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP);
    memcpy(ATIFSStateTable, arb_program_shader_backend.StateTable, sizeof(ATIFSStateTable));

    for(i = 0; i < MAX_TEXTURES; i++) {
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP)].representative = rep;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_COLORARG1)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_COLORARG1)].representative = rep;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_COLORARG2)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_COLORARG2)].representative = rep;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_COLORARG0)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_COLORARG0)].representative = rep;

        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_ALPHAOP)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_ALPHAOP)].representative = rep;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_ALPHAARG1)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_ALPHAARG1)].representative = rep;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_ALPHAARG2)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_ALPHAARG2)].representative = rep;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_ALPHAARG0)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_ALPHAARG0)].representative = rep;

        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_RESULTARG)].apply = set_tex_op_atifs;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_RESULTARG)].representative = rep;

        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_BUMPENVMAT00)].apply = set_bumpmat;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_BUMPENVMAT01)].apply = set_bumpmat;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_BUMPENVMAT10)].apply = set_bumpmat;
        ATIFSStateTable[STATE_TEXTURESTAGE(i, WINED3DTSS_BUMPENVMAT11)].apply = set_bumpmat;
    }

    ATIFSStateTable[STATE_RENDER(WINED3DRS_TEXTUREFACTOR)].apply = state_texfactor_atifs;
    ATIFSStateTable[STATE_RENDER(WINED3DRS_TEXTUREFACTOR)].representative = STATE_RENDER(WINED3DRS_TEXTUREFACTOR);
}

/* GL_ATI_fragment_shader backend.It borrows a lot from a the
 * ARB shader backend, currently the whole vertex processing
 * code. This code would also forward pixel shaders, but if
 * GL_ARB_fragment_program is supported, the atifs shader backend
 * is not used.
 */
static void shader_atifs_select(IWineD3DDevice *iface, BOOL usePS, BOOL useVS) {
    arb_program_shader_backend.shader_select(iface, usePS, useVS);
}

static void shader_atifs_select_depth_blt(IWineD3DDevice *iface) {
    arb_program_shader_backend.shader_select_depth_blt(iface);
}

static void shader_atifs_destroy_depth_blt(IWineD3DDevice *iface) {
    arb_program_shader_backend.shader_destroy_depth_blt(iface);
}

static void shader_atifs_load_constants(IWineD3DDevice *iface, char usePS, char useVS) {
    arb_program_shader_backend.shader_load_constants(iface, usePS, useVS);
}

static void shader_atifs_cleanup(IWineD3DDevice *iface) {
    arb_program_shader_backend.shader_cleanup(iface);
}

static void shader_atifs_color_correction(SHADER_OPCODE_ARG* arg) {
    arb_program_shader_backend.shader_color_correction(arg);
}

static void shader_atifs_destroy(IWineD3DBaseShader *iface) {
    arb_program_shader_backend.shader_destroy(iface);
}

static HRESULT shader_atifs_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    HRESULT hr;
    struct atifs_private_data *priv;
    hr = arb_program_shader_backend.shader_alloc_private(iface);
    if(FAILED(hr)) return hr;

    This->shader_priv = HeapReAlloc(GetProcessHeap(), 0, This->shader_priv,
                                    sizeof(struct atifs_private_data));
    priv = (struct atifs_private_data *) This->shader_priv;
    list_init(&priv->fragment_shaders);
    return WINED3D_OK;
}

#define GLINFO_LOCATION This->adapter->gl_info
static void shader_atifs_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    struct atifs_private_data *priv = (struct atifs_private_data *) This->shader_priv;
    struct ffp_desc *entry, *entry2;
    struct atifs_ffp_desc *entry_ati;

    ENTER_GL();
    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &priv->fragment_shaders, struct ffp_desc, entry) {
        entry_ati = (struct atifs_ffp_desc *) entry;
        GL_EXTCALL(glDeleteFragmentShaderATI(entry_ati->shader));
        checkGLcall("glDeleteFragmentShaderATI(entry->shader)");
        list_remove(&entry->entry);
        HeapFree(GetProcessHeap(), 0, entry);
    }
    LEAVE_GL();

    /* Not actually needed, but revert what we've done before */
    This->shader_priv = HeapReAlloc(GetProcessHeap(), 0, This->shader_priv,
                                    sizeof(struct shader_arb_priv));
    arb_program_shader_backend.shader_free_private(iface);
}
#undef GLINFO_LOCATION

static BOOL shader_atifs_dirty_const(IWineD3DDevice *iface) {
    return arb_program_shader_backend.shader_dirtifyable_constants(iface);
}

static void shader_atifs_load_init(void) {
    arb_program_shader_backend.shader_dll_load_init();
    init_state_table();
}

static void shader_atifs_get_caps(WINED3DDEVTYPE devtype, WineD3D_GL_Info *gl_info, struct shader_caps *caps) {
    arb_program_shader_backend.shader_get_caps(devtype, gl_info, caps);

    caps->TextureOpCaps =  WINED3DTEXOPCAPS_DISABLE                     |
                           WINED3DTEXOPCAPS_SELECTARG1                  |
                           WINED3DTEXOPCAPS_SELECTARG2                  |
                           WINED3DTEXOPCAPS_MODULATE4X                  |
                           WINED3DTEXOPCAPS_MODULATE2X                  |
                           WINED3DTEXOPCAPS_MODULATE                    |
                           WINED3DTEXOPCAPS_ADDSIGNED2X                 |
                           WINED3DTEXOPCAPS_ADDSIGNED                   |
                           WINED3DTEXOPCAPS_ADD                         |
                           WINED3DTEXOPCAPS_SUBTRACT                    |
                           WINED3DTEXOPCAPS_ADDSMOOTH                   |
                           WINED3DTEXOPCAPS_BLENDCURRENTALPHA           |
                           WINED3DTEXOPCAPS_BLENDFACTORALPHA            |
                           WINED3DTEXOPCAPS_BLENDTEXTUREALPHA           |
                           WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA           |
                           WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM         |
                           WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR      |
                           WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA      |
                           WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA   |
                           WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR   |
                           WINED3DTEXOPCAPS_DOTPRODUCT3                 |
                           WINED3DTEXOPCAPS_MULTIPLYADD                 |
                           WINED3DTEXOPCAPS_LERP                        |
                           WINED3DTEXOPCAPS_BUMPENVMAP;

    /* TODO: Implement WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE
    and WINED3DTEXOPCAPS_PREMODULATE */

    /* GL_ATI_fragment_shader only supports up to 6 textures, which was the limit on r200 cards
     * which this extension is exclusively focused on(later cards have GL_ARB_fragment_program).
     * If the current card has more than 8 fixed function textures in OpenGL's regular fixed
     * function pipeline then the ATI_fragment_shader backend imposes a stricter limit. This
     * shouldn't be too hard since Nvidia cards have a limit of 4 textures with the default ffp
     * pipeline, and almost all games are happy with that. We can however support up to 8
     * texture stages because we have a 2nd pass limit of 8 instructions, and per stage we use
     * only 1 instruction.
     *
     * The proper fix for this is not to use GL_ATI_fragment_shader on cards newer than the
     * r200 series and use an ARB or GLSL shader instead
     */
    if(caps->MaxSimultaneousTextures > 6) {
        WARN("OpenGL fixed function supports %d simultaneous textures,\n", caps->MaxSimultaneousTextures);
        WARN("but GL_ATI_fragment_shader limits this to 6\n");
        caps->MaxSimultaneousTextures = 6;
    }

    caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_TSSARGTEMP;
}

static void shader_atifs_generate_pshader(IWineD3DPixelShader *iface, SHADER_BUFFER *buffer) {
    ERR("Should not get here\n");
}

static void shader_atifs_generate_vshader(IWineD3DVertexShader *iface, SHADER_BUFFER *buffer) {
    arb_program_shader_backend.shader_generate_vshader(iface, buffer);
}

const shader_backend_t atifs_shader_backend = {
    shader_atifs_select,
    shader_atifs_select_depth_blt,
    shader_atifs_destroy_depth_blt,
    shader_atifs_load_constants,
    shader_atifs_cleanup,
    shader_atifs_color_correction,
    shader_atifs_destroy,
    shader_atifs_alloc,
    shader_atifs_free,
    shader_atifs_dirty_const,
    shader_atifs_generate_pshader,
    shader_atifs_generate_vshader,
    shader_atifs_get_caps,
    shader_atifs_load_init,
    ATIFSStateTable
};
