/*
 * Copyright (C) 2002 Jason Edmeades
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

#ifndef __WINE_D3D8_H
#define __WINE_D3D8_H

#include "objbase.h"

#include "d3d8types.h"
#include "d3d8caps.h"

/*****************************************************************************
 * #defines and error codes
 */
#define D3DADAPTER_DEFAULT           0
#define D3DENUM_NO_WHQL_LEVEL        2

#define _FACD3D  0x876
#define MAKE_D3DHRESULT( code )  MAKE_HRESULT( 1, _FACD3D, code )

/*
 * Direct3D Errors
 */
#define D3D_OK                                  S_OK
#define D3DERR_WRONGTEXTUREFORMAT               MAKE_D3DHRESULT(2072)
#define D3DERR_UNSUPPORTEDCOLOROPERATION        MAKE_D3DHRESULT(2073)
#define D3DERR_UNSUPPORTEDCOLORARG              MAKE_D3DHRESULT(2074)
#define D3DERR_UNSUPPORTEDALPHAOPERATION        MAKE_D3DHRESULT(2075)
#define D3DERR_UNSUPPORTEDALPHAARG              MAKE_D3DHRESULT(2076)
#define D3DERR_TOOMANYOPERATIONS                MAKE_D3DHRESULT(2077)
#define D3DERR_CONFLICTINGTEXTUREFILTER         MAKE_D3DHRESULT(2078)
#define D3DERR_UNSUPPORTEDFACTORVALUE           MAKE_D3DHRESULT(2079)
#define D3DERR_CONFLICTINGRENDERSTATE           MAKE_D3DHRESULT(2081)
#define D3DERR_UNSUPPORTEDTEXTUREFILTER         MAKE_D3DHRESULT(2082)
#define D3DERR_CONFLICTINGTEXTUREPALETTE        MAKE_D3DHRESULT(2086)
#define D3DERR_DRIVERINTERNALERROR              MAKE_D3DHRESULT(2087)

#define D3DERR_NOTFOUND                         MAKE_D3DHRESULT(2150)
#define D3DERR_MOREDATA                         MAKE_D3DHRESULT(2151)
#define D3DERR_DEVICELOST                       MAKE_D3DHRESULT(2152)
#define D3DERR_DEVICENOTRESET                   MAKE_D3DHRESULT(2153)
#define D3DERR_NOTAVAILABLE                     MAKE_D3DHRESULT(2154)
#define D3DERR_OUTOFVIDEOMEMORY                 MAKE_D3DHRESULT(380)
#define D3DERR_INVALIDDEVICE                    MAKE_D3DHRESULT(2155)
#define D3DERR_INVALIDCALL                      MAKE_D3DHRESULT(2156)
#define D3DERR_DRIVERINVALIDCALL                MAKE_D3DHRESULT(2157)

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IDirect3D8,              0x1DD9E8DA,0x1C77,0x4D40,0xB0,0xCF,0x98,0xFE,0xFD,0xFF,0x95,0x12);
typedef struct IDirect3D8                IDirect3D8, *LPDIRECT3D8;

DEFINE_GUID(IID_IDirect3DDevice8,        0x7385E5DF,0x8FE8,0x41D5,0x86,0xB6,0xD7,0xB4,0x85,0x47,0xB6,0xCF);
typedef struct IDirect3DDevice8          IDirect3DDevice8, *LPDIRECT3DDEVICE8;

DEFINE_GUID(IID_IDirect3DResource8,      0x1B36BB7B,0x09B7,0x410A,0xB4,0x45,0x7D,0x14,0x30,0xD7,0xB3,0x3F);
typedef struct IDirect3DResource8        IDirect3DResource8, *LPDIRECT3DRESOURCE8, *PDIRECT3DRESOURCE8;

DEFINE_GUID(IID_IDirect3DVertexBuffer8,  0x8AEEEAC7,0x05F9,0x44D4,0xB5,0x91,0x00,0x0B,0x0D,0xF1,0xCB,0x95);
typedef struct IDirect3DVertexBuffer8    IDirect3DVertexBuffer8, *LPDIRECT3DVERTEXBUFFER8, *PDIRECT3DVERTEXBUFFER8;

DEFINE_GUID(IID_IDirect3DVolume8,        0xBD7349F5,0x14F1,0x42E4,0x9C,0x79,0x97,0x23,0x80,0xDB,0x40,0xC0);
typedef struct IDirect3DVolume8          IDirect3DVolume8, *LPDIRECT3DVOLUME8, *PDIRECT3DVOLUME8;

DEFINE_GUID(IID_IDirect3DSwapChain8,     0x928C088B,0x76B9,0x4C6B,0xA5,0x36,0xA5,0x90,0x85,0x38,0x76,0xCD);
typedef struct IDirect3DSwapChain8       IDirect3DSwapChain8, *LPDIRECT3DSWAPCHAIN8, *PDIRECT3DSWAPCHAIN8;

DEFINE_GUID(IID_IDirect3DSurface8,       0xB96EEBCA,0xB326,0x4EA5,0x88,0x2F,0x2F,0xF5,0xBA,0xE0,0x21,0xDD);
typedef struct IDirect3DSurface8         IDirect3DSurface8, *LPDIRECT3DSURFACE8, *PDIRECT3DSURFACE8;

DEFINE_GUID(IID_IDirect3DIndexBuffer8,   0x0E689C9A,0x053D,0x44A0,0x9D,0x92,0xDB,0x0E,0x3D,0x75,0x0F,0x86);
typedef struct IDirect3DIndexBuffer8     IDirect3DIndexBuffer8, *LPDIRECT3DINDEXBUFFER8, *PDIRECT3DINDEXBUFFER8;

DEFINE_GUID(IID_IDirect3DBaseTexture8,   0xB4211CFA,0x51B9,0x4A9F,0xAB,0x78,0xDB,0x99,0xB2,0xBB,0x67,0x8E);
typedef struct IDirect3DBaseTexture8     IDirect3DBaseTexture8, *LPDIRECT3DBASETEXTURE8, *PDIRECT3DBASETEXTURE8;

DEFINE_GUID(IID_IDirect3DTexture8,       0xE4CDD575,0x2866,0x4F01,0xB1,0x2E,0x7E,0xEC,0xE1,0xEC,0x93,0x58);
typedef struct IDirect3DTexture8         IDirect3DTexture8, *LPDIRECT3DTEXTURE8, *PDIRECT3DTEXTURE8;

DEFINE_GUID(IID_IDirect3DCubeTexture8,   0x3EE5B968,0x2ACA,0x4C34,0x8B,0xB5,0x7E,0x0C,0x3D,0x19,0xB7,0x50);
typedef struct IDirect3DCubeTexture8     IDirect3DCubeTexture8, *LPDIRECT3DCUBETEXTURE8, *PDIRECT3DCUBETEXTURE8;

DEFINE_GUID(IID_IDirect3DVolumeTexture8, 0x4B8AAAFA,0x140F,0x42BA,0x91,0x31,0x59,0x7E,0xAF,0xAA,0x2E,0xAD);
typedef struct IDirect3DVolumeTexture8   IDirect3DVolumeTexture8, *LPDIRECT3DVOLUMETEXTURE8, *PDIRECT3DVOLUMETEXTURE8;

/*****************************************************************************
 * IDirect3D8 interface
 */
