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

/* Returns bits for what is expected from the fixed function pipeline, and whether
   a vertex shader will be in use. Note the fvf bits returned may be split over
   multiple streams only if the vertex shader was created, otherwise it all relates
   to stream 0                                                                      */
static BOOL initializeFVF(IWineD3DDevice *iface, DWORD *FVFbits)
{

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

#if 0 /* TODO: d3d8 call setvertexshader needs to set the FVF in the state block when implemented */
    /* The first thing to work out is if we are using the fixed function pipeline
       which is either SetVertexShader with < VS_HIGHESTFIXEDFXF - in which case this
       is the FVF, or with a shader which was created with no function - in which
       case there is an FVF per declared stream. If this occurs, we also maintain
       an 'OR' of all the FVF's together so we know what to expect across all the
       streams                                                                        */
#endif
    *FVFbits = This->stateBlock->fvf;
#if 0
        *FVFbits = This->stateBlock->vertexShaderDecl->allFVF;
#endif
    return FALSE;
}

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
        TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, -minZ, -maxZ);
        glOrtho(X, X + width, Y + height, Y, -minZ, -maxZ);
        checkGLcall("glOrtho");

        /* Window Coord 0 is the middle of the first pixel, so translate by half
            a pixel (See comment above glTranslate below)                         */
        glTranslatef(0.375, 0.375, 0);
        checkGLcall("glTranslatef(0.375, 0.375, 0)");
        if (This->renderUpsideDown) {
            glMultMatrixf(invymat);
            checkGLcall("glMultMatrixf(invymat)");
        }
    }

}
/* Setup views - Transformed & lit if RHW, else untransformed.
       Only unlit if Normals are supplied
    Returns: Whether to restore lighting afterwards           */
