/*
 * Direct3D private include file
 *
 * Copyright (c) 1998-2004 Lionel Ulmer
 * Copyright (c) 2002-2005 Christian Costa
 *
 * This file contains all the structure that are not exported
 * through d3d.h and all common macros.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __GRAPHICS_WINE_D3D_PRIVATE_H
#define __GRAPHICS_WINE_D3D_PRIVATE_H

/* THIS FILE MUST NOT CONTAIN X11 or MESA DEFINES */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "d3d.h"

#define MAX_TEXTURES 8
#define MAX_LIGHTS  16

#define HIGHEST_RENDER_STATE         152
#define HIGHEST_TEXTURE_STAGE_STATE   24

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirect3DLightImpl IDirect3DLightImpl;
typedef struct IDirect3DMaterialImpl IDirect3DMaterialImpl;
typedef struct IDirect3DViewportImpl IDirect3DViewportImpl;
typedef struct IDirect3DExecuteBufferImpl IDirect3DExecuteBufferImpl;
typedef struct IDirect3DVertexBufferImpl IDirect3DVertexBufferImpl;

#include "ddraw_private.h"

typedef struct STATEBLOCKFLAGS {
   BOOL render_state[HIGHEST_RENDER_STATE];
   BOOL texture_stage_state[MAX_TEXTURES][HIGHEST_TEXTURE_STAGE_STATE];
} STATEBLOCKFLAGS;

typedef struct STATEBLOCK {
   STATEBLOCKFLAGS set_flags; 
   DWORD render_state[HIGHEST_RENDER_STATE];
   DWORD texture_stage_state[MAX_TEXTURES][HIGHEST_TEXTURE_STAGE_STATE];
} STATEBLOCK;


/*****************************************************************************
 * IDirect3DLight implementation structure
 */
struct IDirect3DLightImpl
{
    ICOM_VFIELD_MULTI(IDirect3DLight);
    LONG ref;
    /* IDirect3DLight fields */
    IDirectDrawImpl *d3d;
    /* If this light is active for one viewport, put the viewport here */
    IDirect3DViewportImpl *active_viewport;

    D3DLIGHT2 light;
    D3DLIGHT7 light7;

    DWORD dwLightIndex;

    /* Chained list used for adding / removing from viewports */
    IDirect3DLightImpl *next;

    /* Activation function */
    void (*activate)(IDirect3DLightImpl*);
    void (*desactivate)(IDirect3DLightImpl*);
    void (*update)(IDirect3DLightImpl*);
};

/*****************************************************************************
 * IDirect3DMaterial implementation structure
 */
struct IDirect3DMaterialImpl
{
    ICOM_VFIELD_MULTI(IDirect3DMaterial3);
    ICOM_VFIELD_MULTI(IDirect3DMaterial2);
    ICOM_VFIELD_MULTI(IDirect3DMaterial);
    LONG  ref;
    /* IDirect3DMaterial2 fields */
    IDirectDrawImpl *d3d;
    IDirect3DDeviceImpl *active_device;

    D3DMATERIAL mat;

    void (*activate)(IDirect3DMaterialImpl* this);
};

/*****************************************************************************
 * IDirect3DViewport implementation structure
 */
struct IDirect3DViewportImpl
{
    ICOM_VFIELD_MULTI(IDirect3DViewport3);
    LONG ref;
    /* IDirect3DViewport fields */
    IDirectDrawImpl *d3d;
    /* If this viewport is active for one device, put the device here */
    IDirect3DDeviceImpl *active_device;

    DWORD num_lights;
    DWORD map_lights;

    int use_vp2;
    union {
        D3DVIEWPORT vp1;
	D3DVIEWPORT2 vp2;
    } viewports;

    /* Activation function */
    void (*activate)(IDirect3DViewportImpl*);

    /* Field used to chain viewports together */
    IDirect3DViewportImpl *next;

    /* Lights list */
    IDirect3DLightImpl *lights;

    /* Background material */
    IDirect3DMaterialImpl *background;
};

/*****************************************************************************
 * IDirect3DExecuteBuffer implementation structure
 */
struct IDirect3DExecuteBufferImpl
{
    ICOM_VFIELD_MULTI(IDirect3DExecuteBuffer);
    LONG ref;
    /* IDirect3DExecuteBuffer fields */
    IDirectDrawImpl *d3d;
    IDirect3DDeviceImpl* d3ddev;

    D3DEXECUTEBUFFERDESC desc;
    D3DEXECUTEDATA data;

    /* This buffer will store the transformed vertices */
    void* vertex_data;
    WORD* indices;
    int nb_indices;

    /* This flags is set to TRUE if we allocated ourselves the
       data buffer */
    BOOL need_free;

    void (*execute)(IDirect3DExecuteBufferImpl* this,
                    IDirect3DDeviceImpl* dev,
                    IDirect3DViewportImpl* vp);
};

