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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_draw);
WINE_DECLARE_DEBUG_CHANNEL(d3d_shader);
#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info

#if 0 /* TODO */
extern IDirect3DVertexShaderImpl*            VertexShaders[64];
extern IDirect3DVertexShaderDeclarationImpl* VertexShaderDeclarations[64];
extern IDirect3DPixelShaderImpl*             PixelShaders[64];

#undef GL_VERSION_1_4 /* To be fixed, caused by mesa headers */
#endif

/* Returns bits for what is expected from the fixed function pipeline, and whether
   a vertex shader will be in use. Note the fvf bits returned may be split over
   multiple streams only if the vertex shader was created, otherwise it all relates
   to stream 0                                                                      */
static BOOL initializeFVF(IWineD3DDevice *iface,
                   DWORD *FVFbits,                 /* What to expect in the FVF across all streams */
                   BOOL *useVertexShaderFunction)  /* Should we use the vertex shader              */
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

    if (This->stateBlock->vertexShader == NULL) {

        /* Use this as the FVF */
        *FVFbits = This->stateBlock->fvf;
        *useVertexShaderFunction = FALSE;
        TRACE("FVF explicitally defined, using fixed function pipeline with FVF=%lx\n", *FVFbits);

    } else {

#if 0 /* TODO */
        /* Use created shader */
        IDirect3DVertexShaderImpl* vertex_shader = NULL;
        vertex_shader = VERTEX_SHADER(This->stateBlock->VertexShader);

        if (vertex_shader == NULL) {

            /* Hmm - User pulled figure out of the air? Unlikely, probably a bug */
            ERR("trying to use unitialised vertex shader: %lu\n", This->stateBlock->VertexShader);
            return TRUE;

        } else {

            *FVFbits = This->stateBlock->vertexShaderDecl->allFVF;

            if (vertex_shader->function == NULL) {
                /* No function, so many streams supplied plus FVF definition pre stream */
                *useVertexShaderFunction = FALSE;
                TRACE("vertex shader (%lx) declared without program, using fixed function pipeline with FVF=%lx\n",
                            This->stateBlock->VertexShader, *FVFbits);
            } else {
                /* Vertex shader needs calling */
                *useVertexShaderFunction = TRUE;
                TRACE("vertex shader will be used (unusued FVF=%lx)\n", *FVFbits);
            }
        }
#else
        FIXME("Vertex Shaders not moved into wined3d yet\n");
#endif
    }
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
        *primType = GL_POINTS;
        NumVertexes = NumPrimitives;
        break;

    case D3DPT_LINELIST:
        TRACE("LINES\n");
        *primType = GL_LINES;
        NumVertexes = NumPrimitives * 2;
        break;

    case D3DPT_LINESTRIP:
        TRACE("LINE_STRIP\n");
        *primType = GL_LINE_STRIP;
        NumVertexes = NumPrimitives + 1;
        break;

    case D3DPT_TRIANGLELIST:
        TRACE("TRIANGLES\n");
        *primType = GL_TRIANGLES;
        NumVertexes = NumPrimitives * 3;
        break;

    case D3DPT_TRIANGLESTRIP:
        TRACE("TRIANGLE_STRIP\n");
        *primType = GL_TRIANGLE_STRIP;
        NumVertexes = NumPrimitives + 2;
        break;

    case D3DPT_TRIANGLEFAN:
        TRACE("TRIANGLE_FAN\n");
        *primType = GL_TRIANGLE_FAN;
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
        if (This->stateBlock->renderState[D3DRS_SPECULARENABLE]) {
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

static GLfloat invymat[16]={
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, -1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f};

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

        /* If the last draw was transformed as well, no need to reapply all the matrixes */
        if (!This->last_was_rhw) {

            double X, Y, height, width, minZ, maxZ;
            This->last_was_rhw = TRUE;

            /* Transformed already into viewport coordinates, so we do not need transform
               matrices. Reset all matrices to identity and leave the default matrix in world
               mode.                                                                         */
            glMatrixMode(GL_MODELVIEW);
            checkGLcall("glMatrixMode");
            glLoadIdentity();
            checkGLcall("glLoadIdentity");

            glMatrixMode(GL_PROJECTION);
            checkGLcall("glMatrixMode");
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
            glTranslatef(0.5, 0.5, 0);
            checkGLcall("glTranslatef(0.5, 0.5, 0)");
            if (This->renderUpsideDown) {
                glMultMatrixf(invymat);
                checkGLcall("glMultMatrixf(invymat)");
            }
        }

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
            glTranslatef(1.0/This->stateBlock->viewport.Width, -1.0/This->stateBlock->viewport.Height, 0);
            checkGLcall("glTranslatef (1.0/width, -1.0/height, 0)");

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
            glTranslatef(1.0/This->stateBlock->viewport.Width, -1.0/This->stateBlock->viewport.Height, 0);
            checkGLcall("glTranslatef (1.0/width, -1.0/height, 0)");
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

void primitiveDeclarationConvertToStridedData(IWineD3DDevice *iface, Direct3DVertexStridedData *strided, LONG BaseVertexIndex, DWORD *fvf) {
     /* We need to deal with frequency data!*/

    int           textureNo =0;
    BYTE  *data    = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexDeclarationImpl* vertexDeclaration = (IWineD3DVertexDeclarationImpl*)This->stateBlock->vertexDecl;
    int i;
    D3DVERTEXELEMENT9 *element;
    DWORD stride;
    for(i = 0 ; i < vertexDeclaration->declaration9NumElements -1; i++){

        element = vertexDeclaration->pDeclaration9 + i;
        TRACE("%p Elements %p %d or %d\n", vertexDeclaration->pDeclaration9, element,  i, vertexDeclaration->declaration9NumElements);
        if (This->stateBlock->streamIsUP) {
            TRACE("Stream is up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            data    = (BYTE *)This->stateBlock->streamSource[element->Stream];
        } else {
             TRACE("Stream isn't up %d, %p\n", element->Stream, This->stateBlock->streamSource[element->Stream]);
            data    = ((IWineD3DVertexBufferImpl *)This->stateBlock->streamSource[element->Stream])->resource.allocatedMemory;
        }
        stride  = This->stateBlock->streamStride[element->Stream];
        data += (BaseVertexIndex * stride);
        data += element->Offset;
        /* Why can't I just use a lookup table instead of a switch statment? */
        switch(element->Usage){
        case D3DDECLUSAGE_POSITION:
                switch(element->UsageIndex){
                case 0: /* N-patch */
                    strided->u.s.position.lpData    = data;
                    strided->u.s.position.dwType    = element->Type;
                    strided->u.s.position.dwStride  = stride;
                break;
                case 1: /* tweened see http://www.gamedev.net/reference/articles/article2017.asp */
                    FIXME("Tweened positions\n");
                break;
                }
        break;
        case D3DDECLUSAGE_NORMAL:
                switch(element->UsageIndex){
                case 0: /* N-patch */
                    strided->u.s.normal.lpData    = data;
                    strided->u.s.normal.dwType    = element->Type;
                    strided->u.s.normal.dwStride  = stride;
                break;
                case 1: /* skinning */
                   FIXME("Skinning normals\n");
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
        break;
        case D3DDECLUSAGE_BLENDWEIGHT:
            strided->u.s.blendWeights.lpData        = data;
            strided->u.s.blendWeights.dwType        = element->Type;
            strided->u.s.blendWeights.dwStride      = stride;
        break;
        case D3DDECLUSAGE_PSIZE:
            strided->u.s.pSize.lpData               = data;
            strided->u.s.pSize.dwType               = element->Type;
            strided->u.s.pSize.dwStride             = stride;
        break;
        case D3DDECLUSAGE_COLOR:
        switch(element->UsageIndex){
        case 0:/* diffuse */
            strided->u.s.diffuse.lpData             = data;
            strided->u.s.diffuse.dwType             = element->Type;
            strided->u.s.diffuse.dwStride           = stride;
        break;
        case 1: /* specular */
            strided->u.s.specular.lpData            = data;
            strided->u.s.specular.dwType            = element->Type;
            strided->u.s.specular.dwStride          = stride;
        }

        break;
        case D3DDECLUSAGE_TEXCOORD:
        /* For some odd reason Microsoft decided to sum usage accross all the streams,
        which means we need to do a count and not just use the usage number */

            strided->u.s.texCoords[textureNo].lpData    = data;
            strided->u.s.texCoords[textureNo].dwType    = element->Type;
            strided->u.s.texCoords[textureNo].dwStride  = stride;

        textureNo++;
        break;
        case D3DDECLUSAGE_TANGENT:
        /* Implement tangents and binormals using http://oss.sgi.com/projects/ogl-sample/registry/EXT/coordinate_frame.txt
        this is easy so long as the OpenGL implementation supports it, otherwise drop back to calculating the
        normal using tangents where no normal data has been provided */
#if 0
        strided->u.s.tangent.lpData   = data;
        strided->u.s.tangent.dwType   = element->type;
        strided->u.s.tangent.dsString = stride;
#endif
        TRACE("Tangents\n");
        break;
        case D3DDECLUSAGE_BINORMAL:
        /* Binormals are really bitangents perpendicular to the normal but s-aligned to the tangent, basically they are the vectors of any two lines on the plain at right angles to the normal and at right angles to each other, like the x,y,z axis.
        tangent data makes it easier to perform some calculations (a bit like using 2d graph paper instead of the normal of the piece of paper)
        The only thing they are useful for in fixed function would be working out normals when none are given.
        */
#if 0
        strided->u.s.binormal.lpData   = data;
        strided->u.s.binormal.dwType   = element->type;
        strided->u.s.binormal.dsString = stride;
#endif
        /* Don't bother showing fixme's tangents aren't that interesting */
        TRACE("BI-Normal\n");
        break;
        case D3DDECLUSAGE_TESSFACTOR:
        /* a google for D3DDECLUSAGE_TESSFACTOR turns up a whopping 36 entries, 7 of which are from MSDN.
        */
#if 0
        strided->u.s.tessFacrot.lpData   = data;
        strided->u.s.tessFactor.dwType   = element->type;
        strided->u.s.tessFactor.dsString = stride;
#else
        FIXME("Tess Factor\n");
#endif
        break;
        case D3DDECLUSAGE_POSITIONT:

               switch(element->UsageIndex){
                case 0: /* N-patch */
                    strided->u.s.position.lpData    = data;
                    strided->u.s.position.dwType    = element->Type;
                    strided->u.s.position.dwStride  = stride;
                break;
                case 1: /* skinning */
                        /* see http://rsn.gamedev.net/tutorials/ms3danim.asp
                        http://xface.blogspot.com/2004_08_01_xface_archive.html
                        */
                    FIXME("Skinning positionsT\n");
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
#if 0
        strided->u.s.fog.lpData   = data;
        strided->u.s.fog.dwType   = element->type;
        strided->u.s.fog.dsString = stride;
#else
        FIXME("Fog\n");
#endif
        break;
        case D3DDECLUSAGE_DEPTH:
        FIXME("depth\n");
#if 0
        strided->u.s.depth.lpData   = data;
        strided->u.s.depth.dwType   = element->type;
        strided->u.s.depth.dsString = stride;
#endif

        break;
        case D3DDECLUSAGE_SAMPLE: /* VertexShader textures */
#if 0
        strided->u.s.sample.lpData   = data;
        strided->u.s.sample.dwType   = element->type;
        strided->u.s.sample.dsString = stride;
#endif
        FIXME("depth\n");
        break;
        };

    };

}

static void primitiveConvertToStridedData(IWineD3DDevice *iface, Direct3DVertexStridedData *strided, LONG BaseVertexIndex) {

    short         LoopThroughTo = 0;
    short         nStream;
    BOOL          canDoViaGLPointers = TRUE;
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
    for (nStream=0; nStream<LoopThroughTo; nStream++) {
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
                data    = ((IWineD3DVertexBufferImpl *)This->stateBlock->streamSource[nStream])->resource.allocatedMemory;
            }
        } else {
#if 0 /* TODO: Vertex shader support */
            thisFVF = This->stateBlock->vertexShaderDecl->fvf[nStream];
            data    = ((IDirect3DVertexBuffer8Impl *)This->stateBlock->streamSource[nStream])->allocatedMemory;
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
            TRACE("Setting blend Weights to %p \n", data);
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
            if (coordIdx > 7) {
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

/*
 * Actually draw using the supplied information.
 * Faster GL version using pointers to data, harder to debug though
 * Note does not handle vertex shaders yet
 */
static void drawStridedFast(IWineD3DDevice *iface, Direct3DVertexStridedData *sd,
                     int PrimitiveType, ULONG NumPrimitives,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx) {
    unsigned int textureNo   = 0;
    GLenum       glPrimType  = GL_POINTS;
    int          NumVertexes = NumPrimitives;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("Using fast vertex array code\n");

    /* Vertex Pointers -----------------------------------------*/
    if (sd->u.s.position.lpData != NULL) {

        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glVertexPointer(%ld, GL_FLOAT, %ld, %p)\n",
                sd->u.s.position.dwStride,
                sd->u.s.position.dwType + 1,
                sd->u.s.position.lpData));

        /* Disable RHW mode as 'w' coord handling for rhw mode should
           not impact screen position whereas in GL it does. This may
           result in very slightly distored textures in rhw mode, but
           a very minimal different                                   */
        glVertexPointer(3, GL_FLOAT,  /* RHW: Was 'sd->u.s.position.dwType + 1' */
                        sd->u.s.position.dwStride,
                        sd->u.s.position.lpData);
        checkGLcall("glVertexPointer(...)");
        glEnableClientState(GL_VERTEX_ARRAY);
        checkGLcall("glEnableClientState(GL_VERTEX_ARRAY)");

    } else {

        glDisableClientState(GL_VERTEX_ARRAY);
        checkGLcall("glDisableClientState(GL_VERTEX_ARRAY)");
    }

    /* Blend Data ----------------------------------------------*/
    if ((sd->u.s.blendWeights.lpData != NULL) ||
        (sd->u.s.blendMatrixIndices.lpData != NULL)) {
#if 1 /* Vertex blend support needs to be added */
        if (GL_SUPPORT(ARB_VERTEX_BLEND)) {
            DWORD fvf = (sd->u.s.blendWeights.dwType - D3DDECLTYPE_FLOAT1) + 1;
            int numBlends = ((fvf & D3DFVF_POSITION_MASK) >> 1) - 2 + ((FALSE == (fvf & D3DFVF_LASTBETA_UBYTE4)) ? 0 : -1);

            /* Note dwType == float3 or float4 == 2 or 3 */
            VTRACE(("glWeightPointerARB(%ld, GL_FLOAT, %ld, %p)\n",
                    numBlends,
                    sd->u.s.blendWeights.dwStride,
                    sd->u.s.blendWeights.lpData));
            GL_EXTCALL(glWeightPointerARB)(numBlends, GL_FLOAT,
                                           sd->u.s.blendWeights.dwStride,
                                           sd->u.s.blendWeights.lpData);
            checkGLcall("glWeightPointerARB(...)");
            glEnableClientState(GL_WEIGHT_ARRAY_ARB);
            checkGLcall("glEnableClientState(GL_VERTEX_ARRAY)");
        } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
            /*FIXME("TODO\n");*/
            /*
            GLExtCall(glVertexWeightPointerEXT)(numBlends, GL_FLOAT, skip, curPos);
            checkGLcall("glVertexWeightPointerEXT(numBlends, ...)");
            glEnableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT);
            checkGLcall("glEnableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT)");
            */
        } else {
            FIXME("unsupported blending in openGl\n");
        }
    } else {
        if (GL_SUPPORT(ARB_VERTEX_BLEND)) {
            TRACE("TODO ARB_VERTEX_BLEND\n");
        } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
            TRACE("TODO EXT_VERTEX_WEIGHTING\n");
            /*
            glDisableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT);
            checkGLcall("glDisableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT)");
            */
        }
#else
        /* FIXME: Won't get here as will drop to slow method        */
        FIXME("Blending not supported in fast draw routine\n");
#endif
    }

    /* Normals -------------------------------------------------*/
    if (sd->u.s.normal.lpData != NULL) {

        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glNormalPointer(GL_FLOAT, %ld, %p)\n",
                sd->u.s.normal.dwStride,
                sd->u.s.normal.lpData));
        glNormalPointer(GL_FLOAT,
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

    /* Point Size ----------------------------------------------*/
    if (sd->u.s.pSize.lpData != NULL) {

        /* no such functionality in the fixed function GL pipeline */
        /* FIXME: Won't get here as will drop to slow method        */
        FIXME("Cannot change ptSize here in openGl\n");
    }

    /* Diffuse Colour ------------------------------------------*/
    /*  WARNING: Data here MUST be in RGBA format, so cannot    */
    /*     go directly into fast mode from app pgm, because     */
    /*     directx requires data in BGRA format.                */
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

        /* Select the correct texture stage */
        GLCLIENTACTIVETEXTURE(textureNo);

        /* Query tex coords */
        if (This->stateBlock->textures[textureNo] != NULL) {
            int coordIdx = This->stateBlock->textureState[textureNo][D3DTSS_TEXCOORDINDEX];

            if (!GL_SUPPORT(ARB_MULTITEXTURE) && textureNo > 0) {
                FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                GLMULTITEXCOORD4F(textureNo, 0, 0, 0, 1);
                continue;
            }

            if (coordIdx > 7) {
                VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                GLMULTITEXCOORD4F(textureNo, 0, 0, 0, 1);

            } else if (sd->u.s.texCoords[coordIdx].lpData == NULL) {
                VTRACE(("Bound texture but no texture coordinates supplied, so skipping\n"));
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                GLMULTITEXCOORD4F(textureNo, 0, 0, 0, 1);

            } else {

                /* The coords to supply depend completely on the fvf / vertex shader */
                GLint size;
                GLenum type;

                switch (sd->u.s.texCoords[coordIdx].dwType) {
                case D3DDECLTYPE_FLOAT1: size = 1, type = GL_FLOAT; break;
                case D3DDECLTYPE_FLOAT2: size = 2, type = GL_FLOAT; break;
                case D3DDECLTYPE_FLOAT3: size = 3, type = GL_FLOAT; break;
                case D3DDECLTYPE_FLOAT4: size = 4, type = GL_FLOAT; break;
                case D3DDECLTYPE_SHORT2: size = 2, type = GL_SHORT; break;
                case D3DDECLTYPE_SHORT4: size = 4, type = GL_SHORT; break;
                case D3DDECLTYPE_UBYTE4: size = 4, type = GL_UNSIGNED_BYTE; break;
                default: FIXME("Unrecognized data type %ld\n", sd->u.s.texCoords[coordIdx].dwType);
                      size = 4; type = GL_UNSIGNED_BYTE;
                }

                glTexCoordPointer(size, type, sd->u.s.texCoords[coordIdx].dwStride, sd->u.s.texCoords[coordIdx].lpData);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            }
        } else {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            GLMULTITEXCOORD4F(textureNo, 0, 0, 0, 1);
        }
    }

    /* Ok, Work out which primitive is requested and how many vertexes that
       will be                                                              */
    NumVertexes = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);

    /* Finally do the drawing */
    if (idxData != NULL) {

        TRACE("glElements(%x, %d, %ld, ...)\n", glPrimType, NumVertexes, minIndex);
#if 1  /* FIXME: Want to use DrawRangeElements, but wrong calculation! */
        glDrawElements(glPrimType, NumVertexes, idxSize==2?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,
                      (const char *)idxData+(idxSize * startIdx));
#else
        glDrawRangeElements(glPrimType, minIndex, minIndex+NumVertexes-1, NumVertexes,
                      idxSize==2?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,
                      (const char *)idxData+(idxSize * startIdx));
#endif
        checkGLcall("glDrawRangeElements");

    } else {

        /* Note first is now zero as we shuffled along earlier */
        TRACE("glDrawArrays(%x, 0, %d)\n", glPrimType, NumVertexes);
        glDrawArrays(glPrimType, 0, NumVertexes);
        checkGLcall("glDrawArrays");

    }
}

