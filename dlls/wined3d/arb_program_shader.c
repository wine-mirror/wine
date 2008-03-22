/*
 * Pixel and vertex shaders implementation using ARB_vertex_program
 * and ARB_fragment_program GL extensions.
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2006 Jason Green
 * Copyright 2006 Henri Verbeet
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
WINE_DECLARE_DEBUG_CHANNEL(d3d_constants);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

#define GLINFO_LOCATION      (*gl_info)

/********************************************************
 * ARB_[vertex/fragment]_program helper functions follow
 ********************************************************/

/** 
 * Loads floating point constants into the currently set ARB_vertex/fragment_program.
 * When constant_list == NULL, it will load all the constants.
 *  
 * @target_type should be either GL_VERTEX_PROGRAM_ARB (for vertex shaders)
 *  or GL_FRAGMENT_PROGRAM_ARB (for pixel shaders)
 */
static unsigned int shader_arb_load_constantsF(IWineD3DBaseShaderImpl* This, WineD3D_GL_Info *gl_info, GLuint target_type,
        unsigned int max_constants, float* constants, char *dirty_consts) {
    local_constant* lconst;
    DWORD i, j;
    unsigned int ret;

    if (TRACE_ON(d3d_shader)) {
        for(i = 0; i < max_constants; i++) {
            if(!dirty_consts[i]) continue;
            TRACE_(d3d_constants)("Loading constants %i: %f, %f, %f, %f\n", i,
                        constants[i * 4 + 0], constants[i * 4 + 1],
                        constants[i * 4 + 2], constants[i * 4 + 3]);
        }
    }
    /* In 1.X pixel shaders constants are implicitly clamped in the range [-1;1] */
    if(target_type == GL_FRAGMENT_PROGRAM_ARB &&
       WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) == 1) {
        float lcl_const[4];
        for(i = 0; i < max_constants; i++) {
            if(!dirty_consts[i]) continue;
            dirty_consts[i] = 0;

            j = 4 * i;
            if(constants[j + 0] > 1.0) lcl_const[0] = 1.0;
            else if(constants[j + 0] < -1.0) lcl_const[0] = -1.0;
            else lcl_const[0] = constants[j + 0];

            if(constants[j + 1] > 1.0) lcl_const[1] = 1.0;
            else if(constants[j + 1] < -1.0) lcl_const[1] = -1.0;
            else lcl_const[1] = constants[j + 1];

            if(constants[j + 2] > 1.0) lcl_const[2] = 1.0;
            else if(constants[j + 2] < -1.0) lcl_const[2] = -1.0;
            else lcl_const[2] = constants[j + 2];

            if(constants[j + 3] > 1.0) lcl_const[3] = 1.0;
            else if(constants[j + 3] < -1.0) lcl_const[3] = -1.0;
            else lcl_const[3] = constants[j + 3];

            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, lcl_const));
        }
    } else {
        if(GL_SUPPORT(EXT_GPU_PROGRAM_PARAMETERS)) {
            /* TODO: Benchmark if we're better of with finding the dirty constants ourselves,
             * or just reloading *all* constants at once
             *
            GL_EXTCALL(glProgramEnvParameters4fvEXT(target_type, 0, max_constants, constants));
             */
            for(i = 0; i < max_constants; i++) {
                if(!dirty_consts[i]) continue;

                /* Find the next block of dirty constants */
                dirty_consts[i] = 0;
                j = i;
                for(i++; (i < max_constants) && dirty_consts[i]; i++) {
                    dirty_consts[i] = 0;
                }

                GL_EXTCALL(glProgramEnvParameters4fvEXT(target_type, j, i - j, constants + (j * 4)));
            }
        } else {
            for(i = 0; i < max_constants; i++) {
                if(dirty_consts[i]) {
                    dirty_consts[i] = 0;
                    GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, constants + (i * 4)));
                }
            }
        }
    }
    checkGLcall("glProgramEnvParameter4fvARB()");

    /* Load immediate constants */
    if(This->baseShader.load_local_constsF) {
        if (TRACE_ON(d3d_shader)) {
            LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
                GLfloat* values = (GLfloat*)lconst->value;
                TRACE_(d3d_constants)("Loading local constants %i: %f, %f, %f, %f\n", lconst->idx,
                        values[0], values[1], values[2], values[3]);
            }
        }
        /* Immediate constants are clamped for 1.X shaders at loading times */
        ret = 0;
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            dirty_consts[lconst->idx] = 1; /* Dirtify so the non-immediate constant overwrites it next time */
            ret = max(ret, lconst->idx);
            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, lconst->idx, (GLfloat*)lconst->value));
        }
        checkGLcall("glProgramEnvParameter4fvARB()");
        return ret; /* The loaded immediate constants need reloading for the next shader */
    } else {
        return 0; /* No constants are dirty now */
    }
}

/**
 * Loads the app-supplied constants into the currently set ARB_[vertex/fragment]_programs.
 * 
 * We only support float constants in ARB at the moment, so don't 
 * worry about the Integers or Booleans
 */
void shader_arb_load_constants(
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader) {
   
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) device; 
    IWineD3DStateBlockImpl* stateBlock = deviceImpl->stateBlock;
    WineD3D_GL_Info *gl_info = &deviceImpl->adapter->gl_info;
    unsigned char i;

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;

        /* Load DirectX 9 float constants for vertex shader */
        deviceImpl->highest_dirty_vs_const = shader_arb_load_constantsF(
                vshader, gl_info, GL_VERTEX_PROGRAM_ARB,
                deviceImpl->highest_dirty_vs_const,
                stateBlock->vertexShaderConstantF,
                deviceImpl->activeContext->vshader_const_dirty);

        /* Upload the position fixup */
        GL_EXTCALL(glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, ARB_SHADER_PRIVCONST_POS, deviceImpl->posFixup));
    }

    if (usePixelShader) {

        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;
        IWineD3DPixelShaderImpl *psi = (IWineD3DPixelShaderImpl *) pshader;

        /* Load DirectX 9 float constants for pixel shader */
        deviceImpl->highest_dirty_ps_const = shader_arb_load_constantsF(
                pshader, gl_info, GL_FRAGMENT_PROGRAM_ARB,
                deviceImpl->highest_dirty_ps_const,
                stateBlock->pixelShaderConstantF,
                deviceImpl->activeContext->pshader_const_dirty);

        for(i = 0; i < psi->numbumpenvmatconsts; i++) {
            /* The state manager takes care that this function is always called if the bump env matrix changes
             */
            float *data = (float *) &stateBlock->textureState[(int) psi->bumpenvmatconst[i].texunit][WINED3DTSS_BUMPENVMAT00];
            GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, psi->bumpenvmatconst[i].const_num, data));
            deviceImpl->activeContext->pshader_const_dirty[psi->bumpenvmatconst[i].const_num] = 1;

            if(psi->luminanceconst[i].const_num != -1) {
                /* WINED3DTSS_BUMPENVLSCALE and WINED3DTSS_BUMPENVLOFFSET are next to each other.
                 * point gl to the scale, and load 4 floats. x = scale, y = offset, z and w are junk, we
                 * don't care about them. The pointers are valid for sure because the stateblock is bigger.
                 * (they're WINED3DTSS_TEXTURETRANSFORMFLAGS and WINED3DTSS_ADDRESSW, so most likely 0 or NaN
                 */
                float *scale = (float *) &stateBlock->textureState[(int) psi->luminanceconst[i].texunit][WINED3DTSS_BUMPENVLSCALE];
                GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, psi->luminanceconst[i].const_num, scale));
                deviceImpl->activeContext->pshader_const_dirty[psi->luminanceconst[i].const_num] = 1;
            }
        }

        if(((IWineD3DPixelShaderImpl *) pshader)->srgb_enabled &&
           !((IWineD3DPixelShaderImpl *) pshader)->srgb_mode_hardcoded) {
            float comparison[4];
            float mul_low[4];

            if(stateBlock->renderState[WINED3DRS_SRGBWRITEENABLE]) {
                comparison[0] = srgb_cmp; comparison[1] = srgb_cmp;
                comparison[2] = srgb_cmp; comparison[3] = srgb_cmp;

                mul_low[0] = srgb_mul_low; mul_low[1] = srgb_mul_low;
                mul_low[2] = srgb_mul_low; mul_low[3] = srgb_mul_low;
            } else {
                comparison[0] = 1.0 / 0.0; comparison[1] = 1.0 / 0.0;
                comparison[2] = 1.0 / 0.0; comparison[3] = 1.0 / 0.0;

                mul_low[0] = 1.0; mul_low[1] = 1.0;
                mul_low[2] = 1.0; mul_low[3] = 1.0;
            }
            GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, psi->srgb_cmp_const, comparison));
            GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, psi->srgb_low_const, mul_low));
            checkGLcall("Load sRGB correction constants\n");
            deviceImpl->activeContext->pshader_const_dirty[psi->srgb_low_const] = 1;
            deviceImpl->activeContext->pshader_const_dirty[psi->srgb_cmp_const] = 1;

        }
    }
}

