/*
 * GLSL pixel and vertex shader implementation
 *
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
#include <stdio.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

#define GLINFO_LOCATION      (*gl_info)

/** Prints the GLSL info log which will contain error messages if they exist */
void print_glsl_info_log(WineD3D_GL_Info *gl_info, GLhandleARB obj) {
    
    int infologLength = 0;
    char *infoLog;

    GL_EXTCALL(glGetObjectParameterivARB(obj,
               GL_OBJECT_INFO_LOG_LENGTH_ARB,
               &infologLength));

    /* A size of 1 is just a null-terminated string, so the log should be bigger than
     * that if there are errors. */
    if (infologLength > 1)
    {
        infoLog = (char *)HeapAlloc(GetProcessHeap(), 0, infologLength);
        GL_EXTCALL(glGetInfoLogARB(obj, infologLength, NULL, infoLog));
        FIXME("Error received from GLSL shader #%u: %s\n", obj, debugstr_a(infoLog));
        HeapFree(GetProcessHeap(), 0, infoLog);
    }
}

/**
 * Loads (pixel shader) samplers
 */
void shader_glsl_load_psamplers(
    WineD3D_GL_Info *gl_info,
    IWineD3DStateBlock* iface) {

    IWineD3DStateBlockImpl* stateBlock = (IWineD3DStateBlockImpl*) iface;
    GLhandleARB programId = stateBlock->shaderPrgId;
    GLhandleARB name_loc;
    int i;
    char sampler_name[20];

    for (i=0; i< GL_LIMITS(samplers); ++i) {
        if (stateBlock->textures[i] != NULL) {
           snprintf(sampler_name, sizeof(sampler_name), "Psampler%d", i);
           name_loc = GL_EXTCALL(glGetUniformLocationARB(programId, sampler_name));
           if (name_loc != -1) {
               TRACE_(d3d_shader)("Loading %s for texture %d\n", sampler_name, i);
               GL_EXTCALL(glUniform1iARB(name_loc, i));
               checkGLcall("glUniform1iARB");
           }
        }
    }
}

/** 
 * Loads floating point constants (aka uniforms) into the currently set GLSL program.
 * When @constants_set == NULL, it will load all the constants.
 */
void shader_glsl_load_constantsF(
    IWineD3DBaseShaderImpl* This,
    WineD3D_GL_Info *gl_info,
    GLhandleARB programId,
    unsigned max_constants,
    float* constants,
    BOOL* constants_set) {
        
    GLhandleARB tmp_loc;
    int i;
    char tmp_name[7];
    char is_pshader = shader_is_pshader_version(This->baseShader.hex_version);
    const char* prefix = is_pshader? "PC":"VC";
    struct list* ptr;

    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {

            TRACE("Loading constants %i: %f, %f, %f, %f\n", i,
                  constants[i * sizeof(float) + 0], constants[i * sizeof(float) + 1],
                  constants[i * sizeof(float) + 2], constants[i * sizeof(float) + 3]);

            /* TODO: Benchmark and see if it would be beneficial to store the 
             * locations of the constants to avoid looking up each time */
            snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, i);
            tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, &constants[i * sizeof(float)]));
                checkGLcall("glUniform4fvARB");
            }
        }
    }

    /* Load immediate constants */
    ptr = list_head(&This->baseShader.constantsF);
    while (ptr) {
        local_constant* lconst = LIST_ENTRY(ptr, struct local_constant, entry);
        unsigned int idx = lconst->idx;
        GLfloat* values = (GLfloat*) lconst->value;

        TRACE("Loading local constants %i: %f, %f, %f, %f\n", idx,
            values[0], values[1], values[2], values[3]);

        snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, values));
            checkGLcall("glUniform4fvARB");
        }
        ptr = list_next(&This->baseShader.constantsF, ptr);
    }
}

/** 
 * Loads integer constants (aka uniforms) into the currently set GLSL program.
 * When @constants_set == NULL, it will load all the constants.
 */
void shader_glsl_load_constantsI(
    IWineD3DBaseShaderImpl* This,
    WineD3D_GL_Info *gl_info,
    GLhandleARB programId,
    unsigned max_constants,
    int* constants,
    BOOL* constants_set) {
    
    GLhandleARB tmp_loc;
    int i;
    char tmp_name[7];
    char is_pshader = shader_is_pshader_version(This->baseShader.hex_version);
    const char* prefix = is_pshader? "PI":"VI";
    struct list* ptr;

    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {

            TRACE("Loading constants %i: %i, %i, %i, %i\n",
                  i, constants[i*4], constants[i*4+1], constants[i*4+2], constants[i*4+3]);

            /* TODO: Benchmark and see if it would be beneficial to store the 
             * locations of the constants to avoid looking up each time */
            snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, i);
            tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform4ivARB(tmp_loc, 1, &constants[i*4]));
                checkGLcall("glUniform4ivARB");
            }
        }
    }

    /* Load immediate constants */
    ptr = list_head(&This->baseShader.constantsI);
    while (ptr) {
        local_constant* lconst = LIST_ENTRY(ptr, struct local_constant, entry);
        unsigned int idx = lconst->idx;
        GLint* values = (GLint*) lconst->value;

        TRACE("Loading local constants %i: %i, %i, %i, %i\n", idx,
            values[0], values[1], values[2], values[3]);

        snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform4ivARB(tmp_loc, 1, values));
            checkGLcall("glUniform4ivARB");
        }
        ptr = list_next(&This->baseShader.constantsI, ptr);
    }
}

/** 
 * Loads boolean constants (aka uniforms) into the currently set GLSL program.
 * When @constants_set == NULL, it will load all the constants.
 */
void shader_glsl_load_constantsB(
    IWineD3DBaseShaderImpl* This,
    WineD3D_GL_Info *gl_info,
    GLhandleARB programId,
    unsigned max_constants,
    BOOL* constants,
    BOOL* constants_set) {
    
    GLhandleARB tmp_loc;
    int i;
    char tmp_name[7];
    char is_pshader = shader_is_pshader_version(This->baseShader.hex_version);
    const char* prefix = is_pshader? "PB":"VB";
    struct list* ptr;

    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {

            TRACE("Loading constants %i: %i;\n", i, constants[i*4]);

            /* TODO: Benchmark and see if it would be beneficial to store the 
             * locations of the constants to avoid looking up each time */
            snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, i);
            tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, &constants[i*4]));
                checkGLcall("glUniform1ivARB");
            }
        }
    }

    /* Load immediate constants */
    ptr = list_head(&This->baseShader.constantsB);
    while (ptr) {
        local_constant* lconst = LIST_ENTRY(ptr, struct local_constant, entry);
        unsigned int idx = lconst->idx;
        GLint* values = (GLint*) lconst->value;

        TRACE("Loading local constants %i: %i\n", idx, values[0]);

        snprintf(tmp_name, sizeof(tmp_name), "%s[%i]", prefix, idx);
        tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform1ivARB(tmp_loc, 1, values));
            checkGLcall("glUniform1ivARB");
        }
        ptr = list_next(&This->baseShader.constantsB, ptr);
    }
}



/**
 * Loads the app-supplied constants into the currently set GLSL program.
 */
