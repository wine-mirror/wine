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
static void shader_arb_load_constantsF(IWineD3DBaseShaderImpl* This, WineD3D_GL_Info *gl_info, GLuint target_type,
        unsigned int max_constants, float* constants, struct list *constant_list) {
    constant_entry *constant;
    local_constant* lconst;
    int i;

    if (!constant_list) {
        if (TRACE_ON(d3d_shader)) {
            for (i = 0; i < max_constants; ++i) {
                TRACE("Loading constants %i: %f, %f, %f, %f\n", i,
                        constants[i * 4 + 0], constants[i * 4 + 1],
                        constants[i * 4 + 2], constants[i * 4 + 3]);
            }
        }
        for (i = 0; i < max_constants; ++i) {
            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, constants + (i * 4)));
        }
        checkGLcall("glProgramEnvParameter4fvARB()");
    } else {
        if (TRACE_ON(d3d_shader)) {
            LIST_FOR_EACH_ENTRY(constant, constant_list, constant_entry, entry) {
                i = constant->idx;
                TRACE("Loading constants %i: %f, %f, %f, %f\n", i,
                        constants[i * 4 + 0], constants[i * 4 + 1],
                        constants[i * 4 + 2], constants[i * 4 + 3]);
            }
        }
        LIST_FOR_EACH_ENTRY(constant, constant_list, constant_entry, entry) {
            i = constant->idx;
            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, constants + (i * 4)));
        }
        checkGLcall("glProgramEnvParameter4fvARB()");
    }

    /* Load immediate constants */
    if (TRACE_ON(d3d_shader)) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            GLfloat* values = (GLfloat*)lconst->value;
            TRACE("Loading local constants %i: %f, %f, %f, %f\n", lconst->idx,
                    values[0], values[1], values[2], values[3]);
        }
    }
    LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
        GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, lconst->idx, (GLfloat*)lconst->value));
    }
    checkGLcall("glProgramEnvParameter4fvARB()");
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
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl*)deviceImpl->wineD3D)->gl_info;

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;
        IWineD3DVertexShaderImpl* vshader_impl = (IWineD3DVertexShaderImpl*) stateBlock->vertexShader;
        IWineD3DVertexDeclarationImpl* vertexDeclaration = 
            (IWineD3DVertexDeclarationImpl*) vshader_impl->vertexDeclaration;

        if (NULL != vertexDeclaration && NULL != vertexDeclaration->constants) {
            /* Load DirectX 8 float constants for vertex shader */
            shader_arb_load_constantsF(vshader, gl_info, GL_VERTEX_PROGRAM_ARB,
                                       GL_LIMITS(vshader_constantsF),
                                       vertexDeclaration->constants, NULL);
        }

        /* Load DirectX 9 float constants for vertex shader */
        shader_arb_load_constantsF(vshader, gl_info, GL_VERTEX_PROGRAM_ARB,
                                   GL_LIMITS(vshader_constantsF),
                                   stateBlock->vertexShaderConstantF,
                                   &stateBlock->set_vconstantsF);

        /* Upload the position fixup */
        GL_EXTCALL(glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, ARB_SHADER_PRIVCONST_POS, deviceImpl->posFixup));
    }

    if (usePixelShader) {

        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;

        /* Load DirectX 9 float constants for pixel shader */
        shader_arb_load_constantsF(pshader, gl_info, GL_FRAGMENT_PROGRAM_ARB, 
                                   GL_LIMITS(pshader_constantsF),
                                   stateBlock->pixelShaderConstantF,
                                   &stateBlock->set_pconstantsF);
    }
}

/* Generate the variable & register declarations for the ARB_vertex_program output target */
void shader_generate_arb_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer,
    WineD3D_GL_Info* gl_info) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    DWORD i;
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    unsigned max_constantsF = min(This->baseShader.limits.constant_float, 
            (pshader ? GL_LIMITS(pshader_constantsF) : GL_LIMITS(vshader_constantsF)));

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

    /* Need to PARAM the environment parameters (constants) so we can use relative addressing */
    shader_addline(buffer, "PARAM C[%d] = { program.env[0..%d] };\n",
                   max_constantsF, max_constantsF - 1);
}

