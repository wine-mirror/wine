/*
 * Direct3D 8 private include file
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
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

#ifndef __WINE_D3D8_PRIVATE_H
#define __WINE_D3D8_PRIVATE_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

/* THIS FILE MUST NOT CONTAIN X11 or MESA DEFINES */
#define XMD_H /* This is to prevent the Xmd.h inclusion bug :-/ */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#define GLX_GLXEXT_PROTOTYPES
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
extern int num_lock;
#if 0
#define ENTER_GL() ++num_lock; TRACE("inc lock to: %d\n", num_lock); wine_tsx11_lock_ptr()
#define LEAVE_GL() if (num_lock > 2) TRACE("fucking locks: %d\n", num_lock); --num_lock; wine_tsx11_unlock_ptr()
#else
#define ENTER_GL() wine_tsx11_lock_ptr()
#define LEAVE_GL() wine_tsx11_unlock_ptr()
#endif

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "d3d8.h"
#include "wine/wined3d_interface.h"

extern int vs_mode;
#define VS_NONE 0
#define VS_HW   1
#define VS_SW   2

extern int ps_mode;
#define PS_NONE 0
#define PS_HW   1

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

/* Performance changes - Only reapply what is necessary */
#define REAPPLY_ALPHAOP  0x0001
#define REAPPLY_ALL      0xFFFF

/* CreateVertexShader can return > 0xFFFF */
#define VS_HIGHESTFIXEDFXF 0xF0000000
#define VERTEX_SHADER(Handle) \
  ((Handle <= VS_HIGHESTFIXEDFXF) ? ((Handle >= sizeof(VertexShaders) / sizeof(IDirect3DVertexShaderImpl*)) ? NULL : VertexShaders[Handle]) : VertexShaders[Handle - VS_HIGHESTFIXEDFXF])
#define VERTEX_SHADER_DECL(Handle) \
  ((Handle <= VS_HIGHESTFIXEDFXF) ? ((Handle >= sizeof(VertexShaderDeclarations) / sizeof(IDirect3DVertexShaderDeclarationImpl*)) ? NULL : VertexShaderDeclarations[Handle]) : VertexShaderDeclarations[Handle - VS_HIGHESTFIXEDFXF])
#define PIXEL_SHADER(Handle) \
  ((Handle <= VS_HIGHESTFIXEDFXF) ? ((Handle >= sizeof(PixelShaders) / sizeof(IDirect3DPixelShaderImpl*)) ? NULL : PixelShaders[Handle]) : PixelShaders[Handle - VS_HIGHESTFIXEDFXF])

/* ===========================================================================
    Macros
   =========================================================================== */
/* Not nice, but it lets wined3d support different versions of directx */
#define D3D8CAPSTOWINECAPS(_pD3D8Caps, _pWineCaps) \
    _pWineCaps->DeviceType                        = &_pD3D8Caps->DeviceType; \
    _pWineCaps->AdapterOrdinal                    = &_pD3D8Caps->AdapterOrdinal; \
    _pWineCaps->Caps                              = &_pD3D8Caps->Caps; \
    _pWineCaps->Caps2                             = &_pD3D8Caps->Caps2; \
    _pWineCaps->Caps3                             = &_pD3D8Caps->Caps3; \
    _pWineCaps->PresentationIntervals             = &_pD3D8Caps->PresentationIntervals; \
    _pWineCaps->CursorCaps                        = &_pD3D8Caps->CursorCaps; \
    _pWineCaps->DevCaps                           = &_pD3D8Caps->DevCaps; \
    _pWineCaps->PrimitiveMiscCaps                 = &_pD3D8Caps->PrimitiveMiscCaps; \
    _pWineCaps->RasterCaps                        = &_pD3D8Caps->RasterCaps; \
    _pWineCaps->ZCmpCaps                          = &_pD3D8Caps->ZCmpCaps; \
    _pWineCaps->SrcBlendCaps                      = &_pD3D8Caps->SrcBlendCaps; \
    _pWineCaps->DestBlendCaps                     = &_pD3D8Caps->DestBlendCaps; \
    _pWineCaps->AlphaCmpCaps                      = &_pD3D8Caps->AlphaCmpCaps; \
    _pWineCaps->ShadeCaps                         = &_pD3D8Caps->ShadeCaps; \
    _pWineCaps->TextureCaps                       = &_pD3D8Caps->TextureCaps; \
    _pWineCaps->TextureFilterCaps                 = &_pD3D8Caps->TextureFilterCaps; \
    _pWineCaps->CubeTextureFilterCaps             = &_pD3D8Caps->CubeTextureFilterCaps; \
    _pWineCaps->VolumeTextureFilterCaps           = &_pD3D8Caps->VolumeTextureFilterCaps; \
    _pWineCaps->TextureAddressCaps                = &_pD3D8Caps->TextureAddressCaps; \
    _pWineCaps->VolumeTextureAddressCaps          = &_pD3D8Caps->VolumeTextureAddressCaps; \
    _pWineCaps->LineCaps                          = &_pD3D8Caps->LineCaps; \
    _pWineCaps->MaxTextureWidth                   = &_pD3D8Caps->MaxTextureWidth; \
    _pWineCaps->MaxTextureHeight                  = &_pD3D8Caps->MaxTextureHeight; \
    _pWineCaps->MaxVolumeExtent                   = &_pD3D8Caps->MaxVolumeExtent; \
    _pWineCaps->MaxTextureRepeat                  = &_pD3D8Caps->MaxTextureRepeat; \
    _pWineCaps->MaxTextureAspectRatio             = &_pD3D8Caps->MaxTextureAspectRatio; \
    _pWineCaps->MaxAnisotropy                     = &_pD3D8Caps->MaxAnisotropy; \
    _pWineCaps->MaxVertexW                        = &_pD3D8Caps->MaxVertexW; \
    _pWineCaps->GuardBandLeft                     = &_pD3D8Caps->GuardBandLeft; \
    _pWineCaps->GuardBandTop                      = &_pD3D8Caps->GuardBandTop; \
    _pWineCaps->GuardBandRight                    = &_pD3D8Caps->GuardBandRight; \
    _pWineCaps->GuardBandBottom                   = &_pD3D8Caps->GuardBandBottom; \
    _pWineCaps->ExtentsAdjust                     = &_pD3D8Caps->ExtentsAdjust; \
    _pWineCaps->StencilCaps                       = &_pD3D8Caps->StencilCaps; \
    _pWineCaps->FVFCaps                           = &_pD3D8Caps->FVFCaps; \
    _pWineCaps->TextureOpCaps                     = &_pD3D8Caps->TextureOpCaps; \
    _pWineCaps->MaxTextureBlendStages             = &_pD3D8Caps->MaxTextureBlendStages; \
    _pWineCaps->MaxSimultaneousTextures           = &_pD3D8Caps->MaxSimultaneousTextures; \
    _pWineCaps->VertexProcessingCaps              = &_pD3D8Caps->VertexProcessingCaps; \
    _pWineCaps->MaxActiveLights                   = &_pD3D8Caps->MaxActiveLights; \
    _pWineCaps->MaxUserClipPlanes                 = &_pD3D8Caps->MaxUserClipPlanes; \
    _pWineCaps->MaxVertexBlendMatrices            = &_pD3D8Caps->MaxVertexBlendMatrices; \
    _pWineCaps->MaxVertexBlendMatrixIndex         = &_pD3D8Caps->MaxVertexBlendMatrixIndex; \
    _pWineCaps->MaxPointSize                      = &_pD3D8Caps->MaxPointSize; \
    _pWineCaps->MaxPrimitiveCount                 = &_pD3D8Caps->MaxPrimitiveCount; \
    _pWineCaps->MaxVertexIndex                    = &_pD3D8Caps->MaxVertexIndex; \
    _pWineCaps->MaxStreams                        = &_pD3D8Caps->MaxStreams; \
    _pWineCaps->MaxStreamStride                   = &_pD3D8Caps->MaxStreamStride; \
    _pWineCaps->VertexShaderVersion               = &_pD3D8Caps->VertexShaderVersion; \
    _pWineCaps->MaxVertexShaderConst              = &_pD3D8Caps->MaxVertexShaderConst; \
    _pWineCaps->PixelShaderVersion                = &_pD3D8Caps->PixelShaderVersion; \
    _pWineCaps->PixelShader1xMaxValue             = &_pD3D8Caps->MaxPixelShaderValue;