void shader_glsl_load_constants(
    IWineD3DStateBlock* iface,
    char usePixelShader,
    char useVertexShader) {
    
    IWineD3DStateBlockImpl* stateBlock = (IWineD3DStateBlockImpl*) iface;
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl*)stateBlock->wineD3DDevice->wineD3D)->gl_info;
    GLhandleARB programId = stateBlock->shaderPrgId;
    
    if (programId == 0) {
        /* No GLSL program set - nothing to do. */
        return;
    }

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;
        IWineD3DVertexShaderImpl* vshader_impl = (IWineD3DVertexShaderImpl*) vshader;

        IWineD3DVertexDeclarationImpl* vertexDeclaration =
            (IWineD3DVertexDeclarationImpl*) vshader_impl->vertexDeclaration;

        if (NULL != vertexDeclaration && NULL != vertexDeclaration->constants) {
            /* Load DirectX 8 float constants/uniforms for vertex shader */
            shader_glsl_load_constantsF(vshader, gl_info, programId, GL_LIMITS(vshader_constantsF),
                                        vertexDeclaration->constants, NULL);
        }

        /* Load DirectX 9 float constants/uniforms for vertex shader */
        shader_glsl_load_constantsF(vshader, gl_info, programId, GL_LIMITS(vshader_constantsF),
                                    stateBlock->vertexShaderConstantF, 
                                    stateBlock->set.vertexShaderConstantsF);

        /* Load DirectX 9 integer constants/uniforms for vertex shader */
        shader_glsl_load_constantsI(vshader, gl_info, programId, MAX_CONST_I,
                                    stateBlock->vertexShaderConstantI,
                                    stateBlock->set.vertexShaderConstantsI);

        /* Load DirectX 9 boolean constants/uniforms for vertex shader */
        shader_glsl_load_constantsB(vshader, gl_info, programId, MAX_CONST_B,
                                    stateBlock->vertexShaderConstantB,
                                    stateBlock->set.vertexShaderConstantsB);
    }

    if (usePixelShader) {

        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;

        /* Load pixel shader samplers */
        shader_glsl_load_psamplers(gl_info, iface);

        /* Load DirectX 9 float constants/uniforms for pixel shader */
        shader_glsl_load_constantsF(pshader, gl_info, programId, GL_LIMITS(pshader_constantsF),
                                    stateBlock->pixelShaderConstantF,
                                    stateBlock->set.pixelShaderConstantsF);

        /* Load DirectX 9 integer constants/uniforms for pixel shader */
        shader_glsl_load_constantsI(pshader, gl_info, programId, MAX_CONST_I,
                                    stateBlock->pixelShaderConstantI, 
                                    stateBlock->set.pixelShaderConstantsI);
        
        /* Load DirectX 9 boolean constants/uniforms for pixel shader */
        shader_glsl_load_constantsB(pshader, gl_info, programId, MAX_CONST_B,
                                    stateBlock->pixelShaderConstantB, 
                                    stateBlock->set.pixelShaderConstantsB);
    }
}

/** Generate the variable & register declarations for the GLSL output target */
void shader_generate_glsl_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer,
    WineD3D_GL_Info* gl_info) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    int i;

    /* There are some minor differences between pixel and vertex shaders */
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    char prefix = pshader ? 'P' : 'V';

    /* Prototype the subroutines */
    for (i = 0; i < This->baseShader.limits.label; i++) {
        if (reg_maps->labels[i])
            shader_addline(buffer, "void subroutine%lu();\n", i);
    }

    /* Declare the constants (aka uniforms) */
    if (This->baseShader.limits.constant_float > 0) {
        unsigned max_constantsF = min(This->baseShader.limits.constant_float, 
                (pshader ? GL_LIMITS(pshader_constantsF) : GL_LIMITS(vshader_constantsF)));
        shader_addline(buffer, "uniform vec4 %cC[%u];\n", prefix, max_constantsF);
    }

    if (This->baseShader.limits.constant_int > 0)
        shader_addline(buffer, "uniform ivec4 %cI[%u];\n", prefix, This->baseShader.limits.constant_int);

    if (This->baseShader.limits.constant_bool > 0)
        shader_addline(buffer, "uniform bool %cB[%u];\n", prefix, This->baseShader.limits.constant_bool);

    /* Declare texture samplers */ 
    for (i = 0; i < This->baseShader.limits.sampler; i++) {
        if (reg_maps->samplers[i]) {

            DWORD stype = reg_maps->samplers[i] & D3DSP_TEXTURETYPE_MASK;
            switch (stype) {

                case D3DSTT_2D:
                    shader_addline(buffer, "uniform sampler2D %csampler%lu;\n", prefix, i);
                    break;
                case D3DSTT_CUBE:
                    shader_addline(buffer, "uniform samplerCube %csampler%lu;\n", prefix, i);
                    break;
                case D3DSTT_VOLUME:
                    shader_addline(buffer, "uniform sampler3D %csampler%lu;\n", prefix, i);
                    break;
                default:
                    shader_addline(buffer, "uniform unsupported_sampler %csampler%lu;\n", prefix, i);
                    FIXME("Unrecognized sampler type: %#lx\n", stype);
                    break;
            }
        }
    }
    
    /* Declare address variables */
    for (i = 0; i < This->baseShader.limits.address; i++) {
        if (reg_maps->address[i])
            shader_addline(buffer, "ivec4 A%ld;\n", i);
    }

    /* Declare texture coordinate temporaries and initialize them */
    for (i = 0; i < This->baseShader.limits.texcoord; i++) {
        if (reg_maps->texcoord[i]) 
            shader_addline(buffer, "vec4 T%lu = gl_TexCoord[%lu];\n", i, i);
    }

    /* Declare input register temporaries */
    for (i=0; i < This->baseShader.limits.packed_input; i++) {
        if (reg_maps->packed_input[i])
            shader_addline(buffer, "vec4 IN%lu;\n", i);
    }

    /* Declare output register temporaries */
    for (i = 0; i < This->baseShader.limits.packed_output; i++) {
        if (reg_maps->packed_output[i])
            shader_addline(buffer, "vec4 OUT%lu;\n", i);
    }

    /* Declare temporary variables */
    for(i = 0; i < This->baseShader.limits.temporary; i++) {
        if (reg_maps->temporary[i])
            shader_addline(buffer, "vec4 R%lu;\n", i);
    }

    /* Declare attributes */
    for (i = 0; i < This->baseShader.limits.attributes; i++) {
        if (reg_maps->attributes[i])
            shader_addline(buffer, "attribute vec4 attrib%i;\n", i);
    }

    /* Declare loop register aL */
    if (reg_maps->loop) {
        shader_addline(buffer, "int aL;\n");
        shader_addline(buffer, "int tmpInt;\n");
    }
    
    /* Temporary variables for matrix operations */
    shader_addline(buffer, "vec4 tmp0;\n");
    shader_addline(buffer, "vec4 tmp1;\n");

    /* Start the main program */
    shader_addline(buffer, "void main() {\n");
}

/*****************************************************************************
 * Functions to generate GLSL strings from DirectX Shader bytecode begin here.
 *
 * For more information, see http://wiki.winehq.org/DirectX-Shaders
 ****************************************************************************/

/* Prototypes */
static void shader_glsl_add_param(
    SHADER_OPCODE_ARG* arg,
    const DWORD param,
    const DWORD addr_token,
    BOOL is_input,
    char *reg_name,
    char *reg_mask,
    char *out_str);

/** Used for opcode modifiers - They multiply the result by the specified amount */
static const char* shift_glsl_tab[] = {
    "",           /*  0 (none) */ 
    "2.0 * ",     /*  1 (x2)   */ 
    "4.0 * ",     /*  2 (x4)   */ 
    "8.0 * ",     /*  3 (x8)   */ 
    "16.0 * ",    /*  4 (x16)  */ 
    "32.0 * ",    /*  5 (x32)  */ 
    "",           /*  6 (x64)  */ 
    "",           /*  7 (x128) */ 
    "",           /*  8 (d256) */ 
    "",           /*  9 (d128) */ 
    "",           /* 10 (d64)  */ 
    "",           /* 11 (d32)  */ 
    "0.0625 * ",  /* 12 (d16)  */ 
    "0.125 * ",   /* 13 (d8)   */ 
    "0.25 * ",    /* 14 (d4)   */ 
    "0.5 * "      /* 15 (d2)   */ 
};

