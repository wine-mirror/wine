/*
 * WINED3D draw functions
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_draw);
WINE_DECLARE_DEBUG_CHANNEL(d3d_shader);
#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info

#include <stdio.h>

#if 0 /* TODO */
extern IWineD3DVertexShaderImpl*            VertexShaders[64];
extern IWineD3DVertexDeclarationImpl*       VertexShaderDeclarations[64];
extern IWineD3DPixelShaderImpl*             PixelShaders[64];

#undef GL_VERSION_1_4 /* To be fixed, caused by mesa headers */
#endif

/* Issues the glBegin call for gl given the primitive type and count */
static DWORD primitiveToGl(D3DPRIMITIVETYPE PrimitiveType,
                    DWORD            NumPrimitives,
                    GLenum          *primType)
{
    DWORD   NumVertexes = NumPrimitives;

    switch (PrimitiveType) {
    case D3DPT_POINTLIST:
        TRACE("POINTS\n");
        *primType   = GL_POINTS;
        NumVertexes = NumPrimitives;
        break;

    case D3DPT_LINELIST:
        TRACE("LINES\n");
        *primType   = GL_LINES;
        NumVertexes = NumPrimitives * 2;
        break;

    case D3DPT_LINESTRIP:
        TRACE("LINE_STRIP\n");
        *primType   = GL_LINE_STRIP;
        NumVertexes = NumPrimitives + 1;
        break;

    case D3DPT_TRIANGLELIST:
        TRACE("TRIANGLES\n");
        *primType   = GL_TRIANGLES;
        NumVertexes = NumPrimitives * 3;
        break;

    case D3DPT_TRIANGLESTRIP:
        TRACE("TRIANGLE_STRIP\n");
        *primType   = GL_TRIANGLE_STRIP;
        NumVertexes = NumPrimitives + 2;
        break;

    case D3DPT_TRIANGLEFAN:
        TRACE("TRIANGLE_FAN\n");
        *primType   = GL_TRIANGLE_FAN;
        NumVertexes = NumPrimitives + 2;
        break;

    default:
        FIXME("Unhandled primitive\n");
        *primType    = GL_POINTS;
        break;
    }
    return NumVertexes;
}

/* Ensure the appropriate material states are set up - only change
   state if really required                                        */
static void init_materials(IWineD3DDevice *iface, BOOL isDiffuseSupplied) {

    BOOL requires_material_reset = FALSE;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (This->tracking_color == NEEDS_TRACKING && isDiffuseSupplied) {
        /* If we have not set up the material color tracking, do it now as required */
        glDisable(GL_COLOR_MATERIAL); /* Note: Man pages state must enable AFTER calling glColorMaterial! Required?*/
        checkGLcall("glDisable GL_COLOR_MATERIAL");
        TRACE("glColorMaterial Parm=%x\n", This->tracking_parm);
        glColorMaterial(GL_FRONT_AND_BACK, This->tracking_parm);
        checkGLcall("glColorMaterial(GL_FRONT_AND_BACK, Parm)");
        glEnable(GL_COLOR_MATERIAL);
        checkGLcall("glEnable GL_COLOR_MATERIAL");
        This->tracking_color = IS_TRACKING;
        requires_material_reset = TRUE; /* Restore material settings as will be used */

    } else if ((This->tracking_color == IS_TRACKING && isDiffuseSupplied == FALSE) ||
               (This->tracking_color == NEEDS_TRACKING && isDiffuseSupplied == FALSE)) {
        /* If we are tracking the current color but one isn't supplied, don't! */
        glDisable(GL_COLOR_MATERIAL);
        checkGLcall("glDisable GL_COLOR_MATERIAL");
        This->tracking_color = NEEDS_TRACKING;
        requires_material_reset = TRUE; /* Restore material settings as will be used */

    } else if (This->tracking_color == IS_TRACKING && isDiffuseSupplied) {
        /* No need to reset material colors since no change to gl_color_material */
        requires_material_reset = FALSE;

    } else if (This->tracking_color == NEEDS_DISABLE) {
        glDisable(GL_COLOR_MATERIAL);
        checkGLcall("glDisable GL_COLOR_MATERIAL");
        This->tracking_color = DISABLED_TRACKING;
        requires_material_reset = TRUE; /* Restore material settings as will be used */
    }

    /* Reset the material colors which may have been tracking the color*/
    if (requires_material_reset) {
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*) &This->stateBlock->material.Ambient);
        checkGLcall("glMaterialfv");
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*) &This->stateBlock->material.Diffuse);
        checkGLcall("glMaterialfv");
        if (This->stateBlock->renderState[WINED3DRS_SPECULARENABLE]) {
           glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &This->stateBlock->material.Specular);
           checkGLcall("glMaterialfv");
        } else {
           float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
           glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
           checkGLcall("glMaterialfv");
        }
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float*) &This->stateBlock->material.Emissive);
        checkGLcall("glMaterialfv");
    }

}

static GLfloat invymat[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, -1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f};

void d3ddevice_set_ortho(IWineD3DDeviceImpl *This) {
    /* If the last draw was transformed as well, no need to reapply all the matrixes */
    if ( (!This->last_was_rhw) || (This->viewport_changed) ) {

        double X, Y, height, width, minZ, maxZ;
        This->last_was_rhw = TRUE;
        This->viewport_changed = FALSE;

        /* Transformed already into viewport coordinates, so we do not need transform
            matrices. Reset all matrices to identity and leave the default matrix in world
            mode.                                                                         */
        glMatrixMode(GL_MODELVIEW);
        checkGLcall("glMatrixMode(GL_MODELVIEW)");
        glLoadIdentity();
        checkGLcall("glLoadIdentity");

        glMatrixMode(GL_PROJECTION);
        checkGLcall("glMatrixMode(GL_PROJECTION)");
        glLoadIdentity();
        checkGLcall("glLoadIdentity");

        /* Set up the viewport to be full viewport */
        X      = This->stateBlock->viewport.X;
        Y      = This->stateBlock->viewport.Y;
        height = This->stateBlock->viewport.Height;
        width  = This->stateBlock->viewport.Width;
        minZ   = This->stateBlock->viewport.MinZ;
        maxZ   = This->stateBlock->viewport.MaxZ;
        if(!This->untransformed) {
            TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, -minZ, -maxZ);
            glOrtho(X, X + width, Y + height, Y, -minZ, -maxZ);
        } else {
            TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, 1.0, -1.0);
            glOrtho(X, X + width, Y + height, Y, 1.0, -1.0);
        }
        checkGLcall("glOrtho");

        /* Window Coord 0 is the middle of the first pixel, so translate by half
            a pixel (See comment above glTranslate below)                         */
        glTranslatef(0.375, 0.375, 0);
        checkGLcall("glTranslatef(0.375, 0.375, 0)");
        if (This->renderUpsideDown) {
            glMultMatrixf(invymat);
            checkGLcall("glMultMatrixf(invymat)");
        }

        /* Vertex fog on transformed vertices? Use the calculated fog factor stored in the specular color */
        if(This->stateBlock->renderState[WINED3DRS_FOGENABLE] && This->stateBlock->renderState[WINED3DRS_FOGVERTEXMODE] != D3DFOG_NONE) {
            if(GL_SUPPORT(EXT_FOG_COORD)) {
                glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
                checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT)");
                glFogi(GL_FOG_MODE, GL_LINEAR);
                checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR)");
                /* The dx fog range in this case is fixed to 0 - 255,
                 * but in GL it still depends on the fog start and end (according to the ext)
                 * Use this to turn around the fog as it's needed. That prevents some
                 * calculations during drawing :-)
                 */
                glFogf(GL_FOG_START, (float) 0xff);
                checkGLcall("glFogfv GL_FOG_END");
                glFogf(GL_FOG_END, 0.0);
                checkGLcall("glFogfv GL_FOG_START");
            } else {
                /* Disable GL fog, handle this in software in drawStridedSlow */
                glDisable(GL_FOG);
                checkGLcall("glDisable(GL_FOG)");
            }
        }
    }
}
/* Setup views - Transformed & lit if RHW, else untransformed.
       Only unlit if Normals are supplied
    Returns: Whether to restore lighting afterwards           */
static void primitiveInitState(
    IWineD3DDevice *iface,
    WineDirect3DVertexStridedData* strided,
    BOOL useVS,
    BOOL* lighting_changed,
    BOOL* lighting_original) {

    BOOL fixed_vtx_transformed =
       (strided->u.s.position.lpData != NULL || strided->u.s.position.VBO != 0 ||
        strided->u.s.position2.lpData != NULL || strided->u.s.position2.VBO != 0) && 
        strided->u.s.position_transformed;

    BOOL fixed_vtx_lit = 
        strided->u.s.normal.lpData == NULL && strided->u.s.normal.VBO == 0 &&
        strided->u.s.normal2.lpData == NULL && strided->u.s.normal2.VBO == 0;

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    *lighting_changed = FALSE;

    /* If no normals, DISABLE lighting otherwise, don't touch lighing as it is
       set by the appropriate render state. Note Vertex Shader output is already lit */
    if (fixed_vtx_lit || useVS) {
        *lighting_changed = TRUE;
        *lighting_original = glIsEnabled(GL_LIGHTING);
        glDisable(GL_LIGHTING);
        checkGLcall("glDisable(GL_LIGHTING);");
        TRACE("Disabled lighting, old state = %d\n", *lighting_original);
    }

    if (!useVS && fixed_vtx_transformed) {
        d3ddevice_set_ortho(This);

    } else {

        /* Untransformed, so relies on the view and projection matrices */
        This->untransformed = TRUE;

        if (!useVS && (This->last_was_rhw || !This->modelview_valid)) {
            /* Only reapply when have to */
            This->modelview_valid = TRUE;
            glMatrixMode(GL_MODELVIEW);
            checkGLcall("glMatrixMode");

            /* In the general case, the view matrix is the identity matrix */
            if (This->view_ident) {
                glLoadMatrixf((float *) &This->stateBlock->transforms[WINED3DTS_WORLDMATRIX(0)].u.m[0][0]);
                checkGLcall("glLoadMatrixf");
            } else {
                glLoadMatrixf((float *) &This->stateBlock->transforms[WINED3DTS_VIEW].u.m[0][0]);
                checkGLcall("glLoadMatrixf");
                glMultMatrixf((float *) &This->stateBlock->transforms[WINED3DTS_WORLDMATRIX(0)].u.m[0][0]);
                checkGLcall("glMultMatrixf");
            }
        }

        if (!useVS && (This->last_was_rhw || !This->proj_valid)) {
            /* Only reapply when have to */
            This->proj_valid = TRUE;
            glMatrixMode(GL_PROJECTION);
            checkGLcall("glMatrixMode");

            /* The rule is that the window coordinate 0 does not correspond to the
               beginning of the first pixel, but the center of the first pixel.
               As a consequence if you want to correctly draw one line exactly from
               the left to the right end of the viewport (with all matrices set to
               be identity), the x coords of both ends of the line would be not
               -1 and 1 respectively but (-1-1/viewport_widh) and (1-1/viewport_width)
               instead.                                                               */
            glLoadIdentity();

            glTranslatef(0.9 / This->stateBlock->viewport.Width, -0.9 / This->stateBlock->viewport.Height, 0);
            checkGLcall("glTranslatef (0.9 / width, -0.9 / height, 0)");

            if (This->renderUpsideDown) {
                glMultMatrixf(invymat);
                checkGLcall("glMultMatrixf(invymat)");
            }
            glMultMatrixf((float *) &This->stateBlock->transforms[WINED3DTS_PROJECTION].u.m[0][0]);
            checkGLcall("glLoadMatrixf");
        }

        /* Vertex Shader output is already transformed, so set up identity matrices */
        if (useVS) {
            glMatrixMode(GL_MODELVIEW);
            checkGLcall("glMatrixMode");
            glLoadIdentity();
            glMatrixMode(GL_PROJECTION);
            checkGLcall("glMatrixMode");
            glLoadIdentity();
            /* Window Coord 0 is the middle of the first pixel, so translate by half
               a pixel (See comment above glTranslate above)                         */
            glTranslatef(0.9 / This->stateBlock->viewport.Width, -0.9 / This->stateBlock->viewport.Height, 0);
            checkGLcall("glTranslatef (0.9 / width, -0.9 / height, 0)");
            if (This->renderUpsideDown) {
                glMultMatrixf(invymat);
                checkGLcall("glMultMatrixf(invymat)");
            }
            This->modelview_valid = FALSE;
            This->proj_valid = FALSE;
        }
        This->last_was_rhw = FALSE;

        /* Setup fogging */
        if (useVS && ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->usesFog) {
            /* In D3D vertex shader return the 'final' fog value, while in OpenGL it is the 'input' fog value.
             * The code below 'disables' the OpenGL postprocessing by setting the formula to '1'. */
            glFogi(GL_FOG_MODE, GL_LINEAR);
            glFogf(GL_FOG_START, 1.0f);
            glFogf(GL_FOG_END, 0.0f);

        } else if(This->stateBlock->renderState[WINED3DRS_FOGENABLE] 
                  && This->stateBlock->renderState[WINED3DRS_FOGVERTEXMODE] != D3DFOG_NONE) {
            
            if(GL_SUPPORT(EXT_FOG_COORD)) {
                glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
                checkGLcall("glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT)\n");
                /* Reapply the fog range */
                IWineD3DDevice_SetRenderState(iface, WINED3DRS_FOGSTART, This->stateBlock->renderState[WINED3DRS_FOGSTART]);
                IWineD3DDevice_SetRenderState(iface, WINED3DRS_FOGEND, This->stateBlock->renderState[WINED3DRS_FOGEND]);
                /* Restore the fog mode */
                IWineD3DDevice_SetRenderState(iface, WINED3DRS_FOGTABLEMODE, This->stateBlock->renderState[WINED3DRS_FOGTABLEMODE]);
            } else {
                /* Enable GL_FOG again because we disabled it above */
                glEnable(GL_FOG);
                checkGLcall("glEnable(GL_FOG)");
            }
        }
    }
}

