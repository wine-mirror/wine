/*
 * GLSL pixel and vertex shader implementation
 *
 * Copyright 2006 Jason Green 
 * Copyright 2006-2007 Henri Verbeet
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

/*
 * D3D shader asm has swizzles on source parameters, and write masks for
 * destination parameters. GLSL uses swizzles for both. The result of this is
 * that for example "mov dst.xw, src.zyxw" becomes "dst.xw = src.zw" in GLSL.
 * Ie, to generate a proper GLSL source swizzle, we need to take the D3D write
 * mask for the destination parameter into account.
 */

#include "config.h"
#include <stdio.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

#define GLINFO_LOCATION      (*gl_info)

typedef struct {
    const char *name;
    DWORD coord_mask;
} glsl_sample_function_t;

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
    GLhandleARB programId = stateBlock->glsl_program->programId;
    GLhandleARB name_loc;
    int i;
    char sampler_name[20];

    for (i=0; i< GL_LIMITS(samplers); ++i) {
        if (stateBlock->textures[i] != NULL) {
           snprintf(sampler_name, sizeof(sampler_name), "Psampler%d", i);
           name_loc = GL_EXTCALL(glGetUniformLocationARB(programId, sampler_name));
           if (name_loc != -1) {
               TRACE("Loading %s for texture %d\n", sampler_name, i);
               GL_EXTCALL(glUniform1iARB(name_loc, i));
               checkGLcall("glUniform1iARB");
           }
        }
    }
}

/** 
 * Loads floating point constants (aka uniforms) into the currently set GLSL program.
 * When constant_list == NULL, it will load all the constants.
 */