/*
 * Actually draw using the supplied information.
 * Slower GL version which extracts info about each vertex in turn
 */
static void drawStridedSlow(IWineD3DDevice *iface, Direct3DVertexStridedData *sd,
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

    /* Ok, Work out which primitive is requested and how many vertexes that will be */
    NumVertexes = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);

    /* Start drawing in GL */
    VTRACE(("glBegin(%x)\n", glPrimType));
    glBegin(glPrimType);

    /* For each primitive */
    for (vx_index = 0; vx_index < NumVertexes; vx_index++) {

        /* Initialize diffuse color */
        diffuseColor = 0xFFFFFFFF;

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
            /*float *ptrToCoords = (float *)(sd->u.s.blendWeights.lpData + (SkipnStrides * sd->u.s.blendWeights.dwStride));*/
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
                float *ptrToCoords = (float *)(sd->u.s.texCoords[coordIdx].lpData + (SkipnStrides * sd->u.s.texCoords[coordIdx].dwStride));
                float  s = 0.0, t = 0.0, r = 0.0, q = 0.0;

                if (coordIdx > 7) {
                    VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
                    continue;
                } else if (sd->u.s.texCoords[coordIdx].lpData == NULL) {
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

                    /* crude support for non-power2 textures */
                    if(((IWineD3DSurfaceImpl *) ((IWineD3DTextureImpl *)This->stateBlock->textures[textureNo])->surfaces[0])->nonpow2){
                        t *= ((IWineD3DSurfaceImpl *)((IWineD3DTextureImpl *)This->stateBlock->textures[textureNo])->surfaces[0])->pow2scalingFactorY;
                        s *= ((IWineD3DSurfaceImpl *)((IWineD3DTextureImpl *)This->stateBlock->textures[textureNo])->surfaces[0])->pow2scalingFactorX;
                    }

                    switch (coordsToUse) {   /* Supply the provided texture coords */
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
                        FIXME("Should not get here as coordsToUse is two bits only (%x)!\n", coordsToUse);
                    }
                }
            }
        } /* End of textures */

        /* Diffuse -------------------------------- */
        if (sd->u.s.diffuse.lpData != NULL) {
            glColor4ub((diffuseColor >> 16) & 0xFF,
                       (diffuseColor >>  8) & 0xFF,
                       (diffuseColor >>  0) & 0xFF,
                       (diffuseColor >> 24) & 0xFF);
            VTRACE(("glColor4f: r,g,b,a=%f,%f,%f,%f\n",
                    ((diffuseColor >> 16) & 0xFF) / 255.0f,
                    ((diffuseColor >>  8) & 0xFF) / 255.0f,
                    ((diffuseColor >>  0) & 0xFF) / 255.0f,
                    ((diffuseColor >> 24) & 0xFF) / 255.0f));
        } else {
            if (vx_index == 0) glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }

        /* Specular ------------------------------- */
        if (sd->u.s.diffuse.lpData != NULL) {
            VTRACE(("glSecondaryColor4ub: r,g,b=%f,%f,%f\n",
                    ((specularColor >> 16) & 0xFF) / 255.0f,
                    ((specularColor >>  8) & 0xFF) / 255.0f,
                    ((specularColor >>  0) & 0xFF) / 255.0f));
            if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
                GL_EXTCALL(glSecondaryColor3ubEXT)(
                           (specularColor >> 16) & 0xFF,
                           (specularColor >>  8) & 0xFF,
                           (specularColor >>  0) & 0xFF);
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
            if (1.0f == rhw || ((rhw < 0.0001f) && (rhw > -0.0001f))) {
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
            SkipnStrides += 1;
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
void drawStridedSoftwareVS(IWineD3DDevice *iface, Direct3DVertexStridedData *sd,
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

    IDirect3DVertexShaderImpl* vertex_shader = NULL;

    TRACE("Using slow software vertex shader code\n");

    /* Variable Initialization */
    if (idxData != NULL) {
        if (idxSize == 2) pIdxBufS = (const short *) idxData;
        else pIdxBufL = (const long *) idxData;
    }

    /* Ok, Work out which primitive is requested and how many vertexes that will be */
    NumVertexes = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);

    /* Retrieve the VS information */
    vertex_shader = VERTEX_SHADER(This->stateBlock->VertexShader);

    /* Start drawing in GL */
    VTRACE(("glBegin(%x)\n", glPrimType));
    glBegin(glPrimType);

    /* For each primitive */
    for (vx_index = 0; vx_index < NumVertexes; vx_index++) {

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
        IDirect3DDeviceImpl_FillVertexShaderInputSW(This, vertex_shader, SkipnStrides);

        /* Initialize the output fields to the same defaults as it would normally have */
        memset(&vertex_shader->output, 0, sizeof(VSHADEROUTPUTDATA8));
        vertex_shader->output.oD[0].x = 1.0;
        vertex_shader->output.oD[0].y = 1.0;
        vertex_shader->output.oD[0].z = 1.0;
        vertex_shader->output.oD[0].w = 1.0;

        /* Now execute the vertex shader */
        IDirect3DVertexShaderImpl_ExecuteSW(vertex_shader, &vertex_shader->input, &vertex_shader->output);

        /*
        TRACE_VECTOR(vertex_shader->output.oPos);
        TRACE_VECTOR(vertex_shader->output.oD[0]);
        TRACE_VECTOR(vertex_shader->output.oD[1]);
        TRACE_VECTOR(vertex_shader->output.oT[0]);
        TRACE_VECTOR(vertex_shader->output.oT[1]);
        TRACE_VECTOR(vertex_shader->input.V[0]);
        TRACE_VECTOR(vertex_shader->data->C[0]);
        TRACE_VECTOR(vertex_shader->data->C[1]);
        TRACE_VECTOR(vertex_shader->data->C[2]);
        TRACE_VECTOR(vertex_shader->data->C[3]);
        TRACE_VECTOR(vertex_shader->data->C[4]);
        TRACE_VECTOR(vertex_shader->data->C[5]);
        TRACE_VECTOR(vertex_shader->data->C[6]);
        TRACE_VECTOR(vertex_shader->data->C[7]);
        */

        /* Extract out the output */
        /*FIXME: Fog coords? */
        x = vertex_shader->output.oPos.x;
        y = vertex_shader->output.oPos.y;
        z = vertex_shader->output.oPos.z;
        rhw = vertex_shader->output.oPos.w;
        ptSize = vertex_shader->output.oPts.x; /* Fixme - Is this right? */

        /** Update textures coords using vertex_shader->output.oT[0->7] */
        memset(texcoords, 0x00, sizeof(texcoords));
        memset(numcoords, 0x00, sizeof(numcoords));
        for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {
            if (This->stateBlock->textures[textureNo] != NULL) {
               texcoords[textureNo].x = vertex_shader->output.oT[textureNo].x;
               texcoords[textureNo].y = vertex_shader->output.oT[textureNo].y;
               texcoords[textureNo].z = vertex_shader->output.oT[textureNo].z;
               texcoords[textureNo].w = vertex_shader->output.oT[textureNo].w;
               if (This->stateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE) {
                   numcoords[textureNo]    = This->stateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & ~D3DTTFF_PROJECTED;
               } else {
                   switch (IDirect3DBaseTexture8Impl_GetType((LPDIRECT3DBASETEXTURE8) This->stateBlock->textures[textureNo])) {
                   case D3DRTYPE_TEXTURE:       numcoords[textureNo] = 2; break;
                   case D3DRTYPE_VOLUMETEXTURE: numcoords[textureNo] = 3; break;
                   default:                     numcoords[textureNo] = 4;
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
                    TRUE, (float*) &vertex_shader->output.oD[0],
                    TRUE, (float*) &vertex_shader->output.oD[1],
                    FALSE, ptSize,         /* FIXME: Change back when supported */
                    texcoords, numcoords);

        /* For non indexed mode, step onto next parts */
        if (idxData == NULL) {
            SkipnStrides += 1;
        }

    } /* for each vertex */

    glEnd();
    checkGLcall("glEnd and previous calls");
}

static void drawStridedHardwareVS(IWineD3DDevice *iface, Direct3DVertexStridedData *sd,
                     int PrimitiveType, ULONG NumPrimitives,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx) {

    IDirect3DVertexShaderImpl* vertex_shader = NULL;
    int                        i;
    int                        NumVertexes;
    int                        glPrimType;
    int                        maxAttribs;

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("Drawing with hardware vertex shaders\n");

    /* Retrieve the VS information */
    vertex_shader = VERTEX_SHADER(This->stateBlock->VertexShader);

    /* Enable the Vertex Shader */
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertex_shader->prgId));
    checkGLcall("glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertex_shader->prgId);");
    glEnable(GL_VERTEX_PROGRAM_ARB);
    checkGLcall("glEnable(GL_VERTEX_PROGRAM_ARB);");

    /* Update the constants */
    for (i=0; i<D3D8_VSHADER_MAX_CONSTANTS; i++) {
        GL_EXTCALL(glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i, (GLfloat *)&This->stateBlock->vertexShaderConstant[i]));
        checkGLcall("glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB");
    }

    /* Set up the vertex.attr[n] inputs */
    IDirect3DDeviceImpl_FillVertexShaderInputArbHW(This, vertex_shader, 0);

    /* Ok, Work out which primitive is requested and how many vertexes that
       will be                                                              */
    NumVertexes = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);

    /* Finally do the drawing */
    if (idxData != NULL) {

        TRACE("glElements(%x, %d, %ld, ...)\n", glPrimType, NumVertexes, minIndex);
#if 1  /* FIXME: Want to use DrawRangeElements, but wrong calculation! */
        glDrawElements(glPrimType, NumVertexes, idxSize==2?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,
                      (const char *)idxData+(idxSize * startIdx));
#else
        glDrawRangeElements(glPrimType, minIndex, minIndex+NumVertexes-1, NumVertexes,
                      idxSize==2?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,
                      (const char *)idxData+(idxSize * startIdx));
#endif
        checkGLcall("glDrawRangeElements");

    } else {

        /* Note first is now zero as we shuffled along earlier */
        TRACE("glDrawArrays(%x, 0, %d)\n", glPrimType, NumVertexes);
        glDrawArrays(glPrimType, 0, NumVertexes);
        checkGLcall("glDrawArrays");

    }

    {
    GLint errPos;
    glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &errPos );
    if (errPos != -1)
        FIXME("HW VertexShader Error at position: %d\n%s\n", errPos, glGetString( GL_PROGRAM_ERROR_STRING_ARB) );
    }


    /* Leave all the attribs disabled */
    glGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &maxAttribs);
    /* MESA does not support it right not */
    if (glGetError() != GL_NO_ERROR)
	maxAttribs = 16;
    for (i=0; i<maxAttribs; i++) {
        GL_EXTCALL(glDisableVertexAttribArrayARB(i));
        checkGLcall("glDisableVertexAttribArrayARB(reg);");
    }

    /* Done */
    glDisable(GL_VERTEX_PROGRAM_ARB);
}
#endif

