/*
 * D3D8 utils
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
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

#include <math.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_shader);

extern IDirect3DVertexShaderImpl*            VertexShaders[64];
extern IDirect3DVertexShaderDeclarationImpl* VertexShaderDeclarations[64];
extern IDirect3DPixelShaderImpl*             PixelShaders[64];

/* Useful holding place for 4 floats */
typedef struct _D3DVECTOR_4 {
    float x;
    float y;
    float z;
    float w;
} D3DVECTOR_4;

#undef GL_VERSION_1_4 /* To be fixed, caused by mesa headers */

/* Returns bits for what is expected from the fixed function pipeline, and whether 
   a vertex shader will be in use. Note the fvf bits returned may be split over
   multiple streams only if the vertex shader was created, otherwise it all relates
   to stream 0                                                                      */
BOOL initializeFVF(LPDIRECT3DDEVICE8 iface, 
                   DWORD *FVFbits,                 /* What to expect in the FVF across all streams */
                   BOOL *useVertexShaderFunction)  /* Should we use the vertex shader              */
{

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* The first thing to work out is if we are using the fixed function pipeline 
       which is either SetVertexShader with < VS_HIGHESTFIXEDFXF - in which case this
       is the FVF, or with a shader which was created with no function - in which
       case there is an FVF per declared stream. If this occurs, we also maintain
       an 'OR' of all the FVF's together so we know what to expect across all the     
       streams                                                                        */

    if (This->UpdateStateBlock->VertexShader <= VS_HIGHESTFIXEDFXF) {

        /* Use this as the FVF */
        *FVFbits = This->UpdateStateBlock->VertexShader;
        *useVertexShaderFunction = FALSE;
        TRACE("FVF explicitally defined, using fixed function pipeline with FVF=%lx\n", *FVFbits);

    } else {

        /* Use created shader */
        IDirect3DVertexShaderImpl* vertex_shader = NULL;
        vertex_shader = VERTEX_SHADER(This->UpdateStateBlock->VertexShader);

        if (vertex_shader == NULL) {

            /* Hmm - User pulled figure out of the air? Unlikely, probably a bug */
            ERR("trying to use unitialised vertex shader: %lu\n", This->UpdateStateBlock->VertexShader);
            return TRUE;

        } else {

            *FVFbits = This->UpdateStateBlock->vertexShaderDecl->allFVF;

            if (vertex_shader->function == NULL) {
                /* No function, so many streams supplied plus FVF definition pre stream */
                *useVertexShaderFunction = FALSE;
                TRACE("vertex shader (%lx) declared without program, using fixed function pipeline with FVF=%lx\n", 
                            This->StateBlock->VertexShader, *FVFbits);
            } else {
                /* Vertex shader needs calling */
                *useVertexShaderFunction = TRUE;
                TRACE("vertex shader will be used (unusued FVF=%lx)\n", *FVFbits);
            }
        }
    }
    return FALSE;
}

/* Issues the glBegin call for gl given the primitive type and count */
DWORD primitiveToGl(D3DPRIMITIVETYPE PrimitiveType,
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

/* Setup views - Transformed & lit if RHW, else untransformed.
       Only unlit if Normals are supplied                       
    Returns: Whether to restore lighting afterwards           */
BOOL primitiveInitState(LPDIRECT3DDEVICE8 iface, BOOL vtx_transformed, BOOL vtx_lit) {

    BOOL isLightingOn = FALSE;
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* If no normals, DISABLE lighting otherwise, dont touch lighing as it is 
       set by the appropriate render state                                    */
    if (vtx_lit) {
        isLightingOn = glIsEnabled(GL_LIGHTING);
        glDisable(GL_LIGHTING);
        checkGLcall("glDisable(GL_LIGHTING);");
        TRACE("Enabled lighting as no normals supplied, old state = %d\n", isLightingOn);
    }

    if (vtx_transformed) {
        double X, Y, height, width, minZ, maxZ;

        /* Transformed already into viewport coordinates, so we do not need transform
           matrices. Reset all matrices to identity and leave the default matrix in world 
           mode.                                                                         */
        glMatrixMode(GL_MODELVIEW);
        checkGLcall("glMatrixMode");
        glLoadIdentity();
        checkGLcall("glLoadIdentity");
        /**
         * As seen in d3d7 code:
         *  See the OpenGL Red Book for an explanation of the following translation (in the OpenGL
         *  Correctness Tips section).
         */
        glTranslatef(0.375f, 0.375f, 0.0f);
        checkGLcall("glTranslatef(0.375f, 0.375f, 0.0f)");

        glMatrixMode(GL_PROJECTION);
        checkGLcall("glMatrixMode");
        glLoadIdentity();
        checkGLcall("glLoadIdentity");

        /* Set up the viewport to be full viewport */
        X      = This->StateBlock->viewport.X;
        Y      = This->StateBlock->viewport.Y;
        height = This->StateBlock->viewport.Height;
        width  = This->StateBlock->viewport.Width;
        minZ   = This->StateBlock->viewport.MinZ;
        maxZ   = This->StateBlock->viewport.MaxZ;
        TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, -minZ, -maxZ);
        glOrtho(X, X + width, Y + height, Y, -minZ, -maxZ);
        checkGLcall("glOrtho");

    } else {

        /* Untransformed, so relies on the view and projection matrices */
        glMatrixMode(GL_MODELVIEW);
        checkGLcall("glMatrixMode");
        glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0]);
        checkGLcall("glLoadMatrixf");
        glMultMatrixf((float *) &This->StateBlock->transforms[D3DTS_WORLDMATRIX(0)].u.m[0][0]);
        checkGLcall("glMultMatrixf");


        glMatrixMode(GL_PROJECTION);
        checkGLcall("glMatrixMode");
        glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_PROJECTION].u.m[0][0]);
        checkGLcall("glLoadMatrixf");
    }
    return isLightingOn;
}

