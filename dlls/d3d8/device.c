/*
 * IDirect3DDevice8 implementation
 *
 * Copyright 2002 Jason Edmeades
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

#include <math.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* CreateVertexShader can return > 0xFFFF */
#define VS_HIGHESTFIXEDFXF 0xF0000000

/* Used for CreateStateBlock */
#define NUM_SAVEDPIXELSTATES_R     38
#define NUM_SAVEDPIXELSTATES_T     27
#define NUM_SAVEDVERTEXSTATES_R    33
#define NUM_SAVEDVERTEXSTATES_T    2

/*
 * Utility functions or macros
 */
#define conv_mat(mat,gl_mat)			\
{                                               \
    TRACE("%f %f %f %f\n", (mat)->u.s._11, (mat)->u.s._12, (mat)->u.s._13, (mat)->u.s._14); \
    TRACE("%f %f %f %f\n", (mat)->u.s._21, (mat)->u.s._22, (mat)->u.s._23, (mat)->u.s._24); \
    TRACE("%f %f %f %f\n", (mat)->u.s._31, (mat)->u.s._32, (mat)->u.s._33, (mat)->u.s._34); \
    TRACE("%f %f %f %f\n", (mat)->u.s._41, (mat)->u.s._42, (mat)->u.s._43, (mat)->u.s._44); \
    memcpy(gl_mat, (mat), 16 * sizeof(float));      \
};

/*
 * Globals
 */
extern DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R];
extern DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T];
extern DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R];
extern DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T];

/* Routine common to the draw primitive and draw indexed primitive routines
   Doesnt use gl pointer arrays as I dont believe we can support the blending
   coordinates that way.                                                      */

void DrawPrimitiveI(LPDIRECT3DDEVICE8 iface,
                    int PrimitiveType,
                    long NumPrimitives,
                    BOOL  isIndexed,

                    /* For Both:*/
                    D3DFORMAT fvf,
                    const void *vertexBufData,

                    /* for Indexed: */
                    long  StartVertexIndex,
                    long  StartIdx,
                    short idxBytes,
                    const void *idxData) {

    int vx_index;
    int NumVertexes = NumPrimitives;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* Dont understand how to handle multiple streams, but if a fixed
         FVF is passed in rather than a handle, it must use stream 0 */

    if (This->StateBlock.VertexShader > VS_HIGHESTFIXEDFXF) {
        FIXME("Cant handle created shaders yet\n");
        return;
    } else {

        int                         skip = This->StateBlock.stream_stride[0];

        BOOL                        normal;
        BOOL                        isRHW;
        BOOL                        isPtSize;
        BOOL                        isDiffuse;
        BOOL                        isSpecular;
        int                         numTextures;
        int                         textureNo;
        const void                 *curVtx = NULL;
        const short                *pIdxBufS = NULL;
        const long                 *pIdxBufL = NULL;
        const void                 *curPos;

        float x=0.0, y=0.0, z=0.0;             /* x,y,z coordinates          */
        float nx=0.0, ny=0.0, nz=0.0;          /* normal x,y,z coordinates   */
        float rhw=0.0;                         /* rhw                        */
        float ptSize=0.0;                      /* Point size                 */
        DWORD diffuseColor=0;                  /* Diffusre Color             */
        DWORD specularColor=0;                 /* Specular Color             */

        ENTER_GL();

        if (isIndexed) {
            if (idxBytes == 2) pIdxBufS = (short *) idxData;
            else pIdxBufL = (long *) idxData;
        }

        /* Check vertex formats expected ? */
        normal        = fvf & D3DFVF_NORMAL;
        isRHW         = fvf & D3DFVF_XYZRHW;
        /*numBlends     = 5 - ((~fvf) & 0xe);*/  /* There must be a simpler way? */
        isPtSize      = fvf & D3DFVF_PSIZE;
        isDiffuse     = fvf & D3DFVF_DIFFUSE;
        isSpecular    = fvf & D3DFVF_SPECULAR;
        numTextures =  (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

        TRACE("Drawing with FVF = %x, (n?%d, rhw?%d, ptSize(%d), diffuse?%d, specular?%d, numTextures=%d)\n",
              fvf, normal, isRHW, isPtSize, isDiffuse, isSpecular, numTextures);

        /* Note: Dont touch lighing anymore - it is set by the appropriate render state */

        if (isRHW) {

            double height, width, minZ, maxZ;

            /*
	         * Already transformed vertex do not need transform
	         * matrices. Reset all matrices to identity.
	         * Leave the default matrix in world mode.
	        */
            glMatrixMode(GL_PROJECTION);
            checkGLcall("glMatrixMode");
            glLoadIdentity();
            checkGLcall("glLoadIdentity");
            glMatrixMode(GL_MODELVIEW);
            checkGLcall("glMatrixMode");
            glLoadIdentity();
            checkGLcall("glLoadIdentity");
            height = This->StateBlock.viewport.Height;
            width = This->StateBlock.viewport.Width;
            minZ = This->StateBlock.viewport.MinZ;
            maxZ = This->StateBlock.viewport.MaxZ;
            TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, -minZ, -maxZ);
            glOrtho(0.0, width, height, 0.0, -minZ, -maxZ);
            checkGLcall("glOrtho");

        } else {
            glMatrixMode(GL_PROJECTION);
	        checkGLcall("glMatrixMode");
            glLoadMatrixf((float *) &This->StateBlock.transforms[D3DTS_PROJECTION].u.m[0][0]);
            checkGLcall("glLoadMatrixf");

            glMatrixMode(GL_MODELVIEW);
            checkGLcall("glMatrixMode");
            glLoadMatrixf((float *) &This->StateBlock.transforms[D3DTS_VIEW].u.m[0][0]);
            checkGLcall("glLoadMatrixf");
            glMultMatrixf((float *) &This->StateBlock.transforms[D3DTS_WORLDMATRIX(0)].u.m[0][0]);
            checkGLcall("glMultMatrixf");
        }

        /* Set OpenGL to the appropriate Primitive Type */
        switch (PrimitiveType) {
        case D3DPT_POINTLIST:
            TRACE("glBegin, Start POINTS\n");
            glBegin(GL_POINTS);
            NumVertexes = NumPrimitives;
            break;

        case D3DPT_LINELIST:
            TRACE("glBegin, Start LINES\n");
            glBegin(GL_LINES);
            NumVertexes = NumPrimitives * 2;
            break;

        case D3DPT_LINESTRIP:
            TRACE("glBegin, Start LINE_STRIP\n");
            glBegin(GL_LINE_STRIP);
            NumVertexes = NumPrimitives + 1;
            break;

        case D3DPT_TRIANGLELIST:
            TRACE("glBegin, Start TRIANGLES\n");
            glBegin(GL_TRIANGLES);
            NumVertexes = NumPrimitives * 3;
            break;

        case D3DPT_TRIANGLESTRIP:
            TRACE("glBegin, Start TRIANGLE_STRIP\n");
            glBegin(GL_TRIANGLE_STRIP);
            NumVertexes = NumPrimitives + 2;
            break;

        case D3DPT_TRIANGLEFAN:
            TRACE("glBegin, Start TRIANGLE_FAN\n");
            glBegin(GL_TRIANGLE_FAN);
            NumVertexes = NumPrimitives + 2;
            break;

        default:
            FIXME("Unhandled primitive\n");
            break;
        }


        /* Draw the primitives */
        curVtx = vertexBufData + (StartVertexIndex * skip);

        for (vx_index = 0; vx_index < NumVertexes; vx_index++) {

            if (!isIndexed) {
                curPos = curVtx;
            } else {
                if (idxBytes == 2) {
                    TRACE("Idx for vertex %d = %d = %d\n", vx_index, pIdxBufS[StartIdx+vx_index], (pIdxBufS[StartIdx+vx_index]));
                    curPos = curVtx + ((pIdxBufS[StartIdx+vx_index]) * skip);
                } else {
                    TRACE("Idx for vertex %d = %ld = %d\n", vx_index, pIdxBufL[StartIdx+vx_index], (pIdxBufS[StartIdx+vx_index]));
                    curPos = curVtx + ((pIdxBufL[StartIdx+vx_index]) * skip);
                }
            }

            /* Work through the vertex buffer */
            x = *(float *)curPos;
            curPos = curPos + sizeof(float);
            y = *(float *)curPos;
            curPos = curPos + sizeof(float);
            z = *(float *)curPos;
            curPos = curPos + sizeof(float);
            TRACE("x,y,z=%f,%f,%f\n", x,y,z);

            /* RHW follows, only if transformed */
            if (isRHW) {
                rhw = *(float *)curPos;
                curPos = curPos + sizeof(float);
                TRACE("rhw=%f\n", rhw);
            }


            /* FIXME: Skip Blending data */

            /* Vertex Normal Data (untransformed only) */
            if (normal) {
                nx = *(float *)curPos;
                curPos = curPos + sizeof(float);
                ny = *(float *)curPos;
                curPos = curPos + sizeof(float);
                nz = *(float *)curPos;
                curPos = curPos + sizeof(float);
                TRACE("nx,ny,nz=%f,%f,%f\n", nx,ny,nz);
            }

            if (isPtSize) {
                ptSize = *(float *)curPos;
                curPos = curPos + sizeof(float);
                TRACE("ptSize=%f\n", ptSize);
            }
            if (isDiffuse) {
                diffuseColor = *(DWORD *)curPos;
                TRACE("diffuseColor=%lx\n", diffuseColor);
                curPos = curPos + sizeof(DWORD);
            }
            if (isSpecular) {
                specularColor = *(DWORD *)curPos;
                TRACE("specularColor=%lx\n", specularColor);
                curPos = curPos + sizeof(DWORD);
            }

            /* ToDo: Texture coords */
            for (textureNo = 0;textureNo<numTextures; textureNo++) {

                float s,t,r,q;

                /* Query tex coords */
                switch (IDirect3DBaseTexture8Impl_GetType((LPDIRECT3DBASETEXTURE8) This->StateBlock.textures[0])) {
                case D3DRTYPE_TEXTURE:
                    s = *(float *)curPos;
                    curPos = curPos + sizeof(float);
                    t = *(float *)curPos;
                    curPos = curPos + sizeof(float);
                    TRACE("tex:%d, s,t=%f,%f\n", textureNo, s,t);
                    glMultiTexCoord2fARB(GL_TEXTURE0_ARB + textureNo, s, t);
                    break;

                case D3DRTYPE_VOLUMETEXTURE:
                    s = *(float *)curPos;
                    curPos = curPos + sizeof(float);
                    t = *(float *)curPos;
                    curPos = curPos + sizeof(float);
                    r = *(float *)curPos;
                    curPos = curPos + sizeof(float);
                    TRACE("tex:%d, s,t,r=%f,%f,%f\n", textureNo, s,t,r);
                    glMultiTexCoord3fARB(GL_TEXTURE0_ARB + textureNo, s, t, r);
                    break;

                default:
                    r=0;q=0; /* Avoid compiler warnings, need these vars later for other textures */
                    FIXME("Unhandled texture type\n");
                }

            }

            /* Handle these vertexes */
            if (isDiffuse) {
                glColor4f(((diffuseColor >> 16) & 0xFF) / 255.0,
                          ((diffuseColor >>  8) & 0xFF) / 255.0,
                          ((diffuseColor >>  0) & 0xFF) / 255.0,
                          ((diffuseColor >> 24) & 0xFF) / 255.0);
                TRACE("glColor4f: r,g,b,a=%f,%f,%f,%f\n", ((diffuseColor >> 16) & 0xFF) / 255.0, ((diffuseColor >>  8) & 0xFF) / 255.0,
                       ((diffuseColor >>  0) & 0xFF) / 255.0, ((diffuseColor >> 24) & 0xFF) / 255.0);
            }

            if (normal) {
                TRACE("Vertex: glVertex:x,y,z=%f,%f,%f  /  glNormal:nx,ny,nz=%f,%f,%f\n", x,y,z,nx,ny,nz);
                glNormal3f(nx, ny, nz);
                glVertex3f(x, y, z);

            } else {
                if (rhw < 0.01) {
                    TRACE("Vertex: glVertex:x,y,z=%f,%f,%f\n", x,y,z);
                    glVertex3f(x, y, z);
                } else {
                    TRACE("Vertex: glVertex:x,y,z=%f,%f,%f / rhw=%f\n", x,y,z,rhw);
                    glVertex4f(x / rhw, y / rhw, z / rhw, 1.0 / rhw);
                }
            }

            if (!isIndexed) {
                curVtx = curVtx + skip;
            }
        }
    }
    glEnd();
    checkGLcall("glEnd and previous calls");
    LEAVE_GL();

    TRACE("glEnd\n");
}

/*
    Simple utility routines used for dx -> gl mapping of byte formats
 */
SHORT bytesPerPixel(D3DFORMAT fmt) {
    SHORT retVal;

    switch (fmt) {
    case D3DFMT_A4R4G4B4:         retVal = 2; break;
    case D3DFMT_A8R8G8B8:         retVal = 4; break;
    case D3DFMT_X8R8G8B8:         retVal = 4; break;
    case D3DFMT_R8G8B8:           retVal = 3; break;
    case D3DFMT_R5G6B5:           retVal = 2; break;
    default:
        FIXME("Unhandled fmt %d\n", fmt);
        retVal = 4;
    }


    TRACE("bytes/Pxl for fmt %d = %d\n", fmt, retVal);
    return retVal;
}

GLint fmt2glintFmt(D3DFORMAT fmt) {
    GLint retVal;

    switch (fmt) {
    case D3DFMT_A4R4G4B4:         retVal = GL_RGBA4; break;
    case D3DFMT_A8R8G8B8:         retVal = GL_RGBA8; break;
    case D3DFMT_X8R8G8B8:         retVal = GL_RGB8; break;
    case D3DFMT_R8G8B8:           retVal = GL_RGB8; break;
    case D3DFMT_R5G6B5:           retVal = GL_RGB5; break; /* fixme: internal format 6 for g? */
    default:
        FIXME("Unhandled fmt %d\n", fmt);
        retVal = 4;
    }
    TRACE("fmt2glintFmt for fmt %d = %x\n", fmt, retVal);
    return retVal;
}
GLenum fmt2glFmt(D3DFORMAT fmt) {
    GLenum retVal;

    switch (fmt) {
    case D3DFMT_A4R4G4B4:         retVal = GL_BGRA; break;
    case D3DFMT_A8R8G8B8:         retVal = GL_BGRA; break;
    case D3DFMT_X8R8G8B8:         retVal = GL_BGRA; break;
    case D3DFMT_R8G8B8:           retVal = GL_BGR; break;
    case D3DFMT_R5G6B5:           retVal = GL_BGR; break;
    default:
        FIXME("Unhandled fmt %d\n", fmt);
        retVal = 4;
    }
    TRACE("fmt2glFmt for fmt %d = %x\n", fmt, retVal);
    return retVal;
}
DWORD fmt2glType(D3DFORMAT fmt) {
    GLenum retVal;

    switch (fmt) {
    case D3DFMT_A4R4G4B4:         retVal = GL_UNSIGNED_SHORT_4_4_4_4_REV; break;
    case D3DFMT_A8R8G8B8:         retVal = GL_UNSIGNED_BYTE; break;
    case D3DFMT_X8R8G8B8:         retVal = GL_UNSIGNED_BYTE; break;
    case D3DFMT_R5G6B5:           retVal = GL_UNSIGNED_SHORT_5_6_5_REV; break;
    case D3DFMT_R8G8B8:           retVal = GL_UNSIGNED_BYTE; break;
    default:
        FIXME("Unhandled fmt %d\n", fmt);
        retVal = 4;
    }


    TRACE("fmt2glType for fmt %d = %x\n", fmt, retVal);
    return retVal;
}