static const char* shift_tab[] = {
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

static void pshader_get_write_mask(const DWORD output_reg, char *write_mask) {
    *write_mask = 0;
    if ((output_reg & WINED3DSP_WRITEMASK_ALL) != WINED3DSP_WRITEMASK_ALL) {
        strcat(write_mask, ".");
        if (output_reg & WINED3DSP_WRITEMASK_0) strcat(write_mask, "r");
        if (output_reg & WINED3DSP_WRITEMASK_1) strcat(write_mask, "g");
        if (output_reg & WINED3DSP_WRITEMASK_2) strcat(write_mask, "b");
        if (output_reg & WINED3DSP_WRITEMASK_3) strcat(write_mask, "a");
    }
}

/* TODO: merge with pixel shader */
static void vshader_program_add_output_param_swizzle(const DWORD param, int is_color, char *hwLine) {
    /** operand output */
    if ((param & WINED3DSP_WRITEMASK_ALL) != WINED3DSP_WRITEMASK_ALL) {
      strcat(hwLine, ".");
      if (param & WINED3DSP_WRITEMASK_0) { strcat(hwLine, "x"); }
      if (param & WINED3DSP_WRITEMASK_1) { strcat(hwLine, "y"); }
      if (param & WINED3DSP_WRITEMASK_2) { strcat(hwLine, "z"); }
      if (param & WINED3DSP_WRITEMASK_3) { strcat(hwLine, "w"); }
    }
}

static void pshader_get_input_register_swizzle(const DWORD instr, char *swzstring) {
    static const char swizzle_reg_chars[] = "rgba";
    DWORD swizzle = (instr & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;
    /**
     * swizzle bits fields:
     *  WWZZYYXX
     */
    *swzstring = 0;
    if ((WINED3DSP_NOSWIZZLE >> WINED3DSP_SWIZZLE_SHIFT) != swizzle) {
        if (swizzle_x == swizzle_y &&
        swizzle_x == swizzle_z &&
        swizzle_x == swizzle_w) {
            sprintf(swzstring, ".%c", swizzle_reg_chars[swizzle_x]);
        } else {
            sprintf(swzstring, ".%c%c%c%c",
                swizzle_reg_chars[swizzle_x],
                swizzle_reg_chars[swizzle_y],
                swizzle_reg_chars[swizzle_z],
                swizzle_reg_chars[swizzle_w]);
        }
    }
}

/* TODO: merge with pixel shader */
static void vshader_program_add_input_param_swizzle(const DWORD param, int is_color, char *hwLine) {
    static const char swizzle_reg_chars_color_fix[] = "zyxw";
    static const char swizzle_reg_chars[] = "xyzw";
    const char* swizzle_regs = NULL;
    char  tmpReg[255];

    /** operand input */
    DWORD swizzle = (param & WINED3DVS_SWIZZLE_MASK) >> WINED3DVS_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;

    if (is_color) {
      swizzle_regs = swizzle_reg_chars_color_fix;
    } else {
      swizzle_regs = swizzle_reg_chars;
    }

    /**
     * swizzle bits fields:
     *  WWZZYYXX
     */
    if ((WINED3DVS_NOSWIZZLE >> WINED3DVS_SWIZZLE_SHIFT) == swizzle) {
      if (is_color) {
        sprintf(tmpReg, ".%c%c%c%c",
                swizzle_regs[swizzle_x],
                swizzle_regs[swizzle_y],
                swizzle_regs[swizzle_z],
                swizzle_regs[swizzle_w]);
        strcat(hwLine, tmpReg);
      }
      return ;
    }
    if (swizzle_x == swizzle_y &&
        swizzle_x == swizzle_z &&
        swizzle_x == swizzle_w)
    {
      sprintf(tmpReg, ".%c", swizzle_regs[swizzle_x]);
      strcat(hwLine, tmpReg);
    } else {
      sprintf(tmpReg, ".%c%c%c%c",
              swizzle_regs[swizzle_x],
              swizzle_regs[swizzle_y],
              swizzle_regs[swizzle_z],
              swizzle_regs[swizzle_w]);
      strcat(hwLine, tmpReg);
    }
}

static void pshader_get_register_name(
    const DWORD param, char* regstr) {

    DWORD reg = param & WINED3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);

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
        sprintf(regstr, "C[%u]", reg);
    break;
    case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
        sprintf(regstr,"T%u", reg);
    break;
    case WINED3DSPR_COLOROUT:
        if (reg == 0)
            sprintf(regstr, "result.color");
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
  static const char* hwrastout_reg_names[] = { "TMP_OUT", "TMP_FOG", "result.pointsize" };

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
    sprintf(tmpReg, "C[%s%u]", (param & WINED3DSHADER_ADDRMODE_RELATIVE) ? "A0.x + " : "", reg);
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
    vshader_program_add_output_param_swizzle(param, is_color, hwLine);
  } else {
    vshader_program_add_input_param_swizzle(param, is_color, hwLine);
  }
}