void primitiveConvertToStridedData(LPDIRECT3DDEVICE8 iface, Direct3DVertexStridedData *strided, LONG BaseVertexIndex) {

    short         LoopThroughTo = 0;
    short         nStream;
    BOOL          canDoViaGLPointers = TRUE;
    int           numBlends;
    int           numTextures;
    int           textureNo;
    int           coordIdxInfo = 0x00;    /* Information on number of coords supplied */
    int           numCoords[8];           /* Holding place for D3DFVF_TEXTUREFORMATx  */

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* OK, Now to setup the data locations 
       For the non-created vertex shaders, the VertexShader var holds the real 
          FVF and only stream 0 matters
       For the created vertex shaders, there is an FVF per stream              */
    if (This->UpdateStateBlock->VertexShader > VS_HIGHESTFIXEDFXF) {
        LoopThroughTo = MAX_STREAMS;
    } else {
        LoopThroughTo = 1;
    }

    /* Work through stream by stream */
    for (nStream=0; nStream<LoopThroughTo; nStream++) {
        DWORD  stride  = This->StateBlock->stream_stride[nStream];
        BYTE  *data    = NULL;
        DWORD  thisFVF = 0;

        /* Skip empty streams */
        if (This->StateBlock->stream_source[nStream] == NULL) continue;

        /* Retrieve appropriate FVF */
        if (LoopThroughTo == 1) { /* VertexShader is FVF */
            thisFVF = This->UpdateStateBlock->VertexShader;
            /* Handle memory passed directly as well as vertex buffers */
            if (This->StateBlock->streamIsUP == TRUE) {
                data    = (BYTE *)This->StateBlock->stream_source[nStream];
            } else {
                data    = ((IDirect3DVertexBuffer8Impl *)This->StateBlock->stream_source[nStream])->allocatedMemory;
            }
        } else {
            thisFVF = This->StateBlock->vertexShaderDecl->fvf[nStream];
            data    = ((IDirect3DVertexBuffer8Impl *)This->StateBlock->stream_source[nStream])->allocatedMemory;
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
            strided->u.s.position.dwType    = D3DVSDT_FLOAT3;
            strided->u.s.position.dwStride  = stride;
            data += 3 * sizeof(float);
            if (thisFVF & D3DFVF_XYZRHW) {
                strided->u.s.position.dwType = D3DVSDT_FLOAT4;
                data += sizeof(float);
            }
        }

        /* Blending is numBlends * FLOATs followed by a DWORD for UBYTE4 */
        /** do we have to Check This->UpdateStateBlock->renderstate[D3DRS_INDEXEDVERTEXBLENDENABLE] ? */
        numBlends = ((thisFVF & D3DFVF_POSITION_MASK) >> 1) - 2 + 
                    ((FALSE == (thisFVF & D3DFVF_LASTBETA_UBYTE4)) ? 0 : -1);    /* WARNING can be < 0 because -2 */
        if (numBlends > 0) {
            canDoViaGLPointers = FALSE; 
            strided->u.s.blendWeights.lpData    = data;
            strided->u.s.blendWeights.dwType    = D3DVSDT_FLOAT1 + (numBlends - 1);
            strided->u.s.blendWeights.dwStride  = stride;
            data += numBlends * sizeof(FLOAT);

            if (thisFVF & D3DFVF_LASTBETA_UBYTE4) {
                strided->u.s.blendMatrixIndices.lpData = data;
                strided->u.s.blendMatrixIndices.dwType  = D3DVSDT_UBYTE4; 
                strided->u.s.blendMatrixIndices.dwStride= stride; 
                data += sizeof(DWORD);
            }
        }

        /* Normal is always 3 floats */
        if (thisFVF & D3DFVF_NORMAL) {
            strided->u.s.normal.lpData    = data;
            strided->u.s.normal.dwType    = D3DVSDT_FLOAT3;
            strided->u.s.normal.dwStride  = stride;
            data += 3 * sizeof(FLOAT);
        }

        /* Pointsize is a single float */
        if (thisFVF & D3DFVF_PSIZE) {
            strided->u.s.pSize.lpData    = data;
            strided->u.s.pSize.dwType    = D3DVSDT_FLOAT1;
            strided->u.s.pSize.dwStride  = stride;
            data += sizeof(FLOAT);
        }

        /* Diffuse is 4 unsigned bytes */
        if (thisFVF & D3DFVF_DIFFUSE) {
            strided->u.s.diffuse.lpData    = data;
            strided->u.s.diffuse.dwType    = D3DVSDT_SHORT4;
            strided->u.s.diffuse.dwStride  = stride;
            data += sizeof(DWORD);
        }

        /* Specular is 4 unsigned bytes */
        if (thisFVF & D3DFVF_SPECULAR) {
            strided->u.s.specular.lpData    = data;
            strided->u.s.specular.dwType    = D3DVSDT_SHORT4;
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
            strided->u.s.texCoords[textureNo].dwType    = D3DVSDT_FLOAT1;
            strided->u.s.texCoords[textureNo].dwStride  = stride;
            numCoords[textureNo] = coordIdxInfo & 0x03;

            /* Always one set */
            data += sizeof(float);
            if (numCoords[textureNo] != D3DFVF_TEXTUREFORMAT1) {
                strided->u.s.texCoords[textureNo].dwType = D3DVSDT_FLOAT2;
                data += sizeof(float);
                if (numCoords[textureNo] != D3DFVF_TEXTUREFORMAT2) {
                    strided->u.s.texCoords[textureNo].dwType = D3DVSDT_FLOAT3;
                    data += sizeof(float);
                    if (numCoords[textureNo] != D3DFVF_TEXTUREFORMAT3) {
                        strided->u.s.texCoords[textureNo].dwType = D3DVSDT_FLOAT4;
                        data += sizeof(float);
                    }
                }
            }
            coordIdxInfo = coordIdxInfo >> 2; /* Drop bottom two bits */
        }
    }
}