/** Print the beginning of the generated GLSL string. example: "reg_name.xyzw = vec4("
 * Will also change the reg_mask if necessary (not all register types are equal in DX vs GL)  */
static void shader_glsl_add_dst(DWORD param, const char* reg_name, char* reg_mask, char* outStr) {

    int shift = (param & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
    char cast[6];
    
    if ((shader_get_regtype(param) == D3DSPR_RASTOUT)
         && ((param & D3DSP_REGNUM_MASK) != 0)) {
        /* gl_FogFragCoord or glPointSize - both floats */
        strcpy(cast, "float");
        strcpy(reg_mask, "");

    } else if (reg_name[0] == 'A') {
        /* Address register for vertex shaders (ivec4) */
        strcpy(cast, "ivec4");
        
    } else {
        /* Everything else should be a 4 component float vector */
        strcpy(cast, "vec4");
    }
    
    sprintf(outStr, "%s%s = %s%s(", reg_name, reg_mask, shift_glsl_tab[shift], cast); 
}

/* Generate a GLSL parameter that does the input modifier computation and return the input register/mask to use */
static void shader_glsl_gen_modifier (
    const DWORD instr,
    const char *in_reg,
    const char *in_regswizzle,
    char *out_str) {

    out_str[0] = 0;
    
    if (instr == D3DSIO_TEXKILL)
        return;

    switch (instr & D3DSP_SRCMOD_MASK) {
    case D3DSPSM_NONE:
        sprintf(out_str, "%s%s", in_reg, in_regswizzle);
        break;
    case D3DSPSM_NEG:
        sprintf(out_str, "-%s%s", in_reg, in_regswizzle);
        break;
    case D3DSPSM_NOT:
        sprintf(out_str, "!%s%s", in_reg, in_regswizzle);
        break;
    case D3DSPSM_BIAS:
        sprintf(out_str, "(%s%s - vec4(0.5)%s)", in_reg, in_regswizzle, in_regswizzle);
        break;
    case D3DSPSM_BIASNEG:
        sprintf(out_str, "-(%s%s - vec4(0.5)%s)", in_reg, in_regswizzle, in_regswizzle);
        break;
    case D3DSPSM_SIGN:
        sprintf(out_str, "(2.0 * (%s%s - 0.5))", in_reg, in_regswizzle);
        break;
    case D3DSPSM_SIGNNEG:
        sprintf(out_str, "-(2.0 * (%s%s - 0.5))", in_reg, in_regswizzle);
        break;
    case D3DSPSM_COMP:
        sprintf(out_str, "(1.0 - %s%s)", in_reg, in_regswizzle);
        break;
    case D3DSPSM_X2:
        sprintf(out_str, "(2.0 * %s%s)", in_reg, in_regswizzle);
        break;
    case D3DSPSM_X2NEG:
        sprintf(out_str, "-(2.0 * %s%s)", in_reg, in_regswizzle);
        break;
    case D3DSPSM_DZ:    /* reg1_db = { reg1.r/b, reg1.g/b, ...}  The g & a components are undefined, so we'll leave them alone */
        sprintf(out_str, "vec4(%s.r / %s.b, %s.g / %s.b, %s.b, %s.a)", in_reg, in_reg, in_reg, in_reg, in_reg, in_reg);
        break;
    case D3DSPSM_DW:
        sprintf(out_str, "vec4(%s.r / %s.a, %s.g / %s.a, %s.b, %s.a)", in_reg, in_reg, in_reg, in_reg, in_reg, in_reg);
        break;
    case D3DSPSM_ABS:
        sprintf(out_str, "abs(%s%s)", in_reg, in_regswizzle);
        break;
    case D3DSPSM_ABSNEG:
        sprintf(out_str, "-abs(%s%s)", in_reg, in_regswizzle);
        break;
    default:
        FIXME("Unhandled modifier %lu\n", (instr & D3DSP_SRCMOD_MASK));
        sprintf(out_str, "%s%s", in_reg, in_regswizzle);
    }
}

/** Writes the GLSL variable name that corresponds to the register that the
 * DX opcode parameter is trying to access */
static void shader_glsl_get_register_name(
    const DWORD param,
    const DWORD addr_token,
    char* regstr,
    BOOL* is_color,
    SHADER_OPCODE_ARG* arg) {

    /* oPos, oFog and oPts in D3D */
    const char* hwrastout_reg_names[] = { "gl_Position", "gl_FogFragCoord", "gl_PointSize" };

    DWORD reg = param & D3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) arg->shader;
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    char tmpStr[50];

    *is_color = FALSE;   
 
    switch (regtype) {
    case D3DSPR_TEMP:
        sprintf(tmpStr, "R%lu", reg);
    break;
    case D3DSPR_INPUT:
        if (pshader) {
            /* Pixel shaders >= 3.0 */
            if (D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 3)
                sprintf(tmpStr, "IN%lu", reg);
             else {
                if (reg==0)
                    strcpy(tmpStr, "gl_Color");
                else
                    strcpy(tmpStr, "gl_SecondaryColor");
            }
        } else {
            if (vshader_input_is_color((IWineD3DVertexShader*) This, reg))
               *is_color = TRUE;
            sprintf(tmpStr, "attrib%lu", reg);
        } 
        break;
    case D3DSPR_CONST:
    {
        const char* prefix = pshader? "PC":"VC";

        /* Relative addressing */
        if (param & D3DVS_ADDRMODE_RELATIVE) {

           /* Relative addressing on shaders 2.0+ have a relative address token, 
            * prior to that, it was hard-coded as "A0.x" because there's only 1 register */
           if (D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 2)  {
               char relStr[100], relReg[50], relMask[6];
               shader_glsl_add_param(arg, addr_token, 0, TRUE, relReg, relMask, relStr);
               sprintf(tmpStr, "%s[%s + %lu]", prefix, relStr, reg);
           } else
               sprintf(tmpStr, "%s[A0.x + %lu]", prefix, reg);

        } else
             sprintf(tmpStr, "%s[%lu]", prefix, reg);

        break;
    }
    case D3DSPR_CONSTINT:
        if (pshader)
            sprintf(tmpStr, "PI[%lu]", reg);
        else
            sprintf(tmpStr, "VI[%lu]", reg);
        break;
    case D3DSPR_CONSTBOOL:
        if (pshader)
            sprintf(tmpStr, "PB[%lu]", reg);
        else
            sprintf(tmpStr, "VB[%lu]", reg);
        break;
    case D3DSPR_TEXTURE: /* case D3DSPR_ADDR: */
        if (pshader) {
            sprintf(tmpStr, "T%lu", reg);
        } else {
            sprintf(tmpStr, "A%lu", reg);
        }
    break;
    case D3DSPR_LOOP:
        sprintf(tmpStr, "aL");
    break;
    case D3DSPR_SAMPLER:
        if (pshader)
            sprintf(tmpStr, "Psampler%lu", reg);
        else
            sprintf(tmpStr, "Vsampler%lu", reg);
    break;
    case D3DSPR_COLOROUT:
        if (reg == 0)
            sprintf(tmpStr, "gl_FragColor");
        else {
            /* TODO: See GL_ARB_draw_buffers */
            FIXME("Unsupported write to render target %lu\n", reg);
            sprintf(tmpStr, "unsupported_register");
        }
    break;
    case D3DSPR_RASTOUT:
        sprintf(tmpStr, "%s", hwrastout_reg_names[reg]);
    break;
    case D3DSPR_DEPTHOUT:
        sprintf(tmpStr, "gl_FragDepth");
    break;
    case D3DSPR_ATTROUT:
        if (reg == 0) {
            sprintf(tmpStr, "gl_FrontColor");
        } else {
            sprintf(tmpStr, "gl_FrontSecondaryColor");
        }
    break;
    case D3DSPR_TEXCRDOUT:
        /* Vertex shaders >= 3.0: D3DSPR_OUTPUT */
        if (D3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 3)
            sprintf(tmpStr, "OUT%lu", reg);
        else
            sprintf(tmpStr, "gl_TexCoord[%lu]", reg);
    break;
    default:
        FIXME("Unhandled register name Type(%ld)\n", regtype);
        sprintf(tmpStr, "unrecognized_register");
    break;
    }

    strcat(regstr, tmpStr);
}