/* Generate the variable & register declarations for the ARB_vertex_program output target */
void shader_generate_arb_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer,
    WineD3D_GL_Info* gl_info) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;
    DWORD i, cur;
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    unsigned max_constantsF = min(This->baseShader.limits.constant_float, 
            (pshader ? GL_LIMITS(pshader_constantsF) : GL_LIMITS(vshader_constantsF)));
    UINT extra_constants_needed = 0;
    local_constant* lconst;

    /* Temporary Output register */
    shader_addline(buffer, "TEMP TMP_OUT;\n");

    for(i = 0; i < This->baseShader.limits.temporary; i++) {
        if (reg_maps->temporary[i])
            shader_addline(buffer, "TEMP R%u;\n", i);
    }

    for (i = 0; i < This->baseShader.limits.address; i++) {
        if (reg_maps->address[i])
            shader_addline(buffer, "ADDRESS A%d;\n", i);
    }

    for(i = 0; i < This->baseShader.limits.texcoord; i++) {
        if (reg_maps->texcoord[i])
            shader_addline(buffer,"TEMP T%u;\n", i);
    }

    /* Texture coordinate registers must be pre-loaded */
    for (i = 0; i < This->baseShader.limits.texcoord; i++) {
        if (reg_maps->texcoord[i])
            shader_addline(buffer, "MOV T%u, fragment.texcoord[%u];\n", i, i);
    }

    for(i = 0; i < (sizeof(reg_maps->bumpmat) / sizeof(reg_maps->bumpmat[0])); i++) {
        IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) This;
        if(!reg_maps->bumpmat[i]) continue;

        cur = ps->numbumpenvmatconsts;
        ps->bumpenvmatconst[cur].const_num = -1;
        ps->bumpenvmatconst[cur].texunit = i;
        ps->luminanceconst[cur].const_num = -1;
        ps->luminanceconst[cur].texunit = i;

        /* If the shader does not use all available constants, use the next free constant to load the bump mapping environment matrix from
         * the stateblock into the shader. If no constant is available don't load, texbem will then just sample the texture without applying
         * bump mapping.
         */
        if(max_constantsF + extra_constants_needed < GL_LIMITS(pshader_constantsF)) {
            ps->bumpenvmatconst[cur].const_num = max_constantsF + extra_constants_needed;
            shader_addline(buffer, "PARAM bumpenvmat%d = program.env[%d];\n",
                           i, ps->bumpenvmatconst[cur].const_num);
            extra_constants_needed++;

            if(reg_maps->luminanceparams && max_constantsF + extra_constants_needed < GL_LIMITS(pshader_constantsF)) {
                ((IWineD3DPixelShaderImpl *)This)->luminanceconst[cur].const_num = max_constantsF + extra_constants_needed;
                shader_addline(buffer, "PARAM luminance%d = program.env[%d];\n",
                               i, ps->luminanceconst[cur].const_num);
                extra_constants_needed++;
            } else if(reg_maps->luminanceparams) {
                FIXME("No free constant to load the luminance parameters\n");
            }
        } else {
            FIXME("No free constant found to load environemnt bump mapping matrix into the shader. texbem instruction will not apply bump mapping\n");
        }

        ps->numbumpenvmatconsts = cur + 1;
    }

    if(device->stateBlock->renderState[WINED3DRS_SRGBWRITEENABLE] && pshader) {
        IWineD3DPixelShaderImpl *ps_impl = (IWineD3DPixelShaderImpl *) This;
        /* If there are 2 constants left to use, use them to pass the sRGB correction values in. This way
         * srgb write correction can be turned on and off dynamically without recompilation. Otherwise
         * hardcode them. The drawback of hardcoding is that the shader needs recompilation to turn sRGB
         * off again
         */
        if(max_constantsF + extra_constants_needed + 1 < GL_LIMITS(pshader_constantsF) && FALSE) {
            /* The idea is that if srgb is enabled, then disabled, the constant loading code
             * can effectively disable sRGB correction by passing 1.0 and INF as the multiplication
             * and comparison constants. If it disables it that way, the shader won't be recompiled
             * and the code will stay in, so sRGB writing can be turned on again by setting the
             * constants from the spec
             */
            ps_impl->srgb_mode_hardcoded = 0;
            ps_impl->srgb_low_const = GL_LIMITS(pshader_constantsF) - extra_constants_needed;
            ps_impl->srgb_cmp_const = GL_LIMITS(pshader_constantsF) - extra_constants_needed - 1;
            shader_addline(buffer, "PARAM srgb_mul_low = program.env[%d];\n", ps_impl->srgb_low_const);
            shader_addline(buffer, "PARAM srgb_comparison = program.env[%d];\n", ps_impl->srgb_cmp_const);
        } else {
            shader_addline(buffer, "PARAM srgb_mul_low = {%f, %f, %f, 1.0};\n",
                           srgb_mul_low, srgb_mul_low, srgb_mul_low);
            shader_addline(buffer, "PARAM srgb_comparison =  {%f, %f, %f, %f};\n",
                           srgb_cmp, srgb_cmp, srgb_cmp, srgb_cmp);
            ps_impl->srgb_mode_hardcoded = 1;
        }
        /* These can be hardcoded, they do not cause any harm because no fragment will enter the high
         * path if the comparison value is set to INF
         */
        shader_addline(buffer, "PARAM srgb_pow =  {%f, %f, %f, 1.0};\n",
                       srgb_pow, srgb_pow, srgb_pow);
        shader_addline(buffer, "PARAM srgb_mul_hi =  {%f, %f, %f, 1.0};\n",
                       srgb_mul_high, srgb_mul_high, srgb_mul_high);
        shader_addline(buffer, "PARAM srgb_sub_hi =  {%f, %f, %f, 0.0};\n",
                       srgb_sub_high, srgb_sub_high, srgb_sub_high);
        ps_impl->srgb_enabled = 1;
    } else if(pshader) {
        IWineD3DPixelShaderImpl *ps_impl = (IWineD3DPixelShaderImpl *) This;

        /* Do not write any srgb fixup into the shader to save shader size and processing time.
         * As a consequence, we can't toggle srgb write on without recompilation
         */
        ps_impl->srgb_enabled = 0;
        ps_impl->srgb_mode_hardcoded = 1;
    }

    /* Hardcodable local constants */
    if(!This->baseShader.load_local_constsF) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            float *value = (float *) lconst->value;
            shader_addline(buffer, "PARAM C%u = {%f, %f, %f, %f};\n", lconst->idx,
                           value[0], value[1], value[2], value[3]);
        }
    }

    /* we use the array-based constants array if the local constants are marked for loading,
     * because then we use indirect addressing, or when the local constant list is empty,
     * because then we don't know if we're using indirect addressing or not. If we're hardcoding
     * local constants do not declare the loaded constants as an array because ARB compilers usually
     * do not optimize unused constants away
     */
    if(This->baseShader.load_local_constsF || list_empty(&This->baseShader.constantsF)) {
        /* Need to PARAM the environment parameters (constants) so we can use relative addressing */
        shader_addline(buffer, "PARAM C[%d] = { program.env[0..%d] };\n",
                    max_constantsF, max_constantsF - 1);
    } else {
        for(i = 0; i < max_constantsF; i++) {
            if(!shader_constant_is_local(This, i)) {
                shader_addline(buffer, "PARAM C%d = program.env[%d];\n",i, i);
            }
        }
    }
}

static const char * const shift_tab[] = {
    "dummy",     /*  0 (none) */
    "coefmul.x", /*  1 (x2)   */
    "coefmul.y", /*  2 (x4)   */
    "coefmul.z", /*  3 (x8)   */
    "coefmul.w", /*  4 (x16)  */
    "dummy",     /*  5 (x32)  */
    "dummy",     /*  6 (x64)  */
    "dummy",     /*  7 (x128) */
    "dummy",     /*  8 (d256) */
    "dummy",     /*  9 (d128) */
    "dummy",     /* 10 (d64)  */
    "dummy",     /* 11 (d32)  */
    "coefdiv.w", /* 12 (d16)  */
    "coefdiv.z", /* 13 (d8)   */
    "coefdiv.y", /* 14 (d4)   */
    "coefdiv.x"  /* 15 (d2)   */
};

static void shader_arb_get_write_mask(SHADER_OPCODE_ARG* arg, const DWORD param, char *write_mask) {
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) arg->shader;
    char *ptr = write_mask;
    char vshader = shader_is_vshader_version(This->baseShader.hex_version);

    if(vshader && shader_get_regtype(param) == WINED3DSPR_ADDR) {
        *ptr++ = '.';
        *ptr++ = 'x';
    } else if ((param & WINED3DSP_WRITEMASK_ALL) != WINED3DSP_WRITEMASK_ALL) {
        *ptr++ = '.';
        if (param & WINED3DSP_WRITEMASK_0) *ptr++ = 'x';
        if (param & WINED3DSP_WRITEMASK_1) *ptr++ = 'y';
        if (param & WINED3DSP_WRITEMASK_2) *ptr++ = 'z';
        if (param & WINED3DSP_WRITEMASK_3) *ptr++ = 'w';
    }

    *ptr = '\0';
}