static void shader_glsl_load_constantsF(IWineD3DBaseShaderImpl* This, WineD3D_GL_Info *gl_info,
        unsigned int max_constants, float* constants, GLhandleARB *constant_locations,
        struct list *constant_list) {
    local_constant* lconst;
    GLhandleARB tmp_loc;
    int i;

    if (!constant_list) {
        if (TRACE_ON(d3d_shader)) {
            for (i = 0; i < max_constants; ++i) {
                tmp_loc = constant_locations[i];
                if (tmp_loc != -1) {
                    TRACE("Loading constants %i: %f, %f, %f, %f\n", i,
                            constants[i * 4 + 0], constants[i * 4 + 1],
                            constants[i * 4 + 2], constants[i * 4 + 3]);
                }
            }
        }
        for (i = 0; i < max_constants; ++i) {
            tmp_loc = constant_locations[i];
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, constants + (i * 4)));
            }
        }
        checkGLcall("glUniform4fvARB()");
    } else {
        constant_entry *constant;
        if (TRACE_ON(d3d_shader)) {
            LIST_FOR_EACH_ENTRY(constant, constant_list, constant_entry, entry) {
                i = constant->idx;
                tmp_loc = constant_locations[i];
                if (tmp_loc != -1) {
                    TRACE("Loading constants %i: %f, %f, %f, %f\n", i,
                            constants[i * 4 + 0], constants[i * 4 + 1],
                            constants[i * 4 + 2], constants[i * 4 + 3]);
                }
            }
        }
        LIST_FOR_EACH_ENTRY(constant, constant_list, constant_entry, entry) {
            i = constant->idx;
            tmp_loc = constant_locations[i];
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, constants + (i * 4)));
            }
        }
        checkGLcall("glUniform4fvARB()");
    }

    /* Load immediate constants */
    if (TRACE_ON(d3d_shader)) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            tmp_loc = constant_locations[lconst->idx];
            if (tmp_loc != -1) {
                GLfloat* values = (GLfloat*)lconst->value;
                TRACE("Loading local constants %i: %f, %f, %f, %f\n", lconst->idx,
                        values[0], values[1], values[2], values[3]);
            }
        }
    }
    LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
        tmp_loc = constant_locations[lconst->idx];
        if (tmp_loc != -1) {
            /* We found this uniform name in the program - go ahead and send the data */
            GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, (GLfloat*)lconst->value));
        }
    }
    checkGLcall("glUniform4fvARB()");
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
    char tmp_name[8];
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
    char tmp_name[8];
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
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader) {
   
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) device;
    IWineD3DStateBlockImpl* stateBlock = deviceImpl->stateBlock;
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl*) deviceImpl->wineD3D)->gl_info;

    GLhandleARB *constant_locations;
    struct list *constant_list;
    GLhandleARB programId;
    
    if (!stateBlock->glsl_program) {
        /* No GLSL program set - nothing to do. */
        return;
    }
    programId = stateBlock->glsl_program->programId;

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;
        IWineD3DVertexShaderImpl* vshader_impl = (IWineD3DVertexShaderImpl*) vshader;
        GLint pos;

        IWineD3DVertexDeclarationImpl* vertexDeclaration =
            (IWineD3DVertexDeclarationImpl*) vshader_impl->vertexDeclaration;

        constant_locations = stateBlock->glsl_program->vuniformF_locations;
        constant_list = &stateBlock->set_vconstantsF;

        if (NULL != vertexDeclaration && NULL != vertexDeclaration->constants) {
            /* Load DirectX 8 float constants/uniforms for vertex shader */
            shader_glsl_load_constantsF(vshader, gl_info, GL_LIMITS(vshader_constantsF),
                    vertexDeclaration->constants, constant_locations, NULL);
        }

        /* Load DirectX 9 float constants/uniforms for vertex shader */
        shader_glsl_load_constantsF(vshader, gl_info, GL_LIMITS(vshader_constantsF),
                stateBlock->vertexShaderConstantF, constant_locations, constant_list);

        /* Load DirectX 9 integer constants/uniforms for vertex shader */
        shader_glsl_load_constantsI(vshader, gl_info, programId, MAX_CONST_I,
                                    stateBlock->vertexShaderConstantI,
                                    stateBlock->set.vertexShaderConstantsI);

        /* Load DirectX 9 boolean constants/uniforms for vertex shader */
        shader_glsl_load_constantsB(vshader, gl_info, programId, MAX_CONST_B,
                                    stateBlock->vertexShaderConstantB,
                                    stateBlock->set.vertexShaderConstantsB);

        /* Upload the position fixup params */
        pos = GL_EXTCALL(glGetUniformLocationARB(programId, "posFixup"));
        checkGLcall("glGetUniformLocationARB");
        GL_EXTCALL(glUniform4fvARB(pos, 1, &deviceImpl->posFixup[0]));
        checkGLcall("glUniform4fvARB");
    }

    if (usePixelShader) {

        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;

        constant_locations = stateBlock->glsl_program->puniformF_locations;
        constant_list = &stateBlock->set_pconstantsF;

        /* Load pixel shader samplers */
        shader_glsl_load_psamplers(gl_info, (IWineD3DStateBlock*) stateBlock);

        /* Load DirectX 9 float constants/uniforms for pixel shader */
        shader_glsl_load_constantsF(pshader, gl_info, GL_LIMITS(pshader_constantsF),
                stateBlock->pixelShaderConstantF, constant_locations, constant_list);

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

    if(!pshader)
        shader_addline(buffer, "uniform vec4 posFixup;\n");

    /* Declare texture samplers */ 
    for (i = 0; i < This->baseShader.limits.sampler; i++) {
        if (reg_maps->samplers[i]) {

            DWORD stype = reg_maps->samplers[i] & WINED3DSP_TEXTURETYPE_MASK;
            switch (stype) {

                case WINED3DSTT_1D:
                    shader_addline(buffer, "uniform sampler1D %csampler%lu;\n", prefix, i);
                    break;
                case WINED3DSTT_2D:
                    shader_addline(buffer, "uniform sampler2D %csampler%lu;\n", prefix, i);
                    break;
                case WINED3DSTT_CUBE:
                    shader_addline(buffer, "uniform samplerCube %csampler%lu;\n", prefix, i);
                    break;
                case WINED3DSTT_VOLUME:
                    shader_addline(buffer, "uniform sampler3D %csampler%lu;\n", prefix, i);
                    break;
                default:
                    shader_addline(buffer, "uniform unsupported_sampler %csampler%lu;\n", prefix, i);
                    FIXME("Unrecognized sampler type: %#x\n", stype);
                    break;
            }
        }
    }
    
    /* Declare address variables */
    for (i = 0; i < This->baseShader.limits.address; i++) {
        if (reg_maps->address[i])
            shader_addline(buffer, "ivec4 A%d;\n", i);
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
static void shader_glsl_add_src_param(SHADER_OPCODE_ARG* arg, const DWORD param,
        const DWORD addr_token, DWORD mask, char *reg_name, char *swizzle, char *out_str);

/* FIXME: This should be removed */
static void shader_glsl_add_src_param_old(SHADER_OPCODE_ARG* arg, const DWORD param,
        const DWORD addr_token, char *reg_name, char *swizzle, char *out_str) {
    shader_glsl_add_src_param(arg, param, addr_token, 0, reg_name, swizzle, out_str);
}

/** Used for opcode modifiers - They multiply the result by the specified amount */
static const char * const shift_glsl_tab[] = {
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

/* Generate a GLSL parameter that does the input modifier computation and return the input register/mask to use */
static void shader_glsl_gen_modifier (
    const DWORD instr,
    const char *in_reg,
    const char *in_regswizzle,
    char *out_str) {

    out_str[0] = 0;
    
    if (instr == WINED3DSIO_TEXKILL)
        return;

    switch (instr & WINED3DSP_SRCMOD_MASK) {
    case WINED3DSPSM_NONE:
        sprintf(out_str, "%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_NEG:
        sprintf(out_str, "-%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_NOT:
        sprintf(out_str, "!%s%s", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_BIAS:
        sprintf(out_str, "(%s%s - vec4(0.5)%s)", in_reg, in_regswizzle, in_regswizzle);
        break;
    case WINED3DSPSM_BIASNEG:
        sprintf(out_str, "-(%s%s - vec4(0.5)%s)", in_reg, in_regswizzle, in_regswizzle);
        break;
    case WINED3DSPSM_SIGN:
        sprintf(out_str, "(2.0 * (%s%s - 0.5))", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_SIGNNEG:
        sprintf(out_str, "-(2.0 * (%s%s - 0.5))", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_COMP:
        sprintf(out_str, "(1.0 - %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_X2:
        sprintf(out_str, "(2.0 * %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_X2NEG:
        sprintf(out_str, "-(2.0 * %s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_DZ:    /* reg1_db = { reg1.r/b, reg1.g/b, ...}  The g & a components are undefined, so we'll leave them alone */
        sprintf(out_str, "vec4(%s.r / %s.b, %s.g / %s.b, %s.b, %s.a)", in_reg, in_reg, in_reg, in_reg, in_reg, in_reg);
        break;
    case WINED3DSPSM_DW:
        sprintf(out_str, "vec4(%s.r / %s.a, %s.g / %s.a, %s.b, %s.a)", in_reg, in_reg, in_reg, in_reg, in_reg, in_reg);
        break;
    case WINED3DSPSM_ABS:
        sprintf(out_str, "abs(%s%s)", in_reg, in_regswizzle);
        break;
    case WINED3DSPSM_ABSNEG:
        sprintf(out_str, "-abs(%s%s)", in_reg, in_regswizzle);
        break;
    default:
        FIXME("Unhandled modifier %u\n", (instr & WINED3DSP_SRCMOD_MASK));
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
    static const char * const hwrastout_reg_names[] = { "gl_Position", "gl_FogFragCoord", "gl_PointSize" };

    DWORD reg = param & WINED3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    WineD3D_GL_Info* gl_info = &((IWineD3DImpl*)deviceImpl->wineD3D)->gl_info;

    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    char tmpStr[50];

    *is_color = FALSE;   
 
    switch (regtype) {
    case WINED3DSPR_TEMP:
        sprintf(tmpStr, "R%u", reg);
    break;
    case WINED3DSPR_INPUT:
        if (pshader) {
            /* Pixel shaders >= 3.0 */
            if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 3)
                sprintf(tmpStr, "IN%u", reg);
             else {
                if (reg==0)
                    strcpy(tmpStr, "gl_Color");
                else
                    strcpy(tmpStr, "gl_SecondaryColor");
            }
        } else {
            if (vshader_input_is_color((IWineD3DVertexShader*) This, reg))
               *is_color = TRUE;
            sprintf(tmpStr, "attrib%u", reg);
        } 
        break;
    case WINED3DSPR_CONST:
    {
        const char* prefix = pshader? "PC":"VC";

        /* Relative addressing */
        if (param & WINED3DSHADER_ADDRMODE_RELATIVE) {

           /* Relative addressing on shaders 2.0+ have a relative address token, 
            * prior to that, it was hard-coded as "A0.x" because there's only 1 register */
           if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 2)  {
               char relStr[100], relReg[50], relMask[6];
               shader_glsl_add_src_param(arg, addr_token, 0, WINED3DSP_WRITEMASK_0, relReg, relMask, relStr);
               sprintf(tmpStr, "%s[%s + %u]", prefix, relStr, reg);
           } else
               sprintf(tmpStr, "%s[A0.x + %u]", prefix, reg);

        } else
             sprintf(tmpStr, "%s[%u]", prefix, reg);

        break;
    }
    case WINED3DSPR_CONSTINT:
        if (pshader)
            sprintf(tmpStr, "PI[%u]", reg);
        else
            sprintf(tmpStr, "VI[%u]", reg);
        break;
    case WINED3DSPR_CONSTBOOL:
        if (pshader)
            sprintf(tmpStr, "PB[%u]", reg);
        else
            sprintf(tmpStr, "VB[%u]", reg);
        break;
    case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
        if (pshader) {
            sprintf(tmpStr, "T%u", reg);
        } else {
            sprintf(tmpStr, "A%u", reg);
        }
    break;
    case WINED3DSPR_LOOP:
        sprintf(tmpStr, "aL");
    break;
    case WINED3DSPR_SAMPLER:
        if (pshader)
            sprintf(tmpStr, "Psampler%u", reg);
        else
            sprintf(tmpStr, "Vsampler%u", reg);
    break;
    case WINED3DSPR_COLOROUT:
        if (reg >= GL_LIMITS(buffers)) {
            WARN("Write to render target %u, only %d supported\n", reg, 4);
        }
        if (GL_SUPPORT(ARB_DRAW_BUFFERS)) {
            sprintf(tmpStr, "gl_FragData[%u]", reg);
        } else { /* On older cards with GLSL support like the GeforceFX there's only one buffer. */
            sprintf(tmpStr, "gl_FragColor");
        }
    break;
    case WINED3DSPR_RASTOUT:
        sprintf(tmpStr, "%s", hwrastout_reg_names[reg]);
    break;
    case WINED3DSPR_DEPTHOUT:
        sprintf(tmpStr, "gl_FragDepth");
    break;
    case WINED3DSPR_ATTROUT:
        if (reg == 0) {
            sprintf(tmpStr, "gl_FrontColor");
        } else {
            sprintf(tmpStr, "gl_FrontSecondaryColor");
        }
    break;
    case WINED3DSPR_TEXCRDOUT:
        /* Vertex shaders >= 3.0: WINED3DSPR_OUTPUT */
        if (WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) >= 3)
            sprintf(tmpStr, "OUT%u", reg);
        else
            sprintf(tmpStr, "gl_TexCoord[%u]", reg);
    break;
    default:
        FIXME("Unhandled register name Type(%d)\n", regtype);
        sprintf(tmpStr, "unrecognized_register");
    break;
    }

    strcat(regstr, tmpStr);
}

/* Get the GLSL write mask for the destination register */
static DWORD shader_glsl_get_write_mask(const DWORD param, char *write_mask) {
    char *ptr = write_mask;
    DWORD mask = param & WINED3DSP_WRITEMASK_ALL;

    /* gl_FogFragCoord and glPointSize are floats, fixup the write mask. */
    if ((shader_get_regtype(param) == WINED3DSPR_RASTOUT) && ((param & WINED3DSP_REGNUM_MASK) != 0)) {
        mask = WINED3DSP_WRITEMASK_0;
    } else {
        if (mask != WINED3DSP_WRITEMASK_ALL) {
            *ptr++ = '.';
            if (param & WINED3DSP_WRITEMASK_0) *ptr++ = 'x';
            if (param & WINED3DSP_WRITEMASK_1) *ptr++ = 'y';
            if (param & WINED3DSP_WRITEMASK_2) *ptr++ = 'z';
            if (param & WINED3DSP_WRITEMASK_3) *ptr++ = 'w';
        }
    }

    *ptr = '\0';

    return mask;
}

static size_t shader_glsl_get_write_mask_size(DWORD write_mask) {
    size_t size = 0;

    if (write_mask & WINED3DSP_WRITEMASK_0) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_1) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_2) ++size;
    if (write_mask & WINED3DSP_WRITEMASK_3) ++size;

    return size;
}

static void shader_glsl_get_swizzle(const DWORD param, BOOL fixup, DWORD mask, char *swizzle_str) {
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

    /* FIXME: This is here to keep shader_glsl_add_src_param_old happy.
     * As soon as that is removed, this needs to go as well. */
    if (!mask) {
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
    } else {
        *ptr++ = '.';
        if (mask & WINED3DSP_WRITEMASK_0) *ptr++ = swizzle_chars[swizzle_x];
        if (mask & WINED3DSP_WRITEMASK_1) *ptr++ = swizzle_chars[swizzle_y];
        if (mask & WINED3DSP_WRITEMASK_2) *ptr++ = swizzle_chars[swizzle_z];
        if (mask & WINED3DSP_WRITEMASK_3) *ptr++ = swizzle_chars[swizzle_w];
    }

    *ptr = '\0';
}

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static void shader_glsl_add_src_param(SHADER_OPCODE_ARG* arg, const DWORD param,
        const DWORD addr_token, DWORD mask, char *reg_name, char *swizzle, char *out_str) {
    BOOL is_color = FALSE;
    swizzle[0] = reg_name[0] = out_str[0] = 0;

    shader_glsl_get_register_name(param, addr_token, reg_name, &is_color, arg);

    shader_glsl_get_swizzle(param, is_color, mask, swizzle);
    shader_glsl_gen_modifier(param, reg_name, swizzle, out_str);
}

/* From a given parameter token, generate the corresponding GLSL string.
 * Also, return the actual register name and swizzle in case the
 * caller needs this information as well. */
static DWORD shader_glsl_add_dst_param(SHADER_OPCODE_ARG* arg, const DWORD param,
        const DWORD addr_token, char *reg_name, char *write_mask, char *out_str) {
    BOOL is_color = FALSE;
    DWORD mask;
    write_mask[0] = reg_name[0] = out_str[0] = 0;

    shader_glsl_get_register_name(param, addr_token, reg_name, &is_color, arg);

    mask = shader_glsl_get_write_mask(param, write_mask);
    sprintf(out_str, "%s%s", reg_name, write_mask);

    return mask;
}

/* Append the destination part of the instruction to the buffer, return the effective write mask */
static DWORD shader_glsl_append_dst(SHADER_BUFFER *buffer, SHADER_OPCODE_ARG *arg) {
    char reg_name[50], write_mask[6], reg_str[100];
    DWORD mask;
    int shift;

    shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    mask = shader_glsl_add_dst_param(arg, arg->dst, arg->dst_addr, reg_name, write_mask, reg_str);
    shader_addline(buffer, "%s = %s(", reg_str, shift_glsl_tab[shift]);

    return mask;
}

/** Process GLSL instruction modifiers */
void shader_glsl_add_instruction_modifiers(SHADER_OPCODE_ARG* arg) {
    
    DWORD mask = arg->dst & WINED3DSP_DSTMOD_MASK;
 
    if (arg->opcode->dst_token && mask != 0) {
        char dst_reg[50];
        char dst_mask[6];
        char dst_str[100];
       
        shader_glsl_add_dst_param(arg, arg->dst, 0, dst_reg, dst_mask, dst_str);

        if (mask & WINED3DSPDM_SATURATE) {
            /* _SAT means to clamp the value of the register to between 0 and 1 */
            shader_addline(arg->buffer, "%s%s = clamp(%s%s, 0.0, 1.0);\n", dst_reg, dst_mask, dst_reg, dst_mask);
        }
        if (mask & WINED3DSPDM_MSAMPCENTROID) {
            FIXME("_centroid modifier not handled\n");
        }
        if (mask & WINED3DSPDM_PARTIALPRECISION) {
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
            FIXME("Unrecognized comparison value: %u\n", op);
            return "(\?\?)";
    }
}

static void shader_glsl_get_sample_function(DWORD sampler_type, BOOL projected, glsl_sample_function_t *sample_function) {
    /* Note that there's no such thing as a projected cube texture. */
    switch(sampler_type) {
        case WINED3DSTT_1D:
            sample_function->name = projected ? "texture1DProj" : "texture1D";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0;
            break;
        case WINED3DSTT_2D:
            sample_function->name = projected ? "texture2DProj" : "texture2D";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1;
            break;
        case WINED3DSTT_CUBE:
            sample_function->name = "textureCube";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
            break;
        case WINED3DSTT_VOLUME:
            sample_function->name = projected ? "texture3DProj" : "texture3D";
            sample_function->coord_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
            break;
        default:
            sample_function->name = "";
            FIXME("Unrecognized sampler type: %#x;\n", sampler_type);
            break;
    }
}

static void shader_glsl_sample(SHADER_OPCODE_ARG* arg, DWORD sampler_idx, const char *dst_str, const char *coord_reg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    const char sampler_prefix = shader_is_pshader_version(This->baseShader.hex_version) ? 'P' : 'V';
    SHADER_BUFFER* buffer = arg->buffer;
    glsl_sample_function_t sample_function;

    if(deviceImpl->stateBlock->textureState[sampler_idx][WINED3DTSS_TEXTURETRANSFORMFLAGS] & WINED3DTTFF_PROJECTED) {
        shader_glsl_get_sample_function(sampler_type, TRUE, &sample_function);
        shader_addline(buffer, "%s = %s(%csampler%u, %s);\n", dst_str, sample_function.name, sampler_prefix, sampler_idx, coord_reg);
    } else {
        char coord_swizzle[6];

        shader_glsl_get_sample_function(sampler_type, FALSE, &sample_function);
        shader_glsl_get_write_mask(sample_function.coord_mask, coord_swizzle);

        shader_addline(buffer, "%s = %s(%csampler%u, %s%s);\n", dst_str, sample_function.name, sampler_prefix, sampler_idx, coord_reg, coord_swizzle);
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
    char src0_reg[50], src1_reg[50];
    char src0_mask[6], src1_mask[6];
    char src0_str[100], src1_str[100];
    DWORD write_mask;
    char op;

    /* Determine the GLSL operator to use based on the opcode */
    switch (curOpcode->opcode) {
        case WINED3DSIO_MUL: op = '*'; break;
        case WINED3DSIO_ADD: op = '+'; break;
        case WINED3DSIO_SUB: op = '-'; break;
        default:
            op = ' ';
            FIXME("Opcode %s not yet handled in GLSL\n", curOpcode->name);
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, src1_reg, src1_mask, src1_str);
    shader_addline(buffer, "%s %c %s);\n", src0_str, op, src1_str);
}

/* Process the WINED3DSIO_MOV opcode using GLSL (dst = src) */
void shader_glsl_mov(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, src0_reg, src0_mask, src0_str);

    /* In vs_1_1 WINED3DSIO_MOV can write to the adress register. In later
     * shader versions WINED3DSIO_MOVA is used for this. */
    if ((WINED3DSHADER_VERSION_MAJOR(shader->baseShader.hex_version) == 1 &&
            !shader_is_pshader_version(shader->baseShader.hex_version) &&
            shader_get_regtype(arg->dst) == WINED3DSPR_ADDR) ||
            arg->opcode->opcode == WINED3DSIO_MOVA) {
        /* We need to *round* to the nearest int here. */
        size_t mask_size = shader_glsl_get_write_mask_size(write_mask);
        if (mask_size > 1) {
            shader_addline(buffer, "ivec%d(floor(%s + vec%d(0.5))));\n", mask_size, src0_str, mask_size);
        } else {
            shader_addline(buffer, "int(floor(%s + 0.5)));\n", src0_str);
        }
    } else {
        shader_addline(buffer, "%s);\n", src0_str);
    }
}

/* Process the dot product operators DP3 and DP4 in GLSL (dst = dot(src0, src1)) */
void shader_glsl_dot(SHADER_OPCODE_ARG* arg) {
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_str[100], src1_str[100];
    char src0_reg[50], src1_reg[50];
    char src0_mask[6], src1_mask[6];
    DWORD dst_write_mask, src_write_mask;
    size_t dst_size = 0;

    dst_write_mask = shader_glsl_append_dst(buffer, arg);
    dst_size = shader_glsl_get_write_mask_size(dst_write_mask);

    /* dp3 works on vec3, dp4 on vec4 */
    if (curOpcode->opcode == WINED3DSIO_DP4) {
        src_write_mask = WINED3DSP_WRITEMASK_ALL;
    } else {
        src_write_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    }

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_write_mask, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], src_write_mask, src1_reg, src1_mask, src1_str);

    if (dst_size > 1) {
        shader_addline(buffer, "vec%d(dot(%s, %s)));\n", dst_size, src0_str, src1_str);
    } else {
        shader_addline(buffer, "dot(%s, %s));\n", src0_str, src1_str);
    }
}

/* Note that this instruction has some restrictions. The destination write mask
 * can't contain the w component, and the source swizzles have to be .xyzw */
void shader_glsl_cross(SHADER_OPCODE_ARG *arg) {
    char src0_reg[50], src0_mask[6], src0_str[100];
    char src1_reg[50], src1_mask[6], src1_str[100];
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    char dst_mask[6];

    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], src_mask, src1_reg, src1_mask, src1_str);
    shader_addline(arg->buffer, "cross(%s, %s).%s);\n", src0_str, src1_str, dst_mask);
}

/* Map the opcode 1-to-1 to the GL code (arg->dst = instruction(src0, src1, ...) */
void shader_glsl_map2gl(SHADER_OPCODE_ARG* arg) {
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    const char *instruction;
    char arguments[256];
    char src_str[100];
    char src_reg[50];
    char src_mask[6];
    DWORD write_mask;
    unsigned i;

    /* Determine the GLSL function to use based on the opcode */
    /* TODO: Possibly make this a table for faster lookups */
    switch (curOpcode->opcode) {
        case WINED3DSIO_MIN: instruction = "min"; break;
        case WINED3DSIO_MAX: instruction = "max"; break;
        case WINED3DSIO_RSQ: instruction = "inversesqrt"; break;
        case WINED3DSIO_ABS: instruction = "abs"; break;
        case WINED3DSIO_FRC: instruction = "fract"; break;
        case WINED3DSIO_POW: instruction = "pow"; break;
        case WINED3DSIO_NRM: instruction = "normalize"; break;
        case WINED3DSIO_LOGP:
        case WINED3DSIO_LOG: instruction = "log2"; break;
        case WINED3DSIO_EXP: instruction = "exp2"; break;
        case WINED3DSIO_SGN: instruction = "sign"; break;
        default: instruction = "";
            FIXME("Opcode %s not yet handled in GLSL\n", curOpcode->name);
            break;
    }

    write_mask = shader_glsl_append_dst(buffer, arg);

    arguments[0] = '\0';
    if (curOpcode->num_params > 0) {
        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, src_reg, src_mask, src_str);
        strcat(arguments, src_str);
        for (i = 2; i < curOpcode->num_params; ++i) {
            strcat(arguments, ", ");
            shader_glsl_add_src_param(arg, arg->src[i-1], arg->src_addr[i-1], write_mask, src_reg, src_mask, src_str);
            strcat(arguments, src_str);
        }
    }

    shader_addline(buffer, "%s(%s));\n", instruction, arguments);
}

/** Process the WINED3DSIO_EXPP instruction in GLSL:
 * For shader model 1.x, do the following (and honor the writemask, so use a temporary variable):
 *   dst.x = 2^(floor(src))
 *   dst.y = src - floor(src)
 *   dst.z = 2^src   (partial precision is allowed, but optional)
 *   dst.w = 1.0;
 * For 2.0 shaders, just do this (honoring writemask and swizzle):
 *   dst = 2^src;    (partial precision is allowed, but optional)
 */
void shader_glsl_expp(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl *shader = (IWineD3DBaseShaderImpl *)arg->shader;
    char src_str[100];
    char src_reg[50];
    char src_mask[6];

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, src_reg, src_mask, src_str);

    if (shader->baseShader.hex_version < WINED3DPS_VERSION(2,0)) {
        char dst_mask[6];

        shader_addline(arg->buffer, "tmp0.x = exp2(floor(%s));\n", src_str);
        shader_addline(arg->buffer, "tmp0.y = %s - floor(%s);\n", src_str, src_str);
        shader_addline(arg->buffer, "tmp0.z = exp2(%s);\n", src_str);
        shader_addline(arg->buffer, "tmp0.w = 1.0;\n");

        shader_glsl_append_dst(arg->buffer, arg);
        shader_glsl_get_write_mask(arg->dst, dst_mask);
        shader_addline(arg->buffer, "tmp0%s);\n", dst_mask);
    } else {
        DWORD write_mask;
        size_t mask_size;

        write_mask = shader_glsl_append_dst(arg->buffer, arg);
        mask_size = shader_glsl_get_write_mask_size(write_mask);

        if (mask_size > 1) {
            shader_addline(arg->buffer, "vec%d(exp2(%s)));\n", mask_size, src_str);
        } else {
            shader_addline(arg->buffer, "exp2(%s));\n", src_str);
        }
    }
}

/** Process the RCP (reciprocal or inverse) opcode in GLSL (dst = 1 / src) */
void shader_glsl_rcp(SHADER_OPCODE_ARG* arg) {
    char src_str[100];
    char src_reg[50];
    char src_mask[6];
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, src_reg, src_mask, src_str);

    if (mask_size > 1) {
        shader_addline(arg->buffer, "vec%d(1.0 / %s));\n", mask_size, src_str);
    } else {
        shader_addline(arg->buffer, "1.0 / %s);\n", src_str);
    }
}

/** Process signed comparison opcodes in GLSL. */
void shader_glsl_compare(SHADER_OPCODE_ARG* arg) {
    char src0_str[100], src1_str[100];
    char src0_reg[50], src1_reg[50];
    char src0_mask[6], src1_mask[6];
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, src1_reg, src1_mask, src1_str);

    if (mask_size > 1) {
        const char *compare;

        switch(arg->opcode->opcode) {
            case WINED3DSIO_SLT: compare = "lessThan"; break;
            case WINED3DSIO_SGE: compare = "greaterThanEqual"; break;
            default: compare = "";
                FIXME("Can't handle opcode %s\n", arg->opcode->name);
        }

        shader_addline(arg->buffer, "vec%d(%s(%s, %s)));\n", mask_size, compare, src0_str, src1_str);
    } else {
        const char *compare;

        switch(arg->opcode->opcode) {
            case WINED3DSIO_SLT: compare = "<"; break;
            case WINED3DSIO_SGE: compare = ">="; break;
            default: compare = "";
                FIXME("Can't handle opcode %s\n", arg->opcode->name);
        }

        shader_addline(arg->buffer, "(%s %s %s) ? 1.0 : 0.0);\n", src0_str, compare, src1_str);
    }
}