static void shader_hw_sample(SHADER_OPCODE_ARG* arg, DWORD sampler_idx, const char *dst_str, const char *coord_reg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;

    SHADER_BUFFER* buffer = arg->buffer;
    DWORD sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    const char *tex_type;

    switch(sampler_type) {
        case WINED3DSTT_1D:
            tex_type = "1D";
            break;

        case WINED3DSTT_2D:
            tex_type = "2D";
            break;

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

    if (deviceImpl->stateBlock->textureState[sampler_idx][WINED3DTSS_TEXTURETRANSFORMFLAGS] & WINED3DTTFF_PROJECTED) {
        shader_addline(buffer, "TXP %s, %s, texture[%u], %s;\n", dst_str, coord_reg, sampler_idx, tex_type);
    } else {
        shader_addline(buffer, "TEX %s, %s, texture[%u], %s;\n", dst_str, coord_reg, sampler_idx, tex_type);
    }
}


static void pshader_gen_input_modifier_line (
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
    pshader_get_register_name(instr, regstr);
    pshader_get_input_register_swizzle(instr, swzstr);

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

inline static void pshader_gen_output_modifier_line(
    SHADER_BUFFER* buffer,
    int saturate,
    char *write_mask,
    int shift,
    char *regstr) {

    /* Generate a line that does the output modifier computation */
    shader_addline(buffer, "MUL%s %s%s, %s, %s;\n", saturate ? "_SAT" : "",
        regstr, write_mask, regstr, shift_tab[shift]);
}

void pshader_hw_cnd(SHADER_OPCODE_ARG* arg) {

    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];

    /* FIXME: support output modifiers */

    /* Handle output register */
    pshader_get_register_name(arg->dst, dst_name);
    pshader_get_write_mask(arg->dst, dst_wmask);
    strcat(dst_name, dst_wmask);

    /* Generate input register names (with modifiers) */
    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(buffer, arg->src[1], 1, src_name[1]);
    pshader_gen_input_modifier_line(buffer, arg->src[2], 2, src_name[2]);

    shader_addline(buffer, "ADD TMP, -%s, coefdiv.x;\n", src_name[0]);
    shader_addline(buffer, "CMP %s, TMP, %s, %s;\n", dst_name, src_name[1], src_name[2]);
}

void pshader_hw_cmp(SHADER_OPCODE_ARG* arg) {

    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];

    /* FIXME: support output modifiers */

    /* Handle output register */
    pshader_get_register_name(arg->dst, dst_name);
    pshader_get_write_mask(arg->dst, dst_wmask);
    strcat(dst_name, dst_wmask);

    /* Generate input register names (with modifiers) */
    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(buffer, arg->src[1], 1, src_name[1]);
    pshader_gen_input_modifier_line(buffer, arg->src[2], 2, src_name[2]);

    shader_addline(buffer, "CMP %s, %s, %s, %s;\n", dst_name,
        src_name[0], src_name[2], src_name[1]);
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
            FIXME("Unrecognized modifier(0x%#x)\n", mask >> WINED3DSP_DSTMOD_SHIFT);

         if (centroid)
             FIXME("Unhandled modifier(0x%#x)\n", mask >> WINED3DSP_DSTMOD_SHIFT);
     }
     shift = (dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;

      /* Generate input and output registers */
      if (curOpcode->num_params > 0) {
          char operands[4][100];

          /* Generate input register names (with modifiers) */
          for (i = 1; i < curOpcode->num_params; ++i)
              pshader_gen_input_modifier_line(buffer, src[i-1], i-1, operands[i]);

          /* Handle output register */
          pshader_get_register_name(dst, output_rname);
          strcpy(operands[0], output_rname);
          pshader_get_write_mask(dst, output_wmask);
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

void pshader_hw_tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;

    DWORD dst = arg->dst;
    DWORD* src = arg->src;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;

    char reg_dest[40];
    char reg_coord[40];
    DWORD reg_dest_code;
    DWORD reg_sampler_code;

    /* All versions have a destination register */
    reg_dest_code = dst & WINED3DSP_REGNUM_MASK;
    pshader_get_register_name(dst, reg_dest);

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
   if (hex_version < WINED3DPS_VERSION(1,4))
      strcpy(reg_coord, reg_dest);
   else
      pshader_gen_input_modifier_line(buffer, src[0], 0, reg_coord);

  /* 1.0-1.4: Use destination register number as texture code.
     2.0+: Use provided sampler number as texure code. */
  if (hex_version < WINED3DPS_VERSION(2,0))
     reg_sampler_code = reg_dest_code;
  else
     reg_sampler_code = src[1] & WINED3DSP_REGNUM_MASK;

  shader_hw_sample(arg, reg_sampler_code, reg_dest, reg_coord);
}

void pshader_hw_texcoord(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD dst = arg->dst;
    DWORD* src = arg->src;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;

    char tmp[20];
    pshader_get_write_mask(dst, tmp);
    if (hex_version != WINED3DPS_VERSION(1,4)) {
        DWORD reg = dst & WINED3DSP_REGNUM_MASK;
        shader_addline(buffer, "MOV_SAT T%u%s, fragment.texcoord[%u];\n", reg, tmp, reg);
    } else {
        DWORD reg1 = dst & WINED3DSP_REGNUM_MASK;
        DWORD reg2 = src[0] & WINED3DSP_REGNUM_MASK;
        shader_addline(buffer, "MOV R%u%s, fragment.texcoord[%u];\n", reg1, tmp, reg2);
   }
}

void pshader_hw_texreg2ar(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;

     DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & WINED3DSP_REGNUM_MASK;
     shader_addline(buffer, "MOV TMP.r, T%u.a;\n", reg2);
     shader_addline(buffer, "MOV TMP.g, T%u.r;\n", reg2);
     shader_addline(buffer, "TEX T%u, TMP, texture[%u], 2D;\n", reg1, reg1);
}

void pshader_hw_texreg2gb(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;

     DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & WINED3DSP_REGNUM_MASK;
     shader_addline(buffer, "MOV TMP.r, T%u.g;\n", reg2);
     shader_addline(buffer, "MOV TMP.g, T%u.b;\n", reg2);
     shader_addline(buffer, "TEX T%u, TMP, texture[%u], 2D;\n", reg1, reg1);
}

void pshader_hw_texbem(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;
     DWORD reg1 = arg->dst  & WINED3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & WINED3DSP_REGNUM_MASK;

     /* FIXME: Should apply the BUMPMAPENV matrix */
     shader_addline(buffer, "ADD TMP.rg, fragment.texcoord[%u], T%u;\n", reg1, reg2);
     shader_addline(buffer, "TEX T%u, TMP, texture[%u], 2D;\n", reg1, reg1);
}

void pshader_hw_texm3x2pad(SHADER_OPCODE_ARG* arg) {

    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.x, T%u, %s;\n", reg, src0_name);
}

void pshader_hw_texm3x2tex(SHADER_OPCODE_ARG* arg) {

    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.y, T%u, %s;\n", reg, src0_name);
    shader_addline(buffer, "TEX T%u, TMP, texture[%u], 2D;\n", reg, reg);
}

void pshader_hw_texm3x3pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.%c, T%u, %s;\n", 'x' + current_state->current_row, reg, src0_name);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

void pshader_hw_texm3x3tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char dst_str[8];
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    shader_hw_sample(arg, reg, dst_str, "TMP");
    current_state->current_row = 0;
}