/* Writes the GLSL writemask for the destination register */
static void shader_glsl_get_output_register_swizzle(
    const DWORD param,
    char *write_mask) {
   
    *write_mask = 0;
    if ((param & D3DSP_WRITEMASK_ALL) != D3DSP_WRITEMASK_ALL) {
        strcat(write_mask, ".");
        if (param & D3DSP_WRITEMASK_0) strcat(write_mask, "x");
        if (param & D3DSP_WRITEMASK_1) strcat(write_mask, "y");
        if (param & D3DSP_WRITEMASK_2) strcat(write_mask, "z");
        if (param & D3DSP_WRITEMASK_3) strcat(write_mask, "w");
    }
}

static void shader_glsl_get_input_register_swizzle(
    const DWORD param,
    BOOL is_color,
    char *reg_mask) {
    
    const char swizzle_reg_chars_color_fix[] = "zyxw";
    const char swizzle_reg_chars[] = "xyzw";
    const char* swizzle_regs = NULL;
   
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
	    sprintf(reg_mask, ".%c%c%c%c",
		swizzle_regs[swizzle_x],
		swizzle_regs[swizzle_y],
		swizzle_regs[swizzle_z],
		swizzle_regs[swizzle_w]);
      }
      return ;
    }
    if (swizzle_x == swizzle_y &&
	swizzle_x == swizzle_z &&
	swizzle_x == swizzle_w)
    {
      sprintf(reg_mask, ".%c", swizzle_regs[swizzle_x]);
    } else {
      sprintf(reg_mask, ".%c%c%c%c",
	      swizzle_regs[swizzle_x],
	      swizzle_regs[swizzle_y],
	      swizzle_regs[swizzle_z],
	      swizzle_regs[swizzle_w]);
    }
}

/** From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the 
 * caller needs this information as well. */
static void shader_glsl_add_param(
    SHADER_OPCODE_ARG* arg,
    const DWORD param,
    const DWORD addr_token,
    BOOL is_input,
    char *reg_name,
    char *reg_mask,
    char *out_str) {

    BOOL is_color = FALSE;
    reg_mask[0] = reg_name[0] = out_str[0] = 0;

    shader_glsl_get_register_name(param, addr_token, reg_name, &is_color, arg);
    
    if (is_input) {
        shader_glsl_get_input_register_swizzle(param, is_color, reg_mask);
        shader_glsl_gen_modifier(param, reg_name, reg_mask, out_str);
    } else {
        shader_glsl_get_output_register_swizzle(param, reg_mask);
        sprintf(out_str, "%s%s", reg_name, reg_mask);
    }
}

/** Process GLSL instruction modifiers */
void shader_glsl_add_instruction_modifiers(SHADER_OPCODE_ARG* arg) {
    
    DWORD mask = arg->dst & D3DSP_DSTMOD_MASK;
 
    if (arg->opcode->dst_token && mask != 0) {
        char dst_reg[50];
        char dst_mask[6];
        char dst_str[100];
       
        shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);

        if (mask & D3DSPDM_SATURATE) {
            /* _SAT means to clamp the value of the register to between 0 and 1 */
            shader_addline(arg->buffer, "%s%s = clamp(%s%s, 0.0, 1.0);\n", dst_reg, dst_mask, dst_reg, dst_mask);
        }
        if (mask & D3DSPDM_MSAMPCENTROID) {
            FIXME("_centroid modifier not handled\n");
        }
        if (mask & D3DSPDM_PARTIALPRECISION) {
            /* MSDN says this modifier can be safely ignored, so that's what we'll do. */
        }
    }
}

static inline const char* shader_get_comp_op(
    const DWORD opcode) {

    DWORD op = (opcode & INST_CONTROLS_MASK) >> INST_CONTROLS_SHIFT;
    switch (op) {
        case COMPARISON_GT: return ">";
        case COMPARISON_EQ: return "==";
        case COMPARISON_GE: return ">=";
        case COMPARISON_LT: return "<";
        case COMPARISON_NE: return "!=";
        case COMPARISON_LE: return "<=";
        default:
            FIXME("Unrecognized comparison value: %lu\n", op);
            return "(\?\?)";
    }
}

/*****************************************************************************
 * 
 * Begin processing individual instruction opcodes
 * 
 ****************************************************************************/

/* Generate GLSL arithmetic functions (dst = src1 + src2) */
void shader_glsl_arith(SHADER_OPCODE_ARG* arg) {

    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    char tmpLine[256];
    char dst_reg[50], src0_reg[50], src1_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6];
    char dst_str[100], src0_str[100], src1_str[100];

    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);
    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    strcat(tmpLine, "vec4(");
    strcat(tmpLine, src0_str);
    strcat(tmpLine, ")");

    /* Determine the GLSL operator to use based on the opcode */
    switch (curOpcode->opcode) {
        case D3DSIO_MUL:    strcat(tmpLine, " * "); break;
        case D3DSIO_ADD:    strcat(tmpLine, " + "); break;
        case D3DSIO_SUB:    strcat(tmpLine, " - "); break;
        default:
            FIXME("Opcode %s not yet handled in GLSL\n", curOpcode->name);
            break;
    }
    shader_addline(buffer, "%svec4(%s))%s;\n", tmpLine, src1_str, dst_mask);
}

/* Process the D3DSIO_MOV opcode using GLSL (dst = src) */
void shader_glsl_mov(SHADER_OPCODE_ARG* arg) {

    SHADER_BUFFER* buffer = arg->buffer;
    char tmpLine[256];
    char dst_str[100], src0_str[100];
    char dst_reg[50], src0_reg[50];
    char dst_mask[6], src0_mask[6];

    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    shader_addline(buffer, "%s%s)%s;\n", tmpLine, src0_str, dst_mask);
}

/* Process the dot product operators DP3 and DP4 in GLSL (dst = dot(src0, src1)) */
void shader_glsl_dot(SHADER_OPCODE_ARG* arg) {

    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    char tmpDest[100];
    char dst_str[100], src0_str[100], src1_str[100];
    char dst_reg[50], src0_reg[50], src1_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6];
    char cast[6];

    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);

    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpDest);
 
    /* Need to cast the src vectors to vec3 for dp3, and vec4 for dp4 */
    if (curOpcode->opcode == D3DSIO_DP4)
        strcpy(cast, "vec4(");
    else
        strcpy(cast, "vec3(");
    
    shader_addline(buffer, "%sdot(%s%s), %s%s)))%s;\n",
                   tmpDest, cast, src0_str, cast, src1_str, dst_mask);
}