static void shader_arb_get_swizzle(const DWORD param, BOOL fixup, char *swizzle_str) {
    /* For registers of type WINED3DDECLTYPE_D3DCOLOR, data is stored as "bgra",
     * but addressed as "rgba". To fix this we need to swap the register's x
     * and z components. */
    const char *swizzle_chars = fixup ? "zyxw" : "xyzw";
    char *ptr = swizzle_str;

    /* swizzle bits fields: wwzzyyxx */
    DWORD swizzle = (param & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;

    /* If the swizzle is the default swizzle (ie, "xyzw"), we don't need to
     * generate a swizzle string. Unless we need to our own swizzling. */
    if ((WINED3DSP_NOSWIZZLE >> WINED3DSP_SWIZZLE_SHIFT) != swizzle || fixup) {
        *ptr++ = '.';
        if (swizzle_x == swizzle_y && swizzle_x == swizzle_z && swizzle_x == swizzle_w) {
            *ptr++ = swizzle_chars[swizzle_x];
        } else {
            *ptr++ = swizzle_chars[swizzle_x];
            *ptr++ = swizzle_chars[swizzle_y];
            *ptr++ = swizzle_chars[swizzle_z];
            *ptr++ = swizzle_chars[swizzle_w];
        }
    }

    *ptr = '\0';
}

static void pshader_get_register_name(IWineD3DBaseShader* iface,
    const DWORD param, char* regstr) {

    DWORD reg = param & WINED3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) iface;

    switch (regtype) {
    case WINED3DSPR_TEMP:
        sprintf(regstr, "R%u", reg);
    break;
    case WINED3DSPR_INPUT:
        if (reg==0) {
            strcpy(regstr, "fragment.color.primary");
        } else {
            strcpy(regstr, "fragment.color.secondary");
        }
    break;
    case WINED3DSPR_CONST:
        if(This->baseShader.load_local_constsF || list_empty(&This->baseShader.constantsF)) {
            sprintf(regstr, "C[%u]", reg);
        } else {
            sprintf(regstr, "C%u", reg);
        }
    break;
    case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
        sprintf(regstr,"T%u", reg);
    break;
    case WINED3DSPR_COLOROUT:
        if (reg == 0)
            sprintf(regstr, "TMP_COLOR");
        else {
            /* TODO: See GL_ARB_draw_buffers */
            FIXME("Unsupported write to render target %u\n", reg);
            sprintf(regstr, "unsupported_register");
        }
    break;
    case WINED3DSPR_DEPTHOUT:
        sprintf(regstr, "result.depth");
    break;
    case WINED3DSPR_ATTROUT:
        sprintf(regstr, "oD[%u]", reg);
    break;
    case WINED3DSPR_TEXCRDOUT:
        sprintf(regstr, "oT[%u]", reg);
    break;
    default:
        FIXME("Unhandled register name Type(%d)\n", regtype);
        sprintf(regstr, "unrecognized_register");
    break;
    }
}

/* TODO: merge with pixel shader */
static void vshader_program_add_param(SHADER_OPCODE_ARG *arg, const DWORD param, BOOL is_input, char *hwLine) {

  IWineD3DVertexShaderImpl* This = (IWineD3DVertexShaderImpl*) arg->shader;

  /* oPos, oFog and oPts in D3D */
  static const char * const hwrastout_reg_names[] = { "TMP_OUT", "result.fogcoord", "result.pointsize" };

  DWORD reg = param & WINED3DSP_REGNUM_MASK;
  DWORD regtype = shader_get_regtype(param);
  char  tmpReg[255];
  BOOL is_color = FALSE;

  if ((param & WINED3DSP_SRCMOD_MASK) == WINED3DSPSM_NEG) {
      strcat(hwLine, " -");
  } else {
      strcat(hwLine, " ");
  }

  switch (regtype) {
  case WINED3DSPR_TEMP:
    sprintf(tmpReg, "R%u", reg);
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_INPUT:

    if (vshader_input_is_color((IWineD3DVertexShader*) This, reg))
        is_color = TRUE;

    sprintf(tmpReg, "vertex.attrib[%u]", reg);
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_CONST:
      if(param & WINED3DSHADER_ADDRMODE_RELATIVE) {
          if(reg >= This->rel_offset) {
              sprintf(tmpReg, "C[A0.x + %u]", reg - This->rel_offset);
          } else {
              sprintf(tmpReg, "C[A0.x - %u]", -reg + This->rel_offset);
          }
      } else {
          if(This->baseShader.load_local_constsF || list_empty(&This->baseShader.constantsF)) {
              sprintf(tmpReg, "C[%u]", reg);
          } else {
              sprintf(tmpReg, "C%u", reg);
          }
      }
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_ADDR: /*case D3DSPR_TEXTURE:*/
    sprintf(tmpReg, "A%u", reg);
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_RASTOUT:
    sprintf(tmpReg, "%s", hwrastout_reg_names[reg]);
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_ATTROUT:
    if (reg==0) {
       strcat(hwLine, "result.color.primary");
    } else {
       strcat(hwLine, "result.color.secondary");
    }
    break;
  case WINED3DSPR_TEXCRDOUT:
    sprintf(tmpReg, "result.texcoord[%u]", reg);
    strcat(hwLine, tmpReg);
    break;
  default:
    FIXME("Unknown reg type %d %d\n", regtype, reg);
    strcat(hwLine, "unrecognized_register");
    break;
  }

  if (!is_input) {
    char write_mask[6];
    shader_arb_get_write_mask(arg, param, write_mask);
    strcat(hwLine, write_mask);
  } else {
    char swizzle[6];
    shader_arb_get_swizzle(param, is_color, swizzle);
    strcat(hwLine, swizzle);
  }
}

static void shader_hw_sample(SHADER_OPCODE_ARG* arg, DWORD sampler_idx, const char *dst_str, const char *coord_reg, BOOL projected, BOOL bias) {
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    const char *tex_type;

    switch(sampler_type) {
        case WINED3DSTT_1D:
            tex_type = "1D";
            break;

        case WINED3DSTT_2D:
        {
            IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) arg->shader;
            IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;
            if(device->stateBlock->textures[sampler_idx] &&
               IWineD3DBaseTexture_GetTextureDimensions(device->stateBlock->textures[sampler_idx]) == GL_TEXTURE_RECTANGLE_ARB) {
                tex_type = "RECT";
            } else {
                tex_type = "2D";
            }
            break;
        }

        case WINED3DSTT_VOLUME:
            tex_type = "3D";
            break;

        case WINED3DSTT_CUBE:
            tex_type = "CUBE";
            break;

        default:
            ERR("Unexpected texture type %d\n", sampler_type);
            tex_type = "";
    }

    if (bias) {
        /* Shouldn't be possible, but let's check for it */
        if(projected) FIXME("Biased and Projected texture sampling\n");
        /* TXB takes the 4th component of the source vector automatically, as d3d. Nothing more to do */
        shader_addline(buffer, "TXB %s, %s, texture[%u], %s;\n", dst_str, coord_reg, sampler_idx, tex_type);
    } else if (projected) {
        shader_addline(buffer, "TXP %s, %s, texture[%u], %s;\n", dst_str, coord_reg, sampler_idx, tex_type);
    } else {
        shader_addline(buffer, "TEX %s, %s, texture[%u], %s;\n", dst_str, coord_reg, sampler_idx, tex_type);
    }
}