void inline drawPrimitiveTraceDataLocations(Direct3DVertexStridedData *dataLocations,DWORD fvf){

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
    return;

}

/* loads any dirty textures and returns true if any of the textures are nonpower2 */
BOOL inline drawPrimitiveUploadDirtyTextures(IWineD3DDeviceImpl* This) {
    BOOL nonPower2 = FALSE;
    unsigned int i;
    register IWineD3DBaseTexture *texture;
    /* And re-upload any dirty textures */
    for (i = 0; i<GL_LIMITS(textures); ++i) {
        texture = This->stateBlock->textures[i];
        if (texture != NULL) {
            if(IWineD3DBaseTexture_GetDirty(texture)) {
                /* Load up the texture now */
                IWineD3DTexture_PreLoad((IWineD3DTexture *)texture);
            }
            if (IWineD3DResourceImpl_GetType((IWineD3DResource *)texture) == D3DRTYPE_TEXTURE) {
                /* TODO: Is this right, as its cast all texture types to texture8... checkme */
                IWineD3DSurface *surface;
                IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)texture, 0, &surface);
                if (((IWineD3DSurfaceImpl *)surface)->nonpow2) {
                    nonPower2 = TRUE;
                }
            }
        }
    }
    return nonPower2;
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(IWineD3DDevice *iface,
                    int PrimitiveType, long NumPrimitives,

                    /* for Indexed: */
                    long  StartVertexIndex,
                    long  StartIdx,
                    short idxSize,
                    const void *idxData,
                    int   minIndex) {

    BOOL                          rc = FALSE;
    DWORD                         fvf = 0;
#if 0 /* TODO: vertex and pixel shaders */
    IDirect3DVertexShaderImpl    *vertex_shader = NULL;
    IDirect3DPixelShaderImpl     *pixel_shader = NULL;
#endif
    IWineD3DDeviceImpl           *This = (IWineD3DDeviceImpl *)iface;
    BOOL                          useVertexShaderFunction = FALSE;
    BOOL                          isLightingOn = FALSE;
    Direct3DVertexStridedData     dataLocations;
    int                           useHW = FALSE;
    BOOL                          nonPower2 = FALSE; /* set to true if any surfaces are non-power2 so that drawslow is used. */

    if (This->stateBlock->vertexDecl == NULL) {
        /* Work out what the FVF should look like */
        rc = initializeFVF(iface, &fvf, &useVertexShaderFunction);
        if (rc) return;
    } else {
        TRACE("(%p) : using vertex declaration %p \n", iface, This->stateBlock->vertexDecl);
    }

    /* If we will be using a vertex shader, do some initialization for it */
    if (useVertexShaderFunction) {
#if 0 /* TODO: vertex and pixel shaders */
        vertex_shader = VERTEX_SHADER(This->stateBlock->VertexShader);
        memset(&vertex_shader->input, 0, sizeof(VSHADERINPUTDATA8));

        useHW = (((vs_mode == VS_HW) && GL_SUPPORT(ARB_VERTEX_PROGRAM)) &&
                 This->devType != D3DDEVTYPE_REF &&
                 !This->stateBlock->renderState[D3DRS_SOFTWAREVERTEXPROCESSING] &&
                 vertex_shader->usage != D3DUSAGE_SOFTWAREPROCESSING);

        /** init Constants */
        if (This->stateBlock->Changed.vertexShaderConstant) {
            TRACE_(d3d_shader)("vertex shader initializing constants\n");
            IDirect3DVertexShaderImpl_SetConstantF(vertex_shader, 0, (CONST FLOAT*) &This->stateBlock->vertexShaderConstant[0], 96);
        }
#endif /* TODO: vertex and pixel shaders */
    }

    /* Ok, we will be updating the screen from here onwards so grab the lock */
    ENTER_GL();

#if 0 /* TODO: vertex and pixel shaders */
    /* If we will be using a pixel, do some initialization for it */
    if ((pixel_shader = PIXEL_SHADER(This->stateBlock->PixelShader))) {
        TRACE("drawing with pixel shader handle %p\n", pixel_shader);
        memset(&pixel_shader->input, 0, sizeof(PSHADERINPUTDATA8));

        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pixel_shader->prgId));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pixel_shader->prgId);");
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB);");

        /* init Constants */
        if (This->stateBlock->Changed.pixelShaderConstant) {
            TRACE_(d3d_shader)("pixel shader initializing constants %p\n",pixel_shader);
            IDirect3DPixelShaderImpl_SetConstantF(pixel_shader, 0, (CONST FLOAT*) &This->stateBlock->pixelShaderConstant[0], 8);
        }
        /* Update the constants */
        for (i=0; i<D3D8_PSHADER_MAX_CONSTANTS; i++) {
            GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i, (GLfloat *)&This->stateBlock->pixelShaderConstant[i]));
            checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB");
        }
    }