/* Map the opcode 1-to-1 to the GL code (arg->dst = instruction(src0, src1, ...) */
void shader_glsl_map2gl(SHADER_OPCODE_ARG* arg) {

    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    char tmpLine[256];
    char dst_str[100], src_str[100];
    char dst_reg[50], src_reg[50];
    char dst_mask[6], src_mask[6];
    unsigned i;
    
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);

    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
 
    /* Determine the GLSL function to use based on the opcode */
    /* TODO: Possibly make this a table for faster lookups */
    switch (curOpcode->opcode) {
            case D3DSIO_MIN:    strcat(tmpLine, "min"); break;
            case D3DSIO_MAX:    strcat(tmpLine, "max"); break;
            case D3DSIO_RSQ:    strcat(tmpLine, "inversesqrt"); break;
            case D3DSIO_ABS:    strcat(tmpLine, "abs"); break;
            case D3DSIO_FRC:    strcat(tmpLine, "fract"); break;
            case D3DSIO_POW:    strcat(tmpLine, "pow"); break;
            case D3DSIO_CRS:    strcat(tmpLine, "cross"); break;
            case D3DSIO_NRM:    strcat(tmpLine, "normalize"); break;
            case D3DSIO_LOGP:
            case D3DSIO_LOG:    strcat(tmpLine, "log2"); break;
            case D3DSIO_EXPP:
            case D3DSIO_EXP:    strcat(tmpLine, "exp2"); break;
            case D3DSIO_SGE:    strcat(tmpLine, "greaterThanEqual"); break;
            case D3DSIO_SLT:    strcat(tmpLine, "lessThan"); break;
            case D3DSIO_SGN:    strcat(tmpLine, "sign"); break;
        default:
            FIXME("Opcode %s not yet handled in GLSL\n", curOpcode->name);
            break;
    }

    strcat(tmpLine, "(");

    if (curOpcode->num_params > 0) {
        strcat(tmpLine, "vec4(");
        shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src_reg, src_mask, src_str);
        strcat(tmpLine, src_str);
        strcat(tmpLine, ")");
        for (i = 2; i < curOpcode->num_params; ++i) {
            strcat(tmpLine, ", vec4(");
            shader_glsl_add_param(arg, arg->src[i-1], arg->src_addr[i-1], TRUE, src_reg, src_mask, src_str);
            strcat(tmpLine, src_str);
            strcat(tmpLine, ")");
        }
    }
    shader_addline(buffer, "%s))%s;\n", tmpLine, dst_mask);

}

/** Process the RCP (reciprocal or inverse) opcode in GLSL (dst = 1 / src) */
void shader_glsl_rcp(SHADER_OPCODE_ARG* arg) {

    char tmpLine[256];
    char dst_str[100], src_str[100];
    char dst_reg[50], src_reg[50];
    char dst_mask[6], src_mask[6];
    
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src_reg, src_mask, src_str);
    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    strcat(tmpLine, "1.0 / ");
    shader_addline(arg->buffer, "%s%s)%s;\n", tmpLine, src_str, dst_mask);
}

/** Process signed comparison opcodes in GLSL. */
void shader_glsl_compare(SHADER_OPCODE_ARG* arg) {

    char tmpLine[256];
    char dst_str[100], src0_str[100], src1_str[100];
    char dst_reg[50], src0_reg[50], src1_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6];
    
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);

    /* If we are comparing vectors and not scalars, we should process this through map2gl using the GLSL functions. */
    if (strlen(src0_mask) != 2) {
        shader_glsl_map2gl(arg);
    } else {
        char compareStr[3];
        compareStr[0] = 0;
        shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);

        switch (arg->opcode->opcode) {
            case D3DSIO_SLT:    strcpy(compareStr, "<"); break;
            case D3DSIO_SGE:    strcpy(compareStr, ">="); break;
            default:
                FIXME("Can't handle opcode %s\n", arg->opcode->name);
        }
        shader_addline(arg->buffer, "%s(float(%s) %s float(%s)) ? 1.0 : 0.0)%s;\n",
                       tmpLine, src0_str, compareStr, src1_str, dst_mask);
    }
}

/** Process CMP instruction in GLSL (dst = src0.x > 0.0 ? src1.x : src2.x), per channel */
void shader_glsl_cmp(SHADER_OPCODE_ARG* arg) {

    char tmpLine[256];
    char dst_str[100], src0_str[100], src1_str[100], src2_str[100];
    char dst_reg[50], src0_reg[50], src1_reg[50], src2_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6], src2_mask[6];
   
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);
    shader_glsl_add_param(arg, arg->src[2], arg->src_addr[2], TRUE, src2_reg, src2_mask, src2_str);

    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    shader_addline(arg->buffer, "%smix(vec4(%s), vec4(%s), vec4(lessThan(vec4(%s), vec4(0.0)))))%s;\n",
        tmpLine, src1_str, src2_str, src0_str, dst_mask);
}

/** Process the CND opcode in GLSL (dst = (src0 < 0.5) ? src1 : src2) */
void shader_glsl_cnd(SHADER_OPCODE_ARG* arg) {

    char tmpLine[256];
    char dst_str[100], src0_str[100], src1_str[100], src2_str[100];
    char dst_reg[50], src0_reg[50], src1_reg[50], src2_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6], src2_mask[6];
 
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);
    shader_glsl_add_param(arg, arg->src[2], arg->src_addr[2], TRUE, src2_reg, src2_mask, src2_str);   
    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    shader_addline(arg->buffer, "%s(%s < 0.5) ? %s : %s)%s;\n", 
                   tmpLine, src0_str, src1_str, src2_str, dst_mask);
}

/** GLSL code generation for D3DSIO_MAD: Multiply the first 2 opcodes, then add the last */
void shader_glsl_mad(SHADER_OPCODE_ARG* arg) {

    char tmpLine[256];
    char dst_str[100], src0_str[100], src1_str[100], src2_str[100];
    char dst_reg[50], src0_reg[50], src1_reg[50], src2_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6], src2_mask[6];
    
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);
    shader_glsl_add_param(arg, arg->src[2], arg->src_addr[2], TRUE, src2_reg, src2_mask, src2_str);     
    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);

    shader_addline(arg->buffer, "%s(vec4(%s) * vec4(%s)) + vec4(%s))%s;\n",
                   tmpLine, src0_str, src1_str, src2_str, dst_mask);
}

/** Handles transforming all D3DSIO_M?x? opcodes for 
    Vertex shaders to GLSL codes */
void shader_glsl_mnxn(SHADER_OPCODE_ARG* arg) {
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
        tmpArg.src[1]      = arg->src[1]+i;
        shader_glsl_dot(&tmpArg);
    }
}

/**
    The LRP instruction performs a component-wise linear interpolation 
    between the second and third operands using the first operand as the
    blend factor.  Equation:  (dst = src2 * (src1 - src0) + src0)
*/
void shader_glsl_lrp(SHADER_OPCODE_ARG* arg) {

    char tmpLine[256];
    char dst_str[100], src0_str[100], src1_str[100], src2_str[100];
    char dst_reg[50], src0_reg[50], src1_reg[50], src2_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6], src2_mask[6];
   
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);
    shader_glsl_add_param(arg, arg->src[2], arg->src_addr[2], TRUE, src2_reg, src2_mask, src2_str);     

    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    
    shader_addline(arg->buffer, "%s%s + %s * (%s - %s))%s;\n",
                   tmpLine, src2_str, src0_str, src1_str, src2_str, dst_mask);
}

/** Process the D3DSIO_LIT instruction in GLSL:
 * dst.x = dst.w = 1.0
 * dst.y = (src0.x > 0) ? src0.x
 * dst.z = (src0.x > 0) ? ((src0.y > 0) ? pow(src0.y, src.w) : 0) : 0
 *                                        where src.w is clamped at +- 128
 */