/** Process CMP instruction in GLSL (dst = src0 >= 0.0 ? src1 : src2), per channel */
void shader_glsl_cmp(SHADER_OPCODE_ARG* arg) {
    char src0_str[100], src1_str[100], src2_str[100];
    char src0_reg[50], src1_reg[50], src2_reg[50];
    char src0_mask[6], src1_mask[6], src2_mask[6];
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, src1_reg, src1_mask, src1_str);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, src2_reg, src2_mask, src2_str);

    if (mask_size > 1) {
        shader_addline(arg->buffer, "mix(%s, %s, vec%d(lessThan(%s, vec%d(0.0)))));\n", src1_str, src2_str, mask_size, src0_str, mask_size);
    } else {
        shader_addline(arg->buffer, "%s >= 0.0 ? %s : %s);\n", src0_str, src1_str, src2_str);
    }
}

/** Process the CND opcode in GLSL (dst = (src0 < 0.5) ? src1 : src2) */
/* For ps 1.1-1.3, only a single component of src0 is used. For ps 1.4
 * the compare is done per component of src0. */
void shader_glsl_cnd(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    char src0_str[100], src1_str[100], src2_str[100];
    char src0_reg[50], src1_reg[50], src2_reg[50];
    char src0_mask[6], src1_mask[6], src2_mask[6];
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);

    if (shader->baseShader.hex_version < WINED3DPS_VERSION(1, 4)) {
        mask_size = 1;
        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, src0_reg, src0_mask, src0_str);
    } else {
        mask_size = shader_glsl_get_write_mask_size(write_mask);
        shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, src0_reg, src0_mask, src0_str);
    }

    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, src1_reg, src1_mask, src1_str);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, src2_reg, src2_mask, src2_str);

    if (mask_size > 1) {
        shader_addline(arg->buffer, "mix(%s, %s, vec%d(lessThan(%s, vec%d(0.5)))));\n", src2_str, src1_str, mask_size, src0_str, mask_size);
    } else {
        shader_addline(arg->buffer, "%s < 0.5 ? %s : %s);\n", src0_str, src1_str, src2_str);
    }
}