#endif /* TODO: vertex and pixel shaders */

    /* Initialize all values to null */
    if (useVertexShaderFunction == FALSE) {
        memset(&dataLocations, 0x00, sizeof(dataLocations));

        /* Convert to strided data */
         if(This->stateBlock->vertexDecl != NULL){
            TRACE("================ Vertex Declaration  ===================\n");
            primitiveDeclarationConvertToStridedData(iface, &dataLocations, StartVertexIndex, &fvf);
         }else{
            TRACE("================ FVF ===================\n");
            primitiveConvertToStridedData(iface, &dataLocations, StartVertexIndex);
         }

        /* write out some debug information*/
        drawPrimitiveTraceDataLocations(&dataLocations, fvf);
    } else {
        FIXME("line %d, drawing using vertex shaders\n", __LINE__);
    }

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
    init_materials(iface, (dataLocations.u.s.diffuse.lpData != NULL));

    nonPower2 = drawPrimitiveUploadDirtyTextures(This);

    /* Now draw the graphics to the screen */
    if  (useVertexShaderFunction) {

        /* Ideally, we should have software FV and hardware VS, possibly
           depending on the device type?                                 */

        if (useHW) {
            TRACE("Swap HW vertex shader\n");
#if 0 /* TODO: vertex and pixel shaders */
            drawStridedHardwareVS(iface, &dataLocations, PrimitiveType, NumPrimitives,
                        idxData, idxSize, minIndex, StartIdx);
#endif
	} else {
            /* We will have to use the very, very slow emulation layer */
            TRACE("Swap SW vertex shader\n");
#if 0 /* TODO: vertex and pixel shaders */
	    drawStridedSoftwareVS(iface, &dataLocations, PrimitiveType, NumPrimitives,
                        idxData, idxSize, minIndex, StartIdx);
#endif
        }

    } else if ((dataLocations.u.s.pSize.lpData           != NULL)
               || (dataLocations.u.s.diffuse.lpData      != NULL)
               || nonPower2
	       /*|| (dataLocations.u.s.blendWeights.lpData != NULL)*/) {

        /* Fixme, Ideally, only use the per-vertex code for software HAL
           but until opengl supports all the functions returned to setup
           vertex arrays, we need to drop down to the slow mechanism for
           certain functions                                              */

        /* We will have to use the slow version of GL per vertex setup */
        drawStridedSlow(iface, &dataLocations, PrimitiveType, NumPrimitives,
                        idxData, idxSize, minIndex, StartIdx);

    } else {

        /* We can use the fast version of GL pointers */
        drawStridedFast(iface, &dataLocations, PrimitiveType, NumPrimitives,
                        idxData, idxSize, minIndex, StartIdx);
    }

    /* If vertex shaders or no normals, restore previous lighting state */
    if (useVertexShaderFunction || !(fvf & D3DFVF_NORMAL)) {
        if (isLightingOn) glEnable(GL_LIGHTING);
        else glDisable(GL_LIGHTING);
        TRACE("Restored lighting to original state\n");
    }