void shader_glsl_lit(SHADER_OPCODE_ARG* arg) {

    char dst_str[100], src0_str[100];
    char dst_reg[50], src0_reg[50];
    char dst_mask[6], src0_mask[6];
   
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);

    shader_addline(arg->buffer,
        "%s = vec4(1.0, (%s.x > 0.0 ? %s.x : 0.0), (%s.x > 0.0 ? ((%s.y > 0.0) ? pow(%s.y, clamp(%s.w, -128.0, 128.0)) : 0.0) : 0.0), 1.0)%s;\n",
        dst_str, src0_reg, src0_reg, src0_reg, src0_reg, src0_reg, src0_reg, dst_mask);
}

/** Process the D3DSIO_DST instruction in GLSL:
 * dst.x = 1.0
 * dst.y = src0.x * src0.y
 * dst.z = src0.z
 * dst.w = src1.w
 */
void shader_glsl_dst(SHADER_OPCODE_ARG* arg) {

    char dst_str[100], src0_str[100], src1_str[100];
    char dst_reg[50], src0_reg[50], src1_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6];
   
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);

    shader_addline(arg->buffer, "%s = vec4(1.0, %s.x * %s.y, %s.z, %s.w)%s;\n",
                   dst_str, src0_reg, src1_reg, src0_reg, src1_reg, dst_mask);
}

/** Process the D3DSIO_SINCOS instruction in GLSL:
 * VS 2.0 requires that specific cosine and sine constants be passed to this instruction so the hardware
 * can handle it.  But, these functions are built-in for GLSL, so we can just ignore the last 2 params.
 * 
 * dst.x = cos(src0.?)
 * dst.y = sin(src0.?)
 * dst.z = dst.z
 * dst.w = dst.w
 */
void shader_glsl_sincos(SHADER_OPCODE_ARG* arg) {
    
    char dst_str[100], src0_str[100];
    char dst_reg[50], src0_reg[50];
    char dst_mask[6], src0_mask[6];
   
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);

    shader_addline(arg->buffer, "%s = vec4(cos(%s), sin(%s), %s.z, %s.w)%s;\n",
                   dst_str, src0_str, src0_str, dst_reg, dst_reg, dst_mask);
}

/** Process the D3DSIO_LOOP instruction in GLSL:
 * Start a for() loop where src0.y is the initial value of aL,
 *  increment aL by src0.z for a total of src0.x iterations.
 *  Need to use a temporary variable for this operation.
 */
void shader_glsl_loop(SHADER_OPCODE_ARG* arg) {
    
    char src1_str[100];
    char src1_reg[50];
    char src1_mask[6];
    
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);
  
    shader_addline(arg->buffer, "for (tmpInt = 0, aL = %s.y; tmpInt < %s.x; tmpInt++, aL += %s.z) {\n",
                   src1_reg, src1_reg, src1_reg);
}

void shader_glsl_end(SHADER_OPCODE_ARG* arg) {
    shader_addline(arg->buffer, "}\n");
}

void shader_glsl_rep(SHADER_OPCODE_ARG* arg) {

    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_addline(arg->buffer, "for (tmpInt = 0; tmpInt < %s.x; tmpInt++) {\n", src0_reg);
}

void shader_glsl_if(SHADER_OPCODE_ARG* arg) {

    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_addline(arg->buffer, "if (%s) {\n", src0_str); 
}

void shader_glsl_ifc(SHADER_OPCODE_ARG* arg) {

    char src0_str[100], src1_str[100];
    char src0_reg[50], src1_reg[50];
    char src0_mask[6], src1_mask[6];

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);

    shader_addline(arg->buffer, "if (%s %s %s) {\n",
        src0_str, shader_get_comp_op(arg->opcode_token), src1_str);
}

void shader_glsl_else(SHADER_OPCODE_ARG* arg) {
    shader_addline(arg->buffer, "} else {\n");
}

void shader_glsl_break(SHADER_OPCODE_ARG* arg) {
    shader_addline(arg->buffer, "break;\n");
}

void shader_glsl_breakc(SHADER_OPCODE_ARG* arg) {

    char src0_str[100], src1_str[100];
    char src0_reg[50], src1_reg[50];
    char src0_mask[6], src1_mask[6];

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);

    shader_addline(arg->buffer, "if (%s %s %s) break;\n",
        src0_str, shader_get_comp_op(arg->opcode_token), src1_str);
}

void shader_glsl_label(SHADER_OPCODE_ARG* arg) {

    DWORD snum = (arg->src[0]) & D3DSP_REGNUM_MASK;
    shader_addline(arg->buffer, "}\n");
    shader_addline(arg->buffer, "void subroutine%lu () {\n",  snum);
}

void shader_glsl_call(SHADER_OPCODE_ARG* arg) {
    DWORD snum = (arg->src[0]) & D3DSP_REGNUM_MASK;
    shader_addline(arg->buffer, "subroutine%lu();\n", snum);
}

void shader_glsl_callnz(SHADER_OPCODE_ARG* arg) {

    char src1_str[100];
    char src1_reg[50];
    char src1_mask[6];
   
    DWORD snum = (arg->src[0]) & D3DSP_REGNUM_MASK;
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);
    shader_addline(arg->buffer, "if (%s) subroutine%lu();\n", src1_str, snum);
}

/*********************************************
 * Pixel Shader Specific Code begins here
 ********************************************/
void pshader_glsl_tex(SHADER_OPCODE_ARG* arg) {

    /* FIXME: Make this work for more than just 2D textures */
    
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;

    char dst_str[100],   dst_reg[50],  dst_mask[6];
    char coord_str[100], coord_reg[50], coord_mask[6];
    char sampler_str[100], sampler_reg[50], sampler_mask[6];
    DWORD reg_dest_code = arg->dst & D3DSP_REGNUM_MASK;
    DWORD sampler_code, sampler_type;

    /* All versions have a destination register */
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
    if (hex_version < D3DPS_VERSION(1,4))
       strcpy(coord_reg, dst_reg);
    else
       shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, coord_reg, coord_mask, coord_str);

    /* 1.0-1.4: Use destination register as coordinate source.
     * 2.0+: Use provided coordinate source register. */
    if (hex_version < D3DPS_VERSION(2,0)) {
        sprintf(sampler_str, "Psampler%lu", reg_dest_code); 
        sampler_code = reg_dest_code;
    }       
    else {
        shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, sampler_reg, sampler_mask, sampler_str);
        sampler_code = arg->src[1] & D3DSP_REGNUM_MASK;
    }         

    sampler_type = arg->reg_maps->samplers[sampler_code] & D3DSP_TEXTURETYPE_MASK;
    switch(sampler_type) {

        case D3DSTT_2D:
            shader_addline(buffer, "%s = texture2D(%s, %s.st);\n", dst_str, sampler_str, coord_reg);
            break;
        case D3DSTT_CUBE:
            shader_addline(buffer, "%s = textureCube(%s, %s.stp);\n", dst_str, sampler_str, coord_reg);
            break;
        case D3DSTT_VOLUME:
            shader_addline(buffer, "%s = texture3D(%s, %s.stp);\n", dst_str, sampler_str, coord_reg);
            break;
        default:
            shader_addline(buffer, "%s = unrecognized_stype(%s, %s.stp);\n", dst_str, sampler_str, coord_reg);
            FIXME("Unrecognized sampler type: %#lx;\n", sampler_type);
            break;
    }
}