/* Draw a single vertex using this information */
void draw_vertex(LPDIRECT3DDEVICE8 iface,                              /* interface    */
                 BOOL isXYZ,    float x, float y, float z, float rhw,  /* xyzn position*/
                 BOOL isNormal, float nx, float ny, float nz,          /* normal       */
                 BOOL isDiffuse, float *dRGBA,                         /* 1st   colors */
                 BOOL isSpecular, float *sRGB,                         /* 2ndry colors */
                 BOOL isPtSize, float ptSize,                       /* pointSize    */
                 D3DVECTOR_4 *texcoords, int *numcoords)               /* texture info */
{
    int textureNo;
    float s, t, r, q;
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* Diffuse -------------------------------- */
    if (isDiffuse == TRUE) {
        glColor4fv(dRGBA);
        VTRACE(("glColor4f: r,g,b,a=%f,%f,%f,%f\n", dRGBA[0], dRGBA[1], dRGBA[2], dRGBA[3]));
    }

    /* Specular Colour ------------------------------------------*/
    if (isSpecular == TRUE) {
#if defined(GL_EXT_secondary_color)
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
          GL_EXTCALL(glSecondaryColor3fvEXT(sRGB));
          VTRACE(("glSecondaryColor4f: r,g,b=%f,%f,%f\n", sRGB[0], sRGB[1], sRGB[2]));
        }
#endif
    }

    /* Normal -------------------------------- */
    if (isNormal == TRUE) {
        VTRACE(("glNormal:nx,ny,nz=%f,%f,%f\n", nx,ny,nz));
        glNormal3f(nx, ny, nz);
    } 

    /* Point Size ----------------------------------------------*/
    if (isPtSize == TRUE) {

        /* no such functionality in the fixed function GL pipeline */
        FIXME("Cannot change ptSize here in openGl\n");
    }

    /* Texture coords --------------------------- */
    for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {

        if (!GL_SUPPORT(ARB_MULTITEXTURE) && textureNo > 0) {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesnt support\n");
            continue ;
        }

        /* Query tex coords */
        if (This->StateBlock->textures[textureNo] != NULL) {

            int    coordIdx = This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXCOORDINDEX];
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
#if defined(GL_VERSION_1_3)
                        glMultiTexCoord1f(GL_TEXTURE0 + textureNo, s);
#else
                        glMultiTexCoord1fARB(GL_TEXTURE0_ARB + textureNo, s);
#endif
                    } else {
                        glTexCoord1f(s);
                    }
                    break;
                case D3DTTFF_COUNT2:
                    VTRACE(("tex:%d, s=%f, t=%f\n", textureNo, s, t));
                    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                        glMultiTexCoord2f(GL_TEXTURE0 + textureNo, s, t);