/* Internal structure to store the state of the clipping planes */
typedef struct d3d7clippingplane 
{
    D3DVALUE plane[4];
} d3d7clippingplane;

/*****************************************************************************
 * IDirect3DDevice implementation structure
 */

#define WORLDMAT_CHANGED (0x00000001 <<  0)
#define VIEWMAT_CHANGED  (0x00000001 <<  1)
#define PROJMAT_CHANGED  (0x00000001 <<  2)
#define TEXMAT0_CHANGED  (0x00000001 <<  3)
#define TEXMAT1_CHANGED  (0x00000001 <<  4)
#define TEXMAT2_CHANGED  (0x00000001 <<  5)
#define TEXMAT3_CHANGED  (0x00000001 <<  6)
#define TEXMAT4_CHANGED  (0x00000001 <<  7)
#define TEXMAT5_CHANGED  (0x00000001 <<  8)
#define TEXMAT6_CHANGED  (0x00000001 <<  9)
#define TEXMAT7_CHANGED  (0x00000001 << 10)

struct IDirect3DDeviceImpl
{
    ICOM_VFIELD_MULTI(IDirect3DDevice7);
    ICOM_VFIELD_MULTI(IDirect3DDevice3);
    ICOM_VFIELD_MULTI(IDirect3DDevice2);
    ICOM_VFIELD_MULTI(IDirect3DDevice);
    LONG  ref;

    /* Version of the Direct3D object from which the device has been created */
    DWORD version;

    /* IDirect3DDevice fields */
    IDirectDrawImpl *d3d;
    IDirectDrawSurfaceImpl *surface;

    IDirect3DViewportImpl *viewport_list;
    IDirect3DViewportImpl *current_viewport;
    D3DVIEWPORT7 active_viewport;

    IDirectDrawSurfaceImpl *current_texture[MAX_TEXTURES];
    IDirectDrawSurfaceImpl *current_zbuffer;
    
    /* Current transformation matrices */
    D3DMATRIX *world_mat;
    D3DMATRIX *view_mat;
    D3DMATRIX *proj_mat;
    D3DMATRIX *tex_mat[MAX_TEXTURES];
    BOOLEAN tex_mat_is_identity[MAX_TEXTURES];
    
    /* Current material used in D3D7 mode */
    D3DMATERIAL7 current_material;

    /* Light state */
    DWORD material;

    /* Light parameters */
    DWORD num_set_lights;
    DWORD max_active_lights;
    LPD3DLIGHT7 light_parameters;
    DWORD *active_lights;

    /* Clipping planes */
    DWORD max_clipping_planes;
    d3d7clippingplane *clipping_planes;
    
    void (*set_context)(IDirect3DDeviceImpl*);
    HRESULT (*clear)(IDirect3DDeviceImpl *This,
		     DWORD dwCount,
		     LPD3DRECT lpRects,
		     DWORD dwFlags,
		     DWORD dwColor,
		     D3DVALUE dvZ,
		     DWORD dwStencil);
    void (*matrices_updated)(IDirect3DDeviceImpl *This, DWORD matrices);
    void (*set_matrices)(IDirect3DDeviceImpl *This, DWORD matrices,
			 D3DMATRIX *world_mat, D3DMATRIX *view_mat, D3DMATRIX *proj_mat);
    void (*flush_to_framebuffer)(IDirect3DDeviceImpl *This, LPCRECT pRect, IDirectDrawSurfaceImpl *surf);

    STATEBLOCK state_block;

    /* Used to prevent locks and rendering to overlap */
    CRITICAL_SECTION crit;

    /* Rendering functions */
    D3DPRIMITIVETYPE primitive_type;
    DWORD vertex_type;
    DWORD render_flags;
    DWORD nb_vertices;
    LPBYTE vertex_buffer;
    DWORD vertex_size;
    DWORD buffer_size;
};

/*****************************************************************************
 * IDirect3DVertexBuffer implementation structure
 */
struct IDirect3DVertexBufferImpl
{
    ICOM_VFIELD_MULTI(IDirect3DVertexBuffer7);
    ICOM_VFIELD_MULTI(IDirect3DVertexBuffer);
    LONG ref;
    IDirectDrawImpl *d3d;
    D3DVERTEXBUFFERDESC desc;
    LPVOID *vertices;
    DWORD vertex_buffer_size;

    BOOLEAN processed;
};