#define INTERFACE IDirect3D8
#define IDirect3D8_METHODS \
    /*** IDirect3D8 methods ***/                         \
    STDMETHOD(RegisterSoftwareDevice)(THIS_ void * pInitializeFunction) PURE; \
    STDMETHOD_(UINT,GetAdapterCount             )(THIS) PURE; \
    STDMETHOD(GetAdapterIdentifier)(THIS_ UINT  Adapter, DWORD  Flags, D3DADAPTER_IDENTIFIER8 * pIdentifier) PURE; \
    STDMETHOD_(UINT,GetAdapterModeCount)(THIS_ UINT  Adapter) PURE; \
    STDMETHOD(EnumAdapterModes)(THIS_ UINT  Adapter, UINT  Mode, D3DDISPLAYMODE * pMode) PURE; \
    STDMETHOD(GetAdapterDisplayMode)(THIS_ UINT  Adapter, D3DDISPLAYMODE * pMode) PURE; \
    STDMETHOD(CheckDeviceType)(THIS_ UINT  Adapter, D3DDEVTYPE  CheckType, D3DFORMAT  DisplayFormat, D3DFORMAT  BackBufferFormat, BOOL  Windowed) PURE; \
    STDMETHOD(CheckDeviceFormat)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  AdapterFormat, DWORD  Usage, D3DRESOURCETYPE  RType, D3DFORMAT  CheckFormat) PURE; \
    STDMETHOD(CheckDeviceMultiSampleType)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  SurfaceFormat, BOOL  Windowed, D3DMULTISAMPLE_TYPE  MultiSampleType) PURE; \
    STDMETHOD(CheckDepthStencilMatch)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  AdapterFormat, D3DFORMAT  RenderTargetFormat, D3DFORMAT  DepthStencilFormat) PURE; \
    STDMETHOD(GetDeviceCaps)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType, D3DCAPS8 * pCaps) PURE; \
    STDMETHOD_(HMONITOR,GetAdapterMonitor)(THIS_ UINT  Adapter) PURE; \
    STDMETHOD(CreateDevice)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType,HWND  hFocusWindow, DWORD  BehaviorFlags, D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DDevice8 ** ppReturnedDeviceInterface) PURE;

    /*** IDirect3D8 methods ***/