static BOOL fixed_get_input(
    BYTE usage, BYTE usage_idx,
    unsigned int* regnum) {

    *regnum = -1;

    /* Those positions must have the order in the
     * named part of the strided data */

    if ((usage == D3DDECLUSAGE_POSITION || usage == D3DDECLUSAGE_POSITIONT) && usage_idx == 0)
        *regnum = 0;
    else if (usage == D3DDECLUSAGE_BLENDWEIGHT && usage_idx == 0)
        *regnum = 1;
    else if (usage == D3DDECLUSAGE_BLENDINDICES && usage_idx == 0)
        *regnum = 2;
    else if (usage == D3DDECLUSAGE_NORMAL && usage_idx == 0)
        *regnum = 3;
    else if (usage == D3DDECLUSAGE_PSIZE && usage_idx == 0)
        *regnum = 4;
    else if (usage == D3DDECLUSAGE_COLOR && usage_idx == 0)
        *regnum = 5;
    else if (usage == D3DDECLUSAGE_COLOR && usage_idx == 1)
        *regnum = 6;
    else if (usage == D3DDECLUSAGE_TEXCOORD && usage_idx < D3DDP_MAXTEXCOORD)
        *regnum = 7 + usage_idx;
    else if ((usage == D3DDECLUSAGE_POSITION || usage == D3DDECLUSAGE_POSITIONT) && usage_idx == 1)
        *regnum = 7 + D3DDP_MAXTEXCOORD;
    else if (usage == D3DDECLUSAGE_NORMAL && usage_idx == 1)
        *regnum = 8 + D3DDP_MAXTEXCOORD;
    else if (usage == D3DDECLUSAGE_TANGENT && usage_idx == 0)
        *regnum = 9 + D3DDP_MAXTEXCOORD;
    else if (usage == D3DDECLUSAGE_BINORMAL && usage_idx == 0)
        *regnum = 10 + D3DDP_MAXTEXCOORD;
    else if (usage == D3DDECLUSAGE_TESSFACTOR && usage_idx == 0)
        *regnum = 11 + D3DDP_MAXTEXCOORD;
    else if (usage == D3DDECLUSAGE_FOG && usage_idx == 0)
        *regnum = 12 + D3DDP_MAXTEXCOORD;
    else if (usage == D3DDECLUSAGE_DEPTH && usage_idx == 0)
        *regnum = 13 + D3DDP_MAXTEXCOORD;
    else if (usage == D3DDECLUSAGE_SAMPLE && usage_idx == 0)
        *regnum = 14 + D3DDP_MAXTEXCOORD;

    if (*regnum < 0) {
        FIXME("Unsupported input stream [usage=%s, usage_idx=%u]\n",
            debug_d3ddeclusage(usage), usage_idx);
        return FALSE;
    }
    return TRUE;
}