/** GLSL code generation for WINED3DSIO_MAD: Multiply the first 2 opcodes, then add the last */
void shader_glsl_mad(SHADER_OPCODE_ARG* arg) {
    char src0_str[100], src1_str[100], src2_str[100];
    char src0_reg[50], src1_reg[50], src2_reg[50];
    char src0_mask[6], src1_mask[6], src2_mask[6];
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, src1_reg, src1_mask, src1_str);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, src2_reg, src2_mask, src2_str);
    shader_addline(arg->buffer, "(%s * %s) + %s);\n", src0_str, src1_str, src2_str);
}

/** Handles transforming all WINED3DSIO_M?x? opcodes for 
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
        tmpArg.src[1]      = arg->src[1]+i;
        shader_glsl_dot(&tmpArg);
    }
}

/**
    The LRP instruction performs a component-wise linear interpolation 
    between the second and third operands using the first operand as the
    blend factor.  Equation:  (dst = src2 + src0 * (src1 - src2))
    This is equivalent to mix(src2, src1, src0);
*/
void shader_glsl_lrp(SHADER_OPCODE_ARG* arg) {
    char src0_str[100], src1_str[100], src2_str[100];
    char src0_reg[50], src1_reg[50], src2_reg[50];
    char src0_mask[6], src1_mask[6], src2_mask[6];
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], write_mask, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], write_mask, src1_reg, src1_mask, src1_str);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], write_mask, src2_reg, src2_mask, src2_str);

    shader_addline(arg->buffer, "mix(%s, %s, %s));\n", src2_str, src1_str, src0_str);
}