#define IDirect3D8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3D8_METHODS
ICOM_DEFINE(IDirect3D8,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3D8_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3D8_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3D8_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3D8 methods ***/
#define IDirect3D8_RegisterSoftwareDevice(p,a)              ICOM_CALL1(RegisterSoftwareDevice,p,a)
#define IDirect3D8_GetAdapterCount(p)                       ICOM_CALL (GetAdapterCount,p)
#define IDirect3D8_GetAdapterIdentifier(p,a,b,c)            ICOM_CALL3(GetAdapterIdentifier,p,a,b,c)
#define IDirect3D8_GetAdapterModeCount(p,a)                 ICOM_CALL1(GetAdapterModeCount,p,a)
#define IDirect3D8_EnumAdapterModes(p,a,b,c)                ICOM_CALL3(EnumAdapterModes,p,a,b,c)
#define IDirect3D8_GetAdapterDisplayMode(p,a,b)             ICOM_CALL2(GetAdapterDisplayMode,p,a,b)
#define IDirect3D8_CheckDeviceType(p,a,b,c,d,e)             ICOM_CALL5(CheckDeviceType,p,a,b,c,d,e)
#define IDirect3D8_CheckDeviceFormat(p,a,b,c,d,e,f)         ICOM_CALL6(CheckDeviceFormat,p,a,b,c,d,e,f)
#define IDirect3D8_CheckDeviceMultiSampleType(p,a,b,c,d,e)  ICOM_CALL5(CheckDeviceMultiSampleType,p,a,b,c,d,e)
#define IDirect3D8_CheckDepthStencilMatch(p,a,b,c,d,e)      ICOM_CALL5(CheckDepthStencilMatch,p,a,b,c,d,e)
#define IDirect3D8_GetDeviceCaps(p,a,b,c)                   ICOM_CALL3(GetDeviceCaps,p,a,b,c)
#define IDirect3D8_GetAdapterMonitor(p,a)                   ICOM_CALL1(GetAdapterMonitor,p,a)
#define IDirect3D8_CreateDevice(p,a,b,c,d,e,f)              ICOM_CALL6(CreateDevice,p,a,b,c,d,e,f)

/*****************************************************************************
 * IDirect3DDevice8 interface
 */
#define INTERFACE IDirect3DDevice8
#define IDirect3DDevice8_METHODS \
    /*** IDirect3DDevice8 methods ***/ \
    STDMETHOD(TestCooperativeLevel)(THIS) PURE; \
    STDMETHOD_(UINT,GetAvailableTextureMem)(THIS) PURE; \
    STDMETHOD(ResourceManagerDiscardBytes)(THIS_ DWORD  Bytes) PURE; \
    STDMETHOD(GetDirect3D)(THIS_ IDirect3D8 ** ppD3D8) PURE; \
    STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS8 * pCaps) PURE; \
    STDMETHOD(GetDisplayMode)(THIS_ D3DDISPLAYMODE * pMode) PURE; \
    STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS  * pParameters) PURE; \
    STDMETHOD(SetCursorProperties)(THIS_ UINT  XHotSpot, UINT  YHotSpot, IDirect3DSurface8 * pCursorBitmap) PURE; \
    STDMETHOD_(void,SetCursorPosition)(THIS_ UINT  XScreenSpace, UINT  YScreenSpace,DWORD  Flags) PURE; \
    STDMETHOD_(BOOL,ShowCursor)(THIS_ BOOL  bShow) PURE; \
    STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS * pPresentationParameters, IDirect3DSwapChain8 ** pSwapChain) PURE; \
    STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS * pPresentationParameters) PURE; \
    STDMETHOD(Present)(THIS_ CONST RECT * pSourceRect,CONST RECT * pDestRect,HWND  hDestWindowOverride,CONST RGNDATA * pDirtyRegion) PURE; \
    STDMETHOD(GetBackBuffer)(THIS_ UINT  BackBuffer,D3DBACKBUFFER_TYPE  Type,IDirect3DSurface8 ** ppBackBuffer) PURE; \
    STDMETHOD(GetRasterStatus)(THIS_ D3DRASTER_STATUS * pRasterStatus) PURE; \
    STDMETHOD_(void,SetGammaRamp)(THIS_ DWORD  Flags,CONST D3DGAMMARAMP * pRamp) PURE; \
    STDMETHOD_(void,GetGammaRamp)(THIS_ D3DGAMMARAMP * pRamp) PURE; \
    STDMETHOD(CreateTexture)(THIS_ UINT  Width,UINT  Height,UINT  Levels,DWORD  Usage,D3DFORMAT  Format,D3DPOOL  Pool,IDirect3DTexture8 ** ppTexture) PURE; \
    STDMETHOD(CreateVolumeTexture)(THIS_ UINT  Width,UINT  Height,UINT  Depth,UINT  Levels,DWORD  Usage,D3DFORMAT  Format,D3DPOOL  Pool,IDirect3DVolumeTexture8 ** ppVolumeTexture) PURE; \
    STDMETHOD(CreateCubeTexture)(THIS_ UINT  EdgeLength,UINT  Levels,DWORD  Usage,D3DFORMAT  Format,D3DPOOL  Pool,IDirect3DCubeTexture8 ** ppCubeTexture) PURE; \
    STDMETHOD(CreateVertexBuffer)(THIS_ UINT  Length,DWORD  Usage,DWORD  FVF,D3DPOOL  Pool,IDirect3DVertexBuffer8 ** ppVertexBuffer) PURE; \
    STDMETHOD(CreateIndexBuffer)(THIS_ UINT  Length,DWORD  Usage,D3DFORMAT  Format,D3DPOOL  Pool,IDirect3DIndexBuffer8 ** ppIndexBuffer) PURE; \
    STDMETHOD(CreateRenderTarget)(THIS_ UINT  Width,UINT  Height,D3DFORMAT  Format,D3DMULTISAMPLE_TYPE  MultiSample,BOOL  Lockable,IDirect3DSurface8 ** ppSurface) PURE; \
    STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT  Width,UINT  Height,D3DFORMAT  Format,D3DMULTISAMPLE_TYPE  MultiSample,IDirect3DSurface8 ** ppSurface) PURE; \
    STDMETHOD(CreateImageSurface)(THIS_ UINT  Width,UINT  Height,D3DFORMAT  Format,IDirect3DSurface8 ** ppSurface) PURE; \
    STDMETHOD(CopyRects)(THIS_ IDirect3DSurface8 * pSourceSurface,CONST RECT * pSourceRectsArray,UINT  cRects,IDirect3DSurface8 * pDestinationSurface,CONST POINT * pDestPointsArray) PURE; \
    STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture8 * pSourceTexture,IDirect3DBaseTexture8 * pDestinationTexture) PURE; \
    STDMETHOD(GetFrontBuffer)(THIS_ IDirect3DSurface8 * pDestSurface) PURE; \
    STDMETHOD(SetRenderTarget)(THIS_ IDirect3DSurface8 * pRenderTarget,IDirect3DSurface8 * pNewZStencil) PURE; \
    STDMETHOD(GetRenderTarget)(THIS_ IDirect3DSurface8 ** ppRenderTarget) PURE; \
    STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface8 ** ppZStencilSurface) PURE; \
    STDMETHOD(BeginScene)(THIS) PURE; \
    STDMETHOD(EndScene)(THIS) PURE; \
    STDMETHOD(Clear)(THIS_ DWORD  Count,CONST D3DRECT * pRects,DWORD  Flags,D3DCOLOR  Color,float  Z,DWORD  Stencil) PURE; \
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE  State,CONST D3DMATRIX * pMatrix) PURE; \
    STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE  State,D3DMATRIX * pMatrix) PURE; \
    STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE  State, CONST D3DMATRIX * pMatrix) PURE; \
    STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT8 * pViewport) PURE; \
    STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT8 * pViewport) PURE; \
    STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL8 * pMaterial) PURE; \
    STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL8 *pMaterial) PURE; \
    STDMETHOD(SetLight)(THIS_ DWORD  Index,CONST D3DLIGHT8 * pLight) PURE; \
    STDMETHOD(GetLight)(THIS_ DWORD  Index,D3DLIGHT8 * pLight) PURE; \
    STDMETHOD(LightEnable)(THIS_ DWORD  Index,BOOL  Enable) PURE; \
    STDMETHOD(GetLightEnable)(THIS_ DWORD  Index,BOOL * pEnable) PURE; \
    STDMETHOD(SetClipPlane)(THIS_ DWORD  Index,CONST float * pPlane) PURE; \
    STDMETHOD(GetClipPlane)(THIS_ DWORD  Index,float * pPlane) PURE; \
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE  State,DWORD  Value) PURE; \
    STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE  State,DWORD * pValue) PURE; \
    STDMETHOD(BeginStateBlock)(THIS) PURE; \
    STDMETHOD(EndStateBlock)(THIS_ DWORD * pToken) PURE; \
    STDMETHOD(ApplyStateBlock)(THIS_ DWORD  Token) PURE; \
    STDMETHOD(CaptureStateBlock)(THIS_ DWORD  Token) PURE; \
    STDMETHOD(DeleteStateBlock)(THIS_ DWORD  Token) PURE; \
    STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE  Type,DWORD * pToken) PURE; \
    STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS8 * pClipStatus) PURE; \
    STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS8 * pClipStatus) PURE; \
    STDMETHOD(GetTexture)(THIS_ DWORD  Stage,IDirect3DBaseTexture8 ** ppTexture) PURE; \
    STDMETHOD(SetTexture)(THIS_ DWORD  Stage,IDirect3DBaseTexture8 * pTexture) PURE; \
    STDMETHOD(GetTextureStageState)(THIS_ DWORD  Stage,D3DTEXTURESTAGESTATETYPE  Type,DWORD * pValue) PURE; \
    STDMETHOD(SetTextureStageState)(THIS_ DWORD  Stage,D3DTEXTURESTAGESTATETYPE  Type,DWORD  Value) PURE; \
    STDMETHOD(ValidateDevice)(THIS_ DWORD * pNumPasses) PURE; \
    STDMETHOD(GetInfo)(THIS_ DWORD  DevInfoID,void * pDevInfoStruct,DWORD  DevInfoStructSize) PURE; \
    STDMETHOD(SetPaletteEntries)(THIS_ UINT  PaletteNumber,CONST PALETTEENTRY * pEntries) PURE; \
    STDMETHOD(GetPaletteEntries)(THIS_ UINT  PaletteNumber,PALETTEENTRY * pEntries) PURE; \
    STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT  PaletteNumber) PURE; \
    STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT  * PaletteNumber) PURE; \
    STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE  PrimitiveType,UINT  StartVertex,UINT  PrimitiveCount) PURE; \
    STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE  PrimitiveType,UINT  minIndex,UINT  NumVertices,UINT  startIndex,UINT  primCount) PURE; \
    STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE  PrimitiveType,UINT  PrimitiveCount,CONST void * pVertexStreamZeroData,UINT  VertexStreamZeroStride) PURE; \
    STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE  PrimitiveType,UINT  MinVertexIndex,UINT  NumVertexIndices,UINT  PrimitiveCount,CONST void * pIndexData,D3DFORMAT  IndexDataFormat,CONST void * pVertexStreamZeroData,UINT  VertexStreamZeroStride) PURE; \
    STDMETHOD(ProcessVertices)(THIS_ UINT  SrcStartIndex,UINT  DestIndex,UINT  VertexCount,IDirect3DVertexBuffer8 * pDestBuffer,DWORD  Flags) PURE; \
    STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD * pDeclaration,CONST DWORD * pFunction,DWORD * pHandle,DWORD  Usage) PURE; \
    STDMETHOD(SetVertexShader)(THIS_ DWORD  Handle) PURE; \
    STDMETHOD(GetVertexShader)(THIS_ DWORD * pHandle) PURE; \
    STDMETHOD(DeleteVertexShader)(THIS_ DWORD  Handle) PURE; \
    STDMETHOD(SetVertexShaderConstant)(THIS_ DWORD  Register,CONST void * pConstantData,DWORD  ConstantCount) PURE; \
    STDMETHOD(GetVertexShaderConstant)(THIS_ DWORD  Register,void * pConstantData,DWORD  ConstantCount) PURE; \
    STDMETHOD(GetVertexShaderDeclaration)(THIS_ DWORD  Handle,void * pData,DWORD * pSizeOfData) PURE; \
    STDMETHOD(GetVertexShaderFunction)(THIS_ DWORD  Handle,void * pData,DWORD * pSizeOfData) PURE; \
    STDMETHOD(SetStreamSource)(THIS_ UINT  StreamNumber,IDirect3DVertexBuffer8 * pStreamData,UINT  Stride) PURE; \
    STDMETHOD(GetStreamSource)(THIS_ UINT  StreamNumber,IDirect3DVertexBuffer8 ** ppStreamData,UINT * pStride) PURE; \
    STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer8 * pIndexData,UINT  BaseVertexIndex) PURE; \
    STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer8 ** ppIndexData,UINT * pBaseVertexIndex) PURE; \
    STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD * pFunction,DWORD * pHandle) PURE; \
    STDMETHOD(SetPixelShader)(THIS_ DWORD  Handle) PURE; \
    STDMETHOD(GetPixelShader)(THIS_ DWORD * pHandle) PURE; \
    STDMETHOD(DeletePixelShader)(THIS_ DWORD  Handle) PURE; \
    STDMETHOD(SetPixelShaderConstant)(THIS_ DWORD  Register,CONST void * pConstantData,DWORD  ConstantCount) PURE; \
    STDMETHOD(GetPixelShaderConstant)(THIS_ DWORD  Register,void * pConstantData,DWORD  ConstantCount) PURE; \
    STDMETHOD(GetPixelShaderFunction)(THIS_ DWORD  Handle,void * pData,DWORD * pSizeOfData) PURE; \
    STDMETHOD(DrawRectPatch)(THIS_ UINT  Handle,CONST float * pNumSegs,CONST D3DRECTPATCH_INFO * pRectPatchInfo) PURE; \
    STDMETHOD(DrawTriPatch)(THIS_ UINT  Handle,CONST float * pNumSegs,CONST D3DTRIPATCH_INFO * pTriPatchInfo) PURE; \
    STDMETHOD(DeletePatch)(THIS_ UINT  Handle) PURE; \

    /*** IDirect3DDevice8 methods ***/