static BOOL primitiveInitState(IWineD3DDevice *iface, BOOL vtx_transformed, BOOL vtx_lit, BOOL useVS) {

    BOOL isLightingOn = FALSE;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* If no normals, DISABLE lighting otherwise, don't touch lighing as it is
       set by the appropriate render state. Note Vertex Shader output is already lit */
    if (vtx_lit || useVS) {
        isLightingOn = glIsEnabled(GL_LIGHTING);
        glDisable(GL_LIGHTING);
        checkGLcall("glDisable(GL_LIGHTING);");
        TRACE("Disabled lighting as no normals supplied, old state = %d\n", isLightingOn);
    }

    if (!useVS && vtx_transformed) {
        d3ddevice_set_ortho(This);
    } else {

        /* Untransformed, so relies on the view and projection matrices */

        if (!useVS && (This->last_was_rhw || !This->modelview_valid)) {
            /* Only reapply when have to */
            This->modelview_valid = TRUE;
            glMatrixMode(GL_MODELVIEW);
            checkGLcall("glMatrixMode");

            /* In the general case, the view matrix is the identity matrix */
            if (This->view_ident) {
                glLoadMatrixf((float *) &This->stateBlock->transforms[D3DTS_WORLDMATRIX(0)].u.m[0][0]);
                checkGLcall("glLoadMatrixf");
            } else {
                glLoadMatrixf((float *) &This->stateBlock->transforms[D3DTS_VIEW].u.m[0][0]);
                checkGLcall("glLoadMatrixf");
                glMultMatrixf((float *) &This->stateBlock->transforms[D3DTS_WORLDMATRIX(0)].u.m[0][0]);
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
            glMultMatrixf((float *) &This->stateBlock->transforms[D3DTS_PROJECTION].u.m[0][0]);
            checkGLcall("glLoadMatrixf");
        }

        /* Vertex Shader output is already transformed, so set up identity matrices */
        /* FIXME: Actually, only true for software emulated ones, so when h/w ones
             come along this needs to take into account whether s/w ones were
             requested or not                                                       */
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
    }
    return isLightingOn;
}

void primitiveDeclarationConvertToStridedData(
     IWineD3DDevice *iface,
     BOOL useVertexShaderFunction,
     WineDirect3DVertexStridedData *strided,
     LONG BaseVertexIndex, 
     DWORD *fvf) {

     /* We need to deal with frequency data!*/

    int           textureNo =0;
    BYTE  *data    = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexDeclarationImpl* vertexDeclaration = NULL;
    int i;
    WINED3DVERTEXELEMENT *element;
    DWORD stride;
    int reg;

    /* Locate the vertex declaration */
    if (useVertexShaderFunction && ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->vertexDeclaration) {
        TRACE("Using vertex declaration from shader\n");
        vertexDeclaration = (IWineD3DVertexDeclarationImpl *)((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->vertexDeclaration;
    } else {
        TRACE("Using vertex declaration\n");
        vertexDeclaration = (IWineD3DVertexDeclarationImpl *)This->stateBlock->vertexDecl;
    }

    /* Translate the declaration into strided data */
    for (i = 0 ; i < vertexDeclaration->declarationWNumElements - 1; ++i) {

        element = vertexDeclaration->pDeclarationWine + i;
        TRACE("%p Elements %p %d or %d\n", vertexDeclaration->pDeclarationWine, element,  i, vertexDeclaration->declarationWNumElements);
        if (This->stateBlock->streamIsUP) {
            TRACE("Stream is up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            data    = (BYTE *)This->stateBlock->streamSource[element->Stream];
        } else {
            TRACE("Stream isn't up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            data    = IWineD3DVertexBufferImpl_GetMemory(This->stateBlock->streamSource[element->Stream], 0);
        }
        stride  = This->stateBlock->streamStride[element->Stream];
        data += (BaseVertexIndex * stride);
        data += element->Offset;
        reg = element->Reg;

        TRACE("Offset %d Stream %d UsageIndex %d\n", element->Offset, element->Stream, element->UsageIndex);

        if (useVertexShaderFunction && reg != -1 && data) {
            WINED3DGLTYPE glType = glTypeLookup[element->Type];

            TRACE("(%p) : Set vertex attrib pointer: reg 0x%08x, d3d type 0x%08x, stride 0x%08lx, data %p)\n", This, reg, element->Type, stride, data);

            GL_EXTCALL(glVertexAttribPointerARB(reg, glType.size, glType.glType, glType.normalized, stride, data));
            checkGLcall("glVertexAttribPointerARB");
            GL_EXTCALL(glEnableVertexAttribArrayARB(reg));
            checkGLcall("glEnableVertexAttribArrayARB");
        }

        switch (element->Usage) {
        case D3DDECLUSAGE_POSITION:
                switch (element->UsageIndex) {
                case 0: /* N-patch */
                    strided->u.s.position.lpData    = data;
                    strided->u.s.position.dwType    = element->Type;
                    strided->u.s.position.dwStride  = stride;
                    TRACE("Set strided %s. data %p, type %d. stride %ld\n", "position", data, element->Type, stride);
                break;
                case 1: /* tweened see http://www.gamedev.net/reference/articles/article2017.asp */
                    TRACE("Tweened positions\n");
                    strided->u.s.position2.lpData    = data;
                    strided->u.s.position2.dwType    = element->Type;
                    strided->u.s.position2.dwStride  = stride;
                    TRACE("Set strided %s. data %p, type %d. stride %ld\n", "position2", data, element->Type, stride);
                break;
                }
        break;
        case D3DDECLUSAGE_NORMAL:
                switch (element->UsageIndex) {
                case 0: /* N-patch */
                    strided->u.s.normal.lpData    = data;
                    strided->u.s.normal.dwType    = element->Type;
                    strided->u.s.normal.dwStride  = stride;
                    TRACE("Set strided %s. data %p, type %d. stride %ld\n", "normal", data, element->Type, stride);
                break;
                case 1: /* skinning */
                    TRACE("Skinning / tween normals\n");
                    strided->u.s.normal2.lpData    = data;
                    strided->u.s.normal2.dwType    = element->Type;
                    strided->u.s.normal2.dwStride  = stride;
                    TRACE("Set strided %s. data %p, type %d. stride %ld\n", "normal2", data, element->Type, stride);
                break;
                }
                *fvf |=  D3DFVF_NORMAL;
        break;
        case D3DDECLUSAGE_BLENDINDICES:
        /* demo @http://www.ati.com/developer/vertexblend.html
            and http://www.flipcode.com/articles/article_dx8shaders.shtml
        */
            strided->u.s.blendMatrixIndices.lpData  = data;
            strided->u.s.blendMatrixIndices.dwType  = element->Type;
            strided->u.s.blendMatrixIndices.dwStride= stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "blendMatrixIndices", data, element->Type, stride);
        break;
        case D3DDECLUSAGE_BLENDWEIGHT:
            strided->u.s.blendWeights.lpData        = data;
            strided->u.s.blendWeights.dwType        = element->Type;
            strided->u.s.blendWeights.dwStride      = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "blendWeights", data, element->Type, stride);
        break;
        case D3DDECLUSAGE_PSIZE:
            strided->u.s.pSize.lpData               = data;
            strided->u.s.pSize.dwType               = element->Type;
            strided->u.s.pSize.dwStride             = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "pSize", data, element->Type, stride);
        break;
        case D3DDECLUSAGE_COLOR:
        switch (element->UsageIndex) {
        case 0:/* diffuse */
            strided->u.s.diffuse.lpData             = data;
            strided->u.s.diffuse.dwType             = element->Type;
            strided->u.s.diffuse.dwStride           = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "diffuse", data, element->Type, stride);
        break;
        case 1: /* specular */
            strided->u.s.specular.lpData            = data;
            strided->u.s.specular.dwType            = element->Type;
            strided->u.s.specular.dwStride          = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "specular", data, element->Type, stride);

        }

        break;
        case D3DDECLUSAGE_TEXCOORD:
        /* For some odd reason Microsoft decided to sum usage accross all the streams,
        which means we need to do a count and not just use the usage number */

            strided->u.s.texCoords[textureNo].lpData    = data;
            strided->u.s.texCoords[textureNo].dwType    = element->Type;
            strided->u.s.texCoords[textureNo].dwStride  = stride;
            TRACE("Set strided %s.%d data %p, type %d. stride %ld\n", "texCoords", textureNo, data, element->Type, stride);

            ++textureNo;
        break;
        case D3DDECLUSAGE_TANGENT:
        /* Implement tangents and binormals using http://oss.sgi.com/projects/ogl-sample/registry/EXT/coordinate_frame.txt
        this is easy so long as the OpenGL implementation supports it, otherwise drop back to calculating the
        normal using tangents where no normal data has been provided */
            TRACE("Tangents\n");
            strided->u.s.tangent.lpData   = data;
            strided->u.s.tangent.dwType   = element->Type;
            strided->u.s.tangent.dwStride = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "tangent", data, element->Type, stride);
        break;
        case D3DDECLUSAGE_BINORMAL:
        /* Binormals are really bitangents perpendicular to the normal but s-aligned to the tangent, basically they are the vectors of any two lines on the plain at right angles to the normal and at right angles to each other, like the x,y,z axis.
        tangent data makes it easier to perform some calculations (a bit like using 2d graph paper instead of the normal of the piece of paper)
        The only thing they are useful for in fixed function would be working out normals when none are given.
        */
            TRACE("BI-Normal\n");
            strided->u.s.binormal.lpData   = data;
            strided->u.s.binormal.dwType   = element->Type;
            strided->u.s.binormal.dwStride = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "binormal", data, element->Type, stride);
        break;
        case D3DDECLUSAGE_TESSFACTOR:
        /* a google for D3DDECLUSAGE_TESSFACTOR turns up a whopping 36 entries, 7 of which are from MSDN.
        */
            TRACE("Tess Factor\n");
            strided->u.s.tessFactor.lpData   = data;
            strided->u.s.tessFactor.dwType   = element->Type;
            strided->u.s.tessFactor.dwStride = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "tessFactor", data, element->Type, stride);
        break;
        case D3DDECLUSAGE_POSITIONT:

               switch (element->UsageIndex) {
                case 0: /* N-patch */
                    strided->u.s.position.lpData    = data;
                    strided->u.s.position.dwType    = element->Type;
                    strided->u.s.position.dwStride  = stride;
                    TRACE("Set strided %s. data %p, type %d. stride %ld\n", "positionT", data, element->Type, stride);
                break;
                case 1: /* skinning */
                        /* see http://rsn.gamedev.net/tutorials/ms3danim.asp
                        http://xface.blogspot.com/2004_08_01_xface_archive.html
                        */
                    TRACE("Skinning positionsT\n");
                    strided->u.s.position2.lpData    = data;
                    strided->u.s.position2.dwType    = element->Type;
                    strided->u.s.position2.dwStride  = stride;
                    TRACE("Set strided %s. data %p, type %d. stride %ld\n", "position2T", data, element->Type, stride);
                break;
                }
                /* TODO: change fvf usage to a plain boolean flag */
                *fvf |= D3DFVF_XYZRHW;
            /* FIXME: were faking this flag so that we don't transform the data again */
        break;
        case D3DDECLUSAGE_FOG:
        /* maybe GL_EXT_fog_coord ?
        * http://oss.sgi.com/projects/ogl-sample/registry/EXT/fog_coord.txt
        * This extension allows specifying an explicit per-vertex fog
        * coordinate to be used in fog computations, rather than using a
        * fragment depth-based fog equation.
        *
        * */
            TRACE("Fog\n");
            strided->u.s.fog.lpData   = data;
            strided->u.s.fog.dwType   = element->Type;
            strided->u.s.fog.dwStride = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "fog", data, element->Type, stride);
        break;
        case D3DDECLUSAGE_DEPTH:
            TRACE("depth\n");
            strided->u.s.depth.lpData   = data;
            strided->u.s.depth.dwType   = element->Type;
            strided->u.s.depth.dwStride = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "depth", data, element->Type, stride);
            break;
        case D3DDECLUSAGE_SAMPLE: /* VertexShader textures */
            TRACE("depth\n");
            strided->u.s.sample.lpData   = data;
            strided->u.s.sample.dwType   = element->Type;
            strided->u.s.sample.dwStride = stride;
            TRACE("Set strided %s. data %p, type %d. stride %ld\n", "sample", data, element->Type, stride);
        break;
        };

    };

}