static void shader_arb_color_correction(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) shader->baseShader.device;
    WineD3D_GL_Info *gl_info = &deviceImpl->adapter->gl_info;
    WINED3DFORMAT fmt;
    WINED3DFORMAT conversion_group;
    IWineD3DBaseTextureImpl *texture;
    UINT i;
    BOOL recorded = FALSE;
    DWORD sampler_idx;
    DWORD hex_version = shader->baseShader.hex_version;
    char reg[256];
    char writemask[6];

    switch(arg->opcode->opcode) {
        case WINED3DSIO_TEX:
            if (hex_version < WINED3DPS_VERSION(2,0)) {
                sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
            } else {
                sampler_idx = arg->src[1] & WINED3DSP_REGNUM_MASK;
            }
            break;

        case WINED3DSIO_TEXLDL:
            FIXME("Add color fixup for vertex texture WINED3DSIO_TEXLDL\n");
            return;

        case WINED3DSIO_TEXDP3TEX:
        case WINED3DSIO_TEXM3x3TEX:
        case WINED3DSIO_TEXM3x3SPEC:
        case WINED3DSIO_TEXM3x3VSPEC:
        case WINED3DSIO_TEXBEM:
        case WINED3DSIO_TEXREG2AR:
        case WINED3DSIO_TEXREG2GB:
        case WINED3DSIO_TEXREG2RGB:
            sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
            break;

        default:
            /* Not a texture sampling instruction, nothing to do */
            return;
    };

    texture = (IWineD3DBaseTextureImpl *) deviceImpl->stateBlock->textures[sampler_idx];
    if(texture) {
        fmt = texture->resource.format;
        conversion_group = texture->baseTexture.shader_conversion_group;
    } else {
        fmt = WINED3DFMT_UNKNOWN;
        conversion_group = WINED3DFMT_UNKNOWN;
    }

    /* before doing anything, record the sampler with the format in the format conversion list,
     * but check if it's not there already
     */
    for(i = 0; i < shader->baseShader.num_sampled_samplers; i++) {
        if(shader->baseShader.sampled_samplers[i] == sampler_idx) {
            recorded = TRUE;
        }
    }
    if(!recorded) {
        shader->baseShader.sampled_samplers[shader->baseShader.num_sampled_samplers] = sampler_idx;
        shader->baseShader.num_sampled_samplers++;
        shader->baseShader.sampled_format[sampler_idx] = conversion_group;
    }

    pshader_get_register_name(arg->shader, arg->dst, reg);
    shader_arb_get_write_mask(arg, arg->dst, writemask);
    if(strlen(writemask) == 0) strcpy(writemask, ".xyzw");

    switch(fmt) {
        case WINED3DFMT_V8U8:
        case WINED3DFMT_V16U16:
            if(GL_SUPPORT(NV_TEXTURE_SHADER) ||
               (GL_SUPPORT(ATI_ENVMAP_BUMPMAP) && fmt == WINED3DFMT_V8U8)) {
#if 0
                /* The 3rd channel returns 1.0 in d3d, but 0.0 in gl. Fix this while we're at it :-)
                 * disabled until an application that needs it is found because it causes unneeded
                 * shader recompilation in some game
                 */
                if(strlen(writemask) >= 4) {
                    shader_addline(arg->buffer, "MOV %s.%c, one.z;\n", reg, writemask[3]);
                }
#endif
            } else {
                /* Correct the sign, but leave the blue as it is - it was loaded correctly already
                 * ARB shaders are a bit picky wrt writemasks and swizzles. If we're free to scale
                 * all registers, do so, this saves an instruction.
                 */
                if(strlen(writemask) >= 5) {
                    shader_addline(arg->buffer, "MAD %s, %s, coefmul.x, -one;\n", reg, reg);
                } else if(strlen(writemask) >= 3) {
                    shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                   reg, writemask[1],
                                   reg, writemask[1]);
                    shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                   reg, writemask[2],
                                   reg, writemask[2]);
                } else if(strlen(writemask) == 2) {
                    shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n", reg, writemask[1],
                                   reg, writemask[1]);
                }
            }
            break;

        case WINED3DFMT_X8L8V8U8:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* Red and blue are the signed channels, fix them up; Blue(=L) is correct already,
                 * and a(X) is always 1.0. Cannot do a full conversion due to L(blue)
                 */
                if(strlen(writemask) >= 3) {
                    shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                   reg, writemask[1],
                                   reg, writemask[1]);
                    shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                   reg, writemask[2],
                                   reg, writemask[2]);
                } else if(strlen(writemask) == 2) {
                    shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                   reg, writemask[1],
                                   reg, writemask[1]);
                }
            }
            break;

        case WINED3DFMT_L6V5U5:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                if(strlen(writemask) >= 4) {
                    /* Swap y and z (U and L), and do a sign conversion on x and the new y(V and U) */
                    shader_addline(arg->buffer, "MOV TMP.g, %s.%c;\n",
                                   reg, writemask[2]);
                    shader_addline(arg->buffer, "MAD %s.%c%c, %s.%c%c, coefmul.x, -one;\n",
                                   reg, writemask[1], writemask[1],
                                   reg, writemask[1], writemask[3]);
                    shader_addline(arg->buffer, "MOV %s.%c, TMP.g;\n", reg,
                                   writemask[3]);
                } else if(strlen(writemask) == 3) {
                    /* This is bad: We have VL, but we need VU */
                    FIXME("2 components sampled from a converted L6V5U5 texture\n");
                } else {
                    shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                   reg, writemask[1],
                                   reg, writemask[1]);
                }
            }
            break;

        case WINED3DFMT_Q8W8V8U8:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* Correct the sign in all channels */
                switch(strlen(writemask)) {
                    case 4:
                        shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                       reg, writemask[3],
                                       reg, writemask[3]);
                        /* drop through */
                    case 3:
                        shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                       reg, writemask[2],
                                       reg, writemask[2]);
                        /* drop through */
                    case 2:
                        shader_addline(arg->buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                       reg, writemask[1],
                                       reg, writemask[1]);
                        break;

                        /* Should not occur, since it's at minimum '.' and a letter */
                    case 1:
                        ERR("Unexpected writemask: \"%s\"\n", writemask);
                        break;

                    case 5:
                    default:
                        shader_addline(arg->buffer, "MAD %s, %s, coefmul.x, -one;\n", reg, reg);
                }
            }
            break;

            /* stupid compiler */
        default:
            break;
    }
}


static void pshader_gen_input_modifier_line (
    IWineD3DBaseShader *iface,
    SHADER_BUFFER* buffer,
    const DWORD instr,
    int tmpreg,
    char *outregstr) {

    /* Generate a line that does the input modifier computation and return the input register to use */
    char regstr[256];
    char swzstr[20];
    int insert_line;

    /* Assume a new line will be added */
    insert_line = 1;

    /* Get register name */
    pshader_get_register_name(iface, instr, regstr);
    shader_arb_get_swizzle(instr, FALSE, swzstr);

    switch (instr & WINED3DSP_SRCMOD_MASK) {
    case WINED3DSPSM_NONE:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = 0;
        break;
    case WINED3DSPSM_NEG:
        sprintf(outregstr, "-%s%s", regstr, swzstr);
        insert_line = 0;
        break;
    case WINED3DSPSM_BIAS:
        shader_addline(buffer, "ADD T%c, %s, -coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_BIASNEG:
        shader_addline(buffer, "ADD T%c, -%s, coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_SIGN:
        shader_addline(buffer, "MAD T%c, %s, coefmul.x, -one.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_SIGNNEG:
        shader_addline(buffer, "MAD T%c, %s, -coefmul.x, one.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_COMP:
        shader_addline(buffer, "SUB T%c, one.x, %s;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_X2:
        shader_addline(buffer, "ADD T%c, %s, %s;\n", 'A' + tmpreg, regstr, regstr);
        break;
    case WINED3DSPSM_X2NEG:
        shader_addline(buffer, "ADD T%c, -%s, -%s;\n", 'A' + tmpreg, regstr, regstr);
        break;
    case WINED3DSPSM_DZ:
        shader_addline(buffer, "RCP T%c, %s.z;\n", 'A' + tmpreg, regstr);
        shader_addline(buffer, "MUL T%c, %s, T%c;\n", 'A' + tmpreg, regstr, 'A' + tmpreg);
        break;
    case WINED3DSPSM_DW:
        shader_addline(buffer, "RCP T%c, %s.w;\n", 'A' + tmpreg, regstr);
        shader_addline(buffer, "MUL T%c, %s, T%c;\n", 'A' + tmpreg, regstr, 'A' + tmpreg);
        break;
    default:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = 0;
    }

    /* Return modified or original register, with swizzle */
    if (insert_line)
        sprintf(outregstr, "T%c%s", 'A' + tmpreg, swzstr);
}

static inline void pshader_gen_output_modifier_line(
    SHADER_BUFFER* buffer,
    int saturate,
    char *write_mask,
    int shift,
    char *regstr) {

    /* Generate a line that does the output modifier computation */
    shader_addline(buffer, "MUL%s %s%s, %s, %s;\n", saturate ? "_SAT" : "",
        regstr, write_mask, regstr, shift_tab[shift]);
}

void pshader_hw_bem(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;

    SHADER_BUFFER* buffer = arg->buffer;
    char dst_name[50];
    char src_name[2][50];
    char dst_wmask[20];
    DWORD sampler_code = arg->dst & WINED3DSP_REGNUM_MASK;
    BOOL has_bumpmat = FALSE;
    int i;

    for(i = 0; i < This->numbumpenvmatconsts; i++) {
        if(This->bumpenvmatconst[i].const_num != -1 && This->bumpenvmatconst[i].texunit == sampler_code) {
            has_bumpmat = TRUE;
            break;
        }
    }

    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);
    strcat(dst_name, dst_wmask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[1], 1, src_name[1]);

    if(has_bumpmat) {
        /* Sampling the perturbation map in Tsrc was done already, including the signedness correction if needed */
        shader_addline(buffer, "SWZ TMP2, bumpenvmat%d, x, z, 0, 0;\n", sampler_code);
        shader_addline(buffer, "DP3 TMP.r, TMP2, %s;\n", src_name[1]);
        shader_addline(buffer, "SWZ TMP2, bumpenvmat%d, y, w, 0, 0;\n", sampler_code);
        shader_addline(buffer, "DP3 TMP.g, TMP2, %s;\n", src_name[1]);

        shader_addline(buffer, "ADD %s, %s, TMP;\n", dst_name, src_name[0]);
    } else {
        shader_addline(buffer, "MOV %s, %s;\n", dst_name, src_name[0]);
    }
}

void pshader_hw_cnd(SHADER_OPCODE_ARG* arg) {

    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;

    /* FIXME: support output modifiers */

    /* Handle output register */
    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    /* Generate input register names (with modifiers) */
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[1], 1, src_name[1]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[2], 2, src_name[2]);

    /* The coissue flag changes the semantic of the cnd instruction in <= 1.3 shaders */
    if (shader->baseShader.hex_version <= WINED3DPS_VERSION(1, 3) &&
        arg->opcode_token & WINED3DSI_COISSUE) {
        shader_addline(buffer, "MOV%s %s%s, %s;\n", sat ? "_SAT" : "", dst_name, dst_wmask, src_name[1]);
    } else {
        shader_addline(buffer, "ADD TMP, -%s, coefdiv.x;\n", src_name[0]);
        shader_addline(buffer, "CMP%s %s%s, TMP, %s, %s;\n",
                                sat ? "_SAT" : "", dst_name, dst_wmask, src_name[1], src_name[2]);
    }
    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);
}

void pshader_hw_cmp(SHADER_OPCODE_ARG* arg) {

    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;

    /* FIXME: support output modifiers */

    /* Handle output register */
    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    /* Generate input register names (with modifiers) */
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[1], 1, src_name[1]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[2], 2, src_name[2]);

    shader_addline(buffer, "CMP%s %s%s, %s, %s, %s;\n", sat ? "_SAT" : "", dst_name, dst_wmask,
                   src_name[0], src_name[2], src_name[1]);

    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);
}

/** Process the WINED3DSIO_DP2ADD instruction in ARB.
 * dst = dot2(src0, src1) + src2 */
void pshader_hw_dp2add(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;

    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[1], 1, src_name[1]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[2], 2, src_name[2]);

    /* Emulate a DP2 with a DP3 and 0.0 */
    shader_addline(buffer, "MOV TMP, %s;\n", src_name[0]);
    shader_addline(buffer, "MOV TMP.z, 0.0;\n");
    shader_addline(buffer, "DP3 TMP2, TMP, %s;\n", src_name[1]);
    shader_addline(buffer, "ADD%s %s%s, TMP2, %s;\n", sat ? "_SAT" : "", dst_name, dst_wmask, src_name[2]);

    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);
}