void primitiveDeclarationConvertToStridedData(
     IWineD3DDevice *iface,
     BOOL useVertexShaderFunction,
     WineDirect3DVertexStridedData *strided,
     LONG BaseVertexIndex, 
     BOOL *fixup) {

     /* We need to deal with frequency data!*/

    BYTE  *data    = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexDeclarationImpl* vertexDeclaration = NULL;
    int i;
    WINED3DVERTEXELEMENT *element;
    DWORD stride;
    int reg;

    /* Locate the vertex declaration */
    if (This->stateBlock->vertexShader && ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->vertexDeclaration) {
        TRACE("Using vertex declaration from shader\n");
        vertexDeclaration = (IWineD3DVertexDeclarationImpl *)((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->vertexDeclaration;
    } else {
        TRACE("Using vertex declaration\n");
        vertexDeclaration = (IWineD3DVertexDeclarationImpl *)This->stateBlock->vertexDecl;
    }

    /* Translate the declaration into strided data */
    for (i = 0 ; i < vertexDeclaration->declarationWNumElements - 1; ++i) {
        GLint streamVBO = 0;
        BOOL stride_used;
        unsigned int idx;

        element = vertexDeclaration->pDeclarationWine + i;
        TRACE("%p Elements %p %d or %d\n", vertexDeclaration->pDeclarationWine, element,  i, vertexDeclaration->declarationWNumElements);
        if (This->stateBlock->streamIsUP) {
            TRACE("Stream is up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            streamVBO = 0;
            data    = (BYTE *)This->stateBlock->streamSource[element->Stream];
            if(fixup && *fixup) FIXME("Missing fixed and unfixed vertices, expect graphics glitches\n");
        } else {
            TRACE("Stream isn't up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            IWineD3DVertexBuffer_PreLoad(This->stateBlock->streamSource[element->Stream]);
            data    = IWineD3DVertexBufferImpl_GetMemory(This->stateBlock->streamSource[element->Stream], 0, &streamVBO);
            if(fixup) {
                if( streamVBO != 0) *fixup = TRUE;
                else if(*fixup) FIXME("Missing fixed and unfixed vertices, expect graphics glitches\n");
            }
        }
        stride  = This->stateBlock->streamStride[element->Stream];
        data += (BaseVertexIndex * stride);
        data += element->Offset;
        reg = element->Reg;

        TRACE("Offset %d Stream %d UsageIndex %d\n", element->Offset, element->Stream, element->UsageIndex);

        if (useVertexShaderFunction)
            stride_used = vshader_get_input(This->stateBlock->vertexShader,
                element->Usage, element->UsageIndex, &idx);
        else
            stride_used = fixed_get_input(element->Usage, element->UsageIndex, &idx);

        if (stride_used) {
           TRACE("Loaded %s array %u [usage=%s, usage_idx=%u, "
                 "stream=%u, offset=%u, stride=%lu, VBO=%u]\n",
                 useVertexShaderFunction? "shader": "fixed function", idx,
                 debug_d3ddeclusage(element->Usage), element->UsageIndex,
                 element->Stream, element->Offset, stride, streamVBO);

           strided->u.input[idx].lpData = data;
           strided->u.input[idx].dwType = element->Type;
           strided->u.input[idx].dwStride = stride;
           strided->u.input[idx].VBO = streamVBO;
           if (!useVertexShaderFunction) {
               if (element->Usage == D3DDECLUSAGE_POSITION)
                   strided->u.s.position_transformed = FALSE;
               else if (element->Usage == D3DDECLUSAGE_POSITIONT)
                   strided->u.s.position_transformed = TRUE;
           }
        }
    };
}

void primitiveConvertFVFtoOffset(DWORD thisFVF, DWORD stride, BYTE *data, WineDirect3DVertexStridedData *strided, GLint streamVBO) {
    int           numBlends;
    int           numTextures;
    int           textureNo;
    int           coordIdxInfo = 0x00;    /* Information on number of coords supplied */
    int           numCoords[8];           /* Holding place for D3DFVF_TEXTUREFORMATx  */

    /* Either 3 or 4 floats depending on the FVF */
    /* FIXME: Can blending data be in a different stream to the position data?
          and if so using the fixed pipeline how do we handle it               */
    if (thisFVF & D3DFVF_POSITION_MASK) {
        strided->u.s.position.lpData    = data;
        strided->u.s.position.dwType    = D3DDECLTYPE_FLOAT3;
        strided->u.s.position.dwStride  = stride;
        strided->u.s.position.VBO       = streamVBO;
        data += 3 * sizeof(float);
        if (thisFVF & D3DFVF_XYZRHW) {
            strided->u.s.position.dwType = D3DDECLTYPE_FLOAT4;
            strided->u.s.position_transformed = TRUE;
            data += sizeof(float);
        } else
            strided->u.s.position_transformed = FALSE;
    }

    /* Blending is numBlends * FLOATs followed by a DWORD for UBYTE4 */
    /** do we have to Check This->stateBlock->renderState[D3DRS_INDEXEDVERTEXBLENDENABLE] ? */
    numBlends = 1 + (((thisFVF & D3DFVF_XYZB5) - D3DFVF_XYZB1) >> 1);
    if(thisFVF & D3DFVF_LASTBETA_UBYTE4) numBlends--;

    if ((thisFVF & D3DFVF_XYZB5 ) > D3DFVF_XYZRHW) {
        TRACE("Setting blend Weights to %p\n", data);
        strided->u.s.blendWeights.lpData    = data;
        strided->u.s.blendWeights.dwType    = D3DDECLTYPE_FLOAT1 + numBlends - 1;
        strided->u.s.blendWeights.dwStride  = stride;
        strided->u.s.blendWeights.VBO       = streamVBO;
        data += numBlends * sizeof(FLOAT);

        if (thisFVF & D3DFVF_LASTBETA_UBYTE4) {
            strided->u.s.blendMatrixIndices.lpData = data;
            strided->u.s.blendMatrixIndices.dwType  = D3DDECLTYPE_UBYTE4;
            strided->u.s.blendMatrixIndices.dwStride= stride;
            strided->u.s.blendMatrixIndices.VBO     = streamVBO;
            data += sizeof(DWORD);
        }
    }

    /* Normal is always 3 floats */
    if (thisFVF & D3DFVF_NORMAL) {
        strided->u.s.normal.lpData    = data;
        strided->u.s.normal.dwType    = D3DDECLTYPE_FLOAT3;
        strided->u.s.normal.dwStride  = stride;
        strided->u.s.normal.VBO     = streamVBO;
        data += 3 * sizeof(FLOAT);
    }

    /* Pointsize is a single float */
    if (thisFVF & D3DFVF_PSIZE) {
        strided->u.s.pSize.lpData    = data;
        strided->u.s.pSize.dwType    = D3DDECLTYPE_FLOAT1;
        strided->u.s.pSize.dwStride  = stride;
        strided->u.s.pSize.VBO       = streamVBO;
        data += sizeof(FLOAT);
    }

    /* Diffuse is 4 unsigned bytes */
    if (thisFVF & D3DFVF_DIFFUSE) {
        strided->u.s.diffuse.lpData    = data;
        strided->u.s.diffuse.dwType    = D3DDECLTYPE_SHORT4;
        strided->u.s.diffuse.dwStride  = stride;
        strided->u.s.diffuse.VBO       = streamVBO;
        data += sizeof(DWORD);
    }

    /* Specular is 4 unsigned bytes */
    if (thisFVF & D3DFVF_SPECULAR) {
        strided->u.s.specular.lpData    = data;
        strided->u.s.specular.dwType    = D3DDECLTYPE_SHORT4;
        strided->u.s.specular.dwStride  = stride;
        strided->u.s.specular.VBO       = streamVBO;
        data += sizeof(DWORD);
    }

    /* Texture coords */
    numTextures   = (thisFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    coordIdxInfo  = (thisFVF & 0x00FF0000) >> 16; /* 16 is from definition of D3DFVF_TEXCOORDSIZE1, and is 8 (0-7 stages) * 2bits long */

    /* numTextures indicates the number of texture coordinates supplied */
    /* However, the first set may not be for stage 0 texture - it all   */
    /*   depends on D3DTSS_TEXCOORDINDEX.                               */
    /* The number of bytes for each coordinate set is based off         */
    /*   D3DFVF_TEXCOORDSIZEn, which are the bottom 2 bits              */

    /* So, for each supplied texture extract the coords */
    for (textureNo = 0; textureNo < numTextures; ++textureNo) {

        strided->u.s.texCoords[textureNo].lpData    = data;
        strided->u.s.texCoords[textureNo].dwType    = D3DDECLTYPE_FLOAT1;
        strided->u.s.texCoords[textureNo].dwStride  = stride;
        strided->u.s.texCoords[textureNo].VBO       = streamVBO;
        numCoords[textureNo] = coordIdxInfo & 0x03;

        /* Always one set */
        data += sizeof(float);
        if (numCoords[textureNo] != D3DFVF_TEXTUREFORMAT1) {
            strided->u.s.texCoords[textureNo].dwType = D3DDECLTYPE_FLOAT2;
            data += sizeof(float);
            if (numCoords[textureNo] != D3DFVF_TEXTUREFORMAT2) {
                strided->u.s.texCoords[textureNo].dwType = D3DDECLTYPE_FLOAT3;
                data += sizeof(float);
                if (numCoords[textureNo] != D3DFVF_TEXTUREFORMAT3) {
                    strided->u.s.texCoords[textureNo].dwType = D3DDECLTYPE_FLOAT4;
                    data += sizeof(float);
                }
            }
        }
        coordIdxInfo = coordIdxInfo >> 2; /* Drop bottom two bits */
    }
}

void primitiveConvertToStridedData(IWineD3DDevice *iface, WineDirect3DVertexStridedData *strided, LONG BaseVertexIndex, BOOL *fixup) {

    short         LoopThroughTo = 0;
    short         nStream;
    GLint         streamVBO = 0;

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* OK, Now to setup the data locations
       For the non-created vertex shaders, the VertexShader var holds the real
          FVF and only stream 0 matters
       For the created vertex shaders, there is an FVF per stream              */
    if (!This->stateBlock->streamIsUP && !(This->stateBlock->vertexShader == NULL)) {
        LoopThroughTo = MAX_STREAMS;
    } else {
        LoopThroughTo = 1;
    }

    /* Work through stream by stream */
    for (nStream=0; nStream<LoopThroughTo; ++nStream) {
        DWORD  stride  = This->stateBlock->streamStride[nStream];
        BYTE  *data    = NULL;
        DWORD  thisFVF = 0;

        /* Skip empty streams */
        if (This->stateBlock->streamSource[nStream] == NULL) continue;

        /* Retrieve appropriate FVF */
        if (LoopThroughTo == 1) { /* Use FVF, not vertex shader */
            thisFVF = This->stateBlock->fvf;
            /* Handle memory passed directly as well as vertex buffers */
            if (This->stateBlock->streamIsUP) {
                streamVBO = 0;
                data    = (BYTE *)This->stateBlock->streamSource[nStream];
            } else {
                IWineD3DVertexBuffer_PreLoad(This->stateBlock->streamSource[nStream]);
                /* GetMemory binds the VBO */
                data = IWineD3DVertexBufferImpl_GetMemory(This->stateBlock->streamSource[nStream], 0, &streamVBO);
                if(fixup) {
                    if(streamVBO != 0 ) *fixup = TRUE;
                }
            }
        } else {
#if 0 /* TODO: Vertex shader support */
            thisFVF = This->stateBlock->vertexShaderDecl->fvf[nStream];
            data    = IWineD3DVertexBufferImpl_GetMemory(This->stateBlock->streamSource[nStream], 0);
#endif
        }
        VTRACE(("FVF for stream %d is %lx\n", nStream, thisFVF));
        if (thisFVF == 0) continue;

        /* Now convert the stream into pointers */

        /* Shuffle to the beginning of the vertexes to render and index from there */
        data = data + (BaseVertexIndex * stride);

        primitiveConvertFVFtoOffset(thisFVF, stride, data, strided, streamVBO);
    }
}

#if 0 /* TODO: Software Shaders */
/* Draw a single vertex using this information */
static void draw_vertex(IWineD3DDevice *iface,                         /* interface    */
                 BOOL isXYZ,    float x, float y, float z, float rhw,  /* xyzn position*/
                 BOOL isNormal, float nx, float ny, float nz,          /* normal       */
                 BOOL isDiffuse, float *dRGBA,                         /* 1st   colors */
                 BOOL isSpecular, float *sRGB,                         /* 2ndry colors */
                 BOOL isPtSize, float ptSize,                       /* pointSize    */
                 WINED3DVECTOR_4 *texcoords, int *numcoords)        /* texture info */
{
    unsigned int textureNo;
    float s, t, r, q;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Diffuse -------------------------------- */
    if (isDiffuse) {
        glColor4fv(dRGBA);
        VTRACE(("glColor4f: r,g,b,a=%f,%f,%f,%f\n", dRGBA[0], dRGBA[1], dRGBA[2], dRGBA[3]));
    }

    /* Specular Colour ------------------------------------------*/
    if (isSpecular) {
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
          GL_EXTCALL(glSecondaryColor3fvEXT(sRGB));
          VTRACE(("glSecondaryColor4f: r,g,b=%f,%f,%f\n", sRGB[0], sRGB[1], sRGB[2]));
        } else {
	  VTRACE(("Specular color extensions not supplied\n"));
	}
    }

    /* Normal -------------------------------- */
    if (isNormal) {
        VTRACE(("glNormal:nx,ny,nz=%f,%f,%f\n", nx,ny,nz));
        glNormal3f(nx, ny, nz);
    }

    /* Point Size ----------------------------------------------*/
    if (isPtSize) {

        /* no such functionality in the fixed function GL pipeline */
        FIXME("Cannot change ptSize here in openGl\n");
    }

    /* Texture coords --------------------------- */
    for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {

        if (!GL_SUPPORT(ARB_MULTITEXTURE) && textureNo > 0) {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            continue ;
        }

        /* Query tex coords */
        if (This->stateBlock->textures[textureNo] != NULL) {

            int    coordIdx = This->stateBlock->textureState[textureNo][D3DTSS_TEXCOORDINDEX];
            if (coordIdx >= MAX_TEXTURES) {
                VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
                continue;
            } else if (numcoords[coordIdx] == 0) {
                TRACE("tex: %d - Skipping tex coords, as no data supplied or no coords supplied\n", textureNo);
                continue;
            } else {

                /* Initialize vars */
                s = 0.0f;
                t = 0.0f;
                r = 0.0f;
                q = 0.0f;

                switch (numcoords[coordIdx]) {
                case 4: q = texcoords[coordIdx].w; /* drop through */
                case 3: r = texcoords[coordIdx].z; /* drop through */
                case 2: t = texcoords[coordIdx].y; /* drop through */
                case 1: s = texcoords[coordIdx].x;
                }

                switch (numcoords[coordIdx]) {   /* Supply the provided texture coords */
                case D3DTTFF_COUNT1:
                    VTRACE(("tex:%d, s=%f\n", textureNo, s));
                    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                        GLMULTITEXCOORD1F(textureNo, s);
                    } else {
                        glTexCoord1f(s);
                    }
                    break;
                case D3DTTFF_COUNT2:
                    VTRACE(("tex:%d, s=%f, t=%f\n", textureNo, s, t));
                    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                        GLMULTITEXCOORD2F(textureNo, s, t);
                    } else {
                        glTexCoord2f(s, t);
                    }
                    break;
                case D3DTTFF_COUNT3:
                    VTRACE(("tex:%d, s=%f, t=%f, r=%f\n", textureNo, s, t, r));
                    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                        GLMULTITEXCOORD3F(textureNo, s, t, r);
                    } else {
                        glTexCoord3f(s, t, r);
                    }
                    break;
                case D3DTTFF_COUNT4:
                    VTRACE(("tex:%d, s=%f, t=%f, r=%f, q=%f\n", textureNo, s, t, r, q));
                    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                        GLMULTITEXCOORD4F(textureNo, s, t, r, q);
                    } else {
                        glTexCoord4f(s, t, r, q);
                    }
                    break;
                default:
                    FIXME("Should not get here as numCoords should be 0->4 (%x)!\n", numcoords[coordIdx]);
                }
            }
        }
    } /* End of textures */

    /* Position -------------------------------- */
    if (isXYZ) {
        if (1.0f == rhw || rhw < 0.00001f) {
            VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f\n", x,y,z));
            glVertex3f(x, y, z);
        } else {
            /* Cannot optimize by dividing through by rhw as rhw is required
               later for perspective in the GL pipeline for vertex shaders   */
            VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f / rhw=%f\n", x,y,z,rhw));
            glVertex4f(x,y,z,rhw);
        }
    }
}
#endif /* TODO: Software shaders */

/* This should match any arrays loaded in loadNumberedArrays. */
/* TODO: Only load / unload arrays if we have to. */
static void unloadNumberedArrays(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* disable any attribs (this is the same for both GLSL and ARB modes) */
    GLint maxAttribs;
    int i;

    /* Leave all the attribs disabled */
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &maxAttribs);
    /* MESA does not support it right not */
    if (glGetError() != GL_NO_ERROR)
        maxAttribs = 16;
    for (i = 0; i < maxAttribs; ++i) {
        GL_EXTCALL(glDisableVertexAttribArrayARB(i));
        checkGLcall("glDisableVertexAttribArrayARB(reg);");
    }
}

/* TODO: Only load / unload arrays if we have to. */
static void loadNumberedArrays(
    IWineD3DDevice *iface,
    IWineD3DVertexShader *shader,
    WineDirect3DVertexStridedData *strided) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    GLint curVBO = -1;
    int i;

    for (i = 0; i < MAX_ATTRIBS; i++) {

        if (!strided->u.input[i].lpData && !strided->u.input[i].VBO)
            continue;

        TRACE_(d3d_shader)("Loading array %u [VBO=%u]\n", i, strided->u.input[i].VBO);

        if(curVBO != strided->u.input[i].VBO) {
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, strided->u.input[i].VBO));
            checkGLcall("glBindBufferARB");
            curVBO = strided->u.input[i].VBO;
        }
        GL_EXTCALL(glVertexAttribPointerARB(i,
                        WINED3D_ATR_SIZE(strided->u.input[i].dwType),
                        WINED3D_ATR_GLTYPE(strided->u.input[i].dwType),
                        WINED3D_ATR_NORMALIZED(strided->u.input[i].dwType),
                        strided->u.input[i].dwStride,
                        strided->u.input[i].lpData));
        GL_EXTCALL(glEnableVertexAttribArrayARB(i));
   }
}

