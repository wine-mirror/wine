/*
 * Direct3D 9 private include file
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
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

#ifndef __WINE_D3D9_PRIVATE_H
#define __WINE_D3D9_PRIVATE_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

/* THIS FILE MUST NOT CONTAIN X11 or MESA DEFINES */
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define XMD_H 
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

#include "d3d9.h"
#include "d3d9_private.h"
#include "wine/wined3d_interface.h"

/* X11 locking */

extern void (*wine_tsx11_lock_ptr)(void);
extern void (*wine_tsx11_unlock_ptr)(void);

/* As GLX relies on X, this is needed */
#define ENTER_GL() wine_tsx11_lock_ptr()
#define LEAVE_GL() wine_tsx11_unlock_ptr()

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "d3d9.h"

/* Device caps */
#define MAX_PALETTES      256
#define MAX_STREAMS       16
#define MAX_ACTIVE_LIGHTS 8
#define MAX_CLIPPLANES    D3DMAXUSERCLIPPLANES
#define MAX_LEVELS        256

/* Other useful values */
#define HIGHEST_RENDER_STATE D3DRS_BLENDOPALPHA
#define HIGHEST_TEXTURE_STATE 29
#define HIGHEST_TRANSFORMSTATE 512
#define D3DSBT_RECORDED 0xfffffffe

#define D3D_VSHADER_MAX_CONSTANTS 96
#define D3D_PSHADER_MAX_CONSTANTS 32

/* ===========================================================================
    Macros
   =========================================================================== */
/* Not nice, but it lets wined3d support different versions of directx */
#define D3D9CAPSTOWINECAPS(_pD3D9Caps, _pWineCaps) \
    _pWineCaps->DeviceType                        = &_pD3D9Caps->DeviceType; \
    _pWineCaps->AdapterOrdinal                    = &_pD3D9Caps->AdapterOrdinal; \
    _pWineCaps->Caps                              = &_pD3D9Caps->Caps; \
    _pWineCaps->Caps2                             = &_pD3D9Caps->Caps2; \
    _pWineCaps->Caps3                             = &_pD3D9Caps->Caps3; \
    _pWineCaps->PresentationIntervals             = &_pD3D9Caps->PresentationIntervals; \
    _pWineCaps->CursorCaps                        = &_pD3D9Caps->CursorCaps; \
    _pWineCaps->DevCaps                           = &_pD3D9Caps->DevCaps; \
    _pWineCaps->PrimitiveMiscCaps                 = &_pD3D9Caps->PrimitiveMiscCaps; \
    _pWineCaps->RasterCaps                        = &_pD3D9Caps->RasterCaps; \
    _pWineCaps->ZCmpCaps                          = &_pD3D9Caps->ZCmpCaps; \
    _pWineCaps->SrcBlendCaps                      = &_pD3D9Caps->SrcBlendCaps; \
    _pWineCaps->DestBlendCaps                     = &_pD3D9Caps->DestBlendCaps; \
    _pWineCaps->AlphaCmpCaps                      = &_pD3D9Caps->AlphaCmpCaps; \
    _pWineCaps->ShadeCaps                         = &_pD3D9Caps->ShadeCaps; \
    _pWineCaps->TextureCaps                       = &_pD3D9Caps->TextureCaps; \
    _pWineCaps->TextureFilterCaps                 = &_pD3D9Caps->TextureFilterCaps; \
    _pWineCaps->CubeTextureFilterCaps             = &_pD3D9Caps->CubeTextureFilterCaps; \
    _pWineCaps->VolumeTextureFilterCaps           = &_pD3D9Caps->VolumeTextureFilterCaps; \
    _pWineCaps->TextureAddressCaps                = &_pD3D9Caps->TextureAddressCaps; \
    _pWineCaps->VolumeTextureAddressCaps          = &_pD3D9Caps->VolumeTextureAddressCaps; \
    _pWineCaps->LineCaps                          = &_pD3D9Caps->LineCaps; \
    _pWineCaps->MaxTextureWidth                   = &_pD3D9Caps->MaxTextureWidth; \
    _pWineCaps->MaxTextureHeight                  = &_pD3D9Caps->MaxTextureHeight; \
    _pWineCaps->MaxVolumeExtent                   = &_pD3D9Caps->MaxVolumeExtent; \
    _pWineCaps->MaxTextureRepeat                  = &_pD3D9Caps->MaxTextureRepeat; \
    _pWineCaps->MaxTextureAspectRatio             = &_pD3D9Caps->MaxTextureAspectRatio; \
    _pWineCaps->MaxAnisotropy                     = &_pD3D9Caps->MaxAnisotropy; \
    _pWineCaps->MaxVertexW                        = &_pD3D9Caps->MaxVertexW; \
    _pWineCaps->GuardBandLeft                     = &_pD3D9Caps->GuardBandLeft; \
    _pWineCaps->GuardBandTop                      = &_pD3D9Caps->GuardBandTop; \
    _pWineCaps->GuardBandRight                    = &_pD3D9Caps->GuardBandRight; \
    _pWineCaps->GuardBandBottom                   = &_pD3D9Caps->GuardBandBottom; \
    _pWineCaps->ExtentsAdjust                     = &_pD3D9Caps->ExtentsAdjust; \
    _pWineCaps->StencilCaps                       = &_pD3D9Caps->StencilCaps; \
    _pWineCaps->FVFCaps                           = &_pD3D9Caps->FVFCaps; \
    _pWineCaps->TextureOpCaps                     = &_pD3D9Caps->TextureOpCaps; \
    _pWineCaps->MaxTextureBlendStages             = &_pD3D9Caps->MaxTextureBlendStages; \
    _pWineCaps->MaxSimultaneousTextures           = &_pD3D9Caps->MaxSimultaneousTextures; \
    _pWineCaps->VertexProcessingCaps              = &_pD3D9Caps->VertexProcessingCaps; \
    _pWineCaps->MaxActiveLights                   = &_pD3D9Caps->MaxActiveLights; \
    _pWineCaps->MaxUserClipPlanes                 = &_pD3D9Caps->MaxUserClipPlanes; \
    _pWineCaps->MaxVertexBlendMatrices            = &_pD3D9Caps->MaxVertexBlendMatrices; \
    _pWineCaps->MaxVertexBlendMatrixIndex         = &_pD3D9Caps->MaxVertexBlendMatrixIndex; \
    _pWineCaps->MaxPointSize                      = &_pD3D9Caps->MaxPointSize; \
    _pWineCaps->MaxPrimitiveCount                 = &_pD3D9Caps->MaxPrimitiveCount; \
    _pWineCaps->MaxVertexIndex                    = &_pD3D9Caps->MaxVertexIndex; \
    _pWineCaps->MaxStreams                        = &_pD3D9Caps->MaxStreams; \
    _pWineCaps->MaxStreamStride                   = &_pD3D9Caps->MaxStreamStride; \
    _pWineCaps->VertexShaderVersion               = &_pD3D9Caps->VertexShaderVersion; \
    _pWineCaps->MaxVertexShaderConst              = &_pD3D9Caps->MaxVertexShaderConst; \
    _pWineCaps->PixelShaderVersion                = &_pD3D9Caps->PixelShaderVersion; \
    _pWineCaps->PixelShader1xMaxValue             = &_pD3D9Caps->PixelShader1xMaxValue; \
    _pWineCaps->DevCaps2                          = &_pD3D9Caps->DevCaps2; \
    _pWineCaps->MaxNpatchTessellationLevel        = &_pD3D9Caps->MaxNpatchTessellationLevel; \
    _pWineCaps->MasterAdapterOrdinal              = &_pD3D9Caps->MasterAdapterOrdinal; \
    _pWineCaps->AdapterOrdinalInGroup             = &_pD3D9Caps->AdapterOrdinalInGroup; \
    _pWineCaps->NumberOfAdaptersInGroup           = &_pD3D9Caps->NumberOfAdaptersInGroup; \
    _pWineCaps->DeclTypes                         = &_pD3D9Caps->DeclTypes; \
    _pWineCaps->NumSimultaneousRTs                = &_pD3D9Caps->NumSimultaneousRTs; \
    _pWineCaps->StretchRectFilterCaps             = &_pD3D9Caps->StretchRectFilterCaps; \
    _pWineCaps->VS20Caps.Caps                     = &_pD3D9Caps->VS20Caps.Caps; \
    _pWineCaps->VS20Caps.DynamicFlowControlDepth  = &_pD3D9Caps->VS20Caps.DynamicFlowControlDepth; \
    _pWineCaps->VS20Caps.NumTemps                 = &_pD3D9Caps->VS20Caps.NumTemps; \
    _pWineCaps->VS20Caps.NumTemps                 = &_pD3D9Caps->VS20Caps.NumTemps; \
    _pWineCaps->VS20Caps.StaticFlowControlDepth   = &_pD3D9Caps->VS20Caps.StaticFlowControlDepth; \
    _pWineCaps->PS20Caps.Caps                     = &_pD3D9Caps->PS20Caps.Caps; \
    _pWineCaps->PS20Caps.DynamicFlowControlDepth  = &_pD3D9Caps->PS20Caps.DynamicFlowControlDepth; \
    _pWineCaps->PS20Caps.NumTemps                 = &_pD3D9Caps->PS20Caps.NumTemps; \
    _pWineCaps->PS20Caps.StaticFlowControlDepth   = &_pD3D9Caps->PS20Caps.StaticFlowControlDepth; \
    _pWineCaps->PS20Caps.NumInstructionSlots      = &_pD3D9Caps->PS20Caps.NumInstructionSlots; \
    _pWineCaps->VertexTextureFilterCaps           = &_pD3D9Caps->VertexTextureFilterCaps; \
    _pWineCaps->MaxVShaderInstructionsExecuted    = &_pD3D9Caps->MaxVShaderInstructionsExecuted; \
    _pWineCaps->MaxPShaderInstructionsExecuted    = &_pD3D9Caps->MaxPShaderInstructionsExecuted; \
    _pWineCaps->MaxVertexShader30InstructionSlots = &_pD3D9Caps->MaxVertexShader30InstructionSlots; \
    _pWineCaps->MaxPixelShader30InstructionSlots  = &_pD3D9Caps->MaxPixelShader30InstructionSlots;

