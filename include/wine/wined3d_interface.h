/*
 * Direct3D wine internal public interface file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
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

#ifndef __WINE_WINED3D_INTERFACE_H
#define __WINE_WINED3D_INTERFACE_H

#if !defined( __WINE_CONFIG_H )
# error You must include config.h to use this header
#endif

#if !defined( __WINE_D3D8_H ) && !defined( __WINE_D3D9_H )
# error You must include d3d8.h or d3d9.h header to use this header
#endif

/*****************************************************************
 * THIS FILE MUST NOT CONTAIN X11 or MESA DEFINES 
 * PLEASE USE wine/wined3d_gl.h INSTEAD
 */


/*****************************************************************************
 * WineD3D Structures to be used when d3d8 and d3d9 are incompatible
 */

typedef struct _WINED3DADAPTER_IDENTIFIER {
    char           *Driver;
    char           *Description;
    char           *DeviceName;
    LARGE_INTEGER  *DriverVersion; 
    DWORD          *VendorId;
    DWORD          *DeviceId;
    DWORD          *SubSysId;
    DWORD          *Revision;
    GUID           *DeviceIdentifier;
    DWORD          *WHQLLevel;
} WINED3DADAPTER_IDENTIFIER;

typedef struct _WINED3DPRESENT_PARAMETERS {
    UINT                *BackBufferWidth;
    UINT                *BackBufferHeight;
    D3DFORMAT           *BackBufferFormat;
    UINT                *BackBufferCount;
    D3DMULTISAMPLE_TYPE *MultiSampleType;
    DWORD               *MultiSampleQuality;
    D3DSWAPEFFECT       *SwapEffect;
    HWND                *hDeviceWindow;
    BOOL                *Windowed;
    BOOL                *EnableAutoDepthStencil;
    D3DFORMAT           *AutoDepthStencilFormat;
    DWORD               *Flags;
    UINT                *FullScreen_RefreshRateInHz;
    UINT                *PresentationInterval;
} WINED3DPRESENT_PARAMETERS;

typedef struct IWineD3D               IWineD3D;
typedef struct IWineD3DDevice         IWineD3DDevice;
typedef struct IWineD3DResource       IWineD3DResource;
typedef struct IWineD3DVertexBuffer   IWineD3DVertexBuffer;
typedef struct IWineD3DStateBlock     IWineD3DStateBlock;

/*****************************************************************************
 * WineD3D interface 
 */

#define INTERFACE IWineD3D
DECLARE_INTERFACE_(IWineD3D,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWineD3D methods ***/
    STDMETHOD_(UINT,GetAdapterCount)(THIS) PURE;
    STDMETHOD(RegisterSoftwareDevice)(THIS_ void * pInitializeFunction) PURE;
    STDMETHOD_(HMONITOR,GetAdapterMonitor)(THIS_ UINT Adapter) PURE;
    STDMETHOD_(UINT,GetAdapterModeCount)(THIS_ UINT Adapter, D3DFORMAT Format) PURE;
    STDMETHOD(EnumAdapterModes)(THIS_ UINT  Adapter, UINT  Mode, D3DFORMAT Format, D3DDISPLAYMODE * pMode) PURE;
    STDMETHOD(GetAdapterDisplayMode)(THIS_ UINT  Adapter, D3DDISPLAYMODE * pMode) PURE;
    STDMETHOD(GetAdapterIdentifier)(THIS_ UINT Adapter, DWORD Flags, WINED3DADAPTER_IDENTIFIER* pIdentifier) PURE;
    STDMETHOD(CheckDeviceMultiSampleType)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  SurfaceFormat, BOOL  Windowed, D3DMULTISAMPLE_TYPE  MultiSampleType, DWORD *pQuality) PURE;
    STDMETHOD(CheckDepthStencilMatch)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  AdapterFormat, D3DFORMAT  RenderTargetFormat, D3DFORMAT  DepthStencilFormat) PURE;
    STDMETHOD(CheckDeviceType)(THIS_ UINT  Adapter, D3DDEVTYPE  CheckType, D3DFORMAT  DisplayFormat, D3DFORMAT  BackBufferFormat, BOOL  Windowed) PURE;
    STDMETHOD(CheckDeviceFormat)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType, D3DFORMAT  AdapterFormat, DWORD  Usage, D3DRESOURCETYPE  RType, D3DFORMAT  CheckFormat) PURE;
    STDMETHOD(CheckDeviceFormatConversion)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) PURE;
    STDMETHOD(GetDeviceCaps)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType, void * pCaps) PURE;
    STDMETHOD(CreateDevice)(THIS_ UINT  Adapter, D3DDEVTYPE  DeviceType,HWND  hFocusWindow, DWORD  BehaviorFlags, WINED3DPRESENT_PARAMETERS * pPresentationParameters, IWineD3DDevice ** ppReturnedDeviceInterface) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IWineD3D_QueryInterface(p,a,b)                    (p)->lpVtbl->QueryInterface(p,a,b)
