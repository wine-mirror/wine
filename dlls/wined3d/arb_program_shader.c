/*
 * Pixel and vertex shaders implementation using ARB_vertex_program
 * and ARB_fragment_program GL extensions.
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2006 Jason Green
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
 * When @constants_set == NULL, it will load all the constants.
 *  
 * @target_type should be either GL_VERTEX_PROGRAM_ARB (for vertex shaders)
 *  or GL_FRAGMENT_PROGRAM_ARB (for pixel shaders)
 */
void shader_arb_load_constantsF(
    WineD3D_GL_Info *gl_info,
    GLuint target_type,
    unsigned max_constants,
    float* constants,
    BOOL* constants_set) {

    int i;
    
    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {
            TRACE("Loading constants %i: %f, %f, %f, %f\n", i,
                  constants[i * sizeof(float) + 0], constants[i * sizeof(float) + 1],
                  constants[i * sizeof(float) + 2], constants[i * sizeof(float) + 3]);

            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, &constants[i * sizeof(float)]));
            checkGLcall("glProgramEnvParameter4fvARB");
        }
    }
}

/**
 * Loads the app-supplied constants into the currently set ARB_[vertex/fragment]_programs.
 * 
 * We only support float constants in ARB at the moment, so don't 
 * worry about the Integers or Booleans
 */
void shader_arb_load_constants(
    IWineD3DStateBlock* iface,
    char usePixelShader,
    char useVertexShader) {
    
    IWineD3DStateBlockImpl* stateBlock = (IWineD3DStateBlockImpl*) iface;
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl*)stateBlock->wineD3DDevice->wineD3D)->gl_info;

    if (useVertexShader) {
        IWineD3DVertexShaderImpl* vshader = (IWineD3DVertexShaderImpl*) stateBlock->vertexShader;
        IWineD3DVertexDeclarationImpl* vertexDeclaration = 
            (IWineD3DVertexDeclarationImpl*) vshader->vertexDeclaration;

        if (NULL != vertexDeclaration && NULL != vertexDeclaration->constants) {
            /* Load DirectX 8 float constants for vertex shader */
            shader_arb_load_constantsF(gl_info, GL_VERTEX_PROGRAM_ARB,
                                       WINED3D_VSHADER_MAX_CONSTANTS,
                                       vertexDeclaration->constants, NULL);
        }

        /* Load DirectX 9 float constants for vertex shader */
        shader_arb_load_constantsF(gl_info, GL_VERTEX_PROGRAM_ARB,
                                   WINED3D_VSHADER_MAX_CONSTANTS,
                                   stateBlock->vertexShaderConstantF,
                                   stateBlock->set.vertexShaderConstantsF);
    }

    if (usePixelShader) {

        /* Load DirectX 9 float constants for pixel shader */
        shader_arb_load_constantsF(gl_info, GL_FRAGMENT_PROGRAM_ARB, WINED3D_PSHADER_MAX_CONSTANTS,
                                   stateBlock->pixelShaderConstantF,
                                   stateBlock->set.pixelShaderConstantsF);
    }
}

/* Generate the variable & register declarations for the ARB_vertex_program output target */
void shader_generate_arb_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    DWORD i;

    for(i = 0; i < This->baseShader.limits.temporary; i++) {
        if (reg_maps->temporary[i])
            shader_addline(buffer, "TEMP R%lu;\n", i);
    }

    for (i = 0; i < This->baseShader.limits.address; i++) {
        if (reg_maps->address[i])
            shader_addline(buffer, "ADDRESS A%ld;\n", i);
    }

    for(i = 0; i < This->baseShader.limits.texcoord; i++) {
        if (reg_maps->texcoord[i])
            shader_addline(buffer,"TEMP T%lu;\n", i);
    }

    /* Texture coordinate registers must be pre-loaded */
    for (i = 0; i < This->baseShader.limits.texcoord; i++) {
        if (reg_maps->texcoord[i])
            shader_addline(buffer, "MOV T%lu, fragment.texcoord[%lu];\n", i, i);
    }

    /* Need to PARAM the environment parameters (constants) so we can use relative addressing */
    shader_addline(buffer, "PARAM C[%d] = { program.env[0..%d] };\n",
                   This->baseShader.limits.constant_float,
                   This->baseShader.limits.constant_float - 1);
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
    if ((output_reg & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
        strcat(write_mask, ".");
        if (output_reg & D3DSP_WRITEMASK_0) strcat(write_mask, "r");
        if (output_reg & D3DSP_WRITEMASK_1) strcat(write_mask, "g");
        if (output_reg & D3DSP_WRITEMASK_2) strcat(write_mask, "b");
        if (output_reg & D3DSP_WRITEMASK_3) strcat(write_mask, "a");
    }
}