/* Direct3D9 Interfaces: */
typedef struct IDirect3D9Impl                  IDirect3D9Impl;
typedef struct IDirect3DDevice9Impl            IDirect3DDevice9Impl;
typedef struct IDirect3DBaseTexture9Impl       IDirect3DBaseTexture9Impl;
typedef struct IDirect3DTexture9Impl           IDirect3DTexture9Impl;
typedef struct IDirect3DVolumeTexture9Impl     IDirect3DVolumeTexture9Impl;
typedef struct IDirect3DCubeTexture9Impl       IDirect3DCubeTexture9Impl;
typedef struct IDirect3DVertexBuffer9Impl      IDirect3DVertexBuffer9Impl;
typedef struct IDirect3DIndexBuffer9Impl       IDirect3DIndexBuffer9Impl;
typedef struct IDirect3DSurface9Impl           IDirect3DSurface9Impl;
typedef struct IDirect3DResource9Impl          IDirect3DResource9Impl;
typedef struct IDirect3DVolume9Impl            IDirect3DVolume9Impl;
typedef struct IDirect3DVertexShader9Impl      IDirect3DVertexShader9Impl;
typedef struct IDirect3DVertexDeclaration9Impl IDirect3DVertexDeclaration9Impl;


#define D3DCOLOR_R(dw) (((float) (((dw) >> 16) & 0xFF)) / 255.0f)
#define D3DCOLOR_G(dw) (((float) (((dw) >>  8) & 0xFF)) / 255.0f)
#define D3DCOLOR_B(dw) (((float) (((dw) >>  0) & 0xFF)) / 255.0f)
#define D3DCOLOR_A(dw) (((float) (((dw) >> 24) & 0xFF)) / 255.0f)

#define D3DCOLORTOCOLORVALUE(dw, col) \
  (col).r = D3DCOLOR_R(dw); \
  (col).g = D3DCOLOR_G(dw); \
  (col).b = D3DCOLOR_B(dw); \
  (col).a = D3DCOLOR_A(dw); 

#define D3DCOLORTOVECTOR4(dw, vec) \
  (vec).x = D3DCOLOR_R(dw); \
  (vec).y = D3DCOLOR_G(dw); \
  (vec).z = D3DCOLOR_B(dw); \
  (vec).w = D3DCOLOR_A(dw);

