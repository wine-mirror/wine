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

#include "config.h"

#include <math.h>

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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_shader);

IDirect3DVertexShaderImpl*            VertexShaders[64];
IDirect3DVertexShaderDeclarationImpl* VertexShaderDeclarations[64];
IDirect3DPixelShaderImpl*             PixelShaders[64];

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
void setupTextureStates(LPDIRECT3DDEVICE8 iface, DWORD Stage) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    int i = 0;
    float col[4];

    /* Make appropriate texture active */
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
        glActiveTexture(GL_TEXTURE0 + Stage);
#else
        glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
#endif
        checkGLcall("glActiveTextureARB");
    } else if (Stage > 0) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesnt support\n");
    }

    TRACE("-----------------------> Updating the texture at stage %ld to have new texture state information\n", Stage);
    for (i = 1; i < HIGHEST_TEXTURE_STATE; i++) {
        IDirect3DDevice8Impl_SetTextureStageState(iface, Stage, i, This->StateBlock->texture_state[Stage][i]);
    }

    /* Note the D3DRS value applies to all textures, but GL has one
     *  per texture, so apply it now ready to be used!
     */
    D3DCOLORTOGLFLOAT4(This->StateBlock->renderstate[D3DRS_TEXTUREFACTOR], col);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &col[0]);
    checkGLcall("glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);");

    TRACE("-----------------------> Updated the texture at stage %ld to have new texture state information\n", Stage);
}