int SOURCEx_RGB_EXT(DWORD arg) {
    switch(arg) {
    case D3DTSS_COLORARG0: return GL_SOURCE2_RGB_EXT;
    case D3DTSS_COLORARG1: return GL_SOURCE0_RGB_EXT;
    case D3DTSS_COLORARG2: return GL_SOURCE1_RGB_EXT;
    case D3DTSS_ALPHAARG0:
    case D3DTSS_ALPHAARG1:
    case D3DTSS_ALPHAARG2:
    default:
        FIXME("Invalid arg %ld\n", arg);
        return GL_SOURCE0_RGB_EXT;
    }
}
int OPERANDx_RGB_EXT(DWORD arg) {
    switch(arg) {
    case D3DTSS_COLORARG0: return GL_OPERAND2_RGB_EXT;
    case D3DTSS_COLORARG1: return GL_OPERAND0_RGB_EXT;
    case D3DTSS_COLORARG2: return GL_OPERAND1_RGB_EXT;
    case D3DTSS_ALPHAARG0:
    case D3DTSS_ALPHAARG1:
    case D3DTSS_ALPHAARG2:
    default:
        FIXME("Invalid arg %ld\n", arg);
        return GL_OPERAND0_RGB_EXT;
    }
}
int SOURCEx_ALPHA_EXT(DWORD arg) {
    switch(arg) {
    case D3DTSS_ALPHAARG0:  return GL_SOURCE2_ALPHA_EXT;
    case D3DTSS_ALPHAARG1:  return GL_SOURCE0_ALPHA_EXT;
    case D3DTSS_ALPHAARG2:  return GL_SOURCE1_ALPHA_EXT;
    case D3DTSS_COLORARG0:
    case D3DTSS_COLORARG1:
    case D3DTSS_COLORARG2:
    default:
        FIXME("Invalid arg %ld\n", arg);
        return GL_SOURCE0_ALPHA_EXT;
    }
}
int OPERANDx_ALPHA_EXT(DWORD arg) {
    switch(arg) {
    case D3DTSS_ALPHAARG0:  return GL_OPERAND2_ALPHA_EXT;
    case D3DTSS_ALPHAARG1:  return GL_OPERAND0_ALPHA_EXT;
    case D3DTSS_ALPHAARG2:  return GL_OPERAND1_ALPHA_EXT;
    case D3DTSS_COLORARG0:
    case D3DTSS_COLORARG1:
    case D3DTSS_COLORARG2:
    default:
        FIXME("Invalid arg %ld\n", arg);
        return GL_OPERAND0_ALPHA_EXT;
    }
}
GLenum StencilOp(DWORD op) {
    switch(op) {                
    case D3DSTENCILOP_KEEP    : return GL_KEEP;
    case D3DSTENCILOP_ZERO    : return GL_ZERO;
    case D3DSTENCILOP_REPLACE : return GL_REPLACE;
    case D3DSTENCILOP_INCRSAT : return GL_INCR;
    case D3DSTENCILOP_DECRSAT : return GL_DECR;
    case D3DSTENCILOP_INVERT  : return GL_INVERT;
    case D3DSTENCILOP_INCR    : return GL_INCR; /* Fixme - needs to support wrap */
    case D3DSTENCILOP_DECR    : return GL_DECR; /* Fixme - needs to support wrap */
    default:
        FIXME("Invalid stencil op %ld\n", op);
        return GL_ALWAYS;
    }
}