#define IWineD3D_AddRef(p)                                (p)->lpVtbl->AddRef(p)
#define IWineD3D_Release(p)                               (p)->lpVtbl->Release(p)
/*** IWineD3D methods ***/
#define IWineD3D_GetAdapterCount(p)                       (p)->lpVtbl->GetAdapterCount(p)
#define IWineD3D_RegisterSoftwareDevice(p,a)              (p)->lpVtbl->RegisterSoftwareDevice(p,a)
#define IWineD3D_GetAdapterMonitor(p,a)                   (p)->lpVtbl->GetAdapterMonitor(p,a)
#define IWineD3D_GetAdapterModeCount(p,a,b)               (p)->lpVtbl->GetAdapterModeCount(p,a,b)
#define IWineD3D_EnumAdapterModes(p,a,b,c,d)              (p)->lpVtbl->EnumAdapterModes(p,a,b,c,d)
#define IWineD3D_GetAdapterDisplayMode(p,a,b)             (p)->lpVtbl->GetAdapterDisplayMode(p,a,b)
#define IWineD3D_GetAdapterIdentifier(p,a,b,c)            (p)->lpVtbl->GetAdapterIdentifier(p,a,b,c)
#define IWineD3D_CheckDeviceMultiSampleType(p,a,b,c,d,e,f) (p)->lpVtbl->CheckDeviceMultiSampleType(p,a,b,c,d,e,f)
#define IWineD3D_CheckDepthStencilMatch(p,a,b,c,d,e)      (p)->lpVtbl->CheckDepthStencilMatch(p,a,b,c,d,e)
#define IWineD3D_CheckDeviceType(p,a,b,c,d,e)             (p)->lpVtbl->CheckDeviceType(p,a,b,c,d,e)
#define IWineD3D_CheckDeviceFormat(p,a,b,c,d,e,f)         (p)->lpVtbl->CheckDeviceFormat(p,a,b,c,d,e,f)
#define IWineD3D_CheckDeviceFormatConversion(p,a,b,c,d)   (p)->lpVtbl->CheckDeviceFormatConversion(p,a,b,c,d)
#define IWineD3D_GetDeviceCaps(p,a,b,c)                   (p)->lpVtbl->GetDeviceCaps(p,a,b,c)
#define IWineD3D_CreateDevice(p,a,b,c,d,e,f)              (p)->lpVtbl->CreateDevice(p,a,b,c,d,e,f)
#endif

/* Define the main WineD3D entrypoint */
IWineD3D* WINAPI WineDirect3DCreate(UINT SDKVersion, UINT dxVersion);

/*****************************************************************************
 * WineD3DDevice interface 
 */
#define INTERFACE IWineD3DDevice
DECLARE_INTERFACE_(IWineD3DDevice,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWineD3D methods ***/
    STDMETHOD(CreateVertexBuffer)(THIS_ UINT  Length,DWORD  Usage,DWORD  FVF,D3DPOOL  Pool,IWineD3DVertexBuffer **ppVertexBuffer, HANDLE *sharedHandle) PURE;
    STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type, IWineD3DStateBlock **ppStateBlock) PURE;
    STDMETHOD(SetFVF)(THIS_ DWORD  fvf) PURE;
    STDMETHOD(GetFVF)(THIS_ DWORD * pfvf) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IWineD3DDevice_QueryInterface(p,a,b)                    (p)->lpVtbl->QueryInterface(p,a,b)
#define IWineD3DDevice_AddRef(p)                                (p)->lpVtbl->AddRef(p)
#define IWineD3DDevice_Release(p)                               (p)->lpVtbl->Release(p)
/*** IWineD3DDevice methods ***/
#define IWineD3DDevice_CreateVertexBuffer(p,a,b,c,d,e,f)        (p)->lpVtbl->CreateVertexBuffer(p,a,b,c,d,e,f)
#define IWineD3DDevice_CreateStateBlock(p,a,b)                  (p)->lpVtbl->CreateStateBlock(p,a,b)
#define IWineD3DDevice_SetFVF(p,a)                              (p)->lpVtbl->SetFVF(p,a)
#define IWineD3DDevice_GetFVF(p,a)                              (p)->lpVtbl->GetFVF(p,a)
#endif