void pshader_hw_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "MOV TMP2.x, fragment.texcoord[%u].w;\n", current_state->texcoord_w[0]);
    shader_addline(buffer, "MOV TMP2.y, fragment.texcoord[%u].w;\n", current_state->texcoord_w[1]);
    shader_addline(buffer, "MOV TMP2.z, fragment.texcoord[%u].w;\n", reg);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
    shader_addline(buffer, "DP3 TMP.w, TMP, TMP2;\n");
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -TMP2;\n");

    /* Cubemap textures will be more used than 3D ones. */
    shader_addline(buffer, "TEX T%u, TMP, texture[%u], CUBE;\n", reg, reg);
    current_state->current_row = 0;
}

void pshader_hw_texm3x3spec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD reg3 = arg->src[1] & WINED3DSP_REGNUM_MASK;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
    shader_addline(buffer, "DP3 TMP.w, TMP, C[%u];\n", reg3);
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -C[%u];\n", reg3);

    /* Cubemap textures will be more used than 3D ones. */
    shader_addline(buffer, "TEX T%u, TMP, texture[%u], CUBE;\n", reg, reg);
    current_state->current_row = 0;
}

/** Handles transforming all WINED3DSIO_M?x? opcodes for
    Vertex shaders to ARB_vertex_program codes */
