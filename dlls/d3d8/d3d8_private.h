/*
 * Direct3D 8 private include file
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

#ifndef __WINE_D3DX8_PRIVATE_H
#define __WINE_D3DX8_PRIVATE_H

/* THIS FILE MUST NOT CONTAIN X11 or MESA DEFINES */
#include "config.h"
#define XMD_H /* This is to prevent the Xmd.h inclusion bug :-/ */
#include <GL/gl.h>
#include <GL/glx.h>
#ifdef HAVE_GL_GLEXT_H
# include <GL/glext.h>
#endif
#undef  XMD_H

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI

/* X11 locking */

extern void (*wine_tsx11_lock_ptr)(void);
extern void (*wine_tsx11_unlock_ptr)(void);

/* As GLX relies on X, this is needed */
#define ENTER_GL() wine_tsx11_lock_ptr()
#define LEAVE_GL() wine_tsx11_unlock_ptr()

#include "d3d8.h"

/* Device caps */
#define MAX_STREAMS       1
#define MAX_ACTIVE_LIGHTS 8
#define MAX_CLIPPLANES    D3DMAXUSERCLIPPLANES
#define MAX_LEVELS 256

/* Other useful values */
#define HIGHEST_RENDER_STATE 174
#define HIGHEST_TEXTURE_STATE 29
#define HIGHEST_TRANSFORMSTATE 512
#define D3DSBT_RECORDED 0xfffffffe

/* Direct38 Interfaces: */
typedef struct IDirect3DBaseTexture8Impl IDirect3DBaseTexture8Impl;
typedef struct IDirect3DVolumeTexture8Impl IDirect3DVolumeTexture8Impl;
typedef struct IDirect3D8Impl IDirect3D8Impl;
typedef struct IDirect3DDevice8Impl IDirect3DDevice8Impl;
typedef struct IDirect3DTexture8Impl IDirect3DTexture8Impl;
typedef struct IDirect3DCubeTexture8Impl IDirect3DCubeTexture8Impl;
typedef struct IDirect3DIndexBuffer8Impl IDirect3DIndexBuffer8Impl;
typedef struct IDirect3DSurface8Impl IDirect3DSurface8Impl;
typedef struct IDirect3DSwapChain8Impl IDirect3DSwapChain8Impl;
typedef struct IDirect3DResource8Impl IDirect3DResource8Impl;
typedef struct IDirect3DVolume8Impl IDirect3DVolume8Impl;
typedef struct IDirect3DVertexBuffer8Impl IDirect3DVertexBuffer8Impl;

/* State Block for Begin/End/Capture/Create/Apply State Block   */
/*   Note: Very long winded but I do not believe gl Lists will  */
/*   resolve everything we need, so doing it manually for now   */
typedef struct SAVEDSTATES {
        BOOL                      lightEnable[MAX_ACTIVE_LIGHTS];
        BOOL                      Indices;
        BOOL                      lights[MAX_ACTIVE_LIGHTS];
        BOOL                      material;
        BOOL                      stream_source[MAX_STREAMS];
        BOOL                      textures[8];
        BOOL                      transform[HIGHEST_TRANSFORMSTATE];
        BOOL                      viewport;
        BOOL                      vertexShader;
        BOOL                      pixelShader;
        BOOL                      renderstate[HIGHEST_RENDER_STATE];
        BOOL                      texture_state[8][HIGHEST_TEXTURE_STATE];
        BOOL                      clipplane[MAX_CLIPPLANES];
} SAVEDSTATES;