/* Direct3D8 Interfaces: */
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

/** Private Interfaces: */
typedef struct IDirect3DStateBlockImpl IDirect3DStateBlockImpl;
typedef struct IDirect3DVertexShaderImpl IDirect3DVertexShaderImpl;
typedef struct IDirect3DPixelShaderImpl IDirect3DPixelShaderImpl;
typedef struct IDirect3DVertexShaderDeclarationImpl IDirect3DVertexShaderDeclarationImpl;

typedef struct D3DSHADERVECTOR {
  float x;
  float y;
  float z;
  float w;
} D3DSHADERVECTOR;

typedef struct D3DSHADERSCALAR {
  float x;
} D3DSHADERSCALAR;

#define D3D8_VSHADER_MAX_CONSTANTS 96
typedef D3DSHADERVECTOR VSHADERCONSTANTS8[D3D8_VSHADER_MAX_CONSTANTS];

typedef struct VSHADERDATA8 {
  /** Run Time Shader Function Constants */
  /*D3DXBUFFER* constants;*/
  VSHADERCONSTANTS8 C;
  /** Shader Code as char ... */
  CONST DWORD* code;
  UINT codeLength;
} VSHADERDATA8;

/** temporary here waiting for buffer code */
typedef struct VSHADERINPUTDATA8 {
  D3DSHADERVECTOR V[17];
} VSHADERINPUTDATA8;

/** temporary here waiting for buffer code */
typedef struct VSHADEROUTPUTDATA8 {
  D3DSHADERVECTOR oPos;
  D3DSHADERVECTOR oD[2];
  D3DSHADERVECTOR oT[8];
  D3DSHADERVECTOR oFog;
  D3DSHADERVECTOR oPts;
} VSHADEROUTPUTDATA8;


#define D3D8_PSHADER_MAX_CONSTANTS 32
typedef D3DSHADERVECTOR PSHADERCONSTANTS8[D3D8_PSHADER_MAX_CONSTANTS];

typedef struct PSHADERDATA8 {
  /** Run Time Shader Function Constants */
  /*D3DXBUFFER* constants;*/
  PSHADERCONSTANTS8 C;
  /** Shader Code as char ... */
  CONST DWORD* code;
  UINT codeLength;
} PSHADERDATA8;

/** temporary here waiting for buffer code */
typedef struct PSHADERINPUTDATA8 {
  D3DSHADERVECTOR V[2];
  D3DSHADERVECTOR T[8];
  D3DSHADERVECTOR S[16];
  /*D3DSHADERVECTOR R[12];*/
} PSHADERINPUTDATA8;

/** temporary here waiting for buffer code */
typedef struct PSHADEROUTPUTDATA8 {
  D3DSHADERVECTOR oC[4];
  D3DSHADERVECTOR oDepth;
} PSHADEROUTPUTDATA8;

/* 
 * Private definitions for internal use only
 */
typedef struct PLIGHTINFOEL PLIGHTINFOEL;
struct PLIGHTINFOEL {
    D3DLIGHT8 OriginalParms;
    DWORD     OriginalIndex;
    LONG      glIndex;
    BOOL      lightEnabled;
    BOOL      changed;
    BOOL      enabledChanged;

    /* Converted parms to speed up swapping lights */
    float                         lightPosn[4];
    float                         lightDirn[4];
    float                         exponent;
    float                         cutoff;

    PLIGHTINFOEL *next;
    PLIGHTINFOEL *prev;
};

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
#define vcheckGLcall(A) \
{ \
    GLint err = glGetError();   \
    if (err != GL_NO_ERROR) { \
       FIXME(">>>>>>>>>>>>>>>>> %x from %s @ %s / %d\n", err, A, __FILE__, __LINE__); \
    } else { \
       VTRACE(("%s call ok %s / %d\n", A, __FILE__, __LINE__)); \
    } \
}

#include "d3dcore_gl.h"


#define GL_LIMITS(ExtName)            (This->direct3d8->gl_info.max_##ExtName)
#define GL_SUPPORT(ExtName)           (TRUE == This->direct3d8->gl_info.supported[ExtName])
#define GL_SUPPORT_DEV(ExtName, dev)  (TRUE == (dev)->direct3d8->gl_info.supported[ExtName])
#define GL_EXTCALL(FuncName)          (This->direct3d8->gl_info.FuncName)
#define GL_EXTCALL_DEV(FuncName, dev) ((dev)->direct3d8->gl_info.FuncName)

#define D3DCOLOR_B_R(dw) (((dw) >> 16) & 0xFF)
#define D3DCOLOR_B_G(dw) (((dw) >>  8) & 0xFF)
#define D3DCOLOR_B_B(dw) (((dw) >>  0) & 0xFF)
#define D3DCOLOR_B_A(dw) (((dw) >> 24) & 0xFF)

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

/* ===========================================================================
    The interfactes themselves
   =========================================================================== */

/* ---------- */
/* IDirect3D8 */
/* ---------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3D8Vtbl Direct3D8_Vtbl;

/*****************************************************************************
 * IDirect3D implementation structure
 */
struct IDirect3D8Impl
{
    /* IUnknown fields */
    const IDirect3D8Vtbl   *lpVtbl;
    LONG                   ref;

    /* The WineD3D device */
    IWineD3D               *WineD3D;
    
    /* IDirect3D8 fields */
    GL_Info                 gl_info;
    BOOL                    isGLInfoValid;
    IDirect3D8Impl         *direct3d8;
};