#define D3DCOLORTOGLFLOAT4(dw, vec) \
  (vec)[0] = D3DCOLOR_R(dw); \
  (vec)[1] = D3DCOLOR_G(dw); \
  (vec)[2] = D3DCOLOR_B(dw); \
  (vec)[3] = D3DCOLOR_A(dw);

typedef struct D3DSHADERVECTORF {
  float x;
  float y;
  float z;
  float w;
} D3DSHADERVECTORF;

typedef struct D3DSHADERVECTORI {
  int x;
  int y;
  int z;
  int w;
} D3DSHADERVECTORI;

/* ===========================================================================
    The interfactes themselves
   =========================================================================== */

/* ---------- */
/* IDirect3D9 */
/* ---------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3D9Vtbl Direct3D9_Vtbl;

/*****************************************************************************
 * IDirect3D implementation structure
 */
struct IDirect3D9Impl
{
    /* IUnknown fields */
    const IDirect3D9Vtbl   *lpVtbl;
    LONG                    ref;

    /* The WineD3D device */
    IWineD3D               *WineD3D;

    /* IDirect3D9 fields */
};

/* IUnknown: */
extern HRESULT WINAPI   IDirect3D9Impl_QueryInterface(LPDIRECT3D9 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI     IDirect3D9Impl_AddRef(LPDIRECT3D9 iface);
extern ULONG WINAPI     IDirect3D9Impl_Release(LPDIRECT3D9 iface);

/* IDirect3d9: */
extern HRESULT  WINAPI  IDirect3D9Impl_RegisterSoftwareDevice(LPDIRECT3D9 iface, void* pInitializeFunction);
extern UINT     WINAPI  IDirect3D9Impl_GetAdapterCount(LPDIRECT3D9 iface);
extern HRESULT  WINAPI  IDirect3D9Impl_GetAdapterIdentifier(LPDIRECT3D9 iface, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier);
extern UINT     WINAPI  IDirect3D9Impl_GetAdapterModeCount(LPDIRECT3D9 iface, UINT Adapter, D3DFORMAT Format);
extern HRESULT  WINAPI  IDirect3D9Impl_EnumAdapterModes(LPDIRECT3D9 iface, UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode);
extern HRESULT  WINAPI  IDirect3D9Impl_GetAdapterDisplayMode(LPDIRECT3D9 iface, UINT Adapter, D3DDISPLAYMODE* pMode);
extern HRESULT  WINAPI  IDirect3D9Impl_CheckDeviceType(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed);
extern HRESULT  WINAPI  IDirect3D9Impl_CheckDeviceFormat(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
extern HRESULT  WINAPI  IDirect3D9Impl_CheckDeviceMultiSampleType(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels);
extern HRESULT  WINAPI  IDirect3D9Impl_CheckDepthStencilMatch(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
extern HRESULT  WINAPI  IDirect3D9Impl_CheckDeviceFormatConversion(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat);
extern HRESULT  WINAPI  IDirect3D9Impl_GetDeviceCaps(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps);
extern HMONITOR WINAPI  IDirect3D9Impl_GetAdapterMonitor(LPDIRECT3D9 iface, UINT Adapter);
extern HRESULT  WINAPI  IDirect3D9Impl_CreateDevice(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);

/* ---------------- */
/* IDirect3DDevice9 */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DDevice9Vtbl Direct3DDevice9_Vtbl;

/*****************************************************************************
 * IDirect3DDevice9 implementation structure
 */
struct IDirect3DDevice9Impl
{
    /* IUnknown fields */
    const IDirect3DDevice9Vtbl   *lpVtbl;
    LONG                          ref;

    /* IDirect3DDevice9 fields */
    IWineD3DDevice               *WineD3DDevice;

    /* FIXME: To be sorted out during move */
    IDirect3DSurface9Impl        *frontBuffer;
    IDirect3DSurface9Impl        *backBuffer;
    IDirect3DSurface9Impl        *depthStencilBuffer;

    IDirect3DSurface9Impl        *renderTarget;
    IDirect3DSurface9Impl        *stencilBufferTarget;
};

/* IUnknown: */
extern HRESULT WINAPI   IDirect3DDevice9Impl_QueryInterface(LPDIRECT3DDEVICE9 iface, REFIID refiid, LPVOID *obj);
extern ULONG WINAPI     IDirect3DDevice9Impl_AddRef(LPDIRECT3DDEVICE9 iface);
extern ULONG WINAPI     IDirect3DDevice9Impl_Release(LPDIRECT3DDEVICE9 iface);

/* IDirect3DDevice9: */
extern HRESULT  WINAPI  IDirect3DDevice9Impl_TestCooperativeLevel(LPDIRECT3DDEVICE9 iface);
extern UINT     WINAPI  IDirect3DDevice9Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE9 iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_EvictManagedRessources(LPDIRECT3DDEVICE9 iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetDirect3D(LPDIRECT3DDEVICE9 iface, IDirect3D9** ppD3D9);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetDeviceCaps(LPDIRECT3DDEVICE9 iface, D3DCAPS9* pCaps);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetDisplayMode(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DDISPLAYMODE* pMode);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetCreationParameters(LPDIRECT3DDEVICE9 iface, D3DDEVICE_CREATION_PARAMETERS* pParameters);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetCursorProperties(LPDIRECT3DDEVICE9 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap);
extern void     WINAPI  IDirect3DDevice9Impl_SetCursorPosition(LPDIRECT3DDEVICE9 iface, int XScreenSpace, int YScreenSpace, DWORD Flags);
extern BOOL     WINAPI  IDirect3DDevice9Impl_ShowCursor(LPDIRECT3DDEVICE9 iface, BOOL bShow);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetSwapChain(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain);
extern UINT     WINAPI  IDirect3DDevice9Impl_GetNumberOfSwapChains(LPDIRECT3DDEVICE9 iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_Reset(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_Present(LPDIRECT3DDEVICE9 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetBackBuffer(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetRasterStatus(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetDialogBoxMode(LPDIRECT3DDEVICE9 iface, BOOL bEnableDialogs);
extern void     WINAPI  IDirect3DDevice9Impl_SetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp);
extern void     WINAPI  IDirect3DDevice9Impl_GetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DGAMMARAMP* pRamp);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVolumeTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateCubeTexture(LPDIRECT3DDEVICE9 iface, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexBuffer(LPDIRECT3DDEVICE9 iface, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateIndexBuffer(LPDIRECT3DDEVICE9 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateRenderTarget(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_UpdateSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_UpdateTexture(LPDIRECT3DDEVICE9 iface, IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetRenderTargetData(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetFrontBufferData(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSurface9* pDestSurface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_StretchRects(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_ColorFill(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateOffscreenPlainSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetRenderTarget(LPDIRECT3DDEVICE9 iface, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetRenderTarget(LPDIRECT3DDEVICE9 iface, DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetDepthStencilSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pNewZStencilSurface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9** ppZStencilSurface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_BeginScene(LPDIRECT3DDEVICE9 iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_EndScene(LPDIRECT3DDEVICE9 iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_Clear(LPDIRECT3DDEVICE9 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_MultiplyTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetViewport(LPDIRECT3DDEVICE9 iface, CONST D3DVIEWPORT9* pViewport);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetViewport(LPDIRECT3DDEVICE9 iface, D3DVIEWPORT9* pViewport);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetMaterial(LPDIRECT3DDEVICE9 iface, CONST D3DMATERIAL9* pMaterial);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetMaterial(LPDIRECT3DDEVICE9 iface, D3DMATERIAL9* pMaterial);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST D3DLIGHT9* pLight);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, D3DLIGHT9* pLight);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_LightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL Enable);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetLightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL* pEnable);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST float* pPlane);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, float* pPlane);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD* pValue);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateStateBlock(LPDIRECT3DDEVICE9 iface, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_BeginStateBlock(LPDIRECT3DDEVICE9 iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_EndStateBlock(LPDIRECT3DDEVICE9 iface, IDirect3DStateBlock9** ppSB);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetClipStatus(LPDIRECT3DDEVICE9 iface, CONST D3DCLIPSTATUS9* pClipStatus);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetClipStatus(LPDIRECT3DDEVICE9 iface, D3DCLIPSTATUS9* pClipStatus);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetTexture(LPDIRECT3DDEVICE9 iface, DWORD Stage, IDirect3DBaseTexture9** ppTexture);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetTexture(LPDIRECT3DDEVICE9 iface, DWORD Stage, IDirect3DBaseTexture9* pTexture);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_ValidateDevice(LPDIRECT3DDEVICE9 iface, DWORD* pNumPasses);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPaletteEntries(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPaletteEntries(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber, PALETTEENTRY* pEntries);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE9 iface, UINT *PaletteNumber);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetScissorRect(LPDIRECT3DDEVICE9 iface, CONST RECT* pRect);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetScissorRect(LPDIRECT3DDEVICE9 iface, RECT* pRect);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface, BOOL bSoftware);
extern BOOL     WINAPI  IDirect3DDevice9Impl_GetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetNPatchMode(LPDIRECT3DDEVICE9 iface, float nSegments);
extern float    WINAPI  IDirect3DDevice9Impl_GetNPatchMode(LPDIRECT3DDEVICE9 iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_DrawPrimitive(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_ProcessVertices(LPDIRECT3DDEVICE9 iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexDeclaration(LPDIRECT3DDEVICE9 iface, CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9* pDecl);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9** ppDecl);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetFVF(LPDIRECT3DDEVICE9 iface, DWORD FVF);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetFVF(LPDIRECT3DDEVICE9 iface, DWORD* pFVF);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9* pShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9** ppShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetStreamSource(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetStreamSource(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetStreamSourceFreq(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, UINT Divider);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetStreamSourceFreq(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, UINT* Divider);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetIndices(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9* pIndexData);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetIndices(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9** ppIndexData);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreatePixelShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9* pShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9** ppShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_DrawRectPatch(LPDIRECT3DDEVICE9 iface, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_DrawTriPatch(LPDIRECT3DDEVICE9 iface, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_DeletePatch(LPDIRECT3DDEVICE9 iface, UINT Handle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateQuery(LPDIRECT3DDEVICE9 iface, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery);


/* ---------------- */
/* IDirect3DVolume9 */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVolume9Vtbl Direct3DVolume9_Vtbl;

/*****************************************************************************
 * IDirect3DVolume9 implementation structure
 */
struct IDirect3DVolume9Impl
{
    /* IUnknown fields */
    const IDirect3DVolume9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DVolume9 fields */
    IWineD3DVolume         *wineD3DVolume;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DVolume9Impl_QueryInterface(LPDIRECT3DVOLUME9 iface, REFIID refiid, LPVOID* obj);
extern ULONG WINAPI   IDirect3DVolume9Impl_AddRef(LPDIRECT3DVOLUME9 iface);
extern ULONG WINAPI   IDirect3DVolume9Impl_Release(LPDIRECT3DVOLUME9 iface);

/* IDirect3DVolume9: */
extern HRESULT WINAPI IDirect3DVolume9Impl_GetDevice(LPDIRECT3DVOLUME9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT WINAPI IDirect3DVolume9Impl_SetPrivateData(LPDIRECT3DVOLUME9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT WINAPI IDirect3DVolume9Impl_GetPrivateData(LPDIRECT3DVOLUME9 iface, REFGUID  refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT WINAPI IDirect3DVolume9Impl_FreePrivateData(LPDIRECT3DVOLUME9 iface, REFGUID refguid);
extern HRESULT WINAPI IDirect3DVolume9Impl_GetContainer(LPDIRECT3DVOLUME9 iface, REFIID riid, void** ppContainer);
extern HRESULT WINAPI IDirect3DVolume9Impl_GetDesc(LPDIRECT3DVOLUME9 iface, D3DVOLUME_DESC* pDesc);
extern HRESULT WINAPI IDirect3DVolume9Impl_LockBox(LPDIRECT3DVOLUME9 iface, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
extern HRESULT WINAPI IDirect3DVolume9Impl_UnlockBox(LPDIRECT3DVOLUME9 iface);


/* ------------------- */
/* IDirect3DSwapChain9 */
/* ------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DSwapChain9Vtbl Direct3DSwapChain9_Vtbl;

/*****************************************************************************
 * IDirect3DSwapChain9 implementation structure
 */
typedef struct IDirect3DSwapChain9Impl
{
    /* IUnknown fields */
    const IDirect3DSwapChain9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DSwapChain9 fields */
    IWineD3DSwapChain      *wineD3DSwapChain;
} IDirect3DSwapChain9Impl;

/* ------------------ */
/* IDirect3DResource9 */
/* ------------------ */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DResource9Vtbl Direct3DResource9_Vtbl;

/*****************************************************************************
 * IDirect3DResource9 implementation structure
 */
struct IDirect3DResource9Impl
{
    /* IUnknown fields */
    const IDirect3DResource9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DResource       *wineD3DResource;
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DResource9Impl_QueryInterface(LPDIRECT3DRESOURCE9 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DResource9Impl_AddRef(LPDIRECT3DRESOURCE9 iface);
extern ULONG WINAPI           IDirect3DResource9Impl_Release(LPDIRECT3DRESOURCE9 iface);

/* IDirect3DResource9: */
extern HRESULT  WINAPI        IDirect3DResource9Impl_GetDevice(LPDIRECT3DRESOURCE9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT  WINAPI        IDirect3DResource9Impl_SetPrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DResource9Impl_GetPrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DResource9Impl_FreePrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DResource9Impl_SetPriority(LPDIRECT3DRESOURCE9 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DResource9Impl_GetPriority(LPDIRECT3DRESOURCE9 iface);
extern void     WINAPI        IDirect3DResource9Impl_PreLoad(LPDIRECT3DRESOURCE9 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DResource9Impl_GetType(LPDIRECT3DRESOURCE9 iface);


/* ----------------- */
/* IDirect3DSurface9 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DSurface9Vtbl Direct3DSurface9_Vtbl;

/*****************************************************************************
 * IDirect3DSurface9 implementation structure
 */
struct IDirect3DSurface9Impl
{
    /* IUnknown fields */
    const IDirect3DSurface9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DSurface        *wineD3DSurface;

};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DSurface9Impl_QueryInterface(LPDIRECT3DSURFACE9 iface, REFIID refiid, LPVOID* obj);
extern ULONG WINAPI   IDirect3DSurface9Impl_AddRef(LPDIRECT3DSURFACE9 iface);
extern ULONG WINAPI   IDirect3DSurface9Impl_Release(LPDIRECT3DSURFACE9 iface);

/* IDirect3DSurface9: (Inherited from IDirect3DResource9) */
extern HRESULT WINAPI IDirect3DSurface9Impl_GetDevice(LPDIRECT3DSURFACE9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT WINAPI IDirect3DSurface9Impl_SetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT WINAPI IDirect3DSurface9Impl_GetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, void* pData,DWORD* pSizeOfData);
extern HRESULT WINAPI IDirect3DSurface9Impl_FreePrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid);
extern DWORD   WINAPI IDirect3DSurface9Impl_SetPriority(LPDIRECT3DSURFACE9 iface, DWORD PriorityNew);
extern DWORD   WINAPI IDirect3DSurface9Impl_GetPriority(LPDIRECT3DSURFACE9 iface);
extern void    WINAPI IDirect3DSurface9Impl_PreLoad(LPDIRECT3DSURFACE9 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DSurface9Impl_GetType(LPDIRECT3DSURFACE9 iface);

/* IDirect3DSurface9: */
extern HRESULT WINAPI IDirect3DSurface9Impl_GetContainer(LPDIRECT3DSURFACE9 iface, REFIID riid, void** ppContainer);
extern HRESULT WINAPI IDirect3DSurface9Impl_GetDesc(LPDIRECT3DSURFACE9 iface, D3DSURFACE_DESC *pDesc);
extern HRESULT WINAPI IDirect3DSurface9Impl_LockRect(LPDIRECT3DSURFACE9 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
extern HRESULT WINAPI IDirect3DSurface9Impl_UnlockRect(LPDIRECT3DSURFACE9 iface);
extern HRESULT WINAPI IDirect3DSurface9Impl_GetDC(LPDIRECT3DSURFACE9 iface, HDC* phdc);
extern HRESULT WINAPI IDirect3DSurface9Impl_ReleaseDC(LPDIRECT3DSURFACE9 iface, HDC hdc);


/* ---------------------- */
/* IDirect3DVertexBuffer9 */
/* ---------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVertexBuffer9Vtbl Direct3DVertexBuffer9_Vtbl;

/*****************************************************************************
 * IDirect3DVertexBuffer9 implementation structure
 */
struct IDirect3DVertexBuffer9Impl
{
    /* IUnknown fields */
    const IDirect3DVertexBuffer9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DVertexBuffer   *wineD3DVertexBuffer;
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DVertexBuffer9Impl_QueryInterface(LPDIRECT3DVERTEXBUFFER9 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DVertexBuffer9Impl_AddRef(LPDIRECT3DVERTEXBUFFER9 iface);
extern ULONG WINAPI           IDirect3DVertexBuffer9Impl_Release(LPDIRECT3DVERTEXBUFFER9 iface);

/* IDirect3DVertexBuffer9: (Inherited from IDirect3DResource9) */
extern HRESULT  WINAPI        IDirect3DVertexBuffer9Impl_GetDevice(LPDIRECT3DVERTEXBUFFER9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT  WINAPI        IDirect3DVertexBuffer9Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVertexBuffer9Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DVertexBuffer9Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DVertexBuffer9Impl_SetPriority(LPDIRECT3DVERTEXBUFFER9 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DVertexBuffer9Impl_GetPriority(LPDIRECT3DVERTEXBUFFER9 iface);
extern void     WINAPI        IDirect3DVertexBuffer9Impl_PreLoad(LPDIRECT3DVERTEXBUFFER9 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer9Impl_GetType(LPDIRECT3DVERTEXBUFFER9 iface);

/* IDirect3DVertexBuffer9: */
extern HRESULT  WINAPI        IDirect3DVertexBuffer9Impl_Lock(LPDIRECT3DVERTEXBUFFER9 iface, UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVertexBuffer9Impl_Unlock(LPDIRECT3DVERTEXBUFFER9 iface);
extern HRESULT  WINAPI        IDirect3DVertexBuffer9Impl_GetDesc(LPDIRECT3DVERTEXBUFFER9 iface, D3DVERTEXBUFFER_DESC *pDesc);


/* --------------------- */
/* IDirect3DIndexBuffer9 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DIndexBuffer9Vtbl Direct3DIndexBuffer9_Vtbl;

/*****************************************************************************
 * IDirect3DIndexBuffer9 implementation structure
 */
struct IDirect3DIndexBuffer9Impl
{
    /* IUnknown fields */
    const IDirect3DIndexBuffer9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DIndexBuffer    *wineD3DIndexBuffer;
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DIndexBuffer9Impl_QueryInterface(LPDIRECT3DINDEXBUFFER9 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DIndexBuffer9Impl_AddRef(LPDIRECT3DINDEXBUFFER9 iface);
extern ULONG WINAPI           IDirect3DIndexBuffer9Impl_Release(LPDIRECT3DINDEXBUFFER9 iface);

/* IDirect3DIndexBuffer9: (Inherited from IDirect3DResource9) */
extern HRESULT  WINAPI        IDirect3DIndexBuffer9Impl_GetDevice(LPDIRECT3DINDEXBUFFER9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT  WINAPI        IDirect3DIndexBuffer9Impl_SetPrivateData(LPDIRECT3DINDEXBUFFER9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DIndexBuffer9Impl_GetPrivateData(LPDIRECT3DINDEXBUFFER9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DIndexBuffer9Impl_FreePrivateData(LPDIRECT3DINDEXBUFFER9 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DIndexBuffer9Impl_SetPriority(LPDIRECT3DINDEXBUFFER9 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DIndexBuffer9Impl_GetPriority(LPDIRECT3DINDEXBUFFER9 iface);
extern void     WINAPI        IDirect3DIndexBuffer9Impl_PreLoad(LPDIRECT3DINDEXBUFFER9 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DIndexBuffer9Impl_GetType(LPDIRECT3DINDEXBUFFER9 iface);

/* IDirect3DIndexBuffer9: */
extern HRESULT  WINAPI        IDirect3DIndexBuffer9Impl_Lock(LPDIRECT3DINDEXBUFFER9 iface, UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DIndexBuffer9Impl_Unlock(LPDIRECT3DINDEXBUFFER9 iface);
extern HRESULT  WINAPI        IDirect3DIndexBuffer9Impl_GetDesc(LPDIRECT3DINDEXBUFFER9 iface, D3DINDEXBUFFER_DESC *pDesc);


/* --------------------- */
/* IDirect3DBaseTexture9 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DBaseTexture9Vtbl Direct3DBaseTexture9_Vtbl;

/*****************************************************************************
 * IDirect3DBaseTexture9 implementation structure
 */
struct IDirect3DBaseTexture9Impl
{
    /* IUnknown fields */
    const IDirect3DBaseTexture9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DBaseTexture    *wineD3DBaseTexture;
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DBaseTexture9Impl_QueryInterface(LPDIRECT3DBASETEXTURE9 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DBaseTexture9Impl_AddRef(LPDIRECT3DBASETEXTURE9 iface);
extern ULONG WINAPI           IDirect3DBaseTexture9Impl_Release(LPDIRECT3DBASETEXTURE9 iface);

/* IDirect3DBaseTexture9: (Inherited from IDirect3DResource9) */
extern HRESULT  WINAPI        IDirect3DBaseTexture9Impl_GetDevice(LPDIRECT3DBASETEXTURE9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT  WINAPI        IDirect3DBaseTexture9Impl_SetPrivateData(LPDIRECT3DBASETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DBaseTexture9Impl_GetPrivateData(LPDIRECT3DBASETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DBaseTexture9Impl_FreePrivateData(LPDIRECT3DBASETEXTURE9 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DBaseTexture9Impl_SetPriority(LPDIRECT3DBASETEXTURE9 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DBaseTexture9Impl_GetPriority(LPDIRECT3DBASETEXTURE9 iface);
extern void     WINAPI        IDirect3DBaseTexture9Impl_PreLoad(LPDIRECT3DBASETEXTURE9 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DBaseTexture9Impl_GetType(LPDIRECT3DBASETEXTURE9 iface);

/* IDirect3DBaseTexture9: */
extern DWORD    WINAPI        IDirect3DBaseTexture9Impl_SetLOD(LPDIRECT3DBASETEXTURE9 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DBaseTexture9Impl_GetLOD(LPDIRECT3DBASETEXTURE9 iface);
extern DWORD    WINAPI        IDirect3DBaseTexture9Impl_GetLevelCount(LPDIRECT3DBASETEXTURE9 iface);
extern HRESULT  WINAPI        IDirect3DBaseTexture9Impl_SetAutoGenFilterType(LPDIRECT3DBASETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType);
extern D3DTEXTUREFILTERTYPE WINAPI IDirect3DBaseTexture9Impl_GetAutoGenFilterType(LPDIRECT3DBASETEXTURE9 iface);
extern void     WINAPI        IDirect3DBaseTexture9Impl_GenerateMipSubLevels(LPDIRECT3DBASETEXTURE9 iface);


/* --------------------- */
/* IDirect3DCubeTexture9 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DCubeTexture9Vtbl Direct3DCubeTexture9_Vtbl;

/*****************************************************************************
 * IDirect3DCubeTexture9 implementation structure
 */
struct IDirect3DCubeTexture9Impl
{
    /* IUnknown fields */
    const IDirect3DCubeTexture9Vtbl *lpVtbl;
    LONG                      ref;

    /* IDirect3DResource9 fields */
    IWineD3DCubeTexture      *wineD3DCubeTexture;
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DCubeTexture9Impl_QueryInterface(LPDIRECT3DCUBETEXTURE9 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DCubeTexture9Impl_AddRef(LPDIRECT3DCUBETEXTURE9 iface);
extern ULONG WINAPI           IDirect3DCubeTexture9Impl_Release(LPDIRECT3DCUBETEXTURE9 iface);

/* IDirect3DCubeTexture9: (Inherited from IDirect3DResource9) */
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_GetDevice(LPDIRECT3DCUBETEXTURE9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_SetPrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_GetPrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_FreePrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DCubeTexture9Impl_SetPriority(LPDIRECT3DCUBETEXTURE9 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DCubeTexture9Impl_GetPriority(LPDIRECT3DCUBETEXTURE9 iface);
extern void     WINAPI        IDirect3DCubeTexture9Impl_PreLoad(LPDIRECT3DCUBETEXTURE9 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DCubeTexture9Impl_GetType(LPDIRECT3DCUBETEXTURE9 iface);

/* IDirect3DCubeTexture9: (Inherited from IDirect3DBaseTexture9) */
extern DWORD    WINAPI        IDirect3DCubeTexture9Impl_SetLOD(LPDIRECT3DCUBETEXTURE9 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DCubeTexture9Impl_GetLOD(LPDIRECT3DCUBETEXTURE9 iface);
extern DWORD    WINAPI        IDirect3DCubeTexture9Impl_GetLevelCount(LPDIRECT3DCUBETEXTURE9 iface);
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_SetAutoGenFilterType(LPDIRECT3DCUBETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType);
extern D3DTEXTUREFILTERTYPE WINAPI IDirect3DCubeTexture9Impl_GetAutoGenFilterType(LPDIRECT3DCUBETEXTURE9 iface);
extern void     WINAPI        IDirect3DCubeTexture9Impl_GenerateMipSubLevels(LPDIRECT3DCUBETEXTURE9 iface);

/* IDirect3DCubeTexture9 */
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_GetLevelDesc(LPDIRECT3DCUBETEXTURE9 iface, UINT Level, D3DSURFACE_DESC* pDesc);
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_GetCubeMapSurface(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9** ppCubeMapSurface);
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_LockRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_UnlockRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, UINT Level);
extern HRESULT  WINAPI        IDirect3DCubeTexture9Impl_AddDirtyRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect);


/* ----------------- */
/* IDirect3DTexture9 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DTexture9Vtbl Direct3DTexture9_Vtbl;

/*****************************************************************************
 * IDirect3DTexture9 implementation structure
 */
struct IDirect3DTexture9Impl
{
    /* IUnknown fields */
    const IDirect3DTexture9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DTexture        *wineD3DTexture;

};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DTexture9Impl_QueryInterface(LPDIRECT3DTEXTURE9 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DTexture9Impl_AddRef(LPDIRECT3DTEXTURE9 iface);
extern ULONG WINAPI           IDirect3DTexture9Impl_Release(LPDIRECT3DTEXTURE9 iface);

/* IDirect3DTexture9: (Inherited from IDirect3DResource9) */
extern HRESULT  WINAPI        IDirect3DTexture9Impl_GetDevice(LPDIRECT3DTEXTURE9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT  WINAPI        IDirect3DTexture9Impl_SetPrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DTexture9Impl_GetPrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DTexture9Impl_FreePrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DTexture9Impl_SetPriority(LPDIRECT3DTEXTURE9 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DTexture9Impl_GetPriority(LPDIRECT3DTEXTURE9 iface);
extern void     WINAPI        IDirect3DTexture9Impl_PreLoad(LPDIRECT3DTEXTURE9 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DTexture9Impl_GetType(LPDIRECT3DTEXTURE9 iface);

/* IDirect3DTexture9: (Inherited from IDirect3DBaseTexture9) */
extern DWORD    WINAPI        IDirect3DTexture9Impl_SetLOD(LPDIRECT3DTEXTURE9 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DTexture9Impl_GetLOD(LPDIRECT3DTEXTURE9 iface);
extern DWORD    WINAPI        IDirect3DTexture9Impl_GetLevelCount(LPDIRECT3DTEXTURE9 iface);
extern HRESULT  WINAPI        IDirect3DTexture9Impl_SetAutoGenFilterType(LPDIRECT3DTEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType);
extern D3DTEXTUREFILTERTYPE WINAPI IDirect3DTexture9Impl_GetAutoGenFilterType(LPDIRECT3DTEXTURE9 iface);
extern void     WINAPI         IDirect3DTexture9Impl_GenerateMipSubLevels(LPDIRECT3DTEXTURE9 iface);

/* IDirect3DTexture9: */
extern HRESULT  WINAPI        IDirect3DTexture9Impl_GetLevelDesc(LPDIRECT3DTEXTURE9 iface, UINT Level, D3DSURFACE_DESC* pDesc);
extern HRESULT  WINAPI        IDirect3DTexture9Impl_GetSurfaceLevel(LPDIRECT3DTEXTURE9 iface, UINT Level, IDirect3DSurface9** ppSurfaceLevel);
extern HRESULT  WINAPI        IDirect3DTexture9Impl_LockRect(LPDIRECT3DTEXTURE9 iface, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DTexture9Impl_UnlockRect(LPDIRECT3DTEXTURE9 iface, UINT Level);
extern HRESULT  WINAPI        IDirect3DTexture9Impl_AddDirtyRect(LPDIRECT3DTEXTURE9 iface, CONST RECT* pDirtyRect);


/* ----------------------- */
/* IDirect3DVolumeTexture9 */
/* ----------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVolumeTexture9Vtbl Direct3DVolumeTexture9_Vtbl;

/*****************************************************************************
 * IDirect3DVolumeTexture9 implementation structure
 */
struct IDirect3DVolumeTexture9Impl
{
    /* IUnknown fields */
    const IDirect3DVolumeTexture9Vtbl *lpVtbl;
    LONG                        ref;

    /* IDirect3DResource9 fields */
    IWineD3DVolumeTexture      *wineD3DVolumeTexture;
};

/* IUnknown: */
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_QueryInterface(LPDIRECT3DVOLUMETEXTURE9 iface,REFIID refiid,LPVOID *obj);
extern ULONG    WINAPI        IDirect3DVolumeTexture9Impl_AddRef(LPDIRECT3DVOLUMETEXTURE9 iface);
extern ULONG    WINAPI        IDirect3DVolumeTexture9Impl_Release(LPDIRECT3DVOLUMETEXTURE9 iface);

/* IDirect3DVolumeTexture9: (Inherited from IDirect3DResource9) */
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_GetDevice(LPDIRECT3DVOLUMETEXTURE9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_SetPrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DVolumeTexture9Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE9 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DVolumeTexture9Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE9 iface);
extern void     WINAPI        IDirect3DVolumeTexture9Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE9 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture9Impl_GetType(LPDIRECT3DVOLUMETEXTURE9 iface);

/* IDirect3DVolumeTexture9: (Inherited from IDirect3DBaseTexture9) */
extern DWORD    WINAPI        IDirect3DVolumeTexture9Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE9 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DVolumeTexture9Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE9 iface);
extern DWORD    WINAPI        IDirect3DVolumeTexture9Impl_GetLevelCount(LPDIRECT3DVOLUMETEXTURE9 iface);
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_SetAutoGenFilterType(LPDIRECT3DVOLUMETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType);
extern D3DTEXTUREFILTERTYPE WINAPI IDirect3DVolumeTexture9Impl_GetAutoGenFilterType(LPDIRECT3DVOLUMETEXTURE9 iface);
extern void     WINAPI         IDirect3DVolumeTexture9Impl_GenerateMipSubLevels(LPDIRECT3DVOLUMETEXTURE9 iface);

/* IDirect3DVolumeTexture9: */
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_GetLevelDesc(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, D3DVOLUME_DESC *pDesc);
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_GetVolumeLevel(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, IDirect3DVolume9** ppVolumeLevel);
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_LockBox(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level);
extern HRESULT  WINAPI        IDirect3DVolumeTexture9Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE9 iface, CONST D3DBOX* pDirtyBox);


/* ----------------------- */
/* IDirect3DStateBlock9 */
/* ----------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DStateBlock9Vtbl Direct3DStateBlock9_Vtbl;

/*****************************************************************************
 * IDirect3DStateBlock9 implementation structure
 */
typedef struct  IDirect3DStateBlock9Impl {
  /* IUnknown fields */
  const IDirect3DStateBlock9Vtbl *lpVtbl;
  LONG                      ref;

  /* IDirect3DStateBlock9 fields */
  IWineD3DStateBlock       *wineD3DStateBlock;
} IDirect3DStateBlock9Impl;


/* --------------------------- */
/* IDirect3DVertexDeclaration9 */
/* --------------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVertexDeclaration9Vtbl Direct3DVertexDeclaration9_Vtbl;

/*****************************************************************************
 * IDirect3DVertexShaderDeclaration implementation structure
 */
struct IDirect3DVertexDeclaration9Impl {
  /* IUnknown fields */
  const IDirect3DVertexDeclaration9Vtbl *lpVtbl;
  LONG    ref;

  /* IDirect3DVertexDeclaration9 fields */
  IWineD3DVertexDeclaration *wineD3DVertexDeclaration;
};

/* IUnknown: */
extern HRESULT  WINAPI      IDirect3DVertexDeclaration9Impl_QueryInterface(LPDIRECT3DVERTEXDECLARATION9 iface, REFIID refiid, LPVOID* obj);
extern ULONG    WINAPI      IDirect3DVertexDeclaration9Impl_AddRef(LPDIRECT3DVERTEXDECLARATION9 iface);
extern ULONG    WINAPI      IDirect3DVertexDeclaration9Impl_Release(LPDIRECT3DVERTEXDECLARATION9 iface);

/* IDirect3DVertexDeclaration9: */
extern HRESULT  WINAPI      IDirect3DVertexDeclaration9Impl_GetDevice(LPDIRECT3DVERTEXDECLARATION9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT  WINAPI      IDirect3DVertexDeclaration9Impl_GetDeclaration(LPDIRECT3DVERTEXDECLARATION9 iface, D3DVERTEXELEMENT9* pDecl, UINT* pNumElements);


/* ---------------------- */
/* IDirect3DVertexShader9 */
/* ---------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVertexShader9Vtbl Direct3DVertexShader9_Vtbl;

/*****************************************************************************
 * IDirect3DVertexShader implementation structure
 */
struct IDirect3DVertexShader9Impl {
  /* IUnknown fields */
  const IDirect3DVertexShader9Vtbl *lpVtbl;
  LONG  ref;

  /* IDirect3DVertexDeclaration9 fields */
  IDirect3DDevice9Impl* Device;

  DWORD* function;
  UINT functionLength;
  DWORD usage; /* 0 || D3DUSAGE_SOFTWAREPROCESSING */
  DWORD version;
  /* run time datas */
  /*
  VSHADERDATA* data;
  VSHADERINPUTDATA input;
  VSHADEROUTPUTDATA output;
  */
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DVertexShader9Impl_QueryInterface(LPDIRECT3DVERTEXSHADER9 iface, REFIID refiid, LPVOID* obj);
extern ULONG WINAPI           IDirect3DVertexShader9Impl_AddRef(LPDIRECT3DVERTEXSHADER9 iface);
extern ULONG WINAPI           IDirect3DVertexShader9Impl_Release(LPDIRECT3DVERTEXSHADER9 iface);

/* IDirect3DVertexShader9: */
extern HRESULT WINAPI         IDirect3DVertexShader9Impl_GetDevice(LPDIRECT3DVERTEXSHADER9 iface, IDirect3DDevice9** ppDevice);
extern HRESULT WINAPI         IDirect3DVertexShader9Impl_GetFunction(LPDIRECT3DVERTEXSHADER9 iface, VOID* pData, UINT* pSizeOfData);


/* --------------------- */
/* IDirect3DPixelShader9 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DPixelShader9Vtbl Direct3DPixelShader9_Vtbl;

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
typedef struct IDirect3DPixelShader9Impl {
  /* IUnknown fields */
    const IDirect3DPixelShader9Vtbl *lpVtbl;
    LONG  ref;

    /* IDirect3DPixelShader9 fields */
    IWineD3DPixelShader       *wineD3DPixelShader;

} IDirect3DPixelShader9Impl;

/* --------------- */
/* IDirect3DQuery9 */
/* --------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DQuery9Vtbl Direct3DQuery9_Vtbl;

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
typedef struct IDirect3DQuery9Impl {
    /* IUnknown fields */
    const IDirect3DQuery9Vtbl *lpVtbl;
    LONG                 ref;

    /* IDirect3DQuery9 fields */
    IWineD3DQuery       *wineD3DQuery;
} IDirect3DQuery9Impl;


/* Callbacks */
extern HRESULT WINAPI D3D9CB_CreateSurface(IUnknown *device, UINT Width, UINT Height, 
                                         WINED3DFORMAT Format, DWORD Usage, D3DPOOL Pool, UINT Level,
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle);

extern HRESULT WINAPI D3D9CB_CreateVolume(IUnknown  *pDevice, UINT Width, UINT Height, UINT Depth, 
                                          WINED3DFORMAT  Format, D3DPOOL Pool, DWORD Usage,
                                          IWineD3DVolume **ppVolume, 
                                          HANDLE   * pSharedHandle);

extern HRESULT WINAPI D3D9CB_CreateDepthStencilSurface(IUnknown *device, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
                                         DWORD MultisampleQuality, BOOL Discard,
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle);

extern HRESULT WINAPI D3D9CB_CreateRenderTarget(IUnknown *device, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
                                         DWORD MultisampleQuality, BOOL Lockable, 
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle);

#endif /* __WINE_D3D9_PRIVATE_H */
