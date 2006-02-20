/*
 * IDirect3DDevice8 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2004 Christian Costa
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
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

/** define  GL_GLEXT_PROTOTYPES for having extensions prototypes defined */
/*#define GL_GLEXT_PROTOTYPES*/
/*#undef  GLX_GLXEXT_LEGACY*/
#include "d3d8_private.h"

/** currently desactiving 1_4 support as mesa doesn't implement all 1_4 support while defining it */
#undef GL_VERSION_1_4

/* Uncomment the next line to get extra traces, important but impact speed */
/* #define EXTRA_TRACES */

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_fps);

IDirect3DVertexShaderImpl*            VertexShaders[64];
IDirect3DVertexShaderDeclarationImpl* VertexShaderDeclarations[64];
IDirect3DPixelShaderImpl*             PixelShaders[64];

/* Debugging aids: */
#ifdef FRAME_DEBUGGING
BOOL isOn             = FALSE;
BOOL isDumpingFrames  = FALSE;
LONG primCounter      = 0;
#endif

/*
 * Utility functions or macros
 */
#define conv_mat(mat,gl_mat)                                                                \
do {                                                                                        \
    TRACE("%f %f %f %f\n", (mat)->u.s._11, (mat)->u.s._12, (mat)->u.s._13, (mat)->u.s._14); \
    TRACE("%f %f %f %f\n", (mat)->u.s._21, (mat)->u.s._22, (mat)->u.s._23, (mat)->u.s._24); \
    TRACE("%f %f %f %f\n", (mat)->u.s._31, (mat)->u.s._32, (mat)->u.s._33, (mat)->u.s._34); \
    TRACE("%f %f %f %f\n", (mat)->u.s._41, (mat)->u.s._42, (mat)->u.s._43, (mat)->u.s._44); \
    memcpy(gl_mat, (mat), 16 * sizeof(float));                                              \
} while (0)

/* Apply the current values to the specified texture stage */
void setupTextureStates(LPDIRECT3DDEVICE8 iface, DWORD Stage, DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    int i = 0;
    float col[4];
    BOOL changeTexture = TRUE;

    TRACE("-----------------------> Updating the texture at stage %ld to have new texture state information\n", Stage);
    for (i = 1; i < HIGHEST_TEXTURE_STATE; i++) {

        BOOL skip = FALSE;

        switch (i) {
        /* Performance: For texture states where multiples effect the outcome, only bother
              applying the last one as it will pick up all the other values                */
        case D3DTSS_COLORARG0:  /* Will be picked up when setting color op */
        case D3DTSS_COLORARG1:  /* Will be picked up when setting color op */
        case D3DTSS_COLORARG2:  /* Will be picked up when setting color op */
        case D3DTSS_ALPHAARG0:  /* Will be picked up when setting alpha op */
        case D3DTSS_ALPHAARG1:  /* Will be picked up when setting alpha op */
        case D3DTSS_ALPHAARG2:  /* Will be picked up when setting alpha op */
           skip = TRUE;
           break;

        /* Performance: If the texture states only impact settings for the texture unit 
             (compared to the texture object) then there is no need to reapply them. The
             only time they need applying is the first time, since we cheat and put the  
             values into the stateblock without applying.                                
             Per-texture unit: texture function (eg. combine), ops and args
                               texture env color                                               
                               texture generation settings                               
           Note: Due to some special conditions there may be a need to do particular ones
             of these, which is what the Flags allows                                     */
        case D3DTSS_COLOROP:       
        case D3DTSS_TEXCOORDINDEX:
            if (!(Flags == REAPPLY_ALL)) skip=TRUE;
            break;

        case D3DTSS_ALPHAOP:       
            if (!(Flags & REAPPLY_ALPHAOP)) skip=TRUE;
            break;

        default:
            skip = FALSE;
        }

        if (skip == FALSE) {
           /* Performance: Only change to this texture if we have to */
           if (changeTexture) {
               /* Make appropriate texture active */
               if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                   glActiveTexture(GL_TEXTURE0 + Stage);
                   checkGLcall("glActiveTexture");
#else
                   glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
                   checkGLcall("glActiveTextureARB");
#endif
                } else if (Stage > 0) {
                    FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
                }
                changeTexture = FALSE;
           }

           /* Now apply the change */
           IDirect3DDevice8Impl_SetTextureStageState(iface, Stage, i, This->StateBlock->texture_state[Stage][i]);
        }
    }

    /* Note the D3DRS value applies to all textures, but GL has one
     *  per texture, so apply it now ready to be used!
     */
    D3DCOLORTOGLFLOAT4(This->StateBlock->renderstate[D3DRS_TEXTUREFACTOR], col);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
    checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");

    TRACE("-----------------------> Updated the texture at stage %ld to have new texture state information\n", Stage);
}

/* Convert the D3DLIGHT8 properties into equivalent gl lights */
static void setup_light(LPDIRECT3DDEVICE8 iface, LONG Index, PLIGHTINFOEL *lightInfo) {

    float quad_att;
    float colRGBA[] = {0.0, 0.0, 0.0, 0.0};
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    /* Light settings are affected by the model view in OpenGL, the View transform in direct3d*/
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0]);

    /* Diffuse: */
    colRGBA[0] = lightInfo->OriginalParms.Diffuse.r;
    colRGBA[1] = lightInfo->OriginalParms.Diffuse.g;
    colRGBA[2] = lightInfo->OriginalParms.Diffuse.b;
    colRGBA[3] = lightInfo->OriginalParms.Diffuse.a;
    glLightfv(GL_LIGHT0+Index, GL_DIFFUSE, colRGBA);
    checkGLcall("glLightfv");

    /* Specular */
    colRGBA[0] = lightInfo->OriginalParms.Specular.r;
    colRGBA[1] = lightInfo->OriginalParms.Specular.g;
    colRGBA[2] = lightInfo->OriginalParms.Specular.b;
    colRGBA[3] = lightInfo->OriginalParms.Specular.a;
    glLightfv(GL_LIGHT0+Index, GL_SPECULAR, colRGBA);
    checkGLcall("glLightfv");

    /* Ambient */
    colRGBA[0] = lightInfo->OriginalParms.Ambient.r;
    colRGBA[1] = lightInfo->OriginalParms.Ambient.g;
    colRGBA[2] = lightInfo->OriginalParms.Ambient.b;
    colRGBA[3] = lightInfo->OriginalParms.Ambient.a;
    glLightfv(GL_LIGHT0+Index, GL_AMBIENT, colRGBA);
    checkGLcall("glLightfv");

    /* Attenuation - Are these right? guessing... */
    glLightf(GL_LIGHT0+Index, GL_CONSTANT_ATTENUATION,  lightInfo->OriginalParms.Attenuation0);
    checkGLcall("glLightf");
    glLightf(GL_LIGHT0+Index, GL_LINEAR_ATTENUATION,    lightInfo->OriginalParms.Attenuation1);
    checkGLcall("glLightf");

    quad_att = 1.4/(lightInfo->OriginalParms.Range*lightInfo->OriginalParms.Range);
    if (quad_att < lightInfo->OriginalParms.Attenuation2) quad_att = lightInfo->OriginalParms.Attenuation2;
    glLightf(GL_LIGHT0+Index, GL_QUADRATIC_ATTENUATION, quad_att);
    checkGLcall("glLightf");

    switch (lightInfo->OriginalParms.Type) {
    case D3DLIGHT_POINT:
        /* Position */
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &lightInfo->lightPosn[0]);
        checkGLcall("glLightfv");
        glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
        checkGLcall("glLightf");
        /* FIXME: Range */
        break;

    case D3DLIGHT_SPOT:
        /* Position */
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &lightInfo->lightPosn[0]);
        checkGLcall("glLightfv");
        /* Direction */
        glLightfv(GL_LIGHT0+Index, GL_SPOT_DIRECTION, &lightInfo->lightDirn[0]);
        checkGLcall("glLightfv");
        glLightf(GL_LIGHT0 + Index, GL_SPOT_EXPONENT, lightInfo->exponent);
        checkGLcall("glLightf");
        glLightf(GL_LIGHT0 + Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
        checkGLcall("glLightf");
        /* FIXME: Range */
        break;

    case D3DLIGHT_DIRECTIONAL:
        /* Direction */
        glLightfv(GL_LIGHT0+Index, GL_POSITION, &lightInfo->lightPosn[0]); /* Note gl uses w position of 0 for direction! */
        checkGLcall("glLightfv");
        glLightf(GL_LIGHT0+Index, GL_SPOT_CUTOFF, lightInfo->cutoff);
        checkGLcall("glLightf");
        glLightf(GL_LIGHT0+Index, GL_SPOT_EXPONENT, 0.0f);
        checkGLcall("glLightf");
        break;

    default:
        FIXME("Unrecognized light type %d\n", lightInfo->OriginalParms.Type);
    }

    /* Restore the modelview matrix */
    glPopMatrix();
}

/* Setup this textures matrix */
static void set_texture_matrix(const float *smat, DWORD flags)
{
    float mat[16];

    glMatrixMode(GL_TEXTURE);

    if (flags == D3DTTFF_DISABLE) {
        glLoadIdentity();
        checkGLcall("glLoadIdentity()");
        return;
    }

    if (flags == (D3DTTFF_COUNT1|D3DTTFF_PROJECTED)) {
        ERR("Invalid texture transform flags: D3DTTFF_COUNT1|D3DTTFF_PROJECTED\n");
        checkGLcall("glLoadIdentity()");
        return;
    }

    memcpy(mat, smat, 16*sizeof(float));

    switch (flags & ~D3DTTFF_PROJECTED) {
    case D3DTTFF_COUNT1: mat[1] = mat[5] = mat[9] = mat[13] = 0;
    case D3DTTFF_COUNT2: mat[2] = mat[6] = mat[10] = mat[14] = 0;
    default: mat[3] = mat[7] = mat[11] = 0, mat[15] = 1;
    }
    
    if (flags & D3DTTFF_PROJECTED) switch (flags & ~D3DTTFF_PROJECTED) {
    case D3DTTFF_COUNT2:
        mat[3] = mat[1], mat[7] = mat[5], mat[11] = mat[9], mat[15] = mat[13];
        mat[1] = mat[5] = mat[9] = mat[13] = 0;
        break;
    case D3DTTFF_COUNT3:
        mat[3] = mat[2], mat[7] = mat[6], mat[11] = mat[10], mat[15] = mat[14];
        mat[2] = mat[6] = mat[10] = mat[14] = 0;
        break;
    }
    glLoadMatrixf(mat);
    checkGLcall("glLoadMatrixf(mat)");
}

/* IDirect3D IUnknown parts follow: */
HRESULT WINAPI IDirect3DDevice8Impl_QueryInterface(LPDIRECT3DDEVICE8 iface,REFIID riid,LPVOID *ppobj)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DDevice8)) {
        IDirect3DDevice8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DDevice8Impl_AddRef(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

/* The function below is needed during the D3D8->WineD3D transition
   in order to release various surfaces like the front- and backBuffer.
   When the transition is complete code similar to this will be executed
   by IWineD3DDevice_Release.
*/
void IDirect3DDevice8_CleanUp(IDirect3DDevice8Impl *This)
{
    LPDIRECT3DSURFACE8 stencilBufferParent;
    /* Release the buffers (with sanity checks)*/
    if(This->stencilBufferTarget != NULL && (IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8)This->stencilBufferTarget) >0)){
        if(This->depthStencilBuffer != This->stencilBufferTarget)
        TRACE("(%p) Something's still holding the depthStencilBuffer\n",This);
    }
    This->stencilBufferTarget = NULL;

    if(This->renderTarget != NULL) {
        IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8)This->renderTarget);
        This->renderTarget = NULL;
    }

    if(This->depthStencilBuffer != NULL) {
        stencilBufferParent = (LPDIRECT3DSURFACE8)D3D8_SURFACE(This->depthStencilBuffer)->resource.parent;
        if(stencilBufferParent != NULL) {
            IDirect3DSurface8_Release(stencilBufferParent);
            This->depthStencilBuffer = NULL;
        }
    }

    if(This->backBuffer != NULL) {
        IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8)This->backBuffer);
        This->backBuffer = NULL;
    }

    if(This->frontBuffer != NULL) {
        IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8)This->frontBuffer);
        This->frontBuffer = NULL;
    }
}