#define IDirect3DDevice8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DDevice8_METHODS
ICOM_DEFINE(IDirect3DDevice8,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DDevice8_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DDevice8_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DDevice8_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3DDevice8 methods ***/
#define IDirect3DDevice8_TestCooperativeLevel(p)                   ICOM_CALL (TestCooperativeLevel,p)
#define IDirect3DDevice8_GetAvailableTextureMem(p)                 ICOM_CALL (GetAvailableTextureMem,p)
#define IDirect3DDevice8_ResourceManagerDiscardBytes(p,a)          ICOM_CALL1(ResourceManagerDiscardBytes,p,a)
#define IDirect3DDevice8_GetDirect3D(p,a)                          ICOM_CALL1(GetDirect3D,p,a)
#define IDirect3DDevice8_GetDeviceCaps(p,a)                        ICOM_CALL1(GetDeviceCaps,p,a)
#define IDirect3DDevice8_GetDisplayMode(p,a)                       ICOM_CALL1(GetDisplayMode,p,a)
#define IDirect3DDevice8_GetCreationParameters(p,a)                ICOM_CALL1(GetCreationParameters,p,a)
#define IDirect3DDevice8_SetCursorProperties(p,a,b,c)              ICOM_CALL3(SetCursorProperties,p,a,b,c)
#define IDirect3DDevice8_SetCursorPosition(p,a,b,c)                ICOM_CALL3(SetCursorPosition,p,a,b,c)
#define IDirect3DDevice8_ShowCursor(p,a)                           ICOM_CALL1(ShowCursor,p,a)
#define IDirect3DDevice8_CreateAdditionalSwapChain(p,a,b)          ICOM_CALL2(CreateAdditionalSwapChain,p,a,b)
#define IDirect3DDevice8_Reset(p,a)                                ICOM_CALL1(Reset,p,a)
#define IDirect3DDevice8_Present(p,a,b,c,d)                        ICOM_CALL4,present,p,a,b,c,d)
#define IDirect3DDevice8_GetBackBuffer(p,a,b,c)                    ICOM_CALL3(GetBackBuffer,p,a,b,c)
#define IDirect3DDevice8_GetRasterStatus(p,a)                      ICOM_CALL1(GetRasterStatus,p,a)
#define IDirect3DDevice8_SetGammaRamp(p,a,b)                       ICOM_CALL2(SetGammaRamp,p,a,b)
#define IDirect3DDevice8_GetGammaRamp(p,a)                         ICOM_CALL1(GetGammaRamp,p,a)
#define IDirect3DDevice8_CreateTexture(p,a,b,c,d,e,f,g)            ICOM_CALL7(CreateTexture,p,a,b,c,d,e,f,g)
#define IDirect3DDevice8_CreateVolumeTexture(p,a,b,c,d,e,f,g,h)    ICOM_CALL8(CreateVolumeTexture,p,a,b,c,d,e,f,g,h)
#define IDirect3DDevice8_CreateCubeTexture(p,a,b,c,d,e,f)          ICOM_CALL6(CreateCubeTexture,p,a,b,c,d,e,f)
#define IDirect3DDevice8_CreateVertexBuffer(p,a,b,c,d,e)           ICOM_CALL5(CreateVertexBuffer,p,a,b,c,d,e)
#define IDirect3DDevice8_CreateIndexBuffer(p,a,b,c,d,e)            ICOM_CALL5(CreateIndexBuffer,p,a,b,c,d,e)
#define IDirect3DDevice8_CreateRenderTarget(p,a,b,c,d,e,f)         ICOM_CALL6(CreateRenderTarget,p,a,b,c,d,e,f)
#define IDirect3DDevice8_CreateDepthStencilSurface(p,a,b,c,d,e)    ICOM_CALL5(CreateDepthStencilSurface,p,a,b,c,d,e)
#define IDirect3DDevice8_CreateImageSurface(p,a,b,c,d)             ICOM_CALL4(CreateImageSurface,p,a,b,c,d)
#define IDirect3DDevice8_CopyRects(p,a,b,c,d,e)                    ICOM_CALL5(CopyRects,p,a,b,c,d,e)
#define IDirect3DDevice8_UpdateTexture(p,a,b)                      ICOM_CALL2(UpdateTexture,p,a,b)
#define IDirect3DDevice8_GetFrontBuffer(p,a)                       ICOM_CALL1(GetFrontBuffer,p,a)
#define IDirect3DDevice8_SetRenderTarget(p,a,b)                    ICOM_CALL2(SetRenderTarget,p,a,b)
#define IDirect3DDevice8_GetRenderTarget(p,a)                      ICOM_CALL1(GetRenderTarget,p,a)
#define IDirect3DDevice8_GetDepthStencilSurface(p,a)               ICOM_CALL1(GetDepthStencilSurface,p,a)
#define IDirect3DDevice8_BeginScene(p)                             ICOM_CALL (BeginScene,p)
#define IDirect3DDevice8_EndScene(p)                               ICOM_CALL (EndScene,p)
#define IDirect3DDevice8_Clear(p,a,b,c,d,e,f)                      ICOM_CALL6(Clear,p,a,b,c,d,e,f)
#define IDirect3DDevice8_SetTransform(p,a,b)                       ICOM_CALL2(SetTransform,p,a,b)
#define IDirect3DDevice8_GetTransform(p,a,b)                       ICOM_CALL2(GetTransform,p,a,b)
#define IDirect3DDevice8_MultiplyTransform(p,a,b)                  ICOM_CALL2(MultiplyTransform,p,a,b)
#define IDirect3DDevice8_SetViewport(p,a)                          ICOM_CALL1(SetViewport,p,a)
#define IDirect3DDevice8_GetViewport(p,a)                          ICOM_CALL1(GetViewport,p,a)
#define IDirect3DDevice8_SetMaterial(p,a)                          ICOM_CALL1(SetMaterial,p,a)
#define IDirect3DDevice8_GetMaterial(p,a)                          ICOM_CALL1(GetMaterial,p,a)
#define IDirect3DDevice8_SetLight(p,a,b)                           ICOM_CALL2(SetLight,p,a,b)
#define IDirect3DDevice8_GetLight(p,a,b)                           ICOM_CALL2(GetLight,p,a,b)
#define IDirect3DDevice8_LightEnable(p,a,b)                        ICOM_CALL2(LightEnable,p,a,b)
#define IDirect3DDevice8_GetLightEnable(p,a,b)                     ICOM_CALL2(GetLightEnable,p,a,b)
#define IDirect3DDevice8_SetClipPlane(p,a,b)                       ICOM_CALL2(SetClipPlane,p,a,b)
#define IDirect3DDevice8_GetClipPlane(p,a,b)                       ICOM_CALL2(GetClipPlane,p,a,b)
#define IDirect3DDevice8_SetRenderState(p,a,b)                     ICOM_CALL2(SetRenderState,p,a,b)
#define IDirect3DDevice8_GetRenderState(p,a,b)                     ICOM_CALL2(GetRenderState,p,a,b)
#define IDirect3DDevice8_BeginStateBlock(p)                        ICOM_CALL (BeginStateBlock,p)
#define IDirect3DDevice8_EndStateBlock(p,a)                        ICOM_CALL1(EndStateBlock,p,a)
#define IDirect3DDevice8_ApplyStateBlock(p,a)                      ICOM_CALL1(ApplyStateBlock,p,a)
#define IDirect3DDevice8_CaptureStateBlock(p,a)                    ICOM_CALL1(CaptureStateBlock,p,a)
#define IDirect3DDevice8_DeleteStateBlock(p,a)                     ICOM_CALL1(DeleteStateBlock,p,a)
#define IDirect3DDevice8_CreateStateBlock(p,a,b)                   ICOM_CALL2(CreateStateBlock,p,a,b)
#define IDirect3DDevice8_SetClipStatus(p,a)                        ICOM_CALL1(SetClipStatus,p,a)
#define IDirect3DDevice8_GetClipStatus(p,a)                        ICOM_CALL1(GetClipStatus,p,a)
#define IDirect3DDevice8_GetTexture(p,a,b)                         ICOM_CALL2(GetTexture,p,a,b)
#define IDirect3DDevice8_SetTexture(p,a,b)                         ICOM_CALL2(SetTexture,p,a,b)
#define IDirect3DDevice8_GetTextureStageState(p,a,b,c)             ICOM_CALL3(GetTextureStageState,p,a,b,c)
#define IDirect3DDevice8_SetTextureStageState(p,a,b,c)             ICOM_CALL3(SetTextureStageState,p,a,b,c)
#define IDirect3DDevice8_ValidateDevice(p,a)                       ICOM_CALL1(ValidateDevice,p,a)
#define IDirect3DDevice8_GetInfo(p,a,b,c)                          ICOM_CALL3(GetInfo,p,a,b,c)
#define IDirect3DDevice8_SetPaletteEntries(p,a,b)                  ICOM_CALL2(SetPaletteEntries,p,a,b)
#define IDirect3DDevice8_GetPaletteEntries(p,a,b)                  ICOM_CALL2(GetPaletteEntries,p,a,b)
#define IDirect3DDevice8_SetCurrentTexturePalette(p,a)             ICOM_CALL1(SetCurrentTexturePalette,p,a)
#define IDirect3DDevice8_GetCurrentTexturePalette(p,a)             ICOM_CALL1(GetCurrentTexturePalette,p,a)
#define IDirect3DDevice8_DrawPrimitive(p,a,b,c)                    ICOM_CALL3(DrawPrimitive,p,a,b,c)
#define IDirect3DDevice8_DrawIndexedPrimitive(p,a,b,c,d,e)         ICOM_CALL5(DrawIndexedPrimitive,p,a,b,c,d,e)
#define IDirect3DDevice8_DrawPrimitiveUP(p,a,b,c,d)                ICOM_CALL4(DrawPrimitiveUP,p,a,b,c,d)
#define IDirect3DDevice8_DrawIndexedPrimitiveUP(p,a,b,c,d,e,f,g,h) ICOM_CALL8(DrawIndexedPrimitiveUP,p,a,b,c,d,e,f,g,h)
#define IDirect3DDevice8_ProcessVertices(p,a,b,c,d,e)              ICOM_CALL5(processVertices,p,a,b,c,d,e)
#define IDirect3DDevice8_CreateVertexShader(p,a,b,c,d)             ICOM_CALL4(CreateVertexShader,p,a,b,c,d)
#define IDirect3DDevice8_SetVertexShader(p,a)                      ICOM_CALL1(SetVertexShader,p,a)
#define IDirect3DDevice8_GetVertexShader(p,a)                      ICOM_CALL1(GetVertexShader,p,a)
#define IDirect3DDevice8_DeleteVertexShader(p,a)                   ICOM_CALL1(DeleteVertexShader,p,a)
#define IDirect3DDevice8_SetVertexShaderConstant(p,a,b,c)          ICOM_CALL3(SetVertexShaderConstant,p,a,b,c)
#define IDirect3DDevice8_GetVertexShaderConstant(p,a,b,c)          ICOM_CALL3(GetVertexShaderConstant,p,a,b,c)
#define IDirect3DDevice8_GetVertexShaderDeclaration(p,a,b,c)       ICOM_CALL3(GetVertexShaderDeclaration,p,a,b,c)
#define IDirect3DDevice8_GetVertexShaderFunction(p,a,b,c)          ICOM_CALL3(GetVertexShaderFunction,p,a,b,c)
#define IDirect3DDevice8_SetStreamSource(p,a,b,c)                  ICOM_CALL3(SetStreamSource,p,a,b,c)
#define IDirect3DDevice8_GetStreamSource(p,a,b,c)                  ICOM_CALL3(GetStreamSource,p,a,b,c)
#define IDirect3DDevice8_SetIndices(p,a,b)                         ICOM_CALL2(SetIndices,p,a,b)
#define IDirect3DDevice8_GetIndices(p,a,b)                         ICOM_CALL2(GetIndices,p,a,b)
#define IDirect3DDevice8_CreatePixelShader(p,a,b)                  ICOM_CALL2(CreatePixelShader,p,a,b)
#define IDirect3DDevice8_SetPixelShader(p,a)                       ICOM_CALL1(SetPixelShader,p,a)
#define IDirect3DDevice8_GetPixelShader(p,a)                       ICOM_CALL1(GetPixelShader,p,a)
#define IDirect3DDevice8_DeletePixelShader(p,a)                    ICOM_CALL1(DeletePixelShader,p,a)
#define IDirect3DDevice8_SetPixelShaderConstant(p,a,b,c)           ICOM_CALL3(SetPixelShaderConstant,p,a,b,c)
#define IDirect3DDevice8_GetPixelShaderConstant(p,a,b,c)           ICOM_CALL3(GetPixelShaderConstant,p,a,b,c)
#define IDirect3DDevice8_GetPixelShaderFunction(p,a,b,c)           ICOM_CALL3(GetPixelShaderFunction,p,a,b,c)
#define IDirect3DDevice8_DrawRectPatch(p,a,b,c)                    ICOM_CALL3(DrawRectPatch,p,a,b,c)
#define IDirect3DDevice8_DrawTriPatch(p,a,b,c)                     ICOM_CALL3(DrawTriPatch,p,a,b,c)
#define IDirect3DDevice8_DeletePatch(p,a)                          ICOM_CALL1(DeletePatch,p,a)

/*****************************************************************************
 * IDirect3DVolume8 interface
 */
#define INTERFACE IDirect3DVolume8
#define IDirect3DVolume8_METHODS \
    /*** IDirect3DVolume8 methods ***/ \
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8 ** ppDevice) PURE; \
    STDMETHOD(SetPrivateData)(THIS_ REFGUID  refguid,CONST void * pData, DWORD  SizeOfData, DWORD  Flags) PURE; \
    STDMETHOD(GetPrivateData)(THIS_ REFGUID   refguid,void * pData, DWORD * pSizeOfData) PURE; \
    STDMETHOD(FreePrivateData)(THIS_ REFGUID  refguid) PURE; \
    STDMETHOD(GetContainer)(THIS_ REFIID  riid, void ** ppContainer) PURE; \
    STDMETHOD(GetDesc)(THIS_ D3DVOLUME_DESC * pDesc) PURE; \
    STDMETHOD(LockBox)(THIS_ D3DLOCKED_BOX * pLockedVolume,CONST D3DBOX * pBox, DWORD  Flags) PURE; \
    STDMETHOD(UnlockBox)(THIS) PURE; \

    /*** IDirect3DVolume8 methods ***/