/** Process the WINED3DSIO_LIT instruction in GLSL:
 * dst.x = dst.w = 1.0
 * dst.y = (src0.x > 0) ? src0.x
 * dst.z = (src0.x > 0) ? ((src0.y > 0) ? pow(src0.y, src.w) : 0) : 0
 *                                        where src.w is clamped at +- 128
 */
void shader_glsl_lit(SHADER_OPCODE_ARG* arg) {
    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];
    char src1_str[100];
    char src1_reg[50];
    char src1_mask[6];
    char src3_str[100];
    char src3_reg[50];
    char src3_mask[6];
    char dst_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_1, src1_reg, src1_mask, src1_str);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_3, src3_reg, src3_mask, src3_str);

    shader_addline(arg->buffer, "vec4(1.0, (%s > 0.0 ? %s : 0.0), (%s > 0.0 ? ((%s > 0.0) ? pow(%s, clamp(%s, -128.0, 128.0)) : 0.0) : 0.0), 1.0)%s);\n",
        src0_str, src0_str, src0_str, src1_str, src1_str, src3_str, dst_mask);
}

/** Process the WINED3DSIO_DST instruction in GLSL:
 * dst.x = 1.0
 * dst.y = src0.x * src0.y
 * dst.z = src0.z
 * dst.w = src1.w
 */
void shader_glsl_dst(SHADER_OPCODE_ARG* arg) {
    char src0y_str[100], src0z_str[100], src1y_str[100], src1w_str[100];
    char src0y_reg[50], src0z_reg[50], src1y_reg[50], src1w_reg[50];
    char dst_mask[6], src0y_mask[6], src0z_mask[6], src1y_mask[6], src1w_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_1, src0y_reg, src0y_mask, src0y_str);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_2, src0z_reg, src0z_mask, src0z_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_1, src1y_reg, src1y_mask, src1y_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_3, src1w_reg, src1w_mask, src1w_str);

    shader_addline(arg->buffer, "vec4(1.0, %s * %s, %s, %s))%s;\n",
            src0y_str, src1y_str, src0z_str, src1w_str, dst_mask);
}

/** Process the WINED3DSIO_SINCOS instruction in GLSL:
 * VS 2.0 requires that specific cosine and sine constants be passed to this instruction so the hardware
 * can handle it.  But, these functions are built-in for GLSL, so we can just ignore the last 2 params.
 * 
 * dst.x = cos(src0.?)
 * dst.y = sin(src0.?)
 * dst.z = dst.z
 * dst.w = dst.w
 */
void shader_glsl_sincos(SHADER_OPCODE_ARG* arg) {
    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];
    DWORD write_mask;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0, src0_reg, src0_mask, src0_str);

    switch (write_mask) {
        case WINED3DSP_WRITEMASK_0:
            shader_addline(arg->buffer, "cos(%s));\n", src0_str);
            break;

        case WINED3DSP_WRITEMASK_1:
            shader_addline(arg->buffer, "sin(%s));\n", src0_str);
            break;

        case (WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1):
            shader_addline(arg->buffer, "vec2(cos(%s), sin(%s)));\n", src0_str, src0_str);
            break;

        default:
            ERR("Write mask should be .x, .y or .xy\n");
            break;
    }
}

/** Process the WINED3DSIO_LOOP instruction in GLSL:
 * Start a for() loop where src0.y is the initial value of aL,
 *  increment aL by src0.z for a total of src0.x iterations.
 *  Need to use a temporary variable for this operation.
 */
void shader_glsl_loop(SHADER_OPCODE_ARG* arg) {
    
    char src1_str[100];
    char src1_reg[50];
    char src1_mask[6];
    
    shader_glsl_add_src_param_old(arg, arg->src[1], arg->src_addr[1], src1_reg, src1_mask, src1_str);
  
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

    shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], src0_reg, src0_mask, src0_str);
    shader_addline(arg->buffer, "for (tmpInt = 0; tmpInt < %s.x; tmpInt++) {\n", src0_reg);
}

void shader_glsl_if(SHADER_OPCODE_ARG* arg) {

    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];

    shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], src0_reg, src0_mask, src0_str);
    shader_addline(arg->buffer, "if (%s) {\n", src0_str); 
}

void shader_glsl_ifc(SHADER_OPCODE_ARG* arg) {

    char src0_str[100], src1_str[100];
    char src0_reg[50], src1_reg[50];
    char src0_mask[6], src1_mask[6];

    shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param_old(arg, arg->src[1], arg->src_addr[1], src1_reg, src1_mask, src1_str);

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

    shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param_old(arg, arg->src[1], arg->src_addr[1], src1_reg, src1_mask, src1_str);

    shader_addline(arg->buffer, "if (%s %s %s) break;\n",
        src0_str, shader_get_comp_op(arg->opcode_token), src1_str);
}

void shader_glsl_label(SHADER_OPCODE_ARG* arg) {

    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_addline(arg->buffer, "}\n");
    shader_addline(arg->buffer, "void subroutine%lu () {\n",  snum);
}

void shader_glsl_call(SHADER_OPCODE_ARG* arg) {
    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_addline(arg->buffer, "subroutine%lu();\n", snum);
}

void shader_glsl_callnz(SHADER_OPCODE_ARG* arg) {

    char src1_str[100];
    char src1_reg[50];
    char src1_mask[6];
   
    DWORD snum = (arg->src[0]) & WINED3DSP_REGNUM_MASK;
    shader_glsl_add_src_param_old(arg, arg->src[1], arg->src_addr[1], src1_reg, src1_mask, src1_str);
    shader_addline(arg->buffer, "if (%s) subroutine%lu();\n", src1_str, snum);
}

/*********************************************
 * Pixel Shader Specific Code begins here
 ********************************************/