/* This should match any arrays loaded in loadVertexData. */
/* TODO: Only load / unload arrays if we have to. */
static void unloadVertexData(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int texture_idx;

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
        glDisableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
    }
    for (texture_idx = 0; texture_idx < GL_LIMITS(textures); ++texture_idx) {
        GL_EXTCALL(glClientActiveTextureARB(GL_TEXTURE0_ARB + texture_idx));
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
}

/* TODO: Only load / unload arrays if we have to. */
static void loadVertexData(IWineD3DDevice *iface, WineDirect3DVertexStridedData *sd) {
    unsigned int textureNo   = 0;
    unsigned int texture_idx = 0;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    GLint curVBO = -1;

    TRACE("Using fast vertex array code\n");
    /* Blend Data ---------------------------------------------- */
    if( (sd->u.s.blendWeights.lpData) || (sd->u.s.blendWeights.VBO) ||
        (sd->u.s.blendMatrixIndices.lpData) || (sd->u.s.blendMatrixIndices.VBO) ) {


        if (GL_SUPPORT(ARB_VERTEX_BLEND)) {

#if 1
            glEnableClientState(GL_WEIGHT_ARRAY_ARB);
            checkGLcall("glEnableClientState(GL_WEIGHT_ARRAY_ARB)");
#endif

            TRACE("Blend %d %p %ld\n", WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType),
                sd->u.s.blendWeights.lpData, sd->u.s.blendWeights.dwStride);
            /* FIXME("TODO\n");*/
            /* Note dwType == float3 or float4 == 2 or 3 */

#if 0
            /* with this on, the normals appear to be being modified,
               but the vertices aren't being translated as they should be
               Maybe the world matrix aren't being setup properly? */
            glVertexBlendARB(WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType) + 1);
#endif


            VTRACE(("glWeightPointerARB(%d, GL_FLOAT, %ld, %p)\n",
                WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType) ,
                sd->u.s.blendWeights.dwStride,
                sd->u.s.blendWeights.lpData));

            if(curVBO != sd->u.s.blendWeights.VBO) {
                GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.blendWeights.VBO));
                checkGLcall("glBindBufferARB");
                curVBO = sd->u.s.blendWeights.VBO;
            }

            GL_EXTCALL(glWeightPointerARB)(
                WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType),
                WINED3D_ATR_GLTYPE(sd->u.s.blendWeights.dwType),
                sd->u.s.blendWeights.dwStride,
                sd->u.s.blendWeights.lpData);

            checkGLcall("glWeightPointerARB");

            if((sd->u.s.blendMatrixIndices.lpData) || (sd->u.s.blendMatrixIndices.VBO)){
                static BOOL showfixme = TRUE;
                if(showfixme){
                    FIXME("blendMatrixIndices support\n");
                    showfixme = FALSE;
                }
            }

        } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
            /* FIXME("TODO\n");*/
#if 0

            GL_EXTCALL(glVertexWeightPointerEXT)(
                WINED3D_ATR_SIZE(sd->u.s.blendWeights.dwType),
                WINED3D_ATR_GLTYPE(sd->u.s.blendWeights.dwType),
                sd->u.s.blendWeights.dwStride,
                sd->u.s.blendWeights.lpData);
            checkGLcall("glVertexWeightPointerEXT(numBlends, ...)");
            glEnableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT);
            checkGLcall("glEnableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT)");
#endif

        } else {
            /* TODO: support blends in fixupVertices */
            FIXME("unsupported blending in openGl\n");
        }
    } else {
        if (GL_SUPPORT(ARB_VERTEX_BLEND)) {
#if 0    /* TODO: Vertex blending */
            glDisable(GL_VERTEX_BLEND_ARB);
#endif
            TRACE("ARB_VERTEX_BLEND\n");
        } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
            TRACE(" EXT_VERTEX_WEIGHTING\n");
            glDisableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT);
            checkGLcall("glDisableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT)");

        }
    }

#if 0 /* FOG  ----------------------------------------------*/
    if (sd->u.s.fog.lpData || sd->u.s.fog.VBO) {
        /* TODO: fog*/
    if (GL_SUPPORT(EXT_FOG_COORD) {
             glEnableClientState(GL_FOG_COORDINATE_EXT);
            (GL_EXTCALL)(FogCoordPointerEXT)(
                WINED3D_ATR_GLTYPE(sd->u.s.fog.dwType),
                sd->u.s.fog.dwStride,
                sd->u.s.fog.lpData);
        } else {
            /* don't bother falling back to 'slow' as we don't support software FOG yet. */
            /* FIXME: fixme once */
            TRACE("Hardware support for FOG is not avaiable, FOG disabled.\n");
        }
    } else {
        if (GL_SUPPRT(EXT_FOR_COORD) {
             /* make sure fog is disabled */
             glDisableClientState(GL_FOG_COORDINATE_EXT);
        }
    }
#endif

#if 0 /* tangents  ----------------------------------------------*/
    if (sd->u.s.tangent.lpData || sd->u.s.tangent.VBO ||
        sd->u.s.binormal.lpData || sd->u.s.binormal.VBO) {
        /* TODO: tangents*/
        if (GL_SUPPORT(EXT_COORDINATE_FRAME) {
            if (sd->u.s.tangent.lpData || sd->u.s.tangent.VBO) {
                glEnable(GL_TANGENT_ARRAY_EXT);
                (GL_EXTCALL)(TangentPointerEXT)(
                    WINED3D_ATR_GLTYPE(sd->u.s.tangent.dwType),
                    sd->u.s.tangent.dwStride,
                    sd->u.s.tangent.lpData);
            } else {
                    glDisable(GL_TANGENT_ARRAY_EXT);
            }
            if (sd->u.s.binormal.lpData || sd->u.s.binormal.VBO) {
                    glEnable(GL_BINORMAL_ARRAY_EXT);
                    (GL_EXTCALL)(BinormalPointerEXT)(
                        WINED3D_ATR_GLTYPE(sd->u.s.binormal.dwType),
                        sd->u.s.binormal.dwStride,
                        sd->u.s.binormal.lpData);
            } else{
                    glDisable(GL_BINORMAL_ARRAY_EXT);
            }

        } else {
            /* don't bother falling back to 'slow' as we don't support software tangents and binormals yet . */
            /* FIXME: fixme once */
            TRACE("Hardware support for tangents and binormals is not avaiable, tangents and binormals disabled.\n");
        }
    } else {
        if (GL_SUPPORT(EXT_COORDINATE_FRAME) {
             /* make sure fog is disabled */
             glDisable(GL_TANGENT_ARRAY_EXT);
             glDisable(GL_BINORMAL_ARRAY_EXT);
        }
    }
#endif

    /* Point Size ----------------------------------------------*/
    if (sd->u.s.pSize.lpData || sd->u.s.pSize.VBO) {

        /* no such functionality in the fixed function GL pipeline */
        TRACE("Cannot change ptSize here in openGl\n");
        /* TODO: Implement this function in using shaders if they are available */

    }

    /* Vertex Pointers -----------------------------------------*/
    if (sd->u.s.position.lpData != NULL || sd->u.s.position.VBO != 0) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glVertexPointer(%ld, GL_FLOAT, %ld, %p)\n",
                sd->u.s.position.dwStride,
                sd->u.s.position.dwType + 1,
                sd->u.s.position.lpData));

        if(curVBO != sd->u.s.position.VBO) {
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.position.VBO));
            checkGLcall("glBindBufferARB");
            curVBO = sd->u.s.position.VBO;
        }

        /* min(WINED3D_ATR_SIZE(position),3) to Disable RHW mode as 'w' coord
           handling for rhw mode should not impact screen position whereas in GL it does.
           This may  result in very slightly distored textures in rhw mode, but
           a very minimal different. There's always the other option of
           fixing the view matrix to prevent w from having any effect

           This only applies to user pointer sources, in VBOs the vertices are fixed up
         */
        if(sd->u.s.position.VBO == 0) {
            glVertexPointer(3 /* min(WINED3D_ATR_SIZE(sd->u.s.position.dwType),3) */,
                WINED3D_ATR_GLTYPE(sd->u.s.position.dwType),
                sd->u.s.position.dwStride, sd->u.s.position.lpData);
        } else {
            glVertexPointer(
                WINED3D_ATR_SIZE(sd->u.s.position.dwType),
                WINED3D_ATR_GLTYPE(sd->u.s.position.dwType),
                sd->u.s.position.dwStride, sd->u.s.position.lpData);
        }
        checkGLcall("glVertexPointer(...)");
        glEnableClientState(GL_VERTEX_ARRAY);
        checkGLcall("glEnableClientState(GL_VERTEX_ARRAY)");

    } else {
        glDisableClientState(GL_VERTEX_ARRAY);
        checkGLcall("glDisableClientState(GL_VERTEX_ARRAY)");
    }

    /* Normals -------------------------------------------------*/
    if (sd->u.s.normal.lpData || sd->u.s.normal.VBO) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glNormalPointer(GL_FLOAT, %ld, %p)\n",
                sd->u.s.normal.dwStride,
                sd->u.s.normal.lpData));
        if(curVBO != sd->u.s.normal.VBO) {
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.normal.VBO));
            checkGLcall("glBindBufferARB");
            curVBO = sd->u.s.normal.VBO;
        }
        glNormalPointer(
            WINED3D_ATR_GLTYPE(sd->u.s.normal.dwType),
            sd->u.s.normal.dwStride,
            sd->u.s.normal.lpData);
        checkGLcall("glNormalPointer(...)");
        glEnableClientState(GL_NORMAL_ARRAY);
        checkGLcall("glEnableClientState(GL_NORMAL_ARRAY)");

    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
        checkGLcall("glDisableClientState(GL_NORMAL_ARRAY)");
        glNormal3f(0, 0, 1);
        checkGLcall("glNormal3f(0, 0, 1)");
    }

    /* Diffuse Colour --------------------------------------------*/
    /*  WARNING: Data here MUST be in RGBA format, so cannot      */
    /*     go directly into fast mode from app pgm, because       */
    /*     directx requires data in BGRA format.                  */
    /* currently fixupVertices swizels the format, but this isn't */
    /* very practical when using VBOS                             */
    /* NOTE: Unless we write a vertex shader to swizel the colour */
    /* , or the user doesn't care and wants the speed advantage   */

    if (sd->u.s.diffuse.lpData || sd->u.s.diffuse.VBO) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glColorPointer(4, GL_UNSIGNED_BYTE, %ld, %p)\n",
                sd->u.s.diffuse.dwStride,
                sd->u.s.diffuse.lpData));

        if(curVBO != sd->u.s.diffuse.VBO) {
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.diffuse.VBO));
            checkGLcall("glBindBufferARB");
            curVBO = sd->u.s.diffuse.VBO;
        }
        glColorPointer(4, GL_UNSIGNED_BYTE,
                       sd->u.s.diffuse.dwStride,
                       sd->u.s.diffuse.lpData);
        checkGLcall("glColorPointer(4, GL_UNSIGNED_BYTE, ...)");
        glEnableClientState(GL_COLOR_ARRAY);
        checkGLcall("glEnableClientState(GL_COLOR_ARRAY)");

    } else {
        glDisableClientState(GL_COLOR_ARRAY);
        checkGLcall("glDisableClientState(GL_COLOR_ARRAY)");
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        checkGLcall("glColor4f(1, 1, 1, 1)");
    }

    /* Specular Colour ------------------------------------------*/
    if (sd->u.s.specular.lpData || sd->u.s.specular.VBO) {
        TRACE("setting specular colour\n");
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glSecondaryColorPointer(4, GL_UNSIGNED_BYTE, %ld, %p)\n",
                sd->u.s.specular.dwStride,
                sd->u.s.specular.lpData));
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
            if(curVBO != sd->u.s.specular.VBO) {
                GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.specular.VBO));
                checkGLcall("glBindBufferARB");
                curVBO = sd->u.s.specular.VBO;
            }
            GL_EXTCALL(glSecondaryColorPointerEXT)(4, GL_UNSIGNED_BYTE,
                                                   sd->u.s.specular.dwStride,
                                                   sd->u.s.specular.lpData);
            vcheckGLcall("glSecondaryColorPointerEXT(4, GL_UNSIGNED_BYTE, ...)");
            glEnableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
            vcheckGLcall("glEnableClientState(GL_SECONDARY_COLOR_ARRAY_EXT)");
        } else {

        /* Missing specular color is not critical, no warnings */
        VTRACE(("Specular colour is not supported in this GL implementation\n"));
        }

    } else {
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {

            glDisableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
            checkGLcall("glDisableClientState(GL_SECONDARY_COLOR_ARRAY_EXT)");
            GL_EXTCALL(glSecondaryColor3fEXT)(0, 0, 0);
            checkGLcall("glSecondaryColor3fEXT(0, 0, 0)");
        } else {

            /* Missing specular color is not critical, no warnings */
            VTRACE(("Specular colour is not supported in this GL implementation\n"));
        }
    }

    /* Texture coords -------------------------------------------*/

    for (textureNo = 0, texture_idx = 0; textureNo < GL_LIMITS(texture_stages); ++textureNo) {
        /* The code below uses glClientActiveTexture and glMultiTexCoord* which are all part of the GL_ARB_multitexture extension. */
        /* Abort if we don't support the extension. */
        if (!GL_SUPPORT(ARB_MULTITEXTURE)) {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            continue;
        }

        if (!GL_SUPPORT(NV_REGISTER_COMBINERS) || This->stateBlock->textures[textureNo]) {
            /* Select the correct texture stage */
            GL_EXTCALL(glClientActiveTextureARB(GL_TEXTURE0_ARB + texture_idx));
        }

        if (This->stateBlock->textures[textureNo] != NULL) {
            int coordIdx = This->stateBlock->textureState[textureNo][D3DTSS_TEXCOORDINDEX];

            if (coordIdx >= MAX_TEXTURES) {
                VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + texture_idx, 0, 0, 0, 1));

            } else if (sd->u.s.texCoords[coordIdx].lpData == NULL && sd->u.s.texCoords[coordIdx].VBO == 0) {
                VTRACE(("Bound texture but no texture coordinates supplied, so skipping\n"));
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + texture_idx, 0, 0, 0, 1));

            } else {
                TRACE("Setting up texture %u, idx %d, cordindx %u, data %p\n",
                      textureNo, texture_idx, coordIdx, sd->u.s.texCoords[coordIdx].lpData);
                if(curVBO != sd->u.s.texCoords[coordIdx].VBO) {
                    GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, sd->u.s.texCoords[coordIdx].VBO));
                    checkGLcall("glBindBufferARB");
                    curVBO = sd->u.s.texCoords[coordIdx].VBO;
                }
                /* The coords to supply depend completely on the fvf / vertex shader */
                glTexCoordPointer(
                    WINED3D_ATR_SIZE(sd->u.s.texCoords[coordIdx].dwType),
                    WINED3D_ATR_GLTYPE(sd->u.s.texCoords[coordIdx].dwType),
                    sd->u.s.texCoords[coordIdx].dwStride,
                    sd->u.s.texCoords[coordIdx].lpData);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            }
        } else if (!GL_SUPPORT(NV_REGISTER_COMBINERS)) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + textureNo, 0, 0, 0, 1));
        }
        if (!GL_SUPPORT(NV_REGISTER_COMBINERS) || This->stateBlock->textures[textureNo]) ++texture_idx;
    }
    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        for (textureNo = texture_idx; textureNo < GL_LIMITS(textures); ++textureNo) {
            GL_EXTCALL(glClientActiveTextureARB(GL_TEXTURE0_ARB + textureNo));
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + textureNo, 0, 0, 0, 1));
        }
    }
}