void primitiveConvertToStridedData(IWineD3DDevice *iface, WineDirect3DVertexStridedData *strided, LONG BaseVertexIndex) {

    short         LoopThroughTo = 0;
    short         nStream;
    int           numBlends;
    int           numTextures;
    int           textureNo;
    int           coordIdxInfo = 0x00;    /* Information on number of coords supplied */
    int           numCoords[8];           /* Holding place for D3DFVF_TEXTUREFORMATx  */

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
                data    = (BYTE *)This->stateBlock->streamSource[nStream];
            } else {
                data = IWineD3DVertexBufferImpl_GetMemory(This->stateBlock->streamSource[nStream], 0);
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

        /* Either 3 or 4 floats depending on the FVF */
        /* FIXME: Can blending data be in a different stream to the position data?
              and if so using the fixed pipeline how do we handle it               */
        if (thisFVF & D3DFVF_POSITION_MASK) {
            strided->u.s.position.lpData    = data;
            strided->u.s.position.dwType    = D3DDECLTYPE_FLOAT3;
            strided->u.s.position.dwStride  = stride;
            data += 3 * sizeof(float);
            if (thisFVF & D3DFVF_XYZRHW) {
                strided->u.s.position.dwType = D3DDECLTYPE_FLOAT4;
                data += sizeof(float);
            }
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
            data += numBlends * sizeof(FLOAT);

            if (thisFVF & D3DFVF_LASTBETA_UBYTE4) {
                strided->u.s.blendMatrixIndices.lpData = data;
                strided->u.s.blendMatrixIndices.dwType  = D3DDECLTYPE_UBYTE4;
                strided->u.s.blendMatrixIndices.dwStride= stride;
                data += sizeof(DWORD);
            }
        }

        /* Normal is always 3 floats */
        if (thisFVF & D3DFVF_NORMAL) {
            strided->u.s.normal.lpData    = data;
            strided->u.s.normal.dwType    = D3DDECLTYPE_FLOAT3;
            strided->u.s.normal.dwStride  = stride;
            data += 3 * sizeof(FLOAT);
        }

        /* Pointsize is a single float */
        if (thisFVF & D3DFVF_PSIZE) {
            strided->u.s.pSize.lpData    = data;
            strided->u.s.pSize.dwType    = D3DDECLTYPE_FLOAT1;
            strided->u.s.pSize.dwStride  = stride;
            data += sizeof(FLOAT);
        }

        /* Diffuse is 4 unsigned bytes */
        if (thisFVF & D3DFVF_DIFFUSE) {
            strided->u.s.diffuse.lpData    = data;
            strided->u.s.diffuse.dwType    = D3DDECLTYPE_SHORT4;
            strided->u.s.diffuse.dwStride  = stride;
            data += sizeof(DWORD);
        }

        /* Specular is 4 unsigned bytes */
        if (thisFVF & D3DFVF_SPECULAR) {
            strided->u.s.specular.lpData    = data;
            strided->u.s.specular.dwType    = D3DDECLTYPE_SHORT4;
            strided->u.s.specular.dwStride  = stride;
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

void loadNumberedArrays(IWineD3DDevice *iface, WineDirect3DVertexStridedData *sd, INT arrayUsageMap[WINED3DSHADERDECLUSAGE_MAX_USAGE]) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

#define LOAD_NUMBERED_ARRAY(_arrayName, _lookupName) \
    if (sd->u.s._arrayName.lpData != NULL && ((arrayUsageMap[WINED3DSHADERDECLUSAGE_##_lookupName] & 0x7FFF) == arrayUsageMap[WINED3DSHADERDECLUSAGE_##_lookupName])) { \
       TRACE_(d3d_shader)("Loading array %u with data from %s\n", arrayUsageMap[WINED3DSHADERDECLUSAGE_##_lookupName], #_arrayName); \
       GL_EXTCALL(glVertexAttribPointerARB(arrayUsageMap[WINED3DSHADERDECLUSAGE_##_lookupName], \
                        WINED3D_ATR_SIZE(_arrayName), \
                        WINED3D_ATR_GLTYPE(_arrayName), \
                        WINED3D_ATR_NORMALIZED(_arrayName), \
                        sd->u.s._arrayName.dwStride, \
                        sd->u.s._arrayName.lpData)); \
        GL_EXTCALL(glEnableVertexAttribArrayARB(arrayUsageMap[WINED3DSHADERDECLUSAGE_##_lookupName])); \
    }


#define LOAD_NUMBERED_POSITION_ARRAY(_lookupNumber) \
    if (sd->u.s.position2.lpData != NULL && ((arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITION2 + _lookupNumber] & 0x7FFF) == arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITION2 + _lookupNumber])) { \
       FIXME_(d3d_shader)("Loading array %u with data from %s\n", arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITION2 + _lookupNumber], "position2"); \
       GL_EXTCALL(glVertexAttribPointerARB(arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITION2 + _lookupNumber], \
                        WINED3D_ATR_SIZE(position2), \
                        WINED3D_ATR_GLTYPE(position2), \
                        WINED3D_ATR_NORMALIZED(position2), \
                        sd->u.s.position2.dwStride, \
                        ((char *)sd->u.s.position2.lpData) + \
                        WINED3D_ATR_SIZE(position2) * WINED3D_ATR_TYPESIZE(position2) * _lookupNumber)); \
        GL_EXTCALL(glEnableVertexAttribArrayARB(arrayUsageMap[WINED3DSHADERDECLUSAGE_POSITION2 + _lookupNumber])); \
    }

/* Generate some lookup tables */
    /* drop the RHW coord, there must be a nicer way of doing this. */
    sd->u.s.position.dwType  = min(D3DDECLTYPE_FLOAT3, sd->u.s.position.dwType);
    sd->u.s.position2.dwType = min(D3DDECLTYPE_FLOAT3, sd->u.s.position.dwType);

    LOAD_NUMBERED_ARRAY(blendWeights,BLENDWEIGHT);
    LOAD_NUMBERED_ARRAY(blendMatrixIndices,BLENDINDICES);
    LOAD_NUMBERED_ARRAY(position,POSITION);
    LOAD_NUMBERED_ARRAY(normal,NORMAL);
    LOAD_NUMBERED_ARRAY(pSize,PSIZE);
    LOAD_NUMBERED_ARRAY(diffuse,DIFFUSE);
    LOAD_NUMBERED_ARRAY(specular,SPECULAR);
    LOAD_NUMBERED_ARRAY(texCoords[0],TEXCOORD0);
    LOAD_NUMBERED_ARRAY(texCoords[1],TEXCOORD1);
    LOAD_NUMBERED_ARRAY(texCoords[2],TEXCOORD2);
    LOAD_NUMBERED_ARRAY(texCoords[3],TEXCOORD3);
    LOAD_NUMBERED_ARRAY(texCoords[4],TEXCOORD4);
    LOAD_NUMBERED_ARRAY(texCoords[5],TEXCOORD5);
    LOAD_NUMBERED_ARRAY(texCoords[6],TEXCOORD6);
    LOAD_NUMBERED_ARRAY(texCoords[7],TEXCOORD7);
#if 0   /* TODO: Samplers may allow for more texture coords */
    LOAD_NUMBERED_ARRAY(texCoords[8],TEXCOORD8);
    LOAD_NUMBERED_ARRAY(texCoords[9],TEXCOORD9);
    LOAD_NUMBERED_ARRAY(texCoords[10],TEXCOORD10);
    LOAD_NUMBERED_ARRAY(texCoords[11],TEXCOORD11);
    LOAD_NUMBERED_ARRAY(texCoords[12],TEXCOORD12);
    LOAD_NUMBERED_ARRAY(texCoords[13],TEXCOORD13);
    LOAD_NUMBERED_ARRAY(texCoords[14],TEXCOORD14);
    LOAD_NUMBERED_ARRAY(texCoords[15],TEXCOORD15);
#endif
    LOAD_NUMBERED_ARRAY(position,POSITIONT);
    /* d3d9 types */
    LOAD_NUMBERED_ARRAY(tangent,TANGENT);
    LOAD_NUMBERED_ARRAY(binormal,BINORMAL);
    LOAD_NUMBERED_ARRAY(tessFactor,TESSFACTOR);
    LOAD_NUMBERED_ARRAY(position2,POSITION2);
    /* there can be lots of position arrays */
    LOAD_NUMBERED_POSITION_ARRAY(0);
    LOAD_NUMBERED_POSITION_ARRAY(1);
    LOAD_NUMBERED_POSITION_ARRAY(2);
    LOAD_NUMBERED_POSITION_ARRAY(3);
    LOAD_NUMBERED_POSITION_ARRAY(4);
    LOAD_NUMBERED_POSITION_ARRAY(5);
    LOAD_NUMBERED_POSITION_ARRAY(6);
    LOAD_NUMBERED_POSITION_ARRAY(7);
    LOAD_NUMBERED_ARRAY(position2,POSITIONT2);
    LOAD_NUMBERED_ARRAY(normal2,NORMAL2);
    LOAD_NUMBERED_ARRAY(fog,FOG);
    LOAD_NUMBERED_ARRAY(depth,DEPTH);
    LOAD_NUMBERED_ARRAY(sample,SAMPLE);

#undef LOAD_NUMBERED_ARRAY
}

static void loadVertexData(IWineD3DDevice *iface, WineDirect3DVertexStridedData *sd) {
    unsigned int textureNo   = 0;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("Using fast vertex array code\n");
    /* Blend Data ---------------------------------------------- */
    if ((sd->u.s.blendWeights.lpData != NULL) ||
        (sd->u.s.blendMatrixIndices.lpData != NULL)) {


        if (GL_SUPPORT(ARB_VERTEX_BLEND)) {

#if 1
            glEnableClientState(GL_WEIGHT_ARRAY_ARB);
            checkGLcall("glEnableClientState(GL_WEIGHT_ARRAY_ARB)");
#endif

            TRACE("Blend %d %p %ld\n", WINED3D_ATR_SIZE(blendWeights), sd->u.s.blendWeights.lpData, sd->u.s.blendWeights.dwStride);
            /* FIXME("TODO\n");*/
            /* Note dwType == float3 or float4 == 2 or 3 */

#if 0
            /* with this on, the normals appear to be being modified,
               but the vertices aren't being translated as they should be
               Maybe the world matrix aren't being setup properly? */
            glVertexBlendARB(WINED3D_ATR_SIZE(blendWeights) + 1);
#endif


            VTRACE(("glWeightPointerARB(%d, GL_FLOAT, %ld, %p)\n",
                WINED3D_ATR_SIZE(blendWeights) ,
                sd->u.s.blendWeights.dwStride,
                sd->u.s.blendWeights.lpData));

            GL_EXTCALL(glWeightPointerARB)(WINED3D_ATR_SIZE(blendWeights), WINED3D_ATR_GLTYPE(blendWeights),
                            sd->u.s.blendWeights.dwStride,
                            sd->u.s.blendWeights.lpData);

            checkGLcall("glWeightPointerARB");

            if(sd->u.s.blendMatrixIndices.lpData != NULL){
                static BOOL showfixme = TRUE;
                if(showfixme){
                    FIXME("blendMatrixIndices support\n");
                    showfixme = FALSE;
                }
            }



        } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
            /* FIXME("TODO\n");*/
#if 0

            GL_EXTCALL(glVertexWeightPointerEXT)(WINED3D_ATR_SIZE(blendWeights), WINED3D_ATR_GLTYPE(blendWeights),
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
    if (sd->u.s.fog.lpData != NULL) {
        /* TODO: fog*/
    if (GL_SUPPORT(EXT_FOG_COORD) {
             glEnableClientState(GL_FOG_COORD_EXT);
            (GL_EXTCALL)(FogCoordPointerEXT)(WINED3D_ATR_GLTYPE(fog),
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
             glDisableClientState(GL_FOG_COORD_EXT);
        }
    }
#endif

#if 0 /* tangents  ----------------------------------------------*/
    if (sd->u.s.tangent.lpData != NULL || sd->u.s.binormal.lpData != NULL) {
        /* TODO: tangents*/
        if (GL_SUPPORT(EXT_COORDINATE_FRAME) {
            if (sd->u.s.tangent.lpData != NULL) {
                glEnable(GL_TANGENT_ARRAY_EXT);
                (GL_EXTCALL)(TangentPointerEXT)(WINED3D_ATR_GLTYPE(tangent),
                            sd->u.s.tangent.dwStride,
                            sd->u.s.tangent.lpData);
            } else {
                    glDisable(GL_TANGENT_ARRAY_EXT);
            }
            if (sd->u.s.binormal.lpData != NULL) {
                    glEnable(GL_BINORMAL_ARRAY_EXT);
                    (GL_EXTCALL)(BinormalPointerEXT)(WINED3D_ATR_GLTYPE(binormal),
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
    if (sd->u.s.pSize.lpData != NULL) {

        /* no such functionality in the fixed function GL pipeline */
        TRACE("Cannot change ptSize here in openGl\n");
        /* TODO: Implement this function in using shaders if they are available */

    }

    /* Vertex Pointers -----------------------------------------*/
    if (sd->u.s.position.lpData != NULL) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glVertexPointer(%ld, GL_FLOAT, %ld, %p)\n",
                sd->u.s.position.dwStride,
                sd->u.s.position.dwType + 1,
                sd->u.s.position.lpData));

        /* min(WINED3D_ATR_SIZE(position),3) to Disable RHW mode as 'w' coord
           handling for rhw mode should not impact screen position whereas in GL it does.
           This may  result in very slightly distored textures in rhw mode, but
           a very minimal different. There's always the other option of
           fixing the view matrix to prevent w from having any effect  */
        glVertexPointer(3 /* min(WINED3D_ATR_SIZE(position),3) */, WINED3D_ATR_GLTYPE(position),
                        sd->u.s.position.dwStride, sd->u.s.position.lpData);
        checkGLcall("glVertexPointer(...)");
        glEnableClientState(GL_VERTEX_ARRAY);
        checkGLcall("glEnableClientState(GL_VERTEX_ARRAY)");

    } else {
        glDisableClientState(GL_VERTEX_ARRAY);
        checkGLcall("glDisableClientState(GL_VERTEX_ARRAY)");
    }

    /* Normals -------------------------------------------------*/
    if (sd->u.s.normal.lpData != NULL) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glNormalPointer(GL_FLOAT, %ld, %p)\n",
                sd->u.s.normal.dwStride,
                sd->u.s.normal.lpData));
        glNormalPointer(WINED3D_ATR_GLTYPE(normal),
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

    if (sd->u.s.diffuse.lpData != NULL) {
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glColorPointer(4, GL_UNSIGNED_BYTE, %ld, %p)\n",
                sd->u.s.diffuse.dwStride,
                sd->u.s.diffuse.lpData));

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
    if (sd->u.s.specular.lpData != NULL) {
        TRACE("setting specular colour\n");
        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glSecondaryColorPointer(4, GL_UNSIGNED_BYTE, %ld, %p)\n",
                sd->u.s.specular.dwStride,
                sd->u.s.specular.lpData));
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
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

    for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {
        /* The code below uses glClientActiveTexture and glMultiTexCoord* which are all part of the GL_ARB_multitexture extension. */
        /* Abort if we don't support the extension. */
        if (!GL_SUPPORT(ARB_MULTITEXTURE)) {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
            continue;
        }

        /* Select the correct texture stage */
        GL_EXTCALL(glClientActiveTextureARB(GL_TEXTURE0_ARB + textureNo));
        if (This->stateBlock->textures[textureNo] != NULL) {
            int coordIdx = This->stateBlock->textureState[textureNo][D3DTSS_TEXCOORDINDEX];
            TRACE("Setting up texture %u, cordindx %u, data %p\n", textureNo, coordIdx, sd->u.s.texCoords[coordIdx].lpData);

            if (coordIdx >= MAX_TEXTURES) {
                VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + textureNo, 0, 0, 0, 1));

            } else if (sd->u.s.texCoords[coordIdx].lpData == NULL) {
                VTRACE(("Bound texture but no texture coordinates supplied, so skipping\n"));
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                GL_EXTCALL(glMultiTexCoord4fARB(GL_TEXTURE0_ARB + textureNo, 0, 0, 0, 1));

            } else {

                /* The coords to supply depend completely on the fvf / vertex shader */
                glTexCoordPointer(WINED3D_ATR_SIZE(texCoords[coordIdx]), WINED3D_ATR_GLTYPE(texCoords[coordIdx]), sd->u.s.texCoords[coordIdx].dwStride, sd->u.s.texCoords[coordIdx].lpData);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            }

        } else {
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
            if (sd->u.s.position.dwType == D3DDECLTYPE_FLOAT4) {
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
        for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {

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
                    continue;
                } else if (coordIdx < 0) {
                    FIXME("tex: %d - Coord index %d is less than zero, expect a crash.\n", textureNo, coordIdx);
                    continue;
                }

                ptrToCoords = (float *)(sd->u.s.texCoords[coordIdx].lpData + (SkipnStrides * sd->u.s.texCoords[coordIdx].dwStride));
                if (sd->u.s.texCoords[coordIdx].lpData == NULL) {
                    TRACE("tex: %d - Skipping tex coords, as no data supplied\n", textureNo);
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
                            GL_EXTCALL(glMultiTexCoord1fARB(textureNo, s));
                        } else {
                            glTexCoord1f(s);
                        }
                        break;
                    case D3DTTFF_COUNT2:
                        VTRACE(("tex:%d, s=%f, t=%f\n", textureNo, s, t));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord2fARB(textureNo, s, t));
                        } else {
                            glTexCoord2f(s, t);
                        }
                        break;
                    case D3DTTFF_COUNT3:
                        VTRACE(("tex:%d, s=%f, t=%f, r=%f\n", textureNo, s, t, r));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord3fARB(textureNo, s, t, r));
                        } else {
                            glTexCoord3f(s, t, r);
                        }
                        break;
                    case D3DTTFF_COUNT4:
                        VTRACE(("tex:%d, s=%f, t=%f, r=%f, q=%f\n", textureNo, s, t, r, q));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                            GL_EXTCALL(glMultiTexCoord4fARB(textureNo, s, t, r, q));
                        } else {
                            glTexCoord4f(s, t, r, q);
                        }
                        break;
                    default:
                        FIXME("Should not get here as coordsToUse is two bits only (%x)!\n", coordsToUse);
                    }
                }
            }
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

/** 
 * Loads floating point constants (aka uniforms) into the currently set GLSL program.
 * When @constants_set == NULL, it will load all the constants.
 */
void drawPrimLoadConstantsGLSL_F(IWineD3DDevice* iface,
                                 unsigned max_constants,
                                 float* constants,
                                 BOOL* constants_set) {
    
    IWineD3DDeviceImpl* This = (IWineD3DDeviceImpl *)iface;
    GLhandleARB programId = This->stateBlock->shaderPrgId;
    GLhandleARB tmp_loc;
    int i;
    char tmp_name[7];
    
    if (programId == 0) {
        /* No GLSL program set - nothing to do. */
        return;
    }
    
    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {
            TRACE_(d3d_shader)("Loading constants %i: %f, %f, %f, %f\n",
                   i, constants[i*4], constants[i*4+1], constants[i*4+2], constants[i*4+3]);

            /* TODO: Benchmark and see if it would be beneficial to store the 
             * locations of the constants to avoid looking up each time */
            snprintf(tmp_name, sizeof(tmp_name), "C[%i]", i);
            tmp_loc = GL_EXTCALL(glGetUniformLocationARB(programId, tmp_name));
            if (tmp_loc != -1) {
                /* We found this uniform name in the program - go ahead and send the data */
                GL_EXTCALL(glUniform4fvARB(tmp_loc, 1, &constants[i*4]));
                checkGLcall("glUniform4fvARB");
            }
        }
    }
}

/** 
 * Loads floating point constants into the currently set ARB_vertex/fragment_program.
 * When @constants_set == NULL, it will load all the constants.
 *  
 * @target_type should be either GL_VERTEX_PROGRAM_ARB (for vertex shaders)
 *  or GL_FRAGMENT_PROGRAM_ARB (for pixel shaders)
 */
void drawPrimLoadConstantsARB_F(IWineD3DDevice* iface,
                                GLuint target_type,
                                unsigned max_constants,
                                float* constants,
                                BOOL* constants_set) {
    
    IWineD3DDeviceImpl* This = (IWineD3DDeviceImpl *)iface;
    int i;
    
    for (i=0; i<max_constants; ++i) {
        if (NULL == constants_set || constants_set[i]) {
            TRACE_(d3d_shader)("Loading constants %i: %f, %f, %f, %f\n",
                    i, constants[i*4], constants[i*4+1], constants[i*4+2], constants[i*4+3]);

            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, &constants[i*4]));
            checkGLcall("glProgramEnvParameter4fvARB");
        }
    }
}
 