/* TODO: merge with pixel shader */
static void vshader_program_add_output_param_swizzle(const DWORD param, int is_color, char *hwLine) {
    /** operand output */
    if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
      strcat(hwLine, ".");
      if (param & D3DSP_WRITEMASK_0) { strcat(hwLine, "x"); }
      if (param & D3DSP_WRITEMASK_1) { strcat(hwLine, "y"); }
      if (param & D3DSP_WRITEMASK_2) { strcat(hwLine, "z"); }
      if (param & D3DSP_WRITEMASK_3) { strcat(hwLine, "w"); }
    }
}

static void pshader_get_input_register_swizzle(const DWORD instr, char *swzstring) {
    static const char swizzle_reg_chars[] = "rgba";
    DWORD swizzle = (instr & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;
    /**
     * swizzle bits fields:
     *  WWZZYYXX
     */
    *swzstring = 0;
    if ((D3DSP_NOSWIZZLE >> D3DSP_SWIZZLE_SHIFT) != swizzle) { /* ! D3DVS_NOSWIZZLE == 0xE4 << D3DVS_SWIZZLE_SHIFT */
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
    DWORD swizzle = (param & D3DVS_SWIZZLE_MASK) >> D3DVS_SWIZZLE_SHIFT;
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
    if ((D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) == swizzle) { /* D3DVS_NOSWIZZLE == 0xE4 << D3DVS_SWIZZLE_SHIFT */
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

static void pshader_get_register_name(const DWORD param, char* regstr, CHAR *constants) {

    DWORD reg = param & D3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);

    switch (regtype) {
    case D3DSPR_TEMP:
        sprintf(regstr, "R%lu", reg);
    break;
    case D3DSPR_INPUT:
        if (reg==0) {
            strcpy(regstr, "fragment.color.primary");
        } else {
            strcpy(regstr, "fragment.color.secondary");
        }
    break;
    case D3DSPR_CONST:
        if (constants[reg])
            sprintf(regstr, "C%lu", reg);
        else
            sprintf(regstr, "C[%lu]", reg);
    break;
    case D3DSPR_TEXTURE: /* case D3DSPR_ADDR: */
        sprintf(regstr,"T%lu", reg);
    break;
    case D3DSPR_COLOROUT:
        if (reg == 0)
            sprintf(regstr, "result.color");
        else {
            /* TODO: See GL_ARB_draw_buffers */
            FIXME("Unsupported write to render target %lu\n", reg);
            sprintf(regstr, "unsupported_register");
        }
    break;
    case D3DSPR_DEPTHOUT:
        sprintf(regstr, "result.depth");
    break;
    case D3DSPR_ATTROUT:
        sprintf(regstr, "oD[%lu]", reg);
    break;
    case D3DSPR_TEXCRDOUT:
        sprintf(regstr, "oT[%lu]", reg);
    break;
    default:
        FIXME("Unhandled register name Type(%ld)\n", regtype);
        sprintf(regstr, "unrecognized_register");
    break;
    }
}

/* TODO: merge with pixel shader */
static void vshader_program_add_param(SHADER_OPCODE_ARG *arg, const DWORD param, BOOL is_input, char *hwLine) {

  /* oPos, oFog and oPts in D3D */
  static const char* hwrastout_reg_names[] = { "result.position", "result.fogcoord", "result.pointsize" };

  DWORD reg = param & D3DSP_REGNUM_MASK;
  DWORD regtype = shader_get_regtype(param);
  char  tmpReg[255];
  BOOL is_color = FALSE;

  if ((param & D3DSP_SRCMOD_MASK) == D3DSPSM_NEG) {
      strcat(hwLine, " -");
  } else {
      strcat(hwLine, " ");
  }

  switch (regtype) {
  case D3DSPR_TEMP:
    sprintf(tmpReg, "R%lu", reg);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_INPUT:

    if (arg->reg_maps->semantics_in[WINED3DSHADERDECLUSAGE_DIFFUSE] &&
        reg == (arg->reg_maps->semantics_in[WINED3DSHADERDECLUSAGE_DIFFUSE] & D3DSP_REGNUM_MASK))
        is_color = TRUE;

    if (arg->reg_maps->semantics_in[WINED3DSHADERDECLUSAGE_SPECULAR] &&
        reg == (arg->reg_maps->semantics_in[WINED3DSHADERDECLUSAGE_SPECULAR] & D3DSP_REGNUM_MASK))
        is_color = TRUE;

    /* FIXME: Shaders in 8.1 appear to not require a dcl statement - use
     * the reg value from the vertex declaration. However, usage map is not initialized
     * in that case - how can we know if an input contains color data or not? */

    sprintf(tmpReg, "vertex.attrib[%lu]", reg);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_CONST:
    /* FIXME: some constants are named so we need a constants map*/
    if (arg->reg_maps->constantsF[reg]) {
        if (param & D3DVS_ADDRMODE_RELATIVE) {
            FIXME("Relative addressing not expected for a named constant %lu\n", reg);
        }
        sprintf(tmpReg, "C%lu", reg);
    } else {
        sprintf(tmpReg, "C[%s%lu]", (param & D3DVS_ADDRMODE_RELATIVE) ? "A0.x + " : "", reg);
    }
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_ADDR: /*case D3DSPR_TEXTURE:*/
    sprintf(tmpReg, "A%lu", reg);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_RASTOUT:
    sprintf(tmpReg, "%s", hwrastout_reg_names[reg]);
    strcat(hwLine, tmpReg);
    break;
  case D3DSPR_ATTROUT:
    if (reg==0) {
       strcat(hwLine, "result.color.primary");
    } else {
       strcat(hwLine, "result.color.secondary");
    }
    break;
  case D3DSPR_TEXCRDOUT:
    sprintf(tmpReg, "result.texcoord[%lu]", reg);
    strcat(hwLine, tmpReg);
    break;
  default:
    FIXME("Unknown reg type %ld %ld\n", regtype, reg);
    strcat(hwLine, "unrecognized_register");
    break;
  }

  if (!is_input) {
    vshader_program_add_output_param_swizzle(param, is_color, hwLine);
  } else {
    vshader_program_add_input_param_swizzle(param, is_color, hwLine);
  }
}

static void pshader_gen_input_modifier_line (
    SHADER_BUFFER* buffer,
    const DWORD instr,
    int tmpreg,
    char *outregstr,
    CHAR *constants) {

    /* Generate a line that does the input modifier computation and return the input register to use */
    char regstr[256];
    char swzstr[20];
    int insert_line;

    /* Assume a new line will be added */
    insert_line = 1;

    /* Get register name */
    pshader_get_register_name(instr, regstr, constants);
    pshader_get_input_register_swizzle(instr, swzstr);

    switch (instr & D3DSP_SRCMOD_MASK) {
    case D3DSPSM_NONE:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = 0;
        break;
    case D3DSPSM_NEG:
        sprintf(outregstr, "-%s%s", regstr, swzstr);
        insert_line = 0;
        break;
    case D3DSPSM_BIAS:
        shader_addline(buffer, "ADD T%c, %s, -coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case D3DSPSM_BIASNEG:
        shader_addline(buffer, "ADD T%c, -%s, coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case D3DSPSM_SIGN:
        shader_addline(buffer, "MAD T%c, %s, coefmul.x, -one.x;\n", 'A' + tmpreg, regstr);
        break;
    case D3DSPSM_SIGNNEG:
        shader_addline(buffer, "MAD T%c, %s, -coefmul.x, one.x;\n", 'A' + tmpreg, regstr);
        break;
    case D3DSPSM_COMP:
        shader_addline(buffer, "SUB T%c, one.x, %s;\n", 'A' + tmpreg, regstr);
        break;
    case D3DSPSM_X2:
        shader_addline(buffer, "ADD T%c, %s, %s;\n", 'A' + tmpreg, regstr, regstr);
        break;
    case D3DSPSM_X2NEG:
        shader_addline(buffer, "ADD T%c, -%s, -%s;\n", 'A' + tmpreg, regstr, regstr);
        break;
    case D3DSPSM_DZ:
        shader_addline(buffer, "RCP T%c, %s.z;\n", 'A' + tmpreg, regstr);
        shader_addline(buffer, "MUL T%c, %s, T%c;\n", 'A' + tmpreg, regstr, 'A' + tmpreg);
        break;
    case D3DSPSM_DW:
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

/** Process the D3DSIO_DEF opcode into an ARB string - creates a local vec4
 * float constant, and stores it's usage on the regmaps. */
void shader_hw_def(SHADER_OPCODE_ARG* arg) {

    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;

    shader_addline(arg->buffer, 
                   "PARAM C%lu = { %f, %f, %f, %f };\n", reg,
                   *((const float *)(arg->src + 0)),
                   *((const float *)(arg->src + 1)),
                   *((const float *)(arg->src + 2)),
                   *((const float *)(arg->src + 3)) );

    arg->reg_maps->constantsF[reg] = 1;
}

void pshader_hw_cnd(SHADER_OPCODE_ARG* arg) {

    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];

    /* FIXME: support output modifiers */

    /* Handle output register */
    pshader_get_register_name(arg->dst, dst_name, arg->reg_maps->constantsF);
    pshader_get_write_mask(arg->dst, dst_wmask);
    strcat(dst_name, dst_wmask);

    /* Generate input register names (with modifiers) */
    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src_name[0], arg->reg_maps->constantsF);
    pshader_gen_input_modifier_line(buffer, arg->src[1], 1, src_name[1], arg->reg_maps->constantsF);
    pshader_gen_input_modifier_line(buffer, arg->src[2], 2, src_name[2], arg->reg_maps->constantsF);

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
    pshader_get_register_name(arg->dst, dst_name, arg->reg_maps->constantsF);
    pshader_get_write_mask(arg->dst, dst_wmask);
    strcat(dst_name, dst_wmask);

    /* Generate input register names (with modifiers) */
    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src_name[0], arg->reg_maps->constantsF);
    pshader_gen_input_modifier_line(buffer, arg->src[1], 1, src_name[1], arg->reg_maps->constantsF);
    pshader_gen_input_modifier_line(buffer, arg->src[2], 2, src_name[2], arg->reg_maps->constantsF);

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
     if (0 != (dst & D3DSP_DSTMOD_MASK)) {
         DWORD mask = dst & D3DSP_DSTMOD_MASK;

         saturate = mask & D3DSPDM_SATURATE;
         centroid = mask & D3DSPDM_MSAMPCENTROID;
         partialprecision = mask & D3DSPDM_PARTIALPRECISION;
         mask &= ~(D3DSPDM_MSAMPCENTROID | D3DSPDM_PARTIALPRECISION | D3DSPDM_SATURATE);
         if (mask)
            FIXME("Unrecognized modifier(0x%#lx)\n", mask >> D3DSP_DSTMOD_SHIFT);

         if (centroid)
             FIXME("Unhandled modifier(0x%#lx)\n", mask >> D3DSP_DSTMOD_SHIFT);
     }
     shift = (dst & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;

      /* Generate input and output registers */
      if (curOpcode->num_params > 0) {
          char operands[4][100];

          /* Generate input register names (with modifiers) */
          for (i = 1; i < curOpcode->num_params; ++i)
              pshader_gen_input_modifier_line(buffer, src[i-1], i-1, operands[i], arg->reg_maps->constantsF);

          /* Handle output register */
          pshader_get_register_name(dst, output_rname, arg->reg_maps->constantsF);
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
    reg_dest_code = dst & D3DSP_REGNUM_MASK;
    pshader_get_register_name(dst, reg_dest, arg->reg_maps->constantsF);

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
   if (hex_version < D3DPS_VERSION(1,4))
      strcpy(reg_coord, reg_dest);
   else
      pshader_gen_input_modifier_line(buffer, src[0], 0, reg_coord, arg->reg_maps->constantsF);

  /* 1.0-1.4: Use destination register number as texture code.
     2.0+: Use provided sampler number as texure code. */
  if (hex_version < D3DPS_VERSION(2,0))
     reg_sampler_code = reg_dest_code;
  else
     reg_sampler_code = src[1] & D3DSP_REGNUM_MASK;

  shader_addline(buffer, "TEX %s, %s, texture[%lu], 2D;\n",
      reg_dest, reg_coord, reg_sampler_code);
}

void pshader_hw_texcoord(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD dst = arg->dst;
    DWORD* src = arg->src;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;

    char tmp[20];
    pshader_get_write_mask(dst, tmp);
    if (hex_version != D3DPS_VERSION(1,4)) {
        DWORD reg = dst & D3DSP_REGNUM_MASK;
        shader_addline(buffer, "MOV T%lu%s, fragment.texcoord[%lu];\n", reg, tmp, reg);
    } else {
        DWORD reg1 = dst & D3DSP_REGNUM_MASK;
        DWORD reg2 = src[0] & D3DSP_REGNUM_MASK;
        shader_addline(buffer, "MOV R%lu%s, fragment.texcoord[%lu];\n", reg1, tmp, reg2);
   }
}

void pshader_hw_texreg2ar(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;

     DWORD reg1 = arg->dst & D3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & D3DSP_REGNUM_MASK;
     shader_addline(buffer, "MOV TMP.r, T%lu.a;\n", reg2);
     shader_addline(buffer, "MOV TMP.g, T%lu.r;\n", reg2);
     shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
}

void pshader_hw_texreg2gb(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;

     DWORD reg1 = arg->dst & D3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & D3DSP_REGNUM_MASK;
     shader_addline(buffer, "MOV TMP.r, T%lu.g;\n", reg2);
     shader_addline(buffer, "MOV TMP.g, T%lu.b;\n", reg2);
     shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
}

void pshader_hw_texbem(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;
     DWORD reg1 = arg->dst  & D3DSP_REGNUM_MASK;
     DWORD reg2 = arg->src[0] & D3DSP_REGNUM_MASK;

     /* FIXME: Should apply the BUMPMAPENV matrix */
     shader_addline(buffer, "ADD TMP.rg, fragment.texcoord[%lu], T%lu;\n", reg1, reg2);
     shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg1, reg1);
}

void pshader_hw_texm3x2pad(SHADER_OPCODE_ARG* arg) {

    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name, arg->reg_maps->constantsF);
    shader_addline(buffer, "DP3 TMP.x, T%lu, %s;\n", reg, src0_name);
}

void pshader_hw_texm3x2tex(SHADER_OPCODE_ARG* arg) {

    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name, arg->reg_maps->constantsF);
    shader_addline(buffer, "DP3 TMP.y, T%lu, %s;\n", reg, src0_name);
    shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], 2D;\n", reg, reg);
}

void pshader_hw_texm3x3pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name, arg->reg_maps->constantsF);
    shader_addline(buffer, "DP3 TMP.%c, T%lu, %s;\n", 'x' + current_state->current_row, reg, src0_name);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

void pshader_hw_texm3x3tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name, arg->reg_maps->constantsF);
    shader_addline(buffer, "DP3 TMP.z, T%lu, %s;\n", reg, src0_name);

    /* Cubemap textures will be more used than 3D ones. */
    shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
    current_state->current_row = 0;
}