/* IUnknown: */
extern HRESULT WINAPI   IDirect3D8Impl_QueryInterface(LPDIRECT3D8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI     IDirect3D8Impl_AddRef(LPDIRECT3D8 iface);
extern ULONG WINAPI     IDirect3D8Impl_Release(LPDIRECT3D8 iface);

/* IDirect3d8: */
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

/* ---------------- */
/* IDirect3DDevice8 */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DDevice8Vtbl Direct3DDevice8_Vtbl;

/*****************************************************************************
 * IDirect3DDevice8 implementation structure
 */
struct IDirect3DDevice8Impl
{
    /* IUnknown fields */
    const IDirect3DDevice8Vtbl   *lpVtbl;
    LONG                         ref;

    /* IDirect3DDevice8 fields */
    IDirect3D8Impl               *direct3d8;
    IWineD3DDevice               *WineD3DDevice;

    IDirect3DSurface8Impl        *frontBuffer;
    IDirect3DSurface8Impl        *backBuffer;
    IDirect3DSurface8Impl        *depthStencilBuffer;

    IDirect3DSurface8Impl        *renderTarget;
    IDirect3DSurface8Impl        *stencilBufferTarget;

    D3DPRESENT_PARAMETERS         PresentParms;
    D3DDEVICE_CREATION_PARAMETERS CreateParms;

    UINT                          adapterNo;
    D3DDEVTYPE                    devType;

    UINT                          srcBlend;
    UINT                          dstBlend;
    UINT                          alphafunc;
    UINT                          stencilfunc;

    /* State block related */
    BOOL                          isRecordingState;
    IDirect3DStateBlockImpl      *StateBlock;
    IDirect3DStateBlockImpl      *UpdateStateBlock;

    /* palettes texture management */
    PALETTEENTRY                  palettes[MAX_PALETTES][256];
    UINT                          currentPalette;

    BOOL                          texture_shader_active;

    /* Optimization */
    BOOL                          modelview_valid;
    BOOL                          proj_valid;
    BOOL                          view_ident;        /* true iff view matrix is identity                */
    BOOL                          last_was_rhw;      /* true iff last draw_primitive was in xyzrhw mode */
    GLenum                        tracking_parm;     /* Which source is tracking current colour         */
    LONG                          tracking_color;    /* used iff GL_COLOR_MATERIAL was enabled          */
#define                         DISABLED_TRACKING  0  /* Disabled                                 */
#define                         IS_TRACKING        1  /* tracking_parm is tracking diffuse color  */
#define                         NEEDS_TRACKING     2  /* Tracking needs to be enabled when needed */
#define                         NEEDS_DISABLE      3  /* Tracking needs to be disabled when needed*/

    /* OpenGL related */
    GLXContext                    glCtx;
    XVisualInfo                  *visInfo;
    Display                      *display;
    HWND                          win_handle;
    Window                        win;
    GLXContext                    render_ctx;
    Drawable                      drawable;
    GLint                         maxConcurrentLights;

    /* OpenGL Extension related */

    /* Cursor management */
    BOOL                          bCursorVisible;
    UINT                          xHotSpot;
    UINT                          yHotSpot;
    UINT                          xScreenSpace;
    UINT                          yScreenSpace;
    GLint                         cursor;

    UINT                          dummyTextureName[8];

    /* For rendering to a texture using glCopyTexImage */
    BOOL                          renderUpsideDown;
};

/* IUnknown: */
extern HRESULT WINAPI   IDirect3DDevice8Impl_QueryInterface(LPDIRECT3DDEVICE8 iface, REFIID refiid, LPVOID *obj);
extern ULONG WINAPI     IDirect3DDevice8Impl_AddRef(LPDIRECT3DDEVICE8 iface);
extern ULONG WINAPI     IDirect3DDevice8Impl_Release(LPDIRECT3DDEVICE8 iface);

/* IDirect3DDevice8: */
extern HRESULT  WINAPI  IDirect3DDevice8Impl_TestCooperativeLevel(LPDIRECT3DDEVICE8 iface);
extern UINT     WINAPI  IDirect3DDevice8Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE8 iface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_ResourceManagerDiscardBytes(LPDIRECT3DDEVICE8 iface, DWORD Bytes);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetDirect3D(LPDIRECT3DDEVICE8 iface, IDirect3D8** ppD3D8);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetDeviceCaps(LPDIRECT3DDEVICE8 iface, D3DCAPS8* pCaps);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetDisplayMode(LPDIRECT3DDEVICE8 iface, D3DDISPLAYMODE* pMode);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetCreationParameters(LPDIRECT3DDEVICE8 iface, D3DDEVICE_CREATION_PARAMETERS* pParameters);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetCursorProperties(LPDIRECT3DDEVICE8 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap);
extern void     WINAPI  IDirect3DDevice8Impl_SetCursorPosition(LPDIRECT3DDEVICE8 iface, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags);
extern BOOL     WINAPI  IDirect3DDevice8Impl_ShowCursor(LPDIRECT3DDEVICE8 iface, BOOL bShow);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_Reset(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_Present(LPDIRECT3DDEVICE8 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetBackBuffer(LPDIRECT3DDEVICE8 iface, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetRasterStatus(LPDIRECT3DDEVICE8 iface, D3DRASTER_STATUS* pRasterStatus);
extern void     WINAPI  IDirect3DDevice8Impl_SetGammaRamp(LPDIRECT3DDEVICE8 iface, DWORD Flags, CONST D3DGAMMARAMP* pRamp);
extern void     WINAPI  IDirect3DDevice8Impl_GetGammaRamp(LPDIRECT3DDEVICE8 iface, D3DGAMMARAMP* pRamp);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateTexture(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVolumeTexture(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateCubeTexture(LPDIRECT3DDEVICE8 iface, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateVertexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateIndexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateRenderTarget(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateImageSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CopyRects(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_UpdateTexture(LPDIRECT3DDEVICE8 iface, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetFrontBuffer(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pDestSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppRenderTarget);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppZStencilSurface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_BeginScene(LPDIRECT3DDEVICE8 iface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_EndScene(LPDIRECT3DDEVICE8 iface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_Clear(LPDIRECT3DDEVICE8 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_MultiplyTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetViewport(LPDIRECT3DDEVICE8 iface, CONST D3DVIEWPORT8* pViewport);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetViewport(LPDIRECT3DDEVICE8 iface, D3DVIEWPORT8* pViewport);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetMaterial(LPDIRECT3DDEVICE8 iface, CONST D3DMATERIAL8* pMaterial);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetMaterial(LPDIRECT3DDEVICE8 iface, D3DMATERIAL8* pMaterial);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetLight(LPDIRECT3DDEVICE8 iface, DWORD Index, CONST D3DLIGHT8* pLight);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetLight(LPDIRECT3DDEVICE8 iface, DWORD Index, D3DLIGHT8* pLight);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_LightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index, BOOL Enable);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetLightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index, BOOL* pEnable);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index, CONST float* pPlane);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index, float* pPlane);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State, DWORD Value);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State, DWORD* pValue);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_BeginStateBlock(LPDIRECT3DDEVICE8 iface);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_EndStateBlock(LPDIRECT3DDEVICE8 iface, DWORD* pToken);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_ApplyStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CaptureStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_CreateStateBlock(LPDIRECT3DDEVICE8 iface, D3DSTATEBLOCKTYPE Type,DWORD* pToken);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetClipStatus(LPDIRECT3DDEVICE8 iface, CONST D3DCLIPSTATUS8* pClipStatus);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetClipStatus(LPDIRECT3DDEVICE8 iface, D3DCLIPSTATUS8* pClipStatus);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage, IDirect3DBaseTexture8** ppTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage, IDirect3DBaseTexture8* pTexture);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_ValidateDevice(LPDIRECT3DDEVICE8 iface, DWORD* pNumPasses);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetInfo(LPDIRECT3DDEVICE8 iface, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, PALETTEENTRY* pEntries);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT *PaletteNumber);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
extern HRESULT  WINAPI  IDirect3DDevice8Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
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

/* internal Interfaces */
extern HRESULT WINAPI   IDirect3DDevice8Impl_CleanRender(LPDIRECT3DDEVICE8 iface);
extern HRESULT WINAPI   IDirect3DDevice8Impl_ActiveRender(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* RenderSurface, IDirect3DSurface8* StencilSurface);


/* ---------------- */
/* IDirect3DVolume8 */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVolume8Vtbl Direct3DVolume8_Vtbl;

/*****************************************************************************
 * IDirect3DVolume8 implementation structure
 */
struct IDirect3DVolume8Impl
{
    /* IUnknown fields */
    const IDirect3DVolume8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DVolume8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    IUnknown               *Container;
    D3DVOLUME_DESC          myDesc;
    BYTE                   *allocatedMemory;
    UINT                    textureName;
    UINT                    bytesPerPixel;

    BOOL                    lockable;
    BOOL                    locked;
    D3DBOX                  lockedBox;
    D3DBOX                  dirtyBox;
    BOOL                    Dirty;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DVolume8Impl_QueryInterface(LPDIRECT3DVOLUME8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI   IDirect3DVolume8Impl_AddRef(LPDIRECT3DVOLUME8 iface);
extern ULONG WINAPI   IDirect3DVolume8Impl_Release(LPDIRECT3DVOLUME8 iface);

/* IDirect3DVolume8: */
extern HRESULT WINAPI IDirect3DVolume8Impl_GetDevice(LPDIRECT3DVOLUME8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT WINAPI IDirect3DVolume8Impl_SetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT WINAPI IDirect3DVolume8Impl_GetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID  refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT WINAPI IDirect3DVolume8Impl_FreePrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid);
extern HRESULT WINAPI IDirect3DVolume8Impl_GetContainer(LPDIRECT3DVOLUME8 iface, REFIID riid, void** ppContainer);
extern HRESULT WINAPI IDirect3DVolume8Impl_GetDesc(LPDIRECT3DVOLUME8 iface, D3DVOLUME_DESC* pDesc);
extern HRESULT WINAPI IDirect3DVolume8Impl_LockBox(LPDIRECT3DVOLUME8 iface, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
extern HRESULT WINAPI IDirect3DVolume8Impl_UnlockBox(LPDIRECT3DVOLUME8 iface);

/* internal Interfaces */
extern HRESULT WINAPI IDirect3DVolume8Impl_CleanDirtyBox(LPDIRECT3DVOLUME8 iface);
extern HRESULT WINAPI IDirect3DVolume8Impl_AddDirtyBox(LPDIRECT3DVOLUME8 iface, CONST D3DBOX* pDirtyBox);

/* ------------------- */
/* IDirect3DSwapChain8 */
/* ------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DSwapChain8Vtbl Direct3DSwapChain8_Vtbl;

/*****************************************************************************
 * IDirect3DSwapChain8 implementation structure
 */
struct IDirect3DSwapChain8Impl
{
    /* IUnknown fields */
    const IDirect3DSwapChain8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DSwapChain8 fields */
    IDirect3DSurface8Impl  *frontBuffer;
    IDirect3DSurface8Impl  *backBuffer;
    IDirect3DSurface8Impl  *depthStencilBuffer;
    D3DPRESENT_PARAMETERS   PresentParms;

    /* OpenGL/GLX related */
    GLXContext              swap_ctx;
    Drawable                swap_drawable;

    /* ready for when we move over to wined3d */
    IWineD3DSwapChain      *wineD3DSwapChain;
};

/* IUnknown: */
extern HRESULT WINAPI IDirect3DSwapChain8Impl_QueryInterface(LPDIRECT3DSWAPCHAIN8 iface, REFIID refiid, LPVOID *obj);
extern ULONG WINAPI   IDirect3DSwapChain8Impl_AddRef(LPDIRECT3DSWAPCHAIN8 iface);
extern ULONG WINAPI   IDirect3DSwapChain8Impl_Release(LPDIRECT3DSWAPCHAIN8 iface);

/* IDirect3DSwapChain8: */
extern HRESULT WINAPI IDirect3DSwapChain8Impl_Present(LPDIRECT3DSWAPCHAIN8 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
extern HRESULT WINAPI IDirect3DSwapChain8Impl_GetBackBuffer(LPDIRECT3DSWAPCHAIN8 iface, UINT BackBuffer, D3DBACKBUFFER_TYPE Type,IDirect3DSurface8** ppBackBuffer);

/* Below follow the definitions of some WineD3D structures which are needed during the D3D8->WineD3D transition. */
typedef struct IWineD3DSurfaceImpl    IWineD3DSurfaceImpl;
typedef struct PrivateData
{
    struct PrivateData* next;

    GUID tag;
    DWORD flags; /* DDSPD_* */
    DWORD uniqueness_value;

    union
    {
        LPVOID data;
        LPUNKNOWN object;
    } ptr;

    DWORD size;
} PrivateData;

typedef struct IWineD3DResourceClass
{
    /* IUnknown fields */
    LONG                    ref;     /* Note: Ref counting not required */

    /* WineD3DResource Information */
    IUnknown               *parent;
    D3DRESOURCETYPE         resourceType;
    void                   *wineD3DDevice;
    D3DPOOL                 pool;
    UINT                    size;
    DWORD                   usage;
    WINED3DFORMAT           format;
    BYTE                   *allocatedMemory;
    PrivateData            *privateData;

} IWineD3DResourceClass;

typedef struct _WINED3DSURFACET_DESC
{
    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD               MultiSampleQuality;
    UINT                Width;
    UINT                Height;
} WINED3DSURFACET_DESC;

struct IWineD3DSurfaceImpl
{
    /* IUnknown & IWineD3DResource Information     */
    const IWineD3DSurfaceVtbl *lpVtbl;
    IWineD3DResourceClass     resource;

    /* IWineD3DSurface fields */
    IUnknown                 *container;
    WINED3DSURFACET_DESC      currentDesc;

    UINT                      textureName;
    UINT                      bytesPerPixel;

    /* TODO: move this off into a management class(maybe!) */
    BOOL                      nonpow2;

    UINT                      pow2Width;
    UINT                      pow2Height;
    UINT                      pow2Size;

#if 0
    /* precalculated x and y scalings for texture coords */
    float                     pow2scalingFactorX; /* =  (Width  / pow2Width ) */
    float                     pow2scalingFactorY; /* =  (Height / pow2Height) */
#endif

    BOOL                      lockable;
    BOOL                      discard;
    BOOL                      locked;
    BOOL                      activeLock;
    
    RECT                      lockedRect;
    RECT                      dirtyRect;
    BOOL                      Dirty;
    
    BOOL                      inTexture;
    BOOL                      inPBuffer;

    glDescriptor              glDescription;
};


/* ----------------- */
/* IDirect3DSurface8 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DSurface8Vtbl Direct3DSurface8_Vtbl;

/*****************************************************************************
 * IDirect3DSurface8 implementation structure
 */
struct IDirect3DSurface8Impl
{
    /* IUnknown fields */
    const IDirect3DSurface8Vtbl *lpVtbl;
    LONG                         ref;

    /* IDirect3DSurface8 fields */
    IWineD3DSurface             *wineD3DSurface;
};
#define D3D8_SURFACE(a) ((IWineD3DSurfaceImpl*)(a->wineD3DSurface))

/* IUnknown: */
extern HRESULT WINAPI IDirect3DSurface8Impl_QueryInterface(LPDIRECT3DSURFACE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI   IDirect3DSurface8Impl_AddRef(LPDIRECT3DSURFACE8 iface);
extern ULONG WINAPI   IDirect3DSurface8Impl_Release(LPDIRECT3DSURFACE8 iface);

/* IDirect3DSurface8: */
extern HRESULT WINAPI IDirect3DSurface8Impl_GetDevice(LPDIRECT3DSURFACE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT WINAPI IDirect3DSurface8Impl_SetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags);
extern HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid,void* pData,DWORD* pSizeOfData);
extern HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid);
extern HRESULT WINAPI IDirect3DSurface8Impl_GetContainer(LPDIRECT3DSURFACE8 iface, REFIID riid,void** ppContainer);
extern HRESULT WINAPI IDirect3DSurface8Impl_GetDesc(LPDIRECT3DSURFACE8 iface, D3DSURFACE_DESC *pDesc);
extern HRESULT WINAPI IDirect3DSurface8Impl_LockRect(LPDIRECT3DSURFACE8 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect,DWORD Flags);
extern HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(LPDIRECT3DSURFACE8 iface);

/* internal Interfaces */
extern HRESULT WINAPI IDirect3DSurface8Impl_LoadTexture(LPDIRECT3DSURFACE8 iface, GLenum gl_target, GLenum gl_level);
extern HRESULT WINAPI IDirect3DSurface8Impl_SaveSnapshot(LPDIRECT3DSURFACE8 iface, const char* filename);
extern HRESULT WINAPI IDirect3DSurface8Impl_CleanDirtyRect(LPDIRECT3DSURFACE8 iface);
extern HRESULT WINAPI IDirect3DSurface8Impl_AddDirtyRect(LPDIRECT3DSURFACE8 iface, CONST RECT* pDirtyRect);

/* ------------------ */
/* IDirect3DResource8 */
/* ------------------ */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DResource8Vtbl Direct3DResource8_Vtbl;

/*****************************************************************************
 * IDirect3DResource8 implementation structure
 */
struct IDirect3DResource8Impl
{
    /* IUnknown fields */
    const IDirect3DResource8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResource8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DResource8Impl_QueryInterface(LPDIRECT3DRESOURCE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DResource8Impl_AddRef(LPDIRECT3DRESOURCE8 iface);
extern ULONG WINAPI           IDirect3DResource8Impl_Release(LPDIRECT3DRESOURCE8 iface);

/* IDirect3DResource8: */
extern HRESULT  WINAPI        IDirect3DResource8Impl_GetDevice(LPDIRECT3DRESOURCE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DResource8Impl_SetPrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DResource8Impl_GetPrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DResource8Impl_FreePrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DResource8Impl_SetPriority(LPDIRECT3DRESOURCE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DResource8Impl_GetPriority(LPDIRECT3DRESOURCE8 iface);
extern void     WINAPI        IDirect3DResource8Impl_PreLoad(LPDIRECT3DRESOURCE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DResource8Impl_GetType(LPDIRECT3DRESOURCE8 iface);

/* internal Interfaces */
extern D3DPOOL WINAPI         IDirect3DResource8Impl_GetPool(LPDIRECT3DRESOURCE8 iface);


/* ---------------------- */
/* IDirect3DVertexBuffer8 */
/* ---------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVertexBuffer8Vtbl Direct3DVertexBuffer8_Vtbl;

/*****************************************************************************
 * IDirect3DVertexBuffer8 implementation structure
 */
struct IDirect3DVertexBuffer8Impl
{
    /* IUnknown fields */
    const IDirect3DVertexBuffer8Vtbl *lpVtbl;
    LONG                   ref;
    LONG                   refInt;

    /* IDirect3DResource8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    /* IDirect3DVertexBuffer8 fields */
    BYTE                   *allocatedMemory;
    D3DVERTEXBUFFER_DESC    currentDesc;
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DVertexBuffer8Impl_QueryInterface(LPDIRECT3DVERTEXBUFFER8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DVertexBuffer8Impl_AddRef(LPDIRECT3DVERTEXBUFFER8 iface);
extern ULONG WINAPI           IDirect3DVertexBuffer8Impl_Release(LPDIRECT3DVERTEXBUFFER8 iface);

/* Internal */
extern ULONG WINAPI           IDirect3DVertexBuffer8Impl_AddRefInt(LPDIRECT3DVERTEXBUFFER8 iface);
extern ULONG WINAPI           IDirect3DVertexBuffer8Impl_ReleaseInt(LPDIRECT3DVERTEXBUFFER8 iface);

/* IDirect3DVertexBuffer8: (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetDevice(LPDIRECT3DVERTEXBUFFER8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DVertexBuffer8Impl_SetPriority(LPDIRECT3DVERTEXBUFFER8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DVertexBuffer8Impl_GetPriority(LPDIRECT3DVERTEXBUFFER8 iface);
extern void     WINAPI        IDirect3DVertexBuffer8Impl_PreLoad(LPDIRECT3DVERTEXBUFFER8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer8Impl_GetType(LPDIRECT3DVERTEXBUFFER8 iface);

/* IDirect3DVertexBuffer8: */
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_Lock(LPDIRECT3DVERTEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_Unlock(LPDIRECT3DVERTEXBUFFER8 iface);
extern HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetDesc(LPDIRECT3DVERTEXBUFFER8 iface, D3DVERTEXBUFFER_DESC *pDesc);


/* --------------------- */
/* IDirect3DIndexBuffer8 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DIndexBuffer8Vtbl Direct3DIndexBuffer8_Vtbl;

/*****************************************************************************
 * IDirect3DIndexBuffer8 implementation structure
 */
struct IDirect3DIndexBuffer8Impl
{
    /* IUnknown fields */
    const IDirect3DIndexBuffer8Vtbl *lpVtbl;
    LONG                   ref;
    LONG                   refInt;

    /* IDirect3DResource8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    /* IDirect3DIndexBuffer8 fields */
    void                   *allocatedMemory;
    D3DINDEXBUFFER_DESC     currentDesc;
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DIndexBuffer8Impl_QueryInterface(LPDIRECT3DINDEXBUFFER8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DIndexBuffer8Impl_AddRef(LPDIRECT3DINDEXBUFFER8 iface);
extern ULONG WINAPI           IDirect3DIndexBuffer8Impl_Release(LPDIRECT3DINDEXBUFFER8 iface);

/* Internal */
extern ULONG WINAPI           IDirect3DIndexBuffer8Impl_AddRefInt(LPDIRECT3DINDEXBUFFER8 iface);
extern ULONG WINAPI           IDirect3DIndexBuffer8Impl_ReleaseInt(LPDIRECT3DINDEXBUFFER8 iface);

/* IDirect3DIndexBuffer8: (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetDevice(LPDIRECT3DINDEXBUFFER8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_SetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_FreePrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DIndexBuffer8Impl_SetPriority(LPDIRECT3DINDEXBUFFER8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DIndexBuffer8Impl_GetPriority(LPDIRECT3DINDEXBUFFER8 iface);
extern void     WINAPI        IDirect3DIndexBuffer8Impl_PreLoad(LPDIRECT3DINDEXBUFFER8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DIndexBuffer8Impl_GetType(LPDIRECT3DINDEXBUFFER8 iface);

/* IDirect3DIndexBuffer8: */
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_Lock(LPDIRECT3DINDEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_Unlock(LPDIRECT3DINDEXBUFFER8 iface);
extern HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetDesc(LPDIRECT3DINDEXBUFFER8 iface, D3DINDEXBUFFER_DESC *pDesc);


/* --------------------- */
/* IDirect3DBaseTexture8 */
/* --------------------- */

/*****************************************************************************
 * IDirect3DBaseTexture8 implementation structure
 */
struct IDirect3DBaseTexture8Impl
{
    /* IUnknown fields */
    const IDirect3DBaseTexture8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResource8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    /* IDirect3DBaseTexture8 fields */
    BOOL                    Dirty;
    D3DFORMAT               format;
    UINT                    levels;
    /*
     *BOOL                    isManaged;
     *DWORD                   lod;
     */
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DBaseTexture8Impl_QueryInterface(LPDIRECT3DBASETEXTURE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DBaseTexture8Impl_AddRef(LPDIRECT3DBASETEXTURE8 iface);
extern ULONG WINAPI           IDirect3DBaseTexture8Impl_Release(LPDIRECT3DBASETEXTURE8 iface);

/* IDirect3DBaseTexture8: (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DBaseTexture8Impl_GetDevice(LPDIRECT3DBASETEXTURE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DBaseTexture8Impl_SetPrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DBaseTexture8Impl_GetPrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DBaseTexture8Impl_FreePrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_SetPriority(LPDIRECT3DBASETEXTURE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetPriority(LPDIRECT3DBASETEXTURE8 iface);
extern void     WINAPI        IDirect3DBaseTexture8Impl_PreLoad(LPDIRECT3DBASETEXTURE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DBaseTexture8Impl_GetType(LPDIRECT3DBASETEXTURE8 iface);

/* IDirect3DBaseTexture8: */
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_SetLOD(LPDIRECT3DBASETEXTURE8 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetLOD(LPDIRECT3DBASETEXTURE8 iface);
extern DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetLevelCount(LPDIRECT3DBASETEXTURE8 iface);

/* internal Interfaces */
extern BOOL     WINAPI        IDirect3DBaseTexture8Impl_IsDirty(LPDIRECT3DBASETEXTURE8 iface);
extern BOOL     WINAPI        IDirect3DBaseTexture8Impl_SetDirty(LPDIRECT3DBASETEXTURE8 iface, BOOL dirty);


/* --------------------- */
/* IDirect3DCubeTexture8 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DCubeTexture8Vtbl Direct3DCubeTexture8_Vtbl;

/*****************************************************************************
 * IDirect3DCubeTexture8 implementation structure
 */
struct IDirect3DCubeTexture8Impl
{
    /* IUnknown fields */
    const IDirect3DCubeTexture8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResource8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    /* IDirect3DBaseTexture8 fields */
    BOOL                    Dirty;
    D3DFORMAT               format;
    UINT                    levels;

    /* IDirect3DCubeTexture8 fields */
    UINT                    edgeLength;
    DWORD                   usage;

    IDirect3DSurface8Impl  *surfaces[6][MAX_LEVELS];
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DCubeTexture8Impl_QueryInterface(LPDIRECT3DCUBETEXTURE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DCubeTexture8Impl_AddRef(LPDIRECT3DCUBETEXTURE8 iface);
extern ULONG WINAPI           IDirect3DCubeTexture8Impl_Release(LPDIRECT3DCUBETEXTURE8 iface);

/* IDirect3DCubeTexture8: (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetDevice(LPDIRECT3DCUBETEXTURE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_SetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_FreePrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_SetPriority(LPDIRECT3DCUBETEXTURE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetPriority(LPDIRECT3DCUBETEXTURE8 iface);
extern void     WINAPI        IDirect3DCubeTexture8Impl_PreLoad(LPDIRECT3DCUBETEXTURE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DCubeTexture8Impl_GetType(LPDIRECT3DCUBETEXTURE8 iface);

/* IDirect3DCubeTexture8: (Inherited from IDirect3DBaseTexture8) */
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_SetLOD(LPDIRECT3DCUBETEXTURE8 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetLOD(LPDIRECT3DCUBETEXTURE8 iface);
extern DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetLevelCount(LPDIRECT3DCUBETEXTURE8 iface);

/* IDirect3DCubeTexture8 */
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetLevelDesc(LPDIRECT3DCUBETEXTURE8 iface, UINT Level, D3DSURFACE_DESC* pDesc);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetCubeMapSurface(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface8** ppCubeMapSurface);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_LockRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_UnlockRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level);
extern HRESULT  WINAPI        IDirect3DCubeTexture8Impl_AddDirtyRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect);


/* ----------------- */
/* IDirect3DTexture8 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DTexture8Vtbl Direct3DTexture8_Vtbl;

/*****************************************************************************
 * IDirect3DTexture8 implementation structure
 */
struct IDirect3DTexture8Impl
{
    /* IUnknown fields */
    const IDirect3DTexture8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResourc8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    /* IDirect3DBaseTexture8 fields */
    BOOL                    Dirty;
    D3DFORMAT               format;
    UINT                    levels;

    /* IDirect3DTexture8 fields */
    UINT                    width;
    UINT                    height;
    DWORD                   usage;

    IDirect3DSurface8Impl  *surfaces[MAX_LEVELS];
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DTexture8Impl_QueryInterface(LPDIRECT3DTEXTURE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DTexture8Impl_AddRef(LPDIRECT3DTEXTURE8 iface);
extern ULONG WINAPI           IDirect3DTexture8Impl_Release(LPDIRECT3DTEXTURE8 iface);

/* IDirect3DTexture8: (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DTexture8Impl_GetDevice(LPDIRECT3DTEXTURE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_SetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_GetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_FreePrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DTexture8Impl_SetPriority(LPDIRECT3DTEXTURE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DTexture8Impl_GetPriority(LPDIRECT3DTEXTURE8 iface);
extern void     WINAPI        IDirect3DTexture8Impl_PreLoad(LPDIRECT3DTEXTURE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DTexture8Impl_GetType(LPDIRECT3DTEXTURE8 iface);

/* IDirect3DTexture8: (Inherited from IDirect3DBaseTexture8) */
extern DWORD    WINAPI        IDirect3DTexture8Impl_SetLOD(LPDIRECT3DTEXTURE8 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DTexture8Impl_GetLOD(LPDIRECT3DTEXTURE8 iface);
extern DWORD    WINAPI        IDirect3DTexture8Impl_GetLevelCount(LPDIRECT3DTEXTURE8 iface);

/* IDirect3DTexture8: */
extern HRESULT  WINAPI        IDirect3DTexture8Impl_GetLevelDesc(LPDIRECT3DTEXTURE8 iface, UINT Level, D3DSURFACE_DESC* pDesc);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_GetSurfaceLevel(LPDIRECT3DTEXTURE8 iface, UINT Level, IDirect3DSurface8** ppSurfaceLevel);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_LockRect(LPDIRECT3DTEXTURE8 iface, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_UnlockRect(LPDIRECT3DTEXTURE8 iface, UINT Level);
extern HRESULT  WINAPI        IDirect3DTexture8Impl_AddDirtyRect(LPDIRECT3DTEXTURE8 iface, CONST RECT* pDirtyRect);


/* ----------------------- */
/* IDirect3DVolumeTexture8 */
/* ----------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVolumeTexture8Vtbl Direct3DVolumeTexture8_Vtbl;

/*****************************************************************************
 * IDirect3DVolumeTexture8 implementation structure
 */
struct IDirect3DVolumeTexture8Impl
{
    /* IUnknown fields */
    const IDirect3DVolumeTexture8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResource8 fields */
    IDirect3DDevice8Impl   *Device;
    D3DRESOURCETYPE         ResourceType;

    /* IDirect3DBaseTexture8 fields */
    BOOL                    Dirty;
    D3DFORMAT               format;
    UINT                    levels;

    /* IDirect3DVolumeTexture8 fields */
    UINT                    width;
    UINT                    height;
    UINT                    depth;
    DWORD                   usage;

    IDirect3DVolume8Impl   *volumes[MAX_LEVELS];
};

/* IUnknown: */
extern HRESULT WINAPI         IDirect3DVolumeTexture8Impl_QueryInterface(LPDIRECT3DVOLUMETEXTURE8 iface,REFIID refiid,LPVOID *obj);
extern ULONG WINAPI           IDirect3DVolumeTexture8Impl_AddRef(LPDIRECT3DVOLUMETEXTURE8 iface);
extern ULONG WINAPI           IDirect3DVolumeTexture8Impl_Release(LPDIRECT3DVOLUMETEXTURE8 iface);

/* IDirect3DVolumeTexture8: (Inherited from IDirect3DResource8) */
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetDevice(LPDIRECT3DVOLUMETEXTURE8 iface, IDirect3DDevice8** ppDevice);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_SetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid);
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD PriorityNew);
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE8 iface);
extern void     WINAPI        IDirect3DVolumeTexture8Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE8 iface);
extern D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture8Impl_GetType(LPDIRECT3DVOLUMETEXTURE8 iface);

/* IDirect3DVolumeTexture8: (Inherited from IDirect3DBaseTexture8) */
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD LODNew);
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE8 iface);
extern DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetLevelCount(LPDIRECT3DVOLUMETEXTURE8 iface);

/* IDirect3DVolumeTexture8: */
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetLevelDesc(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level, D3DVOLUME_DESC *pDesc);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetVolumeLevel(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level, IDirect3DVolume8** ppVolumeLevel);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_LockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level);
extern HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE8 iface, CONST D3DBOX* pDirtyBox);


/* ==============================================================================
    Private interfactes: beginning of cleaning/splitting for HAL and d3d9 support
   ============================================================================== */

/* State Block for Begin/End/Capture/Create/Apply State Block   */
/*   Note: Very long winded but I do not believe gl Lists will  */
/*   resolve everything we need, so doing it manually for now   */
typedef struct SAVEDSTATES {
        BOOL                      Indices;
        BOOL                      material;
        BOOL                      stream_source[MAX_STREAMS];
        BOOL                      textures[8];
        BOOL                      transform[HIGHEST_TRANSFORMSTATE];
        BOOL                      viewport;
        BOOL                      vertexShader;
        BOOL                      vertexShaderConstant;
        BOOL                      vertexShaderDecl;
        BOOL                      pixelShader;
        BOOL                      pixelShaderConstant;
        BOOL                      renderstate[HIGHEST_RENDER_STATE];
        BOOL                      texture_state[8][HIGHEST_TEXTURE_STATE];
        BOOL                      clipplane[MAX_CLIPPLANES];
} SAVEDSTATES;


/* ----------------------- */
/* IDirect3DStateBlockImpl */
/* ----------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
/*extern const IDirect3DStateBlock9Vtbl Direct3DStateBlock9_Vtbl;*/

/*****************************************************************************
 * IDirect3DStateBlock implementation structure
 */
struct  IDirect3DStateBlockImpl {
  /* IUnknown fields */
  /*const IDirect3DStateBlock9Vtbl *lpVtbl;*/
  LONG  ref;

  /* The device, to be replaced by an IDirect3DDeviceImpl */
  IDirect3DDevice8Impl* device;

  D3DSTATEBLOCKTYPE         blockType;

  SAVEDSTATES               Changed;
  SAVEDSTATES               Set;
  
  /* Clipping */
  double                    clipplane[MAX_CLIPPLANES][4];
  D3DCLIPSTATUS8            clip_status;

  /* Stream Source */
  UINT                      stream_stride[MAX_STREAMS];
  IDirect3DVertexBuffer8   *stream_source[MAX_STREAMS];
  BOOL                      streamIsUP;

  /* Indices */
  IDirect3DIndexBuffer8*    pIndexData;
  UINT                      baseVertexIndex;
  
  /* Texture */
  IDirect3DBaseTexture8    *textures[8];
  int                       textureDimensions[8];
  /* Texture State Stage */
  DWORD                     texture_state[8][HIGHEST_TEXTURE_STATE];
  
  /* Lights */
  PLIGHTINFOEL             *lights; /* NOTE: active GL lights must be front of the chain */
  
  /* Material */
  D3DMATERIAL8              material;
  
  /* RenderState */
  DWORD                     renderstate[HIGHEST_RENDER_STATE];
  
  /* Transform */
  D3DMATRIX                 transforms[HIGHEST_TRANSFORMSTATE];
  
  /* ViewPort */
  D3DVIEWPORT8              viewport;
  
  /* Vertex Shader */
  DWORD                     VertexShader;

  /* Vertex Shader Declaration */
  IDirect3DVertexShaderDeclarationImpl* vertexShaderDecl;
  
  /* Pixel Shader */
  DWORD                     PixelShader;
  
  /* Indexed Vertex Blending */
  D3DVERTEXBLENDFLAGS       vertex_blend;
  FLOAT                     tween_factor;

  /* Vertex Shader Constant */
  D3DSHADERVECTOR           vertexShaderConstant[D3D8_VSHADER_MAX_CONSTANTS];
  /* Pixel Shader Constant */
  D3DSHADERVECTOR           pixelShaderConstant[D3D8_PSHADER_MAX_CONSTANTS];
};

/* exported Interfaces */
/* internal Interfaces */
/* temporary internal Interfaces */
extern HRESULT WINAPI IDirect3DDeviceImpl_InitStartupStateBlock(IDirect3DDevice8Impl* This);
extern HRESULT WINAPI IDirect3DDeviceImpl_CreateStateBlock(IDirect3DDevice8Impl* This, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlockImpl** ppStateBlock);
extern HRESULT WINAPI IDirect3DDeviceImpl_DeleteStateBlock(IDirect3DDevice8Impl* This, IDirect3DStateBlockImpl* pSB);
extern HRESULT WINAPI IDirect3DDeviceImpl_BeginStateBlock(IDirect3DDevice8Impl* This);
extern HRESULT WINAPI IDirect3DDeviceImpl_EndStateBlock(IDirect3DDevice8Impl* This, IDirect3DStateBlockImpl** ppStateBlock);
extern HRESULT WINAPI IDirect3DDeviceImpl_ApplyStateBlock(IDirect3DDevice8Impl* iface, IDirect3DStateBlockImpl* pSB);
extern HRESULT WINAPI IDirect3DDeviceImpl_CaptureStateBlock(IDirect3DDevice8Impl* This, IDirect3DStateBlockImpl* pSB);

/* ------------------------------------ */
/* IDirect3DVertexShaderDeclarationImpl */
/* ------------------------------------ */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
/*extern const IDirect3DVertexShaderDeclaration9Vtbl Direct3DVertexShaderDeclaration9_Vtbl;*/

/*****************************************************************************
 * IDirect3DVertexShaderDeclaration implementation structure
 */
struct IDirect3DVertexShaderDeclarationImpl {
  /* IUnknown fields */
  /*const IDirect3DVertexShaderDeclaration9Vtbl *lpVtbl;*/
  LONG  ref;

  /* The device, to be replaced by an IDirect3DDeviceImpl */
  IDirect3DDevice8Impl* device;

  /** precomputed fvf if simple declaration */
  DWORD   fvf[MAX_STREAMS];
  DWORD   allFVF;

  /** dx8 compatible Declaration fields */
  DWORD*  pDeclaration8;
  DWORD   declaration8Length;
};

/* exported Interfaces */
extern HRESULT WINAPI IDirect3DVertexShaderDeclarationImpl_GetDeclaration8(IDirect3DVertexShaderDeclarationImpl* This, DWORD* pData, UINT* pSizeOfData);
/*extern HRESULT IDirect3DVertexShaderDeclarationImpl_GetDeclaration9(IDirect3DVertexShaderDeclarationImpl* This, D3DVERTEXELEMENT9* pData, UINT* pNumElements);*/
/* internal Interfaces */
/* temporary internal Interfaces */
extern HRESULT WINAPI IDirect3DDeviceImpl_CreateVertexShaderDeclaration8(IDirect3DDevice8Impl* This, CONST DWORD* pDeclaration8, IDirect3DVertexShaderDeclarationImpl** ppVertexShaderDecl);


/* ------------------------- */
/* IDirect3DVertexShaderImpl */
/* ------------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
/*extern const IDirect3DVertexShader9Vtbl Direct3DVertexShader9_Vtbl;*/

/*****************************************************************************
 * IDirect3DVertexShader implementation structure
 */
struct IDirect3DVertexShaderImpl {
  /*const IDirect3DVertexShader9Vtbl *lpVtbl;*/
  LONG ref;

  /* The device, to be replaced by an IDirect3DDeviceImpl */
  IDirect3DDevice8Impl* device;

  DWORD* function;
  UINT functionLength;
  DWORD usage; /* 0 || D3DUSAGE_SOFTWAREPROCESSING */
  DWORD version;
  
  /** fields for hw vertex shader use */
  GLuint  prgId;

  /* run time datas */
  VSHADERDATA8* data;
  VSHADERINPUTDATA8 input;
  VSHADEROUTPUTDATA8 output;
};

/* exported Interfaces */
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetFunction(IDirect3DVertexShaderImpl* This, VOID* pData, UINT* pSizeOfData);
/*extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantB(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST BOOL*  pConstantData, UINT BoolCount);*/
/*extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantI(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST INT*   pConstantData, UINT Vector4iCount);*/
extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantF(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST FLOAT* pConstantData, UINT Vector4fCount);
/*extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantB(IDirect3DVertexShaderImpl* This, UINT StartRegister, BOOL*  pConstantData, UINT BoolCount);*/
/*extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantI(IDirect3DVertexShaderImpl* This, UINT StartRegister, INT*   pConstantData, UINT Vector4iCount);*/
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantF(IDirect3DVertexShaderImpl* This, UINT StartRegister, FLOAT* pConstantData, UINT Vector4fCount);
/* internal Interfaces */
extern DWORD WINAPI IDirect3DVertexShaderImpl_GetVersion(IDirect3DVertexShaderImpl* This);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_ExecuteSW(IDirect3DVertexShaderImpl* This, VSHADERINPUTDATA8* input, VSHADEROUTPUTDATA8* output);
/* temporary internal Interfaces */
extern HRESULT WINAPI IDirect3DDeviceImpl_CreateVertexShader(IDirect3DDevice8Impl* This, CONST DWORD* pFunction, DWORD Usage, IDirect3DVertexShaderImpl** ppVertexShader);
extern HRESULT WINAPI IDirect3DDeviceImpl_FillVertexShaderInputSW(IDirect3DDevice8Impl* This, IDirect3DVertexShaderImpl* vshader,  DWORD SkipnStrides);
extern HRESULT WINAPI IDirect3DDeviceImpl_FillVertexShaderInputArbHW(IDirect3DDevice8Impl* This, IDirect3DVertexShaderImpl* vshader,  DWORD SkipnStrides);

/* ------------------------ */
/* IDirect3DPixelShaderImpl */
/* ------------------------ */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
/*extern const IDirect3DPixelShader9Vtbl Direct3DPixelShader9_Vtbl;*/

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
struct IDirect3DPixelShaderImpl { 
  /*const IDirect3DPixelShader9Vtbl *lpVtbl;*/
  LONG ref;

  /* The device, to be replaced by an IDirect3DDeviceImpl */
  IDirect3DDevice8Impl* device;

  DWORD* function;
  UINT functionLength;
  DWORD version;

  /** fields for hw pixel shader use */
  GLuint  prgId;

  /* run time datas */
  PSHADERDATA8* data;
  PSHADERINPUTDATA8 input;
  PSHADEROUTPUTDATA8 output;
};

/* exported Interfaces */
extern HRESULT WINAPI IDirect3DPixelShaderImpl_GetFunction(IDirect3DPixelShaderImpl* This, VOID* pData, UINT* pSizeOfData);
extern HRESULT WINAPI IDirect3DPixelShaderImpl_SetConstantF(IDirect3DPixelShaderImpl* This, UINT StartRegister, CONST FLOAT* pConstantData, UINT Vector4fCount);
/* internal Interfaces */
extern DWORD WINAPI IDirect3DPixelShaderImpl_GetVersion(IDirect3DPixelShaderImpl* This);
/* temporary internal Interfaces */
extern HRESULT WINAPI IDirect3DDeviceImpl_CreatePixelShader(IDirect3DDevice8Impl* This, CONST DWORD* pFunction, IDirect3DPixelShaderImpl** ppPixelShader);


/**
 * Internals functions
 *
 * to see how not defined it here
 */ 
void   GetSrcAndOpFromValue(DWORD iValue, BOOL isAlphaArg, GLenum* source, GLenum* operand);
void   setupTextureStates(LPDIRECT3DDEVICE8 iface, DWORD Stage, DWORD Flags);
void   set_tex_op(LPDIRECT3DDEVICE8 iface, BOOL isAlpha, int Stage, D3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3);

SHORT  D3DFmtGetBpp(IDirect3DDevice8Impl* This, D3DFORMAT fmt);
GLint  D3DFmt2GLIntFmt(IDirect3DDevice8Impl* This, D3DFORMAT fmt);
GLenum D3DFmt2GLFmt(IDirect3DDevice8Impl* This, D3DFORMAT fmt);
GLenum D3DFmt2GLType(IDirect3DDevice8Impl* This, D3DFORMAT fmt);

GLenum D3DFmt2GLDepthFmt(D3DFORMAT fmt);
GLenum D3DFmt2GLDepthType(D3DFORMAT fmt);

int D3DPrimitiveListGetVertexSize(D3DPRIMITIVETYPE PrimitiveType, int iNumPrim);
int D3DPrimitive2GLenum(D3DPRIMITIVETYPE PrimitiveType);
int D3DFVFGetSize(D3DFORMAT fvf);

int SOURCEx_RGB_EXT(DWORD arg);
int OPERANDx_RGB_EXT(DWORD arg);
int SOURCEx_ALPHA_EXT(DWORD arg);
int OPERANDx_ALPHA_EXT(DWORD arg);
GLenum StencilOp(DWORD op);

/**
 * Internals debug functions
 */
const char* debug_d3ddevicetype(D3DDEVTYPE devtype);
const char* debug_d3dusage(DWORD usage);
const char* debug_d3dformat(D3DFORMAT fmt);
const char* debug_d3dressourcetype(D3DRESOURCETYPE res);
const char* debug_d3dprimitivetype(D3DPRIMITIVETYPE PrimitiveType);
const char* debug_d3dpool(D3DPOOL Pool);
const char *debug_d3drenderstate(DWORD State);
const char *debug_d3dtexturestate(DWORD State);

/* Some #defines for additional diagnostics */
#if 0 /* NOTE: Must be 0 in cvs */
  /* To avoid having to get gigabytes of trace, the following can be compiled in, and at the start
     of each frame, a check is made for the existence of C:\D3DTRACE, and if if exists d3d trace
     is enabled, and if it doesn't exists it is disabled.                                           */
# define FRAME_DEBUGGING
  /*  Adding in the SINGLE_FRAME_DEBUGGING gives a trace of just what makes up a single frame, before
      the file is deleted                                                                            */
# if 1 /* NOTE: Must be 1 in cvs, as this is mostly more useful than a trace from program start */
#  define SINGLE_FRAME_DEBUGGING
# endif  
  /* The following, when enabled, lets you see the makeup of the frame, by drawprimitive calls.
     It can only be enabled when FRAME_DEBUGGING is also enabled                               
     The contents of the back buffer are written into /tmp/backbuffer_* after each primitive 
     array is drawn.                                                                            */
# if 0 /* NOTE: Must be 0 in cvs, as this give a lot of ppm files when compiled in */                                                                                       
#  define SHOW_FRAME_MAKEUP 1
# endif  
  /* The following, when enabled, lets you see the makeup of the all the textures used during each
     of the drawprimitive calls. It can only be enabled when SHOW_FRAME_MAKEUP is also enabled.
     The contents of the textures assigned to each stage are written into 
     /tmp/texture_*_<Stage>.ppm after each primitive array is drawn.                            */
# if 0 /* NOTE: Must be 0 in cvs, as this give a lot of ppm files when compiled in */
#  define SHOW_TEXTURE_MAKEUP 0
# endif  
extern BOOL isOn;
extern BOOL isDumpingFrames;
extern LONG primCounter;
#endif

/* Per-vertex trace: */
#if 0 /* NOTE: Must be 0 in cvs */
# define VTRACE(A) TRACE A
#else 
# define VTRACE(A) 
#endif

#define TRACE_VECTOR(name) TRACE( #name "=(%f, %f, %f, %f)\n", name.x, name.y, name.z, name.w);
#define TRACE_STRIDED(sd,name) TRACE( #name "=(data:%p, stride:%ld, type:%ld)\n", sd->u.s.name.lpData, sd->u.s.name.dwStride, sd->u.s.name.dwType);

#define DUMP_LIGHT_CHAIN()                    \
{                                             \
  PLIGHTINFOEL *el = This->StateBlock->lights;\
  while (el) {                                \
    TRACE("Light %p (glIndex %ld, d3dIndex %ld, enabled %d)\n", el, el->glIndex, el->OriginalIndex, el->lightEnabled);\
    el = el->next;                            \
  }                                           \
}

#endif /* __WINE_D3DX8_PRIVATE_H */