ULONG WINAPI IDirect3DDevice8Impl_Release(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IDirect3DDevice8Impl_CleanRender(iface);
        IDirect3D8_Release((LPDIRECT3D8) This->direct3d8);
	IDirect3DDevice8_CleanUp(This);
        IWineD3DDevice_Release(This->WineD3DDevice);
        if (glXGetCurrentContext() == This->glCtx) {
            glXMakeCurrent(This->display, None, NULL);
        }
        glXDestroyContext(This->display, This->glCtx);

        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DDevice Interface follow: */
HRESULT  WINAPI  IDirect3DDevice8Impl_TestCooperativeLevel(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : stub\n", This);    /* No way of notifying yet! */
    return D3D_OK;
}

UINT     WINAPI  IDirect3DDevice8Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : stub, emulating 32Mb for now\n", This);
    /*
     * pretend we have 32MB of any type of memory queried.
     */
    return (1024*1024*32);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_ResourceManagerDiscardBytes(LPDIRECT3DDEVICE8 iface, DWORD Bytes) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetDirect3D(LPDIRECT3DDEVICE8 iface, IDirect3D8** ppD3D8) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : returning %p\n", This, This->direct3d8);

    /* Inc ref count */
    IDirect3D8_AddRef((LPDIRECT3D8) This->direct3d8);

    *ppD3D8 = (IDirect3D8 *)This->direct3d8;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetDeviceCaps(LPDIRECT3DDEVICE8 iface, D3DCAPS8* pCaps) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub, calling idirect3d for now\n", This);
    IDirect3D8Impl_GetDeviceCaps((LPDIRECT3D8) This->direct3d8, This->adapterNo, This->devType, pCaps);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetDisplayMode(LPDIRECT3DDEVICE8 iface, D3DDISPLAYMODE* pMode) {

    HDC hdc;
    int bpp = 0;

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    pMode->Width        = GetSystemMetrics(SM_CXSCREEN);
    pMode->Height       = GetSystemMetrics(SM_CYSCREEN);
    pMode->RefreshRate  = 85; /*FIXME: How to identify? */

    hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    bpp = GetDeviceCaps(hdc, BITSPIXEL);
    DeleteDC(hdc);

    switch (bpp) {
    case  8: pMode->Format       = D3DFMT_R8G8B8; break;
    case 16: pMode->Format       = D3DFMT_R5G6B5; break;
    case 24: /*pMode->Format       = D3DFMT_R8G8B8; break; */
    case 32: pMode->Format       = D3DFMT_A8R8G8B8; break;
    default: 
       FIXME("Unrecognized display mode format\n");
       pMode->Format       = D3DFMT_UNKNOWN;
    }

    FIXME("(%p) : returning w(%d) h(%d) rr(%d) fmt(%u,%s)\n", This, pMode->Width, pMode->Height, pMode->RefreshRate, 
	  pMode->Format, debug_d3dformat(pMode->Format));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetCreationParameters(LPDIRECT3DDEVICE8 iface, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) copying to %p\n", This, pParameters);    
    memcpy(pParameters, &This->CreateParms, sizeof(D3DDEVICE_CREATION_PARAMETERS));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetCursorProperties(LPDIRECT3DDEVICE8 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) {
    IDirect3DSurface8Impl* pSur = (IDirect3DSurface8Impl*) pCursorBitmap;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : Spot Pos(%u,%u)\n", This, XHotSpot, YHotSpot);

    if (D3DFMT_A8R8G8B8 != D3D8_SURFACE(pSur)->resource.format) {
      ERR("(%p) : surface(%p) has an invalid format\n", This, pCursorBitmap);
      return D3DERR_INVALIDCALL;
    }
    if (32 != D3D8_SURFACE(pSur)->currentDesc.Height || 32 != D3D8_SURFACE(pSur)->currentDesc.Width) {
      ERR("(%p) : surface(%p) has an invalid size\n", This, pCursorBitmap);
      return D3DERR_INVALIDCALL;
    }

    This->xHotSpot = XHotSpot;
    This->yHotSpot = YHotSpot;
    return D3D_OK;
}
void     WINAPI  IDirect3DDevice8Impl_SetCursorPosition(LPDIRECT3DDEVICE8 iface, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : SetPos to (%u,%u)\n", This, XScreenSpace, YScreenSpace);
    This->xScreenSpace = XScreenSpace;
    This->yScreenSpace = YScreenSpace;
    return;
}
BOOL     WINAPI  IDirect3DDevice8Impl_ShowCursor(LPDIRECT3DDEVICE8 iface, BOOL bShow) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : visible(%d)\n", This, bShow); 
    This->bCursorVisible = bShow;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain) {
    IDirect3DSwapChain8Impl* object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDevice8Impl));
    if (NULL == object) {
      return D3DERR_OUTOFVIDEOMEMORY;
    }
    object->lpVtbl = &Direct3DSwapChain8_Vtbl;
    object->ref = 1;

    TRACE("(%p)->(DepthStencil:(%u,%s), BackBufferFormat:(%u,%s))\n", This, 
	  pPresentationParameters->AutoDepthStencilFormat, debug_d3dformat(pPresentationParameters->AutoDepthStencilFormat),
	  pPresentationParameters->BackBufferFormat, debug_d3dformat(pPresentationParameters->BackBufferFormat));

    if (pPresentationParameters->Windowed && ((pPresentationParameters->BackBufferWidth  == 0) ||
                                              (pPresentationParameters->BackBufferHeight == 0))) {
      RECT Rect;
      
      GetClientRect(This->win_handle, &Rect);
      
      if (pPresentationParameters->BackBufferWidth == 0) {
	pPresentationParameters->BackBufferWidth = Rect.right;
	TRACE("Updating width to %d\n", pPresentationParameters->BackBufferWidth);
      }
      if (pPresentationParameters->BackBufferHeight == 0) {
	pPresentationParameters->BackBufferHeight = Rect.bottom;
	TRACE("Updating height to %d\n", pPresentationParameters->BackBufferHeight);
      }
    }
    
    /* Save the presentation parms now filled in correctly */
    memcpy(&object->PresentParms, pPresentationParameters, sizeof(D3DPRESENT_PARAMETERS));

    IDirect3DDevice8Impl_CreateRenderTarget((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
					    pPresentationParameters->MultiSampleType,
					    TRUE,
                                            (LPDIRECT3DSURFACE8*) &object->frontBuffer);
    
    IDirect3DDevice8Impl_CreateRenderTarget((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
					    pPresentationParameters->MultiSampleType,
					    TRUE,
                                            (LPDIRECT3DSURFACE8*) &object->backBuffer);

    if (pPresentationParameters->EnableAutoDepthStencil) {
       IDirect3DDevice8Impl_CreateDepthStencilSurface((LPDIRECT3DDEVICE8) object,
	  					      pPresentationParameters->BackBufferWidth,
						      pPresentationParameters->BackBufferHeight,
						      pPresentationParameters->AutoDepthStencilFormat,
						      D3DMULTISAMPLE_NONE,
						      (LPDIRECT3DSURFACE8*) &object->depthStencilBuffer);
    } else {
      object->depthStencilBuffer = NULL;
    }

    *pSwapChain = (IDirect3DSwapChain8*) object;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_Reset(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_Present(LPDIRECT3DDEVICE8 iface, 
					      CONST RECT* pSourceRect, CONST RECT* pDestRect, 
					      HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : complete stub!\n", This);

    ENTER_GL();

    if (pSourceRect || pDestRect) FIXME("Unhandled present options %p/%p\n", pSourceRect, pDestRect);


    glXSwapBuffers(This->display, This->drawable);
    /* Don't call checkGLcall, as glGetError is not applicable here */
    TRACE("glXSwapBuffers called, Starting new frame\n");

    /* FPS support */
    if (TRACE_ON(d3d_fps))
    {
        static long prev_time, frames;

        DWORD time = GetTickCount();
        frames++;
        /* every 1.5 seconds */
        if (time - prev_time > 1500) {
            TRACE_(d3d_fps)("@ approx %.2ffps\n", 1000.0*frames/(time - prev_time));
            prev_time = time;
            frames = 0;
        }
    }

#if defined(FRAME_DEBUGGING)
{
    if (GetFileAttributesA("C:\\D3DTRACE") != INVALID_FILE_ATTRIBUTES) {
        if (!isOn) {
            isOn = TRUE;
            FIXME("Enabling D3D Trace\n");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 1);
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Starting\n");
            isDumpingFrames = TRUE;
            glClear(GL_COLOR_BUFFER_BIT);
#endif

#if defined(SINGLE_FRAME_DEBUGGING)
        } else {
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Finishing\n");
            isDumpingFrames = FALSE;
#endif
            FIXME("Singe Frame trace complete\n");
            DeleteFileA("C:\\D3DTRACE");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 0);
#endif
        }
    } else {
        if (isOn) {
            isOn = FALSE;
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Finishing\n");
            isDumpingFrames = FALSE;
#endif
            FIXME("Disabling D3D Trace\n");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 0);
        }
    }
}
#endif

    LEAVE_GL();
    /* Although this is not strictly required, a simple demo showed this does occur
       on (at least non-debug) d3d                                                  */
    if (This->PresentParms.SwapEffect == D3DSWAPEFFECT_DISCARD) {
       IDirect3DDevice8Impl_Clear(iface, 0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER|D3DCLEAR_TARGET, 0x00, 1.0, 0);
    }

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetBackBuffer(LPDIRECT3DDEVICE8 iface, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    *ppBackBuffer = (LPDIRECT3DSURFACE8) This->backBuffer;
    TRACE("(%p) : BackBuf %d Type %d returning %p\n", This, BackBuffer, Type, *ppBackBuffer);

    if (BackBuffer > This->PresentParms.BackBufferCount - 1) {
        FIXME("Only one backBuffer currently supported\n");
        return D3DERR_INVALIDCALL;
    }

    /* Note inc ref on returned surface */
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppBackBuffer);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetRasterStatus(LPDIRECT3DDEVICE8 iface, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
void     WINAPI  IDirect3DDevice8Impl_SetGammaRamp(LPDIRECT3DDEVICE8 iface, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
    HDC hDC;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    FIXME("(%p) : pRamp@%p\n", This, pRamp);
    hDC = GetDC(This->win_handle);
    SetDeviceGammaRamp(hDC, (LPVOID) pRamp);
    ReleaseDC(This->win_handle, hDC);
    return;
}
void     WINAPI  IDirect3DDevice8Impl_GetGammaRamp(LPDIRECT3DDEVICE8 iface, D3DGAMMARAMP* pRamp) {
    HDC hDC;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    FIXME("(%p) : pRamp@%p\n", This, pRamp);
    hDC = GetDC(This->win_handle);
    GetDeviceGammaRamp(hDC, pRamp);
    ReleaseDC(This->win_handle, hDC);
    return;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateTexture(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                                    D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8 **ppTexture) {
    IDirect3DTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : W(%d) H(%d), Lvl(%d) d(%ld), Fmt(%u), Pool(%d)\n", This, Width, Height, Levels, Usage, Format,  Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DTexture8Impl));

    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
/*        *ppTexture = NULL; */
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DTexture8_Vtbl;
    object->ref = 1;
    hrc = IWineD3DDevice_CreateTexture(This->WineD3DDevice, Width, Height, Levels, Usage,
                                 (WINED3DFORMAT)Format, Pool, &object->wineD3DTexture, NULL, (IUnknown *)object, D3D8CB_CreateSurface);

    if (FAILED(hrc)) {
        /* free up object */ 
        FIXME("(%p) call to IWineD3DDevice_CreateTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
/*      *ppTexture = NULL; */
   } else {
        *ppTexture = (LPDIRECT3DTEXTURE8) object;
   }

   TRACE("(%p) Created Texture %p, %p\n",This,object,object->wineD3DTexture);
   return hrc;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVolumeTexture(LPDIRECT3DDEVICE8 iface, 
                                                          UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, 
                                                          D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) {

    IDirect3DVolumeTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolumeTexture8Impl));
    if (NULL == object) {
        FIXME("(%p) allocation of memory failed\n", This);
        *ppVolumeTexture = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVolumeTexture8_Vtbl;
    object->ref = 1;
    hrc = IWineD3DDevice_CreateVolumeTexture(This->WineD3DDevice, Width, Height, Depth, Levels, Usage,
                                 (WINED3DFORMAT)Format, Pool, &object->wineD3DVolumeTexture, NULL,
                                 (IUnknown *)object, D3D8CB_CreateVolume);

    if (hrc != D3D_OK) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateVolumeTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVolumeTexture = NULL;
    } else {
        *ppVolumeTexture = (LPDIRECT3DVOLUMETEXTURE8) object;
    }
    TRACE("(%p)  returning %p\n", This , *ppVolumeTexture);
    return hrc;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateCubeTexture(LPDIRECT3DDEVICE8 iface, UINT EdgeLength, UINT Levels, DWORD Usage, 
                                                        D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) {

    IDirect3DCubeTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : ELen(%d) Lvl(%d) Usage(%ld) fmt(%u), Pool(%d)\n" , This, EdgeLength, Levels, Usage, Format, Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));

    if (NULL == object) {
        FIXME("(%p) allocation of CubeTexture failed\n", This);
        *ppCubeTexture = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DCubeTexture8_Vtbl;
    object->ref = 1;
    hr = IWineD3DDevice_CreateCubeTexture(This->WineD3DDevice, EdgeLength, Levels, Usage,
                                 (WINED3DFORMAT)Format, Pool, &object->wineD3DCubeTexture, NULL, (IUnknown*)object,
                                 D3D8CB_CreateSurface);

    if (hr != D3D_OK){

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateCubeTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppCubeTexture = NULL;
    } else {
        *ppCubeTexture = (LPDIRECT3DCUBETEXTURE8) object;
    }

    TRACE("(%p) returning %p\n",This, *ppCubeTexture);
    return hr;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexBuffer(LPDIRECT3DDEVICE8 iface, UINT Size, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) {
    IDirect3DVertexBuffer8Impl *object;

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

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

    *ppVertexBuffer = (LPDIRECT3DVERTEXBUFFER8) object;

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateIndexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) {
    IDirect3DIndexBuffer8Impl *object;

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : Len=%d, Use=%lx, Format=(%u,%s), Pool=%d\n", This, Length, Usage, Format, debug_d3dformat(Format), Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DIndexBuffer8Impl));
    object->lpVtbl = &Direct3DIndexBuffer8_Vtbl;
    object->Device = This;
    object->ref = 1;
    object->ResourceType = D3DRTYPE_INDEXBUFFER;

    object->currentDesc.Type = D3DRTYPE_INDEXBUFFER;
    object->currentDesc.Usage = Usage;
    object->currentDesc.Pool  = Pool;
    object->currentDesc.Format  = Format;
    object->currentDesc.Size  = Length;

    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length);

    TRACE("(%p) : Iface@%p allocatedMem @ %p\n", This, object, object->allocatedMemory);

    *ppIndexBuffer = (LPDIRECT3DINDEXBUFFER8) object;

    return D3D_OK;
}

HRESULT  WINAPI IDirect3DDevice8Impl_CreateSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IDirect3DSurface8 **ppSurface,D3DRESOURCETYPE Type, UINT Usage,D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality)  {
    HRESULT hrc;
    IDirect3DSurface8Impl *object;
    IDirect3DDevice8Impl  *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    if(MultisampleQuality < 0) { 
        FIXME("MultisampleQuality out of range %ld, substituting 0 \n", MultisampleQuality);
        /*FIXME: Find out what windows does with a MultisampleQuality < 0 */
        MultisampleQuality=0;
    }

    if(MultisampleQuality > 0){
        FIXME("MultisampleQuality set to %ld, bstituting 0  \n" , MultisampleQuality);
        /*
        MultisampleQuality
        [in] Quality level. The valid range is between zero and one less than the level returned by pQualityLevels used by IDirect3D8::CheckDeviceMultiSampleType. Passing a larger value returns the error D3DERR_INVALIDCALL. The MultisampleQuality values of paired render targets, depth stencil surfaces, and the MultiSample type must all match.
        */
        MultisampleQuality=0;
    }
    /*FIXME: Check MAX bounds of MultisampleQuality*/

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppSurface = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DSurface8_Vtbl;
    object->ref = 1;

    TRACE("(%p) : w(%d) h(%d) fmt(%d) surf@%p\n", This, Width, Height, Format, *ppSurface);

    hrc = IWineD3DDevice_CreateSurface(This->WineD3DDevice, Width, Height, Format, Lockable, Discard, Level,  &object->wineD3DSurface, Type, Usage, Pool,MultiSample,MultisampleQuality, NULL,(IUnknown *)object);
    if (hrc != D3D_OK || NULL == object->wineD3DSurface) {
       /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateSurface failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppSurface = NULL;
    } else {
        *ppSurface = (LPDIRECT3DSURFACE8) object;
    }
    return hrc;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_CreateRenderTarget(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) {
    TRACE("Relay\n");

    return IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, Lockable, FALSE /* Discard */, 0 /* Level */ , ppSurface, D3DRTYPE_SURFACE, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT, MultiSample, 0);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) {
    TRACE("Relay\n");
    /* TODO: Verify that Discard is false */
    return IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Lockable */, FALSE, 0 /* Level */
                                               ,ppSurface, D3DRTYPE_SURFACE, D3DUSAGE_DEPTHSTENCIL,
                                                D3DPOOL_DEFAULT, MultiSample, 0);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_CreateImageSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) {
    TRACE("Relay\n");

    return IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Loackable */ , FALSE /*Discard*/ , 0 /* Level */ , ppSurface, D3DRTYPE_SURFACE, 0 /* Usage (undefined/none) */ , D3DPOOL_SCRATCH, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_CopyRects(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8 *pSourceSurface, CONST RECT *pSourceRects, UINT cRects, IDirect3DSurface8 *pDestinationSurface, CONST POINT *pDestPoints) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_CopyRects(This->WineD3DDevice, pSourceSurface == NULL ? NULL : ((IDirect3DSurface8Impl *)pSourceSurface)->wineD3DSurface,
            pSourceRects, cRects, pDestinationSurface == NULL ? NULL : ((IDirect3DSurface8Impl *)pDestinationSurface)->wineD3DSurface, pDestPoints);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_UpdateTexture(LPDIRECT3DDEVICE8 iface, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_UpdateTexture(This->WineD3DDevice,  ((IDirect3DBaseTexture8Impl *)pSourceTexture)->wineD3DBaseTexture, ((IDirect3DBaseTexture8Impl *)pDestinationTexture)->wineD3DBaseTexture);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetFrontBuffer(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pDestSurface) {
    HRESULT hr;
    D3DLOCKED_RECT lockedRect;
    RECT wantedRect;
    GLint  prev_store;
    GLint  prev_read;

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    FIXME("(%p) : Should return whole screen, only returns GL context window in top left corner\n", This);

    if (D3DFMT_A8R8G8B8 != D3D8_SURFACE(((IDirect3DSurface8Impl*)pDestSurface))->resource.format) {
      ERR("(%p) : surface(%p) has an invalid format\n", This, pDestSurface);
      return D3DERR_INVALIDCALL;
    }
    
    wantedRect.left = 0;
    wantedRect.top = 0;
    wantedRect.right = This->PresentParms.BackBufferWidth;
    wantedRect.bottom = This->PresentParms.BackBufferHeight;
    
    hr = IDirect3DSurface8Impl_LockRect(pDestSurface, &lockedRect, &wantedRect, 0);
    if (FAILED(hr)) {
      ERR("(%p) : cannot lock surface\n", This);
      return D3DERR_INVALIDCALL;
    }

    ENTER_GL();

    glFlush();
    vcheckGLcall("glFlush");
    glGetIntegerv(GL_READ_BUFFER, &prev_read);
    vcheckGLcall("glIntegerv");
    glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
    vcheckGLcall("glIntegerv");
 
    glReadBuffer(GL_FRONT);
    vcheckGLcall("glReadBuffer");
    glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
    vcheckGLcall("glPixelStorei");
    /* stupid copy */
    {
      unsigned long j;
      for (j = 0; j < This->PresentParms.BackBufferHeight; ++j) {
	glReadPixels(0, This->PresentParms.BackBufferHeight - j - 1, This->PresentParms.BackBufferWidth, 1,
		     GL_BGRA, GL_UNSIGNED_BYTE, ((char*) lockedRect.pBits) + (j * lockedRect.Pitch));
	vcheckGLcall("glReadPixels");
      }
    }
    glPixelStorei(GL_PACK_SWAP_BYTES, prev_store);
    vcheckGLcall("glPixelStorei");
    glReadBuffer(prev_read);
    vcheckGLcall("glReadBuffer");

    LEAVE_GL();

    hr = IDirect3DSurface8Impl_UnlockRect(pDestSurface);
    return hr;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) {
    HRESULT      hr = D3D_OK;
    D3DVIEWPORT8 viewport;

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    /* If pRenderTarget == NULL, it seems to default to back buffer */
    if (pRenderTarget == NULL) pRenderTarget = (IDirect3DSurface8*)This->backBuffer;
 
    /* For ease of code later on, handle a null depth as leave alone
        - Have not tested real d3d for this case but doing this avoids 
        numerous null pointer checks                                   */
    if (pNewZStencil == NULL) pNewZStencil = (IDirect3DSurface8*)This->stencilBufferTarget;

    /* If we are trying to set what we already have, don't bother */
    if ((IDirect3DSurface8Impl*) pRenderTarget == This->renderTarget && (IDirect3DSurface8Impl*) pNewZStencil == This->stencilBufferTarget) {
      TRACE("Trying to do a NOP SetRenderTarget operation\n");
    } else {
      /* Otherwise, set the render target up */
      TRACE("(%p) : newRender@%p newZStencil@%p (default is backbuffer=(%p))\n", This, pRenderTarget, pNewZStencil, This->backBuffer);
      IDirect3DDevice8Impl_CleanRender(iface);
      hr = IDirect3DDevice8Impl_ActiveRender(iface, pRenderTarget, pNewZStencil);
    }

    if (SUCCEEDED(hr)) {
	/* Finally, reset the viewport as the MSDN states. */
	viewport.Height = D3D8_SURFACE(((IDirect3DSurface8Impl*)pRenderTarget))->currentDesc.Height;
	viewport.Width  = D3D8_SURFACE(((IDirect3DSurface8Impl*)pRenderTarget))->currentDesc.Width;
	viewport.X      = 0;
	viewport.Y      = 0;
	viewport.MaxZ   = 1.0f;
	viewport.MinZ   = 0.0f;
	IDirect3DDevice8Impl_SetViewport(iface, &viewport);
    }
    
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppRenderTarget) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p)->returning (%p) default is backbuffer=(%p)\n", This, This->renderTarget, This->backBuffer);
    
    *ppRenderTarget = (LPDIRECT3DSURFACE8) This->renderTarget;
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppRenderTarget);
    
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppZStencilSurface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p)->(%p) default(%p)\n", This, This->stencilBufferTarget, This->depthStencilBuffer);
    
    /* Note inc ref on returned surface */
    *ppZStencilSurface = (LPDIRECT3DSURFACE8) This->stencilBufferTarget;
    if (NULL != *ppZStencilSurface) IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppZStencilSurface);

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_BeginScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    return IWineD3DDevice_BeginScene(This->WineD3DDevice);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_EndScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DBaseTexture8* cont = NULL;
    HRESULT hr;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p)\n", This);

    ENTER_GL();

    glFlush();
    checkGLcall("glFlush");

#if 0 /* Useful for debugging sometimes! */
    printf("Hit Enter ...\n");
    getchar();