void pshader_hw_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name, arg->reg_maps->constantsF);
    shader_addline(buffer, "DP3 TMP.z, T%lu, %s;\n", reg, src0_name);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "MOV TMP2.x, fragment.texcoord[%lu].w;\n", current_state->texcoord_w[0]);
    shader_addline(buffer, "MOV TMP2.y, fragment.texcoord[%lu].w;\n", current_state->texcoord_w[1]);
    shader_addline(buffer, "MOV TMP2.z, fragment.texcoord[%lu].w;\n", reg);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
    shader_addline(buffer, "DP3 TMP.w, TMP, TMP2;\n");
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -TMP2;\n");

    /* Cubemap textures will be more used than 3D ones. */
    shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
    current_state->current_row = 0;
}

void pshader_hw_texm3x3spec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    DWORD reg3 = arg->src[1] & D3DSP_REGNUM_MASK;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_name[50];

    pshader_gen_input_modifier_line(buffer, arg->src[0], 0, src0_name, arg->reg_maps->constantsF);
    shader_addline(buffer, "DP3 TMP.z, T%lu, %s;\n", reg, src0_name);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
    shader_addline(buffer, "DP3 TMP.w, TMP, C[%lu];\n", reg3);
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -C[%lu];\n", reg3);

    /* Cubemap textures will be more used than 3D ones. */
    shader_addline(buffer, "TEX T%lu, TMP, texture[%lu], CUBE;\n", reg, reg);
    current_state->current_row = 0;
}