/* Load the constants/uniforms which were passed by the application into either GLSL or ARB shader programs. */
void drawPrimLoadConstants(IWineD3DDevice *iface,
                           BOOL useVertexShader,
                           BOOL usePixelShader) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexShaderImpl *vshader = (IWineD3DVertexShaderImpl*) This->stateBlock->vertexShader;

    if (wined3d_settings.shader_mode == SHADER_GLSL) {
        
        if (useVertexShader) {
            IWineD3DVertexDeclarationImpl* vertexDeclaration =
                (IWineD3DVertexDeclarationImpl*) vshader->vertexDeclaration;

            if (NULL != vertexDeclaration && NULL != vertexDeclaration->constants) {
                /* Load DirectX 8 float constants/uniforms for vertex shader */
                drawPrimLoadConstantsGLSL_F(iface, WINED3D_VSHADER_MAX_CONSTANTS,
                                            vertexDeclaration->constants, NULL);
            }

            /* Load DirectX 9 float constants/uniforms for vertex shader */
            drawPrimLoadConstantsGLSL_F(iface, WINED3D_VSHADER_MAX_CONSTANTS,
                                        This->stateBlock->vertexShaderConstantF, 
                                        This->stateBlock->set.vertexShaderConstantsF);

            /* TODO: Load boolean & integer constants for vertex shader */
        }
        if (usePixelShader) {

            /* Load DirectX 9 float constants/uniforms for pixel shader */
            drawPrimLoadConstantsGLSL_F(iface, WINED3D_PSHADER_MAX_CONSTANTS,
                                        This->stateBlock->pixelShaderConstantF,
                                        This->stateBlock->set.pixelShaderConstantsF);

            /* TODO: Load boolean & integer constants for pixel shader */
        }
    } else if (wined3d_settings.shader_mode == SHADER_ARB) {
        
        /* We only support float constants in ARB at the moment, so don't 
         * worry about the Integers or Booleans */
        
        if (useVertexShader) {
            IWineD3DVertexDeclarationImpl* vertexDeclaration = 
                (IWineD3DVertexDeclarationImpl*) vshader->vertexDeclaration;

            if (NULL != vertexDeclaration && NULL != vertexDeclaration->constants) {
                /* Load DirectX 8 float constants for vertex shader */
                drawPrimLoadConstantsARB_F(iface, GL_VERTEX_PROGRAM_ARB, WINED3D_VSHADER_MAX_CONSTANTS,
                                           vertexDeclaration->constants, NULL);
            }

            /* Load DirectX 9 float constants for vertex shader */
            drawPrimLoadConstantsARB_F(iface, GL_VERTEX_PROGRAM_ARB, WINED3D_VSHADER_MAX_CONSTANTS,
                                       This->stateBlock->vertexShaderConstantF,
                                       This->stateBlock->set.vertexShaderConstantsF);
        }
        if (usePixelShader) {

            /* Load DirectX 9 float constants for pixel shader */
            drawPrimLoadConstantsARB_F(iface, GL_FRAGMENT_PROGRAM_ARB, WINED3D_PSHADER_MAX_CONSTANTS,
                                       This->stateBlock->pixelShaderConstantF,
                                       This->stateBlock->set.pixelShaderConstantsF);
        }
    }
}
   