/* IDirect3D IUnknown parts follow: */
HRESULT WINAPI IDirect3DDevice8Impl_QueryInterface(LPDIRECT3DDEVICE8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DDevice8Impl,iface);

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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DDevice8Impl_Release(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
      IDirect3DDevice8Impl_CleanRender(iface);
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) copying to %p\n", This, pParameters);    
    memcpy(pParameters, &This->CreateParms, sizeof(D3DDEVICE_CREATION_PARAMETERS));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetCursorProperties(LPDIRECT3DDEVICE8 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) {
    IDirect3DSurface8Impl* pSur = (IDirect3DSurface8Impl*) pCursorBitmap;
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Spot Pos(%u,%u)\n", This, XHotSpot, YHotSpot);

    if (D3DFMT_A8R8G8B8 != pSur->myDesc.Format) {
      ERR("(%p) : surface(%p) have a invalid format\n", This, pCursorBitmap);
      return D3DERR_INVALIDCALL;
    }
    if (32 != pSur->myDesc.Height || 32 != pSur->myDesc.Width) {
      ERR("(%p) : surface(%p) have a invalid size\n", This, pCursorBitmap);
      return D3DERR_INVALIDCALL;
    }

    This->xHotSpot = XHotSpot;
    This->yHotSpot = YHotSpot;
    return D3D_OK;
}
void     WINAPI  IDirect3DDevice8Impl_SetCursorPosition(LPDIRECT3DDEVICE8 iface, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : SetPos to (%u,%u)\n", This, XScreenSpace, YScreenSpace);
    This->xScreenSpace = XScreenSpace;
    This->yScreenSpace = YScreenSpace;
    return;
}
BOOL     WINAPI  IDirect3DDevice8Impl_ShowCursor(LPDIRECT3DDEVICE8 iface, BOOL bShow) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : visible(%d)\n", This, bShow); 
    This->bCursorVisible = bShow;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain) {
    IDirect3DSwapChain8Impl* object;
    ICOM_THIS(IDirect3DDevice8Impl,iface);
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_Present(LPDIRECT3DDEVICE8 iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : complete stub!\n", This);

    ENTER_GL();

    glXSwapBuffers(This->display, This->drawable);
    /* Dont call checkGLcall, as glGetError is not applicable here */
    TRACE("glXSwapBuffers called, Starting new frame\n");

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

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetBackBuffer(LPDIRECT3DDEVICE8 iface, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
void     WINAPI  IDirect3DDevice8Impl_SetGammaRamp(LPDIRECT3DDEVICE8 iface, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
    HDC hDC;
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    FIXME("(%p) : pRamp@%p\n", This, pRamp);
    hDC = GetDC(This->win_handle);
    SetDeviceGammaRamp(hDC, (LPVOID) pRamp);
    ReleaseDC(This->win_handle, hDC);
    return;
}
void     WINAPI  IDirect3DDevice8Impl_GetGammaRamp(LPDIRECT3DDEVICE8 iface, D3DGAMMARAMP* pRamp) {
    HDC hDC;
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    FIXME("(%p) : pRamp@%p\n", This, pRamp);
    hDC = GetDC(This->win_handle);
    GetDeviceGammaRamp(hDC, pRamp);
    ReleaseDC(This->win_handle, hDC);
    return;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateTexture(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                                    D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture) {
    IDirect3DTexture8Impl *object;
    int i;
    UINT tmpW;
    UINT tmpH;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* Allocate the storage for the device */
    TRACE("(%p) : W(%d) H(%d), Lvl(%d) Usage(%ld), Fmt(%u,%s), Pool(%d)\n", This, Width, Height, Levels, Usage, Format, debug_d3dformat(Format), Pool);
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

    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        object->levels++;
        tmpW = Width;
        tmpH = Height;
        while (tmpW > 1 && tmpH > 1) {
            tmpW = max(1, tmpW / 2);
            tmpH = max(1, tmpH / 2);
            object->levels++;
        }
        TRACE("Calculated levels = %d\n", object->levels);
    }

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    for (i = 0; i < object->levels; i++) 
    {
        IDirect3DDevice8Impl_CreateImageSurface(iface, tmpW, tmpH, Format, (LPDIRECT3DSURFACE8*) &object->surfaces[i]);
        object->surfaces[i]->Container = (IUnknown*) object;
        object->surfaces[i]->myDesc.Usage = Usage;
        object->surfaces[i]->myDesc.Pool = Pool;
	/** 
	 * As writen in msdn in IDirect3DTexture8::LockRect
	 *  Textures created in D3DPOOL_DEFAULT are not lockable.
	 */
	if (D3DPOOL_DEFAULT == Pool) {
	  object->surfaces[i]->lockable = FALSE;
	}

        TRACE("Created surface level %d @ %p, memory at %p\n", i, object->surfaces[i], object->surfaces[i]->allocatedMemory);
        tmpW = max(1, tmpW / 2);
        tmpH = max(1, tmpH / 2);
    }

    *ppTexture = (LPDIRECT3DTEXTURE8) object;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVolumeTexture(LPDIRECT3DDEVICE8 iface, 
                                                          UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, 
                                                          D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) {

    IDirect3DVolumeTexture8Impl *object;
    int i;
    UINT tmpW;
    UINT tmpH;
    UINT tmpD;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* Allocate the storage for it */
    TRACE("(%p) : W(%d) H(%d) D(%d), Lvl(%d) Usage(%ld), Fmt(%u,%s), Pool(%s)\n", This, Width, Height, Depth, Levels, Usage, Format, debug_d3dformat(Format), debug_d3dpool(Pool));
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolumeTexture8Impl));
    object->lpVtbl = &Direct3DVolumeTexture8_Vtbl;
    object->ResourceType = D3DRTYPE_VOLUMETEXTURE;
    object->Device = This;
    object->ref = 1;

    object->width = Width;
    object->height = Height;
    object->depth = Depth;
    object->levels = Levels;
    object->usage = Usage;
    object->format = Format;

    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        object->levels++;
        tmpW = Width;
        tmpH = Height;
        tmpD = Depth;
        while (tmpW > 1 && tmpH > 1 && tmpD > 1) {
            tmpW = max(1, tmpW / 2);
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

    for (i = 0; i < object->levels; i++) 
    {
        IDirect3DVolume8Impl* volume;

        /* Create the volume - No entry point for this seperately?? */
        volume  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolume8Impl));
        object->volumes[i] = (IDirect3DVolume8Impl *) volume;

        volume->lpVtbl = &Direct3DVolume8_Vtbl;
        volume->Device = This;
        volume->ResourceType = D3DRTYPE_VOLUME;
        volume->Container = (IUnknown*) object;
        volume->ref = 1;

        volume->myDesc.Width  = Width;
        volume->myDesc.Height = Height;
        volume->myDesc.Depth  = Depth;
        volume->myDesc.Format = Format;
        volume->myDesc.Type   = D3DRTYPE_VOLUME;
        volume->myDesc.Pool   = Pool;
        volume->myDesc.Usage  = Usage;
        volume->bytesPerPixel   = D3DFmtGetBpp(This, Format);
        volume->myDesc.Size     = (Width * volume->bytesPerPixel) * Height * Depth;
        volume->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, volume->myDesc.Size);

	volume->lockable = TRUE;
	volume->locked = FALSE;
	memset(&volume->lockedBox, 0, sizeof(D3DBOX));
	volume->Dirty = FALSE;
	IDirect3DVolume8Impl_CleanDirtyBox((LPDIRECT3DVOLUME8) volume);

        TRACE("(%p) : Volume at w(%d) h(%d) d(%d) fmt(%u,%s) surf@%p, surfmem@%p, %d bytes\n", 
              This, Width, Height, Depth, Format, debug_d3dformat(Format),
              volume, volume->allocatedMemory, volume->myDesc.Size);

        tmpW = max(1, tmpW / 2);
        tmpH = max(1, tmpH / 2);
        tmpD = max(1, tmpD / 2);
    }

    *ppVolumeTexture = (LPDIRECT3DVOLUMETEXTURE8) object;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateCubeTexture(LPDIRECT3DDEVICE8 iface, UINT EdgeLength, UINT Levels, DWORD Usage, 
                                                        D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) {

    IDirect3DCubeTexture8Impl *object;
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    int i,j;
    UINT tmpW;

    /* Allocate the storage for it */
    TRACE("(%p) : Len(%d), Lvl(%d) Usage(%ld), Fmt(%u,%s), Pool(%s)\n", This, EdgeLength, Levels, Usage, Format, debug_d3dformat(Format), debug_d3dpool(Pool));
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DCubeTexture8Impl));
    object->lpVtbl = &Direct3DCubeTexture8_Vtbl;
    object->ref = 1;
    object->Device = This;
    object->ResourceType = D3DRTYPE_CUBETEXTURE;

    object->edgeLength = EdgeLength;
    object->levels = Levels;
    object->usage = Usage;
    object->format = Format;

    /* Calculate levels for mip mapping */
    if (Levels == 0) {
        object->levels++;
        tmpW = EdgeLength;
        while (tmpW > 1) {
            tmpW = max(1, tmpW / 2);
            object->levels++;
        }
        TRACE("Calculated levels = %d\n", object->levels);
    }

    /* Generate all the surfaces */
    tmpW = EdgeLength;
    for (i = 0; i < object->levels; i++) {
        /* Create the 6 faces */
        for (j = 0; j < 6; j++) {
           IDirect3DDevice8Impl_CreateImageSurface(iface, tmpW, tmpW, Format, (LPDIRECT3DSURFACE8*) &object->surfaces[j][i]);
           object->surfaces[j][i]->Container = (IUnknown*) object;
           object->surfaces[j][i]->myDesc.Usage = Usage;
           object->surfaces[j][i]->myDesc.Pool = Pool;
	   /** 
	    * As writen in msdn in IDirect3DCubeTexture8::LockRect
	    *  Textures created in D3DPOOL_DEFAULT are not lockable.
	    */
	   if (D3DPOOL_DEFAULT == Pool) {
	     object->surfaces[j][i]->lockable = FALSE;
	   }

           TRACE("Created surface level %d @ %p, memory at %p\n", i, object->surfaces[j][i], object->surfaces[j][i]->allocatedMemory);
        }
        tmpW = max(1, tmpW / 2);
    }

    TRACE("(%p) : Iface@%p\n", This, object);
    *ppCubeTexture = (LPDIRECT3DCUBETEXTURE8) object;
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexBuffer(LPDIRECT3DDEVICE8 iface, UINT Size, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) {
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

    *ppVertexBuffer = (LPDIRECT3DVERTEXBUFFER8) object;

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateIndexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) {
    IDirect3DIndexBuffer8Impl *object;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
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
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateRenderTarget(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) {
    IDirect3DSurface8Impl *object;
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    
    object  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    if (NULL == object) {
      *ppSurface = NULL;
      return D3DERR_OUTOFVIDEOMEMORY;
    }
    *ppSurface = (LPDIRECT3DSURFACE8) object;
    object->lpVtbl = &Direct3DSurface8_Vtbl;
    object->Device = This;
    object->ResourceType = D3DRTYPE_SURFACE;
    object->Container = (IUnknown*) This;

    object->ref = 1;
    object->myDesc.Width  = Width;
    object->myDesc.Height = Height;
    object->myDesc.Format = Format;
    object->myDesc.Type = D3DRTYPE_SURFACE;
    object->myDesc.Usage = D3DUSAGE_RENDERTARGET;
    object->myDesc.Pool = D3DPOOL_DEFAULT;
    object->myDesc.MultiSampleType = MultiSample;
    object->bytesPerPixel = D3DFmtGetBpp(This, Format);
    object->myDesc.Size = (Width * object->bytesPerPixel) * Height;
    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->myDesc.Size);
    object->lockable = Lockable;
    object->locked = FALSE;
    memset(&object->lockedRect, 0, sizeof(RECT));
    IDirect3DSurface8Impl_CleanDirtyRect((LPDIRECT3DSURFACE8) object);

    TRACE("(%p) : w(%d) h(%d) fmt(%d,%s) lockable(%d) surf@%p, surfmem@%p, %d bytes\n", This, Width, Height, Format, debug_d3dformat(Format), Lockable, *ppSurface, object->allocatedMemory, object->myDesc.Size);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) {
    IDirect3DSurface8Impl *object;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    object  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    if (NULL == object) {
      *ppSurface = NULL;
      return D3DERR_OUTOFVIDEOMEMORY;
    }
    *ppSurface = (LPDIRECT3DSURFACE8) object;
    object->lpVtbl = &Direct3DSurface8_Vtbl;
    object->Device = This;
    object->ResourceType = D3DRTYPE_SURFACE;
    object->Container = (IUnknown*) This;

    object->ref = 1;
    object->myDesc.Width  = Width;
    object->myDesc.Height = Height;
    object->myDesc.Format = Format;
    object->myDesc.Type = D3DRTYPE_SURFACE;
    object->myDesc.Usage = D3DUSAGE_DEPTHSTENCIL;
    object->myDesc.Pool = D3DPOOL_DEFAULT;
    object->myDesc.MultiSampleType = MultiSample;
    object->bytesPerPixel = D3DFmtGetBpp(This, Format);
    object->myDesc.Size = (Width * object->bytesPerPixel) * Height;
    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->myDesc.Size);
    object->lockable = (D3DFMT_D16_LOCKABLE == Format) ? TRUE : FALSE;
    object->locked = FALSE;
    memset(&object->lockedRect, 0, sizeof(RECT));
    IDirect3DSurface8Impl_CleanDirtyRect((LPDIRECT3DSURFACE8) object);

    TRACE("(%p) : w(%d) h(%d) fmt(%d,%s) surf@%p, surfmem@%p, %d bytes\n", This, Width, Height, Format, debug_d3dformat(Format), *ppSurface, object->allocatedMemory, object->myDesc.Size);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateImageSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) {
    IDirect3DSurface8Impl *object;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    object  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    *ppSurface = (LPDIRECT3DSURFACE8) object;
    object->lpVtbl = &Direct3DSurface8_Vtbl;
    object->Device = This;
    object->ResourceType = D3DRTYPE_SURFACE;
    object->Container = (IUnknown*) This;

    object->ref = 1;
    object->myDesc.Width  = Width;
    object->myDesc.Height = Height;
    object->myDesc.Format = Format;
    object->myDesc.Type = D3DRTYPE_SURFACE;
    object->myDesc.Usage = 0;
    object->myDesc.Pool = D3DPOOL_SYSTEMMEM;
    object->bytesPerPixel = D3DFmtGetBpp(This, Format);
    object->myDesc.Size = (Width * object->bytesPerPixel) * Height;
    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->myDesc.Size);
    object->lockable = TRUE;
    object->locked = FALSE;
    memset(&object->lockedRect, 0, sizeof(RECT));
    IDirect3DSurface8Impl_CleanDirtyRect((LPDIRECT3DSURFACE8) object);

    TRACE("(%p) : w(%d) h(%d) fmt(%d,%s) surf@%p, surfmem@%p, %d bytes\n", This, Width, Height, Format, debug_d3dformat(Format), *ppSurface, object->allocatedMemory, object->myDesc.Size);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CopyRects(LPDIRECT3DDEVICE8 iface, 
                                                IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects,
                                                IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray) {

    HRESULT rc = D3D_OK;
    IDirect3DBaseTexture8* texture = NULL;


    IDirect3DSurface8Impl* src = (IDirect3DSurface8Impl*) pSourceSurface;
    IDirect3DSurface8Impl* dst = (IDirect3DSurface8Impl*) pDestinationSurface;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) pSrcSur=%p, pSourceRects=%p, cRects=%d, pDstSur=%p, pDestPtsArr=%p\n", This,
          pSourceSurface, pSourceRectsArray, cRects, pDestinationSurface, pDestPointsArray);

    /* Note: Not sure about the d3dfmt_unknown bit, but seems to avoid a problem inside
         a sample and doesnt seem to break anything as far as I can tell               */
    if (src->myDesc.Format != dst->myDesc.Format && (dst->myDesc.Format != D3DFMT_UNKNOWN)) {
        TRACE("Formats do not match (%x,%s) / (%x,%s)\n", 
               src->myDesc.Format, debug_d3dformat(src->myDesc.Format), 
               dst->myDesc.Format, debug_d3dformat(dst->myDesc.Format));
        rc = D3DERR_INVALIDCALL;

    } else if (dst->myDesc.Format == D3DFMT_UNKNOWN) {
        TRACE("Converting dest to same format as source, since dest was unknown\n");
        dst->myDesc.Format = src->myDesc.Format;

        /* Convert container as well */
        IDirect3DSurface8Impl_GetContainer((LPDIRECT3DSURFACE8) dst, &IID_IDirect3DBaseTexture8, (void**) &texture); /* FIXME: Which refid? */
        if (texture != NULL) {
            ((IDirect3DBaseTexture8Impl*) texture)->format = src->myDesc.Format;
	    /** Releasing texture after GetContainer */
	    IDirect3DBaseTexture8_Release(texture);
	    texture = NULL;
        }
    }

    /* Quick if complete copy ... */
    if (rc == D3D_OK && cRects == 0 && pSourceRectsArray == NULL && pDestPointsArray == NULL) {

      if (src->myDesc.Width == dst->myDesc.Width && src->myDesc.Height == dst->myDesc.Height) {

        D3DLOCKED_RECT lrSrc;
        D3DLOCKED_RECT lrDst;
        IDirect3DSurface8Impl_LockRect((LPDIRECT3DSURFACE8) src, &lrSrc, NULL, D3DLOCK_READONLY);
	IDirect3DSurface8Impl_LockRect((LPDIRECT3DSURFACE8) dst, &lrDst, NULL, 0L);
        TRACE("Locked src and dst, Direct copy as surfaces are equal, w=%d, h=%d\n", dst->myDesc.Width, dst->myDesc.Height);

	/*memcpy(dst->allocatedMemory, src->allocatedMemory, src->myDesc.Size);*/
	memcpy(lrDst.pBits, lrSrc.pBits, src->myDesc.Size);
 
        IDirect3DSurface8Impl_UnlockRect((LPDIRECT3DSURFACE8) src);
        rc = IDirect3DSurface8Impl_UnlockRect((LPDIRECT3DSURFACE8) dst);
        TRACE("Unlocked src and dst\n");

      } else {

	FIXME("Wanted to copy all surfaces but size not compatible\n");
        rc = D3DERR_INVALIDCALL;

      }

    } else {

      if (NULL != pSourceRectsArray && NULL != pDestPointsArray) {

        int bytesPerPixel = ((IDirect3DSurface8Impl*) pSourceSurface)->bytesPerPixel;
        int i;
        /* Copy rect by rect */
        for (i = 0; i < cRects; i++) {
            CONST RECT*  r = &pSourceRectsArray[i];
            CONST POINT* p = &pDestPointsArray[i];
            int copyperline = (r->right - r->left) * bytesPerPixel;
            int j;
            D3DLOCKED_RECT lrSrc;
            D3DLOCKED_RECT lrDst;
            RECT dest_rect;
 

            TRACE("Copying rect %d (%ld,%ld),(%ld,%ld) -> (%ld,%ld)\n", i, r->left, r->top, r->right, r->bottom, p->x, p->y);

            IDirect3DSurface8Impl_LockRect((LPDIRECT3DSURFACE8) src, &lrSrc, r, D3DLOCK_READONLY);
            dest_rect.left  = p->x;
            dest_rect.top   = p->y;
            dest_rect.right = p->x + (r->right - r->left);
            dest_rect.left  = p->y + (r->bottom - r->top);
            IDirect3DSurface8Impl_LockRect((LPDIRECT3DSURFACE8) dst, &lrDst, &dest_rect, 0L);
            TRACE("Locked src and dst\n");

            /* Find where to start */
#if 0
            from = copyfrom + (r->top * pitchFrom) + (r->left * bytesPerPixel);
            to   = copyto   + (p->y * pitchTo) + (p->x * bytesPerPixel);
            /* Copy line by line */
            for (j = 0; j < (r->bottom - r->top); j++) {
               memcpy(to + (j * pitchTo), from + (j * pitchFrom), copyperline);
            }
#endif

	    for (j = 0; j < (r->bottom - r->top); j++) {
               memcpy((char*) lrDst.pBits + (j * lrDst.Pitch), (char*) lrSrc.pBits + (j * lrSrc.Pitch), copyperline);
	    }

            IDirect3DSurface8Impl_UnlockRect((LPDIRECT3DSURFACE8) src);
            rc = IDirect3DSurface8Impl_UnlockRect((LPDIRECT3DSURFACE8) dst);
            TRACE("Unlocked src and dst\n");
        }
      
      } else {
      
	FIXME("Wanted to copy partial surfaces not implemented\n");
        rc = D3DERR_INVALIDCALL;	
	
      }
    }

    return rc;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_UpdateTexture(LPDIRECT3DDEVICE8 iface, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) {
    IDirect3DBaseTexture8Impl* src = (IDirect3DBaseTexture8Impl*) pSourceTexture;
    IDirect3DBaseTexture8Impl* dst = (IDirect3DBaseTexture8Impl*) pDestinationTexture;
    D3DRESOURCETYPE srcType;
    D3DRESOURCETYPE dstType;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : first try\n", This);

    srcType = IDirect3DBaseTexture8Impl_GetType(pSourceTexture);
    dstType = IDirect3DBaseTexture8Impl_GetType(pDestinationTexture);

    if (srcType != dstType) {
      return D3DERR_INVALIDCALL;
    }
    if (D3DPOOL_SYSTEMMEM != IDirect3DResource8Impl_GetPool((LPDIRECT3DRESOURCE8) src)) {
      return D3DERR_INVALIDCALL;
    }
    if (D3DPOOL_DEFAULT != IDirect3DResource8Impl_GetPool((LPDIRECT3DRESOURCE8) dst)) {
      return D3DERR_INVALIDCALL;
    }
    if (IDirect3DBaseTexture8Impl_IsDirty(pSourceTexture)) {
      /** Only copy Dirty textures */
      DWORD srcLevelCnt = IDirect3DBaseTexture8Impl_GetLevelCount(pSourceTexture);
      DWORD dstLevelCnt = IDirect3DBaseTexture8Impl_GetLevelCount(pDestinationTexture);
      DWORD skipLevels = (dstLevelCnt < srcLevelCnt) ? srcLevelCnt - dstLevelCnt : 0;
      UINT i, j;

      for (i = skipLevels; i < srcLevelCnt; ++i) {
	HRESULT hr;

	switch (srcType) { 
	case D3DRTYPE_TEXTURE:
	  {
	    IDirect3DSurface8* srcSur = NULL;
	    IDirect3DSurface8* dstSur = NULL;
	    hr = IDirect3DTexture8Impl_GetSurfaceLevel((LPDIRECT3DTEXTURE8) src, i, &srcSur);
	    hr = IDirect3DTexture8Impl_GetSurfaceLevel((LPDIRECT3DTEXTURE8) dst, i - skipLevels, &dstSur);
	    /*IDirect3DDevice8_CopyRects*/
	    IDirect3DSurface8Impl_Release(srcSur);
	    IDirect3DSurface8Impl_Release(dstSur);
	  }
	  break;
	case D3DRTYPE_VOLUMETEXTURE:
	  {
	    FIXME("D3DRTYPE_VOLUMETEXTURE reload currently not implemented\n");
	  }
	  break;
	case D3DRTYPE_CUBETEXTURE:
	  {
	    IDirect3DSurface8* srcSur = NULL;
	    IDirect3DSurface8* dstSur = NULL;
	    for (j = 0; j < 5; ++j) {
	      hr = IDirect3DCubeTexture8Impl_GetCubeMapSurface((LPDIRECT3DCUBETEXTURE8) src, j, i, &srcSur);
	      hr = IDirect3DCubeTexture8Impl_GetCubeMapSurface((LPDIRECT3DCUBETEXTURE8) dst, j, i - skipLevels, &srcSur);
	      /*IDirect3DDevice8_CopyRects*/
	      IDirect3DSurface8Impl_Release(srcSur);
	      IDirect3DSurface8Impl_Release(dstSur);
	    }
	  }
	  break;
	default:
	  break;
	}
      }
      IDirect3DBaseTexture8Impl_SetDirty(pSourceTexture, FALSE);
    }
    
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetFrontBuffer(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pDestSurface) {
    HRESULT hr;
    D3DLOCKED_RECT lockedRect;
    RECT wantedRect;
    GLint  prev_store;
    GLenum prev_read;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    FIXME("(%p) : see if behavior correct\n", This);

    if (D3DFMT_A8R8G8B8 != ((IDirect3DSurface8Impl*) pDestSurface)->myDesc.Format) {
      ERR("(%p) : surface(%p) have a invalid format\n", This, pDestSurface);
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

    /*
    {
      IDirect3DSurface8Impl* tmp = ((IDirect3DSurface8Impl*) pDestSurface);
      FIXME("dest:%u,%u,bpp:%u\n", tmp->myDesc.Width, tmp->myDesc.Height, tmp->bytesPerPixel);
      FIXME("dest2:pitch%u\n", lockedRect.Pitch);
      FIXME("src:%u,%u\n", This->PresentParms.BackBufferWidth, This->PresentParms.BackBufferHeight);
      tmp = This->frontBuffer;
      FIXME("src2:%u,%u,bpp:%u\n", tmp->myDesc.Width, tmp->myDesc.Height, tmp->bytesPerPixel);
    }
    */

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
      long j;
      for (j = 0; j < This->PresentParms.BackBufferHeight; ++j) {
	/*memcpy(lockedRect.pBits + (j * lockedRect.Pitch), This->frontBuffer->allocatedMemory + (j * i), i);*/
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
    HRESULT hr;

    ICOM_THIS(IDirect3DDevice8Impl,iface);

    if ((IDirect3DSurface8Impl*) pRenderTarget == This->renderTarget && (IDirect3DSurface8Impl*) pNewZStencil == This->stencilBufferTarget) {
      TRACE("Trying to do a NOP SetRenderTarget operation\n");
      return D3D_OK;
    }

    IDirect3DDevice8Impl_CleanRender(iface);

    if ((IDirect3DSurface8Impl*) pRenderTarget == This->frontBuffer && (IDirect3DSurface8Impl*) pNewZStencil == This->depthStencilBuffer) {
      IDirect3DSurface8Impl* tmp;

      TRACE("retoring SetRenderTarget defaults\n");

      tmp = This->renderTarget;
      This->renderTarget = (IDirect3DSurface8Impl*) This->frontBuffer;
      IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) This->renderTarget);
      IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8) tmp);
      
      tmp = This->stencilBufferTarget;
      This->stencilBufferTarget = (IDirect3DSurface8Impl*) This->depthStencilBuffer;
      if (NULL != This->stencilBufferTarget) IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) This->stencilBufferTarget);
      if (NULL != tmp) IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8) tmp);

      return D3D_OK;
    }

    TRACE("(%p) : expect crash newRender@%p newZStencil@%p\n", This, pRenderTarget, pNewZStencil);

    hr = IDirect3DDevice8Impl_ActiveRender(iface, pRenderTarget, pNewZStencil);
    
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppRenderTarget) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    TRACE("(%p)->(%p) default(%p)\n", This, This->renderTarget, This->frontBuffer);
    
    *ppRenderTarget = (LPDIRECT3DSURFACE8) This->renderTarget;
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppRenderTarget);
    
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppZStencilSurface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    TRACE("(%p)->(%p) default(%p)\n", This, This->stencilBufferTarget, This->depthStencilBuffer);
    
    /* Note inc ref on returned surface */
    *ppZStencilSurface = (LPDIRECT3DSURFACE8) This->stencilBufferTarget;
    if (NULL != *ppZStencilSurface) IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppZStencilSurface);

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_BeginScene(LPDIRECT3DDEVICE8 iface) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_EndScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DBaseTexture8* cont = NULL;
    HRESULT hr;
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p)\n", This);

    ENTER_GL();

    glFlush();
    checkGLcall("glFlush");

    /* Useful for debugging sometimes!
    printf("Hit Enter ...\n");
    getchar(); */

    if (This->frontBuffer != This->renderTarget) {
      {
	GLenum prev_read;
	glGetIntegerv(GL_READ_BUFFER, &prev_read);
	vcheckGLcall("glIntegerv");
	glReadBuffer(GL_BACK);
	vcheckGLcall("glReadBuffer");
	{
	  long j;
	  long pitch = This->renderTarget->myDesc.Width * This->renderTarget->bytesPerPixel;
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
      }

      hr = IDirect3DSurface8_GetContainer((LPDIRECT3DSURFACE8) This->renderTarget, &IID_IDirect3DBaseTexture8, (void**) &cont);
      if (SUCCEEDED(hr) && NULL != cont) {
	/** always dirtify for now. we must find a better way to see that surface have been modified */
	IDirect3DBaseTexture8Impl_SetDirty(cont, TRUE);
	IDirect3DBaseTexture8_PreLoad(cont);
	IDirect3DBaseTexture8Impl_Release(cont);
	cont = NULL;
      }
    }

    LEAVE_GL();
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_Clear(LPDIRECT3DDEVICE8 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* TODO: From MSDN This method fails if you specify the D3DCLEAR_ZBUFFER or D3DCLEAR_STENCIL flags when the
      render target does not have an attached depth buffer. Similarly, if you specify the D3DCLEAR_STENCIL flag
      when the depth-buffer format does not contain stencil buffer information, this method fails. */
    GLbitfield glMask = 0;
    GLboolean old_ztest;
    GLfloat old_z_clear_value;
    GLint   old_stencil_clear_value;
    GLfloat old_color_clear_value[4];
    int i;
    CONST D3DRECT* curRect;

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

    /* Only set the values up once, as they are not changing */
    if (Flags & D3DCLEAR_STENCIL) {	
        glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &old_stencil_clear_value);
        glClearStencil(Stencil);
        checkGLcall("glClearStencil");
        glMask = glMask | GL_STENCIL_BUFFER_BIT;
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
        glClearColor(((Color >> 16) & 0xFF) / 255.0f, 
		     ((Color >>  8) & 0xFF) / 255.0f,
                     ((Color >>  0) & 0xFF) / 255.0f, 
		     ((Color >> 24) & 0xFF) / 255.0f);
        checkGLcall("glClearColor");
        glMask = glMask | GL_COLOR_BUFFER_BIT;
    }

    /* Now process each rect in turn */
    for (i = 0; i < Count || i == 0; i++) {

        if (curRect) {
            /* Note gl uses lower left, width/height */
            TRACE("(%p) %p Rect=(%ld,%ld)->(%ld,%ld) glRect=(%ld,%ld), len=%ld, hei=%ld\n", This, curRect,
                  curRect->x1, curRect->y1, curRect->x2, curRect->y2,
                  curRect->x1, (This->PresentParms.BackBufferHeight - curRect->y2), 
                  curRect->x2 - curRect->x1, curRect->y2 - curRect->y1);
            glScissor(curRect->x1, (This->PresentParms.BackBufferHeight - curRect->y2), 
                      curRect->x2 - curRect->x1, curRect->y2 - curRect->y1);
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
    }

    if (Count > 0 && pRects) {
        glDisable(GL_SCISSOR_TEST);
        checkGLcall("glDisable");
    }
    LEAVE_GL();

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE d3dts, CONST D3DMATRIX* lpmatrix) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    D3DMATRIX m;
    int k;
    float f;
    BOOL viewChanged = TRUE;
    int  Stage;

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

    if (d3dts < 256) { 
      switch (d3dts) {
      case D3DTS_VIEW:
        conv_mat(lpmatrix, &This->StateBlock->transforms[D3DTS_VIEW]);
        break;

      case D3DTS_PROJECTION:
        conv_mat(lpmatrix, &This->StateBlock->transforms[D3DTS_PROJECTION]);
        break;

      case D3DTS_TEXTURE0:
      case D3DTS_TEXTURE1:
      case D3DTS_TEXTURE2:
      case D3DTS_TEXTURE3:
      case D3DTS_TEXTURE4:
      case D3DTS_TEXTURE5:
      case D3DTS_TEXTURE6:
      case D3DTS_TEXTURE7:
        conv_mat(lpmatrix, &This->StateBlock->transforms[d3dts]);
        break;

      default:
        FIXME("Unhandled transform state!!\n");
        break;
      }
    } else {
      /** 
       * Indexed Vertex Blending Matrices 256 -> 511 
       *  where WORLDMATRIX(0) == 256!
       */
      /** store it */
      conv_mat(lpmatrix, &This->StateBlock->transforms[d3dts]);
#if 0
      if (GL_SUPPORT(ARB_VERTEX_BLEND)) {
          FIXME("TODO\n");
      } else if (GL_SUPPORT(EXT_VERTEX_WEIGHTING)) {
          FIXME("TODO\n");
      }
#endif
    }

    /*
     * Move the GL operation to outside of switch to make it work
     * regardless of transform set order.
     */
    ENTER_GL();
    if (memcmp(&This->lastProj, &This->StateBlock->transforms[D3DTS_PROJECTION].u.m[0][0], sizeof(D3DMATRIX))) {
        glMatrixMode(GL_PROJECTION);
        checkGLcall("glMatrixMode");
        glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_PROJECTION].u.m[0][0]);
        checkGLcall("glLoadMatrixf");
        memcpy(&This->lastProj, &This->StateBlock->transforms[D3DTS_PROJECTION].u.m[0][0], sizeof(D3DMATRIX));
    } else {
        TRACE("Skipping as projection already correct\n");
    }

    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode");
    viewChanged = FALSE;
    if (memcmp(&This->lastView, &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0], sizeof(D3DMATRIX))) {
       glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0]);
       checkGLcall("glLoadMatrixf");
       memcpy(&This->lastView, &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0], sizeof(D3DMATRIX));
       viewChanged = TRUE;

       /* If we are changing the View matrix, reset the light and clipping planes to the new view */
       if (d3dts == D3DTS_VIEW) {

           /* NOTE: We have to reset the positions even if the light/plane is not currently
              enabled, since the call to enable it will not reset the position.             */

           /* Reset lights */
           for (k = 0; k < GL_LIMITS(lights); k++) {
               glLightfv(GL_LIGHT0 + k, GL_POSITION,       &This->lightPosn[k][0]);
               checkGLcall("glLightfv posn");
               glLightfv(GL_LIGHT0 + k, GL_SPOT_DIRECTION, &This->lightDirn[k][0]);
               checkGLcall("glLightfv dirn");
           }

           /* Reset Clipping Planes if clipping is enabled */
           for (k = 0; k < GL_LIMITS(clipplanes); k++) {
               glClipPlane(GL_CLIP_PLANE0 + k, This->StateBlock->clipplane[k]);
               checkGLcall("glClipPlane");
           }

           /* Reapply texture transforms as based off modelview when applied */
           for (Stage = 0; Stage < GL_LIMITS(textures); Stage++) {

               /* Only applicable if the transforms are not disabled */
               if (This->UpdateStateBlock->texture_state[Stage][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE) 
               {
                   /* Now apply texture transforms if not applying to the dummy textures */
#if defined(GL_VERSION_1_3)
                  glActiveTexture(GL_TEXTURE0 + Stage);
#else 
                  glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
#endif
                  checkGLcall("glActiveTexture(GL_TEXTURE0 + Stage);");

                  glMatrixMode(GL_TEXTURE);
                  if (This->StateBlock->textureDimensions[Stage] == GL_TEXTURE_1D) {
                     glLoadIdentity();
                  } else {
                     D3DMATRIX fred;
                     conv_mat(&This->StateBlock->transforms[D3DTS_TEXTURE0+Stage], &fred);
                     glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_TEXTURE0+Stage].u.m[0][0]);
                  }
               }
               checkGLcall("Load matrix for texture");
           }
           glMatrixMode(GL_MODELVIEW);  /* Always leave in model view */
       }
    } else if ((d3dts >= D3DTS_TEXTURE0 && d3dts <= D3DTS_TEXTURE7) &&
              (This->UpdateStateBlock->texture_state[d3dts - D3DTS_TEXTURE0][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE)) {
        /* Now apply texture transforms if not applying to the dummy textures */
        Stage = d3dts - D3DTS_TEXTURE0;

        if (memcmp(&This->lastTexTrans[Stage], &This->StateBlock->transforms[D3DTS_TEXTURE0 + Stage].u.m[0][0], sizeof(D3DMATRIX))) {
	   memcpy(&This->lastTexTrans[Stage], &This->StateBlock->transforms[D3DTS_TEXTURE0 + Stage].u.m[0][0], sizeof(D3DMATRIX));

#if defined(GL_VERSION_1_3)
           glActiveTexture(GL_TEXTURE0 + Stage);
#else
           glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
#endif
           checkGLcall("glActiveTexture(GL_TEXTURE0 + Stage)");

           glMatrixMode(GL_TEXTURE);
           if (This->StateBlock->textureDimensions[Stage] == GL_TEXTURE_1D) {
              glLoadIdentity();
           } else {
              D3DMATRIX fred;
              conv_mat(&This->StateBlock->transforms[D3DTS_TEXTURE0+Stage], &fred);
              glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_TEXTURE0+Stage].u.m[0][0]);
           }
           checkGLcall("Load matrix for texture");
           glMatrixMode(GL_MODELVIEW);  /* Always leave in model view */
        } else {
            TRACE("Skipping texture transform as already correct\n");
        }

    } else {
        TRACE("Skipping view setup as view already correct\n");
    }

    /**
     * Vertex Blending as described
     *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/enums/d3dvertexblendflags.asp
     */
    switch (This->UpdateStateBlock->vertex_blend) {
    case D3DVBF_DISABLE:
      {
          if (viewChanged == TRUE || 
              (memcmp(&This->lastWorld0, &This->StateBlock->transforms[D3DTS_WORLDMATRIX(0)].u.m[0][0], sizeof(D3DMATRIX)))) {
               memcpy(&This->lastWorld0, &This->StateBlock->transforms[D3DTS_WORLDMATRIX(0)].u.m[0][0], sizeof(D3DMATRIX));
              if (viewChanged == FALSE) {
                 glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0]);
                 checkGLcall("glLoadMatrixf");
              }
              glMultMatrixf((float *) &This->StateBlock->transforms[D3DTS_WORLDMATRIX(0)].u.m[0][0]);
              checkGLcall("glMultMatrixf");
          } else {
              TRACE("Skipping as world already correct\n");
          }
      }
      break;
    case D3DVBF_1WEIGHTS:
    case D3DVBF_2WEIGHTS:
    case D3DVBF_3WEIGHTS:
      {
          FIXME("valid/correct D3DVBF_[1..3]WEIGHTS\n");
          /*
           * doc seems to say that the weight values must be in vertex data (specified in FVF by D3DFVF_XYZB*)
           * so waiting for the values before matrix work
          for (k = 0; k < This->UpdateStateBlock->vertex_blend; ++k) {
            glMultMatrixf((float *) &This->StateBlock->transforms[D3DTS_WORLDMATRIX(k)].u.m[0][0]);
            checkGLcall("glMultMatrixf");
          }
          */
      }
      break;
    case D3DVBF_TWEENING:
      {
          FIXME("valid/correct D3DVBF_TWEENING\n");
          f = This->UpdateStateBlock->tween_factor;
          m.u.s._11 = f; m.u.s._12 = f; m.u.s._13 = f; m.u.s._14 = f;
          m.u.s._21 = f; m.u.s._22 = f; m.u.s._23 = f; m.u.s._24 = f;
          m.u.s._31 = f; m.u.s._32 = f; m.u.s._33 = f; m.u.s._34 = f;
          m.u.s._41 = f; m.u.s._42 = f; m.u.s._43 = f; m.u.s._44 = f;
          if (viewChanged == FALSE) {
              glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0]);
              checkGLcall("glLoadMatrixf");
          }
          glMultMatrixf((float *) &m.u.m[0][0]);
          checkGLcall("glMultMatrixf");
      }
      break;
    case D3DVBF_0WEIGHTS:
      {
          FIXME("valid/correct D3DVBF_0WEIGHTS\n");
          /* single matrix of weight 1.0f */
          m.u.s._11 = 1.0f; m.u.s._12 = 1.0f; m.u.s._13 = 1.0f; m.u.s._14 = 1.0f;
          m.u.s._21 = 1.0f; m.u.s._22 = 1.0f; m.u.s._23 = 1.0f; m.u.s._24 = 1.0f;
          m.u.s._31 = 1.0f; m.u.s._32 = 1.0f; m.u.s._33 = 1.0f; m.u.s._34 = 1.0f;
          m.u.s._41 = 1.0f; m.u.s._42 = 1.0f; m.u.s._43 = 1.0f; m.u.s._44 = 1.0f;
          if (viewChanged == FALSE) {
              glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0]);
              checkGLcall("glLoadMatrixf");
          }
          glMultMatrixf((float *) &m.u.m[0][0]);
          checkGLcall("glMultMatrixf");
      }
      break;
    default:
      break; /* stupid compilator */
    }

    LEAVE_GL();

    return D3D_OK;

}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
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
    /* Note: GL requires lower left, DirectX supplies upper left */
    glViewport(pViewport->X, (This->PresentParms.BackBufferHeight - (pViewport->Y + pViewport->Height)), 
               pViewport->Width, pViewport->Height);
    checkGLcall("glViewport");


    return D3D_OK;

}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetViewport(LPDIRECT3DDEVICE8 iface, D3DVIEWPORT8* pViewport) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p)\n", This);
    memcpy(pViewport, &This->StateBlock->viewport, sizeof(D3DVIEWPORT8));
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

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (float*) &This->UpdateStateBlock->material.Ambient);
    checkGLcall("glMaterialfv");
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (float*) &This->UpdateStateBlock->material.Diffuse);
    checkGLcall("glMaterialfv");

    /* Only change material color if specular is enabled, otherwise it is set to black */
    if (This->StateBlock->renderstate[D3DRS_SPECULARENABLE]) {
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &This->UpdateStateBlock->material.Specular);
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    memcpy(pMaterial, &This->UpdateStateBlock->material, sizeof (D3DMATERIAL8));
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g, pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g, pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g, pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g, pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetLight(LPDIRECT3DDEVICE8 iface, DWORD Index, CONST D3DLIGHT8* pLight) {
    float colRGBA[] = {0.0, 0.0, 0.0, 0.0};
    float rho;
    float quad_att;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Idx(%ld), pLight(%p)\n", This, Index, pLight);

    if (Index >= GL_LIMITS(lights)) {
        TRACE("Cannot handle more lights than device supports\n");
        return D3DERR_INVALIDCALL;
    }

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
    glLoadMatrixf((float *) &This->StateBlock->transforms[D3DTS_VIEW].u.m[0][0]);

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
    
    if (Index >= GL_LIMITS(lights)) {
        TRACE("Cannot handle more lights than device supports\n");
        return D3DERR_INVALIDCALL;
    }

    memcpy(pLight, &This->StateBlock->lights[Index], sizeof(D3DLIGHT8));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_LightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL Enable) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : Idx(%ld), enable? %d\n", This, Index, Enable);

    if (Index >= GL_LIMITS(lights)) {
        TRACE("Cannot handle more lights than device supports\n");
        return D3DERR_INVALIDCALL;
    }

    This->UpdateStateBlock->Changed.lightEnable[Index] = TRUE;
    This->UpdateStateBlock->Set.lightEnable[Index] = TRUE;
    This->UpdateStateBlock->lightEnable[Index] = Enable;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    if (Enable) {
        glEnable(GL_LIGHT0 + Index);
        checkGLcall("glEnable GL_LIGHT0+Index");
    } else {
        glDisable(GL_LIGHT0 + Index);
        checkGLcall("glDisable GL_LIGHT0+Index");
    }
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetLightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL* pEnable) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : for idx(%ld)\n", This, Index);

    if (Index >= GL_LIMITS(lights)) {
        TRACE("Cannot handle more lights than device supports\n");
        return D3DERR_INVALIDCALL;
    }

    *pEnable = This->StateBlock->lightEnable[Index];
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST float* pPlane) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : for idx %ld, %p\n", This, Index, pPlane);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesnt support\n");
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

    glPopMatrix();
    checkGLcall("glClipPlane");

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,float* pPlane) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : for idx %ld\n", This, Index);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesnt support\n");
        return D3DERR_INVALIDCALL;
    }

    pPlane[0] = This->StateBlock->clipplane[Index][0];
    pPlane[1] = This->StateBlock->clipplane[Index][1];
    pPlane[2] = This->StateBlock->clipplane[Index][2];
    pPlane[3] = This->StateBlock->clipplane[Index][3];
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD Value) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    DWORD OldValue = This->StateBlock->renderstate[State];
        
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
            int i;

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
                    FIXME("Program using multiple concurrent textures which this opengl implementation doesnt support\n");
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
               Seperate specular color means the specular colour is maintained seperately, whereas
               single color means it is merged in. However in both cases they are being used to
               some extent.
               To disable specular color, set it explicitly to black and turn off GL_COLOR_SUM_EXT
               NOTE: If not supported dont give FIXME as very minimal impact and very few people are
                  yet running 1.4!
             */
              if (Value) {
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (float*) &This->UpdateStateBlock->material.Specular);
                checkGLcall("glMaterialfv");
		if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
		  glEnable(GL_COLOR_SUM_EXT);
		} else {
		  TRACE("Specular colors cannot be enabled in this version of opengl\n");
		}
                checkGLcall("glEnable(GL_COLOR_SUM)\n");
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
                checkGLcall("glDisable(GL_COLOR_SUM)\n");
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
            TRACE("glStencilMask(%lu)\n", Value);
            checkGLcall("glStencilMask");
        }
        break;

    case D3DRS_FOGENABLE                 :
        {
            if (Value && This->StateBlock->renderstate[D3DRS_FOGTABLEMODE] != D3DFOG_NONE) {
               glEnable(GL_FOG);
               checkGLcall("glEnable GL_FOG\n");
            } else {
               glDisable(GL_FOG);
               checkGLcall("glDisable GL_FOG\n");
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
	  switch (Value) {
	  case D3DFOG_NONE:    /* I don't know what to do here */ break;
	  case D3DFOG_EXP:     glFogi(GL_FOG_MODE, GL_EXP); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP"); break; 
	  case D3DFOG_EXP2:    glFogi(GL_FOG_MODE, GL_EXP2); checkGLcall("glFogi(GL_FOG_MODE, GL_EXP2"); break; 
	  case D3DFOG_LINEAR:  glFogi(GL_FOG_MODE, GL_LINEAR); checkGLcall("glFogi(GL_FOG_MODE, GL_LINEAR"); break; 
	  default:
	    FIXME("Unsupported Value(%lu) for D3DRS_FOGTABLEMODE!\n", Value);
	  }
        }
	break;

    case D3DRS_FOGSTART                  :
        {
            float *f = (float*) &Value;
            glFogfv(GL_FOG_START, f);
            checkGLcall("glFogf(GL_FOG_START, (float) Value)");
            TRACE("Fog Start == %f\n", *f);
        }
        break;

    case D3DRS_FOGEND                    :
        {
            float *f = (float*) &Value;
            glFogfv(GL_FOG_END, f);
            checkGLcall("glFogf(GL_FOG_END, (float) Value)");
            TRACE("Fog End == %f\n", *f);
        }
        break;

    case D3DRS_FOGDENSITY                :
        {
            float *f = (float*) &Value;
            glFogfv(GL_FOG_DENSITY, f);
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
	  This->UpdateStateBlock->tween_factor = *((float*) &Value);
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
                glEnable(GL_COLOR_MATERIAL);
                checkGLcall("glEnable GL_GL_COLOR_MATERIAL\n");

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
                    glDisable(GL_COLOR_MATERIAL);
                    checkGLcall("glDisable GL_GL_COLOR_MATERIAL\n");
                } else {
                    TRACE("glColorMaterial Parm=%d\n", Parm);
                    glColorMaterial(GL_FRONT_AND_BACK, Parm);
                    checkGLcall("glColorMaterial(GL_FRONT_AND_BACK, Parm)\n");
                }

            } else {
                glDisable(GL_COLOR_MATERIAL);
                checkGLcall("glDisable GL_GL_COLOR_MATERIAL\n");
            }
        }
        break; 

    case D3DRS_LINEPATTERN               :
        {
            D3DLINEPATTERN *pattern = (D3DLINEPATTERN *)&Value;
            TRACE("Line pattern: repeat %d bits %x\n", pattern->wRepeatFactor, pattern->wLinePattern);

            if (pattern->wRepeatFactor) {
                glLineStipple(pattern->wRepeatFactor, pattern->wLinePattern);
                checkGLcall("glLineStipple(repeat, linepattern)\n");
                glEnable(GL_LINE_STIPPLE);
                checkGLcall("glEnable(GL_LINE_STIPPLE);\n");
            } else {
                glDisable(GL_LINE_STIPPLE);
                checkGLcall("glDisable(GL_LINE_STIPPLE);\n");
            }
        }
        break;

    case D3DRS_ZBIAS                     :
        {
            if (Value) {
                TRACE("ZBias value %f\n", *((float*)&Value));
                glPolygonOffset(0, -*((float*)&Value));
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
        TRACE("Set point size to %f\n", *((float*)&Value));
        glPointSize(*((float*)&Value));
        checkGLcall("glPointSize(...);\n");
        break;

    case D3DRS_POINTSIZE_MIN             :
        if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
	  GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MIN_EXT, *((float*)&Value));
	  checkGLcall("glPointParameterfEXT(...);\n");
	} else {
	  FIXME("D3DRS_POINTSIZE_MIN not supported on this opengl\n");
	}
        break;

    case D3DRS_POINTSIZE_MAX             :
        if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
	  GL_EXTCALL(glPointParameterfEXT)(GL_POINT_SIZE_MAX_EXT, *((float*)&Value));
	  checkGLcall("glPointParameterfEXT(...);\n");
	} else {
	  FIXME("D3DRS_POINTSIZE_MAX not supported on this opengl\n");
	}
        break;

    case D3DRS_POINTSCALE_A              :
    case D3DRS_POINTSCALE_B              :
    case D3DRS_POINTSCALE_C              :
    case D3DRS_POINTSCALEENABLE          :
        {
            /* If enabled, supply the parameters, otherwise fall back to defaults */
            if (This->StateBlock->renderstate[D3DRS_POINTSCALEENABLE]) {
                GLfloat att[3] = {1.0f, 0.0f, 0.0f};
                att[0] = *((float*)&This->StateBlock->renderstate[D3DRS_POINTSCALE_A]);
                att[1] = *((float*)&This->StateBlock->renderstate[D3DRS_POINTSCALE_B]);
                att[2] = *((float*)&This->StateBlock->renderstate[D3DRS_POINTSCALE_C]);

		if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
                  GL_EXTCALL(glPointParameterfvEXT)(GL_DISTANCE_ATTENUATION_EXT, att);
		  checkGLcall("glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, ...);\n");
		} else {
		  TRACE("D3DRS_POINTSCALEENABLE not supported on this opengl\n");
		}
            } else {
                GLfloat att[3] = {1.0f, 0.0f, 0.0f};
		if (GL_SUPPORT(EXT_POINT_PARAMETERS)) {
		  GL_EXTCALL(glPointParameterfvEXT)(GL_DISTANCE_ATTENUATION_EXT, att);
		  checkGLcall("glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, ...);\n");
		} else {
		  TRACE("D3DRS_POINTSCALEENABLE not supported, but not on either\n");
		}
	    }
            break;
        }

    case D3DRS_COLORWRITEENABLE          :
        TRACE("Color mask: r(%d) g(%d) b(%d) a(%d)\n", 
	      Value & D3DCOLORWRITEENABLE_RED   ? 1 : 0,
	      Value & D3DCOLORWRITEENABLE_GREEN ? 1 : 0,
	      Value & D3DCOLORWRITEENABLE_BLUE  ? 1 : 0,
	      Value & D3DCOLORWRITEENABLE_ALPHA ? 1 : 0); 
	glColorMask(Value & D3DCOLORWRITEENABLE_RED, 
                    Value & D3DCOLORWRITEENABLE_GREEN,
		    Value & D3DCOLORWRITEENABLE_BLUE, 
                    Value & D3DCOLORWRITEENABLE_ALPHA);
        checkGLcall("glColorMask(...)\n");
		break;

        /* Unhandled yet...! */
    case D3DRS_LASTPIXEL                 :
    case D3DRS_ZVISIBLE                  :
    case D3DRS_EDGEANTIALIAS             :
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
    case D3DRS_LOCALVIEWER               :
    case D3DRS_SOFTWAREVERTEXPROCESSING  :
    case D3DRS_POINTSPRITEENABLE         :
    case D3DRS_MULTISAMPLEANTIALIAS      :
    case D3DRS_MULTISAMPLEMASK           :
    case D3DRS_PATCHEDGESTYLE            :
    case D3DRS_PATCHSEGMENTS             :
    case D3DRS_DEBUGMONITORTOKEN         :
    case D3DRS_POSITIONORDER             :
    case D3DRS_NORMALORDER               :
        /*Put back later: FIXME("(%p)->(%d,%ld) not handled yet\n", This, State, Value); */
        TRACE("(%p)->(%d,%ld) not handled yet\n", This, State, Value);
        break;
    default:
        FIXME("(%p)->(%d,%ld) unrecognized\n", This, State, Value);
    }

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD* pValue) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) for State %d = %ld\n", This, State, This->UpdateStateBlock->renderstate[State]);
    *pValue = This->StateBlock->renderstate[State];
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_BeginStateBlock(LPDIRECT3DDEVICE8 iface) {
  ICOM_THIS(IDirect3DDevice8Impl,iface);
  
  TRACE("(%p)\n", This);
  
  return IDirect3DDeviceImpl_BeginStateBlock(This);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_EndStateBlock(LPDIRECT3DDEVICE8 iface, DWORD* pToken) {
  IDirect3DStateBlockImpl* pSB;
  ICOM_THIS(IDirect3DDevice8Impl,iface);
  HRESULT res;

  TRACE("(%p)\n", This);

  res = IDirect3DDeviceImpl_EndStateBlock(This, &pSB);
  *pToken = (DWORD) pSB;
  return res;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_ApplyStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
  IDirect3DStateBlockImpl* pSB = (IDirect3DStateBlockImpl*) Token;
  ICOM_THIS(IDirect3DDevice8Impl,iface);

  TRACE("(%p)\n", This);

  return IDirect3DDeviceImpl_ApplyStateBlock(This, pSB);

}
HRESULT  WINAPI  IDirect3DDevice8Impl_CaptureStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
  IDirect3DStateBlockImpl* pSB = (IDirect3DStateBlockImpl*) Token;
  ICOM_THIS(IDirect3DDevice8Impl,iface);

  TRACE("(%p)\n", This);

  return IDirect3DDeviceImpl_CaptureStateBlock(This, pSB);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
  IDirect3DStateBlockImpl* pSB = (IDirect3DStateBlockImpl*) Token;
  ICOM_THIS(IDirect3DDevice8Impl,iface);

  TRACE("(%p)\n", This);

  return IDirect3DDeviceImpl_DeleteStateBlock(This, pSB);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_CreateStateBlock(LPDIRECT3DDEVICE8 iface, D3DSTATEBLOCKTYPE Type, DWORD* pToken) {
  IDirect3DStateBlockImpl* pSB;
  ICOM_THIS(IDirect3DDevice8Impl,iface);
  HRESULT res;

  TRACE("(%p) : for type %d\n", This, Type);

  res = IDirect3DDeviceImpl_CreateStateBlock(This, Type, &pSB);
  *pToken = (DWORD) pSB;
  return res;
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
    TRACE("(%p) : returning %p for stage %ld\n", This, This->UpdateStateBlock->textures[Stage], Stage);
    *ppTexture = (LPDIRECT3DBASETEXTURE8) This->UpdateStateBlock->textures[Stage];
    IDirect3DBaseTexture8Impl_AddRef(*ppTexture);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage, IDirect3DBaseTexture8* pTexture) {

    IDirect3DBaseTexture8 *oldTxt;
    BOOL reapplyStates = TRUE;

    ICOM_THIS(IDirect3DDevice8Impl,iface);
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

    /* Make appropriate texture active */
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
#if defined(GL_VERSION_1_3)
        glActiveTexture(GL_TEXTURE0 + Stage);
#else
        glActiveTextureARB(GL_TEXTURE0_ARB + Stage);
#endif
        checkGLcall("glActiveTextureARB");
    } else if (Stage>0) {
        FIXME("Program using multiple concurrent textures which this opengl implementation doesnt support\n");
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
          if (oldTxt == pTexture && TRUE == IDirect3DBaseTexture8Impl_IsDirty(pTexture)) {
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
	  if (oldTxt == pTexture && TRUE == IDirect3DBaseTexture8Impl_IsDirty(pTexture)) {
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
	  if (oldTxt == pTexture && TRUE == IDirect3DBaseTexture8Impl_IsDirty(pTexture)) {
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

    /* Even if the texture has been set to null, reapply the stages as a null texture to directx requires
       a dummy texture in opengl, and we always need to ensure the current view of the TextureStates apply */
    if (reapplyStates) {
       setupTextureStates(iface, Stage);
    }
       
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : requesting Stage %ld, Type %d getting %ld\n", This, Stage, Type, This->UpdateStateBlock->texture_state[Stage][Type]);
    *pValue = This->UpdateStateBlock->texture_state[Stage][Type];
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    /* FIXME: Handle 3d textures? What if TSS value set before set texture? Need to reapply all values? */
   
    TRACE("(%p) : stub, Stage=%ld, Type=%d, Value=%ld\n", This, Stage, Type, Value);

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
        FIXME("Program using multiple concurrent textures which this opengl implementation doesnt support\n");
    }

    switch (Type) {

    case D3DTSS_MINFILTER             :
    case D3DTSS_MIPFILTER             :
        {
            DWORD ValueMIN = This->StateBlock->texture_state[Stage][D3DTSS_MINFILTER];
            DWORD ValueMIP = This->StateBlock->texture_state[Stage][D3DTSS_MIPFILTER];
            GLint realVal = GL_LINEAR;

            if (ValueMIN == D3DTEXF_POINT) {
                /* GL_NEAREST_* */
                if (ValueMIP == D3DTEXF_POINT) {
                    realVal = GL_NEAREST_MIPMAP_NEAREST;
                } else if (ValueMIP == D3DTEXF_LINEAR) {
                    realVal = GL_NEAREST_MIPMAP_LINEAR;
                } else if (ValueMIP == D3DTEXF_NONE) {
                    realVal = GL_NEAREST;
                } else {
                    FIXME("Unhandled D3DTSS_MIPFILTER value of %ld\n", ValueMIP);
                    realVal = GL_NEAREST_MIPMAP_LINEAR;
                }
            } else if (ValueMIN == D3DTEXF_LINEAR) {
                /* GL_LINEAR_* */
                if (ValueMIP == D3DTEXF_POINT) {
                    realVal = GL_LINEAR_MIPMAP_NEAREST;
                } else if (ValueMIP == D3DTEXF_LINEAR) {
                    realVal = GL_LINEAR_MIPMAP_LINEAR;
                } else if (ValueMIP == D3DTEXF_NONE) {
                    realVal = GL_LINEAR;
                } else {
                    FIXME("Unhandled D3DTSS_MIPFILTER value of %ld\n", ValueMIP);
                    realVal = GL_LINEAR_MIPMAP_LINEAR;
                }
            } else if (ValueMIN == D3DTEXF_NONE) {
                /* Doesnt really make sense - Windows just seems to disable
                   mipmapping when this occurs                              */
                FIXME("Odd - minfilter of none, just disabling mipmaps\n");
                realVal = GL_LINEAR;

            } else {
                FIXME("Unhandled D3DTSS_MINFILTER value of %ld\n", ValueMIN);
                realVal = GL_LINEAR_MIPMAP_LINEAR;
            }

            TRACE("ValueMIN=%ld, ValueMIP=%ld, setting MINFILTER to %x\n", ValueMIN, ValueMIP, realVal);
            glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_MIN_FILTER, realVal);
            checkGLcall("glTexParameter GL_TEXTURE_MINFILTER, ...");
        }
        break;

    case D3DTSS_MAXANISOTROPY         :
      {	
	if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
	  glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_MAX_ANISOTROPY_EXT, This->StateBlock->texture_state[Stage][D3DTSS_MAXANISOTROPY]);
	  checkGLcall("glTexParameteri GL_TEXTURE_MAX_ANISOTROPY_EXT ...");
	}
      }
      break;

    case D3DTSS_MAGFILTER             :
        if (Value == D3DTEXF_POINT) {
            glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            checkGLcall("glTexParameter GL_TEXTURE_MAGFILTER, GL_NEAREST");
        } else if (Value == D3DTEXF_LINEAR) {
            glTexParameteri(This->StateBlock->textureDimensions[Stage], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            checkGLcall("glTexParameter GL_TEXTURE_MAGFILTER, GL_LINEAR");
        } else {
            FIXME("Unhandled D3DTSS_MAGFILTER value of %ld\n", Value);
        }
        break;

    case D3DTSS_ALPHAOP               :
    case D3DTSS_COLOROP               :
        {

            if (Value == D3DTOP_DISABLE) {
                /* TODO: Disable by making this and all later levels disabled */
                if (Type == D3DTSS_COLOROP) {
                    glDisable(GL_TEXTURE_1D);
                    checkGLcall("Disable GL_TEXTURE_1D");
                    glDisable(GL_TEXTURE_2D);
                    checkGLcall("Disable GL_TEXTURE_2D");
                    glDisable(GL_TEXTURE_3D);
                    checkGLcall("Disable GL_TEXTURE_3D");
                }
                break; /* Dont bother setting the texture operations */
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
                        glEnable(GL_TEXTURE_2D);
                        checkGLcall("Enable GL_TEXTURE_2D");
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
            case D3DTADDRESS_BORDER: wrapParm = GL_REPEAT; break;      /* FIXME: Not right, but better */
#if defined(GL_VERSION_1_4)
            case D3DTADDRESS_MIRROR: wrapParm = GL_MIRRORED_REPEAT; break;
#elif defined(GL_ARB_texture_mirrored_repeat)
            case D3DTADDRESS_MIRROR: wrapParm = GL_MIRRORED_REPEAT_ARB; break;
#else
            case D3DTADDRESS_MIRROR:      /* Unsupported in OpenGL pre-1.4 */
#endif
            case D3DTADDRESS_MIRRORONCE:  /* Unsupported in OpenGL         */
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

#if defined(GL_ARB_texture_cube_map) || defined(GL_NV_texgen_reflection)
	    case D3DTSS_TCI_CAMERASPACENORMAL:
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

		glEnable(GL_TEXTURE_GEN_S);
		checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
                checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB)");
		glEnable(GL_TEXTURE_GEN_T);
		checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
                checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB)");
		glEnable(GL_TEXTURE_GEN_R);
		checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
                checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB)");
	      }
	      break;
#endif

#if defined(GL_ARB_texture_cube_map) || defined(GL_NV_texgen_reflection)
	    case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
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
		
		glEnable(GL_TEXTURE_GEN_S);
		checkGLcall("glEnable(GL_TEXTURE_GEN_S);");
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                checkGLcall("glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB)");
		glEnable(GL_TEXTURE_GEN_T);
		checkGLcall("glEnable(GL_TEXTURE_GEN_T);");
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                checkGLcall("glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB)");
		glEnable(GL_TEXTURE_GEN_R);
		checkGLcall("glEnable(GL_TEXTURE_GEN_R);");
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                checkGLcall("glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB)");
	      }
	      break;