void vshader_hw_mnxn(SHADER_OPCODE_ARG* arg) {

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

/* TODO: merge with pixel shader */
/* Map the opcode 1-to-1 to the GL code */
void vshader_hw_map2gl(SHADER_OPCODE_ARG* arg) {

    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD dst = arg->dst;
    DWORD* src = arg->src;

    DWORD dst_regtype = shader_get_regtype(dst);
    char tmpLine[256];
    unsigned int i;

    if ((curOpcode->opcode == WINED3DSIO_MOV && dst_regtype == WINED3DSPR_ADDR) || curOpcode->opcode == WINED3DSIO_MOVA)
        strcpy(tmpLine, "ARL");
    else
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
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl *)(This->wineD3D))->gl_info;

    if (useVS) {
        TRACE("Using vertex shader\n");

        /* Bind the vertex program */
        GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB,
            ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.prgId));
        checkGLcall("glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertexShader->prgId);");

        /* Enable OpenGL vertex programs */
        glEnable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glEnable(GL_VERTEX_PROGRAM_ARB);");
        TRACE_(d3d_shader)("(%p) : Bound vertex program %u and enabled GL_VERTEX_PROGRAM_ARB\n",
            This, ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.prgId);
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
        TRACE_(d3d_shader)("(%p) : Bound fragment program %u and enabled GL_FRAGMENT_PROGRAM_ARB\n",
            This, ((IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader)->baseShader.prgId);
    }
}

static void shader_arb_select_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl *)(This->wineD3D))->gl_info;
    static GLuint vprogram_id = 0;
    static GLuint fprogram_id = 0;

    if (!vprogram_id) vprogram_id = create_arb_blt_vertex_program(gl_info);
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vprogram_id));
    glEnable(GL_VERTEX_PROGRAM_ARB);

    if (!fprogram_id) fprogram_id = create_arb_blt_fragment_program(gl_info);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fprogram_id));
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
}

static void shader_arb_cleanup(BOOL usePS, BOOL useVS) {
    if (useVS) glDisable(GL_VERTEX_PROGRAM_ARB);
    if (usePS) glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

const shader_backend_t arb_program_shader_backend = {
    &shader_arb_select,
    &shader_arb_select_depth_blt,
    &shader_arb_load_constants,
    &shader_arb_cleanup
};