/* Map the opcode 1-to-1 to the GL code */
void pshader_hw_map2gl(SHADER_OPCODE_ARG* arg) {

     CONST SHADER_OPCODE* curOpcode = arg->opcode;
     SHADER_BUFFER* buffer = arg->buffer;
     DWORD dst = arg->dst;
     DWORD* src = arg->src;

     unsigned int i;
     char tmpLine[256];

     /* Output token related */
     char output_rname[256];
     char output_wmask[20];
     BOOL saturate = FALSE;
     BOOL centroid = FALSE;
     BOOL partialprecision = FALSE;
     DWORD shift;

     strcpy(tmpLine, curOpcode->glname);

     /* Process modifiers */
     if (0 != (dst & WINED3DSP_DSTMOD_MASK)) {
         DWORD mask = dst & WINED3DSP_DSTMOD_MASK;

         saturate = mask & WINED3DSPDM_SATURATE;
         centroid = mask & WINED3DSPDM_MSAMPCENTROID;
         partialprecision = mask & WINED3DSPDM_PARTIALPRECISION;
         mask &= ~(WINED3DSPDM_MSAMPCENTROID | WINED3DSPDM_PARTIALPRECISION | WINED3DSPDM_SATURATE);
         if (mask)
            FIXME("Unrecognized modifier(%#x)\n", mask >> WINED3DSP_DSTMOD_SHIFT);

         if (centroid)
             FIXME("Unhandled modifier(%#x)\n", mask >> WINED3DSP_DSTMOD_SHIFT);
     }
     shift = (dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;

      /* Generate input and output registers */
      if (curOpcode->num_params > 0) {
          char operands[4][100];

          /* Generate input register names (with modifiers) */
          for (i = 1; i < curOpcode->num_params; ++i)
              pshader_gen_input_modifier_line(arg->shader, buffer, src[i-1], i-1, operands[i]);

          /* Handle output register */
          pshader_get_register_name(arg->shader, dst, output_rname);
          strcpy(operands[0], output_rname);
          shader_arb_get_write_mask(arg, dst, output_wmask);
          strcat(operands[0], output_wmask);

          if (saturate && (shift == 0))
             strcat(tmpLine, "_SAT");
          strcat(tmpLine, " ");
          strcat(tmpLine, operands[0]);
          for (i = 1; i < curOpcode->num_params; i++) {
              strcat(tmpLine, ", ");
              strcat(tmpLine, operands[i]);
          }
          strcat(tmpLine,";\n");
          shader_addline(buffer, tmpLine);

          /* A shift requires another line. */
          if (shift != 0)
              pshader_gen_output_modifier_line(buffer, saturate, output_wmask, shift, output_rname);
      }
}

void pshader_hw_texkill(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD hex_version = This->baseShader.hex_version;
    SHADER_BUFFER* buffer = arg->buffer;
    char reg_dest[40];

    /* No swizzles are allowed in d3d's texkill. PS 1.x ignores the 4th component as documented,
     * but >= 2.0 honors it(undocumented, but tested by the d3d9 testsuit)
     */
    pshader_get_register_name(arg->shader, arg->dst, reg_dest);

    if(hex_version >= WINED3DPS_VERSION(2,0)) {
        /* The arb backend doesn't claim ps 2.0 support, but try to eat what the app feeds to us */
        shader_addline(buffer, "KIL %s;\n", reg_dest);
    } else {
        /* ARB fp doesn't like swizzles on the parameter of the KIL instruction. To mask the 4th component,
         * copy the register into our general purpose TMP variable, overwrite .w and pass TMP to KIL
         */
        shader_addline(buffer, "MOV TMP, %s;\n", reg_dest);
        shader_addline(buffer, "MOV TMP.w, one.w;\n");
        shader_addline(buffer, "KIL TMP;\n");
    }
}

void pshader_hw_tex(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;

    DWORD dst = arg->dst;
    DWORD* src = arg->src;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;
    BOOL projected = FALSE, bias = FALSE;

    char reg_dest[40];
    char reg_coord[40];
    DWORD reg_dest_code;
    DWORD reg_sampler_code;

    /* All versions have a destination register */
    reg_dest_code = dst & WINED3DSP_REGNUM_MASK;
    pshader_get_register_name(arg->shader, dst, reg_dest);

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
   if (hex_version < WINED3DPS_VERSION(1,4))
      strcpy(reg_coord, reg_dest);
   else
      pshader_gen_input_modifier_line(arg->shader, buffer, src[0], 0, reg_coord);

  /* 1.0-1.4: Use destination register number as texture code.
     2.0+: Use provided sampler number as texure code. */
  if (hex_version < WINED3DPS_VERSION(2,0))
     reg_sampler_code = reg_dest_code;
  else
     reg_sampler_code = src[1] & WINED3DSP_REGNUM_MASK;

  /* projection flag:
   * 1.1, 1.2, 1.3: Use WINED3DTSS_TEXTURETRANSFORMFLAGS
   * 1.4: Use WINED3DSPSM_DZ or WINED3DSPSM_DW on src[0]
   * 2.0+: Use WINED3DSI_TEXLD_PROJECT on the opcode
   */
  if(hex_version < WINED3DPS_VERSION(1,4)) {
      DWORD flags = 0;
      if(reg_sampler_code < MAX_TEXTURES) {
        flags = deviceImpl->stateBlock->textureState[reg_sampler_code][WINED3DTSS_TEXTURETRANSFORMFLAGS];
      }
      if (flags & WINED3DTTFF_PROJECTED) {
          projected = TRUE;
      }
  } else if(hex_version < WINED3DPS_VERSION(2,0)) {
      DWORD src_mod = arg->src[0] & WINED3DSP_SRCMOD_MASK;
      if (src_mod == WINED3DSPSM_DZ) {
          projected = TRUE;
      } else if(src_mod == WINED3DSPSM_DW) {
          projected = TRUE;
      }
  } else {
      if(arg->opcode_token & WINED3DSI_TEXLD_PROJECT) {
          projected = TRUE;
      }
      if(arg->opcode_token & WINED3DSI_TEXLD_BIAS) {
          bias = TRUE;
      }
  }
  shader_hw_sample(arg, reg_sampler_code, reg_dest, reg_coord, projected, bias);
}

void pshader_hw_texcoord(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD dst = arg->dst;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;

    char tmp[20];
    shader_arb_get_write_mask(arg, dst, tmp);
    if (hex_version != WINED3DPS_VERSION(1,4)) {
        DWORD reg = dst & WINED3DSP_REGNUM_MASK;
        shader_addline(buffer, "MOV_SAT T%u%s, fragment.texcoord[%u];\n", reg, tmp, reg);
    } else {
        DWORD reg1 = dst & WINED3DSP_REGNUM_MASK;
        char reg_src[40];

        pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, reg_src);
        shader_addline(buffer, "MOV R%u%s, %s;\n", reg1, tmp, reg_src);
   }
}

void pshader_hw_texreg2ar(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;
     IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
     IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
     DWORD flags;

     DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
     char dst_str[8];
     char src_str[50];

     sprintf(dst_str, "T%u", reg1);
     pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_str);
     shader_addline(buffer, "MOV TMP.r, %s.a;\n", src_str);
     shader_addline(buffer, "MOV TMP.g, %s.r;\n", src_str);
     flags = reg1 < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg1][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
     shader_hw_sample(arg, reg1, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
}

void pshader_hw_texreg2gb(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;
     IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
     IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
     DWORD flags;

     DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
     char dst_str[8];
     char src_str[50];

     sprintf(dst_str, "T%u", reg1);
     pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_str);
     shader_addline(buffer, "MOV TMP.r, %s.g;\n", src_str);
     shader_addline(buffer, "MOV TMP.g, %s.b;\n", src_str);
     flags = reg1 < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg1][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
     shader_hw_sample(arg, reg1, dst_str, "TMP", FALSE, FALSE);
}

void pshader_hw_texreg2rgb(SHADER_OPCODE_ARG* arg) {

    SHADER_BUFFER* buffer = arg->buffer;
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
    char dst_str[8];
    char src_str[50];

    sprintf(dst_str, "T%u", reg1);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_str);
    flags = reg1 < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg1][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg1, dst_str, src_str, FALSE, FALSE);
}