#endif

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
    case D3DTSS_BUMPENVMAT00          :
    case D3DTSS_BUMPENVMAT01          :
        TRACE("BUMPENVMAT0%u Still a stub, Stage=%ld, Type=%d, Value =%ld\n", Type - D3DTSS_BUMPENVMAT00, Stage, Type, Value);
        break;
    case D3DTSS_BUMPENVMAT10          :
    case D3DTSS_BUMPENVMAT11          :
        TRACE("BUMPENVMAT1%u Still a stub, Stage=%ld, Type=%d, Value =%ld\n", Type - D3DTSS_BUMPENVMAT10, Stage, Type, Value);
        break;

    case D3DTSS_TEXTURETRANSFORMFLAGS :
        {
           switch (Value & ~D3DTTFF_PROJECTED)
           {
           case D3DTTFF_DISABLE: /* Disable transform matrix for this texture by setting up the identity matrix */
               glMatrixMode(GL_TEXTURE);
               glLoadIdentity();
               checkGLcall("Load identity matrix for texture");
               glMatrixMode(GL_MODELVIEW);  /* Always leave in model view */
               break;

           default: /* Enable it */
               IDirect3DDevice8Impl_SetTransform(iface, D3DTS_TEXTURE0+Stage, &This->UpdateStateBlock->transforms[D3DTS_TEXTURE0+Stage]);
               break;
           }

           /* From web: <quote source="opengl12.pdf" section="Apendix C, Version 1.1/Other changes"
                2. Texture coordinates s, t, and r are divided by q during the rasterization
                of points, pixel rectangles, and bitmaps. This division was documented
                only for lines and polygons in the 1.0 version. </quote>                            
              I interpret this as we can implement projected transforms in slow vertex mode
                by moving the last coord to the 'q' coord and using one less dimension. The only
                way to do it in TexCoordPtr would be to massage the data stream to insert extra
                coords                                                                              */
        }
        break; 

    case D3DTSS_MIPMAPLODBIAS         :
    case D3DTSS_MAXMIPLEVEL           :
    case D3DTSS_BUMPENVLSCALE         :
    case D3DTSS_BUMPENVLOFFSET        :
    case D3DTSS_RESULTARG             :
    default:
        /* Put back later: FIXME("(%p) : stub, Stage=%ld, Type=%d, Value =%ld\n", This, Stage, Type, Value); */
        TRACE("Still a stub, Stage=%ld, Type=%d, Value =%ld\n", Stage, Type, Value);
    }
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_ValidateDevice(LPDIRECT3DDEVICE8 iface, DWORD* pNumPasses) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : stub\n", This);    /* FIXME: Needs doing, but called often and is harmless */
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetInfo(LPDIRECT3DDEVICE8 iface, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : setting p[%u] <= RGBA(%02x,%02x,%02x,%02x)\n", This, PaletteNumber,
          pEntries->peRed, pEntries->peGreen, pEntries->peBlue, pEntries->peFlags);
    memcpy(This->palettes[PaletteNumber], pEntries, 256 * sizeof(PALETTEENTRY));
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    memcpy(pEntries, This->palettes[PaletteNumber], 256 * sizeof(PALETTEENTRY));
    FIXME("(%p) : returning p[%u] => RGBA(%02x,%02x,%02x,%02x)\n", This, PaletteNumber,
          pEntries->peRed, pEntries->peGreen, pEntries->peBlue, pEntries->peFlags);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : Setting to (%u)\n", This, PaletteNumber);
    This->currentPalette = PaletteNumber;