#else
                        glMultiTexCoord2fARB(GL_TEXTURE0_ARB + textureNo, s, t);
#endif
                    } else {
                        glTexCoord2f(s, t);
                    }
                    break;
                case D3DTTFF_COUNT3:
                    VTRACE(("tex:%d, s=%f, t=%f, r=%f\n", textureNo, s, t, r));
                    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                        glMultiTexCoord3f(GL_TEXTURE0 + textureNo, s, t, r);
#else
                        glMultiTexCoord3fARB(GL_TEXTURE0_ARB + textureNo, s, t, r);
#endif
                    } else {
                        glTexCoord3f(s, t, r);
                    }
                    break;
                case D3DTTFF_COUNT4:
                    VTRACE(("tex:%d, s=%f, t=%f, r=%f, q=%f\n", textureNo, s, t, r, q));
                    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                        glMultiTexCoord4f(GL_TEXTURE0 + textureNo, s, t, r, q);
#else
                        glMultiTexCoord4fARB(GL_TEXTURE0_ARB + textureNo, s, t, r, q);
#endif
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
    if (isXYZ == TRUE) {
        if (1.0f == rhw || rhw < 0.01f) {
            VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f\n", x,y,z));
            glVertex3f(x, y, z);
        } else {
            GLfloat w = 1.0f / rhw;
            VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f / rhw=%f\n", x,y,z,rhw));
            glVertex4f(x * w, y * w, z * w, 1.0f);
        }
    }
}

/* 
 * Actually draw using the supplied information.
 * Faster GL version using pointers to data, harder to debug though 
 * Note does not handle vertex shaders yet                             
 */
void drawStridedFast(LPDIRECT3DDEVICE8 iface, Direct3DVertexStridedData *sd, 
                     int PrimitiveType, ULONG NumPrimitives,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx) {
    int          textureNo   = 0;
    GLenum       glPrimType  = GL_POINTS;
    int          NumVertexes = NumPrimitives;
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    TRACE("Using fast vertex array code\n");

    /* Vertex Pointers -----------------------------------------*/
    if (sd->u.s.position.lpData != NULL) {

        /* Note dwType == float3 or float4 == 2 or 3 */
        VTRACE(("glVertexPointer(%ld, GL_FLOAT, %ld, %p)\n", 
                sd->u.s.position.dwStride, 
                sd->u.s.position.dwType + 1, 
                sd->u.s.position.lpData));
        glVertexPointer(sd->u.s.position.dwType + 1, GL_FLOAT, 
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
        /* FIXME: Wont get here as will drop to slow method        */
        FIXME("Blending not supported in fast draw routine\n");

#if 0 /* Vertex blend support needs to be added */
        if (GL_SUPPORT(ARB_VERTEX_BLEND)) {
            /*FIXME("TODO\n");*/
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
            FIXME("TODO\n");
        } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
            FIXME("TODO\n");
            /*
            glDisableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT);
            checkGLcall("glDisableClientState(GL_VERTEX_WEIGHT_ARRAY_EXT)");
            */
        }
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
        /* FIXME: Wont get here as will drop to slow method        */
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

#if defined(GL_VERSION_1_4)
        glSecondaryColorPointer(4, GL_UNSIGNED_BYTE, 
                                   sd->u.s.specular.dwStride, 
                                   sd->u.s.specular.lpData);
        vcheckGLcall("glSecondaryColorPointer(4, GL_UNSIGNED_BYTE, ...)");
        glEnableClientState(GL_SECONDARY_COLOR_ARRAY);
        vcheckGLcall("glEnableClientState(GL_SECONDARY_COLOR_ARRAY)");
#elif defined(GL_EXT_secondary_color)
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
            GL_EXTCALL(glSecondaryColorPointerEXT)(4, GL_UNSIGNED_BYTE,
                                                   sd->u.s.specular.dwStride, 
                                                   sd->u.s.specular.lpData);
            checkGLcall("glSecondaryColorPointerEXT(4, GL_UNSIGNED_BYTE, ...)");
            glEnableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
            checkGLcall("glEnableClientState(GL_SECONDARY_COLOR_ARRAY_EXT)");
        }
#else
        /* Missing specular color is not critical, no warnings */
        VTRACE(("Specular colour is not supported in this GL implementation\n"));
#endif

    } else {

#if defined(GL_VERSION_1_4)
        glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
        checkGLcall("glDisableClientState(GL_SECONDARY_COLOR_ARRAY)");
        glSecondaryColor3f(0, 0, 0);
        checkGLcall("glSecondaryColor3f(0, 0, 0)");
#elif defined(GL_EXT_secondary_color)
        if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
              glDisableClientState(GL_SECONDARY_COLOR_ARRAY_EXT);
            checkGLcall("glDisableClientState(GL_SECONDARY_COLOR_ARRAY_EXT)");
            GL_EXTCALL(glSecondaryColor3fEXT)(0, 0, 0);
            checkGLcall("glSecondaryColor3fEXT(0, 0, 0)");
        }