#define IDirect3DVolume8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DVolume8_METHODS
ICOM_DEFINE(IDirect3DVolume8,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DVolume8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DVolume8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DVolume8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DVolume8 methods ***/
#define IDirect3DVolume8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DVolume8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DVolume8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DVolume8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DVolume8_GetContainer(p,a,b)          ICOM_CALL2(GetContainer,p,a,b)
#define IDirect3DVolume8_GetDesc(p,a)                 ICOM_CALL1(GetDesc,p,a)
#define IDirect3DVolume8_LockBox(p,a,b,c)             ICOM_CALL3(LockBox,p,a,b,c)
#define IDirect3DVolume8_UnlockBox(p)                 ICOM_CALL (UnlockBox,p)

/*****************************************************************************
 * IDirect3DSwapChain8 interface
 */
#define INTERFACE IDirect3DSwapChain8
#define IDirect3DSwapChain8_METHODS \
    /*** IDirect3DSwapChain8 methods ***/ \
    STDMETHOD(Present)(THIS_ CONST RECT * pSourceRect, CONST RECT * pDestRect, HWND  hDestWindowOverride,CONST RGNDATA * pDirtyRegion) PURE; \
    STDMETHOD(GetBackBuffer)(THIS_ UINT  BackBuffer, D3DBACKBUFFER_TYPE  Type,IDirect3DSurface8 ** ppBackBuffer) PURE; \

    /*** IDirect3DSwapChain8 methods ***/
#define IDirect3DSwapChain8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DSwapChain8_METHODS
ICOM_DEFINE(IDirect3DSwapChain8,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DSwapChain8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DSwapChain8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DSwapChain8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DSwapChain8 methods ***/
#define IDirect3DSwapChain8_Present(p,a,b,c)             ICOM_CALL3(Present,p,a,b,c)
#define IDirect3DSwapChain8_GetBackBuffer(p,a,b,c,d)     ICOM_CALL4(GetBackBuffer,p,a,b,c,d)

/*****************************************************************************
 * IDirect3DSurface8 interface
 */
#define INTERFACE IDirect3DSurface8
#define IDirect3DSurface8_METHODS \
    /*** IDirect3DSurface8 methods ***/ \
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8 ** ppDevice) PURE; \
    STDMETHOD(SetPrivateData)(THIS_ REFGUID  refguid,CONST void * pData,DWORD  SizeOfData,DWORD  Flags) PURE; \
    STDMETHOD(GetPrivateData)(THIS_ REFGUID  refguid,void * pData,DWORD * pSizeOfData) PURE; \
    STDMETHOD(FreePrivateData)(THIS_ REFGUID  refguid) PURE; \
    STDMETHOD(GetContainer)(THIS_ REFIID  riid, void ** ppContainer) PURE; \
    STDMETHOD(GetDesc)(THIS_ D3DSURFACE_DESC * pDesc) PURE; \
    STDMETHOD(LockRect)(THIS_ D3DLOCKED_RECT * pLockedRect, CONST RECT * pRect,DWORD  Flags) PURE; \
    STDMETHOD(UnlockRect)(THIS) PURE; \

    /*** IDirect3DSurface8 methods ***/
#define IDirect3DSurface8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DSurface8_METHODS
ICOM_DEFINE(IDirect3DSurface8,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DSurface8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DSurface8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DSurface8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DSurface8 methods ***/
#define IDirect3DSurface8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DSurface8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DSurface8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DSurface8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DSurface8_GetContainer(p,a,b)          ICOM_CALL2(GetContainer,p,a,b)
#define IDirect3DSurface8_GetDesc(p,a)                 ICOM_CALL1(GetDesc,p,a)
#define IDirect3DSurface8_LockRect(p,a,b,c)            ICOM_CALL3(LockRect,p,a,b,c)
#define IDirect3DSurface8_UnlockRect(p)                ICOM_CALL (UnlockRect,p)

/*****************************************************************************
 * IDirect3DResource8 interface
 */
#define INTERFACE IDirect3DResource8
#define IDirect3DResource8_METHODS \
    /*** IDirect3DResource8 methods ***/ \
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8 ** ppDevice) PURE; \
    STDMETHOD(SetPrivateData)(THIS_ REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags) PURE; \
    STDMETHOD(GetPrivateData)(THIS_ REFGUID  refguid, void * pData, DWORD * pSizeOfData) PURE; \
    STDMETHOD(FreePrivateData)(THIS_ REFGUID  refguid) PURE; \
    STDMETHOD_(DWORD,SetPriority)(THIS_ DWORD  PriorityNew) PURE; \
    STDMETHOD_(DWORD,GetPriority)(THIS) PURE; \
    STDMETHOD_(void,PreLoad)(THIS) PURE; \
    STDMETHOD_(D3DRESOURCETYPE,GetType)(THIS) PURE;

    /*** IDirect3DResource8 methods ***/
#define IDirect3DResource8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DResource8_METHODS
ICOM_DEFINE(IDirect3DResource8,IUnknown)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DResource8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DResource8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DResource8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DResource8 methods ***/
#define IDirect3DResource8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DResource8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DResource8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DResource8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DResource8_SetPriority(p,a)             ICOM_CALL1(SetPriority,p,a)
#define IDirect3DResource8_GetPriority(p)               ICOM_CALL (GetPriority,p)
#define IDirect3DResource8_PreLoad(p)                   ICOM_CALL (PreLoad,p)
#define IDirect3DResource8_GetType(p)                   ICOM_CALL (GetType,p)

/*****************************************************************************
 * IDirect3DVertexBuffer8 interface
 */
#define INTERFACE IDirect3DVertexBuffer8
#define IDirect3DVertexBuffer8_METHODS \
    /*** IDirect3DVertexBuffer8 methods ***/ \
    STDMETHOD(Lock)(THIS_ UINT  OffsetToLock, UINT  SizeToLock, BYTE ** ppbData, DWORD  Flags) PURE; \
    STDMETHOD(Unlock)(THIS) PURE; \
    STDMETHOD(GetDesc)(THIS_ D3DVERTEXBUFFER_DESC  * pDesc) PURE;

    /*** IDirect3DVertexBuffer8 methods ***/
#define IDirect3DVertexBuffer8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DResource8_METHODS \
    IDirect3DVertexBuffer8_METHODS
ICOM_DEFINE(IDirect3DVertexBuffer8,IDirect3DResource8)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DVertexBuffer8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DVertexBuffer8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DVertexBuffer8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DVertexBuffer8 methods: IDirect3DResource8 ***/
#define IDirect3DVertexBuffer8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DVertexBuffer8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DVertexBuffer8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DVertexBuffer8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DVertexBuffer8_SetPriority(p,a)             ICOM_CALL1(SetPriority,p,a)
#define IDirect3DVertexBuffer8_GetPriority(p)               ICOM_CALL (GetPriority,p)
#define IDirect3DVertexBuffer8_PreLoad(p)                   ICOM_CALL (PreLoad,p)
#define IDirect3DVertexBuffer8_GetType(p)                   ICOM_CALL (GetType,p)
/*** IDirect3DVertexBuffer8 methods ***/
#define IDirect3DVertexBuffer8_Lock(p,a,b,c,d)              ICOM_CALL4(Lock,p,a,b,c,d)
#define IDirect3DVertexBuffer8_Unlock(p)                    ICOM_CALL (Unlock,p)
#define IDirect3DVertexBuffer8_GetDesc(p,a)                 ICOM_CALL1(GetDesc,p,a)

/*****************************************************************************
 * IDirect3DIndexBuffer8 interface
 */
#define INTERFACE IDirect3DIndexBuffer8
#define IDirect3DIndexBuffer8_METHODS \
    /*** IDirect3DIndexBuffer8 methods ***/ \
    STDMETHOD(Lock)(THIS_ UINT  OffsetToLock, UINT  SizeToLock, BYTE ** ppbData, DWORD  Flags) PURE; \
    STDMETHOD(Unlock)(THIS) PURE; \
    STDMETHOD(GetDesc)(THIS_ D3DINDEXBUFFER_DESC * pDesc) PURE; \

    /*** IDirect3DIndexBuffer8 methods ***/
#define IDirect3DIndexBuffer8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DResource8_METHODS \
    IDirect3DIndexBuffer8_METHODS
ICOM_DEFINE(IDirect3DIndexBuffer8,IDirect3DResource8)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DIndexBuffer8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DIndexBuffer8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DIndexBuffer8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DIndexBuffer8 methods: IDirect3DResource8 ***/
#define IDirect3DIndexBuffer8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DIndexBuffer8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DIndexBuffer8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DIndexBuffer8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DIndexBuffer8_SetPriority(p,a)             ICOM_CALL1(SetPriority,p,a)
#define IDirect3DIndexBuffer8_GetPriority(p)               ICOM_CALL (GetPriority,p)
#define IDirect3DIndexBuffer8_PreLoad(p)                   ICOM_CALL (PreLoad,p)
#define IDirect3DIndexBuffer8_GetType(p)                   ICOM_CALL (GetType,p)
/*** IDirect3DIndexBuffer8 methods ***/
#define IDirect3DIndexBuffer8_Lock(p,a,b,c,d)              ICOM_CALL4(Lock,p,a,b,c,d)
#define IDirect3DIndexBuffer8_Unlock(p)                    ICOM_CALL (Unlock,p)
#define IDirect3DIndexBuffer8_GetDesc(p,a)                 ICOM_CALL1(GetDesc,p,a)

/*****************************************************************************
 * IDirect3DBaseTexture8 interface
 */
#define INTERFACE IDirect3DBaseTexture8
#define IDirect3DBaseTexture8_METHODS \
    /*** IDirect3DBaseTexture8 methods ***/ \
    STDMETHOD_(DWORD,SetLOD)(THIS_ DWORD  LODNew) PURE; \
    STDMETHOD_(DWORD,GetLOD)(THIS) PURE; \
    STDMETHOD_(DWORD,GetLevelCount)(THIS) PURE;

    /*** IDirect3DBaseTexture8 methods ***/
#define IDirect3DBaseTexture8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DResource8_METHODS \
    IDirect3DBaseTexture8_METHODS
ICOM_DEFINE(IDirect3DBaseTexture8,IDirect3DResource8)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DBaseTexture8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DBaseTexture8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DBaseTexture8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DBaseTexture8 methods: IDirect3DResource8 ***/
#define IDirect3DBaseTexture8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DBaseTexture8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DBaseTexture8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DBaseTexture8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DBaseTexture8_SetPriority(p,a)             ICOM_CALL1(SetPriority,p,a)
#define IDirect3DBaseTexture8_GetPriority(p)               ICOM_CALL (GetPriority,p)
#define IDirect3DBaseTexture8_PreLoad(p)                   ICOM_CALL (PreLoad,p)
#define IDirect3DBaseTexture8_GetType(p)                   ICOM_CALL (GetType,p)
/*** IDirect3DBaseTexture8 methods ***/
#define IDirect3DBaseTexture8_SetLOD(p,a)                  ICOM_CALL1(SetLOD,p,a)
#define IDirect3DBaseTexture8_GetLOD(p)                    ICOM_CALL (GetLOD,p)
#define IDirect3DBaseTexture8_GetLevelCount(p)             ICOM_CALL (GetLevelCount,p)

/*****************************************************************************
 * IDirect3DCubeTexture8 interface
 */
#define INTERFACE IDirect3DCubeTexture8
#define IDirect3DCubeTexture8_METHODS \
    /*** IDirect3DCubeTexture8 methods ***/ \
    STDMETHOD(GetLevelDesc)(THIS_ UINT  Level,D3DSURFACE_DESC * pDesc) PURE; \
    STDMETHOD(GetCubeMapSurface)(THIS_ D3DCUBEMAP_FACES  FaceType,UINT  Level,IDirect3DSurface8 ** ppCubeMapSurface) PURE; \
    STDMETHOD(LockRect)(THIS_ D3DCUBEMAP_FACES  FaceType,UINT  Level,D3DLOCKED_RECT * pLockedRect,CONST RECT * pRect,DWORD  Flags) PURE; \
    STDMETHOD(UnlockRect)(THIS_ D3DCUBEMAP_FACES  FaceType,UINT  Level) PURE; \
    STDMETHOD(AddDirtyRect)(THIS_ D3DCUBEMAP_FACES  FaceType,CONST RECT * pDirtyRect) PURE; \

    /*** IDirect3DCubeTexture8 methods ***/
#define IDirect3DCubeTexture8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DResource8_METHODS \
    IDirect3DBaseTexture8_METHODS \
    IDirect3DCubeTexture8_METHODS
ICOM_DEFINE(IDirect3DCubeTexture8,IDirect3DBaseTexture8)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DCubeTexture8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DCubeTexture8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DCubeTexture8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DCubeTexture8 methods: IDirect3DResource8 ***/
#define IDirect3DCubeTexture8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DCubeTexture8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DCubeTexture8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DCubeTexture8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DCubeTexture8_SetPriority(p,a)             ICOM_CALL1(SetPriority,p,a)
#define IDirect3DCubeTexture8_GetPriority(p)               ICOM_CALL (GetPriority,p)
#define IDirect3DCubeTexture8_PreLoad(p)                   ICOM_CALL (PreLoad,p)
#define IDirect3DCubeTexture8_GetType(p)                   ICOM_CALL (GetType,p)
/*** IDirect3DCubeTexture8 methods: IDirect3DBaseTexture8 ***/
#define IDirect3DCubeTexture8_SetLOD(p,a)                  ICOM_CALL1(SetLOD,p,a)
#define IDirect3DCubeTexture8_GetLOD(p)                    ICOM_CALL (GetLOD,p)
#define IDirect3DCubeTexture8_GetLevelCount(p)             ICOM_CALL (GetLevelCount,p)
/*** IDirect3DCubeTexture8 methods ***/
#define IDirect3DCubeTexture8_GetLevelDesc(p,a,b)          ICOM_CALL2(GetLevelDesc,p,a,b)
#define IDirect3DCubeTexture8_GetCubeMapSurface(p,a,b,c)   ICOM_CALL3(GetCubeMapSurface,p,a,b,c)
#define IDirect3DCubeTexture8_LockRect(p,a,b,c,d,e)        ICOM_CALL5(LockRect,p,a,b,c,d,e)
#define IDirect3DCubeTexture8_UnlockRect(p,a,b)            ICOM_CALL2(UnlockRect,p,a,b)
#define IDirect3DCubeTexture8_AddDirtyRect(p,a,b)          ICOM_CALL2(AddDirtyRect,p,a,b)

/*****************************************************************************
 * IDirect3DTexture8 interface
 */
#define INTERFACE IDirect3DTexture8
#define IDirect3DTexture8_METHODS \
    /*** IDirect3DTexture8 methods ***/ \
    STDMETHOD(GetLevelDesc)(THIS_ UINT  Level,D3DSURFACE_DESC * pDesc) PURE; \
    STDMETHOD(GetSurfaceLevel)(THIS_ UINT  Level,IDirect3DSurface8 ** ppSurfaceLevel) PURE; \
    STDMETHOD(LockRect)(THIS_ UINT  Level,D3DLOCKED_RECT * pLockedRect,CONST RECT * pRect,DWORD  Flags) PURE; \
    STDMETHOD(UnlockRect)(THIS_ UINT  Level) PURE; \
    STDMETHOD(AddDirtyRect)(THIS_ CONST RECT * pDirtyRect) PURE; \

    /*** IDirect3DTexture8 methods ***/
#define IDirect3DTexture8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DResource8_METHODS \
    IDirect3DBaseTexture8_METHODS \
    IDirect3DTexture8_METHODS
ICOM_DEFINE(IDirect3DTexture8,IDirect3DBaseTexture8)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DTexture8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DTexture8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DTexture8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DTexture8 methods: IDirect3DResource8 ***/
#define IDirect3DTexture8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DTexture8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DTexture8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DTexture8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DTexture8_SetPriority(p,a)             ICOM_CALL1(SetPriority,p,a)
#define IDirect3DTexture8_GetPriority(p)               ICOM_CALL (GetPriority,p)
#define IDirect3DTexture8_PreLoad(p)                   ICOM_CALL (PreLoad,p)
#define IDirect3DTexture8_GetType(p)                   ICOM_CALL (GetType,p)
/*** IDirect3DTexture8 methods: IDirect3DBaseTexture8 ***/
#define IDirect3DTexture8_SetLOD(p,a)                  ICOM_CALL1(SetLOD,p,a)
#define IDirect3DTexture8_GetLOD(p)                    ICOM_CALL (GetLOD,p)
#define IDirect3DTexture8_GetLevelCount(p)             ICOM_CALL (GetLevelCount,p)
/*** IDirect3DTexture8 methods ***/
#define IDirect3DTexture8_GetLevelDesc(p,a,b)          ICOM_CALL2(GetLevelDesc,p,a,b)
#define IDirect3DTexture8_GetSurfaceLevel(p,a,b)       ICOM_CALL2(GetSurfaceLevel,p,a,b)
#define IDirect3DTexture8_LockRect(p,a,b,c,d)          ICOM_CALL4(LockRect,p,a,b,c,d)
#define IDirect3DTexture8_UnlockRect(p,a)              ICOM_CALL1(UnlockRect,p,a)
#define IDirect3DTexture8_AddDirtyRect(p,a)            ICOM_CALL1(AddDirtyRect,p,a)

/*****************************************************************************
 * IDirect3DVolumeTexture8 interface
 */
#define INTERFACE IDirect3DVolumeTexture8
#define IDirect3DVolumeTexture8_METHODS \
    /*** IDirect3DVolumeTexture8 methods ***/ \
    STDMETHOD(GetLevelDesc)(THIS_ UINT  Level,D3DVOLUME_DESC * pDesc) PURE; \
    STDMETHOD(GetVolumeLevel)(THIS_ UINT  Level,IDirect3DVolume8 ** ppVolumeLevel) PURE; \
    STDMETHOD(LockBox)(THIS_ UINT  Level,D3DLOCKED_BOX * pLockedVolume,CONST D3DBOX * pBox,DWORD  Flags) PURE; \
    STDMETHOD(UnlockBox)(THIS_ UINT  Level) PURE; \
    STDMETHOD(AddDirtyBox)(THIS_ CONST D3DBOX * pDirtyBox) PURE;

    /*** IDirect3DVolumeTexture8 methods ***/
#define IDirect3DVolumeTexture8_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DResource8_METHODS \
    IDirect3DBaseTexture8_METHODS \
    IDirect3DVolumeTexture8_METHODS
ICOM_DEFINE(IDirect3DVolumeTexture8,IDirect3DBaseTexture8)
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DVolumeTexture8_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DVolumeTexture8_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IDirect3DVolumeTexture8_Release(p)                   ICOM_CALL (Release,p)
/*** IDirect3DVolumeTexture8 methods: IDirect3DResource8 ***/
#define IDirect3DVolumeTexture8_GetDevice(p,a)               ICOM_CALL1(GetDevice,p,a)
#define IDirect3DVolumeTexture8_SetPrivateData(p,a,b,c,d)    ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirect3DVolumeTexture8_GetPrivateData(p,a,b,c)      ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirect3DVolumeTexture8_FreePrivateData(p,a)         ICOM_CALL1(FreePrivateData,p,a)
#define IDirect3DVolumeTexture8_SetPriority(p,a)             ICOM_CALL1(SetPriority,p,a)
#define IDirect3DVolumeTexture8_GetPriority(p)               ICOM_CALL (GetPriority,p)
#define IDirect3DVolumeTexture8_PreLoad(p)                   ICOM_CALL (PreLoad,p)
#define IDirect3DVolumeTexture8_GetType(p)                   ICOM_CALL (GetType,p)
/*** IDirect3DVolumeTexture8 methods: IDirect3DBaseTexture8 ***/
#define IDirect3DVolumeTexture8_SetLOD(p,a)                  ICOM_CALL1(SetLOD,p,a)
#define IDirect3DVolumeTexture8_GetLOD(p)                    ICOM_CALL (GetLOD,p)
#define IDirect3DVolumeTexture8_GetLevelCount(p)             ICOM_CALL (GetLevelCount,p)
/*** IDirect3DVolumeTexture8 methods ***/
#define IDirect3DVolumeTexture8_GetLevelDesc(p,a,b)          ICOM_CALL2(GetLevelDesc,p,a,b)
#define IDirect3DVolumeTexture8_GetVolumeLevel(p,a,b)        ICOM_CALL2(GetVolumeLevel,p,a,b)
#define IDirect3DVolumeTexture8_LockBox(p,a,b,c,d)           ICOM_CALL4(LockBox,p,a,b,c,d)
#define IDirect3DVolumeTexture8_UnlockBox(p,a)               ICOM_CALL1(UnlockBox,p,a)
#define IDirect3DVolumeTexture8_AddDirtyBox(p,a)             ICOM_CALL1(AddDirtyBox,p,a)

#ifdef __cplusplus
extern "C" {
#endif  /* defined(__cplusplus) */

/* Define the main entrypoint as well */
IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_D3D8_H */