static void drawStridedFast(IWineD3DDevice *iface,UINT numberOfVertices, GLenum glPrimitiveType,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (idxData != NULL /* This crashes sometimes!*/) {
        TRACE("(%p) : glElements(%x, %d, %ld, ...)\n", This, glPrimitiveType, numberOfVertices, minIndex);
        idxData = idxData == (void *)-1 ? NULL : idxData;
#if 1
#if 0
        glIndexPointer(idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idxSize, startIdx);
        glEnableClientState(GL_INDEX_ARRAY);
#endif
        glDrawElements(glPrimitiveType, numberOfVertices, idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                     (const char *)idxData+(idxSize * startIdx));
#else /* using drawRangeElements may be faster */

        glDrawRangeElements(glPrimitiveType, minIndex, minIndex + numberOfVertices - 1, numberOfVertices,
                      idxSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                      (const char *)idxData+(idxSize * startIdx));
#endif
        checkGLcall("glDrawRangeElements");

    } else {

        /* Note first is now zero as we shuffled along earlier */
        TRACE("(%p) : glDrawArrays(%x, 0, %d)\n", This, glPrimitiveType, numberOfVertices);
        glDrawArrays(glPrimitiveType, 0, numberOfVertices);
        checkGLcall("glDrawArrays");

    }

    return;
}

/*
 * Actually draw using the supplied information.
 * Slower GL version which extracts info about each vertex in turn
 */
	