#else
        /* Do not worry if specular colour missing and disable request */
#endif
    }

    /* Texture coords -------------------------------------------*/
    for (textureNo = 0; textureNo < GL_LIMITS(textures); ++textureNo) {

        if (!GL_SUPPORT(ARB_MULTITEXTURE) && textureNo > 0) {
            FIXME("Program using multiple concurrent textures which this opengl implementation doesnt support\n");
            continue ;
        }

        /* Query tex coords */
        if (This->StateBlock->textures[textureNo] != NULL) {
            int coordIdx = This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXCOORDINDEX];

            if (coordIdx > 7) {
                VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
            } else if (sd->u.s.texCoords[coordIdx].lpData == NULL) {
                VTRACE(("Bound texture but no texture coordinates supplied, so skipping\n"));
            } else {
                int coordsToUse = sd->u.s.texCoords[coordIdx].dwType + 1; /* 0 == D3DVSDT_FLOAT1 etc */
                int numFloats = coordsToUse;
#if defined(GL_VERSION_1_3)
                glClientActiveTexture(GL_TEXTURE0 + textureNo);
#else
                glClientActiveTextureARB(GL_TEXTURE0_ARB + textureNo);
#endif
                /* If texture transform flags in effect, values passed through to vertex
                   depend on the D3DTSS_TEXTURETRANSFORMFLAGS */
                if (This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE) {

                    /* This indicates how many coords to use regardless of the
                       texture type. However, d3d/opengl fill in the rest appropriately */
                    coordsToUse = This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & ~D3DTTFF_PROJECTED;

                    /* BUT - Projected is more 'fun' - Cant be done for ptr mode.
                       Probably should scan enabled texture units and drop back to
                       slow mode if found? */
                    if (This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & D3DTTFF_PROJECTED) {
                        FIXME("Cannot handle projected transform state in fast mode\n");
                    }

                    /* coordsToUse maps to D3DTTFF_COUNT1,2,3,4 == 1,2,3,4 which is correct */
                }
                if (numFloats == 0) {
                    FIXME("Skipping as invalid request - numfloats=%d, coordIdx=%d\n", numFloats, coordIdx);
                    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                    checkGLcall("glDisableClientState(GL_TEXTURE_COORD_ARRAY);");
                } else {
                    VTRACE(("tex: %d, ptr=%p, numcoords=%d\n", textureNo, sd->u.s.texCoords[coordIdx].lpData, numFloats));
                    glTexCoordPointer(numFloats, GL_FLOAT, sd->u.s.texCoords[coordIdx].dwStride, sd->u.s.texCoords[coordIdx].lpData);
                    checkGLcall("glTexCoordPointer(x, ...)");
                    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    checkGLcall("glEnableClientState(GL_TEXTURE_COORD_ARRAY);");
                }
            }
        }
    }

    /* Ok, Work out which primitive is requested and how many vertexes that 
       will be                                                              */
    NumVertexes = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);

    /* Finally do the drawing */
    if (idxData != NULL) {

        TRACE("glElements(%x, %d, %ld, ...)\n", glPrimType, NumVertexes, minIndex);
        if (idxSize==2) {
#if 1  /* FIXME: Want to use DrawRangeElements, but wrong calculation! */
            glDrawElements(glPrimType, NumVertexes, GL_UNSIGNED_SHORT,
                           (char *)idxData+(2 * startIdx));
#else
            glDrawRangeElements(glPrimType, minIndex, minIndex+NumVertexes-1, NumVertexes, 
                                GL_UNSIGNED_SHORT, (char *)idxData+(2 * startIdx));
#endif
        } else {
#if 1  /* FIXME: Want to use DrawRangeElements, but wrong calculation! */
            glDrawElements(glPrimType, NumVertexes, GL_UNSIGNED_INT,
                           (char *)idxData+(4 * startIdx));
#else
            glDrawRangeElements(glPrimType, minIndex, minIndex+NumVertexes-1, NumVertexes, 
                                GL_UNSIGNED_INT, (char *)idxData+(2 * startIdx));
#endif
        }
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
void drawStridedSlow(LPDIRECT3DDEVICE8 iface, Direct3DVertexStridedData *sd, 
                     int PrimitiveType, ULONG NumPrimitives,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx) {

    int                        textureNo    = 0;
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    TRACE("Using slow vertex array code\n");

    /* Variable Initialization */
    if (idxData != NULL) {
        if (idxSize == 2) pIdxBufS = (short *) idxData;
        else pIdxBufL = (long *) idxData;
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
            if (sd->u.s.position.dwType == D3DVSDT_FLOAT4) {
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
                FIXME("Program using multiple concurrent textures which this opengl implementation doesnt support\n");
                continue ;
            }

            /* Query tex coords */
            if (This->StateBlock->textures[textureNo] != NULL) {

                int    coordIdx = This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXCOORDINDEX];
                float *ptrToCoords = (float *)(sd->u.s.texCoords[coordIdx].lpData + (SkipnStrides * sd->u.s.texCoords[coordIdx].dwStride));
                float  s = 0.0, t = 0.0, r = 0.0, q = 0.0;

                if (coordIdx > 7) {
                    VTRACE(("tex: %d - Skip tex coords, as being system generated\n", textureNo));
                    continue;
                } else if (sd->u.s.texCoords[coordIdx].lpData == NULL) {
                    TRACE("tex: %d - Skipping tex coords, as no data supplied\n", textureNo);
                    continue;
                } else {

                    int coordsToUse = sd->u.s.texCoords[coordIdx].dwType + 1; /* 0 == D3DVSDT_FLOAT1 etc */

                    /* If texture transform flags in effect, values passed through to vertex
                       depend on the D3DTSS_TEXTURETRANSFORMFLAGS */
                    if (This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE) {

                        /* This indicates how many coords to use regardless of the
                           texture type. However, d3d/opengl fill in the rest appropriately */
                        coordsToUse = This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & ~D3DTTFF_PROJECTED;

                        switch (coordsToUse) {
                        case 4: q = ptrToCoords[3]; /* drop through */
                        case 3: r = ptrToCoords[2]; /* drop through */
                        case 2: t = ptrToCoords[1]; /* drop through */
                        case 1: s = ptrToCoords[0]; 
                        }

                        /* BUT - Projected is more 'fun' - Move the last coord to the 'q'
                           parameter (see comments under D3DTSS_TEXTURETRANSFORMFLAGS */
                        if (This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & D3DTTFF_PROJECTED) {
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
                                      This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & D3DTTFF_PROJECTED);
                            }
                        }
                    } else {
                        switch (coordsToUse) {
                        case 4: q = ptrToCoords[3]; /* drop through */
                        case 3: r = ptrToCoords[2]; /* drop through */
                        case 2: t = ptrToCoords[1]; /* drop through */
                        case 1: s = ptrToCoords[0]; 
                        }
                    }

                    switch (coordsToUse) {   /* Supply the provided texture coords */
                    case D3DTTFF_COUNT1:
                        VTRACE(("tex:%d, s=%f\n", textureNo, s));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                            glMultiTexCoord1f(GL_TEXTURE0 + textureNo, s);
#else
                            glMultiTexCoord1fARB(GL_TEXTURE0_ARB + textureNo, s);
#endif
                        } else {
                            glTexCoord1f(s);
                        }
                        break;
                    case D3DTTFF_COUNT2:
                        VTRACE(("tex:%d, s=%f, t=%f\n", textureNo, s, t));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                            glMultiTexCoord2f(GL_TEXTURE0 + textureNo, s, t);
#else
                            glMultiTexCoord2fARB(GL_TEXTURE0_ARB + textureNo, s, t);
#endif
                        } else {
                            glTexCoord2f(s, t);
                        }
                        break;
                    case D3DTTFF_COUNT3:
                        VTRACE(("tex:%d, s=%f, t=%f, r=%f\n", textureNo, s, t, r));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                            glMultiTexCoord3f(GL_TEXTURE0 + textureNo, s, t, r);
#else
                            glMultiTexCoord3fARB(GL_TEXTURE0_ARB + textureNo, s, t, r);
#endif
                        } else {
                            glTexCoord3f(s, t, r);
                        }
                        break;
                    case D3DTTFF_COUNT4:
                        VTRACE(("tex:%d, s=%f, t=%f, r=%f, q=%f\n", textureNo, s, t, r, q));
                        if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                            glMultiTexCoord4f(GL_TEXTURE0 + textureNo, s, t, r, q);