void pshader_glsl_tex(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;

    DWORD hex_version = This->baseShader.hex_version;

    char dst_str[100],   dst_reg[50],  dst_mask[6];
    char coord_str[100], coord_reg[50], coord_mask[6];

    /* All versions have a destination register */
    shader_glsl_add_dst_param(arg, arg->dst, 0, dst_reg, dst_mask, dst_str);

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
    if (hex_version < WINED3DPS_VERSION(1,4))
       strcpy(coord_reg, dst_reg);
    else
       shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], coord_reg, coord_mask, coord_str);

    /* 1.0-1.4: Use destination register as sampler source.
     * 2.0+: Use provided sampler source. */
    if (hex_version < WINED3DPS_VERSION(2,0)) {
        shader_glsl_sample(arg, arg->dst & WINED3DSP_REGNUM_MASK, dst_str, coord_reg);
    } else {
        shader_glsl_sample(arg, arg->src[1] & WINED3DSP_REGNUM_MASK, dst_str, coord_reg);
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

    shader_glsl_add_dst_param(arg, arg->dst, 0, tmpReg, tmpMask, tmpStr);

    if (hex_version != WINED3DPS_VERSION(1,4)) {
        DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
        shader_addline(buffer, "%s = clamp(gl_TexCoord[%u], 0.0, 1.0);\n", tmpReg, reg);
    } else {
        DWORD reg2 = arg->src[0] & WINED3DSP_REGNUM_MASK;
        shader_addline(buffer, "%s = gl_TexCoord[%u]%s;\n", tmpStr, reg2, tmpMask);
   }
}

/** Process the WINED3DSIO_TEXDP3TEX instruction in GLSL:
 * Take a 3-component dot product of the TexCoord[dstreg] and src,
 * then perform a 1D texture lookup from stage dstregnum, place into dst. */
void pshader_glsl_texdp3tex(SHADER_OPCODE_ARG* arg) {
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];
    char dst_mask[6];
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, src0_name, src0_mask, src0_str);

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_addline(arg->buffer, "texture1D(Psampler%u, dot(gl_TexCoord[%u].xyz, %s))%s);\n",
            sampler_idx, sampler_idx, src0_str, dst_mask);
}

/** Process the WINED3DSIO_TEXDP3 instruction in GLSL:
 * Take a 3-component dot product of the TexCoord[dstreg] and src. */
void pshader_glsl_texdp3(SHADER_OPCODE_ARG* arg) {
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];
    DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD dst_mask;
    size_t mask_size;

    dst_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(dst_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, src0_name, src0_mask, src0_str);

    if (mask_size > 1) {
        shader_addline(arg->buffer, "vec%d(dot(T%u.xyz, %s)));\n", mask_size, dstreg, src0_str);
    } else {
        shader_addline(arg->buffer, "dot(T%u.xyz, %s));\n", dstreg, src0_str);
    }
}

/** Process the WINED3DSIO_TEXDEPTH instruction in GLSL:
 * Calculate the depth as dst.x / dst.y   */
void pshader_glsl_texdepth(SHADER_OPCODE_ARG* arg) {
    
    char dst_str[100];
    char dst_reg[50];
    char dst_mask[6];
   
    shader_glsl_add_dst_param(arg, arg->dst, 0, dst_reg, dst_mask, dst_str);

    shader_addline(arg->buffer, "gl_FragDepth = %s.x / %s.y;\n", dst_reg, dst_reg);
}

/** Process the WINED3DSIO_TEXM3X2DEPTH instruction in GLSL:
 * Last row of a 3x2 matrix multiply, use the result to calculate the depth:
 * Calculate tmp0.y = TexCoord[dstreg] . src.xyz;  (tmp0.x has already been calculated)
 * depth = (tmp0.y == 0.0) ? 1.0 : tmp0.x / tmp0.y
 */
void pshader_glsl_texm3x2depth(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
    char src0_str[100], src0_name[50], src0_mask[6];

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, src0_name, src0_mask, src0_str);

    shader_addline(arg->buffer, "tmp0.y = dot(T%u.xyz, %s);\n", dstreg, src0_str);
    shader_addline(arg->buffer, "gl_FragDepth = (tmp0.y == 0.0) ? 1.0 : clamp(tmp0.x / tmp0.y, 0.0, 1.0);\n");
}

/** Process the WINED3DSIO_TEXM3X2PAD instruction in GLSL
 * Calculate the 1st of a 2-row matrix multiplication. */
void pshader_glsl_texm3x2pad(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, src0_name, src0_mask, src0_str);
    shader_addline(buffer, "tmp0.x = dot(T%u.xyz, %s);\n", reg, src0_str);
}

/** Process the WINED3DSIO_TEXM3X3PAD instruction in GLSL
 * Calculate the 1st or 2nd row of a 3-row matrix multiplication. */
void pshader_glsl_texm3x3pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, src0_name, src0_mask, src0_str);
    shader_addline(buffer, "tmp0.%c = dot(T%u.xyz, %s);\n", 'x' + current_state->current_row, reg, src0_str);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

void pshader_glsl_texm3x2tex(SHADER_OPCODE_ARG* arg) {
    DWORD src_mask = WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1 | WINED3DSP_WRITEMASK_2;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];
    char dst_mask[6];

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], src_mask, src0_name, src0_mask, src0_str);
    shader_addline(buffer, "tmp0.y = dot(T%u.xyz, %s);\n", reg, src0_str);

    shader_glsl_append_dst(buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);

    /* Sample the texture using the calculated coordinates */
    shader_addline(buffer, "texture2D(Psampler%u, tmp0.xy)%s);\n", reg, dst_mask);
}

/** Process the WINED3DSIO_TEXM3X3TEX instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply, then sample the texture using the calculated coordinates */
void pshader_glsl_texm3x3tex(SHADER_OPCODE_ARG* arg) {
    char dst_str[8];
    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;

    shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], src0_name, src0_mask, src0_str);
    shader_addline(arg->buffer, "tmp0.z = dot(vec3(T%u), vec3(%s));\n", reg, src0_str);

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    shader_glsl_sample(arg, reg, dst_str, "tmp0");
    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3 instruction in GLSL
 * Perform the 3rd row of a 3x3 matrix multiply */
void pshader_glsl_texm3x3(SHADER_OPCODE_ARG* arg) {

    char src0_str[100];
    char src0_name[50];
    char src0_mask[6];
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    
    shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], src0_name, src0_mask, src0_str);
    
    shader_addline(arg->buffer, "tmp0.z = dot(vec3(T%u), vec3(%s));\n", reg, src0_str);
    shader_addline(arg->buffer, "T%u = vec4(tmp0.x, tmp0.y, tmp0.z, 1.0);\n", reg);
    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3SPEC instruction in GLSL 
 * Peform the final texture lookup based on the previous 2 3x3 matrix multiplies */
void pshader_glsl_texm3x3spec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    char dimensions[5];
    char dst_str[8];
    char src0_str[100], src0_name[50], src0_mask[6];
    char src1_str[100], src1_name[50], src1_mask[6];
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    DWORD stype = arg->reg_maps->samplers[reg] & WINED3DSP_TEXTURETYPE_MASK;

    switch (stype) {
        case WINED3DSTT_2D:     strcpy(dimensions, "2D");   break;
        case WINED3DSTT_CUBE:   strcpy(dimensions, "Cube"); break;
        case WINED3DSTT_VOLUME: strcpy(dimensions, "3D");   break;
        default:
            strcpy(dimensions, "");
            FIXME("Unrecognized sampler type: %#x\n", stype);
            break;
    }

    shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], src0_name, src0_mask, src0_str);
    shader_glsl_add_src_param_old(arg, arg->src[1], arg->src_addr[1], src1_name, src1_mask, src1_str);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(vec3(T%u), vec3(%s));\n", reg, src0_str);

    /* Calculate reflection vector */
    shader_addline(buffer, "tmp0.xyz = reflect(-vec3(%s), vec3(tmp0));\n", src1_str);

    /* Sample the texture */
    sprintf(dst_str, "T%u", reg);
    shader_glsl_sample(arg, reg, dst_str, "tmp0");
    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXM3X3VSPEC instruction in GLSL 
 * Peform the final texture lookup based on the previous 2 3x3 matrix multiplies */