/** Handles transforming all D3DSIO_M?x? opcodes for
    Vertex shaders to ARB_vertex_program codes */
void vshader_hw_mnxn(SHADER_OPCODE_ARG* arg) {

    int i;
    int nComponents = 0;
    SHADER_OPCODE_ARG tmpArg;

    /* Set constants for the temporary argument */
    tmpArg.shader   = arg->shader;
    tmpArg.buffer   = arg->buffer;
    tmpArg.src[0]   = arg->src[0];

    switch(arg->opcode->opcode) {
    case D3DSIO_M4x4:
        nComponents = 4;
        tmpArg.opcode = &IWineD3DVertexShaderImpl_shader_ins[D3DSIO_DP4];
        break;
    case D3DSIO_M4x3:
        nComponents = 3;
        tmpArg.opcode = &IWineD3DVertexShaderImpl_shader_ins[D3DSIO_DP4];
        break;
    case D3DSIO_M3x4:
        nComponents = 4;
        tmpArg.opcode = &IWineD3DVertexShaderImpl_shader_ins[D3DSIO_DP3];
        break;
    case D3DSIO_M3x3:
        nComponents = 3;
        tmpArg.opcode = &IWineD3DVertexShaderImpl_shader_ins[D3DSIO_DP3];
        break;
    case D3DSIO_M3x2:
        nComponents = 2;
        tmpArg.opcode = &IWineD3DVertexShaderImpl_shader_ins[D3DSIO_DP3];
        break;
    default:
        break;
    }

    for (i = 0; i < nComponents; i++) {
        tmpArg.dst = ((arg->dst) & ~D3DSP_WRITEMASK_ALL)|(D3DSP_WRITEMASK_0<<i);
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

    if (curOpcode->opcode == D3DSIO_MOV && dst_regtype == D3DSPR_ADDR)
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