void pshader_hw_texbem(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    BOOL has_bumpmat = FALSE;
    BOOL has_luminance = FALSE;
    int i;

    DWORD dst = arg->dst;
    DWORD src = arg->src[0] & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;

    char reg_coord[40];
    DWORD reg_dest_code;

    /* All versions have a destination register */
    reg_dest_code = dst & WINED3DSP_REGNUM_MASK;
    /* Can directly use the name because texbem is only valid for <= 1.3 shaders */
    pshader_get_register_name(arg->shader, dst, reg_coord);

    for(i = 0; i < This->numbumpenvmatconsts; i++) {
        if(This->bumpenvmatconst[i].const_num != -1 && reg_dest_code == This->bumpenvmatconst[i].texunit) {
            has_bumpmat = TRUE;
            break;
        }
    }
    for(i = 0; i < This->numbumpenvmatconsts; i++) {
        if(This->luminanceconst[i].const_num != -1 && reg_dest_code == This->luminanceconst[i].texunit) {
            has_luminance = TRUE;
            break;
        }
    }

    if(has_bumpmat) {
        /* Sampling the perturbation map in Tsrc was done already, including the signedness correction if needed */

        shader_addline(buffer, "SWZ TMP2, bumpenvmat%d, x, z, 0, 0;\n", reg_dest_code);
        shader_addline(buffer, "DP3 TMP.r, TMP2, T%u;\n", src);
        shader_addline(buffer, "SWZ TMP2, bumpenvmat%d, y, w, 0, 0;\n", reg_dest_code);
        shader_addline(buffer, "DP3 TMP.g, TMP2, T%u;\n", src);

        /* with projective textures, texbem only divides the static texture coord, not the displacement,
         * so we can't let the GL handle this.
         */
        if (((IWineD3DDeviceImpl*) This->baseShader.device)->stateBlock->textureState[reg_dest_code][WINED3DTSS_TEXTURETRANSFORMFLAGS]
              & WINED3DTTFF_PROJECTED) {
            shader_addline(buffer, "RCP TMP2.a, %s.a;\n", reg_coord);
            shader_addline(buffer, "MUL TMP2.rg, %s, TMP2.a;\n", reg_coord);
            shader_addline(buffer, "ADD TMP.rg, TMP, TMP2;\n");
        } else {
            shader_addline(buffer, "ADD TMP.rg, TMP, %s;\n", reg_coord);
        }

        shader_hw_sample(arg, reg_dest_code, reg_coord, "TMP", FALSE, FALSE);

        if(arg->opcode->opcode == WINED3DSIO_TEXBEML && has_luminance) {
            shader_addline(buffer, "MAD TMP, T%u.z, luminance%d.x, luminance%d.y;\n",
                           src, reg_dest_code, reg_dest_code);
            shader_addline(buffer, "MUL %s, %s, TMP;\n", reg_coord, reg_coord);
        }

    } else {
        DWORD tf;
        if(reg_dest_code < MAX_TEXTURES) {
            tf = ((IWineD3DDeviceImpl*) This->baseShader.device)->stateBlock->textureState[reg_dest_code][WINED3DTSS_TEXTURETRANSFORMFLAGS];
        } else {
            tf = 0;
        }
        /* Without a bump matrix loaded, just sample with the unmodified coordinates */
        shader_hw_sample(arg, reg_dest_code, reg_coord, reg_coord, tf & WINED3DTTFF_PROJECTED, FALSE);
    }
}

void pshader_hw_texm3x2pad(SHADER_OPCODE_ARG* arg) {

    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.x, T%u, %s;\n", reg, src0_name);
}

void pshader_hw_texm3x2tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_str[8];
    char src0_name[50];

    sprintf(dst_str, "T%u", reg);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.y, T%u, %s;\n", reg, src0_name);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
}

void pshader_hw_texm3x3pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.%c, T%u, %s;\n", 'x' + current_state->current_row, reg, src0_name);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

void pshader_hw_texm3x3tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char dst_str[8];
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
    current_state->current_row = 0;
}

void pshader_hw_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char dst_str[8];
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "MOV TMP2.x, fragment.texcoord[%u].w;\n", current_state->texcoord_w[0]);
    shader_addline(buffer, "MOV TMP2.y, fragment.texcoord[%u].w;\n", current_state->texcoord_w[1]);
    shader_addline(buffer, "MOV TMP2.z, fragment.texcoord[%u].w;\n", reg);

    /* Calculate reflection vector
     */
    shader_addline(buffer, "DP3 TMP.w, TMP, TMP2;\n");
    /* The .w is ignored when sampling, so I can use TMP2.w to calculate dot(N, N) */
    shader_addline(buffer, "DP3 TMP2.w, TMP, TMP;\n");
    shader_addline(buffer, "RCP TMP2.w, TMP2.w;\n");
    shader_addline(buffer, "MUL TMP.w, TMP.w, TMP2.w;\n");
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -TMP2;\n");

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
    current_state->current_row = 0;
}

void pshader_hw_texm3x3spec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD reg3 = arg->src[1] & WINED3DSP_REGNUM_MASK;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_str[8];
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Calculate reflection vector.
     *
     *               dot(N, E)
     * TMP.xyz = 2 * --------- * N - E
     *               dot(N, N)
     *
     * Which normalizes the normal vector
     */
    shader_addline(buffer, "DP3 TMP.w, TMP, C[%u];\n", reg3);
    shader_addline(buffer, "DP3 TMP2.w, TMP, TMP;\n");
    shader_addline(buffer, "RCP TMP2.w, TMP2.w;\n");
    shader_addline(buffer, "MUL TMP.w, TMP.w, TMP2.w;\n");
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -C[%u];\n", reg3);

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
    current_state->current_row = 0;
}

void pshader_hw_texdepth(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_name[50];

    /* texdepth has an implicit destination, the fragment depth value. It's only parameter,
     * which is essentially an input, is the destination register because it is the first
     * parameter. According to the msdn, this must be register r5, but let's keep it more flexible
     * here
     */
    pshader_get_register_name(arg->shader, arg->dst, dst_name);

    /* According to the msdn, the source register(must be r5) is unusable after
     * the texdepth instruction, so we're free to modify it
     */
    shader_addline(buffer, "MIN %s.g, %s.g, one.g;\n", dst_name, dst_name);

    /* How to deal with the special case dst_name.g == 0? if r != 0, then
     * the r * (1 / 0) will give infinity, which is clamped to 1.0, the correct
     * result. But if r = 0.0, then 0 * inf = 0, which is incorrect.
     */
    shader_addline(buffer, "RCP %s.g, %s.g;\n", dst_name, dst_name);
    shader_addline(buffer, "MUL TMP.x, %s.r, %s.g;\n", dst_name, dst_name);
    shader_addline(buffer, "MIN TMP.x, TMP.x, one.r;\n");
    shader_addline(buffer, "MAX result.depth, TMP.x, 0.0;\n");
}

/** Process the WINED3DSIO_TEXDP3TEX instruction in ARB:
 * Take a 3-component dot product of the TexCoord[dstreg] and src,
 * then perform a 1D texture lookup from stage dstregnum, place into dst. */
void pshader_hw_texdp3tex(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    char src0[50];
    char dst_str[8];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0);
    shader_addline(buffer, "MOV TMP, 0.0;\n");
    shader_addline(buffer, "DP3 TMP.x, T%u, %s;\n", sampler_idx, src0);

    sprintf(dst_str, "T%u", sampler_idx);
    shader_hw_sample(arg, sampler_idx, dst_str, "TMP", FALSE /* Only one coord, can't be projected */, FALSE);
}

/** Process the WINED3DSIO_TEXDP3 instruction in ARB:
 * Take a 3-component dot product of the TexCoord[dstreg] and src. */
void pshader_hw_texdp3(SHADER_OPCODE_ARG* arg) {
    char src0[50];
    char dst_str[50];
    char dst_mask[6];
    DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;

    /* Handle output register */
    pshader_get_register_name(arg->shader, arg->dst, dst_str);
    shader_arb_get_write_mask(arg, arg->dst, dst_mask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0);
    shader_addline(buffer, "DP3 %s%s, T%u, %s;\n", dst_str, dst_mask, dstreg, src0);

    /* TODO: Handle output modifiers */
}

/** Process the WINED3DSIO_TEXM3X3 instruction in ARB
 * Perform the 3rd row of a 3x3 matrix multiply */
void pshader_hw_texm3x3(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_str[50];
    char dst_mask[6];
    char src0[50];
    DWORD dst_reg = arg->dst & WINED3DSP_REGNUM_MASK;

    pshader_get_register_name(arg->shader, arg->dst, dst_str);
    shader_arb_get_write_mask(arg, arg->dst, dst_mask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", dst_reg, src0);
    shader_addline(buffer, "MOV %s%s, TMP;\n", dst_str, dst_mask);

    /* TODO: Handle output modifiers */
}

/** Process the WINED3DSIO_TEXM3X2DEPTH instruction in ARB:
 * Last row of a 3x2 matrix multiply, use the result to calculate the depth:
 * Calculate tmp0.y = TexCoord[dstreg] . src.xyz;  (tmp0.x has already been calculated)
 * depth = (tmp0.y == 0.0) ? 1.0 : tmp0.x / tmp0.y
 */
void pshader_hw_texm3x2depth(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD dst_reg = arg->dst & WINED3DSP_REGNUM_MASK;
    char src0[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0);
    shader_addline(buffer, "DP3 TMP.y, T%u, %s;\n", dst_reg, src0);

    /* How to deal with the special case dst_name.g == 0? if r != 0, then
     * the r * (1 / 0) will give infinity, which is clamped to 1.0, the correct
     * result. But if r = 0.0, then 0 * inf = 0, which is incorrect.
     */
    shader_addline(buffer, "RCP TMP.y, TMP.y;\n");
    shader_addline(buffer, "MUL TMP.x, TMP.x, TMP.y;\n");
    shader_addline(buffer, "MIN TMP.x, TMP.x, one.r;\n");
    shader_addline(buffer, "MAX result.depth, TMP.x, 0.0;\n");
}

/** Handles transforming all WINED3DSIO_M?x? opcodes for
    Vertex/Pixel shaders to ARB_vertex_program codes */
void shader_hw_mnxn(SHADER_OPCODE_ARG* arg) {

    int i;
    int nComponents = 0;
    SHADER_OPCODE_ARG tmpArg;

    memset(&tmpArg, 0, sizeof(SHADER_OPCODE_ARG));

    /* Set constants for the temporary argument */
    tmpArg.shader      = arg->shader;
    tmpArg.buffer      = arg->buffer;
    tmpArg.src[0]      = arg->src[0];
    tmpArg.src_addr[0] = arg->src_addr[0];
    tmpArg.src_addr[1] = arg->src_addr[1];
    tmpArg.reg_maps = arg->reg_maps;

    switch(arg->opcode->opcode) {
    case WINED3DSIO_M4x4:
        nComponents = 4;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP4);
        break;
    case WINED3DSIO_M4x3:
        nComponents = 3;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP4);
        break;
    case WINED3DSIO_M3x4:
        nComponents = 4;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
        break;
    case WINED3DSIO_M3x3:
        nComponents = 3;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
        break;
    case WINED3DSIO_M3x2:
        nComponents = 2;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
        break;
    default:
        break;
    }

    for (i = 0; i < nComponents; i++) {
        tmpArg.dst = ((arg->dst) & ~WINED3DSP_WRITEMASK_ALL)|(WINED3DSP_WRITEMASK_0<<i);
        tmpArg.src[1] = arg->src[1]+i;
        vshader_hw_map2gl(&tmpArg);
    }
}