typedef struct STATEBLOCK {

    D3DSTATEBLOCKTYPE             blockType;

    SAVEDSTATES                   Changed;
    SAVEDSTATES                   Set;

    /* Light Enable */
    BOOL                      lightEnable[MAX_ACTIVE_LIGHTS];

    /* ClipPlane */
    double                    clipplane[MAX_CLIPPLANES][4];

    /* Indices */
    IDirect3DIndexBuffer8*    pIndexData;
    UINT                      baseVertexIndex;

    /* Lights */
    D3DLIGHT8                 lights[MAX_ACTIVE_LIGHTS];

    /* Material */
    D3DMATERIAL8              material;

    /* Pixel Shader */
    DWORD                     PixelShader;

    /* TODO: Pixel Shader Constant */

    /* RenderState */
    DWORD                     renderstate[HIGHEST_RENDER_STATE];

    /* Stream Source */
    UINT                      stream_stride[MAX_STREAMS];
    IDirect3DVertexBuffer8   *stream_source[MAX_STREAMS];

    /* Texture */
    IDirect3DBaseTexture8    *textures[8];

    /* Texture State Stage */
    DWORD                     texture_state[8][HIGHEST_TEXTURE_STATE];

    /* Transform */
    D3DMATRIX                 transforms[HIGHEST_TRANSFORMSTATE];

    /* ViewPort */
    D3DVIEWPORT8              viewport;

    /* Vertex Shader */
    DWORD                     VertexShader;


    /* TODO: Vertex Shader Constant */

} STATEBLOCK;

/*
 * External prototypes
 */
/*BOOL D3DRAW_HAL_Init(HINSTANCE, DWORD, LPVOID); */
void CreateStateBlock(LPDIRECT3DDEVICE8 iface);

/*
 * Macros
 */
#define checkGLcall(A) \
{ \
    GLint err = glGetError();   \
    if (err != GL_NO_ERROR) { \
       FIXME(">>>>>>>>>>>>>>>>> %x from %s @ %s / %d\n", err, A, __FILE__, __LINE__); \
    } else { \
       TRACE("%s call ok %s / %d\n", A, __FILE__, __LINE__); \
    } \
}

/* ===========================================================================
    The interfactes themselves
   =========================================================================== */

/* ---------- */
/* IDirect3D8 */
/* ---------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3D8) Direct3D8_Vtbl;
extern ICOM_VTABLE(IDirect3D8) mesa_d3d8vt;

/*****************************************************************************
 * IDirect3D implementation structure
 */
struct IDirect3D8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3D8);
    DWORD                   ref;

    /* IDirect3D8 fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3D8Impl_QueryInterface(LPDIRECT3D8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3D8Impl_AddRef(LPDIRECT3D8 iface);
extern ULONG WINAPI IDirect3D8Impl_Release(LPDIRECT3D8 iface);

/* IDirect3d8:*/
extern HRESULT  WINAPI  IDirect3D8Impl_RegisterSoftwareDevice(LPDIRECT3D8 iface, void* pInitializeFunction);
extern UINT     WINAPI  IDirect3D8Impl_GetAdapterCount(LPDIRECT3D8 iface);
extern HRESULT  WINAPI  IDirect3D8Impl_GetAdapterIdentifier(LPDIRECT3D8 iface, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier);
extern UINT     WINAPI  IDirect3D8Impl_GetAdapterModeCount(LPDIRECT3D8 iface, UINT Adapter);
extern HRESULT  WINAPI  IDirect3D8Impl_EnumAdapterModes(LPDIRECT3D8 iface, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode);
extern HRESULT  WINAPI  IDirect3D8Impl_GetAdapterDisplayMode(LPDIRECT3D8 iface, UINT Adapter, D3DDISPLAYMODE* pMode);
extern HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceType(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat,
                                                       D3DFORMAT BackBufferFormat, BOOL Windowed);