void pshader_glsl_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* shader = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &shader->baseShader.parse_state;
    char dst_str[8];
    char src0_str[100], src0_name[50], src0_mask[6];

    shader_glsl_add_src_param_old(arg, arg->src[0], arg->src_addr[0], src0_name, src0_mask, src0_str);

    /* Perform the last matrix multiply operation */
    shader_addline(buffer, "tmp0.z = dot(vec3(T%u), vec3(%s));\n", reg, src0_str);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "tmp1.x = gl_TexCoord[%u].w;\n", current_state->texcoord_w[0]);
    shader_addline(buffer, "tmp1.y = gl_TexCoord[%u].w;\n", current_state->texcoord_w[1]);
    shader_addline(buffer, "tmp1.z = gl_TexCoord[%u].w;\n", reg);

    /* Calculate reflection vector (Assume normal is normalized): RF = 2*(N.E)*N -E */
    shader_addline(buffer, "tmp0.x = dot(vec3(tmp0), vec3(tmp1));\n");
    shader_addline(buffer, "tmp0 = tmp0.w * tmp0;\n");
    shader_addline(buffer, "tmp0 = (2.0 * tmp0) - tmp1;\n");

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    shader_glsl_sample(arg, reg, dst_str, "tmp0");
    current_state->current_row = 0;
}

/** Process the WINED3DSIO_TEXBEM instruction in GLSL.
 * Apply a fake bump map transform.
 * FIXME: Should apply the BUMPMAPENV matrix.  For now, just sample the texture */
void pshader_glsl_texbem(SHADER_OPCODE_ARG* arg) {

    DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD reg2 = arg->src[0] & WINED3DSP_REGNUM_MASK;

    FIXME("Not applying the BUMPMAPENV matrix for pixel shader instruction texbem.\n");
    shader_addline(arg->buffer, "T%u = texture2D(Psampler%u, gl_TexCoord[%u].xy + T%u.xy);\n",
            reg1, reg1, reg1, reg2);
}

/** Process the WINED3DSIO_TEXREG2AR instruction in GLSL
 * Sample 2D texture at dst using the alpha & red (wx) components of src as texture coordinates */
void pshader_glsl_texreg2ar(SHADER_OPCODE_ARG* arg) {
    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    char dst_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_ALL, src0_reg, src0_mask, src0_str);

    shader_addline(arg->buffer, "texture2D(Psampler%u, %s.wx)%s);\n", sampler_idx, src0_reg, dst_mask);
}

/** Process the WINED3DSIO_TEXREG2GB instruction in GLSL
 * Sample 2D texture at dst using the green & blue (yz) components of src as texture coordinates */
void pshader_glsl_texreg2gb(SHADER_OPCODE_ARG* arg) {
    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    char dst_mask[6];

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_ALL, src0_reg, src0_mask, src0_str);

    shader_addline(arg->buffer, "texture2D(Psampler%u, %s.yz)%s);\n", sampler_idx, src0_reg, dst_mask);
}

/** Process the WINED3DSIO_TEXREG2RGB instruction in GLSL
 * Sample texture at dst using the rgb (xyz) components of src as texture coordinates */
void pshader_glsl_texreg2rgb(SHADER_OPCODE_ARG* arg) {
    char src0_str[100];
    char src0_reg[50];
    char src0_mask[6];
    char dst_mask[6];
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    glsl_sample_function_t sample_function;

    shader_glsl_append_dst(arg->buffer, arg);
    shader_glsl_get_write_mask(arg->dst, dst_mask);
    shader_glsl_get_sample_function(sampler_type, FALSE, &sample_function);
    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], sample_function.coord_mask, src0_reg, src0_mask, src0_str);

    shader_addline(arg->buffer, "%s(Psampler%u, %s)%s);\n", sample_function.name, sampler_idx, src0_str, dst_mask);
}

/** Process the WINED3DSIO_TEXKILL instruction in GLSL.
 * If any of the first 3 components are < 0, discard this pixel */
void pshader_glsl_texkill(SHADER_OPCODE_ARG* arg) {

    char dst_str[100], dst_name[50], dst_mask[6];

    shader_glsl_add_dst_param(arg, arg->dst, 0, dst_name, dst_mask, dst_str);
    shader_addline(arg->buffer, "if (any(lessThan(%s.xyz, vec3(0.0)))) discard;\n", dst_name);
}

/** Process the WINED3DSIO_DP2ADD instruction in GLSL.
 * dst = dot2(src0, src1) + src2 */