/*****************************************************************************
 * WineD3DResource interface 
 */
#define INTERFACE IWineD3DResource
DECLARE_INTERFACE_(IWineD3DResource,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWineD3DResource methods ***/
    STDMETHOD(GetDevice)(THIS_ IWineD3DDevice ** ppDevice) PURE;
    STDMETHOD(SetPrivateData)(THIS_ REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID  refguid, void * pData, DWORD * pSizeOfData) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID  refguid) PURE;
    STDMETHOD_(DWORD,SetPriority)(THIS_ DWORD  PriorityNew) PURE;
    STDMETHOD_(DWORD,GetPriority)(THIS) PURE;
    STDMETHOD_(void,PreLoad)(THIS) PURE;
    STDMETHOD_(D3DRESOURCETYPE,GetType)(THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IWineD3DResource_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IWineD3DResource_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IWineD3DResource_Release(p)                   (p)->lpVtbl->Release(p)
/*** IWineD3DResource methods ***/
#define IWineD3DResource_GetDevice(p,a)               (p)->lpVtbl->GetDevice(p,a)
#define IWineD3DResource_SetPrivateData(p,a,b,c,d)    (p)->lpVtbl->SetPrivateData(p,a,b,c,d)
#define IWineD3DResource_GetPrivateData(p,a,b,c)      (p)->lpVtbl->GetPrivateData(p,a,b,c)
#define IWineD3DResource_FreePrivateData(p,a)         (p)->lpVtbl->FreePrivateData(p,a)
#define IWineD3DResource_SetPriority(p,a)             (p)->lpVtbl->SetPriority(p,a)
#define IWineD3DResource_GetPriority(p)               (p)->lpVtbl->GetPriority(p)
#define IWineD3DResource_PreLoad(p)                   (p)->lpVtbl->PreLoad(p)
#define IWineD3DResource_GetType(p)                   (p)->lpVtbl->GetType(p)
#endif

/*****************************************************************************
 * WineD3DVertexBuffer interface 
 */
#define INTERFACE IWineD3DVertexBuffer
DECLARE_INTERFACE_(IWineD3DVertexBuffer,IDirect3DResource8)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWineD3DResource methods ***/
    STDMETHOD(GetDevice)(THIS_ IWineD3DDevice ** ppDevice) PURE;
    STDMETHOD(SetPrivateData)(THIS_ REFGUID  refguid, CONST void * pData, DWORD  SizeOfData, DWORD  Flags) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID  refguid, void * pData, DWORD * pSizeOfData) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID  refguid) PURE;
    STDMETHOD_(DWORD,SetPriority)(THIS_ DWORD  PriorityNew) PURE;
    STDMETHOD_(DWORD,GetPriority)(THIS) PURE;
    STDMETHOD_(void,PreLoad)(THIS) PURE;
    STDMETHOD_(D3DRESOURCETYPE,GetType)(THIS) PURE;
    /*** IWineD3DVertexBuffer methods ***/
    STDMETHOD(Lock)(THIS_ UINT  OffsetToLock, UINT  SizeToLock, BYTE ** ppbData, DWORD  Flags) PURE;
    STDMETHOD(Unlock)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ D3DVERTEXBUFFER_DESC  * pDesc) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IWineD3DVertexBuffer_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IWineD3DVertexBuffer_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IWineD3DVertexBuffer_Release(p)                   (p)->lpVtbl->Release(p)
/*** IWineD3DResource methods ***/
#define IWineD3DVertexBuffer_GetDevice(p,a)               (p)->lpVtbl->GetDevice(p,a)
#define IWineD3DVertexBuffer_SetPrivateData(p,a,b,c,d)    (p)->lpVtbl->SetPrivateData(p,a,b,c,d)
#define IWineD3DVertexBuffer_GetPrivateData(p,a,b,c)      (p)->lpVtbl->GetPrivateData(p,a,b,c)
#define IWineD3DVertexBuffer_FreePrivateData(p,a)         (p)->lpVtbl->FreePrivateData(p,a)
#define IWineD3DVertexBuffer_SetPriority(p,a)             (p)->lpVtbl->SetPriority(p,a)
#define IWineD3DVertexBuffer_GetPriority(p)               (p)->lpVtbl->GetPriority(p)
#define IWineD3DVertexBuffer_PreLoad(p)                   (p)->lpVtbl->PreLoad(p)
#define IWineD3DVertexBuffer_GetType(p)                   (p)->lpVtbl->GetType(p)
/*** IWineD3DVertexBuffer methods ***/
#define IWineD3DVertexBuffer_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
#define IWineD3DVertexBuffer_Unlock(p)                    (p)->lpVtbl->Unlock(p)
#define IWineD3DVertexBuffer_GetDesc(p,a)                 (p)->lpVtbl->GetDesc(p,a)
#endif