void pshader_glsl_texcoord(SHADER_OPCODE_ARG* arg) {

    /* FIXME: Make this work for more than just 2D textures */
    
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;

    char tmpStr[100];
    char tmpReg[50];
    char tmpMask[6];
    tmpReg[0] = 0;

    shader_glsl_add_param(arg, arg->dst, 0, FALSE, tmpReg, tmpMask, tmpStr);

    if (hex_version != D3DPS_VERSION(1,4)) {
        DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
        shader_addline(buffer, "%s = clamp(gl_TexCoord[%lu], 0.0, 1.0);\n", tmpReg, reg);
    } else {
        DWORD reg2 = arg->src[0] & D3DSP_REGNUM_MASK;
        shader_addline(buffer, "%s = gl_TexCoord[%lu]%s;\n", tmpStr, reg2, tmpMask);
   }
}

void pshader_glsl_texm3x2pad(SHADER_OPCODE_ARG* arg) {

    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_name, src0_mask, src0_str);
    shader_addline(buffer, "tmp0.x = dot(vec3(T%lu), vec3(%s));\n", reg, src0_name, src0_str);
}

/** Process the D3DSIO_TEXM3X3PAD instruction in GLSL
 * Calculate the 1st or 2nd row of a 3-row matrix multiplication. */
void pshader_glsl_texm3x3pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_name, src0_mask, src0_str);
    shader_addline(buffer, "tmp0.%c = dot(vec3(T%lu), vec3(%s));\n", 'x' + current_state->current_row, reg, src0_str);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

void pshader_glsl_texm3x2tex(SHADER_OPCODE_ARG* arg) {

    /* FIXME: Make this work for more than just 2D textures */
    
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_name, src0_mask, src0_str);
    shader_addline(buffer, "tmp0.y = dot(vec3(T%lu), vec3(%s));\n", reg, src0_str);
    shader_addline(buffer, "T%lu = texture2D(Psampler%lu, tmp0.st);\n", reg, reg);
}

/** Process the D3DSIO_TEXM3X3TEX instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply, then sample the texture using the calculate coordinates */
void pshader_glsl_texm3x3tex(SHADER_OPCODE_ARG* arg) {

    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];
    char dimensions[5];
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    DWORD src0_regnum = arg->src[0] & D3DSP_REGNUM_MASK;
    DWORD stype = arg->reg_maps->samplers[src0_regnum] & D3DSP_TEXTURETYPE_MASK;
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    
    switch (stype) {
        case D3DSTT_2D:     strcpy(dimensions, "2D");   break;
        case D3DSTT_CUBE:   strcpy(dimensions, "Cube"); break;
        case D3DSTT_VOLUME: strcpy(dimensions, "3D");   break;
        default:
            strcpy(dimensions, ""); break;
            FIXME("Unrecognized sampler type: %#lx\n", stype);
            break;
    }

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_name, src0_mask, src0_str);
    shader_addline(arg->buffer, "tmp0.z = dot(vec3(T%lu), vec3(%s));\n", reg, src0_str);
    shader_addline(arg->buffer, "T%lu = texture%s(Psampler%lu, tmp0.%s);\n", 
            reg, dimensions, reg, (stype == D3DSTT_2D) ? "xy" : "xyz");
    current_state->current_row = 0;
}

/** Process the D3DSIO_TEXM3X3SPEC instruction in GLSL 
 * Peform the final texture lookup based on the previous 2 3x3 matrix multiplies */
void pshader_glsl_texm3x3spec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    char dimensions[5];
    char src0_str[100], src0_name[50], src0_mask[6];
    char src1_str[100], src1_name[50], src1_mask[6];
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    DWORD stype = arg->reg_maps->samplers[reg] & D3DSP_TEXTURETYPE_MASK;

    switch (stype) {
        case D3DSTT_2D:     strcpy(dimensions, "2D");   break;
        case D3DSTT_CUBE:   strcpy(dimensions, "Cube"); break;
        case D3DSTT_VOLUME: strcpy(dimensions, "3D");   break;
        default:
            strcpy(dimensions, ""); break;
            FIXME("Unrecognized sampler type: %#lx\n", stype);
            break;
    }

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_name, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_name, src1_mask, src1_str);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(vec3(T%lu), vec3(%s));\n", reg, src0_str);

    /* Calculate reflection vector */
    shader_addline(buffer, "tmp0.xyz = reflect(-vec3(%s), vec3(tmp0));\n", src1_str);

    /* Sample the texture */
    shader_addline(buffer, "T%lu = texture%s(Psampler%lu, tmp0.%s);\n", 
            reg, dimensions, reg, (stype == D3DSTT_2D) ? "xy" : "xyz");
    current_state->current_row = 0;
}

/** Process the D3DSIO_TEXM3X3VSPEC instruction in GLSL 
 * Peform the final texture lookup based on the previous 2 3x3 matrix multiplies */
void pshader_glsl_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & D3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    char src0_str[100], src0_name[50], src0_mask[6];

    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_name, src0_mask, src0_str);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(vec3(T%lu), vec3(%s));\n", reg, src0_str);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "tmp1.x = gl_TexCoord[%lu].w;\n", current_state->texcoord_w[0]);
    shader_addline(buffer, "tmp1.y = gl_TexCoord[%lu].w;\n", current_state->texcoord_w[1]);
    shader_addline(buffer, "tmp1.z = gl_TexCoord[%lu].w;\n", reg);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
    shader_addline(buffer, "tmp0.x = dot(vec3(tmp0), vec3(tmp1));\n");
    shader_addline(buffer, "tmp0 = tmp0.w * tmp0;\n");
    shader_addline(buffer, "tmp0 = (2.0 * tmp0) - tmp1;\n");

    /* FIXME:
     * We don't really know if a Cube or a Volume texture is being sampled, but since Cube textures
     * are used more commonly, we'll default to that.
     * We probably need to push back the pixel shader generation code until drawPrimitive() for 
     * shader versions < 2.0, since that's the only time we can guarantee that we're sampling
     * the correct type of texture because we can lookup what textures are bound at that point.
     */
    shader_addline(buffer, "T%lu = textureCube(Psampler%lu, tmp0.xyz);\n", reg, reg);
    current_state->current_row = 0;
}

/** Process the D3DSIO_TEXBEM instruction in GLSL.
 * Apply a fake bump map transform.
 * FIXME: Should apply the BUMPMAPENV matrix.  For now, just sample the texture */
void pshader_glsl_texbem(SHADER_OPCODE_ARG* arg) {

    DWORD reg1 = arg->dst & D3DSP_REGNUM_MASK;
    DWORD reg2 = arg->src[0] & D3DSP_REGNUM_MASK;

    FIXME("Not applying the BUMPMAPENV matrix for pixel shader instruction texbem.\n");
    shader_addline(arg->buffer, "T%lu = texture2D(Psampler%lu, gl_TexCoord[%lu].xy + T%lu.xy);\n",
            reg1, reg1, reg1, reg2);
}

/** Process the D3DSIO_TEXREG2AR instruction in GLSL
 * Sample 2D texture at dst using the alpha & red (wx) components of src as texture coordinates */
void pshader_glsl_texreg2ar(SHADER_OPCODE_ARG* arg) {
    
    char tmpLine[255];
    char dst_str[100], src0_str[100];
    char dst_reg[50], src0_reg[50];
    char dst_mask[6], src0_mask[6];
    DWORD src0_regnum = arg->src[0] & D3DSP_REGNUM_MASK;

    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);

    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    shader_addline(arg->buffer, "%stexture2D(Psampler%lu, %s.yz))%s;\n",
            tmpLine, src0_regnum, dst_reg, dst_mask);
}

/** Process the D3DSIO_TEXREG2GB instruction in GLSL
 * Sample 2D texture at dst using the green & blue (yz) components of src as texture coordinates */