#if 0 /* TODO: vertex and pixel shaders */
    if (pixel_shader)
    {
#if 0
      GLint errPos;
      glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &errPos );
      if (errPos != -1)
        FIXME("HW PixelShader Error at position: %d\n%s\n", errPos, glGetString( GL_PROGRAM_ERROR_STRING_ARB) );
#endif
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }
#endif

    /* Finshed updating the screen, restore lock */
    LEAVE_GL();
    TRACE("Done all gl drawing\n");

    /* Diagnostics */
#if defined(SHOW_FRAME_MAKEUP)
    {
        if (isDumpingFrames) {
            D3DLOCKED_RECT r;
            char buffer[80];
            IDirect3DSurface8Impl_LockRect((LPDIRECT3DSURFACE8) This->renderTarget, &r, NULL, D3DLOCK_READONLY);
            sprintf(buffer, "/tmp/backbuffer_%ld.ppm", primCounter);
            TRACE("Saving screenshot %s\n", buffer);
            IDirect3DSurface8Impl_SaveSnapshot((LPDIRECT3DSURFACE8) This->renderTarget, buffer);
            IDirect3DSurface8Impl_UnlockRect((LPDIRECT3DSURFACE8) This->renderTarget);

#if defined(SHOW_TEXTURE_MAKEUP)
           {
            LPDIRECT3DSURFACE8 pSur;
            int textureNo;
            for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {
                if (This->stateBlock->textures[textureNo] != NULL) {
                    sprintf(buffer, "/tmp/texture_%ld_%d.ppm", primCounter, textureNo);
                    TRACE("Saving texture %s (Format:%s)\n", buffer, debug_d3dformat(((IDirect3DBaseTexture8Impl *)This->stateBlock->textures[textureNo])->format));
                    IDirect3DTexture8Impl_GetSurfaceLevel((LPDIRECT3DTEXTURE8) This->stateBlock->textures[textureNo], 0, &pSur);
                    IDirect3DSurface8Impl_SaveSnapshot(pSur, buffer);
                    IDirect3DSurface8Impl_Release(pSur);
                }
            }
           }
#endif
           primCounter = primCounter + 1;
        }
    }
#endif
}