/* Various dump and helper functions */
#define GET_TEXCOUNT_FROM_FVF(d3dvtVertexType) \
    (((d3dvtVertexType) & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)

#define GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_num) \
    (((((d3dvtVertexType) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1)

extern const char *_get_renderstate(D3DRENDERSTATETYPE type);
extern void dump_D3DMATERIAL7(LPD3DMATERIAL7 lpMat);
extern void dump_D3DCOLORVALUE(D3DCOLORVALUE *lpCol);
extern void dump_D3DLIGHT7(LPD3DLIGHT7 lpLight);
extern void dump_DPFLAGS(DWORD dwFlags);
extern void dump_D3DMATRIX(D3DMATRIX *mat);
extern void dump_D3DVECTOR(D3DVECTOR *lpVec);
extern void dump_flexible_vertex(DWORD d3dvtVertexType);
extern DWORD get_flexible_vertex_size(DWORD d3dvtVertexType);
extern void convert_FVF_to_strided_data(DWORD d3dvtVertexType, LPVOID lpvVertices, D3DDRAWPRIMITIVESTRIDEDDATA *strided, DWORD dwStartVertex);
extern void dump_D3DVOP(DWORD dwVertexOp);
extern void dump_D3DPV(DWORD dwFlags);
extern void multiply_matrix(LPD3DMATRIX,LPD3DMATRIX,LPD3DMATRIX);
extern void InitDefaultStateBlock(STATEBLOCK* lpStateBlock, int version);

extern const float id_mat[16];

/*****************************************************************************
 * IDirect3D object methods
 */
HRESULT WINAPI
Main_IDirect3DImpl_7_EnumDevices(LPDIRECT3D7 iface,
                                 LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback,
                                 LPVOID lpUserArg);

HRESULT WINAPI
Main_IDirect3DImpl_7_CreateDevice(LPDIRECT3D7 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE7 lpDDS,
                                  LPDIRECT3DDEVICE7* lplpD3DDevice);

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_CreateVertexBuffer(LPDIRECT3D7 iface,
					   LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
					   LPDIRECT3DVERTEXBUFFER7* lplpD3DVertBuf,
					   DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_EnumZBufferFormats(LPDIRECT3D7 iface,
                                           REFCLSID riidDevice,
                                           LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
                                           LPVOID lpContext);

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_EvictManagedTextures(LPDIRECT3D7 iface);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_EnumDevices(LPDIRECT3D3 iface,
                                       LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
                                       LPVOID lpUserArg);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateLight(LPDIRECT3D3 iface,
                                       LPDIRECT3DLIGHT* lplpDirect3DLight,
                                       IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateMaterial(LPDIRECT3D3 iface,
					  LPDIRECT3DMATERIAL3* lplpDirect3DMaterial3,
					  IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateViewport(LPDIRECT3D3 iface,
					  LPDIRECT3DVIEWPORT3* lplpD3DViewport3,
					  IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_FindDevice(LPDIRECT3D3 iface,
				      LPD3DFINDDEVICESEARCH lpD3DDFS,
				      LPD3DFINDDEVICERESULT lpD3DFDR);

HRESULT WINAPI
Main_IDirect3DImpl_3_CreateDevice(LPDIRECT3D3 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE4 lpDDS,
                                  LPDIRECT3DDEVICE3* lplpD3DDevice3,
                                  LPUNKNOWN lpUnk);

HRESULT WINAPI
Thunk_IDirect3DImpl_3_CreateVertexBuffer(LPDIRECT3D3 iface,
					 LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
					 LPDIRECT3DVERTEXBUFFER* lplpD3DVertBuf,
					 DWORD dwFlags,
					 LPUNKNOWN lpUnk);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateMaterial(LPDIRECT3D2 iface,
				     LPDIRECT3DMATERIAL2* lplpDirect3DMaterial2,
				     IUnknown* pUnkOuter);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateViewport(LPDIRECT3D2 iface,
				     LPDIRECT3DVIEWPORT2* lplpD3DViewport2,
				     IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_2_CreateDevice(LPDIRECT3D2 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE lpDDS,
                                  LPDIRECT3DDEVICE2* lplpD3DDevice2);

HRESULT WINAPI
Main_IDirect3DImpl_1_Initialize(LPDIRECT3D iface,
                                REFIID riid);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateMaterial(LPDIRECT3D iface,
				     LPDIRECT3DMATERIAL* lplpDirect3DMaterial,
				     IUnknown* pUnkOuter);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateViewport(LPDIRECT3D iface,
				     LPDIRECT3DVIEWPORT* lplpD3DViewport,
				     IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_1_FindDevice(LPDIRECT3D iface,
                                LPD3DFINDDEVICESEARCH lpD3DDFS,
                                LPD3DFINDDEVICERESULT lplpD3DDevice);

HRESULT WINAPI
Thunk_IDirect3DImpl_7_QueryInterface(LPDIRECT3D7 iface,
                                     REFIID riid,
                                     LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DImpl_3_QueryInterface(LPDIRECT3D3 iface,
                                     REFIID riid,
                                     LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_QueryInterface(LPDIRECT3D2 iface,
                                     REFIID riid,
                                     LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_QueryInterface(LPDIRECT3D iface,
                                     REFIID riid,
                                     LPVOID* obp);

ULONG WINAPI
Thunk_IDirect3DImpl_7_AddRef(LPDIRECT3D7 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_3_AddRef(LPDIRECT3D3 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_2_AddRef(LPDIRECT3D2 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_1_AddRef(LPDIRECT3D iface);

ULONG WINAPI
Thunk_IDirect3DImpl_7_Release(LPDIRECT3D7 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_3_Release(LPDIRECT3D3 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_2_Release(LPDIRECT3D2 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_1_Release(LPDIRECT3D iface);

HRESULT WINAPI
Thunk_IDirect3DImpl_3_EnumZBufferFormats(LPDIRECT3D3 iface,
                                         REFCLSID riidDevice,
                                         LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
                                         LPVOID lpContext);

HRESULT WINAPI
Thunk_IDirect3DImpl_3_EvictManagedTextures(LPDIRECT3D3 iface);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_EnumDevices(LPDIRECT3D2 iface,
                                  LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
                                  LPVOID lpUserArg);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_EnumDevices(LPDIRECT3D iface,
                                  LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
                                  LPVOID lpUserArg);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateLight(LPDIRECT3D2 iface,
                                  LPDIRECT3DLIGHT* lplpDirect3DLight,
                                  IUnknown* pUnkOuter);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateLight(LPDIRECT3D iface,
                                  LPDIRECT3DLIGHT* lplpDirect3DLight,
                                  IUnknown* pUnkOuter);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_FindDevice(LPDIRECT3D iface,
				 LPD3DFINDDEVICESEARCH lpD3DDFS,
				 LPD3DFINDDEVICERESULT lplpD3DDevice);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_FindDevice(LPDIRECT3D2 iface,
				 LPD3DFINDDEVICESEARCH lpD3DDFS,
				 LPD3DFINDDEVICERESULT lpD3DFDR);

/*****************************************************************************
 * IDirect3DDevice object methods
 */
HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_QueryInterface(LPDIRECT3DDEVICE7 iface,
                                                   REFIID riid,
                                                   LPVOID* obp);

ULONG WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_AddRef(LPDIRECT3DDEVICE7 iface);

ULONG WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_Release(LPDIRECT3DDEVICE7 iface);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetCaps(LPDIRECT3DDEVICE7 iface,
                                   LPD3DDEVICEDESC7 lpD3DHELDevDesc);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_EnumTextureFormats(LPDIRECT3DDEVICE7 iface,
                                                 LPD3DENUMPIXELFORMATSCALLBACK lpD3DEnumPixelProc,
                                                 LPVOID lpArg);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_BeginScene(LPDIRECT3DDEVICE7 iface);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_EndScene(LPDIRECT3DDEVICE7 iface);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_1T_GetDirect3D(LPDIRECT3DDEVICE7 iface,
						LPDIRECT3D7* lplpDirect3D3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetRenderTarget(LPDIRECT3DDEVICE7 iface,
						 LPDIRECTDRAWSURFACE7 lpNewRenderTarget,
						 DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetRenderTarget(LPDIRECT3DDEVICE7 iface,
						 LPDIRECTDRAWSURFACE7* lplpRenderTarget);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_Clear(LPDIRECT3DDEVICE7 iface,
                                 DWORD dwCount,
                                 LPD3DRECT lpRects,
                                 DWORD dwFlags,
                                 D3DCOLOR dwColor,
                                 D3DVALUE dvZ,
                                 DWORD dwStencil);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetTransform(LPDIRECT3DDEVICE7 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetTransform(LPDIRECT3DDEVICE7 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetViewport(LPDIRECT3DDEVICE7 iface,
                                       LPD3DVIEWPORT7 lpData);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_MultiplyTransform(LPDIRECT3DDEVICE7 iface,
                                                   D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                                   LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetViewport(LPDIRECT3DDEVICE7 iface,
                                       LPD3DVIEWPORT7 lpData);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetMaterial(LPDIRECT3DDEVICE7 iface,
                                       LPD3DMATERIAL7 lpMat);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetMaterial(LPDIRECT3DDEVICE7 iface,
                                       LPD3DMATERIAL7 lpMat);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetLight(LPDIRECT3DDEVICE7 iface,
                                    DWORD dwLightIndex,
                                    LPD3DLIGHT7 lpLight);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetLight(LPDIRECT3DDEVICE7 iface,
                                    DWORD dwLightIndex,
                                    LPD3DLIGHT7 lpLight);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetRenderState(LPDIRECT3DDEVICE7 iface,
                                                D3DRENDERSTATETYPE dwRenderStateType,
                                                DWORD dwRenderState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetRenderState(LPDIRECT3DDEVICE7 iface,
                                                D3DRENDERSTATETYPE dwRenderStateType,
                                                LPDWORD lpdwRenderState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_BeginStateBlock(LPDIRECT3DDEVICE7 iface);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_EndStateBlock(LPDIRECT3DDEVICE7 iface,
                                         LPDWORD lpdwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_PreLoad(LPDIRECT3DDEVICE7 iface,
                                   LPDIRECTDRAWSURFACE7 lpddsTexture);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitive(LPDIRECT3DDEVICE7 iface,
                                            D3DPRIMITIVETYPE d3dptPrimitiveType,
                                            DWORD d3dvtVertexType,
                                            LPVOID lpvVertices,
                                            DWORD dwVertexCount,
                                            DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitive(LPDIRECT3DDEVICE7 iface,
                                                   D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                   DWORD d3dvtVertexType,
                                                   LPVOID lpvVertices,
                                                   DWORD dwVertexCount,
                                                   LPWORD dwIndices,
                                                   DWORD dwIndexCount,
                                                   DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_SetClipStatus(LPDIRECT3DDEVICE7 iface,
                                               LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_2T_GetClipStatus(LPDIRECT3DDEVICE7 iface,
                                               LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitiveStrided(LPDIRECT3DDEVICE7 iface,
                                                   D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                   DWORD dwVertexType,
                                                   LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                   DWORD dwVertexCount,
                                                   DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveStrided(LPDIRECT3DDEVICE7 iface,
                                                          D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                          DWORD dwVertexType,
                                                          LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                          DWORD dwVertexCount,
                                                          LPWORD lpIndex,
                                                          DWORD dwIndexCount,
                                                          DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawPrimitiveVB(LPDIRECT3DDEVICE7 iface,
					      D3DPRIMITIVETYPE d3dptPrimitiveType,
					      LPDIRECT3DVERTEXBUFFER7 lpD3DVertexBuf,
					      DWORD dwStartVertex,
					      DWORD dwNumVertices,
					      DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE7 iface,
						     D3DPRIMITIVETYPE d3dptPrimitiveType,
						     LPDIRECT3DVERTEXBUFFER7 lpD3DVertexBuf,
						     DWORD dwStartVertex,
						     DWORD dwNumVertices,
						     LPWORD lpwIndices,
						     DWORD dwIndexCount,
						     DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_ComputeSphereVisibility(LPDIRECT3DDEVICE7 iface,
                                                      LPD3DVECTOR lpCenters,
                                                      LPD3DVALUE lpRadii,
                                                      DWORD dwNumSpheres,
                                                      DWORD dwFlags,
                                                      LPDWORD lpdwReturnValues);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_GetTexture(LPDIRECT3DDEVICE7 iface,
					 DWORD dwStage,
					 LPDIRECTDRAWSURFACE7* lpTexture);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_SetTexture(LPDIRECT3DDEVICE7 iface,
					 DWORD dwStage,
					 LPDIRECTDRAWSURFACE7 lpTexture);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_GetTextureStageState(LPDIRECT3DDEVICE7 iface,
                                                   DWORD dwStage,
                                                   D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                   LPDWORD lpdwState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_SetTextureStageState(LPDIRECT3DDEVICE7 iface,
                                                   DWORD dwStage,
                                                   D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                   DWORD dwState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_3T_ValidateDevice(LPDIRECT3DDEVICE7 iface,
                                             LPDWORD lpdwPasses);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_ApplyStateBlock(LPDIRECT3DDEVICE7 iface,
                                           DWORD dwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_CaptureStateBlock(LPDIRECT3DDEVICE7 iface,
                                             DWORD dwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_DeleteStateBlock(LPDIRECT3DDEVICE7 iface,
                                            DWORD dwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_CreateStateBlock(LPDIRECT3DDEVICE7 iface,
                                            D3DSTATEBLOCKTYPE d3dsbType,
                                            LPDWORD lpdwBlockHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_Load(LPDIRECT3DDEVICE7 iface,
                                LPDIRECTDRAWSURFACE7 lpDestTex,
                                LPPOINT lpDestPoint,
                                LPDIRECTDRAWSURFACE7 lpSrcTex,
                                LPRECT lprcSrcRect,
                                DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_LightEnable(LPDIRECT3DDEVICE7 iface,
                                       DWORD dwLightIndex,
                                       BOOL bEnable);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetLightEnable(LPDIRECT3DDEVICE7 iface,
                                          DWORD dwLightIndex,
                                          BOOL* pbEnable);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_SetClipPlane(LPDIRECT3DDEVICE7 iface,
                                        DWORD dwIndex,
                                        D3DVALUE* pPlaneEquation);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetClipPlane(LPDIRECT3DDEVICE7 iface,
                                        DWORD dwIndex,
                                        D3DVALUE* pPlaneEquation);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_7_GetInfo(LPDIRECT3DDEVICE7 iface,
                                   DWORD dwDevInfoID,
                                   LPVOID pDevInfoStruct,
                                   DWORD dwSize);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_GetCaps(LPDIRECT3DDEVICE3 iface,
                                         LPD3DDEVICEDESC lpD3DHWDevDesc,
                                         LPD3DDEVICEDESC lpD3DHELDevDesc);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_GetStats(LPDIRECT3DDEVICE3 iface,
                                          LPD3DSTATS lpD3DStats);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_AddViewport(LPDIRECT3DDEVICE3 iface,
					     LPDIRECT3DVIEWPORT3 lpDirect3DViewport3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_DeleteViewport(LPDIRECT3DDEVICE3 iface,
						LPDIRECT3DVIEWPORT3 lpDirect3DViewport3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_1T_NextViewport(LPDIRECT3DDEVICE3 iface,
					      LPDIRECT3DVIEWPORT3 lpDirect3DViewport3,
					      LPDIRECT3DVIEWPORT3* lplpDirect3DViewport3,
					      DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_SetCurrentViewport(LPDIRECT3DDEVICE3 iface,
						 LPDIRECT3DVIEWPORT3 lpDirect3DViewport3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_GetCurrentViewport(LPDIRECT3DDEVICE3 iface,
						 LPDIRECT3DVIEWPORT3* lplpDirect3DViewport3);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_Begin(LPDIRECT3DDEVICE3 iface,
                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                 DWORD dwVertexTypeDesc,
                                 DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_BeginIndexed(LPDIRECT3DDEVICE3 iface,
                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                        DWORD d3dvtVertexType,
                                        LPVOID lpvVertices,
                                        DWORD dwNumVertices,
                                        DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_Vertex(LPDIRECT3DDEVICE3 iface,
                                     LPVOID lpVertexType);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_Index(LPDIRECT3DDEVICE3 iface,
                                    WORD wVertexIndex);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_End(LPDIRECT3DDEVICE3 iface,
                                  DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_GetLightState(LPDIRECT3DDEVICE3 iface,
                                            D3DLIGHTSTATETYPE dwLightStateType,
                                            LPDWORD lpdwLightState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_3_2T_SetLightState(LPDIRECT3DDEVICE3 iface,
                                            D3DLIGHTSTATETYPE dwLightStateType,
                                            DWORD dwLightState);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_1T_SwapTextureHandles(LPDIRECT3DDEVICE2 iface,
                                                 LPDIRECT3DTEXTURE2 lpD3DTex1,
                                                 LPDIRECT3DTEXTURE2 lpD3DTex2);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_1T_EnumTextureFormats(LPDIRECT3DDEVICE2 iface,
                                                 LPD3DENUMTEXTUREFORMATSCALLBACK lpD3DEnumTextureProc,
                                                 LPVOID lpArg);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_Begin(LPDIRECT3DDEVICE2 iface,
                                 D3DPRIMITIVETYPE d3dpt,
                                 D3DVERTEXTYPE dwVertexTypeDesc,
                                 DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_BeginIndexed(LPDIRECT3DDEVICE2 iface,
                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                        D3DVERTEXTYPE d3dvtVertexType,
                                        LPVOID lpvVertices,
                                        DWORD dwNumVertices,
                                        DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_DrawPrimitive(LPDIRECT3DDEVICE2 iface,
                                         D3DPRIMITIVETYPE d3dptPrimitiveType,
                                         D3DVERTEXTYPE d3dvtVertexType,
                                         LPVOID lpvVertices,
                                         DWORD dwVertexCount,
                                         DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_2_DrawIndexedPrimitive(LPDIRECT3DDEVICE2 iface,
                                                D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                D3DVERTEXTYPE d3dvtVertexType,
                                                LPVOID lpvVertices,
                                                DWORD dwVertexCount,
                                                LPWORD dwIndices,
                                                DWORD dwIndexCount,
                                                DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Initialize(LPDIRECT3DDEVICE iface,
                                      LPDIRECT3D lpDirect3D,
                                      LPGUID lpGUID,
                                      LPD3DDEVICEDESC lpD3DDVDesc);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_CreateExecuteBuffer(LPDIRECT3DDEVICE iface,
                                               LPD3DEXECUTEBUFFERDESC lpDesc,
                                               LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
                                               IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Execute(LPDIRECT3DDEVICE iface,
                                   LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
                                   LPDIRECT3DVIEWPORT lpDirect3DViewport,
                                   DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_Pick(LPDIRECT3DDEVICE iface,
                                LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
                                LPDIRECT3DVIEWPORT lpDirect3DViewport,
                                DWORD dwFlags,
                                LPD3DRECT lpRect);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_GetPickRecords(LPDIRECT3DDEVICE iface,
                                          LPDWORD lpCount,
                                          LPD3DPICKRECORD lpD3DPickRec);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_CreateMatrix(LPDIRECT3DDEVICE iface,
                                        LPD3DMATRIXHANDLE lpD3DMatHandle);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_SetMatrix(LPDIRECT3DDEVICE iface,
                                     D3DMATRIXHANDLE D3DMatHandle,
                                     LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_GetMatrix(LPDIRECT3DDEVICE iface,
                                     D3DMATRIXHANDLE D3DMatHandle,
                                     LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Main_IDirect3DDeviceImpl_1_DeleteMatrix(LPDIRECT3DDEVICE iface,
                                        D3DMATRIXHANDLE D3DMatHandle);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_QueryInterface(LPDIRECT3DDEVICE3 iface,
                                           REFIID riid,
                                           LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_QueryInterface(LPDIRECT3DDEVICE2 iface,
                                           REFIID riid,
                                           LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_QueryInterface(LPDIRECT3DDEVICE iface,
                                           REFIID riid,
                                           LPVOID* obp);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_AddRef(LPDIRECT3DDEVICE3 iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_AddRef(LPDIRECT3DDEVICE2 iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_AddRef(LPDIRECT3DDEVICE iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_Release(LPDIRECT3DDEVICE3 iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_Release(LPDIRECT3DDEVICE2 iface);

ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_Release(LPDIRECT3DDEVICE iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_AddViewport(LPDIRECT3DDEVICE2 iface,
					LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_AddViewport(LPDIRECT3DDEVICE iface,
					LPDIRECT3DVIEWPORT lpDirect3DViewport);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_DeleteViewport(LPDIRECT3DDEVICE2 iface,
					   LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_DeleteViewport(LPDIRECT3DDEVICE iface,
					   LPDIRECT3DVIEWPORT lpDirect3DViewport);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_NextViewport(LPDIRECT3DDEVICE2 iface,
					 LPDIRECT3DVIEWPORT2 lpDirect3DViewport2,
					 LPDIRECT3DVIEWPORT2* lplpDirect3DViewport2,
					 DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_NextViewport(LPDIRECT3DDEVICE iface,
					 LPDIRECT3DVIEWPORT lpDirect3DViewport,
					 LPDIRECT3DVIEWPORT* lplpDirect3DViewport,
					 DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetDirect3D(LPDIRECT3DDEVICE3 iface,
					LPDIRECT3D3* lplpDirect3D3);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetDirect3D(LPDIRECT3DDEVICE2 iface,
					LPDIRECT3D2* lplpDirect3D2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetDirect3D(LPDIRECT3DDEVICE iface,
					LPDIRECT3D* lplpDirect3D);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetCurrentViewport(LPDIRECT3DDEVICE2 iface,
					       LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCurrentViewport(LPDIRECT3DDEVICE2 iface,
					       LPDIRECT3DVIEWPORT2* lpDirect3DViewport2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EnumTextureFormats(LPDIRECT3DDEVICE3 iface,
                                               LPD3DENUMPIXELFORMATSCALLBACK lpD3DEnumPixelProc,
                                               LPVOID lpArg);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_BeginScene(LPDIRECT3DDEVICE3 iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_BeginScene(LPDIRECT3DDEVICE2 iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_BeginScene(LPDIRECT3DDEVICE iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EndScene(LPDIRECT3DDEVICE3 iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_EndScene(LPDIRECT3DDEVICE2 iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EndScene(LPDIRECT3DDEVICE iface);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTransform(LPDIRECT3DDEVICE3 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetTransform(LPDIRECT3DDEVICE2 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTransform(LPDIRECT3DDEVICE3 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetTransform(LPDIRECT3DDEVICE2 iface,
                                         D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                         LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_MultiplyTransform(LPDIRECT3DDEVICE3 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_MultiplyTransform(LPDIRECT3DDEVICE2 iface,
                                              D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                              LPD3DMATRIX lpD3DMatrix);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderState(LPDIRECT3DDEVICE3 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD dwRenderState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderState(LPDIRECT3DDEVICE2 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD dwRenderState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderState(LPDIRECT3DDEVICE3 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           LPDWORD lpdwRenderState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderState(LPDIRECT3DDEVICE2 iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           LPDWORD lpdwRenderState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitive(LPDIRECT3DDEVICE3 iface,
                                          D3DPRIMITIVETYPE d3dptPrimitiveType,
                                          DWORD d3dvtVertexType,
                                          LPVOID lpvVertices,
                                          DWORD dwVertexCount,
                                          DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitive(LPDIRECT3DDEVICE3 iface,
                                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                 DWORD d3dvtVertexType,
                                                 LPVOID lpvVertices,
                                                 DWORD dwVertexCount,
                                                 LPWORD dwIndices,
                                                 DWORD dwIndexCount,
                                                 DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetClipStatus(LPDIRECT3DDEVICE3 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetClipStatus(LPDIRECT3DDEVICE2 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetClipStatus(LPDIRECT3DDEVICE3 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetClipStatus(LPDIRECT3DDEVICE2 iface,
                                          LPD3DCLIPSTATUS lpD3DClipStatus);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveStrided(LPDIRECT3DDEVICE3 iface,
                                                 D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                 DWORD dwVertexType,
                                                 LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                 DWORD dwVertexCount,
                                                 DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveStrided(LPDIRECT3DDEVICE3 iface,
                                                        D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                        DWORD dwVertexType,
                                                        LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                        DWORD dwVertexCount,
                                                        LPWORD lpIndex,
                                                        DWORD dwIndexCount,
                                                        DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ComputeSphereVisibility(LPDIRECT3DDEVICE3 iface,
                                                    LPD3DVECTOR lpCenters,
                                                    LPD3DVALUE lpRadii,
                                                    DWORD dwNumSpheres,
                                                    DWORD dwFlags,
                                                    LPDWORD lpdwReturnValues);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTextureStageState(LPDIRECT3DDEVICE3 iface,
                                                 DWORD dwStage,
                                                 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                 LPDWORD lpdwState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTextureStageState(LPDIRECT3DDEVICE3 iface,
                                                 DWORD dwStage,
                                                 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
                                                 DWORD dwState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ValidateDevice(LPDIRECT3DDEVICE3 iface,
                                           LPDWORD lpdwPasses);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCaps(LPDIRECT3DDEVICE2 iface,
                                    LPD3DDEVICEDESC lpD3DHWDevDesc,
                                    LPD3DDEVICEDESC lpD3DHELDevDesc);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetCaps(LPDIRECT3DDEVICE iface,
                                    LPD3DDEVICEDESC lpD3DHWDevDesc,
                                    LPD3DDEVICEDESC lpD3DHELDevDesc);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_SwapTextureHandles(LPDIRECT3DDEVICE iface,
                                               LPDIRECT3DTEXTURE lpD3Dtex1,
                                               LPDIRECT3DTEXTURE lpD3DTex2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetStats(LPDIRECT3DDEVICE2 iface,
                                     LPD3DSTATS lpD3DStats);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetStats(LPDIRECT3DDEVICE iface,
                                     LPD3DSTATS lpD3DStats);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderTarget(LPDIRECT3DDEVICE3 iface,
                                            LPDIRECTDRAWSURFACE4 lpNewRenderTarget,
                                            DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderTarget(LPDIRECT3DDEVICE3 iface,
                                            LPDIRECTDRAWSURFACE4* lplpRenderTarget);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderTarget(LPDIRECT3DDEVICE2 iface,
                                            LPDIRECTDRAWSURFACE lpNewRenderTarget,
                                            DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderTarget(LPDIRECT3DDEVICE2 iface,
                                            LPDIRECTDRAWSURFACE* lplpRenderTarget);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Vertex(LPDIRECT3DDEVICE2 iface,
                                   LPVOID lpVertexType);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Index(LPDIRECT3DDEVICE2 iface,
                                  WORD wVertexIndex);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_End(LPDIRECT3DDEVICE2 iface,
                                DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetLightState(LPDIRECT3DDEVICE2 iface,
                                          D3DLIGHTSTATETYPE dwLightStateType,
                                          LPDWORD lpdwLightState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetLightState(LPDIRECT3DDEVICE2 iface,
                                          D3DLIGHTSTATETYPE dwLightStateType,
                                          DWORD dwLightState);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EnumTextureFormats(LPDIRECT3DDEVICE iface,
                                               LPD3DENUMTEXTUREFORMATSCALLBACK lpD3DEnumTextureProc,
                                               LPVOID lpArg);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTexture(LPDIRECT3DDEVICE3 iface,
				       DWORD dwStage,
				       LPDIRECT3DTEXTURE2 lpTexture2);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveVB(LPDIRECT3DDEVICE3 iface,
					    D3DPRIMITIVETYPE d3dptPrimitiveType,
					    LPDIRECT3DVERTEXBUFFER lpD3DVertexBuf,
					    DWORD dwStartVertex,
					    DWORD dwNumVertices,
					    DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE3 iface,
						   D3DPRIMITIVETYPE d3dptPrimitiveType,
						   LPDIRECT3DVERTEXBUFFER lpD3DVertexBuf,
						   LPWORD lpwIndices,
						   DWORD dwIndexCount,
						   DWORD dwFlags);

HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTexture(LPDIRECT3DDEVICE3 iface,
				       DWORD dwStage,
				       LPDIRECT3DTEXTURE2* lplpTexture2);

#endif /* __GRAPHICS_WINE_D3D_PRIVATE_H */