/*****************************************************************************
 * WineD3DStateBlock interface 
 */
#define INTERFACE IWineD3DStateBlock
DECLARE_INTERFACE_(IWineD3DStateBlock,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWineD3DStateBlock methods ***/
    STDMETHOD(InitStartupStateBlock)(THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IWineD3DStateBlock_QueryInterface(p,a,b)                (p)->lpVtbl->QueryInterface(p,a,b)
#define IWineD3DStateBlock_AddRef(p)                            (p)->lpVtbl->AddRef(p)
#define IWineD3DStateBlock_Release(p)                           (p)->lpVtbl->Release(p)
/*** IWineD3DStateBlock methods ***/
#define IWineD3DStateBlock_InitStartupStateBlock(p)             (p)->lpVtbl->InitStartupStateBlock(p)
#endif


#if 0 /* FIXME: During porting in from d3d8 - the following will be used */
/*****************************************************************
 * Some defines
 */

/* Device caps */
#define MAX_PALETTES      256
#define MAX_STREAMS       16
#define MAX_CLIPPLANES    D3DMAXUSERCLIPPLANES
#define MAX_LEVELS        256

/* Other useful values */
#define HIGHEST_RENDER_STATE 174
#define HIGHEST_TEXTURE_STATE 29
#define HIGHEST_TRANSFORMSTATE 512
#define D3DSBT_RECORDED 0xfffffffe

#define D3D_VSHADER_MAX_CONSTANTS 96
#define D3D_PSHADER_MAX_CONSTANTS 32

/*****************************************************************
 * Some includes
 */

#include "wine/wined3d_gl.h"
#include "wine/wined3d_types.h"

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

/*****************************************************************
 * Some defines
 */

typedef struct IDirect3DImpl IDirect3DImpl;
typedef struct IDirect3DBaseTextureImpl IDirect3DBaseTextureImpl;
typedef struct IDirect3DVolumeTextureImpl IDirect3DVolumeTextureImpl;
typedef struct IDirect3DDeviceImpl IDirect3DDeviceImpl;
typedef struct IDirect3DTextureImpl IDirect3DTextureImpl;
typedef struct IDirect3DCubeTextureImpl IDirect3DCubeTextureImpl;
typedef struct IDirect3DIndexBufferImpl IDirect3DIndexBufferImpl;
typedef struct IDirect3DSurfaceImpl IDirect3DSurfaceImpl;
typedef struct IDirect3DSwapChainImpl IDirect3DSwapChainImpl;
typedef struct IDirect3DResourceImpl IDirect3DResourceImpl;
typedef struct IDirect3DVolumeImpl IDirect3DVolumeImpl;
typedef struct IDirect3DVertexBufferImpl IDirect3DVertexBufferImpl;
typedef struct IDirect3DStateBlockImpl IDirect3DStateBlockImpl;
typedef struct IDirect3DVertexShaderImpl IDirect3DVertexShaderImpl;
typedef struct IDirect3DPixelShaderImpl IDirect3DPixelShaderImpl;
typedef struct IDirect3DVertexShaderDeclarationImpl IDirect3DVertexShaderDeclarationImpl;


/*************************************************************
 * d3dcore interfaces defs
 */

/** Vertex Shader API */
extern HRESULT WINAPI IDirect3DVertexShaderImpl_ParseProgram(IDirect3DVertexShaderImpl* This, CONST DWORD* pFunction);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetFunction(IDirect3DVertexShaderImpl* This, VOID* pData, UINT* pSizeOfData);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantB(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST BOOL*  pConstantData, UINT BoolCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantI(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST INT*   pConstantData, UINT Vector4iCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantF(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST FLOAT* pConstantData, UINT Vector4fCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantB(IDirect3DVertexShaderImpl* This, UINT StartRegister, BOOL*  pConstantData, UINT BoolCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantI(IDirect3DVertexShaderImpl* This, UINT StartRegister, INT*   pConstantData, UINT Vector4iCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantF(IDirect3DVertexShaderImpl* This, UINT StartRegister, FLOAT* pConstantData, UINT Vector4fCount);
/* internal Interfaces */
extern DWORD   WINAPI IDirect3DVertexShaderImpl_GetVersion(IDirect3DVertexShaderImpl* This);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_ExecuteSW(IDirect3DVertexShaderImpl* This, VSHADERINPUTDATA* input, VSHADEROUTPUTDATA* output);

#endif /* Temporary #if 0 */


#endif
