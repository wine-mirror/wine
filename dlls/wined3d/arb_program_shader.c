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

/* TODO: Add more ARB_[vertex/fragment]_program specific code here */