/* IDirect3D IUnknown parts follow: */
HRESULT WINAPI IDirect3DDevice8Impl_QueryInterface(LPDIRECT3DDEVICE8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory)) {
        IDirect3DDevice8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DDevice8Impl_AddRef(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DDevice8Impl_Release(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DDevice Interface follow: */
HRESULT  WINAPI  IDirect3DDevice8Impl_TestCooperativeLevel(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : stub\n", This);    /* No way of notifying yet! */
    return D3D_OK;
}


UINT     WINAPI  IDirect3DDevice8Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : stub, emulating 32Mb for now\n", This);
    /*
     * pretend we have 32MB of any type of memory queried.
     */
    return (1024*1024*32);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_ResourceManagerDiscardBytes(LPDIRECT3DDEVICE8 iface, DWORD Bytes) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetDirect3D(LPDIRECT3DDEVICE8 iface, IDirect3D8** ppD3D8) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->direct3d8);

    /* Inc ref count */
    IDirect3D8_AddRef((LPDIRECT3D8) This->direct3d8);

    *ppD3D8 = (IDirect3D8 *)This->direct3d8;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetDeviceCaps(LPDIRECT3DDEVICE8 iface, D3DCAPS8* pCaps) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub, calling idirect3d for now\n", This);
    IDirect3D8Impl_GetDeviceCaps((LPDIRECT3D8) This->direct3d8, This->adapterNo, This->devType, pCaps);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetDisplayMode(LPDIRECT3DDEVICE8 iface, D3DDISPLAYMODE* pMode) {

    HDC hdc;
    int bpp = 0;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    pMode->Width        = GetSystemMetrics(SM_CXSCREEN);
    pMode->Height       = GetSystemMetrics(SM_CYSCREEN);
    pMode->RefreshRate  = 85; /*FIXME: How to identify? */

    hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    bpp = GetDeviceCaps(hdc, BITSPIXEL);
    DeleteDC(hdc);

    switch (bpp) {
    case  8: pMode->Format       = D3DFMT_R8G8B8; break;
    case 16: pMode->Format       = D3DFMT_R5G6B5; break;
    case 24: pMode->Format       = D3DFMT_R8G8B8; break;
    case 32: pMode->Format       = D3DFMT_A8R8G8B8; break;
    default: pMode->Format       = D3DFMT_UNKNOWN;
    }

    FIXME("(%p) : returning w(%d) h(%d) rr(%d) fmt(%d)\n", This, pMode->Width, pMode->Height, pMode->RefreshRate, pMode->Format);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetCreationParameters(LPDIRECT3DDEVICE8 iface, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) copying to %p\n", This, pParameters);    
    memcpy(pParameters, &This->CreateParms, sizeof(D3DDEVICE_CREATION_PARAMETERS));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetCursorProperties(LPDIRECT3DDEVICE8 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
void     WINAPI  IDirect3DDevice8Impl_SetCursorPosition(LPDIRECT3DDEVICE8 iface, UINT XScreenSpace, UINT YScreenSpace,DWORD Flags) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return;
}
BOOL     WINAPI  IDirect3DDevice8Impl_ShowCursor(LPDIRECT3DDEVICE8 iface, BOOL bShow) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_Reset(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_Present(LPDIRECT3DDEVICE8 iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : complete stub!\n", This);

    ENTER_GL();

    glXSwapBuffers(This->display, This->win);
    checkGLcall("glXSwapBuffers");

    LEAVE_GL();

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetBackBuffer(LPDIRECT3DDEVICE8 iface, UINT BackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface8** ppBackBuffer) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    *ppBackBuffer = (LPDIRECT3DSURFACE8) This->backBuffer;
    TRACE("(%p) : BackBuf %d Type %d returning %p\n", This, BackBuffer, Type, *ppBackBuffer);

    /* Note inc ref on returned surface */
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppBackBuffer);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetRasterStatus(LPDIRECT3DDEVICE8 iface, D3DRASTER_STATUS* pRasterStatus) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
void     WINAPI  IDirect3DDevice8Impl_SetGammaRamp(LPDIRECT3DDEVICE8 iface, DWORD Flags,CONST D3DGAMMARAMP* pRamp) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return;
}
void     WINAPI  IDirect3DDevice8Impl_GetGammaRamp(LPDIRECT3DDEVICE8 iface, D3DGAMMARAMP* pRamp) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateTexture(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                                    D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture8** ppTexture) {
    IDirect3DTexture8Impl *object;
    int i;
    UINT tmpW;
    UINT tmpH;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* Allocate the storage for the device */
    TRACE("(%p) : W(%d) H(%d), Lvl(%d) Usage(%ld), Fmt(%d), Pool(%d)\n", This, Width, Height, Levels, Usage, Format, Pool);
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DTexture8Impl));
    object->lpVtbl = &Direct3DTexture8_Vtbl;
    object->Device = This;
    object->ResourceType = D3DRTYPE_TEXTURE;
    object->ref = 1;
    object->width = Width;
    object->height = Height;
    object->levels = Levels;
    object->usage = Usage;
    object->format = Format;
    object->device = This;

    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        object->levels++;
        tmpW = Width;
        tmpH = Height;
        while (tmpW > 1 && tmpH > 1) {
            tmpW = max(1,tmpW / 2);
            tmpH = max(1, tmpH / 2);
            object->levels++;
        }
        TRACE("Calculated levels = %d\n", object->levels);
    }

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    /*for (i=0; i<object->levels; i++) { */
    i=0;
    {
        IDirect3DDevice8Impl_CreateImageSurface(iface, tmpW, tmpH, Format, (LPDIRECT3DSURFACE8*) &object->surfaces[i]);
        object->surfaces[i]->Container = object;
        object->surfaces[i]->myDesc.Usage = Usage;
        object->surfaces[i]->myDesc.Pool = Pool ;

        TRACE("Created surface level %d @ %p, memory at %p\n", i, object->surfaces[i], object->surfaces[i]->allocatedMemory);
        tmpW = max(1,tmpW / 2);
        tmpH = max(1, tmpH / 2);
    }

    *ppTexture = (LPDIRECT3DTEXTURE8)object;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVolumeTexture(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture8** ppVolumeTexture) {

    IDirect3DVolumeTexture8Impl *object;
    int i;
    UINT tmpW;
    UINT tmpH;
    UINT tmpD;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* Allocate the storage for it */
    TRACE("(%p) : W(%d) H(%d) D(%d), Lvl(%d) Usage(%ld), Fmt(%d), Pool(%d)\n", This, Width, Height, Depth, Levels, Usage, Format, Pool);
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolumeTexture8Impl));
    object->lpVtbl = &Direct3DVolumeTexture8_Vtbl;
    object->ResourceType = D3DRTYPE_VOLUMETEXTURE;
    object->ref = 1;


    object->width = Width;
    object->height = Height;
    object->depth = Depth;
    object->levels = Levels;
    object->usage = Usage;
    object->format = Format;
    object->device = This;

    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        object->levels++;
        tmpW = Width;
        tmpH = Height;
        tmpD = Depth;
        while (tmpW > 1 && tmpH > 1 && tmpD > 1) {
            tmpW = max(1,tmpW / 2);
            tmpH = max(1, tmpH / 2);
            tmpD = max(1, tmpD / 2);
            object->levels++;
        }
        TRACE("Calculated levels = %d\n", object->levels);
    }

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    tmpD = Depth;

    /*for (i=0; i<object->levels; i++) { */
    i=0;
    {
        IDirect3DVolume8Impl *volume;

        /* Create the volume - No entry point for this seperately?? */
        volume  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolume8Impl));
        object->volumes[i] = (IDirect3DVolume8Impl *) volume;

        volume->lpVtbl = &Direct3DVolume8_Vtbl;
        volume->Device = This;
        volume->ResourceType = D3DRTYPE_VOLUME;
        volume->Container = object;
        volume->ref = 1;

        volume->myDesc.Width = Width;
        volume->myDesc.Height= Height;
        volume->myDesc.Depth = Depth;
        volume->myDesc.Format= Format;
        volume->myDesc.Type  = D3DRTYPE_VOLUME;
        volume->myDesc.Pool  = Pool;
        volume->myDesc.Usage = Usage;
        volume->bytesPerPixel   = bytesPerPixel(Format);
        volume->myDesc.Size     = (Width * volume->bytesPerPixel) * Height * Depth;
        volume->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, volume->myDesc.Size);

        TRACE("(%p) : Volume at w(%d) h(%d) d(%d) fmt(%d) surf@%p, surfmem@%p, %d bytes\n", This, Width, Height, Depth, Format, 
                  volume, volume->allocatedMemory, volume->myDesc.Size);

        tmpW = max(1,tmpW / 2);
        tmpH = max(1, tmpH / 2);
        tmpD = max(1, tmpD / 2);
    }

    *ppVolumeTexture = (LPDIRECT3DVOLUMETEXTURE8)object;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateCubeTexture(LPDIRECT3DDEVICE8 iface, UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture8** ppCubeTexture) {

    IDirect3DCubeTexture8Impl *object;
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    int i,j;
    UINT tmpW;

    /* Allocate the storage for it */
    TRACE("(%p) : Len(%d), Lvl(%d) Usage(%ld), Fmt(%d), Pool(%d)\n", This, EdgeLength, Levels, Usage, Format, Pool);
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DCubeTexture8Impl));
    object->lpVtbl = &Direct3DCubeTexture8_Vtbl;
    object->ref = 1;
    object->Device = This;
    object->ResourceType = D3DRTYPE_CUBETEXTURE;

    object->edgeLength = EdgeLength;
    object->levels = Levels;
    object->usage = Usage;
    object->format = Format;
    object->device = This;

    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        object->levels++;
        tmpW = EdgeLength;
        while (tmpW > 1) {
            tmpW = max(1,tmpW / 2);
            object->levels++;
        }
        TRACE("Calculated levels = %d\n", object->levels);
    }

    /* Generate all the surfaces */
    tmpW = EdgeLength;
    /*for (i=0; i<object->levels; i++) { */
    i=0;
    {
        /* Create the 6 faces */
        for (j=0;j<6;j++) {
           IDirect3DDevice8Impl_CreateImageSurface(iface, tmpW, tmpW, Format, (LPDIRECT3DSURFACE8*) &object->surfaces[j][i]);
           object->surfaces[j][i]->Container = object;
           object->surfaces[j][i]->myDesc.Usage = Usage;
           object->surfaces[j][i]->myDesc.Pool = Pool ;

           TRACE("Created surface level %d @ %p, memory at %p\n", i, object->surfaces[j][i], object->surfaces[j][i]->allocatedMemory);
           tmpW = max(1,tmpW / 2);
        }
    }

    TRACE("(%p) : Iface@%p\n", This, object);
    *ppCubeTexture = (LPDIRECT3DCUBETEXTURE8)object;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexBuffer(LPDIRECT3DDEVICE8 iface, UINT Size,DWORD Usage,
                                                         DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer8** ppVertexBuffer) {
    IDirect3DVertexBuffer8Impl *object;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexBuffer8Impl));
    object->lpVtbl = &Direct3DVertexBuffer8_Vtbl;
    object->Device = This;
    object->ResourceType = D3DRTYPE_VERTEXBUFFER;
    object->ref = 1;
    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
    object->currentDesc.Usage = Usage;
    object->currentDesc.Pool  = Pool;
    object->currentDesc.FVF   = FVF;
    object->currentDesc.Size  = Size;

    TRACE("(%p) : Size=%d, Usage=%ld, FVF=%lx, Pool=%d - Memory@%p, Iface@%p\n", This, Size, Usage, FVF, Pool, object->allocatedMemory, object);

    *ppVertexBuffer = (LPDIRECT3DVERTEXBUFFER8)object;

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateIndexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer8** ppIndexBuffer) {

    IDirect3DIndexBuffer8Impl *object;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Len=%d, Use=%lx, Format=%x, Pool=%d\n", This, Length, Usage, Format, Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DIndexBuffer8Impl));
    object->lpVtbl = &Direct3DIndexBuffer8_Vtbl;
    object->ref = 1;
    object->Device = This;
    object->ResourceType = D3DRTYPE_INDEXBUFFER;

    object->currentDesc.Type = D3DRTYPE_INDEXBUFFER;
    object->currentDesc.Usage = Usage;
    object->currentDesc.Pool  = Pool;
    object->currentDesc.Format  = Format;
    object->currentDesc.Size  = Length;

    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length);

    TRACE("(%p) : Iface@%p allocatedMem @ %p\n", This, object, object->allocatedMemory);

    *ppIndexBuffer = (LPDIRECT3DINDEXBUFFER8)object;

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateRenderTarget(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,BOOL Lockable,IDirect3DSurface8** ppSurface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    /* up ref count on surface, surface->container = This */
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,IDirect3DSurface8** ppSurface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    /* surface->container = This */
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateImageSurface(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,D3DFORMAT Format,IDirect3DSurface8** ppSurface) {
    IDirect3DSurface8Impl *object;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    object  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    *ppSurface = (LPDIRECT3DSURFACE8) object;
    object->lpVtbl = &Direct3DSurface8_Vtbl;
    object->Device = This;
    object->ResourceType = D3DRTYPE_SURFACE;
    object->Container = This;

    object->ref = 1;
    object->myDesc.Width = Width;
    object->myDesc.Height= Height;
    object->myDesc.Format= Format;
    object->myDesc.Type = D3DRTYPE_SURFACE;
    /*object->myDesc.Usage */
    object->myDesc.Pool = D3DPOOL_SYSTEMMEM ;
    object->bytesPerPixel = bytesPerPixel(Format);
    object->myDesc.Size = (Width * object->bytesPerPixel) * Height;
    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->myDesc.Size);

    TRACE("(%p) : w(%d) h(%d) fmt(%d) surf@%p, surfmem@%p, %d bytes\n", This, Width, Height, Format, *ppSurface, object->allocatedMemory, object->myDesc.Size);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CopyRects(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,
                                                IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray) {

    HRESULT rc = D3D_OK;

    IDirect3DSurface8Impl *src = (IDirect3DSurface8Impl*) pSourceSurface;
    IDirect3DSurface8Impl *dst = (IDirect3DSurface8Impl*) pDestinationSurface;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) srcsur=%p, pSourceRects=%p, cRects=%d, pDstSur=%p, pDestPtsArr=%p\n", This,
          pSourceSurface, pSourceRectsArray, cRects, pDestinationSurface, pDestPointsArray);

    if (src->myDesc.Format != dst->myDesc.Format) {
        TRACE("Formats do not match %x / %x\n", src->myDesc.Format, dst->myDesc.Format);
        rc = D3DERR_INVALIDCALL;
    }

    /* Quick if complete copy ... */
    if (rc == D3D_OK && (cRects == 0 && pSourceRectsArray==NULL && pDestPointsArray==NULL &&
                         src->myDesc.Width == dst->myDesc.Width &&
                         src->myDesc.Height == dst->myDesc.Height)) {
        TRACE("Direct copy as surfaces are equal, w=%d, h=%d\n", dst->myDesc.Width, dst->myDesc.Height);
        memcpy(dst->allocatedMemory, src->allocatedMemory, src->myDesc.Size);

    } else {
        FIXME("(%p) : stub\n", This);
    }
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_UpdateTexture(LPDIRECT3DDEVICE8 iface, IDirect3DBaseTexture8* pSourceTexture,IDirect3DBaseTexture8* pDestinationTexture) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetFrontBuffer(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pDestSurface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pRenderTarget,IDirect3DSurface8* pNewZStencil) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppRenderTarget) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    /*TRACE("(%p) : returning %p\n", This, This->renderTarget); */
    FIXME("(%p) : stub\n", This);

    /*
     **ppRenderTarget = (LPDIRECT3DSURFACE8) This->renderTarget;
     *IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppRenderTarget);
     */

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppZStencilSurface) {

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* Note inc ref on returned surface *
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppBackBuffer); */

    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_BeginScene(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_EndScene(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p)\n", This);

    ENTER_GL();

    glFlush();
    checkGLcall("glFlush");

    /* Useful for debugging sometimes!
    printf("Hit Enter ...\n");
    getchar(); */

    LEAVE_GL();
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_Clear(LPDIRECT3DDEVICE8 iface, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* TODO: From MSDN This method fails if you specify the D3DCLEAR_ZBUFFER or D3DCLEAR_STENCIL flags when the
      render target does not have an attached depth buffer. Similarly, if you specify the D3DCLEAR_STENCIL flag
      when the depth-buffer format does not contain stencil buffer information, this method fails. */
    GLbitfield glMask = 0;
    int i;
    CONST D3DRECT   *curRect;

    TRACE("(%p) Count (%ld), pRects (%p), Flags (%lx), Z (%f), Stencil (%ld)\n", This,
          Count, pRects, Flags, Z, Stencil);

    ENTER_GL();
    if (Count > 0 && pRects) {
        glEnable(GL_SCISSOR_TEST);
        checkGLcall("glEnable GL_SCISSOR_TEST");
        curRect = pRects;
    } else {
        curRect = NULL;
    }

    for (i=0;i<Count || i==0; i++) {

        if (curRect) {
            /* Note gl uses lower left, width/height */
            TRACE("(%p) %p Rect=(%ld,%ld)->(%ld,%ld) glRect=(%ld,%ld), len=%ld, hei=%ld\n", This, curRect,
                  curRect->x1, curRect->y1, curRect->x2, curRect->y2,
                  curRect->x1, curRect->y2, curRect->x2 - curRect->x1, curRect->y2 - curRect->y1);
            glScissor(curRect->x1, curRect->y2, curRect->x2 - curRect->x1, curRect->y2 - curRect->y1);
            checkGLcall("glScissor");
        } else {
            TRACE("Clearing screen with glClear to color %lx\n", Color);
        }

        /* Clear the whole screen */
        if (Flags & D3DCLEAR_STENCIL) {
            glClearStencil(Stencil);
            checkGLcall("glClearStencil");
            glMask = glMask | GL_STENCIL_BUFFER_BIT;
        }

        if (Flags & D3DCLEAR_ZBUFFER) {
            glClearDepth(Z);
            checkGLcall("glClearDepth");
            glMask = glMask | GL_DEPTH_BUFFER_BIT;
        }

        if (Flags & D3DCLEAR_TARGET) {
            glClearColor(((Color >> 16) & 0xFF) / 255.0, ((Color >>  8) & 0xFF) / 255.0,
                         ((Color >>  0) & 0xFF) / 255.0, ((Color >> 24) & 0xFF) / 255.0);
            checkGLcall("glClearColor");
            glMask = glMask | GL_COLOR_BUFFER_BIT;
        }

        glClear(glMask);
        checkGLcall("glClear");

        if (curRect) curRect = curRect + sizeof(D3DRECT);
    }

    if (Count > 0 && pRects) {
        glDisable(GL_SCISSOR_TEST);
        checkGLcall("glDisable");
    }
    LEAVE_GL();

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE d3dts,CONST D3DMATRIX* lpmatrix) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    int k;

    /* Most of this routine, comments included copied from ddraw tree initially: */
    TRACE("(%p) : State=%d\n", This, d3dts);

    This->UpdateStateBlock->Changed.transform[d3dts] = TRUE;
    This->UpdateStateBlock->Set.transform[d3dts] = TRUE;
    memcpy(&This->UpdateStateBlock->transforms[d3dts], lpmatrix, sizeof(D3DMATRIX));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /*
       ScreenCoord = ProjectionMat * ViewMat * WorldMat * ObjectCoord

       where ViewMat = Camera space, WorldMat = world space.

       In OpenGL, camera and world space is combined into GL_MODELVIEW
       matrix.  The Projection matrix stay projection matrix. */

    /* After reading through both OpenGL and Direct3D documentations, I
       thought that D3D matrices were written in 'line major mode' transposed
       from OpenGL's 'column major mode'. But I found out that a simple memcpy
       works fine to transfer one matrix format to the other (it did not work
       when transposing)....

       So :
        1) are the documentations wrong
        2) does the matrix work even if they are not read correctly
        3) is Mesa's implementation of OpenGL not compliant regarding Matrix
           loading using glLoadMatrix ?

       Anyway, I always use 'conv_mat' to transfer the matrices from one format
       to the other so that if I ever find out that I need to transpose them, I
       will able to do it quickly, only by changing the macro conv_mat. */

    switch (d3dts) {
    case D3DTS_WORLDMATRIX(0): {
            conv_mat(lpmatrix, &This->StateBlock.transforms[D3DTS_WORLDMATRIX(0)]);
        } break;

    case D3DTS_VIEW: {
            conv_mat(lpmatrix, &This->StateBlock.transforms[D3DTS_VIEW]);
        } break;

    case D3DTS_PROJECTION: {
            conv_mat(lpmatrix, &This->StateBlock.transforms[D3DTS_PROJECTION]);
        } break;

    default:
        break;
    }

    /*
     * Move the GL operation to outside of switch to make it work
     * regardless of transform set order. Optimize later.
     */
    ENTER_GL();
    glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode");
    glLoadMatrixf((float *) &This->StateBlock.transforms[D3DTS_PROJECTION].u.m[0][0]);
    checkGLcall("glLoadMatrixf");

    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode");
    glLoadMatrixf((float *) &This->StateBlock.transforms[D3DTS_VIEW].u.m[0][0]);
    checkGLcall("glLoadMatrixf");

    /* If we are changing the View matrix, reset the light information to the new view */
    if (d3dts == D3DTS_VIEW) {
        for (k = 0; k < MAX_ACTIVE_LIGHTS; k++) {
            glLightfv(GL_LIGHT0 + k, GL_POSITION,       &This->lightPosn[k][0]);
            checkGLcall("glLightfv posn");
            glLightfv(GL_LIGHT0 + k, GL_SPOT_DIRECTION, &This->lightDirn[k][0]);
            checkGLcall("glLightfv dirn");
        }
    }

    glMultMatrixf((float *) &This->StateBlock.transforms[D3DTS_WORLDMATRIX(0)].u.m[0][0]);
    checkGLcall("glMultMatrixf");

    LEAVE_GL();

    return D3D_OK;

}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : for State %d\n", This, State);
    memcpy(pMatrix, &This->StateBlock.transforms[State], sizeof(D3DMATRIX));
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_MultiplyTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetViewport(LPDIRECT3DDEVICE8 iface, CONST D3DVIEWPORT8* pViewport) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    TRACE("(%p)\n", This);
    This->UpdateStateBlock->Changed.viewport = TRUE;
    This->UpdateStateBlock->Set.viewport = TRUE;
    memcpy(&This->UpdateStateBlock->viewport, pViewport, sizeof(D3DVIEWPORT8));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    TRACE("(%p) : x=%ld, y=%ld, wid=%ld, hei=%ld, minz=%f, maxz=%f\n", This,
          pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height, pViewport->MinZ, pViewport->MaxZ);

    glDepthRange(pViewport->MinZ, pViewport->MaxZ);
    checkGLcall("glDepthRange");
    /* Fixme? Note GL requires lower left, DirectX supplies upper left */
    glViewport(pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height);
    checkGLcall("glViewport");


    return D3D_OK;

}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetViewport(LPDIRECT3DDEVICE8 iface, D3DVIEWPORT8* pViewport) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p)\n", This);
    memcpy(pViewport, &This->StateBlock.viewport, sizeof(D3DVIEWPORT8));
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetMaterial(LPDIRECT3DDEVICE8 iface, CONST D3DMATERIAL8* pMaterial) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    This->UpdateStateBlock->Changed.material = TRUE;
    This->UpdateStateBlock->Set.material = TRUE;
    memcpy(&This->UpdateStateBlock->material, pMaterial, sizeof(D3DMATERIAL8));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    ENTER_GL();
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g, pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g, pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g, pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g, pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float *)&This->UpdateStateBlock->material.Ambient);
    checkGLcall("glMaterialfv");
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float *)&This->UpdateStateBlock->material.Diffuse);
    checkGLcall("glMaterialfv");

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float *)&This->UpdateStateBlock->material.Specular);
    checkGLcall("glMaterialfv");
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float *)&This->UpdateStateBlock->material.Emissive);
    checkGLcall("glMaterialfv");
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, This->UpdateStateBlock->material.Power);
    checkGLcall("glMaterialf");

    LEAVE_GL();
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetMaterial(LPDIRECT3DDEVICE8 iface, D3DMATERIAL8* pMaterial) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    memcpy(pMaterial, &This->UpdateStateBlock->material, sizeof (D3DMATERIAL8));
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g, pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g, pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g, pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g, pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetLight(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST D3DLIGHT8* pLight) {
    float colRGBA[] = {0.0, 0.0, 0.0, 0.0};
    float rho;
    float quad_att;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Idx(%ld), pLight(%p)\n", This, Index, pLight);

    TRACE("Light %ld setting to type %d, Diffuse(%f,%f,%f,%f), Specular(%f,%f,%f,%f), Ambient(%f,%f,%f,%f)\n", Index, pLight->Type,
          pLight->Diffuse.r, pLight->Diffuse.g, pLight->Diffuse.b, pLight->Diffuse.a,
          pLight->Specular.r, pLight->Specular.g, pLight->Specular.b, pLight->Specular.a,
          pLight->Ambient.r, pLight->Ambient.g, pLight->Ambient.b, pLight->Ambient.a);
    TRACE("... Pos(%f,%f,%f), Dirn(%f,%f,%f)\n", pLight->Position.x, pLight->Position.y, pLight->Position.z,
          pLight->Direction.x, pLight->Direction.y, pLight->Direction.z);
    TRACE("... Range(%f), Falloff(%f), Theta(%f), Phi(%f)\n", pLight->Range, pLight->Falloff, pLight->Theta, pLight->Phi);

    This->UpdateStateBlock->Changed.lights[Index] = TRUE;
    This->UpdateStateBlock->Set.lights[Index] = TRUE;
    memcpy(&This->UpdateStateBlock->lights[Index], pLight, sizeof(D3DLIGHT8));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* Diffuse: */
    colRGBA[0] = pLight->Diffuse.r;
    colRGBA[1] = pLight->Diffuse.g;
    colRGBA[2] = pLight->Diffuse.b;
    colRGBA[3] = pLight->Diffuse.a;
    glLightfv(GL_LIGHT0+Index, GL_DIFFUSE, colRGBA);
    checkGLcall("glLightfv");

    /* Specular */
    colRGBA[0] = pLight->Specular.r;
    colRGBA[1] = pLight->Specular.g;
    colRGBA[2] = pLight->Specular.b;
    colRGBA[3] = pLight->Specular.a;
    glLightfv(GL_LIGHT0+Index, GL_SPECULAR, colRGBA);
    checkGLcall("glLightfv");

    /* Ambient */
    colRGBA[0] = pLight->Ambient.r;
    colRGBA[1] = pLight->Ambient.g;
    colRGBA[2] = pLight->Ambient.b;
    colRGBA[3] = pLight->Ambient.a;
    glLightfv(GL_LIGHT0+Index, GL_AMBIENT, colRGBA);
    checkGLcall("glLightfv");

    /* Light settings are affected by the model view in OpenGL, the View transform in direct3d*/
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
    glLoadMatrixf((float *) &This->StateBlock.transforms[D3DTS_VIEW].u.m[0][0]);

    /* Attenuation - Are these right? guessing... */
    glLightf(GL_LIGHT0+Index, GL_CONSTANT_ATTENUATION,  pLight->Attenuation0);
    checkGLcall("glLightf");
    glLightf(GL_LIGHT0+Index, GL_LINEAR_ATTENUATION,    pLight->Attenuation1);
    checkGLcall("glLightf");

    quad_att = 1.4/(pLight->Range*pLight->Range);
    if (quad_att < pLight->Attenuation2) quad_att = pLight->Attenuation2;
    glLightf(GL_LIGHT0+Index, GL_QUADRATIC_ATTENUATION, quad_att);
    checkGLcall("glLightf");

    switch (pLight->Type) {
    case D3DLIGHT_POINT:
        /* Position */
        This->lightPosn[Index][0] = pLight->Position.x;
        This->lightPosn[Index][1] = pLight->Position.y;
        This->lightPosn[Index][2] = pLight->Position.z;
        This->lightPosn[Index][3] = 1.0;
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &This->lightPosn[Index][0]);
        checkGLcall("glLightfv");

        glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, 180);
        checkGLcall("glLightf");

        /* FIXME: Range */
        break;

    case D3DLIGHT_SPOT:
        /* Position */
        This->lightPosn[Index][0] = pLight->Position.x;
        This->lightPosn[Index][1] = pLight->Position.y;
        This->lightPosn[Index][2] = pLight->Position.z;
        This->lightPosn[Index][3] = 1.0;
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &This->lightPosn[Index][0]);
        checkGLcall("glLightfv");

        /* Direction */
        This->lightDirn[Index][0] = pLight->Direction.x;
        This->lightDirn[Index][1] = pLight->Direction.y;
        This->lightDirn[Index][2] = pLight->Direction.z;
        This->lightDirn[Index][3] = 1.0;
        glLightfv(GL_LIGHT0+Index, GL_SPOT_DIRECTION, &This->lightDirn[Index][0]);
        checkGLcall("glLightfv");

        /*
         * opengl-ish and d3d-ish spot lights use too different models for the
         * light "intensity" as a function of the angle towards the main light direction,
         * so we only can approximate very roughly.
         * however spot lights are rather rarely used in games (if ever used at all).
         * furthermore if still used, probably nobody pays attention to such details.
         */
        if (pLight->Falloff == 0) {
            rho = 6.28f;
        } else {
            rho = pLight->Theta + (pLight->Phi - pLight->Theta)/(2*pLight->Falloff);
        }
        if (rho < 0.0001) rho = 0.0001f;
        glLightf(GL_LIGHT0 + Index, GL_SPOT_EXPONENT, -0.3/log(cos(rho/2)));
        glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, pLight->Phi*90/M_PI);

        /* FIXME: Range */
        break;
    case D3DLIGHT_DIRECTIONAL:
        /* Direction */
        This->lightPosn[Index][0] = -pLight->Direction.x;
        This->lightPosn[Index][1] = -pLight->Direction.y;
        This->lightPosn[Index][2] = -pLight->Direction.z;
        This->lightPosn[Index][3] = 0.0;
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &This->lightPosn[Index][0]); /* Note gl uses w position of 0 for direction! */
        checkGLcall("glLightfv");

        glLightf(GL_LIGHT0+Index, GL_SPOT_CUTOFF, 180.0f);
        glLightf(GL_LIGHT0+Index, GL_SPOT_EXPONENT, 0.0f);


        break;
    default:
        FIXME("Unrecognized light type %d\n", pLight->Type);
    }

    /* Restore the modelview matrix */
    glPopMatrix();

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetLight(LPDIRECT3DDEVICE8 iface, DWORD Index,D3DLIGHT8* pLight) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Idx(%ld), pLight(%p)\n", This, Index, pLight);
    memcpy(pLight, &This->StateBlock.lights[Index], sizeof(D3DLIGHT8));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_LightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL Enable) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Idx(%ld), enable? %d\n", This, Index, Enable);

    This->UpdateStateBlock->Changed.lightEnable[Index] = TRUE;
    This->UpdateStateBlock->Set.lightEnable[Index] = TRUE;
    This->UpdateStateBlock->lightEnable[Index] = Enable;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    if (Enable) {
        glEnable(GL_LIGHT0+Index);
        checkGLcall("glEnable GL_LIGHT0+Index");
    } else {
        glDisable(GL_LIGHT0+Index);
        checkGLcall("glDisable GL_LIGHT0+Index");
    }
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetLightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL* pEnable) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : for idx(%ld)\n", This, Index);
    *pEnable = This->StateBlock.lightEnable[Index];
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST float* pPlane) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : for idx %ld, %p\n", This, Index, pPlane);

    This->UpdateStateBlock->Changed.clipplane[Index] = TRUE;
    This->UpdateStateBlock->Set.clipplane[Index] = TRUE;
    This->UpdateStateBlock->clipplane[Index][0] = pPlane[0];
    This->UpdateStateBlock->clipplane[Index][1] = pPlane[1];
    This->UpdateStateBlock->clipplane[Index][2] = pPlane[2];
    This->UpdateStateBlock->clipplane[Index][3] = pPlane[3];

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* Apply it */

    /* Clip Plane settings are affected by the model view in OpenGL, the World transform in direct3d, I think?*/
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
    glLoadMatrixf((float *) &This->StateBlock.transforms[D3DTS_WORLD].u.m[0][0]);

    TRACE("Clipplane [%f,%f,%f,%f]\n", This->UpdateStateBlock->clipplane[Index][0], This->UpdateStateBlock->clipplane[Index][1],
          This->UpdateStateBlock->clipplane[Index][2], This->UpdateStateBlock->clipplane[Index][3]);
    glClipPlane(GL_CLIP_PLANE0+Index, This->UpdateStateBlock->clipplane[Index]);

    glPopMatrix();
    checkGLcall("glClipPlane");

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,float* pPlane) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : for idx %ld\n", This, Index);
    pPlane[0] = This->StateBlock.clipplane[Index][0];
    pPlane[1] = This->StateBlock.clipplane[Index][0];
    pPlane[2] = This->StateBlock.clipplane[Index][0];
    pPlane[3] = This->StateBlock.clipplane[Index][0];
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD Value) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    DWORD OldValue = This->StateBlock.renderstate[State];
        
    TRACE("(%p)->state = %d, value = %ld\n", This, State, Value);
    This->UpdateStateBlock->Changed.renderstate[State] = TRUE;
    This->UpdateStateBlock->Set.renderstate[State] = TRUE;
    This->UpdateStateBlock->renderstate[State] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    switch (State) {
    case D3DRS_FILLMODE                  :
        switch ((D3DFILLMODE) Value) {
        case D3DFILL_POINT               : glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
        case D3DFILL_WIREFRAME           : glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
        case D3DFILL_SOLID               : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
        default:
            FIXME("Unrecognized D3DRS_FILLMODE value %ld\n", Value);
        }
        checkGLcall("glPolygonMode (fillmode)");
        break;

    case D3DRS_LIGHTING                  :
        if (Value) {
            glEnable(GL_LIGHTING);
            checkGLcall("glEnable GL_LIGHTING");
        } else {
            glDisable(GL_LIGHTING);
            checkGLcall("glDisable GL_LIGHTING");
        }
        break;

    case D3DRS_ZENABLE                   :
        switch ((D3DZBUFFERTYPE) Value) {
        case D3DZB_FALSE:
            glDisable(GL_DEPTH_TEST);
            checkGLcall("glDisable GL_DEPTH_TEST");
            break;
        case D3DZB_TRUE:
            glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            break;

        case D3DZB_USEW:
        default:
            FIXME("Unrecognized/Unhandled D3DZBUFFERTYPE value %ld\n", Value);
        }
        break;

    case D3DRS_CULLMODE                  :

        /* If we are culling "back faces with clockwise vertices" then
           set front faces to be counter clockwise and enable culling  
           of back faces                                               */
        switch ((D3DCULL) Value) {
        case D3DCULL_NONE:
            glDisable(GL_CULL_FACE);
            checkGLcall("glDisable GL_CULL_FACE");
            break;
        case D3DCULL_CW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            glFrontFace(GL_CCW);
            checkGLcall("glFrontFace GL_CCW");
            glCullFace(GL_BACK);
            break;
        case D3DCULL_CCW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            glFrontFace(GL_CW); 
            checkGLcall("glFrontFace GL_CW");
            glCullFace(GL_BACK);
            break;
        default:
            FIXME("Unrecognized/Unhandled D3DCULL value %ld\n", Value);
        }
        break;

    case D3DRS_SHADEMODE                 :
        switch ((D3DSHADEMODE) Value) {
        case D3DSHADE_FLAT:
            glShadeModel(GL_FLAT);
            checkGLcall("glShadeModel");
            break;
        case D3DSHADE_GOURAUD:
            glShadeModel(GL_SMOOTH);
            checkGLcall("glShadeModel");
            break;
        case D3DSHADE_PHONG:
            FIXME("D3DSHADE_PHONG isnt supported?\n");
            return D3DERR_INVALIDCALL;
        default:
            FIXME("Unrecognized/Unhandled D3DSHADEMODE value %ld\n", Value);
        }
        break;

    case D3DRS_DITHERENABLE              :
        if (Value) {
            glEnable(GL_DITHER);
            checkGLcall("glEnable GL_DITHER");
        } else {
            glDisable(GL_DITHER);
            checkGLcall("glDisable GL_DITHER");
        }
        break;

    case D3DRS_ZWRITEENABLE              :
        if (Value) {
            glDepthMask(1);
            checkGLcall("glDepthMask");
        } else {
            glDepthMask(0);
            checkGLcall("glDepthMask");
        }
        break;

    case D3DRS_ZFUNC                     :
        {
            int glParm = GL_LESS;

            switch ((D3DCMPFUNC) Value) {
            case D3DCMP_NEVER:         glParm=GL_NEVER; break;
            case D3DCMP_LESS:          glParm=GL_LESS; break;
            case D3DCMP_EQUAL:         glParm=GL_EQUAL; break;
            case D3DCMP_LESSEQUAL:     glParm=GL_LEQUAL; break;
            case D3DCMP_GREATER:       glParm=GL_GREATER; break;
            case D3DCMP_NOTEQUAL:      glParm=GL_NOTEQUAL; break;
            case D3DCMP_GREATEREQUAL:  glParm=GL_GEQUAL; break;
            case D3DCMP_ALWAYS:        glParm=GL_ALWAYS; break;
            default:
                FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
            }
            glDepthFunc(glParm);
            checkGLcall("glDepthFunc");
        }
        break;

    case D3DRS_AMBIENT                   :
        {

            float col[4];
            col[0] = ((Value >> 16) & 0xFF) / 255.0;
            col[1] = ((Value >> 8 ) & 0xFF) / 255.0;
            col[2] = ((Value >> 0 ) & 0xFF) / 255.0;
            col[3] = ((Value >> 24 ) & 0xFF) / 255.0;
            TRACE("Setting ambient to (%f,%f,%f,%f)\n", col[0],col[1],col[2],col[3]);
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, col);
            checkGLcall("glLightModel for MODEL_AMBIENT");

        }
        break;

    case D3DRS_ALPHABLENDENABLE          :
        if (Value) {
            glEnable(GL_BLEND);
            checkGLcall("glEnable GL_BLEND");
        } else {
            glDisable(GL_BLEND);
            checkGLcall("glDisable GL_BLEND");
        };
        break;

    case D3DRS_SRCBLEND                  :
    case D3DRS_DESTBLEND                 :
        {
            int newVal = GL_ZERO;
            switch (Value) {
            case D3DBLEND_ZERO               : newVal = GL_ZERO;  break;
            case D3DBLEND_ONE                : newVal = GL_ONE;  break;
            case D3DBLEND_SRCCOLOR           : newVal = GL_SRC_COLOR;  break;
            case D3DBLEND_INVSRCCOLOR        : newVal = GL_ONE_MINUS_SRC_COLOR;  break;
            case D3DBLEND_SRCALPHA           : newVal = GL_SRC_ALPHA;  break;
            case D3DBLEND_INVSRCALPHA        : newVal = GL_ONE_MINUS_SRC_ALPHA;  break;
            case D3DBLEND_DESTALPHA          : newVal = GL_DST_ALPHA;  break;
            case D3DBLEND_INVDESTALPHA       : newVal = GL_ONE_MINUS_DST_ALPHA;  break;
            case D3DBLEND_DESTCOLOR          : newVal = GL_DST_COLOR;  break;
            case D3DBLEND_INVDESTCOLOR       : newVal = GL_ONE_MINUS_DST_COLOR;  break;
            case D3DBLEND_SRCALPHASAT        : newVal = GL_SRC_ALPHA_SATURATE;  break;

            case D3DBLEND_BOTHSRCALPHA       : newVal = GL_SRC_ALPHA;
                This->srcBlend = newVal;
                This->dstBlend = newVal;
                break;

            case D3DBLEND_BOTHINVSRCALPHA    : newVal = GL_ONE_MINUS_SRC_ALPHA;
                This->srcBlend = newVal;
                This->dstBlend = newVal;
                break;
            default:
                FIXME("Unrecognized src/dest blend value %ld (%d)\n", Value, State);
            }

            if (State == D3DRS_SRCBLEND) This->srcBlend = newVal;
            if (State == D3DRS_DESTBLEND) This->dstBlend = newVal;
            TRACE("glBlendFunc src=%x, dst=%x\n", This->srcBlend, This->dstBlend);
            glBlendFunc(This->srcBlend, This->dstBlend);

            checkGLcall("glBlendFunc");
        }
        break;

    case D3DRS_ALPHATESTENABLE           :
        if (Value) {
            glEnable(GL_ALPHA_TEST);
            checkGLcall("glEnable GL_ALPHA_TEST");
        } else {
            glDisable(GL_ALPHA_TEST);
            checkGLcall("glDisable GL_ALPHA_TEST");
        }
        break;

    case D3DRS_ALPHAFUNC                 :
        {
            int glParm = GL_LESS;
            float ref = 1.0;

            glGetFloatv(GL_ALPHA_TEST_REF, &ref);
            checkGLcall("glGetFloatv(GL_ALPHA_TEST_REF, &ref);");

            switch ((D3DCMPFUNC) Value) {
            case D3DCMP_NEVER:         glParm=GL_NEVER; break;
            case D3DCMP_LESS:          glParm=GL_LESS; break;
            case D3DCMP_EQUAL:         glParm=GL_EQUAL; break;
            case D3DCMP_LESSEQUAL:     glParm=GL_LEQUAL; break;
            case D3DCMP_GREATER:       glParm=GL_GREATER; break;
            case D3DCMP_NOTEQUAL:      glParm=GL_NOTEQUAL; break;
            case D3DCMP_GREATEREQUAL:  glParm=GL_GEQUAL; break;
            case D3DCMP_ALWAYS:        glParm=GL_ALWAYS; break;
            default:
                FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
            }
            TRACE("glAlphaFunc with Parm=%x, ref=%f\n", glParm, ref);
            glAlphaFunc(glParm, ref);
            checkGLcall("glAlphaFunc");
        }
        break;

    case D3DRS_ALPHAREF                  :
        {
            int glParm = GL_LESS;
            float ref = 1.0;

            glGetIntegerv(GL_ALPHA_TEST_FUNC, &glParm);
            checkGLcall("glGetFloatv(GL_ALPHA_TEST_FUNC, &glParm);");

            ref = ((float) Value) / 255.0;
            TRACE("glAlphaFunc with Parm=%x, ref=%f\n", glParm, ref);
            glAlphaFunc(glParm, ref);
            checkGLcall("glAlphaFunc");
        }
        break;

    case D3DRS_CLIPPLANEENABLE           :
    case D3DRS_CLIPPING                  :
        {
            /* Ensure we only do the changed clip planes */
            DWORD enable  = 0xFFFFFFFF;
            DWORD disable = 0x00000000;
            
            /* If enabling / disabling all */
            if (State == D3DRS_CLIPPING) {
                if (Value) {
                    enable  = This->StateBlock.renderstate[D3DRS_CLIPPLANEENABLE];
                    disable = 0x00;
                } else {
                    disable = This->StateBlock.renderstate[D3DRS_CLIPPLANEENABLE];
                    enable  = 0x00;
                }
            } else {
                enable =   Value & ~OldValue;
                disable = ~Value &  OldValue;
            }
            
            if (enable & D3DCLIPPLANE0)  { glEnable(GL_CLIP_PLANE0);  checkGLcall("glEnable(clip plane 0)"); }
            if (enable & D3DCLIPPLANE1)  { glEnable(GL_CLIP_PLANE1);  checkGLcall("glEnable(clip plane 1)"); }
            if (enable & D3DCLIPPLANE2)  { glEnable(GL_CLIP_PLANE2);  checkGLcall("glEnable(clip plane 2)"); }
            if (enable & D3DCLIPPLANE3)  { glEnable(GL_CLIP_PLANE3);  checkGLcall("glEnable(clip plane 3)"); }
            if (enable & D3DCLIPPLANE4)  { glEnable(GL_CLIP_PLANE4);  checkGLcall("glEnable(clip plane 4)"); }
            if (enable & D3DCLIPPLANE5)  { glEnable(GL_CLIP_PLANE5);  checkGLcall("glEnable(clip plane 5)"); }
            
            if (disable & D3DCLIPPLANE0) { glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)"); }
            if (disable & D3DCLIPPLANE1) { glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)"); }
            if (disable & D3DCLIPPLANE2) { glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)"); }
            if (disable & D3DCLIPPLANE3) { glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)"); }
            if (disable & D3DCLIPPLANE4) { glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)"); }
            if (disable & D3DCLIPPLANE5) { glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)"); }
        }
        break;

    case D3DRS_BLENDOP                   :
        {
            int glParm = GL_FUNC_ADD;

            switch ((D3DBLENDOP) Value) {
            case D3DBLENDOP_ADD              : glParm = GL_FUNC_ADD;              break;
            case D3DBLENDOP_SUBTRACT         : glParm = GL_FUNC_SUBTRACT;         break;
            case D3DBLENDOP_REVSUBTRACT      : glParm = GL_FUNC_REVERSE_SUBTRACT; break;
            case D3DBLENDOP_MIN              : glParm = GL_MIN;                   break;
            case D3DBLENDOP_MAX              : glParm = GL_MAX;                   break;
            default:
                FIXME("Unrecognized/Unhandled D3DBLENDOP value %ld\n", Value);
            }
            glBlendEquation(glParm);
            checkGLcall("glBlendEquation");
        }
        break;

    case D3DRS_TEXTUREFACTOR             :
        {
            int i;

            /* Note the texture color applies to all textures whereas 
               GL_TEXTURE_ENV_COLOR applies to active only */
            float col[4];
            col[0] = ((Value >> 16) & 0xFF) / 255.0;
            col[1] = ((Value >> 8 ) & 0xFF) / 255.0;
            col[2] = ((Value >> 0 ) & 0xFF) / 255.0;
            col[3] = ((Value >> 24 ) & 0xFF) / 255.0;

            /* Set the default alpha blend color */
            glBlendColor(col[0], col[1], col[2], col[3]);
            checkGLcall("glBlendColor");

            /* And now the default texture color as well */
            for (i=0; i<8; i++) {

                if (This->StateBlock.textures[i]) {
                   glActiveTextureARB(GL_TEXTURE0_ARB + i);
                   checkGLcall("Activate texture.. to update const color");

                   /* Note the D3DRS value applies to all textures, but GL has one
                      per texture, so apply it now ready to be used!               */
                   glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
                   checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");
                }
            }
        }
        break;

    case D3DRS_SPECULARENABLE            : 
        {
            if (Value) {
                glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
                checkGLcall("glLightModel (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);");
            } else {
                glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
                checkGLcall("glLightModel (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);");
            }
        }
        break;

    case D3DRS_STENCILENABLE             :
        if (Value) {
            glEnable(GL_STENCIL_TEST);
            checkGLcall("glEnable GL_STENCIL_TEST");
        } else {
            glDisable(GL_STENCIL_TEST);
            checkGLcall("glDisable GL_STENCIL_TEST");
        }
        break;

    case D3DRS_STENCILFUNC               :
        {
           int glParm = GL_ALWAYS;
           int ref = 0;
           GLuint mask = 0xFFFFFFFF;

           glGetIntegerv(GL_STENCIL_REF, &ref);
           checkGLcall("glGetFloatv(GL_STENCIL_REF, &ref);");
           glGetIntegerv(GL_STENCIL_VALUE_MASK, &mask);
           checkGLcall("glGetFloatv(GL_STENCIL_VALUE_MASK, &glParm);");

           switch ((D3DCMPFUNC) Value) {
           case D3DCMP_NEVER:         glParm=GL_NEVER; break;
           case D3DCMP_LESS:          glParm=GL_LESS; break;
           case D3DCMP_EQUAL:         glParm=GL_EQUAL; break;
           case D3DCMP_LESSEQUAL:     glParm=GL_LEQUAL; break;
           case D3DCMP_GREATER:       glParm=GL_GREATER; break;
           case D3DCMP_NOTEQUAL:      glParm=GL_NOTEQUAL; break;
           case D3DCMP_GREATEREQUAL:  glParm=GL_GEQUAL; break;
           case D3DCMP_ALWAYS:        glParm=GL_ALWAYS; break;
           default:
               FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
           }
           TRACE("glStencilFunc with Parm=%x, ref=%d, mask=%x\n", glParm, ref, mask);
           glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case D3DRS_STENCILREF                :
        {
           int glParm = GL_ALWAYS;
           int ref = 0;
           GLuint mask = 0xFFFFFFFF;

           glGetIntegerv(GL_STENCIL_FUNC, &glParm);
           checkGLcall("glGetFloatv(GL_STENCIL_FUNC, &glParm);");
           glGetIntegerv(GL_STENCIL_VALUE_MASK, &mask);
           checkGLcall("glGetFloatv(GL_STENCIL_VALUE_MASK, &glParm);");

           ref = Value;
           TRACE("glStencilFunc with Parm=%x, ref=%d, mask=%x\n", glParm, ref, mask);
           glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case D3DRS_STENCILMASK               :
        {
           int glParm = GL_ALWAYS;
           int ref = 0.0;
           GLuint mask = Value;

           glGetIntegerv(GL_STENCIL_REF, &ref);
           checkGLcall("glGetFloatv(GL_STENCIL_REF, &ref);");
           glGetIntegerv(GL_STENCIL_FUNC, &glParm);
           checkGLcall("glGetFloatv(GL_STENCIL_FUNC, &glParm);");

           TRACE("glStencilFunc with Parm=%x, ref=%d, mask=%x\n", glParm, ref, mask);
           glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case D3DRS_STENCILFAIL               :
        {
            GLenum fail  ; 
            GLenum zpass ; 
            GLenum zfail ; 

            fail = StencilOp(Value);
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);
            checkGLcall("glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);");
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);
            checkGLcall("glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);");

            TRACE("StencilOp fail=%x, zfail=%x, zpass=%x\n", fail, zfail, zpass);
            glStencilOp(fail, zfail, zpass);
            checkGLcall("glStencilOp(fail, zfail, zpass);");
        }
        break;
    case D3DRS_STENCILZFAIL              :
        {
            GLenum fail  ; 
            GLenum zpass ; 
            GLenum zfail ; 

            glGetIntegerv(GL_STENCIL_FAIL, &fail);
            checkGLcall("glGetIntegerv(GL_STENCIL_FAIL, &fail);");
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);
            checkGLcall("glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);");
            zfail = StencilOp(Value);

            TRACE("StencilOp fail=%x, zfail=%x, zpass=%x\n", fail, zfail, zpass);
            glStencilOp(fail, zfail, zpass);
            checkGLcall("glStencilOp(fail, zfail, zpass);");
        }
        break;
    case D3DRS_STENCILPASS               :
        {
            GLenum fail  ; 
            GLenum zpass ; 
            GLenum zfail ; 

            glGetIntegerv(GL_STENCIL_FAIL, &fail);
            checkGLcall("glGetIntegerv(GL_STENCIL_FAIL, &fail);");
            zpass = StencilOp(Value);
            glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);
            checkGLcall("glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);");

            TRACE("StencilOp fail=%x, zfail=%x, zpass=%x\n", fail, zfail, zpass);
            glStencilOp(fail, zfail, zpass);
            checkGLcall("glStencilOp(fail, zfail, zpass);");
        }
        break;

    case D3DRS_STENCILWRITEMASK          :
        {
            glStencilMask(Value);
            checkGLcall("glStencilMask");
        }
        break;

        /* Unhandled yet...! */
    case D3DRS_LINEPATTERN               :
    case D3DRS_LASTPIXEL                 :
    case D3DRS_FOGENABLE                 :
    case D3DRS_ZVISIBLE                  :
    case D3DRS_FOGCOLOR                  :
    case D3DRS_FOGTABLEMODE              :
    case D3DRS_FOGSTART                  :
    case D3DRS_FOGEND                    :
    case D3DRS_FOGDENSITY                :
    case D3DRS_EDGEANTIALIAS             :
    case D3DRS_ZBIAS                     :
    case D3DRS_RANGEFOGENABLE            :
    case D3DRS_WRAP0                     :
    case D3DRS_WRAP1                     :
    case D3DRS_WRAP2                     :
    case D3DRS_WRAP3                     :
    case D3DRS_WRAP4                     :
    case D3DRS_WRAP5                     :
    case D3DRS_WRAP6                     :
    case D3DRS_WRAP7                     :
    case D3DRS_FOGVERTEXMODE             :
    case D3DRS_COLORVERTEX               :
    case D3DRS_LOCALVIEWER               :
    case D3DRS_NORMALIZENORMALS          :
    case D3DRS_DIFFUSEMATERIALSOURCE     :
    case D3DRS_SPECULARMATERIALSOURCE    :
    case D3DRS_AMBIENTMATERIALSOURCE     :
    case D3DRS_EMISSIVEMATERIALSOURCE    :
    case D3DRS_VERTEXBLEND               :
    case D3DRS_SOFTWAREVERTEXPROCESSING  :
    case D3DRS_POINTSIZE                 :
    case D3DRS_POINTSIZE_MIN             :
    case D3DRS_POINTSPRITEENABLE         :
    case D3DRS_POINTSCALEENABLE          :
    case D3DRS_POINTSCALE_A              :
    case D3DRS_POINTSCALE_B              :
    case D3DRS_POINTSCALE_C              :
    case D3DRS_MULTISAMPLEANTIALIAS      :
    case D3DRS_MULTISAMPLEMASK           :
    case D3DRS_PATCHEDGESTYLE            :
    case D3DRS_PATCHSEGMENTS             :
    case D3DRS_DEBUGMONITORTOKEN         :
    case D3DRS_POINTSIZE_MAX             :
    case D3DRS_INDEXEDVERTEXBLENDENABLE  :
    case D3DRS_COLORWRITEENABLE          :
    case D3DRS_TWEENFACTOR               :
        FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value);
        break;
    default:
        FIXME("(%p)->(%d,%ld) unrecognized\n", This, State, Value);
    }

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD* pValue) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) for State %d = %ld\n", This, State, This->UpdateStateBlock->renderstate[State]);
    *pValue = This->StateBlock.renderstate[State];
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_BeginStateBlock(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    void *memory;

    TRACE("(%p)\n", This);
    if (This->isRecordingState) {
        TRACE("(%p) already recording! returning error\n", This);
        return D3DERR_INVALIDCALL;
    }

    /* Allocate Storage */
    memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(STATEBLOCK));
    This->isRecordingState = TRUE;
    This->UpdateStateBlock = memory;

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_EndStateBlock(LPDIRECT3DDEVICE8 iface, DWORD* pToken) {

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p)\n", This);

    if (!This->isRecordingState) {
        TRACE("(%p) not recording! returning error\n", This);
        return D3DERR_INVALIDCALL;
    }

    This->UpdateStateBlock->blockType = D3DSBT_RECORDED;
    *pToken = (DWORD) This->UpdateStateBlock;
    This->isRecordingState = FALSE;
    This->UpdateStateBlock = &This->StateBlock;

    TRACE("(%p) returning token (ptr to stateblock) of %lx\n", This, *pToken);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_ApplyStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {

    STATEBLOCK *pSB = (STATEBLOCK *)Token;
    int i,j;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Applying state block %lx ------------------v\n", This, Token);

    /* FIXME: Only apply applicable states not all states */

    if (pSB->blockType == D3DSBT_RECORDED || pSB->blockType == D3DSBT_ALL || pSB->blockType == D3DSBT_VERTEXSTATE) {

        for (i=0; i<MAX_ACTIVE_LIGHTS; i++) {

            if (pSB->Set.lightEnable[i] && pSB->Changed.lightEnable[i])
                IDirect3DDevice8Impl_LightEnable(iface, i, pSB->lightEnable[i]);
            if (pSB->Set.lights[i] && pSB->Changed.lights[i])
                IDirect3DDevice8Impl_SetLight(iface, i, &pSB->lights[i]);
        }

        if (pSB->Set.vertexShader && pSB->Changed.vertexShader)
            IDirect3DDevice8Impl_SetVertexShader(iface, pSB->VertexShader);

        /* TODO: Vertex Shader Constants */
    }

    if (pSB->blockType == D3DSBT_RECORDED || pSB->blockType == D3DSBT_ALL || pSB->blockType == D3DSBT_PIXELSTATE) {

        if (pSB->Set.pixelShader && pSB->Changed.pixelShader)
            IDirect3DDevice8Impl_SetVertexShader(iface, pSB->PixelShader);

        /* TODO: Pixel Shader Constants */
    }

    /* Others + Render & Texture */
    if (pSB->blockType == D3DSBT_RECORDED || pSB->blockType == D3DSBT_ALL) {
        for (i=0; i<HIGHEST_TRANSFORMSTATE; i++) {
            if (pSB->Set.transform[i] && pSB->Changed.transform[i])
                IDirect3DDevice8Impl_SetTransform(iface, i, &pSB->transforms[i]);
        }

        if (pSB->Set.Indices && pSB->Changed.Indices)
            IDirect3DDevice8Impl_SetIndices(iface, pSB->pIndexData, pSB->baseVertexIndex);

        if (pSB->Set.material && pSB->Changed.material)
            IDirect3DDevice8Impl_SetMaterial(iface, &pSB->material);

        if (pSB->Set.viewport && pSB->Changed.viewport)
            IDirect3DDevice8Impl_SetViewport(iface, &pSB->viewport);

        for (i=0; i<MAX_STREAMS; i++) {
            if (pSB->Set.stream_source[i] && pSB->Changed.stream_source[i])
                IDirect3DDevice8Impl_SetStreamSource(iface, i, pSB->stream_source[i], pSB->stream_stride[i]);
        }

        for (i=0; i<MAX_CLIPPLANES; i++) {
            if (pSB->Set.clipplane[i] && pSB->Changed.clipplane[i]) {
                float clip[4];

                clip[0] = pSB->clipplane[i][0];
                clip[1] = pSB->clipplane[i][1];
                clip[2] = pSB->clipplane[i][2];
                clip[3] = pSB->clipplane[i][3];
                IDirect3DDevice8Impl_SetClipPlane(iface, i, clip);
            }
        }

        /* Render */
        for (i=0; i<HIGHEST_RENDER_STATE; i++) {

            if (pSB->Set.renderstate[i] && pSB->Changed.renderstate[i])
                IDirect3DDevice8Impl_SetRenderState(iface, i, pSB->renderstate[i]);

        }

        /* Texture */
        for (j=0; j<8; j++) {
            for (i=0; i<HIGHEST_TEXTURE_STATE; i++) {

                if (pSB->Set.texture_state[j][i] && pSB->Changed.texture_state[j][i])
                    IDirect3DDevice8Impl_SetTextureStageState(iface, j, i, pSB->texture_state[j][i]);
            }

        }

    } else if (pSB->blockType == D3DSBT_PIXELSTATE) {

        for (i=0; i<NUM_SAVEDPIXELSTATES_R; i++) {
            if (pSB->Set.renderstate[SavedPixelStates_R[i]] && pSB->Changed.renderstate[SavedPixelStates_R[i]])
                IDirect3DDevice8Impl_SetRenderState(iface, SavedPixelStates_R[i], pSB->renderstate[SavedPixelStates_R[i]]);

        }

        for (j=0; j<8; i++) {
            for (i=0; i<NUM_SAVEDPIXELSTATES_T; i++) {

                if (pSB->Set.texture_state[j][SavedPixelStates_T[i]] &&
                    pSB->Changed.texture_state[j][SavedPixelStates_T[i]])
                    IDirect3DDevice8Impl_SetTextureStageState(iface, j, SavedPixelStates_T[i], pSB->texture_state[j][SavedPixelStates_T[i]]);
            }
        }

    } else if (pSB->blockType == D3DSBT_VERTEXSTATE) {

        for (i=0; i<NUM_SAVEDVERTEXSTATES_R; i++) {
            if (pSB->Set.renderstate[SavedVertexStates_R[i]] && pSB->Changed.renderstate[SavedVertexStates_R[i]])
                IDirect3DDevice8Impl_SetRenderState(iface, SavedVertexStates_R[i], pSB->renderstate[SavedVertexStates_R[i]]);

        }

        for (j=0; j<8; i++) {
            for (i=0; i<NUM_SAVEDVERTEXSTATES_T; i++) {

                if (pSB->Set.texture_state[j][SavedVertexStates_T[i]] &&
                    pSB->Changed.texture_state[j][SavedVertexStates_T[i]])
                    IDirect3DDevice8Impl_SetTextureStageState(iface, j, SavedVertexStates_T[i], pSB->texture_state[j][SavedVertexStates_T[i]]);
            }
        }


    } else {
        FIXME("Unrecognized state block type %d\n", pSB->blockType);
    }
    memcpy(&This->StateBlock.Changed, &pSB->Changed, sizeof(This->StateBlock.Changed));
    TRACE("(%p) : Applied state block %lx ------------------^\n", This, Token);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CaptureStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {

    STATEBLOCK *updateBlock = (STATEBLOCK *)Token;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    TRACE("(%p) : Updating state block %lx ------------------v \n", This, Token);

    /* If not recorded, then update can just recapture */
    if (updateBlock->blockType != D3DSBT_RECORDED) {
        DWORD tmpToken;
        STATEBLOCK *tmpBlock;
        IDirect3DDevice8Impl_CreateStateBlock(iface, updateBlock->blockType, &tmpToken);
        tmpBlock = (STATEBLOCK *)tmpToken;
        memcpy(updateBlock, tmpBlock, sizeof(STATEBLOCK));
        IDirect3DDevice8Impl_DeleteStateBlock(iface, tmpToken);

        /* FIXME: This will record states of new lights! May need to have and save set_lights
           across this action */

    } else {
        int i,j;

        /* Recorded => Only update 'changed' values */
        if (updateBlock->Set.vertexShader && updateBlock->VertexShader != This->StateBlock.VertexShader) {
            updateBlock->VertexShader = This->StateBlock.VertexShader;
            TRACE("Updating vertex shader to %ld\n", This->StateBlock.VertexShader);
        }

        /* TODO: Vertex Shader Constants */

        for (i=0; i<MAX_ACTIVE_LIGHTS; i++) {
          if (updateBlock->Set.lightEnable[i] && This->StateBlock.lightEnable[i] != updateBlock->lightEnable[i]) {
              TRACE("Updating light enable for light %d to %d\n", i, This->StateBlock.lightEnable[i]);
              updateBlock->lightEnable[i] = This->StateBlock.lightEnable[i];
          }

          if (updateBlock->Set.lights[i] && memcmp(&This->StateBlock.lights[i], 
                                                   &updateBlock->lights[i], 
                                                   sizeof(D3DLIGHT8)) != 0) {
              TRACE("Updating lights for light %d\n", i);
              memcpy(&updateBlock->lights[i], &This->StateBlock.lights[i], sizeof(D3DLIGHT8));
          }
        }

        if (updateBlock->Set.pixelShader && updateBlock->PixelShader != This->StateBlock.PixelShader) {
            TRACE("Updating pixel shader to %ld\n", This->StateBlock.PixelShader);
            updateBlock->lights[i] = This->StateBlock.lights[i];
                IDirect3DDevice8Impl_SetVertexShader(iface, updateBlock->PixelShader);
        }

        /* TODO: Pixel Shader Constants */

        /* Others + Render & Texture */
       for (i=0; i<HIGHEST_TRANSFORMSTATE; i++) {
            if (updateBlock->Set.transform[i] && memcmp(&This->StateBlock.transforms[i], 
                                                   &updateBlock->transforms[i], 
                                                   sizeof(D3DMATRIX)) != 0) {
                TRACE("Updating transform %d\n", i);
                memcpy(&updateBlock->transforms[i], &This->StateBlock.transforms[i], sizeof(D3DMATRIX));
            }
       }

       if (updateBlock->Set.Indices && ((updateBlock->pIndexData != This->StateBlock.pIndexData)
                                    || (updateBlock->baseVertexIndex != This->StateBlock.baseVertexIndex))) {
           TRACE("Updating pindexData to %p, baseVertexIndex to %d\n", 
                       This->StateBlock.pIndexData, This->StateBlock.baseVertexIndex);
           updateBlock->pIndexData = This->StateBlock.pIndexData;
           updateBlock->baseVertexIndex = This->StateBlock.baseVertexIndex;
       }

       if (updateBlock->Set.material && memcmp(&This->StateBlock.material, 
                                                   &updateBlock->material, 
                                                   sizeof(D3DMATERIAL8)) != 0) {
                TRACE("Updating material\n");
                memcpy(&updateBlock->material, &This->StateBlock.material, sizeof(D3DMATERIAL8));
       }
           
       if (updateBlock->Set.viewport && memcmp(&This->StateBlock.viewport, 
                                                   &updateBlock->viewport, 
                                                   sizeof(D3DVIEWPORT8)) != 0) {
                TRACE("Updating viewport\n");
                memcpy(&updateBlock->viewport, &This->StateBlock.viewport, sizeof(D3DVIEWPORT8));
       }

       for (i=0; i<MAX_STREAMS; i++) {
           if (updateBlock->Set.stream_source[i] && 
                           ((updateBlock->stream_stride[i] != This->StateBlock.stream_stride[i]) ||
                           (updateBlock->stream_source[i] != This->StateBlock.stream_source[i]))) {
               TRACE("Updating stream source %d to %p, stride to %d\n", i, This->StateBlock.stream_source[i], 
                                                                        This->StateBlock.stream_stride[i]);
               updateBlock->stream_stride[i] = This->StateBlock.stream_stride[i];
               updateBlock->stream_source[i] = This->StateBlock.stream_source[i];
           }
       }

       for (i=0; i<MAX_CLIPPLANES; i++) {
           if (updateBlock->Set.clipplane[i] && memcmp(&This->StateBlock.clipplane[i], 
                                                       &updateBlock->clipplane[i], 
                                                       sizeof(updateBlock->clipplane)) != 0) {

               TRACE("Updating clipplane %d\n", i);
               memcpy(&updateBlock->clipplane[i], &This->StateBlock.clipplane[i], 
                                       sizeof(updateBlock->clipplane));
           }
       }

       /* Render */
       for (i=0; i<HIGHEST_RENDER_STATE; i++) {

           if (updateBlock->Set.renderstate[i] && (updateBlock->renderstate[i] != 
                                                       This->StateBlock.renderstate[i])) {
               TRACE("Updating renderstate %d to %ld\n", i, This->StateBlock.renderstate[i]);
               updateBlock->renderstate[i] = This->StateBlock.renderstate[i];
           }
       }

       /* Texture */
       for (j=0; j<8; j++) {
           for (i=0; i<HIGHEST_TEXTURE_STATE; i++) {

               if (updateBlock->Set.texture_state[j][i] && (updateBlock->texture_state[j][i] != 
                                                                This->StateBlock.texture_state[j][i])) {
                   TRACE("Updating texturestagestate %d,%d to %ld\n", j,i, This->StateBlock.texture_state[j][i]);
                   updateBlock->texture_state[j][i] =  This->StateBlock.texture_state[j][i];
               }
           }

       }
    }

    TRACE("(%p) : Updated state block %lx ------------------^\n", This, Token);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : freeing token %lx\n", This, Token);
    HeapFree(GetProcessHeap(), 0, (void *)Token);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_CreateStateBlock(LPDIRECT3DDEVICE8 iface, D3DSTATEBLOCKTYPE Type,DWORD* pToken) {
    void *memory;
    STATEBLOCK *s;
    int i,j;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : for type %d\n", This, Type);

    /* Allocate Storage */
    memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(STATEBLOCK));
    if (memory) memcpy(memory, &This->StateBlock, sizeof(STATEBLOCK));
    *pToken = (DWORD) memory;
    s = memory;
    s->blockType = Type;

    TRACE("Updating changed flags appropriate for type %d\n", Type);

    if (Type == D3DSBT_ALL) {
        TRACE("ALL => Pretend everything has changed\n");
        memset(&s->Changed, TRUE, sizeof(This->StateBlock.Changed));

    } else if (Type == D3DSBT_PIXELSTATE) {

        memset(&s->Changed, FALSE, sizeof(This->StateBlock.Changed));

        /* TODO: Pixel Shader Constants */
        s->Changed.pixelShader = TRUE;
        for (i=0; i<NUM_SAVEDPIXELSTATES_R; i++) {
            s->Changed.renderstate[SavedPixelStates_R[i]] = TRUE;
        }
        for (j=0; j<8; i++) {
            for (i=0; i<NUM_SAVEDPIXELSTATES_T; i++) {
                s->Changed.texture_state[j][SavedPixelStates_T[i]] = TRUE;
            }
        }

    } else if (Type == D3DSBT_VERTEXSTATE) {

        memset(&s->Changed, FALSE, sizeof(This->StateBlock.Changed));

        /* TODO: Vertex Shader Constants */
        s->Changed.vertexShader = TRUE;

        for (i=0; i<NUM_SAVEDVERTEXSTATES_R; i++) {
            s->Changed.renderstate[SavedVertexStates_R[i]] = TRUE;
        }
        for (j=0; j<8; i++) {
            for (i=0; i<NUM_SAVEDVERTEXSTATES_T; i++) {
                s->Changed.texture_state[j][SavedVertexStates_T[i]] = TRUE;
            }
        }

        for (i=0; i<MAX_ACTIVE_LIGHTS; i++) {
            s->Changed.lightEnable[i] = TRUE;
            s->Changed.lights[i] = TRUE;
        }

    } else {
        FIXME("Unrecognized state block type %d\n", Type);
    }
    TRACE("(%p) returning token (ptr to stateblock) of %lx\n", This, *pToken);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipStatus(LPDIRECT3DDEVICE8 iface, CONST D3DCLIPSTATUS8* pClipStatus) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipStatus(LPDIRECT3DDEVICE8 iface, D3DCLIPSTATUS8* pClipStatus) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage,IDirect3DBaseTexture8** ppTexture) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : returning %p for stage %ld\n", This, This->StateBlock.textures[Stage], Stage);
    *ppTexture = (LPDIRECT3DBASETEXTURE8)This->StateBlock.textures[Stage];
    IDirect3DBaseTexture8Impl_AddRef(*ppTexture);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage,IDirect3DBaseTexture8* pTexture) {

    IDirect3DBaseTexture8 *oldTxt;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    D3DRESOURCETYPE textureType;

    oldTxt = This->StateBlock.textures[Stage];
    TRACE("(%p) : Stage(%ld), Texture (%p)\n", This, Stage, pTexture);

    This->UpdateStateBlock->Set.textures[Stage] = TRUE;
    This->UpdateStateBlock->Changed.textures[Stage] = TRUE;
    This->UpdateStateBlock->textures[Stage] = pTexture;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* Make appropriate texture active */
    glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
    checkGLcall("glActiveTextureARB");

    /* Decrement the count of the previous texture */
    if (oldTxt != NULL) {
        IDirect3DBaseTexture8Impl_Release(oldTxt);
    }

    if (pTexture) {
        IDirect3DBaseTexture8Impl_AddRef((LPDIRECT3DBASETEXTURE8)This->StateBlock.textures[Stage]);

        /* Now setup the texture appropraitly */
        textureType = IDirect3DBaseTexture8Impl_GetType(pTexture);

        if (textureType == D3DRTYPE_TEXTURE) {
            IDirect3DTexture8Impl *pTexture2 = (IDirect3DTexture8Impl *) pTexture;
            int i;
            float col[4];

            /* Standard 2D texture */
            TRACE("Standard 2d texture\n");
/*            for (i=0; i<pTexture2->levels; i++) { */
            i=0;
            {

                if (pTexture2->surfaces[i]->textureName != 0 && pTexture2->Dirty == FALSE) {
                    glBindTexture(GL_TEXTURE_2D, pTexture2->surfaces[i]->textureName);
                    checkGLcall("glBindTexture");
                    TRACE("Texture %p given name %d\n", pTexture2->surfaces[i], pTexture2->surfaces[i]->textureName);
                } else {

                    if (pTexture2->surfaces[i]->textureName == 0) {
                        glGenTextures(1, &pTexture2->surfaces[i]->textureName);
                        checkGLcall("glGenTextures");
                        TRACE("Texture %p given name %d\n", pTexture2->surfaces[i], pTexture2->surfaces[i]->textureName);
                    }

                    glBindTexture(GL_TEXTURE_2D, pTexture2->surfaces[i]->textureName);
                    checkGLcall("glBindTexture");

                    TRACE("Calling glTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%lx, Mem=%p\n",
                          GL_TEXTURE_2D, i, fmt2glintFmt(pTexture2->format), pTexture2->surfaces[i]->myDesc.Width,
                          pTexture2->surfaces[i]->myDesc.Height, 0, fmt2glFmt(pTexture2->format),fmt2glType(pTexture2->format),
                          pTexture2->surfaces[i]->allocatedMemory);
                    glTexImage2D(GL_TEXTURE_2D, i,
                                 fmt2glintFmt(pTexture2->format),
                                 pTexture2->surfaces[i]->myDesc.Width,
                                 pTexture2->surfaces[i]->myDesc.Height,
                                 0,
                                 fmt2glFmt(pTexture2->format),
                                 fmt2glType(pTexture2->format),
                                 pTexture2->surfaces[i]->allocatedMemory
                                );
                    checkGLcall("glTexImage2D");

                    /*
                     * The following enable things to work but I dont think
                     * they all go here - FIXME! @@@
                     */
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );


                    glEnable(GL_TEXTURE_2D);
                    checkGLcall("glEnable");

                    pTexture2->Dirty = FALSE;
                }

                /* Note the D3DRS value applies to all textures, but GL has one
                   per texture, so apply it now ready to be used!               */
                col[0] = ((This->StateBlock.renderstate[D3DRS_TEXTUREFACTOR]>> 16) & 0xFF) / 255.0;
                col[1] = ((This->StateBlock.renderstate[D3DRS_TEXTUREFACTOR] >> 8 ) & 0xFF) / 255.0;
                col[2] = ((This->StateBlock.renderstate[D3DRS_TEXTUREFACTOR] >> 0 ) & 0xFF) / 255.0;
                col[3] = ((This->StateBlock.renderstate[D3DRS_TEXTUREFACTOR] >> 24 ) & 0xFF) / 255.0;
                glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
                checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");

            }

        } else if (textureType == D3DRTYPE_VOLUMETEXTURE) {
            IDirect3DVolumeTexture8Impl *pTexture2 = (IDirect3DVolumeTexture8Impl *) pTexture;
            int i;
            float col[4];

            /* Standard 3D (volume) texture */
            TRACE("Standard 3d texture\n");
/*            for (i=0; i<pTexture2->levels; i++) { */
            i=0;
            {

                if (pTexture2->volumes[i]->textureName != 0 && pTexture2->Dirty == FALSE) {
                    glBindTexture(GL_TEXTURE_3D, pTexture2->volumes[i]->textureName);
                    checkGLcall("glBindTexture");
                    TRACE("Texture %p given name %d\n", pTexture2->volumes[i], pTexture2->volumes[i]->textureName);
                } else {

                    if (pTexture2->volumes[i]->textureName == 0) {
                        glGenTextures(1, &pTexture2->volumes[i]->textureName);
                        checkGLcall("glGenTextures");
                        TRACE("Texture %p given name %d\n", pTexture2->volumes[i], pTexture2->volumes[i]->textureName);
                    }

                    glBindTexture(GL_TEXTURE_3D, pTexture2->volumes[i]->textureName);
                    checkGLcall("glBindTexture");

                    TRACE("Calling glTexImage3D %x i=%d, intfmt=%x, w=%d, h=%d,d=%d, 0=%d, glFmt=%x, glType=%lx, Mem=%p\n",
                          GL_TEXTURE_3D, i, fmt2glintFmt(pTexture2->format), pTexture2->volumes[i]->myDesc.Width,
                          pTexture2->volumes[i]->myDesc.Height, pTexture2->volumes[i]->myDesc.Depth,
                          0, fmt2glFmt(pTexture2->format),fmt2glType(pTexture2->format),
                          pTexture2->volumes[i]->allocatedMemory);
                    glTexImage3D(GL_TEXTURE_3D, i,
                                 fmt2glintFmt(pTexture2->format),
                                 pTexture2->volumes[i]->myDesc.Width,
                                 pTexture2->volumes[i]->myDesc.Height,
                                 pTexture2->volumes[i]->myDesc.Depth,
                                 0,
                                 fmt2glFmt(pTexture2->format),
                                 fmt2glType(pTexture2->format),
                                 pTexture2->volumes[i]->allocatedMemory
                                );
                    checkGLcall("glTexImage3D");

                    /*
                     * The following enable things to work but I dont think
                     * they all go here - FIXME! @@@
                     */
                    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT );
                    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
                    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );


                    glEnable(GL_TEXTURE_3D);
                    checkGLcall("glEnable");

                    pTexture2->Dirty = FALSE;
                }

                /* Note the D3DRS value applies to all textures, but GL has one
                   per texture, so apply it now ready to be used!               */
                col[0] = ((This->StateBlock.renderstate[D3DRS_TEXTUREFACTOR]>> 16) & 0xFF) / 255.0;
                col[1] = ((This->StateBlock.renderstate[D3DRS_TEXTUREFACTOR] >> 8 ) & 0xFF) / 255.0;
                col[2] = ((This->StateBlock.renderstate[D3DRS_TEXTUREFACTOR] >> 0 ) & 0xFF) / 255.0;
                col[3] = ((This->StateBlock.renderstate[D3DRS_TEXTUREFACTOR] >> 24 ) & 0xFF) / 255.0;
                glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
                checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");

            }

        } else {
            FIXME("(%p) : Incorrect type for a texture : %d\n", This, textureType);
        }
    }
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : requesting Stage %ld, Type %d getting %ld\n", This, Stage, Type, This->StateBlock.texture_state[Stage][Type]);
    *pValue = This->StateBlock.texture_state[Stage][Type];
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* FIXME: Handle 3d textures? What if TSS value set before set texture? Need to reapply all values? */
   
    TRACE("(%p) : stub, Stage=%ld, Type=%d, Value =%ld\n", This, Stage, Type, Value);

    This->UpdateStateBlock->Changed.texture_state[Stage][Type] = TRUE;
    This->UpdateStateBlock->Set.texture_state[Stage][Type] = TRUE;
    This->UpdateStateBlock->texture_state[Stage][Type] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* Make appropriate texture active */
    TRACE("Activating appropriate texture state\n");
    glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
    checkGLcall("glActiveTextureARB");

    switch (Type) {

    case D3DTSS_MINFILTER             :
        if (Value == D3DTEXF_POINT) {  
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            checkGLcall("glTexParameter GL_TEXTURE_MINFILTER, GL_NEAREST");
        } else if (Value == D3DTEXF_LINEAR) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            checkGLcall("glTexParameter GL_TEXTURE_MINFILTER, GL_LINEAR");
        } else {
            FIXME("Unhandled D3DTSS_MINFILTER value of %ld\n", Value);
        }
        break;


    case D3DTSS_MAGFILTER             :
        if (Value == D3DTEXF_POINT) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            checkGLcall("glTexParameter GL_TEXTURE_MAGFILTER, GL_NEAREST");
        } else if (Value == D3DTEXF_LINEAR) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            checkGLcall("glTexParameter GL_TEXTURE_MAGFILTER, GL_LINEAR");
        } else {
            FIXME("Unhandled D3DTSS_MAGFILTER value of %ld\n", Value);
        }
        break;

    case D3DTSS_COLORARG1             :
    case D3DTSS_COLORARG2             :
    case D3DTSS_COLORARG0             :
    case D3DTSS_ALPHAARG1             :
    case D3DTSS_ALPHAARG2             :
    case D3DTSS_ALPHAARG0             :
        {
            BOOL isAlphaReplicate = FALSE;
            BOOL isComplement     = FALSE;
            BOOL isAlphaArg       = (Type == D3DTSS_ALPHAARG1 || Type == D3DTSS_ALPHAARG2 || Type == D3DTSS_ALPHAARG0);
            int  operand= GL_SRC_COLOR;
            int  source = GL_TEXTURE;

            /* Catch alpha replicate */
            if (Value & D3DTA_ALPHAREPLICATE) {
                Value = Value & ~D3DTA_ALPHAREPLICATE;
                isAlphaReplicate = TRUE;
            }

            /* Catch Complement */
            if (Value & D3DTA_COMPLEMENT) {
                Value = Value & ~D3DTA_COMPLEMENT;
                isComplement = TRUE;
            }

            /* Calculate the operand */
            if (isAlphaReplicate && !isComplement) {
                operand = GL_SRC_ALPHA;
            } else if (isAlphaReplicate && isComplement) {
                operand = GL_ONE_MINUS_SRC_ALPHA;
            } else if (isComplement) {
                if (isAlphaArg) {
                    operand = GL_ONE_MINUS_SRC_COLOR;
                } else {
                    operand = GL_ONE_MINUS_SRC_ALPHA;
                }
            } else {
                if (isAlphaArg) {
                    operand = GL_SRC_ALPHA;
                } else {
                    operand = GL_SRC_COLOR;
                }
            }

            /* Calculate the source */
            switch (Value) {
            case D3DTA_CURRENT:   source  = GL_PREVIOUS_EXT;
                                  break;
            case D3DTA_DIFFUSE:   source  = GL_PRIMARY_COLOR_EXT;
                                  break;
            case D3DTA_TEXTURE:   source  = GL_TEXTURE;
                                  break;
            case D3DTA_TFACTOR:   source  = GL_CONSTANT_EXT;
                                  break;

            /* According to the GL_ARB_texture_env_combine specs, SPECULAR is 'Secondary color' and
               isnt supported until base GL supports it
               There is no concept of temp registers as far as I can tell                          */

            default:
                FIXME("Unrecognized or unhandled texture arg %ld\n", Value);
            }

            if (isAlphaArg) {
                TRACE("Source %x = %x, Operand %x = %x\n", SOURCEx_ALPHA_EXT(Type), source, OPERANDx_ALPHA_EXT(Type), operand);
                glTexEnvi(GL_TEXTURE_ENV, SOURCEx_ALPHA_EXT(Type), source);
                checkGLcall("glTexEnvi(GL_TEXTURE_ENV, SOURCEx_ALPHA_EXT, source);");
                glTexEnvi(GL_TEXTURE_ENV, OPERANDx_ALPHA_EXT(Type), operand);
                checkGLcall("glTexEnvi(GL_TEXTURE_ENV, OPERANDx_ALPHA_EXT, operand);");
            } else {
                TRACE("Source %x = %x, Operand %x = %x\n", SOURCEx_RGB_EXT(Type), source, OPERANDx_RGB_EXT(Type), operand);
                glTexEnvi(GL_TEXTURE_ENV, SOURCEx_RGB_EXT(Type), source);
                checkGLcall("glTexEnvi(GL_TEXTURE_ENV, SOURCEx_RGB_EXT, source);");
                glTexEnvi(GL_TEXTURE_ENV, OPERANDx_RGB_EXT(Type), operand);
                checkGLcall("glTexEnvi(GL_TEXTURE_ENV, OPERANDx_RGB_EXT, operand);");
            }
        }
        break;

    case D3DTSS_ALPHAOP               :
    case D3DTSS_COLOROP               :
        {

            int Scale = 1;
            int Parm = (Type == D3DTSS_ALPHAOP)? GL_COMBINE_ALPHA_EXT : GL_COMBINE_RGB_EXT;

            switch (Value) {
            case D3DTOP_DISABLE                   :
                /* TODO: Disable by making this and all later levels disabled */
                glDisable(GL_TEXTURE_2D);
                checkGLcall("Disable GL_TEXTURE_2D");
                break;

            case D3DTOP_SELECTARG1                :
                glTexEnvi(GL_TEXTURE_ENV, Parm, GL_REPLACE);
                break;

            case D3DTOP_MODULATE4X                : Scale = Scale * 2;  /* Drop through */
            case D3DTOP_MODULATE2X                : Scale = Scale * 2;  /* Drop through */
            case D3DTOP_MODULATE                  :

                /* Correct scale */
                if (Type == D3DTSS_ALPHAOP) glTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, Scale);
                else glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, Scale);

                glTexEnvi(GL_TEXTURE_ENV, Parm, GL_MODULATE);
                break;

            case D3DTOP_ADD                       :
                glTexEnvi(GL_TEXTURE_ENV, Parm, GL_ADD);
                break;


            case D3DTOP_ADDSIGNED2X               : Scale = Scale * 2;  /* Drop through */
            case D3DTOP_ADDSIGNED                 :
                glTexEnvi(GL_TEXTURE_ENV, Parm, GL_ADD_SIGNED_EXT);
                break;


            case D3DTOP_SUBTRACT                  :
                /* glTexEnvi(GL_TEXTURE_ENV, Parm, GL_SUBTRACT); Missing? */
            case D3DTOP_SELECTARG2                :
                /* GL_REPLACE, swap args 0 and 1? */
            case D3DTOP_ADDSMOOTH                 :
            case D3DTOP_BLENDDIFFUSEALPHA         :
            case D3DTOP_BLENDTEXTUREALPHA         :
            case D3DTOP_BLENDFACTORALPHA          :
            case D3DTOP_BLENDTEXTUREALPHAPM       :
            case D3DTOP_BLENDCURRENTALPHA         :
            case D3DTOP_PREMODULATE               :
            case D3DTOP_MODULATEALPHA_ADDCOLOR    :
            case D3DTOP_MODULATECOLOR_ADDALPHA    :
            case D3DTOP_MODULATEINVALPHA_ADDCOLOR :
            case D3DTOP_MODULATEINVCOLOR_ADDALPHA :
            case D3DTOP_BUMPENVMAP                :
            case D3DTOP_BUMPENVMAPLUMINANCE       :
            case D3DTOP_DOTPRODUCT3               :
            case D3DTOP_MULTIPLYADD               :
            case D3DTOP_LERP                      :
            default:
                FIXME("Unhandled texture operation %ld\n", Value);
            }
            break;
        }

        /* Unhandled */
    case D3DTSS_BUMPENVMAT00          :
    case D3DTSS_BUMPENVMAT01          :
    case D3DTSS_BUMPENVMAT10          :
    case D3DTSS_BUMPENVMAT11          :
    case D3DTSS_TEXCOORDINDEX         :
    case D3DTSS_ADDRESSU              :
    case D3DTSS_ADDRESSV              :
    case D3DTSS_BORDERCOLOR           :
    case D3DTSS_MIPFILTER             :
    case D3DTSS_MIPMAPLODBIAS         :
    case D3DTSS_MAXMIPLEVEL           :
    case D3DTSS_MAXANISOTROPY         :
    case D3DTSS_BUMPENVLSCALE         :
    case D3DTSS_BUMPENVLOFFSET        :
    case D3DTSS_TEXTURETRANSFORMFLAGS :
    case D3DTSS_ADDRESSW              :
    case D3DTSS_RESULTARG             :
    default:
        FIXME("(%p) : stub, Stage=%ld, Type=%d, Value =%ld\n", This, Stage, Type, Value);
    }
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_ValidateDevice(LPDIRECT3DDEVICE8 iface, DWORD* pNumPasses) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetInfo(LPDIRECT3DDEVICE8 iface, DWORD DevInfoID,void* pDevInfoStruct,DWORD DevInfoStructSize) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber,CONST PALETTEENTRY* pEntries) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber,PALETTEENTRY* pEntries) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT *PaletteNumber) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) {

    IDirect3DVertexBuffer8     *pVB;
    D3DVERTEXBUFFER_DESC        VtxBufDsc;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    pVB = This->StateBlock.stream_source[0];

    TRACE("(%p) : Type=%d, Start=%d, Count=%d\n", This, PrimitiveType, StartVertex, PrimitiveCount);

    IDirect3DVertexBuffer8Impl_GetDesc(pVB, &VtxBufDsc);
    DrawPrimitiveI(iface, PrimitiveType, PrimitiveCount, FALSE,
                   VtxBufDsc.FVF, ((IDirect3DVertexBuffer8Impl *)pVB)->allocatedMemory, StartVertex, -1, 0, NULL);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,
                                                           UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount) {
    UINT idxStride = 2;
    IDirect3DIndexBuffer8      *pIB;
    IDirect3DVertexBuffer8     *pVB;
    D3DINDEXBUFFER_DESC         IdxBufDsc;
    D3DVERTEXBUFFER_DESC        VtxBufDsc;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    pIB = This->StateBlock.pIndexData;
    pVB = This->StateBlock.stream_source[0];

    TRACE("(%p) : Type=%d, min=%d, CountV=%d, startIdx=%d, countP=%d \n", This, PrimitiveType,
          minIndex, NumVertices, startIndex, primCount);

    IDirect3DIndexBuffer8Impl_GetDesc(pIB, &IdxBufDsc);
    if (IdxBufDsc.Format == D3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    IDirect3DVertexBuffer8Impl_GetDesc(pVB, &VtxBufDsc);
    DrawPrimitiveI(iface, PrimitiveType, primCount, TRUE, VtxBufDsc.FVF, ((IDirect3DVertexBuffer8Impl *)pVB)->allocatedMemory,
                   This->StateBlock.baseVertexIndex, startIndex, idxStride, ((IDirect3DIndexBuffer8Impl *) pIB)->allocatedMemory);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    TRACE("(%p) : Type=%d, pCount=%d, pVtxData=%p, Stride=%d\n", This, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);

    if (This->StateBlock.stream_source[0] != NULL) IDirect3DVertexBuffer8Impl_Release(This->StateBlock.stream_source[0]);

    This->StateBlock.stream_source[0] = NULL;
    This->StateBlock.stream_stride[0] = VertexStreamZeroStride;
    DrawPrimitiveI(iface, PrimitiveType, PrimitiveCount, FALSE, This->StateBlock.VertexShader, pVertexStreamZeroData,
                   0, 0, 0, NULL);
    This->StateBlock.stream_stride[0] = 0;

    /*stream zero settings set to null at end */
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,
                                                             UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,
                                                             D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,
                                                             UINT VertexStreamZeroStride) {
    int idxStride;
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Type=%d, MinVtxIdx=%d, NumVIdx=%d, PCount=%d, pidxdata=%p, IdxFmt=%d, pVtxdata=%p, stride=%d\n", This, PrimitiveType,
          MinVertexIndex, NumVertexIndices, PrimitiveCount, pIndexData,  IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);

    if (This->StateBlock.stream_source[0] != NULL) IDirect3DVertexBuffer8Impl_Release(This->StateBlock.stream_source[0]);
    if (IndexDataFormat == D3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    This->StateBlock.stream_source[0] = NULL;
    This->StateBlock.stream_stride[0] = VertexStreamZeroStride;
    DrawPrimitiveI(iface, PrimitiveType, PrimitiveCount, TRUE, This->StateBlock.VertexShader, pVertexStreamZeroData,
                   This->StateBlock.baseVertexIndex, 0, idxStride, pIndexData);

    /*stream zero settings set to null at end */
    This->StateBlock.stream_stride[0] = 0;
    IDirect3DDevice8Impl_SetIndices(iface, NULL, 0);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_ProcessVertices(LPDIRECT3DDEVICE8 iface, UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer8* pDestBuffer,DWORD Flags) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pDeclaration,CONST DWORD* pFunction,DWORD* pHandle,DWORD Usage) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    This->UpdateStateBlock->VertexShader = Handle;
    This->UpdateStateBlock->Changed.vertexShader = TRUE;
    This->UpdateStateBlock->Set.vertexShader = TRUE;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    if (Handle <= VS_HIGHESTFIXEDFXF) {
        TRACE("(%p) : FVF Shader, Handle=%ld\n", This, Handle);
        return D3D_OK;
    } else {
        FIXME("(%p) : Created shader, Handle=%lx stub\n", This, Handle);
        return D3D_OK;
    }
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD* pHandle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) = %ld\n", This, This->StateBlock.VertexShader);
    *pHandle = This->StateBlock.VertexShader;
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteVertexShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register,CONST void* pConstantData,DWORD ConstantCount) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register,void* pConstantData,DWORD ConstantCount) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderDeclaration(LPDIRECT3DDEVICE8 iface, DWORD Handle,void* pData,DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD Handle,void* pData,DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8* pIndexData,UINT BaseVertexIndex) {

    IDirect3DIndexBuffer8 *oldIdxs;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Setting to %p, base %d\n", This, pIndexData, BaseVertexIndex);
    oldIdxs = This->StateBlock.pIndexData;

    This->UpdateStateBlock->Changed.Indices = TRUE;
    This->UpdateStateBlock->Set.Indices = TRUE;
    This->UpdateStateBlock->pIndexData = pIndexData;
    This->UpdateStateBlock->baseVertexIndex = BaseVertexIndex;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    if (oldIdxs) IDirect3DIndexBuffer8Impl_Release(oldIdxs);
    if (pIndexData) IDirect3DIndexBuffer8Impl_AddRef(This->StateBlock.pIndexData);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8** ppIndexData,UINT* pBaseVertexIndex) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);

    *ppIndexData = This->StateBlock.pIndexData;
    /* up ref count on ppindexdata */
    if (*ppIndexData) IDirect3DIndexBuffer8Impl_AddRef(*ppIndexData);
    *pBaseVertexIndex = This->StateBlock.baseVertexIndex;

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreatePixelShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pFunction,DWORD* pHandle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    This->UpdateStateBlock->PixelShader = Handle;
    This->UpdateStateBlock->Changed.pixelShader = TRUE;
    This->UpdateStateBlock->Set.pixelShader = TRUE;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    FIXME("(%p) : stub\n", This);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD* pHandle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : returning %ld\n", This, This->StateBlock.PixelShader);
    *pHandle = This->StateBlock.PixelShader;
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_DeletePixelShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register,CONST void* pConstantData,DWORD ConstantCount) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register,void* pConstantData,DWORD ConstantCount) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD Handle,void* pData,DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawRectPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawTriPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DeletePatch(LPDIRECT3DDEVICE8 iface, UINT Handle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8* pStreamData,UINT Stride) {
    IDirect3DVertexBuffer8 *oldSrc;
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    oldSrc = This->StateBlock.stream_source[StreamNumber];
    TRACE("(%p) : StreamNo: %d, OldStream (%p), NewStream (%p), NewStride %d\n", This, StreamNumber, oldSrc, pStreamData, Stride);

    This->UpdateStateBlock->Changed.stream_source[StreamNumber] = TRUE;
    This->UpdateStateBlock->Set.stream_source[StreamNumber] = TRUE;
    This->UpdateStateBlock->stream_stride[StreamNumber] = Stride;
    This->UpdateStateBlock->stream_source[StreamNumber] = pStreamData;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    if (oldSrc != NULL) IDirect3DVertexBuffer8Impl_Release(oldSrc);
    if (pStreamData != NULL) IDirect3DVertexBuffer8Impl_AddRef(pStreamData);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8** pStream,UINT* pStride) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : StreamNo: %d, Stream (%p), Stride %d\n", This, StreamNumber, This->StateBlock.stream_source[StreamNumber], This->StateBlock.stream_stride[StreamNumber]);
    *pStream = This->StateBlock.stream_source[StreamNumber];
    *pStride = This->StateBlock.stream_stride[StreamNumber];
    IDirect3DVertexBuffer8Impl_AddRef((LPDIRECT3DVERTEXBUFFER8) *pStream);
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DDevice8) Direct3DDevice8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DDevice8Impl_QueryInterface,
    IDirect3DDevice8Impl_AddRef,
    IDirect3DDevice8Impl_Release,
    IDirect3DDevice8Impl_TestCooperativeLevel,
    IDirect3DDevice8Impl_GetAvailableTextureMem,
    IDirect3DDevice8Impl_ResourceManagerDiscardBytes,
    IDirect3DDevice8Impl_GetDirect3D,
    IDirect3DDevice8Impl_GetDeviceCaps,
    IDirect3DDevice8Impl_GetDisplayMode,
    IDirect3DDevice8Impl_GetCreationParameters,
    IDirect3DDevice8Impl_SetCursorProperties,
    IDirect3DDevice8Impl_SetCursorPosition,
    IDirect3DDevice8Impl_ShowCursor,
    IDirect3DDevice8Impl_CreateAdditionalSwapChain,
    IDirect3DDevice8Impl_Reset,
    IDirect3DDevice8Impl_Present,
    IDirect3DDevice8Impl_GetBackBuffer,
    IDirect3DDevice8Impl_GetRasterStatus,
    IDirect3DDevice8Impl_SetGammaRamp,
    IDirect3DDevice8Impl_GetGammaRamp,
    IDirect3DDevice8Impl_CreateTexture,
    IDirect3DDevice8Impl_CreateVolumeTexture,
    IDirect3DDevice8Impl_CreateCubeTexture,
    IDirect3DDevice8Impl_CreateVertexBuffer,
    IDirect3DDevice8Impl_CreateIndexBuffer,
    IDirect3DDevice8Impl_CreateRenderTarget,
    IDirect3DDevice8Impl_CreateDepthStencilSurface,
    IDirect3DDevice8Impl_CreateImageSurface,
    IDirect3DDevice8Impl_CopyRects,
    IDirect3DDevice8Impl_UpdateTexture,
    IDirect3DDevice8Impl_GetFrontBuffer,
    IDirect3DDevice8Impl_SetRenderTarget,
    IDirect3DDevice8Impl_GetRenderTarget,
    IDirect3DDevice8Impl_GetDepthStencilSurface,
    IDirect3DDevice8Impl_BeginScene,
    IDirect3DDevice8Impl_EndScene,
    IDirect3DDevice8Impl_Clear,
    IDirect3DDevice8Impl_SetTransform,
    IDirect3DDevice8Impl_GetTransform,
    IDirect3DDevice8Impl_MultiplyTransform,
    IDirect3DDevice8Impl_SetViewport,
    IDirect3DDevice8Impl_GetViewport,
    IDirect3DDevice8Impl_SetMaterial,
    IDirect3DDevice8Impl_GetMaterial,
    IDirect3DDevice8Impl_SetLight,
    IDirect3DDevice8Impl_GetLight,
    IDirect3DDevice8Impl_LightEnable,
    IDirect3DDevice8Impl_GetLightEnable,
    IDirect3DDevice8Impl_SetClipPlane,
    IDirect3DDevice8Impl_GetClipPlane,
    IDirect3DDevice8Impl_SetRenderState,
    IDirect3DDevice8Impl_GetRenderState,
    IDirect3DDevice8Impl_BeginStateBlock,
    IDirect3DDevice8Impl_EndStateBlock,
    IDirect3DDevice8Impl_ApplyStateBlock,
    IDirect3DDevice8Impl_CaptureStateBlock,
    IDirect3DDevice8Impl_DeleteStateBlock,
    IDirect3DDevice8Impl_CreateStateBlock,
    IDirect3DDevice8Impl_SetClipStatus,
    IDirect3DDevice8Impl_GetClipStatus,
    IDirect3DDevice8Impl_GetTexture,
    IDirect3DDevice8Impl_SetTexture,
    IDirect3DDevice8Impl_GetTextureStageState,
    IDirect3DDevice8Impl_SetTextureStageState,
    IDirect3DDevice8Impl_ValidateDevice,
    IDirect3DDevice8Impl_GetInfo,
    IDirect3DDevice8Impl_SetPaletteEntries,
    IDirect3DDevice8Impl_GetPaletteEntries,
    IDirect3DDevice8Impl_SetCurrentTexturePalette,
    IDirect3DDevice8Impl_GetCurrentTexturePalette,
    IDirect3DDevice8Impl_DrawPrimitive,
    IDirect3DDevice8Impl_DrawIndexedPrimitive,
    IDirect3DDevice8Impl_DrawPrimitiveUP,
    IDirect3DDevice8Impl_DrawIndexedPrimitiveUP,
    IDirect3DDevice8Impl_ProcessVertices,
    IDirect3DDevice8Impl_CreateVertexShader,
    IDirect3DDevice8Impl_SetVertexShader,
    IDirect3DDevice8Impl_GetVertexShader,
    IDirect3DDevice8Impl_DeleteVertexShader,
    IDirect3DDevice8Impl_SetVertexShaderConstant,
    IDirect3DDevice8Impl_GetVertexShaderConstant,
    IDirect3DDevice8Impl_GetVertexShaderDeclaration,
    IDirect3DDevice8Impl_GetVertexShaderFunction,
    IDirect3DDevice8Impl_SetStreamSource,
    IDirect3DDevice8Impl_GetStreamSource,
    IDirect3DDevice8Impl_SetIndices,
    IDirect3DDevice8Impl_GetIndices,
    IDirect3DDevice8Impl_CreatePixelShader,
    IDirect3DDevice8Impl_SetPixelShader,
    IDirect3DDevice8Impl_GetPixelShader,
    IDirect3DDevice8Impl_DeletePixelShader,
    IDirect3DDevice8Impl_SetPixelShaderConstant,
    IDirect3DDevice8Impl_GetPixelShaderConstant,
    IDirect3DDevice8Impl_GetPixelShaderFunction,
    IDirect3DDevice8Impl_DrawRectPatch,
    IDirect3DDevice8Impl_DrawTriPatch,
    IDirect3DDevice8Impl_DeletePatch
};