#endif
    if ((This->frontBuffer != This->renderTarget) && (This->backBuffer != This->renderTarget)) {
#if 0
	GLenum prev_read;
	glGetIntegerv(GL_READ_BUFFER, &prev_read);
	vcheckGLcall("glIntegerv");
	glReadBuffer(GL_FRONT);
	vcheckGLcall("glReadBuffer");
	{
	  long j;
	  long pitch = This->renderTarget->myDesc.Width * This->renderTarget->bytesPerPixel;

          if (This->renderTarget->myDesc.Format == D3DFMT_DXT1) /* DXT1 is half byte per pixel */
              pitch = pitch / 2;

	  for (j = 0; j < This->renderTarget->myDesc.Height; ++j) {
	    glReadPixels(0, 
			 This->renderTarget->myDesc.Height - j - 1, 
			 This->renderTarget->myDesc.Width, 
			 1,
			 D3DFmt2GLFmt(This, This->renderTarget->myDesc.Format), 
			 D3DFmt2GLType(This, This->renderTarget->myDesc.Format), 
			 This->renderTarget->allocatedMemory + j * pitch);
	    vcheckGLcall("glReadPixels");
	  }
	}      
	glReadBuffer(prev_read);
	vcheckGLcall("glReadBuffer");
#endif

      hr = IDirect3DSurface8_GetContainer((LPDIRECT3DSURFACE8) This->renderTarget, &IID_IDirect3DBaseTexture8, (void**) &cont);
      if (SUCCEEDED(hr) && NULL != cont) {
	/** always dirtify for now. we must find a better way to see that surface have been modified */
	D3D8_SURFACE(This->renderTarget)->inPBuffer = TRUE;
	D3D8_SURFACE(This->renderTarget)->inTexture = FALSE;
      	IDirect3DBaseTexture8Impl_SetDirty(cont, TRUE);
	IDirect3DBaseTexture8_PreLoad(cont);
	D3D8_SURFACE(This->renderTarget)->inPBuffer = FALSE;
	IDirect3DBaseTexture8Impl_Release(cont);
	cont = NULL;
      }
    }

    LEAVE_GL();
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_Clear(LPDIRECT3DDEVICE8 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    /* TODO: From MSDN This method fails if you specify the D3DCLEAR_ZBUFFER or D3DCLEAR_STENCIL flags when the
      render target does not have an attached depth buffer. Similarly, if you specify the D3DCLEAR_STENCIL flag
      when the depth-buffer format does not contain stencil buffer information, this method fails. */
    GLbitfield glMask = 0;
    GLboolean old_ztest;
    GLfloat old_z_clear_value;
    GLint   old_stencil_clear_value;
    GLfloat old_color_clear_value[4];
    unsigned int i;
    CONST D3DRECT* curRect;

    TRACE("(%p) Count (%ld), pRects (%p), Flags (%lx), Z (%f), Stencil (%ld)\n", This,
          Count, pRects, Flags, Z, Stencil);

    ENTER_GL();
    glEnable(GL_SCISSOR_TEST);
    checkGLcall("glEnable GL_SCISSOR_TEST");
    if (Count > 0 && pRects) {
        curRect = pRects;
    } else {
        curRect = NULL;
    }

    /* Only set the values up once, as they are not changing */
    if (Flags & D3DCLEAR_STENCIL) {	
        glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &old_stencil_clear_value);
        glClearStencil(Stencil);
        checkGLcall("glClearStencil");
        glMask = glMask | GL_STENCIL_BUFFER_BIT;
        glStencilMask(0xFFFFFFFF);
    }

    if (Flags & D3DCLEAR_ZBUFFER) {
        glGetBooleanv(GL_DEPTH_WRITEMASK, &old_ztest);
        glDepthMask(GL_TRUE); 
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &old_z_clear_value);
        glClearDepth(Z);
        checkGLcall("glClearDepth");
        glMask = glMask | GL_DEPTH_BUFFER_BIT;
    }

    if (Flags & D3DCLEAR_TARGET) {
        TRACE("Clearing screen with glClear to color %lx\n", Color);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, old_color_clear_value);
        glClearColor(D3DCOLOR_R(Color),
		     D3DCOLOR_G(Color),
		     D3DCOLOR_B(Color),
		     D3DCOLOR_A(Color));
        checkGLcall("glClearColor");

        /* Clear ALL colors! */
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glMask = glMask | GL_COLOR_BUFFER_BIT;
    }

    /* Now process each rect in turn */
    for (i = 0; i < Count || i == 0; i++) {

        if (curRect) {
            /* Note gl uses lower left, width/height */
            TRACE("(%p) %p Rect=(%ld,%ld)->(%ld,%ld) glRect=(%ld,%ld), len=%ld, hei=%ld\n", This, curRect,
                  curRect->x1, curRect->y1, curRect->x2, curRect->y2,
                  curRect->x1, (D3D8_SURFACE(This->renderTarget)->currentDesc.Height - curRect->y2), 
                  curRect->x2 - curRect->x1, curRect->y2 - curRect->y1);
            glScissor(curRect->x1, (D3D8_SURFACE(This->renderTarget)->currentDesc.Height - curRect->y2), 
                      curRect->x2 - curRect->x1, curRect->y2 - curRect->y1);
            checkGLcall("glScissor");
        } else {
            glScissor(This->StateBlock->viewport.X, 
                      (D3D8_SURFACE(This->renderTarget)->currentDesc.Height - (This->StateBlock->viewport.Y + This->StateBlock->viewport.Height)), 
                      This->StateBlock->viewport.Width, 
                      This->StateBlock->viewport.Height);
            checkGLcall("glScissor");
        }

        /* Clear the selected rectangle (or full screen) */
        glClear(glMask);
        checkGLcall("glClear");

        /* Step to the next rectangle */
        if (curRect) curRect = curRect + sizeof(D3DRECT);
    }

    /* Restore the old values (why..?) */
    if (Flags & D3DCLEAR_STENCIL) {
        glClearStencil(old_stencil_clear_value);
        glStencilMask(This->StateBlock->renderstate[D3DRS_STENCILWRITEMASK]);
    }    
    if (Flags & D3DCLEAR_ZBUFFER) {
        glDepthMask(old_ztest);
        glClearDepth(old_z_clear_value);
    }
    if (Flags & D3DCLEAR_TARGET) {
        glClearColor(old_color_clear_value[0], 
                     old_color_clear_value[1],
                     old_color_clear_value[2], 
                     old_color_clear_value[3]);
        glColorMask(This->StateBlock->renderstate[D3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE, 
                    This->StateBlock->renderstate[D3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
                    This->StateBlock->renderstate[D3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_BLUE  ? GL_TRUE : GL_FALSE, 
                    This->StateBlock->renderstate[D3DRS_COLORWRITEENABLE] & D3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);
    }

    glDisable(GL_SCISSOR_TEST);
    checkGLcall("glDisable");
    LEAVE_GL();

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE d3dts, CONST D3DMATRIX* lpmatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    unsigned int k;

    /* Most of this routine, comments included copied from ddraw tree initially: */
    TRACE("(%p) : State=%d\n", This, d3dts);

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        This->UpdateStateBlock->Changed.transform[d3dts] = TRUE;
        This->UpdateStateBlock->Set.transform[d3dts] = TRUE;
        memcpy(&This->UpdateStateBlock->transforms[d3dts], lpmatrix, sizeof(D3DMATRIX));
        return D3D_OK;
    }

    /*
     * if the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some dumb reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters
     * but note
     */
    if (!memcmp(&This->StateBlock->transforms[d3dts].u.m[0][0], lpmatrix, sizeof(D3DMATRIX))) {
        TRACE("The app is setting the same matrix over again\n");
        return D3D_OK;
    } else {
        conv_mat(lpmatrix, &This->StateBlock->transforms[d3dts].u.m[0][0]);
    }

    /*
       ScreenCoord = ProjectionMat * ViewMat * WorldMat * ObjectCoord
       where ViewMat = Camera space, WorldMat = world space.

       In OpenGL, camera and world space is combined into GL_MODELVIEW
       matrix.  The Projection matrix stay projection matrix. 
     */

    /* Capture the times we can just ignore the change */
    if (d3dts == D3DTS_WORLDMATRIX(0)) {
        This->modelview_valid = FALSE;
        return D3D_OK;

    } else if (d3dts == D3DTS_PROJECTION) {
        This->proj_valid = FALSE;
        return D3D_OK;

    } else if (d3dts >= D3DTS_WORLDMATRIX(1) && d3dts <= D3DTS_WORLDMATRIX(255)) { /* Indexed Vertex Blending Matrices 256 -> 511  */
        /* Use arb_vertex_blend or NV_VERTEX_WEIGHTING? */
        FIXME("D3DTS_WORLDMATRIX(1..255) not handled\n");
        return D3D_OK;
    } 
    
    /* Chances are we really are going to have to change a matrix */
    ENTER_GL();

    if (d3dts >= D3DTS_TEXTURE0 && d3dts <= D3DTS_TEXTURE7) { /* handle texture matrices */
        if (d3dts < GL_LIMITS(textures)) {
            int tex = d3dts - D3DTS_TEXTURE0;
#if defined(GL_VERSION_1_3)
            glActiveTexture(GL_TEXTURE0 + tex);
            checkGLcall("glActiveTexture");
#else 
            glActiveTextureARB(GL_TEXTURE0_ARB + tex);
            checkGLcall("glActiveTextureARB");
#endif
            set_texture_matrix((const float *)lpmatrix, This->UpdateStateBlock->texture_state[tex][D3DTSS_TEXTURETRANSFORMFLAGS]);
        }

    } else if (d3dts == D3DTS_VIEW) { /* handle the VIEW matrice */

        PLIGHTINFOEL *lightChain = NULL;
        float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        This->modelview_valid = FALSE;
        This->view_ident = !memcmp(lpmatrix, identity, 16*sizeof(float));
        glMatrixMode(GL_MODELVIEW);
        checkGLcall("glMatrixMode(GL_MODELVIEW)");
        glPushMatrix();
        glLoadMatrixf((const float *)lpmatrix);
        checkGLcall("glLoadMatrixf(...)");

        /* If we are changing the View matrix, reset the light and clipping planes to the new view   
         * NOTE: We have to reset the positions even if the light/plane is not currently
         *       enabled, since the call to enable it will not reset the position.                 
         * NOTE2: Apparently texture transforms do NOT need reapplying
         */

        /* Reset lights */
        lightChain = This->StateBlock->lights;
        while (lightChain && lightChain->glIndex != -1) {
            glLightfv(GL_LIGHT0 + lightChain->glIndex, GL_POSITION, lightChain->lightPosn);
            checkGLcall("glLightfv posn");
            glLightfv(GL_LIGHT0 + lightChain->glIndex, GL_SPOT_DIRECTION, lightChain->lightDirn);
            checkGLcall("glLightfv dirn");
            lightChain = lightChain->next;
        }
        /* Reset Clipping Planes if clipping is enabled */
        for (k = 0; k < GL_LIMITS(clipplanes); k++) {
            glClipPlane(GL_CLIP_PLANE0 + k, This->StateBlock->clipplane[k]);
            checkGLcall("glClipPlane");
        }
        glPopMatrix();

    } else { /* What was requested!?? */
        WARN("invalid matrix specified: %i\n", d3dts);

    }

    /* Release lock, all done */
    LEAVE_GL();
    return D3D_OK;

}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : for State %d\n", This, State);
    memcpy(pMatrix, &This->StateBlock->transforms[State], sizeof(D3DMATRIX));
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_MultiplyTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    D3DMATRIX *mat = NULL;
    D3DMATRIX temp;

    /* Note: Using UpdateStateBlock means it would be recorded in a state block change,
        but works regardless of recording being on. 
        If this is found to be wrong, change to StateBlock.                             */
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : For state %u\n", This, State);

    if (State < HIGHEST_TRANSFORMSTATE)
    {
        mat = &This->UpdateStateBlock->transforms[State];
    } else {
        FIXME("Unhandled transform state!!\n");
    }

    /* Copied from ddraw code:  */
    temp.u.s._11 = (mat->u.s._11 * pMatrix->u.s._11) + (mat->u.s._21 * pMatrix->u.s._12) + (mat->u.s._31 * pMatrix->u.s._13) + (mat->u.s._41 * pMatrix->u.s._14);
    temp.u.s._21 = (mat->u.s._11 * pMatrix->u.s._21) + (mat->u.s._21 * pMatrix->u.s._22) + (mat->u.s._31 * pMatrix->u.s._23) + (mat->u.s._41 * pMatrix->u.s._24);
    temp.u.s._31 = (mat->u.s._11 * pMatrix->u.s._31) + (mat->u.s._21 * pMatrix->u.s._32) + (mat->u.s._31 * pMatrix->u.s._33) + (mat->u.s._41 * pMatrix->u.s._34);
    temp.u.s._41 = (mat->u.s._11 * pMatrix->u.s._41) + (mat->u.s._21 * pMatrix->u.s._42) + (mat->u.s._31 * pMatrix->u.s._43) + (mat->u.s._41 * pMatrix->u.s._44);

    temp.u.s._12 = (mat->u.s._12 * pMatrix->u.s._11) + (mat->u.s._22 * pMatrix->u.s._12) + (mat->u.s._32 * pMatrix->u.s._13) + (mat->u.s._42 * pMatrix->u.s._14);
    temp.u.s._22 = (mat->u.s._12 * pMatrix->u.s._21) + (mat->u.s._22 * pMatrix->u.s._22) + (mat->u.s._32 * pMatrix->u.s._23) + (mat->u.s._42 * pMatrix->u.s._24);
    temp.u.s._32 = (mat->u.s._12 * pMatrix->u.s._31) + (mat->u.s._22 * pMatrix->u.s._32) + (mat->u.s._32 * pMatrix->u.s._33) + (mat->u.s._42 * pMatrix->u.s._34);
    temp.u.s._42 = (mat->u.s._12 * pMatrix->u.s._41) + (mat->u.s._22 * pMatrix->u.s._42) + (mat->u.s._32 * pMatrix->u.s._43) + (mat->u.s._42 * pMatrix->u.s._44);

    temp.u.s._13 = (mat->u.s._13 * pMatrix->u.s._11) + (mat->u.s._23 * pMatrix->u.s._12) + (mat->u.s._33 * pMatrix->u.s._13) + (mat->u.s._43 * pMatrix->u.s._14);
    temp.u.s._23 = (mat->u.s._13 * pMatrix->u.s._21) + (mat->u.s._23 * pMatrix->u.s._22) + (mat->u.s._33 * pMatrix->u.s._23) + (mat->u.s._43 * pMatrix->u.s._24);
    temp.u.s._33 = (mat->u.s._13 * pMatrix->u.s._31) + (mat->u.s._23 * pMatrix->u.s._32) + (mat->u.s._33 * pMatrix->u.s._33) + (mat->u.s._43 * pMatrix->u.s._34);
    temp.u.s._43 = (mat->u.s._13 * pMatrix->u.s._41) + (mat->u.s._23 * pMatrix->u.s._42) + (mat->u.s._33 * pMatrix->u.s._43) + (mat->u.s._43 * pMatrix->u.s._44);

    temp.u.s._14 = (mat->u.s._14 * pMatrix->u.s._11) + (mat->u.s._24 * pMatrix->u.s._12) + (mat->u.s._34 * pMatrix->u.s._13) + (mat->u.s._44 * pMatrix->u.s._14);
    temp.u.s._24 = (mat->u.s._14 * pMatrix->u.s._21) + (mat->u.s._24 * pMatrix->u.s._22) + (mat->u.s._34 * pMatrix->u.s._23) + (mat->u.s._44 * pMatrix->u.s._24);
    temp.u.s._34 = (mat->u.s._14 * pMatrix->u.s._31) + (mat->u.s._24 * pMatrix->u.s._32) + (mat->u.s._34 * pMatrix->u.s._33) + (mat->u.s._44 * pMatrix->u.s._34);
    temp.u.s._44 = (mat->u.s._14 * pMatrix->u.s._41) + (mat->u.s._24 * pMatrix->u.s._42) + (mat->u.s._34 * pMatrix->u.s._43) + (mat->u.s._44 * pMatrix->u.s._44);

    /* Apply change via set transform - will reapply to eg. lights this way */
    IDirect3DDevice8Impl_SetTransform(iface, State, &temp);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetViewport(LPDIRECT3DDEVICE8 iface, CONST D3DVIEWPORT8* pViewport) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p)\n", This);
    This->UpdateStateBlock->Changed.viewport = TRUE;
    This->UpdateStateBlock->Set.viewport = TRUE;
    memcpy(&This->UpdateStateBlock->viewport, pViewport, sizeof(D3DVIEWPORT8));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    ENTER_GL();

    TRACE("(%p) : x=%ld, y=%ld, wid=%ld, hei=%ld, minz=%f, maxz=%f\n", This,
          pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height, pViewport->MinZ, pViewport->MaxZ);

    glDepthRange(pViewport->MinZ, pViewport->MaxZ);
    checkGLcall("glDepthRange");
    /* Note: GL requires lower left, DirectX supplies upper left */
    glViewport(pViewport->X, (D3D8_SURFACE(This->renderTarget)->currentDesc.Height - (pViewport->Y + pViewport->Height)), 
               pViewport->Width, pViewport->Height);
    checkGLcall("glViewport");

    LEAVE_GL();

    return D3D_OK;

}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetViewport(LPDIRECT3DDEVICE8 iface, D3DVIEWPORT8* pViewport) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p)\n", This);
    memcpy(pViewport, &This->StateBlock->viewport, sizeof(D3DVIEWPORT8));
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetMaterial(LPDIRECT3DDEVICE8 iface, CONST D3DMATERIAL8* pMaterial) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

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

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*) &This->UpdateStateBlock->material.Ambient);
    checkGLcall("glMaterialfv");
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*) &This->UpdateStateBlock->material.Diffuse);
    checkGLcall("glMaterialfv");

    /* Only change material color if specular is enabled, otherwise it is set to black */
    if (This->StateBlock->renderstate[D3DRS_SPECULARENABLE]) {
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &This->UpdateStateBlock->material.Specular);
       checkGLcall("glMaterialfv");
    } else {
       float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
       checkGLcall("glMaterialfv");
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (float*) &This->UpdateStateBlock->material.Emissive);
    checkGLcall("glMaterialfv");
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, This->UpdateStateBlock->material.Power);
    checkGLcall("glMaterialf");

    LEAVE_GL();
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetMaterial(LPDIRECT3DDEVICE8 iface, D3DMATERIAL8* pMaterial) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    memcpy(pMaterial, &This->UpdateStateBlock->material, sizeof (D3DMATERIAL8));
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g, pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g, pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g, pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g, pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);
    return D3D_OK;
}

/* Note lights are real special cases. Although the device caps state only eg. 8 are supported,
   you can reference any indexes you want as long as that number max are enabled are any
   one point in time! Therefore since the indexes can be anything, we need a linked list of them.
   However, this causes stateblock problems. When capturing the state block, I duplicate the list,
   but when recording, just build a chain pretty much of commands to be replayed.                  */
   