static void drawStridedSlow(IWineD3DDevice *iface, WineDirect3DVertexStridedData *sd,
                     UINT NumVertexes, GLenum glPrimType,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx) {

    unsigned int               textureNo    = 0;
    unsigned int               texture_idx  = 0;
    const short               *pIdxBufS     = NULL;
    const long                *pIdxBufL     = NULL;
    LONG                       SkipnStrides = 0;
    LONG                       vx_index;
    float x  = 0.0f, y  = 0.0f, z = 0.0f;  /* x,y,z coordinates          */
    float nx = 0.0f, ny = 0.0, nz = 0.0f;  /* normal x,y,z coordinates   */
    float rhw = 0.0f;                      /* rhw                        */
    float ptSize = 0.0f;                   /* Point size                 */
    DWORD diffuseColor = 0xFFFFFFFF;       /* Diffuse Color              */
    DWORD specularColor = 0;               /* Specular Color             */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("Using slow vertex array code\n");

    /* Variable Initialization */
    if (idxData != NULL) {
        if (idxSize == 2) pIdxBufS = (const short *) idxData;
        else pIdxBufL = (const long *) idxData;
    }

    /* Start drawing in GL */
    VTRACE(("glBegin(%x)\n", glPrimType));
    glBegin(glPrimType);

    /* We shouldn't start this function if any VBO is involved. Should I put a safety check here?
     * Guess it's not necessary(we crash then anyway) and would only eat CPU time
     */

    /* For each primitive */
    for (vx_index = 0; vx_index < NumVertexes; ++vx_index) {

        /* Initialize diffuse color */
        diffuseColor = 0xFFFFFFFF;

        /* For indexed data, we need to go a few more strides in */
        if (idxData != NULL) {

            /* Indexed so work out the number of strides to skip */
            if (idxSize == 2) {
                VTRACE(("Idx for vertex %ld = %d\n", vx_index, pIdxBufS[startIdx+vx_index]));
                SkipnStrides = pIdxBufS[startIdx + vx_index];
            } else {
                VTRACE(("Idx for vertex %ld = %ld\n", vx_index, pIdxBufL[startIdx+vx_index]));
                SkipnStrides = pIdxBufL[startIdx + vx_index];
            }
        }

        /* Position Information ------------------ */
        if (sd->u.s.position.lpData != NULL) {

            float *ptrToCoords = (float *)(sd->u.s.position.lpData + (SkipnStrides * sd->u.s.position.dwStride));
            x = ptrToCoords[0];
            y = ptrToCoords[1];
            z = ptrToCoords[2];
            rhw = 1.0;
            VTRACE(("x,y,z=%f,%f,%f\n", x,y,z));

            /* RHW follows, only if transformed, ie 4 floats were provided */
            if (sd->u.s.position_transformed) {
                rhw = ptrToCoords[3];
                VTRACE(("rhw=%f\n", rhw));
            }
        }

        /* Blending data -------------------------- */
        if (sd->u.s.blendWeights.lpData != NULL) {
            /* float *ptrToCoords = (float *)(sd->u.s.blendWeights.lpData + (SkipnStrides * sd->u.s.blendWeights.dwStride)); */
            FIXME("Blending not supported yet\n");

            if (sd->u.s.blendMatrixIndices.lpData != NULL) {
                /*DWORD *ptrToCoords = (DWORD *)(sd->u.s.blendMatrixIndices.lpData + (SkipnStrides * sd->u.s.blendMatrixIndices.dwStride));*/
            }
        }

        /* Vertex Normal Data (untransformed only)- */
        if (sd->u.s.normal.lpData != NULL) {

            float *ptrToCoords = (float *)(sd->u.s.normal.lpData + (SkipnStrides * sd->u.s.normal.dwStride));
            nx = ptrToCoords[0];
            ny = ptrToCoords[1];
            nz = ptrToCoords[2];
            VTRACE(("nx,ny,nz=%f,%f,%f\n", nx, ny, nz));
        }

        /* Point Size ----------------------------- */
        if (sd->u.s.pSize.lpData != NULL) {

            float *ptrToCoords = (float *)(sd->u.s.pSize.lpData + (SkipnStrides * sd->u.s.pSize.dwStride));
            ptSize = ptrToCoords[0];
            VTRACE(("ptSize=%f\n", ptSize));
            FIXME("No support for ptSize yet\n");
        }

        /* Diffuse -------------------------------- */
        if (sd->u.s.diffuse.lpData != NULL) {

            DWORD *ptrToCoords = (DWORD *)(sd->u.s.diffuse.lpData + (SkipnStrides * sd->u.s.diffuse.dwStride));
            diffuseColor = ptrToCoords[0];
            VTRACE(("diffuseColor=%lx\n", diffuseColor));
        }

        /* Specular  -------------------------------- */
        if (sd->u.s.specular.lpData != NULL) {

            DWORD *ptrToCoords = (DWORD *)(sd->u.s.specular.lpData + (SkipnStrides * sd->u.s.specular.dwStride));
            specularColor = ptrToCoords[0];
            VTRACE(("specularColor=%lx\n", specularColor));
        }

        /* Texture coords --------------------------- */
        for (textureNo = 0, texture_idx = 0; textureNo < GL_LIMITS(texture_stages); ++textureNo) {

            if (!GL_SUPPORT(ARB_MULTITEXTURE) && textureNo > 0) {
                FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
                continue ;
            }

            /* Query tex coords */
            if (This->stateBlock->textures[textureNo] != NULL) {

                int    coordIdx = This->stateBlock->textureState[textureNo][D3DTSS_TEXCOORDINDEX];
                float *ptrToCoords = NULL;
                float  s = 0.0, t = 0.0, r = 0.0, q = 0.0;

                if (coordIdx > 7) {
                    VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
                    ++texture_idx;
                    continue;
                } else if (coordIdx < 0) {
                    FIXME("tex: %d - Coord index %d is less than zero, expect a crash.\n", textureNo, coordIdx);
                    ++texture_idx;
                    continue;
                }

                ptrToCoords = (float *)(sd->u.s.texCoords[coordIdx].lpData + (SkipnStrides * sd->u.s.texCoords[coordIdx].dwStride));
                if (sd->u.s.texCoords[coordIdx].lpData == NULL) {
                    TRACE("tex: %d - Skipping tex coords, as no data supplied\n", textureNo);
                    ++texture_idx;
                    continue;
                } else {

                    int coordsToUse = sd->u.s.texCoords[coordIdx].dwType + 1; /* 0 == D3DDECLTYPE_FLOAT1 etc */

                    /* The coords to supply depend completely on the fvf / vertex shader */
                    switch (coordsToUse) {
                    case 4: q = ptrToCoords[3]; /* drop through */
                    case 3: r = ptrToCoords[2]; /* drop through */
                    case 2: t = ptrToCoords[1]; /* drop through */
                    case 1: s = ptrToCoords[0];
                    }

                    /* Projected is more 'fun' - Move the last coord to the 'q'
                          parameter (see comments under D3DTSS_TEXTURETRANSFORMFLAGS */
                    if ((This->stateBlock->textureState[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE) &&
                        (This->stateBlock->textureState[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & D3DTTFF_PROJECTED)) {

                        if (This->stateBlock->textureState[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & D3DTTFF_PROJECTED) {
                            switch (coordsToUse) {
                            case 0:  /* Drop Through */
                            case 1:
                                FIXME("D3DTTFF_PROJECTED but only zero or one coordinate?\n");
                                break;
                            case 2:
                                q = t;
                                t = 0.0;
                                coordsToUse = 4;
                                break;
                            case 3:
                                q = r;
                                r = 0.0;
                                coordsToUse = 4;
                                break;
                            case 4:  /* Nop here */
                                break;
                            default:
                                FIXME("Unexpected D3DTSS_TEXTURETRANSFORMFLAGS value of %ld\n",
                                      This->stateBlock->textureState[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & D3DTTFF_PROJECTED);
                            }
                        }
                    }

                    switch (coordsToUse) {   /* Supply the provided texture coords */
                    case D3DTTFF_COUNT1:
                        VTRACE(("tex:%d, s=%f\n", textureNo, s));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord1fARB(texture_idx, s));
                        } else {
                            glTexCoord1f(s);
                        }
                        break;
                    case D3DTTFF_COUNT2:
                        VTRACE(("tex:%d, s=%f, t=%f\n", textureNo, s, t));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord2fARB(texture_idx, s, t));
                        } else {
                            glTexCoord2f(s, t);
                        }
                        break;
                    case D3DTTFF_COUNT3:
                        VTRACE(("tex:%d, s=%f, t=%f, r=%f\n", textureNo, s, t, r));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord3fARB(texture_idx, s, t, r));
                        } else {
                            glTexCoord3f(s, t, r);
                        }
                        break;
                    case D3DTTFF_COUNT4:
                        VTRACE(("tex:%d, s=%f, t=%f, r=%f, q=%f\n", textureNo, s, t, r, q));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord4fARB(texture_idx, s, t, r, q));
                        } else {
                            glTexCoord4f(s, t, r, q);
                        }
                        break;
                    default:
                        FIXME("Should not get here as coordsToUse is two bits only (%x)!\n", coordsToUse);
                    }
                }
            }
            if (!GL_SUPPORT(NV_REGISTER_COMBINERS) || This->stateBlock->textures[textureNo]) ++texture_idx;
        } /* End of textures */

        /* Diffuse -------------------------------- */
        if (sd->u.s.diffuse.lpData != NULL) {
	  glColor4ub(D3DCOLOR_B_R(diffuseColor),
		     D3DCOLOR_B_G(diffuseColor),
		     D3DCOLOR_B_B(diffuseColor),
		     D3DCOLOR_B_A(diffuseColor));
            VTRACE(("glColor4ub: r,g,b,a=%lu,%lu,%lu,%lu\n", 
                    D3DCOLOR_B_R(diffuseColor),
		    D3DCOLOR_B_G(diffuseColor),
		    D3DCOLOR_B_B(diffuseColor),
		    D3DCOLOR_B_A(diffuseColor)));
        } else {
            if (vx_index == 0) glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }

        /* Specular ------------------------------- */
        if (sd->u.s.specular.lpData != NULL) {
            /* special case where the fog density is stored in the diffuse alpha channel */
            if(This->stateBlock->renderState[WINED3DRS_FOGENABLE] &&
              (This->stateBlock->renderState[WINED3DRS_FOGVERTEXMODE] == D3DFOG_NONE || sd->u.s.position.dwType == D3DDECLTYPE_FLOAT4 )&&
              This->stateBlock->renderState[WINED3DRS_FOGTABLEMODE] == D3DFOG_NONE) {
                if(GL_SUPPORT(EXT_FOG_COORD)) {
                    GL_EXTCALL(glFogCoordfEXT(specularColor >> 24));
                } else {
                    static BOOL warned = FALSE;
                    if(!warned) {
                        /* TODO: Use the fog table code from old ddraw */
                        FIXME("Implement fog for transformed vertices in software\n");
                        warned = TRUE;
                    }
                }
            }

            VTRACE(("glSecondaryColor4ub: r,g,b=%lu,%lu,%lu\n", 
                    D3DCOLOR_B_R(specularColor), 
                    D3DCOLOR_B_G(specularColor), 
                    D3DCOLOR_B_B(specularColor)));
            if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
                GL_EXTCALL(glSecondaryColor3ubEXT)(
                           D3DCOLOR_B_R(specularColor),
                           D3DCOLOR_B_G(specularColor),
                           D3DCOLOR_B_B(specularColor));
            } else {
                /* Do not worry if specular colour missing and disable request */
                VTRACE(("Specular color extensions not supplied\n"));
            }
        } else {
            if (vx_index == 0) {
                if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
                    GL_EXTCALL(glSecondaryColor3fEXT)(0, 0, 0);
                } else {
                    /* Do not worry if specular colour missing and disable request */
                    VTRACE(("Specular color extensions not supplied\n"));
                }
            }
        }

        /* Normal -------------------------------- */
        if (sd->u.s.normal.lpData != NULL) {
            VTRACE(("glNormal:nx,ny,nz=%f,%f,%f\n", nx,ny,nz));
            glNormal3f(nx, ny, nz);
        } else {
            if (vx_index == 0) glNormal3f(0, 0, 1);
        }

        /* Position -------------------------------- */
        if (sd->u.s.position.lpData != NULL) {
            if (1.0f == rhw || ((rhw < eps) && (rhw > -eps))) {
                VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f\n", x,y,z));
                glVertex3f(x, y, z);
            } else {
                GLfloat w = 1.0 / rhw;
                VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f / rhw=%f\n", x,y,z,rhw));
                glVertex4f(x*w, y*w, z*w, w);
            }
        }

        /* For non indexed mode, step onto next parts */
        if (idxData == NULL) {
            ++SkipnStrides;
        }
    }

    glEnd();
    checkGLcall("glEnd and previous calls");
}

#if 0 /* TODO: Software/Hardware vertex blending support */
/*
 * Draw with emulated vertex shaders
 * Note: strided data is uninitialized, as we need to pass the vertex
 *     shader directly as ordering irs yet
 */
void drawStridedSoftwareVS(IWineD3DDevice *iface, WineDirect3DVertexStridedData *sd,
                     int PrimitiveType, ULONG NumPrimitives,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx) {

    unsigned int               textureNo    = 0;
    GLenum                     glPrimType   = GL_POINTS;
    int                        NumVertexes  = NumPrimitives;
    const short               *pIdxBufS     = NULL;
    const long                *pIdxBufL     = NULL;
    LONG                       SkipnStrides = 0;
    LONG                       vx_index;
    float x  = 0.0f, y  = 0.0f, z = 0.0f;  /* x,y,z coordinates          */
    float rhw = 0.0f;                      /* rhw                        */
    float ptSize = 0.0f;                   /* Point size                 */
    D3DVECTOR_4 texcoords[8];              /* Texture Coords             */
    int   numcoords[8];                    /* Number of coords           */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    IDirect3DVertexShaderImpl* vertexShader = NULL;

    TRACE("Using slow software vertex shader code\n");

    /* Variable Initialization */
    if (idxData != NULL) {
        if (idxSize == 2) pIdxBufS = (const short *) idxData;
        else pIdxBufL = (const long *) idxData;
    }

    /* Ok, Work out which primitive is requested and how many vertexes that will be */
    NumVertexes = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);

    /* Retrieve the VS information */
    vertexShader = (IWineD3DVertexShaderImp *)This->stateBlock->VertexShader;

    /* Start drawing in GL */
    VTRACE(("glBegin(%x)\n", glPrimType));
    glBegin(glPrimType);

    /* For each primitive */
    for (vx_index = 0; vx_index < NumVertexes; ++vx_index) {

        /* For indexed data, we need to go a few more strides in */
        if (idxData != NULL) {

            /* Indexed so work out the number of strides to skip */
            if (idxSize == 2) {
                VTRACE(("Idx for vertex %ld = %d\n", vx_index, pIdxBufS[startIdx+vx_index]));
                SkipnStrides = pIdxBufS[startIdx+vx_index];
            } else {
                VTRACE(("Idx for vertex %ld = %ld\n", vx_index, pIdxBufL[startIdx+vx_index]));
                SkipnStrides = pIdxBufL[startIdx+vx_index];
            }
        }

        /* Fill the vertex shader input */
        IDirect3DDeviceImpl_FillVertexShaderInputSW(This, vertexShader, SkipnStrides);

        /* Initialize the output fields to the same defaults as it would normally have */
        memset(&vertexShader->output, 0, sizeof(VSHADEROUTPUTDATA8));
        vertexShader->output.oD[0].x = 1.0;
        vertexShader->output.oD[0].y = 1.0;
        vertexShader->output.oD[0].z = 1.0;
        vertexShader->output.oD[0].w = 1.0;

        /* Now execute the vertex shader */
        IDirect3DVertexShaderImpl_ExecuteSW(vertexShader, &vertexShader->input, &vertexShader->output);

        /*
        TRACE_VECTOR(vertexShader->output.oPos);
        TRACE_VECTOR(vertexShader->output.oD[0]);
        TRACE_VECTOR(vertexShader->output.oD[1]);
        TRACE_VECTOR(vertexShader->output.oT[0]);
        TRACE_VECTOR(vertexShader->output.oT[1]);
        TRACE_VECTOR(vertexShader->input.V[0]);
        TRACE_VECTOR(vertexShader->data->C[0]);
        TRACE_VECTOR(vertexShader->data->C[1]);
        TRACE_VECTOR(vertexShader->data->C[2]);
        TRACE_VECTOR(vertexShader->data->C[3]);
        TRACE_VECTOR(vertexShader->data->C[4]);
        TRACE_VECTOR(vertexShader->data->C[5]);
        TRACE_VECTOR(vertexShader->data->C[6]);
        TRACE_VECTOR(vertexShader->data->C[7]);
        */

        /* Extract out the output */
        /* FIXME: Fog coords? */
        x = vertexShader->output.oPos.x;
        y = vertexShader->output.oPos.y;
        z = vertexShader->output.oPos.z;
        rhw = vertexShader->output.oPos.w;
        ptSize = vertexShader->output.oPts.x; /* Fixme - Is this right? */

        /** Update textures coords using vertexShader->output.oT[0->7] */
        memset(texcoords, 0x00, sizeof(texcoords));
        memset(numcoords, 0x00, sizeof(numcoords));
        for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {
            if (This->stateBlock->textures[textureNo] != NULL) {
               texcoords[textureNo].x = vertexShader->output.oT[textureNo].x;
               texcoords[textureNo].y = vertexShader->output.oT[textureNo].y;
               texcoords[textureNo].z = vertexShader->output.oT[textureNo].z;
               texcoords[textureNo].w = vertexShader->output.oT[textureNo].w;
               if (This->stateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE) {
                   numcoords[textureNo]    = This->stateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & ~D3DTTFF_PROJECTED;
               } else {
                   switch (IDirect3DBaseTexture8Impl_GetType((LPDIRECT3DBASETEXTURE8) This->stateBlock->textures[textureNo])) {
                   case WINED3DRTYPE_TEXTURE:       numcoords[textureNo] = 2; break;
                   case WINED3DRTYPE_VOLUMETEXTURE: numcoords[textureNo] = 3; break;
                   default:                         numcoords[textureNo] = 4;
                   }
               }
            } else {
                numcoords[textureNo] = 0;
            }
        }

        /* Draw using this information */
        draw_vertex(iface,
                    TRUE, x, y, z, rhw,
                    TRUE, 0.0f, 0.0f, 1.0f,
                    TRUE, (float*) &vertexShader->output.oD[0],
                    TRUE, (float*) &vertexShader->output.oD[1],
                    FALSE, ptSize,         /* FIXME: Change back when supported */
                    texcoords, numcoords);

        /* For non indexed mode, step onto next parts */
        if (idxData == NULL) {
           ++SkipnStrides;
        }

    } /* for each vertex */

    glEnd();
    checkGLcall("glEnd and previous calls");
}