extern HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceFormat(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                                       DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
extern HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceMultiSampleType(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
                                                       BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType);
extern HRESULT  WINAPI  IDirect3D8Impl_CheckDepthStencilMatch(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                                       D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
extern HRESULT  WINAPI  IDirect3D8Impl_GetDeviceCaps(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps);
extern HMONITOR WINAPI  IDirect3D8Impl_GetAdapterMonitor(LPDIRECT3D8 iface, UINT Adapter);
extern HRESULT  WINAPI  IDirect3D8Impl_CreateDevice(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                                       DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                       IDirect3DDevice8** ppReturnedDeviceInterface);
/* --------------------- */
/* IDirect3DBaseTexture8 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DBaseTexture8) Direct3DBaseTexture8_Vtbl;

/*****************************************************************************
 * IDirect3DBaseTexture8 implementation structure
 */
struct IDirect3DBaseTexture8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DBaseTexture8);
    DWORD                   ref;

    /* IDirect3DBaseTexture8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DBaseTexture8Impl_QueryInterface(LPDIRECT3DBASETEXTURE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI   IDirect3DBaseTexture8Impl_AddRef(LPDIRECT3DBASETEXTURE8 iface);
extern ULONG WINAPI   IDirect3DBaseTexture8Impl_Release(LPDIRECT3DBASETEXTURE8 iface);

/* IDirect3DBaseTexture8 (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DBaseTexture8Impl_GetDevice(LPDIRECT3DBASETEXTURE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DBaseTexture8Impl_SetPrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DBaseTexture8Impl_GetPrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DBaseTexture8Impl_FreePrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_SetPriority(LPDIRECT3DBASETEXTURE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetPriority(LPDIRECT3DBASETEXTURE8 iface);
extern void     WINAPI        IDirect3DBaseTexture8Impl_PreLoad(LPDIRECT3DBASETEXTURE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DBaseTexture8Impl_GetType(LPDIRECT3DBASETEXTURE8 iface);

/* IDirect3DBaseTexture8 */
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_SetLOD(LPDIRECT3DBASETEXTURE8 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetLOD(LPDIRECT3DBASETEXTURE8 iface);
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetLevelCount(LPDIRECT3DBASETEXTURE8 iface);

/* ---------------- */
/* IDirect3DDevice8 */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DDevice8) Direct3DDevice8_Vtbl;


/*****************************************************************************
 * IDirect3DDevice8 implementation structure
 */
struct IDirect3DDevice8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DDevice8);
    DWORD                   ref;

    /* IDirect3DDevice8 fields */
    IDirect3D8Impl            *direct3d8;
    IDirect3DSurface8Impl     *backBuffer;
    D3DPRESENT_PARAMETERS         PresentParms;
    D3DDEVICE_CREATION_PARAMETERS CreateParms;

    UINT                       adapterNo;
    D3DDEVTYPE                 devType;

    UINT                       srcBlend;
    UINT                       dstBlend;

    /* State block related */
    BOOL                       isRecordingState;
    STATEBLOCK                 StateBlock;
    STATEBLOCK                *UpdateStateBlock;

    /* Other required values */
    float lightPosn[MAX_ACTIVE_LIGHTS][4];
    float lightDirn[MAX_ACTIVE_LIGHTS][4];

    /* OpenGL related */
    GLXContext   glCtx;
    XVisualInfo *visInfo;
    Display     *display;
    Window       win;

};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DDevice8Impl_QueryInterface(LPDIRECT3DDEVICE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DDevice8Impl_AddRef(LPDIRECT3DDEVICE8 iface);
extern ULONG WINAPI IDirect3DDevice8Impl_Release(LPDIRECT3DDEVICE8 iface);

/* IDirect3DDevice8:*/
extern HRESULT  WINAPI  IDirect3DDevice8Impl_TestCooperativeLevel(LPDIRECT3DDEVICE8 iface);
extern UINT     WINAPI  IDirect3DDevice8Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE8 iface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_ResourceManagerDiscardBytes(LPDIRECT3DDEVICE8 iface, DWORD Bytes);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetDirect3D(LPDIRECT3DDEVICE8 iface, IDirect3D8** ppD3D8);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetDeviceCaps(LPDIRECT3DDEVICE8 iface, D3DCAPS8* pCaps);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetDisplayMode(LPDIRECT3DDEVICE8 iface, D3DDISPLAYMODE* pMode);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetCreationParameters(LPDIRECT3DDEVICE8 iface, D3DDEVICE_CREATION_PARAMETERS* pParameters);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetCursorProperties(LPDIRECT3DDEVICE8 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap);
extern void     WINAPI  IDirect3DDevice8Impl_SetCursorPosition(LPDIRECT3DDEVICE8 iface, UINT XScreenSpace, UINT YScreenSpace,DWORD Flags);
extern BOOL     WINAPI  IDirect3DDevice8Impl_ShowCursor(LPDIRECT3DDEVICE8 iface, BOOL bShow);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_Reset(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_Present(LPDIRECT3DDEVICE8 iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetBackBuffer(LPDIRECT3DDEVICE8 iface, UINT BackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface8** ppBackBuffer);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetRasterStatus(LPDIRECT3DDEVICE8 iface, D3DRASTER_STATUS* pRasterStatus);
extern void     WINAPI  IDirect3DDevice8Impl_SetGammaRamp(LPDIRECT3DDEVICE8 iface, DWORD Flags,CONST D3DGAMMARAMP* pRamp);
extern void     WINAPI  IDirect3DDevice8Impl_GetGammaRamp(LPDIRECT3DDEVICE8 iface, D3DGAMMARAMP* pRamp);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateTexture(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture8** ppTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVolumeTexture(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture8** ppVolumeTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateCubeTexture(LPDIRECT3DDEVICE8 iface, UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture8** ppCubeTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer8** ppVertexBuffer);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateIndexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer8** ppIndexBuffer);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateRenderTarget(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,BOOL Lockable,IDirect3DSurface8** ppSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,IDirect3DSurface8** ppSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateImageSurface(LPDIRECT3DDEVICE8 iface, UINT Width,UINT Height,D3DFORMAT Format,IDirect3DSurface8** ppSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CopyRects(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_UpdateTexture(LPDIRECT3DDEVICE8 iface, IDirect3DBaseTexture8* pSourceTexture,IDirect3DBaseTexture8* pDestinationTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetFrontBuffer(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pDestSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pRenderTarget,IDirect3DSurface8* pNewZStencil);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppRenderTarget);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppZStencilSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_BeginScene(LPDIRECT3DDEVICE8 iface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_EndScene(LPDIRECT3DDEVICE8 iface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_Clear(LPDIRECT3DDEVICE8 iface, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_MultiplyTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetViewport(LPDIRECT3DDEVICE8 iface, CONST D3DVIEWPORT8* pViewport);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetViewport(LPDIRECT3DDEVICE8 iface, D3DVIEWPORT8* pViewport);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetMaterial(LPDIRECT3DDEVICE8 iface, CONST D3DMATERIAL8* pMaterial);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetMaterial(LPDIRECT3DDEVICE8 iface, D3DMATERIAL8* pMaterial);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetLight(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST D3DLIGHT8* pLight);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetLight(LPDIRECT3DDEVICE8 iface, DWORD Index,D3DLIGHT8* pLight);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_LightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL Enable);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetLightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL* pEnable);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST float* pPlane);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,float* pPlane);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD Value);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD* pValue);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_BeginStateBlock(LPDIRECT3DDEVICE8 iface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_EndStateBlock(LPDIRECT3DDEVICE8 iface, DWORD* pToken);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_ApplyStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CaptureStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateStateBlock(LPDIRECT3DDEVICE8 iface, D3DSTATEBLOCKTYPE Type,DWORD* pToken);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipStatus(LPDIRECT3DDEVICE8 iface, CONST D3DCLIPSTATUS8* pClipStatus);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipStatus(LPDIRECT3DDEVICE8 iface, D3DCLIPSTATUS8* pClipStatus);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage,IDirect3DBaseTexture8** ppTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage,IDirect3DBaseTexture8* pTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_ValidateDevice(LPDIRECT3DDEVICE8 iface, DWORD* pNumPasses);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetInfo(LPDIRECT3DDEVICE8 iface, DWORD DevInfoID,void* pDevInfoStruct,DWORD DevInfoStructSize);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber,CONST PALETTEENTRY* pEntries);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber,PALETTEENTRY* pEntries);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT *PaletteNumber);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_ProcessVertices(LPDIRECT3DDEVICE8 iface, UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer8* pDestBuffer,DWORD Flags);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pDeclaration,CONST DWORD* pFunction,DWORD* pHandle,DWORD Usage);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD Handle);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD* pHandle);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteVertexShader(LPDIRECT3DDEVICE8 iface, DWORD Handle);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register,CONST void* pConstantData,DWORD ConstantCount);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register,void* pConstantData,DWORD ConstantCount);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderDeclaration(LPDIRECT3DDEVICE8 iface, DWORD Handle,void* pData,DWORD* pSizeOfData);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetVertexShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD Handle,void* pData,DWORD* pSizeOfData);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8* pStreamData,UINT Stride);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8** ppStreamData,UINT* pStride);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8* pIndexData,UINT BaseVertexIndex);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8** ppIndexData,UINT* pBaseVertexIndex);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreatePixelShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pFunction,DWORD* pHandle);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD Handle);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD* pHandle);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DeletePixelShader(LPDIRECT3DDEVICE8 iface, DWORD Handle);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register,CONST void* pConstantData,DWORD ConstantCount);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register,void* pConstantData,DWORD ConstantCount);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetPixelShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD Handle,void* pData,DWORD* pSizeOfData);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawRectPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawTriPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DeletePatch(LPDIRECT3DDEVICE8 iface, UINT Handle);