void CreateStateBlock(LPDIRECT3DDEVICE8 iface) {
    D3DLINEPATTERN lp;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    FIXME("Need to sort out defaults for the state information! \n");

    TRACE("-----------------------> Setting up device defaults...\n");
    This->StateBlock.blockType = D3DSBT_ALL;

    /* Set some of the defaults */


    /* Render states: */
    if (This->PresentParms.EnableAutoDepthStencil) {
       IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZENABLE, D3DZB_TRUE );
    } else {
       IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZENABLE, D3DZB_FALSE );
    }
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FILLMODE, D3DFILL_SOLID);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    lp.wRepeatFactor = 0; lp.wLinePattern = 0; IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_LINEPATTERN, (DWORD) &lp);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZWRITEENABLE, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ALPHATESTENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_LASTPIXEL, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SRCBLEND, D3DBLEND_ONE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_DESTBLEND, D3DBLEND_ZERO);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_CULLMODE, D3DCULL_CCW);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ALPHAREF, 0xff); /*??*/
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_DITHERENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ALPHABLENDENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SPECULARENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZVISIBLE, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGCOLOR, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGSTART, 0.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGEND, 1.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGDENSITY, 1.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_EDGEANTIALIAS, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_ZBIAS, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_RANGEFOGENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILREF, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILMASK, 0xFFFFFFFF);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP0, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP1, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP2, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP3, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP4, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP5, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP6, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_WRAP7, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_CLIPPING, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_LIGHTING, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_AMBIENT, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_COLORVERTEX, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_LOCALVIEWER, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_NORMALIZENORMALS, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR2);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_CLIPPLANEENABLE, 0);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_SOFTWAREVERTEXPROCESSING, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSIZE, 1.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSIZE_MIN, 0.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSPRITEENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSCALEENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSCALE_A, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSCALE_B, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSCALE_C, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_PATCHSEGMENTS, 1.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_DEBUGMONITORTOKEN, D3DDMT_DISABLE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POINTSIZE_MAX, (DWORD) 64.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_COLORWRITEENABLE, 0x0000000F);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_TWEENFACTOR, (DWORD) 0.0f);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_BLENDOP, D3DBLENDOP_ADD);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_POSITIONORDER, D3DORDER_CUBIC);
    IDirect3DDevice8Impl_SetRenderState(iface, D3DRS_NORMALORDER, D3DORDER_LINEAR);





    TRACE("-----------------------> Device defaults now set up...\n");

}

DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R] = {
    D3DRS_ALPHABLENDENABLE   ,
    D3DRS_ALPHAFUNC          ,
    D3DRS_ALPHAREF           ,
    D3DRS_ALPHATESTENABLE    ,
    D3DRS_BLENDOP            ,
    D3DRS_COLORWRITEENABLE   ,
    D3DRS_DESTBLEND          ,
    D3DRS_DITHERENABLE       ,
    D3DRS_EDGEANTIALIAS      ,
    D3DRS_FILLMODE           ,
    D3DRS_FOGDENSITY         ,
    D3DRS_FOGEND             ,
    D3DRS_FOGSTART           ,
    D3DRS_LASTPIXEL          ,
    D3DRS_LINEPATTERN        ,
    D3DRS_SHADEMODE          ,
    D3DRS_SRCBLEND           ,
    D3DRS_STENCILENABLE      ,
    D3DRS_STENCILFAIL        ,
    D3DRS_STENCILFUNC        ,
    D3DRS_STENCILMASK        ,
    D3DRS_STENCILPASS        ,
    D3DRS_STENCILREF         ,
    D3DRS_STENCILWRITEMASK   ,
    D3DRS_STENCILZFAIL       ,
    D3DRS_TEXTUREFACTOR      ,
    D3DRS_WRAP0              ,
    D3DRS_WRAP1              ,
    D3DRS_WRAP2              ,
    D3DRS_WRAP3              ,
    D3DRS_WRAP4              ,
    D3DRS_WRAP5              ,
    D3DRS_WRAP6              ,
    D3DRS_WRAP7              ,
    D3DRS_ZBIAS              ,
    D3DRS_ZENABLE            ,
    D3DRS_ZFUNC              ,
    D3DRS_ZWRITEENABLE
};

DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T] = {
    D3DTSS_ADDRESSU              ,
    D3DTSS_ADDRESSV              ,
    D3DTSS_ADDRESSW              ,
    D3DTSS_ALPHAARG0             ,
    D3DTSS_ALPHAARG1             ,
    D3DTSS_ALPHAARG2             ,
    D3DTSS_ALPHAOP               ,
    D3DTSS_BORDERCOLOR           ,
    D3DTSS_BUMPENVLOFFSET        ,
    D3DTSS_BUMPENVLSCALE         ,
    D3DTSS_BUMPENVMAT00          ,
    D3DTSS_BUMPENVMAT01          ,
    D3DTSS_BUMPENVMAT10          ,
    D3DTSS_BUMPENVMAT11          ,
    D3DTSS_COLORARG0             ,
    D3DTSS_COLORARG1             ,
    D3DTSS_COLORARG2             ,
    D3DTSS_COLOROP               ,
    D3DTSS_MAGFILTER             ,
    D3DTSS_MAXANISOTROPY         ,
    D3DTSS_MAXMIPLEVEL           ,
    D3DTSS_MINFILTER             ,
    D3DTSS_MIPFILTER             ,
    D3DTSS_MIPMAPLODBIAS         ,
    D3DTSS_RESULTARG             ,
    D3DTSS_TEXCOORDINDEX         ,
    D3DTSS_TEXTURETRANSFORMFLAGS
};

DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R] = {
    D3DRS_AMBIENT                       ,
    D3DRS_AMBIENTMATERIALSOURCE         ,
    D3DRS_CLIPPING                      ,
    D3DRS_CLIPPLANEENABLE               ,
    D3DRS_COLORVERTEX                   ,
    D3DRS_DIFFUSEMATERIALSOURCE         ,
    D3DRS_EMISSIVEMATERIALSOURCE        ,
    D3DRS_FOGDENSITY                    ,
    D3DRS_FOGEND                        ,
    D3DRS_FOGSTART                      ,
    D3DRS_FOGTABLEMODE                  ,
    D3DRS_FOGVERTEXMODE                 ,
    D3DRS_INDEXEDVERTEXBLENDENABLE      ,
    D3DRS_LIGHTING                      ,
    D3DRS_LOCALVIEWER                   ,
    D3DRS_MULTISAMPLEANTIALIAS          ,
    D3DRS_MULTISAMPLEMASK               ,
    D3DRS_NORMALIZENORMALS              ,
    D3DRS_PATCHEDGESTYLE                ,
    D3DRS_PATCHSEGMENTS                 ,
    D3DRS_POINTSCALE_A                  ,
    D3DRS_POINTSCALE_B                  ,
    D3DRS_POINTSCALE_C                  ,
    D3DRS_POINTSCALEENABLE              ,
    D3DRS_POINTSIZE                     ,
    D3DRS_POINTSIZE_MAX                 ,
    D3DRS_POINTSIZE_MIN                 ,
    D3DRS_POINTSPRITEENABLE             ,
    D3DRS_RANGEFOGENABLE                ,
    D3DRS_SOFTWAREVERTEXPROCESSING      ,
    D3DRS_SPECULARMATERIALSOURCE        ,
    D3DRS_TWEENFACTOR                   ,
    D3DRS_VERTEXBLEND
};

DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T] = {
    D3DTSS_TEXCOORDINDEX         ,
    D3DTSS_TEXTURETRANSFORMFLAGS
};