#if defined(GL_EXT_paletted_texture)
    if (GL_SUPPORT(EXT_PALETTED_TEXTURE)) {
      GL_EXTCALL(glColorTableEXT)(GL_TEXTURE_2D,    /* target */
				  GL_RGBA,          /* internal format */
				  256,              /* table size */
				  GL_RGBA,          /* table format */
				  GL_UNSIGNED_BYTE, /* table type */
				  This->palettes[PaletteNumber]);
      checkGLcall("glColorTableEXT");
    } else {
      /* Delayed palette handling ... waiting for software emulation into preload code */
    }
#endif
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT *PaletteNumber) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    *PaletteNumber = This->currentPalette;
    FIXME("(%p) : Returning (%u)\n", This, *PaletteNumber);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {

    ICOM_THIS(IDirect3DDevice8Impl,iface);
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

    ICOM_THIS(IDirect3DDevice8Impl,iface);
    pIB = This->StateBlock->pIndexData;
    This->StateBlock->streamIsUP = FALSE;

    TRACE("(%p) : Type=(%d,%s), min=%d, CountV=%d, startIdx=%d, countP=%d \n", This, 
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);

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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    IDirect3DVertexShaderImpl* object;
    IDirect3DVertexShaderDeclarationImpl* attached_decl;
    HRESULT res;
    UINT i;

    TRACE_(d3d_shader)("(%p) : VertexShader not fully supported yet : Decl=%p, Func=%p\n", This, pDeclaration, pFunction);    
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);

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
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE_(d3d_shader)("(%p) : GetVertexShader returning %ld\n", This, This->StateBlock->VertexShader);
    *pHandle = This->StateBlock->VertexShader;
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteVertexShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
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
    if (NULL != object->function) HeapFree(GetProcessHeap(), 0, (void *)object->function);
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
  ICOM_THIS(IDirect3DDevice8Impl,iface);

  if (Register + ConstantCount > D3D8_VSHADER_MAX_CONSTANTS) {
    ERR_(d3d_shader)("(%p) : SetVertexShaderConstant C[%lu] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (ConstantCount > 1) {
    FLOAT* f = (FLOAT*)pConstantData;
    UINT i;
    TRACE_(d3d_shader)("(%p) : SetVertexShaderConstant C[%lu..%lu]=\n", This, Register, Register + ConstantCount - 1);
    for (i = 0; i < ConstantCount; ++i) {
      TRACE_(d3d_shader)("{%f, %f, %f, %f}\n", f[0], f[1], f[2], f[3]);
      f += 4;
    }
  } else { 
    FLOAT* f = (FLOAT*) pConstantData;
    TRACE_(d3d_shader)("(%p) : SetVertexShaderConstant, C[%lu]={%f, %f, %f, %f}\n", This, Register, f[0], f[1], f[2], f[3]);
  }
  This->UpdateStateBlock->Changed.vertexShaderConstant = TRUE;
  memcpy(&This->UpdateStateBlock->vertexShaderConstant[Register], pConstantData, ConstantCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
  ICOM_THIS(IDirect3DDevice8Impl,iface);

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
  /*ICOM_THIS(IDirect3DDevice8Impl,iface);*/
  IDirect3DVertexShaderDeclarationImpl* attached_decl;
  
  attached_decl = VERTEX_SHADER_DECL(Handle);
  if (NULL == attached_decl) {
    return D3DERR_INVALIDCALL;
  }
  return IDirect3DVertexShaderDeclarationImpl_GetDeclaration8(attached_decl, pData, (UINT*) pSizeOfData);
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD Handle, void* pData, DWORD* pSizeOfData) {
  /*ICOM_THIS(IDirect3DDevice8Impl,iface);*/
  IDirect3DVertexShaderImpl* object;
  
  object = VERTEX_SHADER(Handle);
  if (NULL == object) {
    return D3DERR_INVALIDCALL;
  }
  return IDirect3DVertexShaderImpl_GetFunction(object, pData, (UINT*) pSizeOfData);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
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

    if (oldIdxs) IDirect3DIndexBuffer8Impl_Release(oldIdxs);
    if (pIndexData) IDirect3DIndexBuffer8Impl_AddRef(This->StateBlock->pIndexData);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8** ppIndexData,UINT* pBaseVertexIndex) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    FIXME("(%p) : stub\n", This);

    *ppIndexData = This->StateBlock->pIndexData;
    /* up ref count on ppindexdata */
    if (*ppIndexData) IDirect3DIndexBuffer8Impl_AddRef(*ppIndexData);
    *pBaseVertexIndex = This->StateBlock->baseVertexIndex;

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_CreatePixelShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pFunction, DWORD* pHandle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
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
    ICOM_THIS(IDirect3DDevice8Impl,iface);

    This->UpdateStateBlock->PixelShader = Handle;
    This->UpdateStateBlock->Changed.pixelShader = TRUE;
    This->UpdateStateBlock->Set.pixelShader = TRUE;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE_(d3d_shader)("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* FIXME: Quieten when not being used */
    if (Handle != 0) {
      FIXME_(d3d_shader)("(%p) : stub %ld\n", This, Handle);
    } else {
      TRACE_(d3d_shader)("(%p) : stub %ld\n", This, Handle);
    }

    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD* pHandle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE_(d3d_shader)("(%p) : GetPixelShader returning %ld\n", This, This->StateBlock->PixelShader);
    *pHandle = This->StateBlock->PixelShader;
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_DeletePixelShader(LPDIRECT3DDEVICE8 iface, DWORD Handle) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    IDirect3DPixelShaderImpl* object;   

    if (Handle <= VS_HIGHESTFIXEDFXF) { /* only delete user defined shaders */
      return D3DERR_INVALIDCALL;
    }
    object = PixelShaders[Handle - VS_HIGHESTFIXEDFXF];
    TRACE_(d3d_shader)("(%p) : freeing PixelShader %p\n", This, object);
    /* TODO: check validity of object before free */
    if (NULL != object->function) HeapFree(GetProcessHeap(), 0, (void *)object->function);
    HeapFree(GetProcessHeap(), 0, (void *)object->data);
    HeapFree(GetProcessHeap(), 0, (void *)object);
    PixelShaders[Handle - VS_HIGHESTFIXEDFXF] = NULL;

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
  ICOM_THIS(IDirect3DDevice8Impl,iface);

  if (Register + ConstantCount > D3D8_PSHADER_MAX_CONSTANTS) {
    ERR_(d3d_shader)("(%p) : SetPixelShaderConstant C[%lu] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (ConstantCount > 1) {
    FLOAT* f = (FLOAT*)pConstantData;
    UINT i;
    TRACE_(d3d_shader)("(%p) : SetPixelShaderConstant C[%lu..%lu]=\n", This, Register, Register + ConstantCount - 1);
    for (i = 0; i < ConstantCount; ++i) {
      TRACE_(d3d_shader)("{%f, %f, %f, %f}\n", f[0], f[1], f[2], f[3]);
      f += 4;
    }
  } else { 
    FLOAT* f = (FLOAT*) pConstantData;
    TRACE_(d3d_shader)("(%p) : SetPixelShaderConstant, C[%lu]={%f, %f, %f, %f}\n", This, Register, f[0], f[1], f[2], f[3]);
  }
  This->UpdateStateBlock->Changed.pixelShaderConstant = TRUE;
  memcpy(&This->UpdateStateBlock->pixelShaderConstant[Register], pConstantData, ConstantCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
  ICOM_THIS(IDirect3DDevice8Impl,iface);

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

    if (oldSrc != NULL) IDirect3DVertexBuffer8Impl_Release(oldSrc);
    if (pStreamData != NULL) IDirect3DVertexBuffer8Impl_AddRef(pStreamData);
    return D3D_OK;
}
HRESULT  WINAPI  IDirect3DDevice8Impl_GetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8** pStream,UINT* pStride) {
    ICOM_THIS(IDirect3DDevice8Impl,iface);
    TRACE("(%p) : StreamNo: %d, Stream (%p), Stride %d\n", This, StreamNumber, This->StateBlock->stream_source[StreamNumber], This->StateBlock->stream_stride[StreamNumber]);
    *pStream = This->StateBlock->stream_source[StreamNumber];
    *pStride = This->StateBlock->stream_stride[StreamNumber];
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

HRESULT WINAPI IDirect3DDevice8Impl_CleanRender(LPDIRECT3DDEVICE8 iface) {
  ICOM_THIS(IDirect3DDevice8Impl,iface);
#if defined(GL_VERSION_1_3) /* @see comments on ActiveRender */
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
#endif
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice8Impl_ActiveRender(LPDIRECT3DDEVICE8 iface,
						IDirect3DSurface8* RenderSurface, 
						IDirect3DSurface8* StencilSurface) {

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
  D3DFORMAT BackBufferFormat = ((IDirect3DSurface8Impl*) RenderSurface)->myDesc.Format;
  D3DFORMAT StencilBufferFormat = (NULL != StencilSurface) ? ((IDirect3DSurface8Impl*) StencilSurface)->myDesc.Format : 0;
  UINT Width = ((IDirect3DSurface8Impl*) RenderSurface)->myDesc.Width;
  UINT Height = ((IDirect3DSurface8Impl*) RenderSurface)->myDesc.Height;
  IDirect3DSurface8Impl* tmp;

  ICOM_THIS(IDirect3DDevice8Impl,iface);
 
#define PUSH1(att)        attribs[nAttribs++] = (att); 
#define PUSH2(att,value)  attribs[nAttribs++] = (att); attribs[nAttribs++] = (value);

  PUSH2(GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_WINDOW | GLX_PBUFFER_BIT);
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
    PUSH2(GLX_DEPTH_SIZE,   32);
    break;

  default:
    break;
  }

  PUSH1(None);
  
  cfgs = glXChooseFBConfig(This->display, DefaultScreen(This->display), attribs, &nCfgs);
  if (NULL != cfgs) {
#if 0
    int i;
    for (i = 0; i < nCfgs; ++i) {
      TRACE("for (%u,%s)/(%u,%s) found config[%d]@%p\n", BackBufferFormat, debug_d3dformat(BackBufferFormat), StencilBufferFormat, debug_d3dformat(StencilBufferFormat), i, cfgs[i]);
    }
#endif

    if (NULL != This->renderTarget) {
      GLenum prev_read;      
      glFlush();
      vcheckGLcall("glFlush");

#if 0
      /** very very usefull debug code */
      glXSwapBuffers(This->display, This->drawable);   
      printf("Hit Enter to get next frame ...\n");
      getchar();
#endif

      glGetIntegerv(GL_READ_BUFFER, &prev_read);
      vcheckGLcall("glIntegerv");
      glReadBuffer(GL_BACK);
      vcheckGLcall("glReadBuffer");
      {
	long j;
	long pitch = This->renderTarget->myDesc.Width * This->renderTarget->bytesPerPixel;
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
    }

    if (BackBufferFormat != This->renderTarget->myDesc.Format && 
	StencilBufferFormat != This->stencilBufferTarget->myDesc.Format) {
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

    return D3D_OK;

  } else {
    ERR("cannot get valides GLXFBConfig for (%u,%s)/(%u,%s)\n", BackBufferFormat, debug_d3dformat(BackBufferFormat), StencilBufferFormat, debug_d3dformat(StencilBufferFormat));
  }

#undef PUSH1
#undef PUSH2

#endif

  return D3DERR_INVALIDCALL;
}