#else
                            glMultiTexCoord4fARB(GL_TEXTURE0_ARB + textureNo, s, t, r, q);
#endif
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
        }

        /* Normal -------------------------------- */
        if (sd->u.s.normal.lpData != NULL) {
            VTRACE(("glNormal:nx,ny,nz=%f,%f,%f\n", nx,ny,nz));
            glNormal3f(nx, ny, nz);
        } 
        
        /* Position -------------------------------- */
        if (sd->u.s.position.lpData != NULL) {
            if (1.0f == rhw || rhw < 0.01f) {
                VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f\n", x,y,z));
                glVertex3f(x, y, z);
            } else {
                VTRACE(("Vertex: glVertex:x,y,z=%f,%f,%f / rhw=%f\n", x,y,z,rhw));
                glVertex4f(x / rhw, y / rhw, z / rhw, 1.0f / rhw);
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

/* 
 * Draw with emulated vertex shaders
 * Note: strided data is uninitialized, as we need to pass the vertex
 *     shader directly as ordering irs yet                             
 */
void drawStridedSoftwareVS(LPDIRECT3DDEVICE8 iface, Direct3DVertexStridedData *sd, 
                     int PrimitiveType, ULONG NumPrimitives,
                     const void *idxData, short idxSize, ULONG minIndex, ULONG startIdx) {

    int                        textureNo    = 0;
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    IDirect3DVertexShaderImpl* vertex_shader = NULL;

    TRACE("Using slow software vertex shader code\n");

    /* Variable Initialization */
    if (idxData != NULL) {
        if (idxSize == 2) pIdxBufS = (short *) idxData;
        else pIdxBufL = (long *) idxData;
    }

    /* Ok, Work out which primitive is requested and how many vertexes that will be */
    NumVertexes = primitiveToGl(PrimitiveType, NumPrimitives, &glPrimType);

    /* Retrieve the VS information */
    vertex_shader = VERTEX_SHADER(This->StateBlock->VertexShader);

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
        IDirect3DDeviceImpl_FillVertexShaderInput(This, vertex_shader, SkipnStrides);

        /* Now execute the vertex shader */
        memset(&vertex_shader->output, 0, sizeof(VSHADEROUTPUTDATA8));
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
            if (This->StateBlock->textures[textureNo] != NULL) {
               texcoords[textureNo].x   = vertex_shader->output.oT[textureNo].x;
               texcoords[textureNo].y   = vertex_shader->output.oT[textureNo].y;
               texcoords[textureNo].z   = vertex_shader->output.oT[textureNo].z;
               texcoords[textureNo].w   = vertex_shader->output.oT[textureNo].w;
               if (This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE) {
                   numcoords[textureNo]    = This->UpdateStateBlock->texture_state[textureNo][D3DTSS_TEXTURETRANSFORMFLAGS] & ~D3DTTFF_PROJECTED;
               } else {
                   switch (IDirect3DBaseTexture8Impl_GetType((LPDIRECT3DBASETEXTURE8) This->StateBlock->textures[textureNo])) {
                   case D3DRTYPE_TEXTURE:       numcoords[textureNo]    = 2; break;
                   case D3DRTYPE_VOLUMETEXTURE: numcoords[textureNo]    = 3; break;
                   default:                     numcoords[textureNo]    = 4;
                   }
               }
            } else {
                numcoords[textureNo]    = 0;
            }
        }

        /* Draw using this information */
        draw_vertex(iface,
                    TRUE, x, y, z, rhw, 
                    FALSE, 0.0f, 0.0f, 0.0f, 
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

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(LPDIRECT3DDEVICE8 iface,
                    int PrimitiveType, long NumPrimitives,

                    /* for Indexed: */
                    long  StartVertexIndex,
                    long  StartIdx,
                    short idxSize,
                    const void *idxData,
                    int   minIndex) {

    BOOL                          rc = FALSE;
    DWORD                         fvf = 0;
    IDirect3DVertexShaderImpl    *vertex_shader = NULL;
    BOOL                          useVertexShaderFunction = FALSE;
    BOOL                          isLightingOn = FALSE;
    Direct3DVertexStridedData     dataLocations;
    ICOM_THIS(IDirect3DDevice8Impl,iface);


    /* Work out what the FVF should look like */
    rc = initializeFVF(iface, &fvf, &useVertexShaderFunction);
    if (rc) return;

    /* If we will be using a vertex shader, do some initialization for it */
    if (useVertexShaderFunction == TRUE) {
        vertex_shader = VERTEX_SHADER(This->UpdateStateBlock->VertexShader);
        memset(&vertex_shader->input, 0, sizeof(VSHADERINPUTDATA8));

        /** init Constants */
        if (TRUE == This->UpdateStateBlock->Changed.vertexShaderConstant) {
            TRACE_(d3d_shader)("vertex shader init Constant\n");
            IDirect3DVertexShaderImpl_SetConstantF(vertex_shader, 0, (CONST FLOAT*) &This->UpdateStateBlock->vertexShaderConstant[0], 96);
        }
    }

    /* Ok, we will be updating the screen from here onwards so grab the lock */
    ENTER_GL();

    /* Setup transform matrices and sort out */
    isLightingOn = primitiveInitState(iface, 
                                      fvf & D3DFVF_XYZRHW, 
                                      !(fvf & D3DFVF_NORMAL));

    /* Initialize all values to null */
    if (useVertexShaderFunction == FALSE) {
        memset(&dataLocations, 0x00, sizeof(dataLocations));

        /* Convert to strided data */
        primitiveConvertToStridedData(iface, &dataLocations, StartVertexIndex); 

        /* Dump out what parts we have supplied */
        TRACE("Strided Data (from FVF/VS): %lx\n", fvf);
        TRACE_STRIDED((&dataLocations), position);
        TRACE_STRIDED((&dataLocations), blendWeights);
        TRACE_STRIDED((&dataLocations), blendMatrixIndices);
        TRACE_STRIDED((&dataLocations), normal);
        TRACE_STRIDED((&dataLocations), pSize);
        TRACE_STRIDED((&dataLocations), diffuse);
        TRACE_STRIDED((&dataLocations), specular);
        TRACE_STRIDED((&dataLocations), texCoords[0]);
        TRACE_STRIDED((&dataLocations), texCoords[1]);
        TRACE_STRIDED((&dataLocations), texCoords[2]);
        TRACE_STRIDED((&dataLocations), texCoords[3]);
        TRACE_STRIDED((&dataLocations), texCoords[4]);
        TRACE_STRIDED((&dataLocations), texCoords[5]);
        TRACE_STRIDED((&dataLocations), texCoords[6]);
        TRACE_STRIDED((&dataLocations), texCoords[7]);
    }

    /* Now draw the graphics to the screen */
    if  (useVertexShaderFunction == TRUE) {

        /* Ideally, we should have software FV and hardware VS, possibly
           depending on the device type?                                 */

        /* We will have to use the very, very slow emulation layer */
        drawStridedSoftwareVS(iface, &dataLocations, PrimitiveType, NumPrimitives, 
                        idxData, idxSize, minIndex, StartIdx);            

    } else if ((dataLocations.u.s.pSize.lpData        != NULL) || 
               (dataLocations.u.s.diffuse.lpData      != NULL) || 
               (dataLocations.u.s.blendWeights.lpData != NULL)) {

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

    /* If no normals, restore previous lighting state */
    if (!(fvf & D3DFVF_NORMAL)) {
        if (isLightingOn) glEnable(GL_LIGHTING);
        else glDisable(GL_LIGHTING);
        TRACE("Restored lighting to original state\n");
    }

    /* Finshed updating the screen, restore lock */
    LEAVE_GL();
    TRACE("Done all gl drawing\n");

    /* Diagnostics */
#if defined(SHOW_FRAME_MAKEUP)
    {
        if (isDumpingFrames == TRUE) {
            D3DLOCKED_RECT r;
            char buffer[80];
            IDirect3DSurface8Impl_LockRect((LPDIRECT3DSURFACE8) This->backBuffer, &r, NULL, D3DLOCK_READONLY);
            sprintf(buffer, "/tmp/backbuffer_%ld.ppm", primCounter++);
            TRACE("Saving screenshot %s\n", buffer);
            IDirect3DSurface8Impl_SaveSnapshot((LPDIRECT3DSURFACE8) This->backBuffer, buffer);
            IDirect3DSurface8Impl_UnlockRect((LPDIRECT3DSURFACE8) This->backBuffer);
        }
    }
#endif
}