void vshader_hw_rsq_rcp(SHADER_OPCODE_ARG* arg) {
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD dst = arg->dst;
    DWORD src = arg->src[0];
    DWORD swizzle = (src & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;

    char tmpLine[256];

    strcpy(tmpLine, curOpcode->glname); /* Opcode */
    vshader_program_add_param(arg, dst, FALSE, tmpLine); /* Destination */
    strcat(tmpLine, ",");
    vshader_program_add_param(arg, src, TRUE, tmpLine);
    if ((WINED3DSP_NOSWIZZLE >> WINED3DSP_SWIZZLE_SHIFT) == swizzle) {
        /* Dx sdk says .x is used if no swizzle is given, but our test shows that
         * .w is used
         */
        strcat(tmpLine, ".w");
    }

    shader_addline(buffer, "%s;\n", tmpLine);
}

void shader_hw_nrm(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_name[50];
    char src_name[50];
    char dst_wmask[20];
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;

    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name);
    shader_addline(buffer, "DP3 TMP, %s, %s;\n", src_name, src_name);
    shader_addline(buffer, "RSQ TMP, TMP.x;\n");
    /* dst.w = src[0].w * 1 / (src.x^2 + src.y^2 + src.z^2)^(1/2) according to msdn*/
    shader_addline(buffer, "MUL%s %s%s, %s, TMP;\n", sat ? "_SAT" : "", dst_name, dst_wmask,
                   src_name);

    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);
}

void shader_hw_sincos(SHADER_OPCODE_ARG* arg) {
    /* This instruction exists in ARB, but the d3d instruction takes two extra parameters which
     * must contain fixed constants. So we need a separate function to filter those constants and
     * can't use map2gl
     */
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_name[50];
    char src_name[50];
    char dst_wmask[20];
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;

    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name);
    shader_addline(buffer, "SCS%s %s%s, %s;\n", sat ? "_SAT" : "", dst_name, dst_wmask,
                   src_name);

    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);

}

/* TODO: merge with pixel shader */
/* Map the opcode 1-to-1 to the GL code */
void vshader_hw_map2gl(SHADER_OPCODE_ARG* arg) {

    IWineD3DVertexShaderImpl *shader = (IWineD3DVertexShaderImpl*) arg->shader;
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD dst = arg->dst;
    DWORD* src = arg->src;

    DWORD dst_regtype = shader_get_regtype(dst);
    char tmpLine[256];
    unsigned int i;

    if ((curOpcode->opcode == WINED3DSIO_MOV && dst_regtype == WINED3DSPR_ADDR) || curOpcode->opcode == WINED3DSIO_MOVA) {
        if(shader->rel_offset) {
            memset(tmpLine, 0, sizeof(tmpLine));
            vshader_program_add_param(arg, src[0], TRUE, tmpLine);
            shader_addline(buffer, "ADD TMP.x, %s, helper_const.z;\n", tmpLine);
            shader_addline(buffer, "ARL A0.x, TMP.x;\n");
            return;
        } else {
            strcpy(tmpLine, "ARL");
        }
    } else
        strcpy(tmpLine, curOpcode->glname);

    if (curOpcode->num_params > 0) {
        vshader_program_add_param(arg, dst, FALSE, tmpLine);
        for (i = 1; i < curOpcode->num_params; ++i) {
           strcat(tmpLine, ",");
           vshader_program_add_param(arg, src[i-1], TRUE, tmpLine);
        }
    }
   shader_addline(buffer, "%s;\n", tmpLine);
}

static GLuint create_arb_blt_vertex_program(WineD3D_GL_Info *gl_info) {
    GLuint program_id = 0;
    const char *blt_vprogram =
        "!!ARBvp1.0\n"
        "PARAM c[1] = { { 1, 0.5 } };\n"
        "MOV result.position, vertex.position;\n"
        "MOV result.color, c[0].x;\n"
        "MAD result.texcoord[0].y, -vertex.position, c[0], c[0];\n"
        "MAD result.texcoord[0].x, vertex.position, c[0].y, c[0].y;\n"
        "END\n";

    GL_EXTCALL(glGenProgramsARB(1, &program_id));
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, program_id));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(blt_vprogram), blt_vprogram));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint pos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
        FIXME("Vertex program error at position %d: %s\n", pos,
            debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
    }

    return program_id;
}

static GLuint create_arb_blt_fragment_program(WineD3D_GL_Info *gl_info) {
    GLuint program_id = 0;
    const char *blt_fprogram =
        "!!ARBfp1.0\n"
        "TEMP R0;\n"
        "TEX R0.x, fragment.texcoord[0], texture[0], 2D;\n"
        "MOV result.depth.z, R0.x;\n"
        "END\n";

    GL_EXTCALL(glGenProgramsARB(1, &program_id));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program_id));
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(blt_fprogram), blt_fprogram));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint pos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
        FIXME("Fragment program error at position %d: %s\n", pos,
            debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
    }

    return program_id;
}

static void shader_arb_select(IWineD3DDevice *iface, BOOL usePS, BOOL useVS) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;

    if (useVS) {
        TRACE("Using vertex shader\n");

        /* Bind the vertex program */
        GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB,
            ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.prgId));
        checkGLcall("glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertexShader->prgId);");

        /* Enable OpenGL vertex programs */
        glEnable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glEnable(GL_VERTEX_PROGRAM_ARB);");
        TRACE("(%p) : Bound vertex program %u and enabled GL_VERTEX_PROGRAM_ARB\n",
            This, ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.prgId);
    } else if(GL_SUPPORT(ARB_VERTEX_PROGRAM)) {
        glDisable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glDisable(GL_VERTEX_PROGRAM_ARB)");
    }

    if (usePS) {
        TRACE("Using pixel shader\n");

        /* Bind the fragment program */
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
            ((IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader)->baseShader.prgId));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pixelShader->prgId);");

        /* Enable OpenGL fragment programs */
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB);");
        TRACE("(%p) : Bound fragment program %u and enabled GL_FRAGMENT_PROGRAM_ARB\n",
            This, ((IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader)->baseShader.prgId);
    } else if(GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
    }
}

static void shader_arb_select_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_arb_priv *priv = (struct shader_arb_priv *) This->shader_priv;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;

    if (!priv->depth_blt_vprogram_id) priv->depth_blt_vprogram_id = create_arb_blt_vertex_program(gl_info);
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->depth_blt_vprogram_id));
    glEnable(GL_VERTEX_PROGRAM_ARB);

    if (!priv->depth_blt_fprogram_id) priv->depth_blt_fprogram_id = create_arb_blt_fragment_program(gl_info);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->depth_blt_fprogram_id));
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
}

static void shader_arb_destroy_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_arb_priv *priv = (struct shader_arb_priv *) This->shader_priv;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;

    if(priv->depth_blt_vprogram_id) {
        GL_EXTCALL(glDeleteProgramsARB(1, &priv->depth_blt_vprogram_id));
        priv->depth_blt_vprogram_id = 0;
    }
    if(priv->depth_blt_fprogram_id) {
        GL_EXTCALL(glDeleteProgramsARB(1, &priv->depth_blt_fprogram_id));
        priv->depth_blt_fprogram_id = 0;
    }
}

static void shader_arb_cleanup(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    if (GL_SUPPORT(ARB_VERTEX_PROGRAM)) glDisable(GL_VERTEX_PROGRAM_ARB);
    if (GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

static void shader_arb_destroy(IWineD3DBaseShader *iface) {
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) iface;
    WineD3D_GL_Info *gl_info = &((IWineD3DDeviceImpl *) This->baseShader.device)->adapter->gl_info;

    ENTER_GL();
    GL_EXTCALL(glDeleteProgramsARB(1, &This->baseShader.prgId));
    checkGLcall("GL_EXTCALL(glDeleteProgramsARB(1, &This->baseShader.prgId))");
    LEAVE_GL();
    This->baseShader.prgId = 0;
    This->baseShader.is_compiled = FALSE;
}

static HRESULT shader_arb_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    This->shader_priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct shader_arb_priv));
    return WINED3D_OK;
}

static void shader_arb_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HeapFree(GetProcessHeap(), 0, This->shader_priv);
}

static BOOL shader_arb_dirty_const(IWineD3DDevice *iface) {
    return TRUE;
}