void inline  drawPrimitiveDrawStrided(IWineD3DDevice *iface, BOOL useVertexShaderFunction, BOOL usePixelShaderFunction, int useHW, WineDirect3DVertexStridedData *dataLocations,
UINT numberOfvertices, UINT numberOfIndicies, GLenum glPrimType, const void *idxData, short idxSize, int minIndex, long StartIdx) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Now draw the graphics to the screen */
    if  (FALSE /* disable software vs for now */ && useVertexShaderFunction && !useHW) {
        FIXME("drawing using software vertex shaders (line %d)\n", __LINE__);
        /* Ideally, we should have software FV and hardware VS, possibly
           depending on the device type? */
#if 0 /* TODO: vertex and pixel shaders */
        drawStridedSoftwareVS(iface, dataLocations, PrimitiveType, NumPrimitives,
                    idxData, idxSize, minIndex, StartIdx);
#endif

    } else {

        /* TODO: Work out if fixup are required at all (this can be a flag against the vertex declaration) */
        int startStride = idxData == NULL ? 0 : idxData == (void *) -1 ? 0 :(idxSize == 2 ? *(((const short *) idxData) + StartIdx) : *((const int *) idxData) + StartIdx);
        int endStride = startStride;
        TRACE("begin Start stride %d, end stride %d, number of indices%d, number of vertices%d\n", startStride, endStride, numberOfIndicies, numberOfvertices);

#if 0 /* TODO: Vertex fixups (diffuse and specular) */
        if (idxData != NULL) { /* index data isn't linear, so lookup the real start and end strides */
            int t;
            if (idxSize == 2) {
                unsigned short *index = (unsigned short *)idxData;
                index += StartIdx;
                for (t = 0 ; t < numberOfIndicies; t++) {
                    if (startStride >  *index)
                        startStride = *index;
                    if (endStride < *index)
                        endStride = *index;
                    index++;
                }
            } else {  /* idxSize == 4 */
                unsigned int *index = (unsigned int *)idxData;
                index += StartIdx;
                for (t = 0 ; t < numberOfIndicies; t++) {
                    if (startStride > *index)
                        startStride = *index;
                    if (endStride < *index)
                        endStride = *index;
                    index++;
                }
            }
        } else {
            endStride += numberOfvertices -1;
        }
#endif
        TRACE("end Start stride %d, end stride %d, number of indices%d, number of vertices%d\n", startStride, endStride, numberOfIndicies, numberOfvertices);
        /* pre-transform verticex */
        /* TODO: Caching, VBO's etc.. */

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

#if 0/* TODO: Vertex fixups (diffuse and specular) */
        fixupVertices(This, dataLocations, &transformedDataLocations, 1 + endStride - startStride, startStride);
#endif

        /* If the only vertex data used by the shader is supported by OpenGL then*/
        if (!useVertexShaderFunction &&
             dataLocations->u.s.pSize.lpData == NULL &&
             dataLocations->u.s.diffuse.lpData == NULL &&
             dataLocations->u.s.specular.lpData == NULL) {

            /* Load the vertex data using named arrays */
            TRACE("(%p) Loading vertex data\n", This);
            loadVertexData(iface, dataLocations);

        } else if(useVertexShaderFunction) {

            /* load the array data using ordinal mapping */
            loadNumberedArrays(iface, dataLocations, 
                ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->arrayUsageMap);

        } else { /* If this happens we must drawStridedSlow later on */ 
		TRACE("Not loading vertex data\n");
        }

        TRACE("Loaded arrays\n");

        /* Bind the correct GLSL shader program based on the currently set vertex & pixel shaders. */
        if (wined3d_settings.shader_mode == SHADER_GLSL) {
            set_glsl_shader_program(iface);
            /* Start using this program ID (if it's 0, there is no shader program to use, so 
             * glUseProgramObjectARB(0) will disable the use of any shaders) */
            if (This->stateBlock->shaderPrgId) {
                TRACE_(d3d_shader)("Using GLSL program %u\n", This->stateBlock->shaderPrgId);
            }
            GL_EXTCALL(glUseProgramObjectARB(This->stateBlock->shaderPrgId));
            checkGLcall("glUseProgramObjectARB");
        }
        
        if (useVertexShaderFunction) {

            TRACE("Using vertex shader\n");

            if (wined3d_settings.shader_mode == SHADER_ARB) {
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

            if (wined3d_settings.shader_mode == SHADER_ARB) {
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
        drawPrimLoadConstants(iface, useVertexShaderFunction, usePixelShaderFunction);

        /* DirectX colours are in a different format to opengl colours
         * so if diffuse or specular are used then we need to use drawStridedSlow
         * to correct the colours */
        if (!useVertexShaderFunction &&
              ((dataLocations->u.s.pSize.lpData        != NULL)
           || (dataLocations->u.s.diffuse.lpData      != NULL)
           || (dataLocations->u.s.specular.lpData     != NULL))) {
            /* TODO: replace drawStridedSlow with vertex fixups */

            drawStridedSlow(iface, dataLocations, numberOfIndicies, glPrimType,
                            idxData, idxSize, minIndex,  StartIdx);

        } else {
            /* OpenGL can manage everything in hardware so we can use drawStridedFast */
            drawStridedFast(iface, numberOfIndicies, glPrimType,
                idxData, idxSize, minIndex, StartIdx);
        }

        /* Cleanup vertex program */
        if (useVertexShaderFunction) {
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

            if (wined3d_settings.shader_mode == SHADER_ARB)
                glDisable(GL_VERTEX_PROGRAM_ARB);
        }

        /* Cleanup fragment program */
        if (usePixelShaderFunction && wined3d_settings.shader_mode == SHADER_ARB) {
            glDisable(GL_FRAGMENT_PROGRAM_ARB);
        }
    }
}

void inline drawPrimitiveTraceDataLocations(WineDirect3DVertexStridedData *dataLocations,DWORD fvf) {

    /* Dump out what parts we have supplied */
    TRACE("Strided Data (from FVF/VS): %lx\n", fvf);
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

/* uploads textures and setup texture states ready for rendering */
void inline drawPrimitiveUploadTextures(IWineD3DDeviceImpl* This) {

    unsigned int i;
/**
* OK, here we clear down any old junk iect in the context
* enable the new texture and apply any state changes:
*
* Loop through all textures
* select texture unit
* if there is a texture bound to that unit then..
* disable all textures types on that unit
* enable and bind the texture that is bound to that unit.
* otherwise disable all texture types on that unit.
**/
    /* upload the textures */
    for (i = 0; i< GL_LIMITS(textures); ++i) {
        /* Bind the texture to the stage here */
        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
            checkGLcall("glActiveTextureARB");
        } else if (0 < i) {
            /* This isn't so much a warn as a message to the user about lack of hardware support */
            WARN("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
        }

        /* don't bother with textures that have a colorop of disable */
        if (This->stateBlock->textureState[i][WINED3DTSS_COLOROP] != D3DTOP_DISABLE) {
            if (This->stateBlock->textures[i] != NULL) {

                glDisable(GL_TEXTURE_1D);
                This->stateBlock->textureDimensions[i] = IWineD3DBaseTexture_GetTextureDimensions(This->stateBlock->textures[i]);
                /* disable all texture states that aren't the selected textures' dimension */
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
                  /* Load up the texture now */
                IWineD3DBaseTexture_PreLoad((IWineD3DBaseTexture *) This->stateBlock->textures[i]);
                IWineD3DDevice_SetupTextureStates((IWineD3DDevice *)This, i, REAPPLY_ALPHAOP);
                /* this is a stub function representing the state blocks
                 * being separated here we are only updating the texture
                 * state changes, other objects and units get updated when
                 * they change (or need to be updated), e.g. states that
                 * relate to a context member line the texture unit are
                 * only updated when the context needs updating
                 */
                /* Tell the abse texture to sync it's states */
                IWineD3DBaseTexture_ApplyStateChanges(This->stateBlock->textures[i], This->stateBlock->textureState[i], This->stateBlock->samplerState[i]);

              }
            /* Bind a default texture if no texture has been set, but colour-op is enabled */
            else {
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_TEXTURE_3D);
                glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                glEnable(GL_TEXTURE_1D);
                This->stateBlock->textureDimensions[i] = GL_TEXTURE_1D;
                glBindTexture(GL_TEXTURE_1D, This->dummyTextureName[i]);
            }
/** these ops apply to the texture unit, so they are preserved between texture changes, but for now brute force and reapply all
        dx9_1pass_emboss_bump_mapping and dx9_2pass_emboss_bump_mapping are good texts to make sure the states are being applied when needed **/
            set_tex_op((IWineD3DDevice *)This, FALSE, i, This->stateBlock->textureState[i][WINED3DTSS_COLOROP],
                        This->stateBlock->textureState[i][WINED3DTSS_COLORARG1],
                        This->stateBlock->textureState[i][WINED3DTSS_COLORARG2],
                        This->stateBlock->textureState[i][WINED3DTSS_COLORARG0]);
            /* alphaop */
            set_tex_op((IWineD3DDevice *)This, TRUE, i, This->stateBlock->textureState[i][WINED3DTSS_ALPHAOP],
                        This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG1],
                        This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG2],
                        This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG0]);
        } else {

            /* no colorop so disable all the texture states */
            glDisable(GL_TEXTURE_1D);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_3D);
            glDisable(GL_TEXTURE_CUBE_MAP_ARB);
          }

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

    BOOL                          rc = FALSE;
    DWORD                         fvf = 0;
    IWineD3DDeviceImpl           *This = (IWineD3DDeviceImpl *)iface;
    BOOL                          useVertexShaderFunction = FALSE;
    BOOL                          usePixelShaderFunction = FALSE;
    BOOL                          isLightingOn = FALSE;
    WineDirect3DVertexStridedData *dataLocations;
    IWineD3DSwapChainImpl         *swapchain;
    int                           useHW = FALSE, i;

    if (This->stateBlock->vertexShader != NULL && wined3d_settings.vs_mode != VS_NONE 
            &&((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.function != NULL
            && GL_SUPPORT(ARB_VERTEX_PROGRAM)) {
        useVertexShaderFunction = TRUE;
    } else {
        useVertexShaderFunction = FALSE;
    }

    if (wined3d_settings.ps_mode != PS_NONE && GL_SUPPORT(ARB_FRAGMENT_PROGRAM)
            && This->stateBlock->pixelShader
            && ((IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader)->baseShader.function) {
        usePixelShaderFunction = TRUE;
    }

    if (This->stateBlock->vertexDecl == NULL) {
        /* Work out what the FVF should look like */
        rc = initializeFVF(iface, &fvf);
        if (rc) return;
    } else {
        TRACE("(%p) : using vertex declaration %p\n", iface, This->stateBlock->vertexDecl);
    }

    /* Invalidate the back buffer memory so LockRect will read it the next time */
    for(i = 0; i < IWineD3DDevice_GetNumberOfSwapChains(iface); i++) {
        IWineD3DDevice_GetSwapChain(iface, i, (IWineD3DSwapChain **) &swapchain);
        if(swapchain) {
            if(swapchain->backBuffer) ((IWineD3DSurfaceImpl *) swapchain->backBuffer)->Flags |= SFLAG_GLDIRTY;
            IWineD3DSwapChain_Release( (IWineD3DSwapChain *) swapchain);
        }
    }

    /* Ok, we will be updating the screen from here onwards so grab the lock */
    ENTER_GL();

    /* convert the FVF or vertexDeclaration into a strided stream (this should be done when the fvf or declaration is created) */

    if(DrawPrimStrideData) {
        TRACE("================ Strided Input ===================\n");
        dataLocations = DrawPrimStrideData;
    }
    else if (This->stateBlock->vertexDecl != NULL || (useVertexShaderFunction  && NULL != ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->vertexDeclaration)) {

        TRACE("================ Vertex Declaration  ===================\n");
        dataLocations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dataLocations));
        if(!dataLocations) {
            ERR("Out of memory!\n");
            return;
        }
        primitiveDeclarationConvertToStridedData(iface, useVertexShaderFunction, dataLocations, StartVertexIndex, &fvf);

    } else {
        TRACE("================ FVF ===================\n");
        dataLocations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dataLocations));
        if(!dataLocations) {
            ERR("Out of memory!\n");
            return;
        }
        primitiveConvertToStridedData(iface, dataLocations, StartVertexIndex);
    }

    /* write out some debug information*/
    drawPrimitiveTraceDataLocations(dataLocations, fvf);

    /* Setup transform matrices and sort out */
    if (useHW) {
        /* Lighting is not completely bypassed with ATI drivers although it should be. Mesa is ok from this respect...
        So make sure lighting is disabled. */
        isLightingOn = glIsEnabled(GL_LIGHTING);
        glDisable(GL_LIGHTING);
        checkGLcall("glDisable(GL_LIGHTING);");
        TRACE("Disabled lighting as no normals supplied, old state = %d\n", isLightingOn);
    } else {
        isLightingOn = primitiveInitState(iface,
                                          fvf & D3DFVF_XYZRHW,
                                          !(fvf & D3DFVF_NORMAL),
                                          useVertexShaderFunction);
    }

    /* Now initialize the materials state */
    init_materials(iface, (dataLocations->u.s.diffuse.lpData != NULL));

    drawPrimitiveUploadTextures(This);


    {
        GLenum glPrimType;
        /* Ok, Work out which primitive is requested and how many vertexes that
           will be                                                              */
        UINT calculatedNumberOfindices = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);
#if 0 /* debugging code... just information not an error */
        if(numberOfVertices != 0 && numberOfVertices != calculatedNumberOfindices){
            FIXME("Number of vertices %u and Caculated number of indicies %u differ\n", numberOfVertices, calculatedNumberOfindices);
        }
#endif
        if (numberOfVertices == 0 )
            numberOfVertices = calculatedNumberOfindices;
        drawPrimitiveDrawStrided(iface, useVertexShaderFunction, usePixelShaderFunction, useHW, dataLocations, numberOfVertices, calculatedNumberOfindices, glPrimType, idxData, idxSize, minIndex, StartIdx);
    }

    if(!DrawPrimStrideData) HeapFree(GetProcessHeap(), 0, dataLocations);

    /* If vertex shaders or no normals, restore previous lighting state */
    if (useVertexShaderFunction || !(fvf & D3DFVF_NORMAL)) {
        if (isLightingOn) glEnable(GL_LIGHTING);
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