#endif

inline static void drawPrimitiveDrawStrided(
    IWineD3DDevice *iface,
    BOOL useVertexShaderFunction,
    BOOL usePixelShaderFunction,
    WineDirect3DVertexStridedData *dataLocations,
    UINT numberOfvertices,
    UINT numberOfIndicies,
    GLenum glPrimType,
    const void *idxData,
    short idxSize,
    int minIndex,
    long StartIdx,
    BOOL fixup) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    BOOL useDrawStridedSlow;

    int startStride = idxData == NULL ? 0 : 
                      idxData == (void *) -1 ? 0 :
                      (idxSize == 2 ? *(((const short *) idxData) + StartIdx) : *((const int *) idxData) + StartIdx);
    int endStride = startStride;
    TRACE("begin Start stride %d, end stride %d, number of indices%d, number of vertices%d\n",
        startStride, endStride, numberOfIndicies, numberOfvertices);

/* Generate some fixme's if unsupported functionality is being used */
#define BUFFER_OR_DATA(_attribute) dataLocations->u.s._attribute.lpData
    /* TODO: Either support missing functionality in fixupVertices or by creating a shader to replace the pipeline. */
    if (!useVertexShaderFunction && (BUFFER_OR_DATA(blendMatrixIndices) || BUFFER_OR_DATA(blendWeights))) {
        FIXME("Blending data is only valid with vertex shaders %p %p\n",dataLocations->u.s.blendWeights.lpData,dataLocations->u.s.blendWeights.lpData);
    }
    if (!useVertexShaderFunction && (BUFFER_OR_DATA(position2) || BUFFER_OR_DATA(normal2))) {
        FIXME("Tweening is only valid with vertex shaders\n");
    }
    if (!useVertexShaderFunction && (BUFFER_OR_DATA(tangent) || BUFFER_OR_DATA(binormal))) {
        FIXME("Tangent and binormal bump mapping is only valid with vertex shaders\n");
    }
    if (!useVertexShaderFunction && (BUFFER_OR_DATA(tessFactor) || BUFFER_OR_DATA(fog) || BUFFER_OR_DATA(depth) || BUFFER_OR_DATA(sample))) {
        FIXME("Extended attributes are only valid with vertex shaders\n");
    }