static void shader_arb_generate_pshader(IWineD3DPixelShader *iface, SHADER_BUFFER *buffer) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    shader_reg_maps* reg_maps = &This->baseShader.reg_maps;
    CONST DWORD *function = This->baseShader.function;
    const char *fragcolor;
    WineD3D_GL_Info *gl_info = &((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info;

    /*  Create the hw ARB shader */
    shader_addline(buffer, "!!ARBfp1.0\n");

    shader_addline(buffer, "TEMP TMP;\n");     /* Used in matrix ops */
    shader_addline(buffer, "TEMP TMP2;\n");    /* Used in matrix ops */
    shader_addline(buffer, "TEMP TA;\n");      /* Used for modifiers */
    shader_addline(buffer, "TEMP TB;\n");      /* Used for modifiers */
    shader_addline(buffer, "TEMP TC;\n");      /* Used for modifiers */
    shader_addline(buffer, "PARAM coefdiv = { 0.5, 0.25, 0.125, 0.0625 };\n");
    shader_addline(buffer, "PARAM coefmul = { 2, 4, 8, 16 };\n");
    shader_addline(buffer, "PARAM one = { 1.0, 1.0, 1.0, 1.0 };\n");

    /* Base Declarations */
    shader_generate_arb_declarations( (IWineD3DBaseShader*) This, reg_maps, buffer, &GLINFO_LOCATION);

    /* We need two variables for fog blending */
    shader_addline(buffer, "TEMP TMP_FOG;\n");
    if (This->baseShader.hex_version >= WINED3DPS_VERSION(2,0)) {
        shader_addline(buffer, "TEMP TMP_COLOR;\n");
    }

    /* Base Shader Body */
    shader_generate_main( (IWineD3DBaseShader*) This, buffer, reg_maps, function);

    /* calculate fog and blend it
     * NOTE: state.fog.params.y and state.fog.params.z don't hold fog start s and end e but
     * -1/(e-s) and e/(e-s) respectively.
     */
    shader_addline(buffer, "MAD_SAT TMP_FOG, fragment.fogcoord, state.fog.params.y, state.fog.params.z;\n");

    if (This->baseShader.hex_version < WINED3DPS_VERSION(2,0)) {
        fragcolor = "R0";
    } else {
        fragcolor = "TMP_COLOR";
    }
    if(This->srgb_enabled) {
        /* Perform sRGB write correction. See GLX_EXT_framebuffer_sRGB */

        /* Calculate the > 0.0031308 case */
        shader_addline(buffer, "POW TMP.x, %s.x, srgb_pow.x;\n", fragcolor);
        shader_addline(buffer, "POW TMP.y, %s.y, srgb_pow.y;\n", fragcolor);
        shader_addline(buffer, "POW TMP.z, %s.z, srgb_pow.z;\n", fragcolor);
        shader_addline(buffer, "MUL TMP, TMP, srgb_mul_hi;\n");
        shader_addline(buffer, "SUB TMP, TMP, srgb_sub_hi;\n");
        /* Calculate the < case */
        shader_addline(buffer, "MUL TMP2, srgb_mul_low, %s;\n", fragcolor);
        /* Get 1.0 / 0.0 masks for > 0.0031308 and < 0.0031308 */
        shader_addline(buffer, "SLT TA, srgb_comparison, %s;\n", fragcolor);
        shader_addline(buffer, "SGE TB, srgb_comparison, %s;\n", fragcolor);
        /* Store the components > 0.0031308 in the destination */
        shader_addline(buffer, "MUL %s, TMP, TA;\n", fragcolor);
        /* Add the components that are < 0.0031308 */
        shader_addline(buffer, "MAD result.color.xyz, TMP2, TB, %s;\n", fragcolor);
        /* [0.0;1.0] clamping. Not needed, this is done implicitly */
    }
    if (This->baseShader.hex_version < WINED3DPS_VERSION(3,0)) {
        shader_addline(buffer, "LRP result.color.rgb, TMP_FOG.x, %s, state.fog.color;\n", fragcolor);
        shader_addline(buffer, "MOV result.color.a, %s.a;\n", fragcolor);
    }

    shader_addline(buffer, "END\n");

    /* TODO: change to resource.glObjectHandle or something like that */
    GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));

    TRACE("Creating a hw pixel shader, prg=%d\n", This->baseShader.prgId);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, This->baseShader.prgId));

    TRACE("Created hw pixel shader, prg=%d\n", This->baseShader.prgId);
    /* Create the program and check for errors */
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
               buffer->bsize, buffer->buffer));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint errPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
        FIXME("HW PixelShader Error at position %d: %s\n",
              errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        This->baseShader.prgId = -1;
    }
}

static void shader_arb_generate_vshader(IWineD3DVertexShader *iface, SHADER_BUFFER *buffer) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    shader_reg_maps* reg_maps = &This->baseShader.reg_maps;
    CONST DWORD *function = This->baseShader.function;
    WineD3D_GL_Info *gl_info = &((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info;

    /*  Create the hw ARB shader */
    shader_addline(buffer, "!!ARBvp1.0\n");
    shader_addline(buffer, "PARAM helper_const = { 2.0, -1.0, %d.0, 0.0 };\n", This->rel_offset);

    /* Mesa supports only 95 constants */
    if (GL_VEND(MESA) || GL_VEND(WINE))
        This->baseShader.limits.constant_float =
                min(95, This->baseShader.limits.constant_float);

    shader_addline(buffer, "TEMP TMP;\n");

    /* Base Declarations */
    shader_generate_arb_declarations( (IWineD3DBaseShader*) This, reg_maps, buffer, &GLINFO_LOCATION);

    /* We need a constant to fixup the final position */
    shader_addline(buffer, "PARAM posFixup = program.env[%d];\n", ARB_SHADER_PRIVCONST_POS);

    if((GLINFO_LOCATION).set_texcoord_w) {
        int i;
        for(i = 0; i < min(8, MAX_REG_TEXCRD); i++) {
            if(This->baseShader.reg_maps.texcoord_mask[i] != 0 &&
               This->baseShader.reg_maps.texcoord_mask[i] != WINED3DSP_WRITEMASK_ALL) {
                shader_addline(buffer, "MOV result.texcoord[%u].w, -helper_const.y;\n", i);
            }
        }
    }

    /* Base Shader Body */
    shader_generate_main( (IWineD3DBaseShader*) This, buffer, reg_maps, function);

    /* If this shader doesn't use fog copy the z coord to the fog coord so that we can use table fog */
    if (!reg_maps->fog)
        shader_addline(buffer, "MOV result.fogcoord, TMP_OUT.z;\n");

    /* Write the final position.
     *
     * OpenGL coordinates specify the center of the pixel while d3d coords specify
     * the corner. The offsets are stored in z and w in posFixup. posFixup.y contains
     * 1.0 or -1.0 to turn the rendering upside down for offscreen rendering. PosFixup.x
     * contains 1.0 to allow a mad, but arb vs swizzles are too restricted for that.
     */
    shader_addline(buffer, "MUL TMP, posFixup, TMP_OUT.w;\n");
    shader_addline(buffer, "ADD TMP_OUT.x, TMP_OUT.x, TMP.z;\n");
    shader_addline(buffer, "MAD TMP_OUT.y, TMP_OUT.y, posFixup.y, TMP.w;\n");

    /* Z coord [0;1]->[-1;1] mapping, see comment in transform_projection in state.c
     * and the glsl equivalent
     */
    shader_addline(buffer, "MAD TMP_OUT.z, TMP_OUT.z, helper_const.x, -TMP_OUT.w;\n");

    shader_addline(buffer, "MOV result.position, TMP_OUT;\n");

    shader_addline(buffer, "END\n");

    /* TODO: change to resource.glObjectHandle or something like that */
    GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));

    TRACE("Creating a hw vertex shader, prg=%d\n", This->baseShader.prgId);
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, This->baseShader.prgId));

    TRACE("Created hw vertex shader, prg=%d\n", This->baseShader.prgId);
    /* Create the program and check for errors */
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
               buffer->bsize, buffer->buffer));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint errPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
        FIXME("HW VertexShader Error at position %d: %s\n",
              errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        This->baseShader.prgId = -1;
    }
}

static void shader_arb_get_caps(WINED3DDEVTYPE devtype, WineD3D_GL_Info *gl_info, struct shader_caps *pCaps) {
    /* We don't have an ARB fixed function pipeline yet, so let the none backend set its caps,
     * then overwrite the shader specific ones
     */
    none_shader_backend.shader_get_caps(devtype, gl_info, pCaps);

    pCaps->VertexShaderVersion = WINED3DVS_VERSION(1,1);
    TRACE_(d3d_caps)("Hardware vertex shader version 1.1 enabled (ARB_PROGRAM)\n");
    pCaps->MaxVertexShaderConst = GL_LIMITS(vshader_constantsF);

    pCaps->PixelShaderVersion    = WINED3DPS_VERSION(1,4);
    pCaps->PixelShader1xMaxValue = 8.0;
    TRACE_(d3d_caps)("Hardware pixel shader version 1.4 enabled (ARB_PROGRAM)\n");
}

static void shader_arb_load_init(void) {
}

const shader_backend_t arb_program_shader_backend = {
    &shader_arb_select,
    &shader_arb_select_depth_blt,
    &shader_arb_destroy_depth_blt,
    &shader_arb_load_constants,
    &shader_arb_cleanup,
    &shader_arb_color_correction,
    &shader_arb_destroy,
    &shader_arb_alloc,
    &shader_arb_free,
    &shader_arb_dirty_const,
    &shader_arb_generate_pshader,
    &shader_arb_generate_vshader,
    &shader_arb_get_caps,
    &shader_arb_load_init,
    FFPStateTable
};