void pshader_glsl_texreg2gb(SHADER_OPCODE_ARG* arg) {

    char tmpLine[255];
    char dst_str[100], src0_str[100];
    char dst_reg[50], src0_reg[50];
    char dst_mask[6], src0_mask[6];
    DWORD src0_regnum = arg->src[0] & D3DSP_REGNUM_MASK;

    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);

    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    shader_addline(arg->buffer, "%stexture2D(Psampler%lu, %s.yz))%s;\n",
            tmpLine, src0_regnum, dst_reg, dst_mask);
}

/** Process the D3DSIO_TEXREG2RGB instruction in GLSL
 * Sample texture at dst using the rgb (xyz) components of src as texture coordinates */
void pshader_glsl_texreg2rgb(SHADER_OPCODE_ARG* arg) {

    char tmpLine[255];
    char dst_str[100], src0_str[100];
    char dst_reg[50], src0_reg[50];
    char dst_mask[6], src0_mask[6];
    char dimensions[5];
    DWORD src0_regnum = arg->src[0] & D3DSP_REGNUM_MASK;
    DWORD stype = arg->reg_maps->samplers[src0_regnum] & D3DSP_TEXTURETYPE_MASK;
    switch (stype) {
        case D3DSTT_2D:     strcpy(dimensions, "2D");   break;
        case D3DSTT_CUBE:   strcpy(dimensions, "Cube"); break;
        case D3DSTT_VOLUME: strcpy(dimensions, "3D");   break;
        default:
            strcpy(dimensions, ""); break;
            FIXME("Unrecognized sampler type: %#lx\n", stype);
            break;
    }

    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);

    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    shader_addline(arg->buffer, "%stexture%s(Psampler%lu, %s.%s))%s;\n",
            tmpLine, dimensions, src0_regnum, dst_reg, (stype == D3DSTT_2D) ? "xy" : "xyz", dst_mask);
}

/** Process the D3DSIO_TEXKILL instruction in GLSL.
 * If any of the first 3 components are < 0, discard this pixel */
void pshader_glsl_texkill(SHADER_OPCODE_ARG* arg) {

    char dst_str[100], dst_name[50], dst_mask[6];

    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_name, dst_mask, dst_str);
    shader_addline(arg->buffer, "if (any(lessThan(%s.xyz, vec3(0.0)))) discard;\n", dst_name);
}

/** Process the D3DSIO_DP2ADD instruction in GLSL.
 * dst = dot2(src0, src1) + src2 */
void pshader_glsl_dp2add(SHADER_OPCODE_ARG* arg) {

    char tmpLine[256];
    char dst_str[100], src0_str[100], src1_str[100], src2_str[100];
    char dst_reg[50], src0_reg[50], src1_reg[50], src2_reg[50];
    char dst_mask[6], src0_mask[6], src1_mask[6], src2_mask[6];
 
    shader_glsl_add_param(arg, arg->dst, 0, FALSE, dst_reg, dst_mask, dst_str);
    shader_glsl_add_param(arg, arg->src[0], arg->src_addr[0], TRUE, src0_reg, src0_mask, src0_str);
    shader_glsl_add_param(arg, arg->src[1], arg->src_addr[1], TRUE, src1_reg, src1_mask, src1_str);
    shader_glsl_add_param(arg, arg->src[2], arg->src_addr[2], TRUE, src2_reg, src2_mask, src2_str);   
    shader_glsl_add_dst(arg->dst, dst_reg, dst_mask, tmpLine);
    shader_addline(arg->buffer, "%sdot(vec2(%s), vec2(%s)) + %s)%s;\n",
                   tmpLine, src0_str, src1_str, src2_str, dst_mask);
}

void pshader_glsl_input_pack(
   SHADER_BUFFER* buffer,
   semantic* semantics_in) {

   unsigned int i;

   for (i = 0; i < MAX_REG_INPUT; i++) {

       DWORD usage_token = semantics_in[i].usage;
       DWORD register_token = semantics_in[i].reg;
       DWORD usage, usage_idx;
       char reg_mask[6];

       /* Uninitialized */
       if (!usage_token) continue;
       usage = (usage_token & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT;
       usage_idx = (usage_token & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;
       shader_glsl_get_output_register_swizzle(register_token, reg_mask);

       switch(usage) {

           case D3DDECLUSAGE_COLOR:
               if (usage_idx == 0)
                   shader_addline(buffer, "IN%lu%s = vec4(gl_Color)%s;\n",
                       i, reg_mask, reg_mask);
               else if (usage_idx == 1)
                   shader_addline(buffer, "IN%lu%s = vec4(gl_SecondaryColor)%s;\n",
                       i, reg_mask, reg_mask);
               else
                   shader_addline(buffer, "IN%lu%s = vec4(unsupported_color_input)%s;\n",
                       i, reg_mask, reg_mask);
               break;

           case D3DDECLUSAGE_TEXCOORD:
               shader_addline(buffer, "IN%lu%s = vec4(gl_TexCoord[%lu])%s;\n",
                   i, reg_mask, usage_idx, reg_mask );
               break;

           case D3DDECLUSAGE_FOG:
               shader_addline(buffer, "IN%lu%s = vec4(gl_FogFragCoord)%s;\n",
                   i, reg_mask, reg_mask);
               break;

           default:
               shader_addline(buffer, "IN%lu%s = vec4(unsupported_input)%s;\n",
                   i, reg_mask, reg_mask);
        }
    }
}

/*********************************************
 * Vertex Shader Specific Code begins here
 ********************************************/

void vshader_glsl_output_unpack(
   SHADER_BUFFER* buffer,
   semantic* semantics_out) {

   unsigned int i;

   for (i = 0; i < MAX_REG_OUTPUT; i++) {

       DWORD usage_token = semantics_out[i].usage;
       DWORD register_token = semantics_out[i].reg;
       DWORD usage, usage_idx;
       char reg_mask[6];

       /* Uninitialized */
       if (!usage_token) continue;

       usage = (usage_token & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT;
       usage_idx = (usage_token & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;
       shader_glsl_get_output_register_swizzle(register_token, reg_mask);

       switch(usage) {

           case D3DDECLUSAGE_COLOR:
               if (usage_idx == 0)
                   shader_addline(buffer, "gl_FrontColor%s = OUT%lu%s;\n", reg_mask, i, reg_mask);
               else if (usage_idx == 1)
                   shader_addline(buffer, "gl_FrontSecondaryColor%s = OUT%lu%s;\n", reg_mask, i, reg_mask);
               else
                   shader_addline(buffer, "unsupported_color_output%s = OUT%lu%s;\n", reg_mask, i, reg_mask);
               break;

           case D3DDECLUSAGE_POSITION:
               shader_addline(buffer, "gl_Position%s = OUT%lu%s;\n", reg_mask, i, reg_mask);
               break;
 
           case D3DDECLUSAGE_TEXCOORD:
               shader_addline(buffer, "gl_TexCoord[%lu]%s = OUT%lu%s;\n",
                   usage_idx, reg_mask, i, reg_mask);
               break;

           case WINED3DSHADERDECLUSAGE_PSIZE:
               shader_addline(buffer, "gl_PointSize = OUT%lu.x;\n", i);
               break;

           case WINED3DSHADERDECLUSAGE_FOG:
               shader_addline(buffer, "gl_FogFragCoord%s = OUT%lu%s;\n", reg_mask, i, reg_mask);
               break;

           default:
               shader_addline(buffer, "unsupported_output%s = OUT%lu%s;\n", reg_mask, i, reg_mask);
       }
    }
}