void pshader_glsl_dp2add(SHADER_OPCODE_ARG* arg) {
    char src0_str[100], src1_str[100], src2_str[100];
    char src0_reg[50], src1_reg[50], src2_reg[50];
    char src0_mask[6], src1_mask[6], src2_mask[6];
    DWORD write_mask;
    size_t mask_size;

    write_mask = shader_glsl_append_dst(arg->buffer, arg);
    mask_size = shader_glsl_get_write_mask_size(write_mask);

    shader_glsl_add_src_param(arg, arg->src[0], arg->src_addr[0], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, src0_reg, src0_mask, src0_str);
    shader_glsl_add_src_param(arg, arg->src[1], arg->src_addr[1], WINED3DSP_WRITEMASK_0 | WINED3DSP_WRITEMASK_1, src1_reg, src1_mask, src1_str);
    shader_glsl_add_src_param(arg, arg->src[2], arg->src_addr[2], WINED3DSP_WRITEMASK_0, src2_reg, src2_mask, src2_str);

    shader_addline(arg->buffer, "dot(%s, %s) + %s);\n", src0_str, src1_str, src2_str);
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
       usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
       usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
       shader_glsl_get_write_mask(register_token, reg_mask);

       switch(usage) {

           case D3DDECLUSAGE_COLOR:
               if (usage_idx == 0)
                   shader_addline(buffer, "IN%u%s = vec4(gl_Color)%s;\n",
                       i, reg_mask, reg_mask);
               else if (usage_idx == 1)
                   shader_addline(buffer, "IN%u%s = vec4(gl_SecondaryColor)%s;\n",
                       i, reg_mask, reg_mask);
               else
                   shader_addline(buffer, "IN%u%s = vec4(unsupported_color_input)%s;\n",
                       i, reg_mask, reg_mask);
               break;

           case D3DDECLUSAGE_TEXCOORD:
               shader_addline(buffer, "IN%u%s = vec4(gl_TexCoord[%u])%s;\n",
                   i, reg_mask, usage_idx, reg_mask );
               break;

           case D3DDECLUSAGE_FOG:
               shader_addline(buffer, "IN%u%s = vec4(gl_FogFragCoord)%s;\n",
                   i, reg_mask, reg_mask);
               break;

           default:
               shader_addline(buffer, "IN%u%s = vec4(unsupported_input)%s;\n",
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

       usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
       usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
       shader_glsl_get_write_mask(register_token, reg_mask);

       switch(usage) {

           case D3DDECLUSAGE_COLOR:
               if (usage_idx == 0)
                   shader_addline(buffer, "gl_FrontColor%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               else if (usage_idx == 1)
                   shader_addline(buffer, "gl_FrontSecondaryColor%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               else
                   shader_addline(buffer, "unsupported_color_output%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               break;

           case D3DDECLUSAGE_POSITION:
               shader_addline(buffer, "gl_Position%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               break;
 
           case D3DDECLUSAGE_TEXCOORD:
               shader_addline(buffer, "gl_TexCoord[%u]%s = OUT%u%s;\n",
                   usage_idx, reg_mask, i, reg_mask);
               break;

           case WINED3DSHADERDECLUSAGE_PSIZE:
               shader_addline(buffer, "gl_PointSize = OUT%u.x;\n", i);
               break;

           case WINED3DSHADERDECLUSAGE_FOG:
               shader_addline(buffer, "gl_FogFragCoord%s = OUT%u%s;\n", reg_mask, i, reg_mask);
               break;

           default:
               shader_addline(buffer, "unsupported_output%s = OUT%u%s;\n", reg_mask, i, reg_mask);
       }
    }
}

/** Attach a GLSL pixel or vertex shader object to the shader program */
static void attach_glsl_shader(IWineD3DDevice *iface, IWineD3DBaseShader* shader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl *)(This->wineD3D))->gl_info;
    GLhandleARB shaderObj = ((IWineD3DBaseShaderImpl*)shader)->baseShader.prgId;
    if (This->stateBlock->glsl_program && shaderObj != 0) {
        TRACE("Attaching GLSL shader object %u to program %u\n", shaderObj, This->stateBlock->glsl_program->programId);
        GL_EXTCALL(glAttachObjectARB(This->stateBlock->glsl_program->programId, shaderObj));
        checkGLcall("glAttachObjectARB");
    }
}

/** Sets the GLSL program ID for the given pixel and vertex shader combination.
 * It sets the programId on the current StateBlock (because it should be called
 * inside of the DrawPrimitive() part of the render loop).
 *
 * If a program for the given combination does not exist, create one, and store
 * the program in the list.  If it creates a program, it will link the given
 * objects, too.
 *
 * We keep the shader programs around on a list because linking
 * shader objects together is an expensive operation.  It's much
 * faster to loop through a list of pre-compiled & linked programs
 * each time that the application sets a new pixel or vertex shader
 * than it is to re-link them together at that time.
 *
 * The list will be deleted in IWineD3DDevice::Release().
 */
static void set_glsl_shader_program(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This               = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info               = &((IWineD3DImpl *)(This->wineD3D))->gl_info;
    IWineD3DPixelShader  *pshader          = This->stateBlock->pixelShader;
    IWineD3DVertexShader *vshader          = This->stateBlock->vertexShader;
    struct glsl_shader_prog_link *curLink  = NULL;
    struct glsl_shader_prog_link *newLink  = NULL;
    struct list *ptr                       = NULL;
    GLhandleARB programId                  = 0;
    int i;
    char glsl_name[8];

    ptr = list_head( &This->glsl_shader_progs );
    while (ptr) {
        /* At least one program exists - see if it matches our ps/vs combination */
        curLink = LIST_ENTRY( ptr, struct glsl_shader_prog_link, entry );
        if (vshader == curLink->vertexShader && pshader == curLink->pixelShader) {
            /* Existing Program found, use it */
            TRACE("Found existing program (%u) for this vertex/pixel shader combination\n",
                   curLink->programId);
            This->stateBlock->glsl_program = curLink;
            return;
        }
        /* This isn't the entry we need - try the next one */
        ptr = list_next( &This->glsl_shader_progs, ptr );
    }

    /* If we get to this point, then no matching program exists, so we create one */
    programId = GL_EXTCALL(glCreateProgramObjectARB());
    TRACE("Created new GLSL shader program %u\n", programId);

    /* Allocate a new link for the list of programs */
    newLink = HeapAlloc(GetProcessHeap(), 0, sizeof(struct glsl_shader_prog_link));
    newLink->programId    = programId;
    This->stateBlock->glsl_program = newLink;

    /* Attach GLSL vshader */
    if (NULL != vshader && This->vs_selected_mode == SHADER_GLSL) {
        int i;
        int max_attribs = 16;   /* TODO: Will this always be the case? It is at the moment... */
        char tmp_name[10];

        TRACE("Attaching vertex shader to GLSL program\n");
        attach_glsl_shader(iface, (IWineD3DBaseShader*)vshader);

        /* Bind vertex attributes to a corresponding index number to match
         * the same index numbers as ARB_vertex_programs (makes loading
         * vertex attributes simpler).  With this method, we can use the
         * exact same code to load the attributes later for both ARB and
         * GLSL shaders.
         *
         * We have to do this here because we need to know the Program ID
         * in order to make the bindings work, and it has to be done prior
         * to linking the GLSL program. */
        for (i = 0; i < max_attribs; ++i) {
             snprintf(tmp_name, sizeof(tmp_name), "attrib%i", i);
             GL_EXTCALL(glBindAttribLocationARB(programId, i, tmp_name));
        }
        checkGLcall("glBindAttribLocationARB");
        newLink->vertexShader = vshader;
    }

    /* Attach GLSL pshader */
    if (NULL != pshader && This->ps_selected_mode == SHADER_GLSL) {
        TRACE("Attaching pixel shader to GLSL program\n");
        attach_glsl_shader(iface, (IWineD3DBaseShader*)pshader);
        newLink->pixelShader = pshader;
    }

    /* Link the program */
    TRACE("Linking GLSL shader program %u\n", programId);
    GL_EXTCALL(glLinkProgramARB(programId));
    print_glsl_info_log(&GLINFO_LOCATION, programId);
    list_add_head( &This->glsl_shader_progs, &newLink->entry);

    newLink->vuniformF_locations = HeapAlloc(GetProcessHeap(), 0, sizeof(GLhandleARB) * GL_LIMITS(vshader_constantsF));
    for (i = 0; i < GL_LIMITS(vshader_constantsF); ++i) {
        snprintf(glsl_name, sizeof(glsl_name), "VC[%i]", i);
        newLink->vuniformF_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }
    newLink->puniformF_locations = HeapAlloc(GetProcessHeap(), 0, sizeof(GLhandleARB) * GL_LIMITS(pshader_constantsF));
    for (i = 0; i < GL_LIMITS(pshader_constantsF); ++i) {
        snprintf(glsl_name, sizeof(glsl_name), "PC[%i]", i);
        newLink->puniformF_locations[i] = GL_EXTCALL(glGetUniformLocationARB(programId, glsl_name));
    }

    return;
}

static GLhandleARB create_glsl_blt_shader(WineD3D_GL_Info *gl_info) {
    GLhandleARB program_id;
    GLhandleARB vshader_id, pshader_id;
    const char *blt_vshader[] = {
        "void main(void)\n"
        "{\n"
        "    gl_Position = gl_Vertex;\n"
        "    gl_FrontColor = vec4(1.0);\n"
        "    gl_TexCoord[0].x = (gl_Vertex.x * 0.5) + 0.5;\n"
        "    gl_TexCoord[0].y = (-gl_Vertex.y * 0.5) + 0.5;\n"
        "}\n"
    };

    const char *blt_pshader[] = {
        "uniform sampler2D sampler;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragDepth = texture2D(sampler, gl_TexCoord[0].xy).x;\n"
        "}\n"
    };

    vshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB));
    GL_EXTCALL(glShaderSourceARB(vshader_id, 1, blt_vshader, NULL));
    GL_EXTCALL(glCompileShaderARB(vshader_id));

    pshader_id = GL_EXTCALL(glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB));
    GL_EXTCALL(glShaderSourceARB(pshader_id, 1, blt_pshader, NULL));
    GL_EXTCALL(glCompileShaderARB(pshader_id));

    program_id = GL_EXTCALL(glCreateProgramObjectARB());
    GL_EXTCALL(glAttachObjectARB(program_id, vshader_id));
    GL_EXTCALL(glAttachObjectARB(program_id, pshader_id));
    GL_EXTCALL(glLinkProgramARB(program_id));

    print_glsl_info_log(&GLINFO_LOCATION, program_id);

    return program_id;
}

static void shader_glsl_select(IWineD3DDevice *iface, BOOL usePS, BOOL useVS) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl *)(This->wineD3D))->gl_info;
    GLhandleARB program_id = 0;

    if (useVS || usePS) set_glsl_shader_program(iface);
    else This->stateBlock->glsl_program = NULL;

    program_id = This->stateBlock->glsl_program ? This->stateBlock->glsl_program->programId : 0;
    if (program_id) TRACE("Using GLSL program %u\n", program_id);
    GL_EXTCALL(glUseProgramObjectARB(program_id));
    checkGLcall("glUseProgramObjectARB");
}

static void shader_glsl_select_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &((IWineD3DImpl *)(This->wineD3D))->gl_info;
    static GLhandleARB program_id = 0;
    static GLhandleARB loc = -1;

    if (!program_id) {
        program_id = create_glsl_blt_shader(gl_info);
        loc = GL_EXTCALL(glGetUniformLocationARB(program_id, "sampler"));
    }

    GL_EXTCALL(glUseProgramObjectARB(program_id));
    GL_EXTCALL(glUniform1iARB(loc, 0));
}

static void shader_glsl_cleanup(BOOL usePS, BOOL useVS) {
    /* Nothing to do */
}

const shader_backend_t glsl_shader_backend = {
    &shader_glsl_select,
    &shader_glsl_select_depth_blt,
    &shader_glsl_load_constants,
    &shader_glsl_cleanup
};