#undef BUFFER_OR_DATA

    /* Fixed pipeline, no fixups required - load arrays */
    if (!useVertexShaderFunction &&
       ((dataLocations->u.s.pSize.lpData == NULL &&
         dataLocations->u.s.diffuse.lpData == NULL &&
         dataLocations->u.s.specular.lpData == NULL) ||
         fixup) ) {

        /* Load the vertex data using named arrays */
        TRACE("(%p) Loading vertex data\n", This);
        loadVertexData(iface, dataLocations);
        useDrawStridedSlow = FALSE;

    /* Shader pipeline - load attribute arrays */
    } else if(useVertexShaderFunction) {

        loadNumberedArrays(iface, This->stateBlock->vertexShader, dataLocations);
        useDrawStridedSlow = FALSE;

        /* We compile the shader here because we need the vertex declaration
         * in order to determine if we need to do any swizzling for D3DCOLOR
         * registers. If the shader is already compiled this call will do nothing. */
        IWineD3DVertexShader_CompileShader(This->stateBlock->vertexShader);
    /* Draw vertex by vertex */
    } else { 
        TRACE("Not loading vertex data\n");
        useDrawStridedSlow = TRUE;
    }

    /* If GLSL is used for either pixel or vertex shaders, make a GLSL program 
     * Otherwise set NULL, to restore fixed function */
    if ((wined3d_settings.vs_selected_mode == SHADER_GLSL && useVertexShaderFunction) ||
        (wined3d_settings.ps_selected_mode == SHADER_GLSL && usePixelShaderFunction)) 
        set_glsl_shader_program(iface);
    else
        This->stateBlock->glsl_program = NULL;

    /* If GLSL is used now, or might have been used before, (re)set the program */
    if (wined3d_settings.vs_selected_mode == SHADER_GLSL || 
        wined3d_settings.ps_selected_mode == SHADER_GLSL) {

        GLhandleARB progId = This->stateBlock->glsl_program ? This->stateBlock->glsl_program->programId : 0;
        if (progId)
            TRACE_(d3d_shader)("Using GLSL program %u\n", progId);
        GL_EXTCALL(glUseProgramObjectARB(progId));
        checkGLcall("glUseProgramObjectARB");
    }
        
    if (useVertexShaderFunction) {

        TRACE("Using vertex shader\n");

        if (wined3d_settings.vs_selected_mode == SHADER_ARB) {
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
    }

    if (usePixelShaderFunction) {

        TRACE("Using pixel shader\n");

        if (wined3d_settings.ps_selected_mode == SHADER_ARB) {
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
       
    /* Load any global constants/uniforms that may have been set by the application */
    if (wined3d_settings.vs_selected_mode == SHADER_GLSL || wined3d_settings.ps_selected_mode == SHADER_GLSL)
        shader_glsl_load_constants((IWineD3DStateBlock*)This->stateBlock, usePixelShaderFunction, useVertexShaderFunction);
    else if (wined3d_settings.vs_selected_mode== SHADER_ARB || wined3d_settings.ps_selected_mode == SHADER_ARB)
        shader_arb_load_constants((IWineD3DStateBlock*)This->stateBlock, usePixelShaderFunction, useVertexShaderFunction); 
        
    /* Draw vertex-by-vertex */
    if (useDrawStridedSlow)
        drawStridedSlow(iface, dataLocations, numberOfIndicies, glPrimType, idxData, idxSize, minIndex,  StartIdx);
    else
        drawStridedFast(iface, numberOfIndicies, glPrimType, idxData, idxSize, minIndex, StartIdx);

    /* Cleanup vertex program */
    if (useVertexShaderFunction) {
        unloadNumberedArrays(iface);

        if (wined3d_settings.vs_selected_mode == SHADER_ARB)
            glDisable(GL_VERTEX_PROGRAM_ARB);
    } else {
        unloadVertexData(iface);
    }

    /* Cleanup fragment program */
    if (usePixelShaderFunction && wined3d_settings.ps_selected_mode == SHADER_ARB) 
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

inline void drawPrimitiveTraceDataLocations(
    WineDirect3DVertexStridedData *dataLocations) {

    /* Dump out what parts we have supplied */
    TRACE("Strided Data:\n");
    TRACE_STRIDED((dataLocations), position);
    TRACE_STRIDED((dataLocations), blendWeights);
    TRACE_STRIDED((dataLocations), blendMatrixIndices);
    TRACE_STRIDED((dataLocations), normal);
    TRACE_STRIDED((dataLocations), pSize);
    TRACE_STRIDED((dataLocations), diffuse);
    TRACE_STRIDED((dataLocations), specular);
    TRACE_STRIDED((dataLocations), texCoords[0]);
    TRACE_STRIDED((dataLocations), texCoords[1]);
    TRACE_STRIDED((dataLocations), texCoords[2]);
    TRACE_STRIDED((dataLocations), texCoords[3]);
    TRACE_STRIDED((dataLocations), texCoords[4]);
    TRACE_STRIDED((dataLocations), texCoords[5]);
    TRACE_STRIDED((dataLocations), texCoords[6]);
    TRACE_STRIDED((dataLocations), texCoords[7]);
    TRACE_STRIDED((dataLocations), position2);
    TRACE_STRIDED((dataLocations), normal2);
    TRACE_STRIDED((dataLocations), tangent);
    TRACE_STRIDED((dataLocations), binormal);
    TRACE_STRIDED((dataLocations), tessFactor);
    TRACE_STRIDED((dataLocations), fog);
    TRACE_STRIDED((dataLocations), depth);
    TRACE_STRIDED((dataLocations), sample);

    return;

}

static void drawPrimitiveUploadTexturesPS(IWineD3DDeviceImpl* This) {
    INT i;

    for (i = 0; i < GL_LIMITS(samplers); ++i) {
        /* Pixel shader support should imply multitexture support. */
        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
            checkGLcall("glActiveTextureARB");
        } else if (i) {
            WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
        }

        if (!This->stateBlock->textures[i]) continue;

        /* Enable the correct target. Is this required for GLSL? For ARB_fragment_program it isn't, afaik. */
        glDisable(GL_TEXTURE_1D);
        This->stateBlock->textureDimensions[i] = IWineD3DBaseTexture_GetTextureDimensions(This->stateBlock->textures[i]);
        switch(This->stateBlock->textureDimensions[i]) {
            case GL_TEXTURE_2D:
                glDisable(GL_TEXTURE_3D);
                glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                break;
            case GL_TEXTURE_3D:
                glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                glDisable(GL_TEXTURE_2D);
                break;
            case GLTEXTURECUBEMAP:
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_TEXTURE_3D);
                break;
        }
        glEnable(This->stateBlock->textureDimensions[i]);

        /* Upload texture, apply states */
        IWineD3DBaseTexture_PreLoad((IWineD3DBaseTexture *) This->stateBlock->textures[i]);
        IWineD3DDevice_SetupTextureStates((IWineD3DDevice *)This, i, i, REAPPLY_ALPHAOP);
        IWineD3DBaseTexture_ApplyStateChanges(This->stateBlock->textures[i], This->stateBlock->textureState[i], This->stateBlock->samplerState[i]);
    }
}

/* uploads textures and setup texture states ready for rendering */
static void drawPrimitiveUploadTextures(IWineD3DDeviceImpl* This) {
    INT current_sampler = 0;
    float constant_color[4];
    unsigned int i;

    /* ARB_texture_env_combine is limited to GL_MAX_TEXTURE_UNITS stages. On
     * nVidia cards GL_MAX_TEXTURE_UNITS is generally not larger than 4.
     * Register combiners however provide up to 8 combiner stages. In order to
     * take advantage of this, we need to be separate D3D texture stages from
     * GL texture units. When using register combiners GL_MAX_TEXTURE_UNITS
     * corresponds to MaxSimultaneousTextures and GL_MAX_GENERAL_COMBINERS_NV
     * corresponds to MaxTextureBlendStages in the caps. */

    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        glEnable(GL_REGISTER_COMBINERS_NV);
        D3DCOLORTOGLFLOAT4(This->stateBlock->renderState[WINED3DRS_TEXTUREFACTOR], constant_color);
        GL_EXTCALL(glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, &constant_color[0]));
    }

    for (i = 0; i < GL_LIMITS(texture_stages); ++i) {
        INT texture_idx = -1;

        /* D3DTOP_DISABLE disables the current & any higher texture stages */
        if (This->stateBlock->textureState[i][WINED3DTSS_COLOROP] == D3DTOP_DISABLE) break;

        if (!GL_SUPPORT(NV_REGISTER_COMBINERS) || This->stateBlock->textures[i]) {
            texture_idx = current_sampler++;

            /* Active the texture unit corresponding to the current texture stage */
            if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + texture_idx));
                checkGLcall("glActiveTextureARB");
            } else if (i) {
                WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            }
        }

        if (This->stateBlock->textures[i]) {
            /* Enable the correct target. */
            glDisable(GL_TEXTURE_1D);
            This->stateBlock->textureDimensions[i] = IWineD3DBaseTexture_GetTextureDimensions(This->stateBlock->textures[i]);
            switch(This->stateBlock->textureDimensions[i]) {
                case GL_TEXTURE_2D:
                    glDisable(GL_TEXTURE_3D);
                    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                    break;
                case GL_TEXTURE_3D:
                    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                    glDisable(GL_TEXTURE_2D);
                    break;
                case GLTEXTURECUBEMAP:
                    glDisable(GL_TEXTURE_2D);
                    glDisable(GL_TEXTURE_3D);
                    break;
            }

            /* imply GL_SUPPORT(NV_TEXTURE_SHADER) when setting texture_shader_active */
            if (This->texture_shader_active && This->stateBlock->textureDimensions[i] == GL_TEXTURE_2D) {
                glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_2D);
            } else {
                glEnable(This->stateBlock->textureDimensions[i]);
            }

            /* Upload texture, apply states */
            IWineD3DBaseTexture_PreLoad((IWineD3DBaseTexture *) This->stateBlock->textures[i]);
            IWineD3DDevice_SetupTextureStates((IWineD3DDevice *)This, i, texture_idx, REAPPLY_ALPHAOP);
            IWineD3DBaseTexture_ApplyStateChanges(This->stateBlock->textures[i], This->stateBlock->textureState[i], This->stateBlock->samplerState[i]);
        } else if (!GL_SUPPORT(NV_REGISTER_COMBINERS)) {
            /* ARB_texture_env_combine needs a valid texture bound to the
             * texture unit, even if it isn't used. Bind a dummy texture. */
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_3D);
            glDisable(GL_TEXTURE_CUBE_MAP_ARB);
            glEnable(GL_TEXTURE_1D);
            This->stateBlock->textureDimensions[i] = GL_TEXTURE_1D;
            glBindTexture(GL_TEXTURE_1D, This->dummyTextureName[i]);
        }

        /** these ops apply to the texture unit, so they are preserved between texture changes, but for now brute force and reapply all
          dx9_1pass_emboss_bump_mapping and dx9_2pass_emboss_bump_mapping are good texts to make sure the states are being applied when needed **/
        if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
            set_tex_op_nvrc((IWineD3DDevice *)This, FALSE, i, This->stateBlock->textureState[i][WINED3DTSS_COLOROP],
                    This->stateBlock->textureState[i][WINED3DTSS_COLORARG1],
                    This->stateBlock->textureState[i][WINED3DTSS_COLORARG2],
                    This->stateBlock->textureState[i][WINED3DTSS_COLORARG0],
                    texture_idx);
            /* alphaop */
            set_tex_op_nvrc((IWineD3DDevice *)This, TRUE, i, This->stateBlock->textureState[i][WINED3DTSS_ALPHAOP],
                    This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG1],
                    This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG2],
                    This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG0],
                    texture_idx);
        } else {
            set_tex_op((IWineD3DDevice *)This, FALSE, i, This->stateBlock->textureState[i][WINED3DTSS_COLOROP],
                    This->stateBlock->textureState[i][WINED3DTSS_COLORARG1],
                    This->stateBlock->textureState[i][WINED3DTSS_COLORARG2],
                    This->stateBlock->textureState[i][WINED3DTSS_COLORARG0]);
            /* alphaop */
            set_tex_op((IWineD3DDevice *)This, TRUE, i, This->stateBlock->textureState[i][WINED3DTSS_ALPHAOP],
                    This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG1],
                    This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG2],
                    This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG0]);
        }
    }

    /* If we're using register combiners, set the amount of *used* combiners.
     * Ie, the number of stages below the first stage to have a color op of
     * D3DTOP_DISABLE. */
    if (GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        /* NUM_GENERAL_COMBINERS_NV should be > 0 */
        if (!i) glDisable(GL_REGISTER_COMBINERS_NV);
        else GL_EXTCALL(glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, i));
    }

    /* Disable the remaining texture units. */
    for (i = current_sampler; i < GL_LIMITS(textures); ++i) {
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_3D);
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
    }
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(IWineD3DDevice *iface,
                   int PrimitiveType,
                   long NumPrimitives,
                   /* for Indexed: */
                   long  StartVertexIndex,
                   UINT  numberOfVertices,
                   long  StartIdx,
                   short idxSize,
                   const void *idxData,
                   int   minIndex,
                   WineDirect3DVertexStridedData *DrawPrimStrideData) {

    IWineD3DDeviceImpl           *This = (IWineD3DDeviceImpl *)iface;
    BOOL                          useVertexShaderFunction = FALSE;
    BOOL                          usePixelShaderFunction = FALSE;
    WineDirect3DVertexStridedData *dataLocations;
    IWineD3DSwapChainImpl         *swapchain;
    int                           i;
    BOOL                          fixup = FALSE;

    BOOL lighting_changed, lighting_original = FALSE;

    /* Shaders can be implemented using ARB_PROGRAM, GLSL, or software - 
     * here simply check whether a shader was set, or the user disabled shaders */
    if (wined3d_settings.vs_selected_mode != SHADER_NONE && This->stateBlock->vertexShader && 
        ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.function != NULL) 
        useVertexShaderFunction = TRUE;

    if (wined3d_settings.ps_selected_mode != SHADER_NONE && This->stateBlock->pixelShader &&
        ((IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader)->baseShader.function) 
        usePixelShaderFunction = TRUE;

    /* Invalidate the back buffer memory so LockRect will read it the next time */
    for(i = 0; i < IWineD3DDevice_GetNumberOfSwapChains(iface); i++) {
        IWineD3DDevice_GetSwapChain(iface, i, (IWineD3DSwapChain **) &swapchain);
        if(swapchain) {
            if(swapchain->backBuffer) ((IWineD3DSurfaceImpl *) swapchain->backBuffer[0])->Flags |= SFLAG_GLDIRTY;
            IWineD3DSwapChain_Release( (IWineD3DSwapChain *) swapchain);
        }
    }

    /* Ok, we will be updating the screen from here onwards so grab the lock */
    ENTER_GL();

    if(DrawPrimStrideData) {

        /* Note: this is a ddraw fixed-function code path */

        TRACE("================ Strided Input ===================\n");
        dataLocations = DrawPrimStrideData;
        drawPrimitiveTraceDataLocations(dataLocations);
        fixup = FALSE;
    }

    else if (This->stateBlock->vertexDecl || This->stateBlock->vertexShader) {

        /* Note: This is a fixed function or shader codepath.
         * This means it must handle both types of strided data.
         * Shaders must go through here to zero the strided data, even if they
         * don't set any declaration at all */

        TRACE("================ Vertex Declaration  ===================\n");
        dataLocations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dataLocations));
        if(!dataLocations) {
            ERR("Out of memory!\n");
            return;
        }

        if (This->stateBlock->vertexDecl != NULL ||
            ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->vertexDeclaration != NULL)            

            primitiveDeclarationConvertToStridedData(iface, useVertexShaderFunction, 
                dataLocations, StartVertexIndex, &fixup);

    } else {

        /* Note: This codepath is not reachable from d3d9 (see fvf->decl9 conversion)
         * It is reachable through d3d8, but only for fixed-function.
         * It will not work properly for shaders. */

        TRACE("================ FVF ===================\n");
        dataLocations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dataLocations));
        if(!dataLocations) {
            ERR("Out of memory!\n");
            return;
        }
        primitiveConvertToStridedData(iface, dataLocations, StartVertexIndex, &fixup);
        drawPrimitiveTraceDataLocations(dataLocations);
    }

    /* Setup transform matrices and sort out */
    primitiveInitState(iface, dataLocations, useVertexShaderFunction, &lighting_changed, &lighting_original);

    /* Now initialize the materials state */
    init_materials(iface, (dataLocations->u.s.diffuse.lpData != NULL || dataLocations->u.s.diffuse.VBO != 0));

    if (usePixelShaderFunction) {
        drawPrimitiveUploadTexturesPS(This);
    } else {
        drawPrimitiveUploadTextures(This);
    }

    {
        GLenum glPrimType;
        /* Ok, Work out which primitive is requested and how many vertexes that
           will be                                                              */
        UINT calculatedNumberOfindices = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);
        if (numberOfVertices == 0 )
            numberOfVertices = calculatedNumberOfindices;

        drawPrimitiveDrawStrided(iface, useVertexShaderFunction, usePixelShaderFunction,
            dataLocations, numberOfVertices, calculatedNumberOfindices, glPrimType,
            idxData, idxSize, minIndex, StartIdx, fixup);
    }

    if(!DrawPrimStrideData) HeapFree(GetProcessHeap(), 0, dataLocations);

    /* If vertex shaders or no normals, restore previous lighting state */
    if (lighting_changed) {
        if (lighting_original) glEnable(GL_LIGHTING);
        else glDisable(GL_LIGHTING);
        TRACE("Restored lighting to original state\n");
    }

    /* Finshed updating the screen, restore lock */
    LEAVE_GL();
    TRACE("Done all gl drawing\n");

    /* Diagnostics */
#ifdef SHOW_FRAME_MAKEUP
    {
        static long int primCounter = 0;
        /* NOTE: set primCounter to the value reported by drawprim 
           before you want to to write frame makeup to /tmp */
        if (primCounter >= 0) {
            WINED3DLOCKED_RECT r;
            char buffer[80];
            IWineD3DSurface_LockRect(This->renderTarget, &r, NULL, WINED3DLOCK_READONLY);
            sprintf(buffer, "/tmp/backbuffer_%ld.tga", primCounter);
            TRACE("Saving screenshot %s\n", buffer);
            IWineD3DSurface_SaveSnapshot(This->renderTarget, buffer);
            IWineD3DSurface_UnlockRect(This->renderTarget);

#ifdef SHOW_TEXTURE_MAKEUP
           {
            IWineD3DSurface *pSur;
            int textureNo;
            for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {
                if (This->stateBlock->textures[textureNo] != NULL) {
                    sprintf(buffer, "/tmp/texture_%p_%ld_%d.tga", This->stateBlock->textures[textureNo], primCounter, textureNo);
                    TRACE("Saving texture %s\n", buffer);
                    if (IWineD3DBaseTexture_GetType(This->stateBlock->textures[textureNo]) == WINED3DRTYPE_TEXTURE) {
                            IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)This->stateBlock->textures[textureNo], 0, &pSur);
                            IWineD3DSurface_SaveSnapshot(pSur, buffer);
                            IWineD3DSurface_Release(pSur);
                    } else  {
                        FIXME("base Texture isn't of type texture %d\n", IWineD3DBaseTexture_GetType(This->stateBlock->textures[textureNo]));
                    }
                }
            }
           }
#endif
        }
        TRACE("drawprim #%ld\n", primCounter);
        ++primCounter;
    }
#endif
}