/* ------------------ */
/* IDirect3DResource8 */
/* ------------------ */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DResource8) Direct3DResource8_Vtbl;

/*****************************************************************************
 * IDirect3DResource8 implementation structure
 */
struct IDirect3DResource8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DResource8);
    DWORD                   ref;

    /* IDirect3DResource8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DResource8Impl_QueryInterface(LPDIRECT3DRESOURCE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DResource8Impl_AddRef(LPDIRECT3DRESOURCE8 iface);
extern ULONG WINAPI IDirect3DResource8Impl_Release(LPDIRECT3DRESOURCE8 iface);

/* IDirect3DResource8 */
extern HRESULT  WINAPI        IDirect3DResource8Impl_GetDevice(LPDIRECT3DRESOURCE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DResource8Impl_SetPrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DResource8Impl_GetPrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DResource8Impl_FreePrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DResource8Impl_SetPriority(LPDIRECT3DRESOURCE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DResource8Impl_GetPriority(LPDIRECT3DRESOURCE8 iface);
extern void     WINAPI        IDirect3DResource8Impl_PreLoad(LPDIRECT3DRESOURCE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DResource8Impl_GetType(LPDIRECT3DRESOURCE8 iface);

/* ---------------------- */
/* IDirect3DVertexBuffer8 */
/* ---------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DVertexBuffer8) Direct3DVertexBuffer8_Vtbl;

/*****************************************************************************
 * IDirect3DVertexBuffer8 implementation structure
 */
struct IDirect3DVertexBuffer8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DVertexBuffer8);
    DWORD                   ref;

    /* IDirect3DVertexBuffer8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;
    BYTE *allocatedMemory;
    D3DVERTEXBUFFER_DESC currentDesc;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DVertexBuffer8Impl_QueryInterface(LPDIRECT3DVERTEXBUFFER8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DVertexBuffer8Impl_AddRef(LPDIRECT3DVERTEXBUFFER8 iface);
extern ULONG WINAPI IDirect3DVertexBuffer8Impl_Release(LPDIRECT3DVERTEXBUFFER8 iface);

/* IDirect3DVertexBuffer8 (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetDevice(LPDIRECT3DVERTEXBUFFER8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DVertexBuffer8Impl_SetPriority(LPDIRECT3DVERTEXBUFFER8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DVertexBuffer8Impl_GetPriority(LPDIRECT3DVERTEXBUFFER8 iface);
extern void     WINAPI        IDirect3DVertexBuffer8Impl_PreLoad(LPDIRECT3DVERTEXBUFFER8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer8Impl_GetType(LPDIRECT3DVERTEXBUFFER8 iface);

/* IDirect3DVertexBuffer8 */
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_Lock(LPDIRECT3DVERTEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_Unlock(LPDIRECT3DVERTEXBUFFER8 iface);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetDesc(LPDIRECT3DVERTEXBUFFER8 iface, D3DVERTEXBUFFER_DESC *pDesc);

/* IDirect3DVolume8 private include file
 */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DVolume8) Direct3DVolume8_Vtbl;

/*****************************************************************************
 * IDirect3DVolume8 implementation structure
 */
struct IDirect3DVolume8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DVolume8);
    DWORD                   ref;

    /* IDirect3DVolume8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    void                   *Container;
    D3DVOLUME_DESC          myDesc;
    BYTE                   *allocatedMemory;
    UINT                    textureName;
    UINT                    bytesPerPixel;

};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DVolume8Impl_QueryInterface(LPDIRECT3DVOLUME8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DVolume8Impl_AddRef(LPDIRECT3DVOLUME8 iface);
extern ULONG WINAPI IDirect3DVolume8Impl_Release(LPDIRECT3DVOLUME8 iface);

/* IDirect3DVolume8 */
extern HRESULT WINAPI IDirect3DVolume8Impl_GetDevice(LPDIRECT3DVOLUME8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT WINAPI IDirect3DVolume8Impl_SetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT WINAPI IDirect3DVolume8Impl_GetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID  refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT WINAPI IDirect3DVolume8Impl_FreePrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid);
extern HRESULT WINAPI IDirect3DVolume8Impl_GetContainer(LPDIRECT3DVOLUME8 iface, REFIID riid, void** ppContainer);
extern HRESULT WINAPI IDirect3DVolume8Impl_GetDesc(LPDIRECT3DVOLUME8 iface, D3DVOLUME_DESC* pDesc);
extern HRESULT WINAPI IDirect3DVolume8Impl_LockBox(LPDIRECT3DVOLUME8 iface, D3DLOCKED_BOX* pLockedVolume,CONST D3DBOX* pBox, DWORD Flags);
extern HRESULT WINAPI IDirect3DVolume8Impl_UnlockBox(LPDIRECT3DVOLUME8 iface);

/* ------------------- */
/* IDirect3DSwapChain8 */
/* ------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DSwapChain8) Direct3DSwapChain8_Vtbl;

/*****************************************************************************
 * IDirect3DSwapChain8 implementation structure
 */
struct IDirect3DSwapChain8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DSwapChain8);
    DWORD                   ref;

    /* IDirect3DSwapChain8 fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DSwapChain8Impl_QueryInterface(LPDIRECT3DSWAPCHAIN8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DSwapChain8Impl_AddRef(LPDIRECT3DSWAPCHAIN8 iface);
extern ULONG WINAPI IDirect3DSwapChain8Impl_Release(LPDIRECT3DSWAPCHAIN8 iface);

/* IDirect3DSwapChain8 */
extern HRESULT WINAPI IDirect3DSwapChain8Impl_Present(LPDIRECT3DSWAPCHAIN8 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
extern HRESULT WINAPI IDirect3DSwapChain8Impl_GetBackBuffer(LPDIRECT3DSWAPCHAIN8 iface, UINT BackBuffer, D3DBACKBUFFER_TYPE Type,IDirect3DSurface8** ppBackBuffer);

/* ----------------- */
/* IDirect3DSurface8 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DSurface8) Direct3DSurface8_Vtbl;

/*****************************************************************************
 * IDirect3DSurface8 implementation structure
 */
struct IDirect3DSurface8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DSurface8);
    DWORD                   ref;

    /* IDirect3DSurface8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    void                   *Container;

    D3DSURFACE_DESC         myDesc;
    BYTE                   *allocatedMemory;
    UINT                    textureName;
    UINT                    bytesPerPixel;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DSurface8Impl_QueryInterface(LPDIRECT3DSURFACE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DSurface8Impl_AddRef(LPDIRECT3DSURFACE8 iface);
extern ULONG WINAPI IDirect3DSurface8Impl_Release(LPDIRECT3DSURFACE8 iface);

/* IDirect3DSurface8 */
extern HRESULT WINAPI IDirect3DSurface8Impl_GetDevice(LPDIRECT3DSURFACE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT WINAPI IDirect3DSurface8Impl_SetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags);
extern HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid,void* pData,DWORD* pSizeOfData);
extern HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid);
extern HRESULT WINAPI IDirect3DSurface8Impl_GetContainer(LPDIRECT3DSURFACE8 iface, REFIID riid,void** ppContainer);
extern HRESULT WINAPI IDirect3DSurface8Impl_GetDesc(LPDIRECT3DSURFACE8 iface, D3DSURFACE_DESC *pDesc);
extern HRESULT WINAPI IDirect3DSurface8Impl_LockRect(LPDIRECT3DSURFACE8 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect,DWORD Flags);
extern HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(LPDIRECT3DSURFACE8 iface);