HRESULT  WINAPI  IDirect3DDevice8Impl_SetLight(LPDIRECT3DDEVICE8 iface, DWORD Index, CONST D3DLIGHT8* pLight) {
    float rho;
    PLIGHTINFOEL *object, *temp;

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : Idx(%ld), pLight(%p)\n", This, Index, pLight);

    /* If recording state block, just add to end of lights chain */
    if (This->isRecordingState) {
        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLIGHTINFOEL));
        if (NULL == object) {
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        memcpy(&object->OriginalParms, pLight, sizeof(D3DLIGHT8));
        object->OriginalIndex = Index;
        object->glIndex = -1;
        object->changed = TRUE;

        /* Add to the END of the chain of lights changes to be replayed */
        if (This->UpdateStateBlock->lights == NULL) {
            This->UpdateStateBlock->lights = object;
        } else {
            temp = This->UpdateStateBlock->lights;
            while (temp->next != NULL) temp=temp->next;
            temp->next = object;
        }
        TRACE("Recording... not performing anything more\n");
        return D3D_OK;
    }

    /* Ok, not recording any longer so do real work */
    object = This->StateBlock->lights;
    while (object != NULL && object->OriginalIndex != Index) object = object->next;

    /* If we didn't find it in the list of lights, time to add it */
    if (object == NULL) {
        PLIGHTINFOEL *insertAt,*prevPos;

        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLIGHTINFOEL));
        if (NULL == object) {
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        object->OriginalIndex = Index;
        object->glIndex = -1;

        /* Add it to the front of list with the idea that lights will be changed as needed 
           BUT after any lights currently assigned GL indexes                             */
        insertAt = This->StateBlock->lights;
        prevPos  = NULL;
        while (insertAt != NULL && insertAt->glIndex != -1) {
            prevPos  = insertAt;
            insertAt = insertAt->next;
        }

        if (insertAt == NULL && prevPos == NULL) { /* Start of list */
            This->StateBlock->lights = object;
        } else if (insertAt == NULL) { /* End of list */
            prevPos->next = object;
            object->prev = prevPos;
        } else { /* Middle of chain */
            if (prevPos == NULL) {
                This->StateBlock->lights = object;
            } else {
                prevPos->next = object;
            }
            object->prev = prevPos;
            object->next = insertAt;
            insertAt->prev = object;
        }
    }

    /* Initialze the object */
    TRACE("Light %ld setting to type %d, Diffuse(%f,%f,%f,%f), Specular(%f,%f,%f,%f), Ambient(%f,%f,%f,%f)\n", Index, pLight->Type,
          pLight->Diffuse.r, pLight->Diffuse.g, pLight->Diffuse.b, pLight->Diffuse.a,
          pLight->Specular.r, pLight->Specular.g, pLight->Specular.b, pLight->Specular.a,
          pLight->Ambient.r, pLight->Ambient.g, pLight->Ambient.b, pLight->Ambient.a);
    TRACE("... Pos(%f,%f,%f), Dirn(%f,%f,%f)\n", pLight->Position.x, pLight->Position.y, pLight->Position.z,
          pLight->Direction.x, pLight->Direction.y, pLight->Direction.z);
    TRACE("... Range(%f), Falloff(%f), Theta(%f), Phi(%f)\n", pLight->Range, pLight->Falloff, pLight->Theta, pLight->Phi);

    /* Save away the information */
    memcpy(&object->OriginalParms, pLight, sizeof(D3DLIGHT8));

    switch (pLight->Type) {
    case D3DLIGHT_POINT:
        /* Position */
        object->lightPosn[0] = pLight->Position.x;
        object->lightPosn[1] = pLight->Position.y;
        object->lightPosn[2] = pLight->Position.z;
        object->lightPosn[3] = 1.0f;
        object->cutoff = 180.0f;
        /* FIXME: Range */
        break;

    case D3DLIGHT_DIRECTIONAL:
        /* Direction */
        object->lightPosn[0] = -pLight->Direction.x;
        object->lightPosn[1] = -pLight->Direction.y;
        object->lightPosn[2] = -pLight->Direction.z;
        object->lightPosn[3] = 0.0;
        object->exponent     = 0.0f;
        object->cutoff       = 180.0f;
        break;

    case D3DLIGHT_SPOT:
        /* Position */
        object->lightPosn[0] = pLight->Position.x;
        object->lightPosn[1] = pLight->Position.y;
        object->lightPosn[2] = pLight->Position.z;
        object->lightPosn[3] = 1.0;

        /* Direction */
        object->lightDirn[0] = pLight->Direction.x;
        object->lightDirn[1] = pLight->Direction.y;
        object->lightDirn[2] = pLight->Direction.z;
        object->lightDirn[3] = 1.0;

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
        object->exponent = -0.3/log(cos(rho/2));
        object->cutoff = pLight->Phi*90/M_PI;

        /* FIXME: Range */
        break;

    default:
        FIXME("Unrecognized light type %d\n", pLight->Type);
    }

    /* Update the live definitions if the light is currently assigned a glIndex */
    if (object->glIndex != -1) {
        setup_light(iface, object->glIndex, object);
    }
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetLight(LPDIRECT3DDEVICE8 iface, DWORD Index,D3DLIGHT8* pLight) {
    PLIGHTINFOEL *lightInfo = NULL;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface; 
    TRACE("(%p) : Idx(%ld), pLight(%p)\n", This, Index, pLight);
    
    /* Locate the light in the live lights */
    lightInfo = This->StateBlock->lights;
    while (lightInfo != NULL && lightInfo->OriginalIndex != Index) lightInfo = lightInfo->next;

    if (lightInfo == NULL) {
        TRACE("Light information requested but light not defined\n");
        return D3DERR_INVALIDCALL;
    }

    memcpy(pLight, &lightInfo->OriginalParms, sizeof(D3DLIGHT8));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_LightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL Enable) {
    PLIGHTINFOEL *lightInfo = NULL;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : Idx(%ld), enable? %d\n", This, Index, Enable);

    /* If recording state block, just add to end of lights chain with changedEnable set to true */
    if (This->isRecordingState) {
        lightInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PLIGHTINFOEL));
        if (NULL == lightInfo) {
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        lightInfo->OriginalIndex = Index;
        lightInfo->glIndex = -1;
        lightInfo->enabledChanged = TRUE;

        /* Add to the END of the chain of lights changes to be replayed */
        if (This->UpdateStateBlock->lights == NULL) {
            This->UpdateStateBlock->lights = lightInfo;
        } else {
            PLIGHTINFOEL *temp = This->UpdateStateBlock->lights;
            while (temp->next != NULL) temp=temp->next;
            temp->next = lightInfo;
        }
        TRACE("Recording... not performing anything more\n");
        return D3D_OK;
    }

    /* Not recording... So, locate the light in the live lights */
    lightInfo = This->StateBlock->lights;
    while (lightInfo != NULL && lightInfo->OriginalIndex != Index) lightInfo = lightInfo->next;

    /* Special case - enabling an undefined light creates one with a strict set of parms! */
    if (lightInfo == NULL) {
        D3DLIGHT8 lightParms;
        /* Warning - untested code :-) Prob safe to change fixme to a trace but
             wait until someone confirms it seems to work!                     */
        TRACE("Light enabled requested but light not defined, so defining one!\n"); 
        lightParms.Type = D3DLIGHT_DIRECTIONAL;
        lightParms.Diffuse.r = 1.0;
        lightParms.Diffuse.g = 1.0;
        lightParms.Diffuse.b = 1.0;
        lightParms.Diffuse.a = 0.0;
        lightParms.Specular.r = 0.0;
        lightParms.Specular.g = 0.0;
        lightParms.Specular.b = 0.0;
        lightParms.Specular.a = 0.0;
        lightParms.Ambient.r = 0.0;
        lightParms.Ambient.g = 0.0;
        lightParms.Ambient.b = 0.0;
        lightParms.Ambient.a = 0.0;
        lightParms.Position.x = 0.0;
        lightParms.Position.y = 0.0;
        lightParms.Position.z = 0.0;
        lightParms.Direction.x = 0.0;
        lightParms.Direction.y = 0.0;
        lightParms.Direction.z = 1.0;
        lightParms.Range = 0.0;
        lightParms.Falloff = 0.0;
        lightParms.Attenuation0 = 0.0;
        lightParms.Attenuation1 = 0.0;
        lightParms.Attenuation2 = 0.0;
        lightParms.Theta = 0.0;
        lightParms.Phi = 0.0;
        IDirect3DDevice8Impl_SetLight(iface, Index, &lightParms);

        /* Search for it again! Should be fairly quick as near head of list */
        lightInfo = This->StateBlock->lights;
        while (lightInfo != NULL && lightInfo->OriginalIndex != Index) lightInfo = lightInfo->next;
        if (lightInfo == NULL) {
            FIXME("Adding default lights has failed dismally\n");
            return D3DERR_INVALIDCALL;
        }
    }

    /* OK, we now have a light... */
    if (Enable == FALSE) {

        /* If we are disabling it, check it was enabled, and
           still only do something if it has assigned a glIndex (which it should have!)   */
        if (lightInfo->lightEnabled && (lightInfo->glIndex != -1)) {
            TRACE("Disabling light set up at gl idx %ld\n", lightInfo->glIndex);
            ENTER_GL();
            glDisable(GL_LIGHT0 + lightInfo->glIndex);
            checkGLcall("glDisable GL_LIGHT0+Index");
            LEAVE_GL();
        } else {
            TRACE("Nothing to do as light was not enabled\n");
        }
        lightInfo->lightEnabled = FALSE;
    } else {

        /* We are enabling it. If it is enabled, its really simple */
        if (lightInfo->lightEnabled) {
            /* nop */
            TRACE("Nothing to do as light was enabled\n");

        /* If it already has a glIndex, its still simple */
        } else if (lightInfo->glIndex != -1) {
            TRACE("Reusing light as already set up at gl idx %ld\n", lightInfo->glIndex);
            lightInfo->lightEnabled = TRUE;
            ENTER_GL();
            glEnable(GL_LIGHT0 + lightInfo->glIndex);
            checkGLcall("glEnable GL_LIGHT0+Index already setup");
            LEAVE_GL();

        /* Otherwise got to find space - lights are ordered gl indexes first */
        } else {
            PLIGHTINFOEL *bsf  = NULL;
            PLIGHTINFOEL *pos  = This->StateBlock->lights;
            PLIGHTINFOEL *prev = NULL;
            int           Index= 0;
            int           glIndex = -1;

            /* Try to minimize changes as much as possible */
            while (pos != NULL && pos->glIndex != -1 && Index < This->maxConcurrentLights) {

                /* Try to remember which index can be replaced if necessary */
                if (bsf==NULL && pos->lightEnabled == FALSE) {
                    /* Found a light we can replace, save as best replacement */
                    bsf = pos;
                }

                /* Step to next space */
                prev = pos;
                pos = pos->next;
                Index ++;
            }

            /* If we have too many active lights, fail the call */
            if ((Index == This->maxConcurrentLights) && (bsf == NULL)) {
                FIXME("Program requests too many concurrent lights\n");
                return D3DERR_INVALIDCALL;

            /* If we have allocated all lights, but not all are enabled,
               reuse one which is not enabled                           */
            } else if (Index == This->maxConcurrentLights) {
                /* use bsf - Simply swap the new light and the BSF one */
                PLIGHTINFOEL *bsfNext = bsf->next;
                PLIGHTINFOEL *bsfPrev = bsf->prev;

                /* Sort out ends */
                if (lightInfo->next != NULL) lightInfo->next->prev = bsf;
                if (bsf->prev != NULL) {
                    bsf->prev->next = lightInfo;
                } else {
                    This->StateBlock->lights = lightInfo;
                }

                /* If not side by side, lots of chains to update */
                if (bsf->next != lightInfo) {
                    lightInfo->prev->next = bsf;
                    bsf->next->prev = lightInfo;
                    bsf->next       = lightInfo->next;
                    bsf->prev       = lightInfo->prev;
                    lightInfo->next = bsfNext;
                    lightInfo->prev = bsfPrev;

                } else {
                    /* Simple swaps */
                    bsf->prev = lightInfo;
                    bsf->next = lightInfo->next;
                    lightInfo->next = bsf;
                    lightInfo->prev = bsfPrev;
                }


                /* Update states */
                glIndex = bsf->glIndex;
                bsf->glIndex = -1;
                lightInfo->glIndex = glIndex;
                lightInfo->lightEnabled = TRUE;

                /* Finally set up the light in gl itself */
                TRACE("Replacing light which was set up at gl idx %ld\n", lightInfo->glIndex);
                ENTER_GL();
                setup_light(iface, glIndex, lightInfo);
                glEnable(GL_LIGHT0 + glIndex);
                checkGLcall("glEnable GL_LIGHT0 new setup");
                LEAVE_GL();

            /* If we reached the end of the allocated lights, with space in the
               gl lights, setup a new light                                     */
            } else if (pos->glIndex == -1) {

                /* We reached the end of the allocated gl lights, so already 
                    know the index of the next one!                          */
                glIndex = Index;
                lightInfo->glIndex = glIndex;
                lightInfo->lightEnabled = TRUE;

                /* In an ideal world, its already in the right place */
                if (lightInfo->prev == NULL || lightInfo->prev->glIndex!=-1) {
                   /* No need to move it */
                } else {
                    /* Remove this light from the list */
                    lightInfo->prev->next = lightInfo->next;
                    if (lightInfo->next != NULL) {
                        lightInfo->next->prev = lightInfo->prev;
                    }

                    /* Add in at appropriate place (inbetween prev and pos) */
                    lightInfo->prev = prev;
                    lightInfo->next = pos;
                    if (prev == NULL) {
                        This->StateBlock->lights = lightInfo;
                    } else {
                        prev->next = lightInfo;
                    }
                    if (pos != NULL) {
                        pos->prev = lightInfo;
                    }
                }

                /* Finally set up the light in gl itself */
                TRACE("Defining new light at gl idx %ld\n", lightInfo->glIndex);
                ENTER_GL();
                setup_light(iface, glIndex, lightInfo);
                glEnable(GL_LIGHT0 + glIndex);
                checkGLcall("glEnable GL_LIGHT0 new setup");
                LEAVE_GL();
                
            }
        }
    }
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetLightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL* pEnable) {

    PLIGHTINFOEL *lightInfo = NULL;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface; 
    TRACE("(%p) : for idx(%ld)\n", This, Index);
    
    /* Locate the light in the live lights */
    lightInfo = This->StateBlock->lights;
    while (lightInfo != NULL && lightInfo->OriginalIndex != Index) lightInfo = lightInfo->next;

    if (lightInfo == NULL) {
        TRACE("Light enabled state requested but light not defined\n");
        return D3DERR_INVALIDCALL;
    }
    *pEnable = lightInfo->lightEnabled;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST float* pPlane) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : for idx %ld, %p\n", This, Index, pPlane);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesn't support\n");
        return D3DERR_INVALIDCALL;
    }

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

    ENTER_GL();

    /* Clip Plane settings are affected by the model view in OpenGL, the View transform in direct3d */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0]);

    TRACE("Clipplane [%f,%f,%f,%f]\n", 
	  This->UpdateStateBlock->clipplane[Index][0], 
	  This->UpdateStateBlock->clipplane[Index][1],
          This->UpdateStateBlock->clipplane[Index][2], 
	  This->UpdateStateBlock->clipplane[Index][3]);
    glClipPlane(GL_CLIP_PLANE0 + Index, This->UpdateStateBlock->clipplane[Index]);
    checkGLcall("glClipPlane");

    glPopMatrix();

    LEAVE_GL();

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,float* pPlane) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : for idx %ld\n", This, Index);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesn't support\n");
        return D3DERR_INVALIDCALL;
    }

    pPlane[0] = This->StateBlock->clipplane[Index][0];
    pPlane[1] = This->StateBlock->clipplane[Index][1];
    pPlane[2] = This->StateBlock->clipplane[Index][2];
    pPlane[3] = This->StateBlock->clipplane[Index][3];
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD Value) {
    union {
	DWORD d;
	float f;
    } tmpvalue;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    DWORD OldValue = This->StateBlock->renderstate[State];
        
    TRACE("(%p)->state = %s(%d), value = %ld\n", This, debug_d3drenderstate(State), State, Value);
    This->UpdateStateBlock->Changed.renderstate[State] = TRUE;
    This->UpdateStateBlock->Set.renderstate[State] = TRUE;
    This->UpdateStateBlock->renderstate[State] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    ENTER_GL();

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
            glEnable(GL_DEPTH_TEST);
            checkGLcall("glEnable GL_DEPTH_TEST");
            FIXME("W buffer is not well handled\n");
            break;
        default:
            FIXME("Unrecognized D3DZBUFFERTYPE value %ld\n", Value);
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
            if (This->renderUpsideDown) {
                glFrontFace(GL_CW);
                checkGLcall("glFrontFace GL_CW");
            } else {
                glFrontFace(GL_CCW);
                checkGLcall("glFrontFace GL_CCW");
            }
            glCullFace(GL_BACK);
            break;
        case D3DCULL_CCW:
            glEnable(GL_CULL_FACE);
            checkGLcall("glEnable GL_CULL_FACE");
            if (This->renderUpsideDown) {
                glFrontFace(GL_CCW); 
                checkGLcall("glFrontFace GL_CCW");
            } else {
                glFrontFace(GL_CW);
                checkGLcall("glFrontFace GL_CW");
            }
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
            FIXME("D3DSHADE_PHONG isn't supported?\n");

	    LEAVE_GL();
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
	    D3DCOLORTOGLFLOAT4(Value, col);
            TRACE("Setting ambient to (%f,%f,%f,%f)\n", col[0], col[1], col[2], col[3]);
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
            float ref = ((float) This->StateBlock->renderstate[D3DRS_ALPHAREF]) / 255.0f;

            switch ((D3DCMPFUNC) Value) {
            case D3DCMP_NEVER:         glParm = GL_NEVER; break;
            case D3DCMP_LESS:          glParm = GL_LESS; break;
            case D3DCMP_EQUAL:         glParm = GL_EQUAL; break;
            case D3DCMP_LESSEQUAL:     glParm = GL_LEQUAL; break;
            case D3DCMP_GREATER:       glParm = GL_GREATER; break;
            case D3DCMP_NOTEQUAL:      glParm = GL_NOTEQUAL; break;
            case D3DCMP_GREATEREQUAL:  glParm = GL_GEQUAL; break;
            case D3DCMP_ALWAYS:        glParm = GL_ALWAYS; break;
            default:
                FIXME("Unrecognized/Unhandled D3DCMPFUNC value %ld\n", Value);
            }
            TRACE("glAlphaFunc with Parm=%x, ref=%f\n", glParm, ref);
            glAlphaFunc(glParm, ref);
            This->alphafunc = glParm;
            checkGLcall("glAlphaFunc");
        }
        break;

    case D3DRS_ALPHAREF                  :
        {
            int glParm = This->alphafunc;
            float ref = 1.0f;

            ref = ((float) Value) / 255.0f;
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
                    enable  = This->StateBlock->renderstate[D3DRS_CLIPPLANEENABLE];
                    disable = 0x00;
                } else {
                    disable = This->StateBlock->renderstate[D3DRS_CLIPPLANEENABLE];
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

	    /** update clipping status */
	    if (enable) {
	      This->StateBlock->clip_status.ClipUnion = 0;
	      This->StateBlock->clip_status.ClipIntersection = 0xFFFFFFFF;
	    } else {
	      This->StateBlock->clip_status.ClipUnion = 0;
	      This->StateBlock->clip_status.ClipIntersection = 0;
	    }
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
            TRACE("glBlendEquation(%x)\n", glParm);
            glBlendEquation(glParm);
            checkGLcall("glBlendEquation");
        }
        break;

    case D3DRS_TEXTUREFACTOR             :
        {
            unsigned int i;

            /* Note the texture color applies to all textures whereas 
               GL_TEXTURE_ENV_COLOR applies to active only */
            float col[4];
	    D3DCOLORTOGLFLOAT4(Value, col);
            /* Set the default alpha blend color */
            glBlendColor(col[0], col[1], col[2], col[3]);
            checkGLcall("glBlendColor");

            /* And now the default texture color as well */
            for (i = 0; i < GL_LIMITS(textures); i++) {

                /* Note the D3DRS value applies to all textures, but GL has one
                   per texture, so apply it now ready to be used!               */
                if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
                    glActiveTexture(GL_TEXTURE0 + i);
#else
                    glActiveTextureARB(GL_TEXTURE0_ARB + i);
#endif
                    checkGLcall("Activate texture.. to update const color");
                } else if (i>0) {
                    FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
                }

                glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
                checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");
            }
        }
        break;

    case D3DRS_SPECULARENABLE            : 
        {
            /* Originally this used glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR)
               and (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR) to swap between enabled/disabled
               specular color. This is wrong:
               Separate specular color means the specular colour is maintained separately, whereas
               single color means it is merged in. However in both cases they are being used to
               some extent.
               To disable specular color, set it explicitly to black and turn off GL_COLOR_SUM_EXT
               NOTE: If not supported don't give FIXMEs the impact is really minimal and very few people are
                  running 1.4 yet!
             */
              if (Value) {
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &This->UpdateStateBlock->material.Specular);
                checkGLcall("glMaterialfv");
		if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
		  glEnable(GL_COLOR_SUM_EXT);
		} else {
		  TRACE("Specular colors cannot be enabled in this version of opengl\n");
		}
                checkGLcall("glEnable(GL_COLOR_SUM)");
              } else {
                float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};

                /* for the case of enabled lighting: */
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &black[0]);
                checkGLcall("glMaterialfv");

                /* for the case of disabled lighting: */
		if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
		  glDisable(GL_COLOR_SUM_EXT);
		} else {
		  TRACE("Specular colors cannot be disabled in this version of opengl\n");
		}
                checkGLcall("glDisable(GL_COLOR_SUM)");
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
           int ref = This->StateBlock->renderstate[D3DRS_STENCILREF];
           GLuint mask = This->StateBlock->renderstate[D3DRS_STENCILMASK];

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
	   This->stencilfunc = glParm;
	   glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case D3DRS_STENCILREF                :
        {
           int glParm = This->stencilfunc;
           int ref = 0;
           GLuint mask = This->StateBlock->renderstate[D3DRS_STENCILMASK];

           ref = Value;
           TRACE("glStencilFunc with Parm=%x, ref=%d, mask=%x\n", glParm, ref, mask);
           glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case D3DRS_STENCILMASK               :
        {
           int glParm = This->stencilfunc;
           int ref = This->StateBlock->renderstate[D3DRS_STENCILREF];
           GLuint mask = Value;

           TRACE("glStencilFunc with Parm=%x, ref=%d, mask=%x\n", glParm, ref, mask);
           glStencilFunc(glParm, ref, mask);
           checkGLcall("glStencilFunc");
        }
        break;

    case D3DRS_STENCILFAIL               :
        {
            GLenum fail  ; 
            GLint  zpass ; 
            GLint  zfail ; 

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
            GLint  fail  ; 
            GLint  zpass ; 
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
            GLint  fail  ; 
            GLenum zpass ; 
            GLint  zfail ; 

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
            TRACE("glStencilMask(%lu)\n", Value);
            checkGLcall("glStencilMask");
        }
        break;

    case D3DRS_FOGENABLE                 :
        {
	  if (Value/* && This->StateBlock->renderstate[D3DRS_FOGTABLEMODE] != D3DFOG_NONE*/) {
               glEnable(GL_FOG);
               checkGLcall("glEnable GL_FOG");
            } else {
               glDisable(GL_FOG);
               checkGLcall("glDisable GL_FOG");
            }
        }
        break;

    case D3DRS_RANGEFOGENABLE            :
        {
            if (Value) {
	      TRACE("Enabled RANGEFOG");
            } else {
	      TRACE("Disabled RANGEFOG");
            }
        }
	break;

    case D3DRS_FOGCOLOR                  :
        {
            float col[4];
	    D3DCOLORTOGLFLOAT4(Value, col);
            /* Set the default alpha blend color */
            glFogfv(GL_FOG_COLOR, &col[0]);
            checkGLcall("glFog GL_FOG_COLOR");
        }
        break;

    case D3DRS_FOGTABLEMODE              :
        { 
	  glHint(GL_FOG_HINT, GL_NICEST);
	  switch (Value) {
	  case D3DFOG_NONE:    /* I don't know what to do here */ checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break; 
	  case D3DFOG_EXP:     glFogi(GL_FOG_MODE, GL_EXP); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break; 
	  case D3DFOG_EXP2:    glFogi(GL_FOG_MODE, GL_EXP2); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2"); break; 
	  case D3DFOG_LINEAR:  glFogi(GL_FOG_MODE, GL_LINEAR); checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR"); break; 
	  default:
	    FIXME("Unsupported Value(%lu) for D3DRS_FOGTABLEMODE!\n", Value);
	  }
	  if (GL_SUPPORT(NV_FOG_DISTANCE)) {
	    glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV);
	  }
        }
	break;

    case D3DRS_FOGVERTEXMODE             :
        { 
	  glHint(GL_FOG_HINT, GL_FASTEST);
	  switch (Value) {
	  case D3DFOG_NONE:    /* I don't know what to do here */ checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break; 
	  case D3DFOG_EXP:     glFogi(GL_FOG_MODE, GL_EXP); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break; 
	  case D3DFOG_EXP2:    glFogi(GL_FOG_MODE, GL_EXP2); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2"); break; 
	  case D3DFOG_LINEAR:  glFogi(GL_FOG_MODE, GL_LINEAR); checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR"); break; 
	  default:
	    FIXME("Unsupported Value(%lu) for D3DRS_FOGTABLEMODE!\n", Value);
	  }
	  if (GL_SUPPORT(NV_FOG_DISTANCE)) {
	    glFogi(GL_FOG_DISTANCE_MODE_NV, This->StateBlock->renderstate[D3DRS_RANGEFOGENABLE] ? GL_EYE_RADIAL_NV : GL_EYE_PLANE_ABSOLUTE_NV);
	  }
	}
	break;

    case D3DRS_FOGSTART                  :
        {
	    tmpvalue.d = Value;
            glFogfv(GL_FOG_START, &tmpvalue.f);
            checkGLcall("glFogf(GL_FOG_START, (float) Value)");
            TRACE("Fog Start == %f\n", tmpvalue.f);
        }
        break;

    case D3DRS_FOGEND                    :
        {
            tmpvalue.d = Value;
            glFogfv(GL_FOG_END, &tmpvalue.f);
            checkGLcall("glFogf(GL_FOG_END, (float) Value)");
            TRACE("Fog End == %f\n", tmpvalue.f);
        }
        break;

    case D3DRS_FOGDENSITY                :
        {
            tmpvalue.d = Value;
            glFogfv(GL_FOG_DENSITY, &tmpvalue.f);
            checkGLcall("glFogf(GL_FOG_DENSITY, (float) Value)");
        }
        break;

    case D3DRS_VERTEXBLEND               :
        {
	  This->UpdateStateBlock->vertex_blend = (D3DVERTEXBLENDFLAGS) Value;
	  TRACE("Vertex Blending state to %ld\n",  Value);
        }
	break;

    case D3DRS_TWEENFACTOR               :
        {
	  tmpvalue.d = Value;
	  This->UpdateStateBlock->tween_factor = tmpvalue.f;
	  TRACE("Vertex Blending Tween Factor to %f\n", This->UpdateStateBlock->tween_factor);
        }
	break;

    case D3DRS_INDEXEDVERTEXBLENDENABLE  :
        {
	  TRACE("Indexed Vertex Blend Enable to %ul\n", (BOOL) Value);
        }
	break;

    case D3DRS_COLORVERTEX               :
    case D3DRS_DIFFUSEMATERIALSOURCE     :
    case D3DRS_SPECULARMATERIALSOURCE    :
    case D3DRS_AMBIENTMATERIALSOURCE     :
    case D3DRS_EMISSIVEMATERIALSOURCE    :
        {
            GLenum Parm = GL_AMBIENT_AND_DIFFUSE;

            if (This->StateBlock->renderstate[D3DRS_COLORVERTEX]) {
                TRACE("diff %ld, amb %ld, emis %ld, spec %ld\n",
                      This->StateBlock->renderstate[D3DRS_DIFFUSEMATERIALSOURCE],
                      This->StateBlock->renderstate[D3DRS_AMBIENTMATERIALSOURCE],
                      This->StateBlock->renderstate[D3DRS_EMISSIVEMATERIALSOURCE],
                      This->StateBlock->renderstate[D3DRS_SPECULARMATERIALSOURCE]);

                if (This->StateBlock->renderstate[D3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_COLOR1) {
                    if (This->StateBlock->renderstate[D3DRS_AMBIENTMATERIALSOURCE] == D3DMCS_COLOR1) {
                        Parm = GL_AMBIENT_AND_DIFFUSE;
                    } else {
                        Parm = GL_DIFFUSE;
                    }
                } else if (This->StateBlock->renderstate[D3DRS_AMBIENTMATERIALSOURCE] == D3DMCS_COLOR1) {
                    Parm = GL_AMBIENT;
                } else if (This->StateBlock->renderstate[D3DRS_EMISSIVEMATERIALSOURCE] == D3DMCS_COLOR1) {
                    Parm = GL_EMISSION;
                } else if (This->StateBlock->renderstate[D3DRS_SPECULARMATERIALSOURCE] == D3DMCS_COLOR1) {
                    Parm = GL_SPECULAR;
                } else {
                    Parm = -1;
                }

                if (Parm == -1) {
                    if (This->tracking_color != DISABLED_TRACKING) This->tracking_color = NEEDS_DISABLE;
                } else {
                    This->tracking_color = NEEDS_TRACKING;
                    This->tracking_parm  = Parm;
                }

            } else {
                if (This->tracking_color != DISABLED_TRACKING) This->tracking_color = NEEDS_DISABLE;
            }
        }
        break; 

    case D3DRS_LINEPATTERN               :
        {
            union {
		DWORD 		d;
		D3DLINEPATTERN	lp;
	    } tmppattern;
	    tmppattern.d = Value;

            TRACE("Line pattern: repeat %d bits %x\n", tmppattern.lp.wRepeatFactor, tmppattern.lp.wLinePattern);

            if (tmppattern.lp.wRepeatFactor) {
                glLineStipple(tmppattern.lp.wRepeatFactor, tmppattern.lp.wLinePattern);
                checkGLcall("glLineStipple(repeat, linepattern)");
                glEnable(GL_LINE_STIPPLE);
                checkGLcall("glEnable(GL_LINE_STIPPLE);");
            } else {
                glDisable(GL_LINE_STIPPLE);
                checkGLcall("glDisable(GL_LINE_STIPPLE);");
            }
        }
        break;

    case D3DRS_ZBIAS                     :
        {
            if (Value) {
		tmpvalue.d = Value;
                TRACE("ZBias value %f\n", tmpvalue.f);
                glPolygonOffset(0, -tmpvalue.f);
                checkGLcall("glPolygonOffset(0, -Value)");
                glEnable(GL_POLYGON_OFFSET_FILL);
                checkGLcall("glEnable(GL_POLYGON_OFFSET_FILL);");
                glEnable(GL_POLYGON_OFFSET_LINE);
                checkGLcall("glEnable(GL_POLYGON_OFFSET_LINE);");
                glEnable(GL_POLYGON_OFFSET_POINT);
                checkGLcall("glEnable(GL_POLYGON_OFFSET_POINT);");
            } else {
                glDisable(GL_POLYGON_OFFSET_FILL);
                checkGLcall("glDisable(GL_POLYGON_OFFSET_FILL);");
                glDisable(GL_POLYGON_OFFSET_LINE);
                checkGLcall("glDisable(GL_POLYGON_OFFSET_LINE);");
                glDisable(GL_POLYGON_OFFSET_POINT);
                checkGLcall("glDisable(GL_POLYGON_OFFSET_POINT);");
            }
        }
        break;

    case D3DRS_NORMALIZENORMALS          :
        if (Value) {
            glEnable(GL_NORMALIZE);
            checkGLcall("glEnable(GL_NORMALIZE);");
        } else {
            glDisable(GL_NORMALIZE);
            checkGLcall("glDisable(GL_NORMALIZE);");
        }
        break;

    case D3DRS_POINTSIZE                 :
	tmpvalue.d = Value;
        TRACE("Set point size to %f\n", tmpvalue.f);
        glPointSize(tmpvalue.f);
        checkGLcall("glPointSize(...);");
        break;

    case D3DRS_POINTSIZE_MIN             :
        if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
	  tmpvalue.d = Value;
	  GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MIN_EXT, tmpvalue.f);
	  checkGLcall("glPointParameterfEXT(...);");
	} else {
	  FIXME("D3DRS_POINTSIZE_MIN not supported on this opengl\n");
	}
        break;

    case D3DRS_POINTSIZE_MAX             :
        if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
	  tmpvalue.d = Value;
	  GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MAX_EXT, tmpvalue.f);
	  checkGLcall("glPointParameterfEXT(...);");
	} else {
	  FIXME("D3DRS_POINTSIZE_MAX not supported on this opengl\n");
	}
        break;

    case D3DRS_POINTSCALE_A              :
    case D3DRS_POINTSCALE_B              :
    case D3DRS_POINTSCALE_C              :
    case D3DRS_POINTSCALEENABLE          :
    {
        /* 
         * POINTSCALEENABLE controls how point size value is treated. If set to
         * true, the point size is scaled with respect to height of viewport.
         * When set to false point size is in pixels.
         * 
         * http://msdn.microsoft.com/library/en-us/directx9_c/point_sprites.asp
         */

        /* Default values */
        GLfloat att[3] = {1.0f, 0.0f, 0.0f};
        
        /*
         * Minium valid point size for OpenGL is 1.0f. For Direct3D it is 0.0f.
         * This means that OpenGL will clamp really small point sizes to 1.0f.
         * To correct for this we need to multiply by the scale factor when sizes
         * are less than 1.0f. scale_factor =  1.0f / point_size.
         */
        GLfloat pointSize = *((float*)&This->StateBlock->renderstate[D3DRS_POINTSIZE]);
        GLfloat scaleFactor;
        if(pointSize < 1.0f) {
            scaleFactor = pointSize * pointSize;
        } else {
            scaleFactor = 1.0f;
        }
        
        if(This->StateBlock->renderstate[D3DRS_POINTSCALEENABLE]) {
            att[0] = *((float*)&This->StateBlock->renderstate[D3DRS_POINTSCALE_A]) /
                (This->StateBlock->viewport.Height * This->StateBlock->viewport.Height * scaleFactor);
            att[1] = *((float*)&This->StateBlock->renderstate[D3DRS_POINTSCALE_B]) /
                (This->StateBlock->viewport.Height * This->StateBlock->viewport.Height * scaleFactor);
            att[2] = *((float*)&This->StateBlock->renderstate[D3DRS_POINTSCALE_C]) /
                (This->StateBlock->viewport.Height * This->StateBlock->viewport.Height * scaleFactor);
        }

        if(GL_SUPPORT(ARB_POINT_PARAMETERS)) {
            GL_EXTCALL(glPointParameterfvARB)(GL_POINT_DISTANCE_ATTENUATION_ARB, att);
            checkGLcall("glPointParameterfvARB(GL_DISTANCE_ATTENUATION_ARB, ...");
        }
        else if(GL_SUPPORT(EXT_POINT_PARAMETERS)) {
            GL_EXTCALL(glPointParameterfvEXT)(GL_DISTANCE_ATTENUATION_EXT, att);
            checkGLcall("glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, ...");
        }
        else {
            TRACE("POINT_PARAMETERS not supported in this version of opengl\n");
        }
        break;
    }
    case D3DRS_COLORWRITEENABLE          :
      {
        TRACE("Color mask: r(%d) g(%d) b(%d) a(%d)\n", 
	      Value & D3DCOLORWRITEENABLE_RED   ? 1 : 0,
	      Value & D3DCOLORWRITEENABLE_GREEN ? 1 : 0,
	      Value & D3DCOLORWRITEENABLE_BLUE  ? 1 : 0,
	      Value & D3DCOLORWRITEENABLE_ALPHA ? 1 : 0); 
        glColorMask(Value & D3DCOLORWRITEENABLE_RED   ? GL_TRUE : GL_FALSE, 
                    Value & D3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
                    Value & D3DCOLORWRITEENABLE_BLUE  ? GL_TRUE : GL_FALSE, 
                    Value & D3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);
        checkGLcall("glColorMask(...)");
      }
      break;

    case D3DRS_LOCALVIEWER               :
      {
	GLint state = (Value) ? 1 : 0;
	TRACE("Local Viewer Enable to %ul\n", (BOOL) Value);	
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, state);
      }
      break;

    case D3DRS_LASTPIXEL                 :
      {
	if (Value) {
	  TRACE("Last Pixel Drawing Enabled\n");  
	} else {
	  FIXME("Last Pixel Drawing Disabled, not handled yet\n");  
	}
      }
      break;

    case D3DRS_SOFTWAREVERTEXPROCESSING  :
      {
	if (Value) {
	  TRACE("Software Processing Enabled\n");  
	} else {
	  TRACE("Software Processing Disabled\n");  
	}
      }
      break;

      /** not supported */
    case D3DRS_ZVISIBLE                  :
      {
	LEAVE_GL();
	return D3DERR_INVALIDCALL;
      }

        /* Unhandled yet...! */
    case D3DRS_EDGEANTIALIAS             :
    {
        if(Value) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            checkGLcall("glEnable GL_BLEND");
            glEnable(GL_LINE_SMOOTH);
            checkGLcall("glEnable Gl_LINE_SMOOTH");
        } else {
            glDisable(GL_BLEND);
            checkGLcall("glDisable GL_BLEND");
            glDisable(GL_LINE_SMOOTH);
            checkGLcall("glDisable GL_LINE_SMOOTH");
        }
        break;
    }
    case D3DRS_WRAP0                     :
    case D3DRS_WRAP1                     :
    case D3DRS_WRAP2                     :
    case D3DRS_WRAP3                     :
    case D3DRS_WRAP4                     :
    case D3DRS_WRAP5                     :
    case D3DRS_WRAP6                     :
    case D3DRS_WRAP7                     :
    {
        FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value);
        break;
    }
    case D3DRS_POINTSPRITEENABLE         :
    {
        if (!GL_SUPPORT(ARB_POINT_SPRITE)) {
            TRACE("Point sprites not supported\n");
            break;
        }

        /*
         * Point sprites are always enabled. Value controls texture coordinate
         * replacement mode. Must be set true for point sprites to use
         * textures.
         */
        glEnable(GL_POINT_SPRITE_ARB);
        checkGLcall("glEnable GL_POINT_SPRITE_ARB");

        if (Value) {
            glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, TRUE);
        } else {
            glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, FALSE);
        }
        break;
    }
    case D3DRS_MULTISAMPLEANTIALIAS      :
    {
        if (!GL_SUPPORT(ARB_MULTISAMPLE)) {
            TRACE("Multisample antialiasing not supported\n");
            break;
        }

        if(Value) {
            glEnable(GL_MULTISAMPLE_ARB);
            checkGLcall("glEnable GL_MULTISAMPLE_ARB");
        } else {
            glDisable(GL_MULTISAMPLE_ARB);
            checkGLcall("glDisable GL_MULTISAMPLE_ARB");
        }
        break;
    }
    case D3DRS_MULTISAMPLEMASK           :
    case D3DRS_PATCHEDGESTYLE            :
    case D3DRS_PATCHSEGMENTS             :
    case D3DRS_DEBUGMONITORTOKEN         :
    case D3DRS_POSITIONORDER             :
    case D3DRS_NORMALORDER               :
        /*Put back later: FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value); */
        FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value);
        break;
    default:
        FIXME("(%p)->(%d,%ld) unrecognized\n", This, State, Value);
    }

    LEAVE_GL();

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD* pValue) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) for State %d = %ld\n", This, State, This->UpdateStateBlock->renderstate[State]);
    *pValue = This->StateBlock->renderstate[State];
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_BeginStateBlock(LPDIRECT3DDEVICE8 iface) {
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
  
  TRACE("(%p)\n", This);
  
  return IDirect3DDeviceImpl_BeginStateBlock(This);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_EndStateBlock(LPDIRECT3DDEVICE8 iface, DWORD* pToken) {
  IDirect3DStateBlockImpl* pSB;
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
  HRESULT res;

  TRACE("(%p)\n", This);

  res = IDirect3DDeviceImpl_EndStateBlock(This, &pSB);
  *pToken = (DWORD) pSB;
  return res;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_ApplyStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
  IDirect3DStateBlockImpl* pSB = (IDirect3DStateBlockImpl*) Token;
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  TRACE("(%p)\n", This);

  return IDirect3DDeviceImpl_ApplyStateBlock(This, pSB);

}
HRESULT  WINAPI  IDirect3DDevice8Impl_CaptureStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
  IDirect3DStateBlockImpl* pSB = (IDirect3DStateBlockImpl*) Token;
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  TRACE("(%p)\n", This);

  return IDirect3DDeviceImpl_CaptureStateBlock(This, pSB);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
  IDirect3DStateBlockImpl* pSB = (IDirect3DStateBlockImpl*) Token;
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  TRACE("(%p)\n", This);

  return IDirect3DDeviceImpl_DeleteStateBlock(This, pSB);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_CreateStateBlock(LPDIRECT3DDEVICE8 iface, D3DSTATEBLOCKTYPE Type, DWORD* pToken) {
  IDirect3DStateBlockImpl* pSB;
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
  HRESULT res;

  TRACE("(%p) : for type %d\n", This, Type);

  res = IDirect3DDeviceImpl_CreateStateBlock(This, Type, &pSB);
  *pToken = (DWORD) pSB;
  return res;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipStatus(LPDIRECT3DDEVICE8 iface, CONST D3DCLIPSTATUS8* pClipStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);
    if (NULL == pClipStatus) {
      return D3DERR_INVALIDCALL;
    }
    This->UpdateStateBlock->clip_status.ClipUnion = pClipStatus->ClipUnion;
    This->UpdateStateBlock->clip_status.ClipIntersection = pClipStatus->ClipIntersection;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipStatus(LPDIRECT3DDEVICE8 iface, D3DCLIPSTATUS8* pClipStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    if (NULL == pClipStatus) {
      return D3DERR_INVALIDCALL;
    }
    pClipStatus->ClipUnion = This->UpdateStateBlock->clip_status.ClipUnion;
    pClipStatus->ClipIntersection = This->UpdateStateBlock->clip_status.ClipIntersection;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage,IDirect3DBaseTexture8** ppTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : returning %p for stage %ld\n", This, This->UpdateStateBlock->textures[Stage], Stage);
    *ppTexture = (LPDIRECT3DBASETEXTURE8) This->UpdateStateBlock->textures[Stage];
    if (*ppTexture)
        IDirect3DBaseTexture8Impl_AddRef(*ppTexture);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage, IDirect3DBaseTexture8* pTexture) {

    IDirect3DBaseTexture8 *oldTxt;
    BOOL reapplyStates = TRUE;
    INT oldTextureDimensions = -1;
    DWORD reapplyFlags = 0;

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    D3DRESOURCETYPE textureType;

    oldTxt = This->UpdateStateBlock->textures[Stage];
    TRACE("(%p) : Stage(%ld), Texture (%p)\n", This, Stage, pTexture);

    /* Reject invalid texture units */
    if (Stage >= GL_LIMITS(textures)) {
        TRACE("Attempt to access invalid texture rejected\n");
        return D3DERR_INVALIDCALL;
    }

    This->UpdateStateBlock->Set.textures[Stage] = TRUE;
    This->UpdateStateBlock->Changed.textures[Stage] = TRUE;
    This->UpdateStateBlock->textures[Stage] = pTexture;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    oldTextureDimensions = This->UpdateStateBlock->textureDimensions[Stage];
    ENTER_GL();

    /* Make appropriate texture active */
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
        glActiveTexture(GL_TEXTURE0 + Stage);
        checkGLcall("glActiveTexture");
#else
        glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
        checkGLcall("glActiveTextureARB");
#endif
    } else if (Stage>0) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
    }

    /* Decrement the count of the previous texture */
    if (NULL != oldTxt) {
        IDirect3DBaseTexture8Impl_Release(oldTxt);
    }

    if (NULL != pTexture) {
        IDirect3DBaseTexture8Impl_AddRef((LPDIRECT3DBASETEXTURE8) This->UpdateStateBlock->textures[Stage]);

        /* Now setup the texture appropraitly */
        textureType = IDirect3DBaseTexture8Impl_GetType(pTexture);
        if (textureType == D3DRTYPE_TEXTURE) {
          if (oldTxt == pTexture && IDirect3DBaseTexture8Impl_IsDirty(pTexture)) {
            TRACE("Skipping setting texture as old == new\n");
            reapplyStates = FALSE;
          } else {
            /* Standard 2D texture */
            TRACE("Standard 2d texture\n");
            This->UpdateStateBlock->textureDimensions[Stage] = GL_TEXTURE_2D;

            /* Load up the texture now */
            IDirect3DTexture8Impl_PreLoad((LPDIRECT3DTEXTURE8) pTexture);
          }
        } else if (textureType == D3DRTYPE_VOLUMETEXTURE) {
	  if (oldTxt == pTexture && IDirect3DBaseTexture8Impl_IsDirty(pTexture)) {
	    TRACE("Skipping setting texture as old == new\n");
	    reapplyStates = FALSE;
          } else {
	    /* Standard 3D (volume) texture */
            TRACE("Standard 3d texture\n");
            This->UpdateStateBlock->textureDimensions[Stage] = GL_TEXTURE_3D;

            /* Load up the texture now */
	    IDirect3DVolumeTexture8Impl_PreLoad((LPDIRECT3DVOLUMETEXTURE8) pTexture);
	  }
        } else if (textureType == D3DRTYPE_CUBETEXTURE) {
	  if (oldTxt == pTexture && IDirect3DBaseTexture8Impl_IsDirty(pTexture)) {
	    TRACE("Skipping setting texture as old == new\n");
	    reapplyStates = FALSE;
          } else {
	    /* Standard Cube texture */
	    TRACE("Standard Cube texture\n");
	    This->UpdateStateBlock->textureDimensions[Stage] = GL_TEXTURE_CUBE_MAP_ARB;

	    /* Load up the texture now */
	    IDirect3DCubeTexture8Impl_PreLoad((LPDIRECT3DCUBETEXTURE8) pTexture);
	  }
	} else {
            FIXME("(%p) : Incorrect type for a texture : (%d,%s)\n", This, textureType, debug_d3dressourcetype(textureType));
        }
    } else {
        TRACE("Setting to no texture (ie default texture)\n");
        This->UpdateStateBlock->textureDimensions[Stage] = GL_TEXTURE_1D;
        glBindTexture(GL_TEXTURE_1D, This->dummyTextureName[Stage]);
        checkGLcall("glBindTexture");
        TRACE("Bound dummy Texture to stage %ld (gl name %d)\n", Stage, This->dummyTextureName[Stage]);
    }

    /* Disable the old texture binding and enable the new one (unless operations are disabled) */
    if (oldTextureDimensions != This->UpdateStateBlock->textureDimensions[Stage]) {
       glDisable(oldTextureDimensions);
       checkGLcall("Disable oldTextureDimensions");
       if (This->StateBlock->texture_state[Stage][D3DTSS_COLOROP] != D3DTOP_DISABLE) {
          glEnable(This->UpdateStateBlock->textureDimensions[Stage]);
          checkGLcall("glEnable new texture dimensions");
       }

       /* If Alpha arg1 is texture then handle the special case when there changes between a
          texture and no texture - See comments in set_tex_op                                  */
       if ((This->StateBlock->texture_state[Stage][D3DTSS_ALPHAARG1] == D3DTA_TEXTURE) && 
           (((oldTxt == NULL) && (pTexture != NULL)) || ((pTexture == NULL) && (oldTxt != NULL))))
       {
           reapplyFlags |= REAPPLY_ALPHAOP;
       }
    }


    /* Even if the texture has been set to null, reapply the stages as a null texture to directx requires
       a dummy texture in opengl, and we always need to ensure the current view of the TextureStates apply */
    if (reapplyStates) {
       setupTextureStates(iface, Stage, reapplyFlags);
    }

    LEAVE_GL();   

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : requesting Stage %ld, Type %d getting %ld\n", This, Stage, Type, This->UpdateStateBlock->texture_state[Stage][Type]);
    *pValue = This->UpdateStateBlock->texture_state[Stage][Type];
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    union {
	float f;
	DWORD d;
    } tmpvalue;

    /* FIXME: Handle 3d textures? What if TSS value set before set texture? Need to reapply all values? */
   
    TRACE("(%p) : Stage=%ld, Type=%s(%d), Value=%ld\n", This, Stage, debug_d3dtexturestate(Type), Type, Value);

    /* Reject invalid texture units */
    if (Stage >= GL_LIMITS(textures)) {
        TRACE("Attempt to access invalid texture rejected\n");
        return D3DERR_INVALIDCALL;
    }

    This->UpdateStateBlock->Changed.texture_state[Stage][Type] = TRUE;
    This->UpdateStateBlock->Set.texture_state[Stage][Type] = TRUE;
    This->UpdateStateBlock->texture_state[Stage][Type] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    ENTER_GL();

    /* Make appropriate texture active */
    VTRACE(("Activating appropriate texture state %ld\n", Stage));
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
        glActiveTexture(GL_TEXTURE0 + Stage);
        vcheckGLcall("glActiveTexture");
#else
        glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
        vcheckGLcall("glActiveTextureARB");
#endif
    } else if (Stage > 0) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesn't support\n");
    }

    switch (Type) {

    case D3DTSS_MINFILTER             :
    case D3DTSS_MIPFILTER             :
        {
            DWORD ValueMIN = This->StateBlock->texture_state[Stage][D3DTSS_MINFILTER];
            DWORD ValueMIP = This->StateBlock->texture_state[Stage][D3DTSS_MIPFILTER];
            GLint realVal = GL_LINEAR;

            if (ValueMIN == D3DTEXF_NONE) {
	      /* Doesn't really make sense - Windows just seems to disable
		 mipmapping when this occurs                              */
	      FIXME("Odd - minfilter of none, just disabling mipmaps\n");
	      realVal = GL_LINEAR;
	    } else if (ValueMIN == D3DTEXF_POINT) {
                /* GL_NEAREST_* */
	      if (ValueMIP == D3DTEXF_NONE) {
                    realVal = GL_NEAREST;
                } else if (ValueMIP == D3DTEXF_POINT) {
                    realVal = GL_NEAREST_MIPMAP_NEAREST;
                } else if (ValueMIP == D3DTEXF_LINEAR) {
                    realVal = GL_NEAREST_MIPMAP_LINEAR;
                } else {
                    FIXME("Unhandled D3DTSS_MIPFILTER value of %ld\n", ValueMIP);
                    realVal = GL_NEAREST;
                }
            } else if (ValueMIN == D3DTEXF_LINEAR) {
                /* GL_LINEAR_* */
                if (ValueMIP == D3DTEXF_NONE) {
                    realVal = GL_LINEAR;
                } else if (ValueMIP == D3DTEXF_POINT) {
                    realVal = GL_LINEAR_MIPMAP_NEAREST;
                } else if (ValueMIP == D3DTEXF_LINEAR) {
                    realVal = GL_LINEAR_MIPMAP_LINEAR;
                } else {
                    FIXME("Unhandled D3DTSS_MIPFILTER value of %ld\n", ValueMIP);
                    realVal = GL_LINEAR;
                }
	    } else if (ValueMIN == D3DTEXF_ANISOTROPIC) {
	      if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
		if (ValueMIP == D3DTEXF_NONE) {
		  realVal = GL_LINEAR_MIPMAP_LINEAR;		  
                } else if (ValueMIP == D3DTEXF_POINT) {
		  realVal = GL_LINEAR_MIPMAP_NEAREST;
                } else if (ValueMIP == D3DTEXF_LINEAR) {
                    realVal = GL_LINEAR_MIPMAP_LINEAR;
                } else {
		  FIXME("Unhandled D3DTSS_MIPFILTER value of %ld\n", ValueMIP);
		  realVal = GL_LINEAR;
                }
	      } else {
		WARN("Trying to use ANISOTROPIC_FILTERING for D3DTSS_MINFILTER. But not supported by OpenGL driver\n");
		realVal = GL_LINEAR;
	      }
            } else {
                FIXME("Unhandled D3DTSS_MINFILTER value of %ld\n", ValueMIN);
                realVal = GL_LINEAR_MIPMAP_LINEAR;
            }

            TRACE("ValueMIN=%ld, ValueMIP=%ld, setting MINFILTER to %x\n", ValueMIN, ValueMIP, realVal);
            glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_MIN_FILTER, realVal);
            checkGLcall("glTexParameter GL_TEXTURE_MIN_FILTER, ...");
	    /**
	     * if we juste choose to use ANISOTROPIC filtering, refresh openGL state
	     */
	    if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC) && D3DTEXF_ANISOTROPIC == ValueMIN) {
	      glTexParameteri(This->StateBlock->textureDimensions[Stage], 
			      GL_TEXTURE_MAX_ANISOTROPY_EXT, 
			      This->StateBlock->texture_state[Stage][D3DTSS_MAXANISOTROPY]);
	      checkGLcall("glTexParameter GL_TEXTURE_MAX_ANISOTROPY_EXT, ...");
	    }
        }
        break;

    case D3DTSS_MAGFILTER             :
      {
	DWORD ValueMAG = This->StateBlock->texture_state[Stage][D3DTSS_MAGFILTER];
	GLint realVal = GL_NEAREST;

        if (ValueMAG == D3DTEXF_POINT) {
	  realVal = GL_NEAREST;
        } else if (ValueMAG == D3DTEXF_LINEAR) {
	  realVal = GL_LINEAR;
	} else if (ValueMAG == D3DTEXF_ANISOTROPIC) {
	  if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
	    realVal = GL_LINEAR;
	  } else {
	    FIXME("Trying to use ANISOTROPIC_FILTERING for D3DTSS_MAGFILTER. But not supported by current OpenGL driver\n");
	    realVal = GL_NEAREST;
	  }
	} else {
	  FIXME("Unhandled D3DTSS_MAGFILTER value of %ld\n", ValueMAG);
	  realVal = GL_NEAREST;
        }
	TRACE("ValueMAG=%ld setting MAGFILTER to %x\n", ValueMAG, realVal);
	glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_MAG_FILTER, realVal);
	checkGLcall("glTexParameter GL_TEXTURE_MAG_FILTER, ...");
	/**
	 * if we juste choose to use ANISOTROPIC filtering, refresh openGL state
	 */
	if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC) && D3DTEXF_ANISOTROPIC == ValueMAG) {
	  glTexParameteri(This->StateBlock->textureDimensions[Stage], 
			  GL_TEXTURE_MAX_ANISOTROPY_EXT, 
			  This->StateBlock->texture_state[Stage][D3DTSS_MAXANISOTROPY]);
	  checkGLcall("glTexParameter GL_TEXTURE_MAX_ANISOTROPY_EXT, ...");
	}
      }
      break;

    case D3DTSS_MAXMIPLEVEL           :
      {
	/**
	 * Not really the same, but the more apprioprate than nothing
	 */
	glTexParameteri(This->StateBlock->textureDimensions[Stage], 
			GL_TEXTURE_BASE_LEVEL, 
			This->StateBlock->texture_state[Stage][D3DTSS_MAXMIPLEVEL]);
	checkGLcall("glTexParameteri GL_TEXTURE_BASE_LEVEL ...");
      }
      break;

    case D3DTSS_MAXANISOTROPY         :
      {	
	if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
	  glTexParameteri(This->StateBlock->textureDimensions[Stage], 
			  GL_TEXTURE_MAX_ANISOTROPY_EXT, 
			  This->StateBlock->texture_state[Stage][D3DTSS_MAXANISOTROPY]);
	  checkGLcall("glTexParameteri GL_TEXTURE_MAX_ANISOTROPY_EXT ...");
	}
      }
      break;

    case D3DTSS_MIPMAPLODBIAS         :
      {	
	if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
	  tmpvalue.d = Value;
	  glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, 
		    GL_TEXTURE_LOD_BIAS_EXT,
		    tmpvalue.f);
	  checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
	}
      }
      break;

    case D3DTSS_ALPHAOP               :
    case D3DTSS_COLOROP               :
        {

            if ((Value == D3DTOP_DISABLE) && (Type == D3DTSS_COLOROP)) {
                /* TODO: Disable by making this and all later levels disabled */
                glDisable(GL_TEXTURE_1D);
                checkGLcall("Disable GL_TEXTURE_1D");
                glDisable(GL_TEXTURE_2D);
                checkGLcall("Disable GL_TEXTURE_2D");
                glDisable(GL_TEXTURE_3D);
                checkGLcall("Disable GL_TEXTURE_3D");
                break; /* Don't bother setting the texture operations */
            } else {
                /* Enable only the appropriate texture dimension */
                if (Type == D3DTSS_COLOROP) {
                    if (This->StateBlock->textureDimensions[Stage] == GL_TEXTURE_1D) {
                        glEnable(GL_TEXTURE_1D);
                        checkGLcall("Enable GL_TEXTURE_1D");
                    } else {
                        glDisable(GL_TEXTURE_1D);
                        checkGLcall("Disable GL_TEXTURE_1D");
                    } 
                    if (This->StateBlock->textureDimensions[Stage] == GL_TEXTURE_2D) {
		      if (GL_SUPPORT(NV_TEXTURE_SHADER) && This->texture_shader_active) {
			glTexEnvi(GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV, GL_TEXTURE_2D);
			checkGLcall("Enable GL_TEXTURE_2D");
		      } else {
                        glEnable(GL_TEXTURE_2D);
                        checkGLcall("Enable GL_TEXTURE_2D");
		      }
                    } else {
                        glDisable(GL_TEXTURE_2D);
                        checkGLcall("Disable GL_TEXTURE_2D");
                    }
                    if (This->StateBlock->textureDimensions[Stage] == GL_TEXTURE_3D) {
                        glEnable(GL_TEXTURE_3D);
                        checkGLcall("Enable GL_TEXTURE_3D");
                    } else {
                        glDisable(GL_TEXTURE_3D);
                        checkGLcall("Disable GL_TEXTURE_3D");
                    }
                    if (This->StateBlock->textureDimensions[Stage] == GL_TEXTURE_CUBE_MAP_ARB) {
                        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
                        checkGLcall("Enable GL_TEXTURE_CUBE_MAP");
                    } else {
                        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                        checkGLcall("Disable GL_TEXTURE_CUBE_MAP");
                    }
                }
            }
            /* Drop through... (Except disable case) */
        case D3DTSS_COLORARG0             :
        case D3DTSS_COLORARG1             :
        case D3DTSS_COLORARG2             :
        case D3DTSS_ALPHAARG0             :
        case D3DTSS_ALPHAARG1             :
        case D3DTSS_ALPHAARG2             :
            {
                BOOL isAlphaArg = (Type == D3DTSS_ALPHAOP || Type == D3DTSS_ALPHAARG1 || 
                                   Type == D3DTSS_ALPHAARG2 || Type == D3DTSS_ALPHAARG0);
                if (isAlphaArg) {
                    set_tex_op(iface, TRUE, Stage, This->StateBlock->texture_state[Stage][D3DTSS_ALPHAOP],
                               This->StateBlock->texture_state[Stage][D3DTSS_ALPHAARG1], 
                               This->StateBlock->texture_state[Stage][D3DTSS_ALPHAARG2], 
                               This->StateBlock->texture_state[Stage][D3DTSS_ALPHAARG0]);
                } else {
                    set_tex_op(iface, FALSE, Stage, This->StateBlock->texture_state[Stage][D3DTSS_COLOROP],
                               This->StateBlock->texture_state[Stage][D3DTSS_COLORARG1], 
                               This->StateBlock->texture_state[Stage][D3DTSS_COLORARG2], 
                               This->StateBlock->texture_state[Stage][D3DTSS_COLORARG0]);
                }
            }
            break;
        }

    case D3DTSS_ADDRESSU              :
    case D3DTSS_ADDRESSV              :
    case D3DTSS_ADDRESSW              :
        {
            GLint wrapParm = GL_REPEAT;

            switch (Value) {
            case D3DTADDRESS_WRAP:   wrapParm = GL_REPEAT; break;
            case D3DTADDRESS_CLAMP:  wrapParm = GL_CLAMP_TO_EDGE; break;      
            case D3DTADDRESS_BORDER: 
	      {
		if (GL_SUPPORT(ARB_TEXTURE_BORDER_CLAMP)) {
		  wrapParm = GL_CLAMP_TO_BORDER_ARB; 
		} else {
		  /* FIXME: Not right, but better */
		  FIXME("Unrecognized or unsupported D3DTADDRESS_* value %ld, state %d\n", Value, Type);
		  wrapParm = GL_REPEAT; 
		}
	      }
	      break;
            case D3DTADDRESS_MIRROR: 
	      {
		if (GL_SUPPORT(ARB_TEXTURE_MIRRORED_REPEAT)) {
		  wrapParm = GL_MIRRORED_REPEAT_ARB;
		} else {
		  /* Unsupported in OpenGL pre-1.4 */
		  FIXME("Unsupported D3DTADDRESS_MIRROR (needs GL_ARB_texture_mirrored_repeat) state %d\n", Type);
		  wrapParm = GL_REPEAT;
		}
	      }
	      break;
            case D3DTADDRESS_MIRRORONCE: 
	      {
		if (GL_SUPPORT(ATI_TEXTURE_MIRROR_ONCE)) {
		  wrapParm = GL_MIRROR_CLAMP_TO_EDGE_ATI;
		} else {
		  FIXME("Unsupported D3DTADDRESS_MIRRORONCE (needs GL_ATI_texture_mirror_once) state %d\n", Type);
		  wrapParm = GL_REPEAT; 
		}
	      }
	      break;

            default:
                FIXME("Unrecognized or unsupported D3DTADDRESS_* value %ld, state %d\n", Value, Type);
                wrapParm = GL_REPEAT; 
            }

            switch (Type) {
            case D3DTSS_ADDRESSU:
                TRACE("Setting WRAP_S to %d for %x\n", wrapParm, This->StateBlock->textureDimensions[Stage]);
                glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_WRAP_S, wrapParm);
                checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_S, wrapParm)");
                break;
            case D3DTSS_ADDRESSV:
                TRACE("Setting WRAP_T to %d for %x\n", wrapParm, This->StateBlock->textureDimensions[Stage]);
                glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_WRAP_T, wrapParm);
                checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_T, wrapParm)");
                break;
            case D3DTSS_ADDRESSW:
                TRACE("Setting WRAP_R to %d for %x\n", wrapParm, This->StateBlock->textureDimensions[Stage]);
                glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_WRAP_R, wrapParm);
                checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_R, wrapParm)");
                break;
            default: /* nop */
      	        break; /** stupic compilator */
            }
        }
        break;

    case D3DTSS_BORDERCOLOR           :
        {
            float col[4];
	    D3DCOLORTOGLFLOAT4(Value, col);
            TRACE("Setting border color for %x to %lx\n", This->StateBlock->textureDimensions[Stage], Value); 
            glTexParameterfv(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_BORDER_COLOR, &col[0]);
            checkGLcall("glTexParameteri(..., GL_TEXTURE_BORDER_COLOR, ...)");
        }
        break;

    case D3DTSS_TEXCOORDINDEX         :
        {
            /* Values 0-7 are indexes into the FVF tex coords - See comments in DrawPrimitive */

            /* FIXME: From MSDN: The D3DTSS_TCI_* flags are mutually exclusive. If you include 
                  one flag, you can still specify an index value, which the system uses to 
                  determine the texture wrapping mode.  
                  eg. SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION | 1 );
                  means use the vertex position (camera-space) as the input texture coordinates 
                  for this texture stage, and the wrap mode set in the D3DRS_WRAP1 render 
                  state. We do not (yet) support the D3DRENDERSTATE_WRAPx values, nor tie them up
                  to the TEXCOORDINDEX value */
          
            /** 
             * Be careful the value of the mask 0xF0000 come from d3d8types.h infos 
             */
            switch (Value & 0xFFFF0000) {
            case D3DTSS_TCI_PASSTHRU:
              /*Use the specified texture coordinates contained within the vertex format. This value resolves to zero.*/
	      glDisable(GL_TEXTURE_GEN_S);
	      glDisable(GL_TEXTURE_GEN_T);
	      glDisable(GL_TEXTURE_GEN_R);
              checkGLcall("glDisable(GL_TEXTURE_GEN_S,T,R)");
              break;

            case D3DTSS_TCI_CAMERASPACEPOSITION:
              /* CameraSpacePosition means use the vertex position, transformed to camera space, 
                 as the input texture coordinates for this stage's texture transformation. This 
                 equates roughly to EYE_LINEAR                                                  */
              {
                float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
                float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
                float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
                float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
                TRACE("D3DTSS_TCI_CAMERASPACEPOSITION - Set eye plane\n");

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadIdentity();
                glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
                glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
                glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
                glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
                glPopMatrix();
                
                TRACE("D3DTSS_TCI_CAMERASPACEPOSITION - Set GL_TEXTURE_GEN_x and GL_x, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR\n");
                glEnable(GL_TEXTURE_GEN_S);
                checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
                glEnable(GL_TEXTURE_GEN_T);
                checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
                glEnable(GL_TEXTURE_GEN_R);
                checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
                glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)");
              }
              break;

	    case D3DTSS_TCI_CAMERASPACENORMAL:
	      {
		if (GL_SUPPORT(NV_TEXGEN_REFLECTION)) {
		  float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
		  float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
		  float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
		  float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
		  TRACE("D3DTSS_TCI_CAMERASPACEPOSITION - Set eye plane\n");

		  glMatrixMode(GL_MODELVIEW);
		  glPushMatrix();
		  glLoadIdentity();
		  glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
		  glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
		  glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
		  glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
		  glPopMatrix();
		  
		  glEnable(GL_TEXTURE_GEN_S);
		  checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
		  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
		  checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
		  glEnable(GL_TEXTURE_GEN_T);
		  checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
		  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
		  checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
		  glEnable(GL_TEXTURE_GEN_R);
		  checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
		  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV);
		  checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_NV)");
		}
	      }
	      break;

	    case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
	      {
		if (GL_SUPPORT(NV_TEXGEN_REFLECTION)) {
		  float s_plane[] = { 1.0, 0.0, 0.0, 0.0 };
		  float t_plane[] = { 0.0, 1.0, 0.0, 0.0 };
		  float r_plane[] = { 0.0, 0.0, 1.0, 0.0 };
		  float q_plane[] = { 0.0, 0.0, 0.0, 1.0 };
		  TRACE("D3DTSS_TCI_CAMERASPACEPOSITION - Set eye plane\n");
		  
		  glMatrixMode(GL_MODELVIEW);
		  glPushMatrix();
		  glLoadIdentity();
		  glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
		  glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
		  glTexGenfv(GL_R, GL_EYE_PLANE, r_plane);
		  glTexGenfv(GL_Q, GL_EYE_PLANE, q_plane);
		  glPopMatrix();
		  
		  glEnable(GL_TEXTURE_GEN_S);
		  checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
		  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
		  checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
		  glEnable(GL_TEXTURE_GEN_T);
		  checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
		  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
		  checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
		  glEnable(GL_TEXTURE_GEN_R);
		  checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
		  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV);
		  checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_NV)");
		}
	      }
	      break;

            /* Unhandled types: */
            default:
                /* Todo: */
                /* ? disable GL_TEXTURE_GEN_n ? */ 
	        glDisable(GL_TEXTURE_GEN_S);
	        glDisable(GL_TEXTURE_GEN_T);
	        glDisable(GL_TEXTURE_GEN_R);
                FIXME("Unhandled D3DTSS_TEXCOORDINDEX %lx\n", Value);
                break;
            }
        }
        break;

        /* Unhandled */
    case D3DTSS_TEXTURETRANSFORMFLAGS :
        set_texture_matrix((float *)&This->StateBlock->transforms[D3DTS_TEXTURE0 + Stage].u.m[0][0], Value);
        break; 

    case D3DTSS_BUMPENVMAT00          :
    case D3DTSS_BUMPENVMAT01          :
        TRACE("BUMPENVMAT0%u Stage=%ld, Type=%d, Value =%ld\n", Type - D3DTSS_BUMPENVMAT00, Stage, Type, Value);
        break;
    case D3DTSS_BUMPENVMAT10          :
    case D3DTSS_BUMPENVMAT11          :
        TRACE("BUMPENVMAT1%u Stage=%ld, Type=%d, Value =%ld\n", Type - D3DTSS_BUMPENVMAT10, Stage, Type, Value);
        break;

    case D3DTSS_BUMPENVLSCALE         :
      TRACE("BUMPENVLSCALE Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
      break;

    case D3DTSS_BUMPENVLOFFSET        :
      TRACE("BUMPENVLOFFSET Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
      break;

    case D3DTSS_RESULTARG             :
      TRACE("RESULTARG Still a stub, Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
      break;

    default:
        /* Put back later: FIXME("(%p) : stub, Stage=%ld, Type=%d, Value =%ld\n", This, Stage, Type, Value); */
        TRACE("Still a stub, Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
    }

    LEAVE_GL();

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_ValidateDevice(LPDIRECT3DDEVICE8 iface, DWORD* pNumPasses) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : stub\n", This);    /* FIXME: Needs doing, but called often and is harmless */
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetInfo(LPDIRECT3DDEVICE8 iface, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : setting p[%u] <= RGBA(%02x,%02x,%02x,%02x)\n", This, PaletteNumber,
          pEntries->peRed, pEntries->peGreen, pEntries->peBlue, pEntries->peFlags);
    if (PaletteNumber >= MAX_PALETTES) {
        return D3DERR_INVALIDCALL;
    }
    memcpy(This->palettes[PaletteNumber], pEntries, 256 * sizeof(PALETTEENTRY));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    if (PaletteNumber >= MAX_PALETTES) {
        return D3DERR_INVALIDCALL;
    }
    memcpy(pEntries, This->palettes[PaletteNumber], 256 * sizeof(PALETTEENTRY));
    FIXME("(%p) : returning p[%u] => RGBA(%02x,%02x,%02x,%02x)\n", This, PaletteNumber,
          pEntries->peRed, pEntries->peGreen, pEntries->peBlue, pEntries->peFlags);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : Setting to (%u)\n", This, PaletteNumber);
    if (PaletteNumber >= MAX_PALETTES) {
        return D3DERR_INVALIDCALL;
    }
    This->currentPalette = PaletteNumber;

#if defined(GL_EXT_paletted_texture)
    if (GL_SUPPORT(EXT_PALETTED_TEXTURE)) {

      ENTER_GL();

      GL_EXTCALL(glColorTableEXT)(GL_TEXTURE_2D,    /* target */
				  GL_RGBA,          /* internal format */
				  256,              /* table size */
				  GL_RGBA,          /* table format */
				  GL_UNSIGNED_BYTE, /* table type */
				  This->palettes[PaletteNumber]);
      checkGLcall("glColorTableEXT");

      LEAVE_GL();

    } else {
      /* Delayed palette handling ... waiting for software emulation into preload code */
    }
#endif
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT *PaletteNumber) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    *PaletteNumber = This->currentPalette;
    FIXME("(%p) : Returning (%u)\n", This, *PaletteNumber);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    This->StateBlock->streamIsUP = FALSE;

    TRACE("(%p) : Type=(%d,%s), Start=%d, Count=%d\n", This, PrimitiveType, debug_d3dprimitivetype(PrimitiveType), StartVertex, PrimitiveCount);
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, StartVertex, -1, 0, NULL, 0);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,
                                                           UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount) {
    UINT idxStride = 2;
    IDirect3DIndexBuffer8      *pIB;
    D3DINDEXBUFFER_DESC         IdxBufDsc;

    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    pIB = This->StateBlock->pIndexData;
    This->StateBlock->streamIsUP = FALSE;

    TRACE("(%p) : Type=(%d,%s), min=%d, CountV=%d, startIdx=%d, countP=%d\n", This, 
          PrimitiveType, debug_d3dprimitivetype(PrimitiveType),
          minIndex, NumVertices, startIndex, primCount);

    IDirect3DIndexBuffer8Impl_GetDesc(pIB, &IdxBufDsc);
    if (IdxBufDsc.Format == D3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    drawPrimitive(iface, PrimitiveType, primCount, This->StateBlock->baseVertexIndex, startIndex, idxStride, ((IDirect3DIndexBuffer8Impl *) pIB)->allocatedMemory,
                   minIndex);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) : Type=(%d,%s), pCount=%d, pVtxData=%p, Stride=%d\n", This, PrimitiveType, debug_d3dprimitivetype(PrimitiveType), 
          PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);

    if (This->StateBlock->stream_source[0] != NULL) IDirect3DVertexBuffer8Impl_Release(This->StateBlock->stream_source[0]);

    /* Note in the following, its not this type, but thats the purpose of streamIsUP */
    This->StateBlock->stream_source[0] = (IDirect3DVertexBuffer8 *)pVertexStreamZeroData; 
    This->StateBlock->stream_stride[0] = VertexStreamZeroStride;
    This->StateBlock->streamIsUP = TRUE;
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0, 0, 0, NULL, 0);
    This->StateBlock->stream_stride[0] = 0;
    This->StateBlock->stream_source[0] = NULL;

    /*stream zero settings set to null at end */
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,
                                                             UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,
                                                             D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,
                                                             UINT VertexStreamZeroStride) {
    int idxStride;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : Type=(%d,%s), MinVtxIdx=%d, NumVIdx=%d, PCount=%d, pidxdata=%p, IdxFmt=%d, pVtxdata=%p, stride=%d\n", This, PrimitiveType, debug_d3dprimitivetype(PrimitiveType),
          MinVertexIndex, NumVertexIndices, PrimitiveCount, pIndexData,  IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);

    if (This->StateBlock->stream_source[0] != NULL) IDirect3DVertexBuffer8Impl_Release(This->StateBlock->stream_source[0]);
    if (IndexDataFormat == D3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    /* Note in the following, its not this type, but thats the purpose of streamIsUP */
    This->StateBlock->stream_source[0] = (IDirect3DVertexBuffer8 *)pVertexStreamZeroData;
    This->StateBlock->streamIsUP = TRUE;
    This->StateBlock->stream_stride[0] = VertexStreamZeroStride;
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, This->StateBlock->baseVertexIndex, 0, idxStride, pIndexData, MinVertexIndex);

    /*stream zero settings set to null at end */
    This->StateBlock->stream_source[0] = NULL;
    This->StateBlock->stream_stride[0] = 0;
    IDirect3DDevice8Impl_SetIndices(iface, NULL, 0);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_ProcessVertices(LPDIRECT3DDEVICE8 iface, UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer8* pDestBuffer,DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DVertexShaderImpl* object;
    IDirect3DVertexShaderDeclarationImpl* attached_decl;
    HRESULT res;
    UINT i;

    TRACE_(d3d_shader)("(%p) : VertexShader not fully supported yet : Decl=%p, Func=%p, Usage=%lu\n", This, pDeclaration, pFunction, Usage);
    if (NULL == pDeclaration || NULL == pHandle) { /* pFunction can be NULL see MSDN */
      return D3DERR_INVALIDCALL;
    }
    for (i = 1; NULL != VertexShaders[i] && i < sizeof(VertexShaders) / sizeof(IDirect3DVertexShaderImpl*); ++i) ;
    if (i >= sizeof(VertexShaders) / sizeof(IDirect3DVertexShaderImpl*)) {
      return D3DERR_OUTOFVIDEOMEMORY;
    }

    /** Create the Vertex Shader */
    res = IDirect3DDeviceImpl_CreateVertexShader(This, pFunction, Usage, &object);
    /** TODO: check FAILED(res) */

    /** Create and Bind the Vertex Shader Declaration */
    res = IDirect3DDeviceImpl_CreateVertexShaderDeclaration8(This, pDeclaration, &attached_decl);
    /** TODO: check FAILED(res) */

    VertexShaders[i] = object;
    VertexShaderDeclarations[i] = attached_decl;
    *pHandle = VS_HIGHESTFIXEDFXF + i;
    TRACE("Finished creating vertex shader %lx\n", *pHandle);

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    This->UpdateStateBlock->VertexShader = Handle;
    This->UpdateStateBlock->Changed.vertexShader = TRUE;
    This->UpdateStateBlock->Set.vertexShader = TRUE;
    
    if (Handle > VS_HIGHESTFIXEDFXF) { /* only valid with non FVF shaders */
      TRACE_(d3d_shader)("(%p) : Created shader, Handle=%lx\n", This, Handle);
      This->UpdateStateBlock->vertexShaderDecl = VERTEX_SHADER_DECL(Handle);
      This->UpdateStateBlock->Changed.vertexShaderDecl = TRUE;
      This->UpdateStateBlock->Set.vertexShaderDecl = TRUE;
    } else { /* use a fvf, so desactivate the vshader decl */
      TRACE("(%p) : FVF Shader, Handle=%lx\n", This, Handle);
      This->UpdateStateBlock->vertexShaderDecl = NULL;
      This->UpdateStateBlock->Changed.vertexShaderDecl = TRUE;
      This->UpdateStateBlock->Set.vertexShaderDecl = TRUE;
    }
    /* Handle recording of state blocks */
    if (This->isRecordingState) {
      TRACE("Recording... not performing anything\n");
      return D3D_OK;
    }
    /**
     * TODO: merge HAL shaders context switching from prototype
     */
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD* pHandle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE_(d3d_shader)("(%p) : GetVertexShader returning %ld\n", This, This->StateBlock->VertexShader);
    *pHandle = This->StateBlock->VertexShader;
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteVertexShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DVertexShaderImpl* object; 
    IDirect3DVertexShaderDeclarationImpl* attached_decl;

    if (Handle <= VS_HIGHESTFIXEDFXF) { /* only delete user defined shaders */
      return D3DERR_INVALIDCALL;
    }

    /**
     * Delete Vertex Shader
     */
    object = VertexShaders[Handle - VS_HIGHESTFIXEDFXF];
    if (NULL == object) {
      return D3DERR_INVALIDCALL;
    }
    TRACE_(d3d_shader)("(%p) : freing VertexShader %p\n", This, object);
    /* TODO: check validity of object */
    HeapFree(GetProcessHeap(), 0, (void *)object->function);
    if (object->prgId != 0) {
        GL_EXTCALL(glDeleteProgramsARB( 1, &object->prgId ));
    }
    HeapFree(GetProcessHeap(), 0, (void *)object->data);
    HeapFree(GetProcessHeap(), 0, (void *)object);
    VertexShaders[Handle - VS_HIGHESTFIXEDFXF] = NULL;

    /**
     * Delete Vertex Shader Declaration
     */
    attached_decl = VertexShaderDeclarations[Handle - VS_HIGHESTFIXEDFXF];
    if (NULL == attached_decl) {
      return D3DERR_INVALIDCALL;
    } 
    TRACE_(d3d_shader)("(%p) : freing VertexShaderDeclaration %p\n", This, attached_decl);
    /* TODO: check validity of object */
    HeapFree(GetProcessHeap(), 0, (void *)attached_decl->pDeclaration8);
    HeapFree(GetProcessHeap(), 0, (void *)attached_decl);
    VertexShaderDeclarations[Handle - VS_HIGHESTFIXEDFXF] = NULL;

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  if (Register + ConstantCount > D3D8_VSHADER_MAX_CONSTANTS) {
    ERR_(d3d_shader)("(%p) : SetVertexShaderConstant C[%lu] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (ConstantCount > 1) {
    const FLOAT* f = (const FLOAT*)pConstantData;
    UINT i;
    TRACE_(d3d_shader)("(%p) : SetVertexShaderConstant C[%lu..%lu]=\n", This, Register, Register + ConstantCount - 1);
    for (i = 0; i < ConstantCount; ++i) {
      TRACE_(d3d_shader)("{%f, %f, %f, %f}\n", f[0], f[1], f[2], f[3]);
      f += 4;
    }
  } else { 
    const FLOAT* f = (const FLOAT*) pConstantData;
    TRACE_(d3d_shader)("(%p) : SetVertexShaderConstant, C[%lu]={%f, %f, %f, %f}\n", This, Register, f[0], f[1], f[2], f[3]);
  }
  This->UpdateStateBlock->Changed.vertexShaderConstant = TRUE;
  memcpy(&This->UpdateStateBlock->vertexShaderConstant[Register], pConstantData, ConstantCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  TRACE_(d3d_shader)("(%p) : C[%lu] count=%ld\n", This, Register, ConstantCount);
  if (Register + ConstantCount > D3D8_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  memcpy(pConstantData, &This->UpdateStateBlock->vertexShaderConstant[Register], ConstantCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderDeclaration(LPDIRECT3DDEVICE8 iface, DWORD Handle, void* pData, DWORD* pSizeOfData) {
  /*IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;*/
  IDirect3DVertexShaderDeclarationImpl* attached_decl;
  
  attached_decl = VERTEX_SHADER_DECL(Handle);
  if (NULL == attached_decl) {
    return D3DERR_INVALIDCALL;
  }
  return IDirect3DVertexShaderDeclarationImpl_GetDeclaration8(attached_decl, pData, (UINT*) pSizeOfData);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD Handle, void* pData, DWORD* pSizeOfData) {
  /*IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;*/
  IDirect3DVertexShaderImpl* object;
  
  object = VERTEX_SHADER(Handle);
  if (NULL == object) {
    return D3DERR_INVALIDCALL;
  }
  return IDirect3DVertexShaderImpl_GetFunction(object, pData, (UINT*) pSizeOfData);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DIndexBuffer8 *oldIdxs;

    TRACE("(%p) : Setting to %p, base %d\n", This, pIndexData, BaseVertexIndex);
    oldIdxs = This->StateBlock->pIndexData;

    This->UpdateStateBlock->Changed.Indices = TRUE;
    This->UpdateStateBlock->Set.Indices = TRUE;
    This->UpdateStateBlock->pIndexData = pIndexData;
    This->UpdateStateBlock->baseVertexIndex = BaseVertexIndex;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    if (pIndexData) IDirect3DIndexBuffer8Impl_AddRefInt(This->StateBlock->pIndexData);
    if (oldIdxs) IDirect3DIndexBuffer8Impl_ReleaseInt(oldIdxs);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8** ppIndexData,UINT* pBaseVertexIndex) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);

    *ppIndexData = This->StateBlock->pIndexData;
    /* up ref count on ppindexdata */
    if (*ppIndexData) IDirect3DIndexBuffer8Impl_AddRef(*ppIndexData);
    *pBaseVertexIndex = This->StateBlock->baseVertexIndex;

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreatePixelShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pFunction, DWORD* pHandle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShaderImpl* object;
    HRESULT res;
    UINT i;

    TRACE_(d3d_shader)("(%p) : PixelShader not fully supported yet : Func=%p\n", This, pFunction);
    if (NULL == pFunction || NULL == pHandle) {
      return D3DERR_INVALIDCALL;
    }
    for (i = 1; NULL != PixelShaders[i] && i < sizeof(PixelShaders) / sizeof(IDirect3DPixelShaderImpl*); ++i) ;
    if (i >= sizeof(PixelShaders) / sizeof(IDirect3DPixelShaderImpl*)) {
      return D3DERR_OUTOFVIDEOMEMORY;
    }

    /** Create the Pixel Shader */
    res = IDirect3DDeviceImpl_CreatePixelShader(This, pFunction, &object);
    if (SUCCEEDED(res)) {
      PixelShaders[i] = object;
      *pHandle = VS_HIGHESTFIXEDFXF + i;
      return D3D_OK;
    } 
    *pHandle = 0xFFFFFFFF;
    return res;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    This->UpdateStateBlock->PixelShader = Handle;
    This->UpdateStateBlock->Changed.pixelShader = TRUE;
    This->UpdateStateBlock->Set.pixelShader = TRUE;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE_(d3d_shader)("Recording... not performing anything\n");
        return D3D_OK;
    }

    if (Handle != 0) {
      TRACE_(d3d_shader)("(%p) : Set pixel shader with handle %lx\n", This, Handle);
    } else {
      TRACE_(d3d_shader)("(%p) : Remove pixel shader\n", This);
    }

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD* pHandle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE_(d3d_shader)("(%p) : GetPixelShader returning %ld\n", This, This->StateBlock->PixelShader);
    *pHandle = This->StateBlock->PixelShader;
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_DeletePixelShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShaderImpl* object;   

    if (Handle <= VS_HIGHESTFIXEDFXF) { /* only delete user defined shaders */
      return D3DERR_INVALIDCALL;
    }
    object = PixelShaders[Handle - VS_HIGHESTFIXEDFXF];
    if (NULL == object) {
      return D3DERR_INVALIDCALL;
    }
    TRACE_(d3d_shader)("(%p) : freeing PixelShader %p\n", This, object);
    /* TODO: check validity of object before free */
    HeapFree(GetProcessHeap(), 0, (void *)object->function);
    if (object->prgId != 0) {
        GL_EXTCALL(glDeleteProgramsARB( 1, &object->prgId ));
    }
    HeapFree(GetProcessHeap(), 0, (void *)object->data);
    HeapFree(GetProcessHeap(), 0, (void *)object);
    PixelShaders[Handle - VS_HIGHESTFIXEDFXF] = NULL;

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  if (Register + ConstantCount > D3D8_PSHADER_MAX_CONSTANTS) {
    ERR_(d3d_shader)("(%p) : SetPixelShaderConstant C[%lu] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (ConstantCount > 1) {
    const FLOAT* f = (const FLOAT*)pConstantData;
    UINT i;
    TRACE_(d3d_shader)("(%p) : SetPixelShaderConstant C[%lu..%lu]=\n", This, Register, Register + ConstantCount - 1);
    for (i = 0; i < ConstantCount; ++i) {
      TRACE_(d3d_shader)("{%f, %f, %f, %f}\n", f[0], f[1], f[2], f[3]);
      f += 4;
    }
  } else { 
    const FLOAT* f = (const FLOAT*) pConstantData;
    TRACE_(d3d_shader)("(%p) : SetPixelShaderConstant, C[%lu]={%f, %f, %f, %f}\n", This, Register, f[0], f[1], f[2], f[3]);
  }
  This->UpdateStateBlock->Changed.pixelShaderConstant = TRUE;
  memcpy(&This->UpdateStateBlock->pixelShaderConstant[Register], pConstantData, ConstantCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  TRACE_(d3d_shader)("(%p) : C[%lu] count=%ld\n", This, Register, ConstantCount);
  if (Register + ConstantCount > D3D8_PSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  memcpy(pConstantData, &This->UpdateStateBlock->pixelShaderConstant[Register], ConstantCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD Handle, void* pData, DWORD* pSizeOfData) {
    IDirect3DPixelShaderImpl* object;

    object = PIXEL_SHADER(Handle);
    if (NULL == object) {
      return D3DERR_INVALIDCALL;
    } 
    return IDirect3DPixelShaderImpl_GetFunction(object, pData, (UINT*) pSizeOfData);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawRectPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawTriPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DeletePatch(LPDIRECT3DDEVICE8 iface, UINT Handle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8* pStreamData,UINT Stride) {
    IDirect3DVertexBuffer8 *oldSrc;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    oldSrc = This->StateBlock->stream_source[StreamNumber];
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

    if (pStreamData != NULL) IDirect3DVertexBuffer8Impl_AddRefInt(pStreamData);
    if (oldSrc != NULL) IDirect3DVertexBuffer8Impl_ReleaseInt(oldSrc);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8** pStream,UINT* pStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : StreamNo: %d, Stream (%p), Stride %d\n", This, StreamNumber, This->StateBlock->stream_source[StreamNumber], This->StateBlock->stream_stride[StreamNumber]);
    *pStream = This->StateBlock->stream_source[StreamNumber];
    *pStride = This->StateBlock->stream_stride[StreamNumber];
    if (*pStream != NULL) IDirect3DVertexBuffer8Impl_AddRef((LPDIRECT3DVERTEXBUFFER8) *pStream);
    return D3D_OK;
}


const IDirect3DDevice8Vtbl Direct3DDevice8_Vtbl =
{
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

HRESULT WINAPI IDirect3DDevice8Impl_CleanRender(LPDIRECT3DDEVICE8 iface)
{
#if defined(GL_VERSION_1_3) /* @see comments on ActiveRender */
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  ENTER_GL();

#if 0
  if (This->glCtx != This->render_ctx) {
    glXDestroyContext(This->display, This->render_ctx);
    This->render_ctx = This->glCtx;
  }
#endif
  if (This->win != This->drawable) {
    glXDestroyPbuffer(This->display, This->drawable);
    This->drawable = This->win;
  }

  LEAVE_GL();

#endif
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice8Impl_ActiveRender(LPDIRECT3DDEVICE8 iface,
						IDirect3DSurface8* RenderSurface, 
						IDirect3DSurface8* StencilSurface) {

  HRESULT ret =  D3DERR_INVALIDCALL; 
  /**
   * Currently only active for GLX >= 1.3
   * for others versions we'll have to use GLXPixmaps
   *
   * normally we must test GLX_VERSION_1_3 but nvidia headers are not correct
   * as they implements GLX 1.3 but only define GLX_VERSION_1_2
   * so only check OpenGL version
   */
#if defined(GL_VERSION_1_3) 
  GLXFBConfig* cfgs = NULL;
  int nCfgs = 0;
  int attribs[256];
  int nAttribs = 0;
  D3DFORMAT BackBufferFormat = D3D8_SURFACE(((IDirect3DSurface8Impl*) RenderSurface))->resource.format;
  D3DFORMAT StencilBufferFormat = (NULL != StencilSurface) ? D3D8_SURFACE(((IDirect3DSurface8Impl*) StencilSurface))->resource.format : 0;
  UINT Width = D3D8_SURFACE(((IDirect3DSurface8Impl*) RenderSurface))->currentDesc.Width;
  UINT Height = D3D8_SURFACE(((IDirect3DSurface8Impl*) RenderSurface))->currentDesc.Height;
  IDirect3DSurface8Impl* tmp;

  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
 
#define PUSH1(att)        attribs[nAttribs++] = (att); 
#define PUSH2(att,value)  attribs[nAttribs++] = (att); attribs[nAttribs++] = (value);

  PUSH2(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT);
  PUSH2(GLX_X_RENDERABLE,  TRUE);
  PUSH2(GLX_DOUBLEBUFFER, TRUE);
  
  switch (BackBufferFormat) {
    /* color buffer */
  case D3DFMT_P8:
    PUSH2(GLX_RENDER_TYPE,  GLX_COLOR_INDEX_BIT);
    PUSH2(GLX_BUFFER_SIZE,  8);
    PUSH2(GLX_DOUBLEBUFFER, TRUE);
    break;
    
  case D3DFMT_R3G3B2:
    PUSH2(GLX_RENDER_TYPE,  GLX_RGBA_BIT);
    PUSH2(GLX_RED_SIZE,     3);
    PUSH2(GLX_GREEN_SIZE,   3);
    PUSH2(GLX_BLUE_SIZE,    2);
    break;
    
  case D3DFMT_A1R5G5B5:
    PUSH2(GLX_ALPHA_SIZE,   1);
  case D3DFMT_X1R5G5B5:
    PUSH2(GLX_RED_SIZE,     5);
    PUSH2(GLX_GREEN_SIZE,   5);
    PUSH2(GLX_BLUE_SIZE,    5);
    break;
    
  case D3DFMT_R5G6B5:
    PUSH2(GLX_RED_SIZE,     5);
    PUSH2(GLX_GREEN_SIZE,   6);
    PUSH2(GLX_BLUE_SIZE,    5);
    break;
    
  case D3DFMT_A4R4G4B4:
    PUSH2(GLX_ALPHA_SIZE,   4);
  case D3DFMT_X4R4G4B4:
    PUSH2(GLX_RED_SIZE,     4);
    PUSH2(GLX_GREEN_SIZE,   4);
    PUSH2(GLX_BLUE_SIZE,    4);
    break;
    
  case D3DFMT_A8R8G8B8:
    PUSH2(GLX_ALPHA_SIZE,   8);
  case D3DFMT_R8G8B8:
  case D3DFMT_X8R8G8B8:
    PUSH2(GLX_RED_SIZE,     8);
    PUSH2(GLX_GREEN_SIZE,   8);
    PUSH2(GLX_BLUE_SIZE,    8);
    break;

  default:
    break;
  }
   
  switch (StencilBufferFormat) { 
  case D3DFMT_D16_LOCKABLE:
  case D3DFMT_D16:
    PUSH2(GLX_DEPTH_SIZE,   16);
    break;
    
  case D3DFMT_D15S1:
    PUSH2(GLX_DEPTH_SIZE,   15);
    break;
    
  case D3DFMT_D24X8:
    PUSH2(GLX_DEPTH_SIZE,   24);
    break;
    
  case D3DFMT_D24X4S4:
    PUSH2(GLX_DEPTH_SIZE,   24);
    PUSH2(GLX_STENCIL_SIZE, 4);
    break;
    
  case D3DFMT_D24S8:
    PUSH2(GLX_DEPTH_SIZE,   24);
    PUSH2(GLX_STENCIL_SIZE, 8);
    break;
    
  case D3DFMT_D32:
    PUSH2(GLX_DEPTH_SIZE,   24);
    break;

  default:
    break;
  }

  PUSH1(None);
  
  ENTER_GL();

  cfgs = glXChooseFBConfig(This->display, DefaultScreen(This->display), attribs, &nCfgs);
  if (NULL != cfgs) {
#ifdef EXTRA_TRACES
    int i;
    for (i = 0; i < nCfgs; ++i) {
      TRACE("for (%u,%s)/(%u,%s) found config[%d]@%p\n", BackBufferFormat, debug_d3dformat(BackBufferFormat), StencilBufferFormat, debug_d3dformat(StencilBufferFormat), i, cfgs[i]);
    }
#endif

    if (NULL != This->renderTarget) {
      /*GLenum prev_read; */
      glFlush();
      vcheckGLcall("glFlush");

#ifdef EXTRA_TRACES
      /** very very useful debug code */
      glXSwapBuffers(This->display, This->drawable);   
      printf("Hit Enter to get next frame ...\n");
      getchar();
#endif

#if 0
      glGetIntegerv(GL_READ_BUFFER, &prev_read);
      vcheckGLcall("glIntegerv");
      glReadBuffer(GL_BACK);
      vcheckGLcall("glReadBuffer");
      {
	long j;
	long pitch = This->renderTarget->myDesc.Width * This->renderTarget->bytesPerPixel;

        if (This->renderTarget->myDesc.Format == D3DFMT_DXT1) /* DXT1 is half byte per pixel */
            pitch = pitch / 2;

	for (j = 0; j < This->renderTarget->myDesc.Height; ++j) {
	  glReadPixels(0, 
		       This->renderTarget->myDesc.Height - j - 1, 
		       This->renderTarget->myDesc.Width, 
		       1,
		       D3DFmt2GLFmt(This, This->renderTarget->myDesc.Format), 
		       D3DFmt2GLType(This, This->renderTarget->myDesc.Format), 
		       This->renderTarget->allocatedMemory + j * pitch);
	  vcheckGLcall("glReadPixels");
	}
      }      
      glReadBuffer(prev_read);
      vcheckGLcall("glReadBuffer");
#endif
    }

    if (BackBufferFormat != D3D8_SURFACE(This->renderTarget)->resource.format && 
	StencilBufferFormat != D3D8_SURFACE(This->stencilBufferTarget)->resource.format) {
      nAttribs = 0;
      PUSH2(GLX_PBUFFER_WIDTH,  Width);
      PUSH2(GLX_PBUFFER_HEIGHT, Height);
      PUSH1(None);
      This->drawable = glXCreatePbuffer(This->display, cfgs[0], attribs);
            
      This->render_ctx = glXCreateNewContext(This->display, cfgs[0], GLX_RGBA_TYPE, This->glCtx, TRUE);
      if (NULL == This->render_ctx) {
	ERR("cannot create glxContext\n"); 
      }

      glFlush();
      glXSwapBuffers(This->display, This->drawable);
      if (glXMakeContextCurrent(This->display, This->drawable, This->drawable, This->render_ctx) == False) {
	TRACE("Error in setting current context: context %p drawable %ld (default %ld)!\n", This->glCtx, This->drawable, This->win);
      }
      checkGLcall("glXMakeContextCurrent");
    }

    tmp = This->renderTarget;
    This->renderTarget = (IDirect3DSurface8Impl*) RenderSurface;
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) This->renderTarget);
    IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8) tmp);

    tmp = This->stencilBufferTarget;
    This->stencilBufferTarget = (IDirect3DSurface8Impl*) StencilSurface;
    if (NULL != This->stencilBufferTarget) IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) This->stencilBufferTarget);
    if (NULL != tmp) IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8) tmp);

    {
      DWORD value;
      /* The surface must be rendered upside down to cancel the flip produce by glCopyTexImage */
      This->renderUpsideDown = (This->renderTarget != This->frontBuffer) && (This->renderTarget != This->backBuffer);
      /* Force updating the cull mode */
      IDirect3DDevice8_GetRenderState(iface, D3DRS_CULLMODE, &value);
      IDirect3DDevice8_SetRenderState(iface, D3DRS_CULLMODE, value);
      /* Force updating projection matrix */
      This->last_was_rhw = FALSE;
      This->proj_valid = FALSE;
    }

    ret = D3D_OK;

  } else {
    ERR("cannot get valides GLXFBConfig for (%u,%s)/(%u,%s)\n", BackBufferFormat, debug_d3dformat(BackBufferFormat), StencilBufferFormat, debug_d3dformat(StencilBufferFormat));
  }

#undef PUSH1
#undef PUSH2

  LEAVE_GL();

#endif

  return ret;
}

/* Internal function called back during the CreateDevice to create a render target  */
HRESULT WINAPI D3D8CB_CreateSurface(IUnknown *device, UINT Width, UINT Height, 
                                         WINED3DFORMAT Format, DWORD Usage, D3DPOOL Pool, UINT Level,
                                         IWineD3DSurface **ppSurface, HANDLE *pSharedHandle) {

    HRESULT res = D3D_OK;
    IDirect3DSurface8Impl *d3dSurface = NULL;
    BOOL Lockable = TRUE;

    if((D3DPOOL_DEFAULT == Pool && D3DUSAGE_DYNAMIC != Usage))
        Lockable = FALSE;

    TRACE("relay\n");
    res = IDirect3DDevice8Impl_CreateSurface((IDirect3DDevice8 *)device, Width, Height, (D3DFORMAT)Format, Lockable, FALSE/*Discard*/, Level,  (IDirect3DSurface8 **)&d3dSurface, D3DRTYPE_SURFACE, Usage, Pool, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);

    if (res == D3D_OK) {
        *ppSurface = d3dSurface->wineD3DSurface;
    } else {
        FIXME("(%p) IDirect3DDevice8_CreateSurface failed\n", device);
    }
    return res;
}