/* --------------------- */
/* IDirect3DIndexBuffer8 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DIndexBuffer8) Direct3DIndexBuffer8_Vtbl;

/*****************************************************************************
 * IDirect3DIndexBuffer8 implementation structure
 */
struct IDirect3DIndexBuffer8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DIndexBuffer8);
    DWORD                   ref;

    /* IDirect3DIndexBuffer8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    D3DINDEXBUFFER_DESC     currentDesc;
    void                   *allocatedMemory;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DIndexBuffer8Impl_QueryInterface(LPDIRECT3DINDEXBUFFER8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DIndexBuffer8Impl_AddRef(LPDIRECT3DINDEXBUFFER8 iface);
extern ULONG WINAPI IDirect3DIndexBuffer8Impl_Release(LPDIRECT3DINDEXBUFFER8 iface);

/* IDirect3DIndexBuffer8 (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetDevice(LPDIRECT3DINDEXBUFFER8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_SetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_FreePrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DIndexBuffer8Impl_SetPriority(LPDIRECT3DINDEXBUFFER8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DIndexBuffer8Impl_GetPriority(LPDIRECT3DINDEXBUFFER8 iface);
extern void     WINAPI        IDirect3DIndexBuffer8Impl_PreLoad(LPDIRECT3DINDEXBUFFER8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DIndexBuffer8Impl_GetType(LPDIRECT3DINDEXBUFFER8 iface);

/* IDirect3DIndexBuffer8 */
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_Lock(LPDIRECT3DINDEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_Unlock(LPDIRECT3DINDEXBUFFER8 iface);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetDesc(LPDIRECT3DINDEXBUFFER8 iface, D3DINDEXBUFFER_DESC *pDesc);

/* --------------------- */
/* IDirect3DCubeTexture8 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DCubeTexture8) Direct3DCubeTexture8_Vtbl;

/*****************************************************************************
 * IDirect3DCubeTexture8 implementation structure
 */
struct IDirect3DCubeTexture8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DCubeTexture8);
    DWORD                   ref;

    /* IDirect3DCubeTexture8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    UINT edgeLength;
    DWORD usage;
    UINT levels;
    D3DFORMAT format;

    IDirect3DDevice8Impl  *device;
    IDirect3DSurface8Impl *surfaces[6][MAX_LEVELS];
    BOOL Dirty;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DCubeTexture8Impl_QueryInterface(LPDIRECT3DCUBETEXTURE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DCubeTexture8Impl_AddRef(LPDIRECT3DCUBETEXTURE8 iface);
extern ULONG WINAPI IDirect3DCubeTexture8Impl_Release(LPDIRECT3DCUBETEXTURE8 iface);

/* IDirect3DCubeTexture8 (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetDevice(LPDIRECT3DCUBETEXTURE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_SetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_FreePrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_SetPriority(LPDIRECT3DCUBETEXTURE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetPriority(LPDIRECT3DCUBETEXTURE8 iface);
extern void     WINAPI        IDirect3DCubeTexture8Impl_PreLoad(LPDIRECT3DCUBETEXTURE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DCubeTexture8Impl_GetType(LPDIRECT3DCUBETEXTURE8 iface);

/* IDirect3DCubeTexture8 (Inherited from IDirect3DBaseTexture8) */
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_SetLOD(LPDIRECT3DCUBETEXTURE8 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetLOD(LPDIRECT3DCUBETEXTURE8 iface);
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetLevelCount(LPDIRECT3DCUBETEXTURE8 iface);

/* IDirect3DCubeTexture8 */
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetLevelDesc(LPDIRECT3DCUBETEXTURE8 iface,UINT Level,D3DSURFACE_DESC *pDesc);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetCubeMapSurface(LPDIRECT3DCUBETEXTURE8 iface,D3DCUBEMAP_FACES FaceType,UINT Level,IDirect3DSurface8** ppCubeMapSurface);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_LockRect(LPDIRECT3DCUBETEXTURE8 iface,D3DCUBEMAP_FACES FaceType,UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_UnlockRect(LPDIRECT3DCUBETEXTURE8 iface,D3DCUBEMAP_FACES FaceType,UINT Level);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_AddDirtyRect(LPDIRECT3DCUBETEXTURE8 iface,D3DCUBEMAP_FACES FaceType,CONST RECT* pDirtyRect);

/* ----------------- */
/* IDirect3DTexture8 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DTexture8) Direct3DTexture8_Vtbl;

/*****************************************************************************
 * IDirect3DTexture8 implementation structure
 */
struct IDirect3DTexture8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DTexture8);
    DWORD                   ref;

    /* IDirect3DTexture8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;
    UINT width;
    UINT height;
    UINT levels;
    DWORD usage;
    D3DFORMAT format;

    IDirect3DDevice8Impl *device;
    IDirect3DSurface8Impl *surfaces[MAX_LEVELS];
    BOOL Dirty;

};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DTexture8Impl_QueryInterface(LPDIRECT3DTEXTURE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI   IDirect3DTexture8Impl_AddRef(LPDIRECT3DTEXTURE8 iface);
extern ULONG WINAPI   IDirect3DTexture8Impl_Release(LPDIRECT3DTEXTURE8 iface);

/* IDirect3DTexture8 (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DTexture8Impl_GetDevice(LPDIRECT3DTEXTURE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_SetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_GetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_FreePrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DTexture8Impl_SetPriority(LPDIRECT3DTEXTURE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DTexture8Impl_GetPriority(LPDIRECT3DTEXTURE8 iface);
extern void     WINAPI        IDirect3DTexture8Impl_PreLoad(LPDIRECT3DTEXTURE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DTexture8Impl_GetType(LPDIRECT3DTEXTURE8 iface);

/* IDirect3DTexture8 (Inherited from IDirect3DBaseTexture8) */
extern DWORD    WINAPI        IDirect3DTexture8Impl_SetLOD(LPDIRECT3DTEXTURE8 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DTexture8Impl_GetLOD(LPDIRECT3DTEXTURE8 iface);
extern DWORD    WINAPI        IDirect3DTexture8Impl_GetLevelCount(LPDIRECT3DTEXTURE8 iface);

/* IDirect3DTexture8 */
extern HRESULT  WINAPI        IDirect3DTexture8Impl_GetLevelDesc(LPDIRECT3DTEXTURE8 iface, UINT Level,D3DSURFACE_DESC* pDesc);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_GetSurfaceLevel(LPDIRECT3DTEXTURE8 iface, UINT Level,IDirect3DSurface8** ppSurfaceLevel);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_LockRect(LPDIRECT3DTEXTURE8 iface, UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_UnlockRect(LPDIRECT3DTEXTURE8 iface, UINT Level);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_AddDirtyRect(LPDIRECT3DTEXTURE8 iface, CONST RECT* pDirtyRect);

/* ----------------------- */
/* IDirect3DVolumeTexture8 */
/* ----------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirect3DVolumeTexture8) Direct3DVolumeTexture8_Vtbl;

/*****************************************************************************
 * IDirect3DVolumeTexture8 implementation structure
 */
struct IDirect3DVolumeTexture8Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DVolumeTexture8);
    DWORD                   ref;

    /* IDirect3DVolumeTexture8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;
    UINT                    width;
    UINT                    height;
    UINT                    depth;
    UINT                    levels;
    DWORD                   usage;
    D3DFORMAT               format;

    IDirect3DDevice8Impl *device;
    IDirect3DVolume8Impl *volumes[MAX_LEVELS];
    BOOL Dirty;

};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DVolumeTexture8Impl_QueryInterface(LPDIRECT3DVOLUMETEXTURE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI IDirect3DVolumeTexture8Impl_AddRef(LPDIRECT3DVOLUMETEXTURE8 iface);
extern ULONG WINAPI IDirect3DVolumeTexture8Impl_Release(LPDIRECT3DVOLUMETEXTURE8 iface);

/* IDirect3DVolumeTexture8 (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetDevice(LPDIRECT3DVOLUMETEXTURE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_SetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE8 iface);
extern void     WINAPI        IDirect3DVolumeTexture8Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture8Impl_GetType(LPDIRECT3DVOLUMETEXTURE8 iface);

/* IDirect3DVolumeTexture8 (Inherited from IDirect3DBaseTexture8) */
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE8 iface);
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetLevelCount(LPDIRECT3DVOLUMETEXTURE8 iface);

/* IDirect3DVolumeTexture8 */
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetLevelDesc(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level,D3DVOLUME_DESC *pDesc);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetVolumeLevel(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level,IDirect3DVolume8** ppVolumeLevel);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_LockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level,D3DLOCKED_BOX* pLockedVolume,CONST D3DBOX* pBox,DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE8 iface, CONST D3DBOX* pDirtyBox);

#endif /* __WINE_D3DX8_PRIVATE_H */
