/* Direct3D private include file
 * Copyright (c) 1998 Lionel ULMER
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __GRAPHICS_WINE_D3D_PRIVATE_H
#define __GRAPHICS_WINE_D3D_PRIVATE_H

/* THIS FILE MUST NOT CONTAIN X11 or MESA DEFINES */

#include "d3d.h"

#define HIGHEST_RENDER_STATE         152
#define HIGHEST_TEXTURE_STAGE_STATE   24
#define HIGHEST_LIGHT_STATE            8

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirect3DImpl IDirect3DImpl;
typedef struct IDirect3DLightImpl IDirect3DLightImpl;
typedef struct IDirect3DMaterialImpl IDirect3DMaterialImpl;
typedef struct IDirect3DViewportImpl IDirect3DViewportImpl;
typedef struct IDirect3DExecuteBufferImpl IDirect3DExecuteBufferImpl;
typedef struct IDirect3DDeviceImpl IDirect3DDeviceImpl;
typedef struct IDirect3DVertexBufferImpl IDirect3DVertexBufferImpl;

#include "ddraw_private.h"

typedef struct STATEBLOCKFLAGS {
   BOOL render_state[HIGHEST_RENDER_STATE];
   BOOL texture_stage_state[8][HIGHEST_TEXTURE_STAGE_STATE];
   BOOL light_state[HIGHEST_LIGHT_STATE];
} STATEBLOCKFLAGS;

typedef struct STATEBLOCK {
   STATEBLOCKFLAGS set_flags; 
   DWORD render_state[HIGHEST_RENDER_STATE];
   DWORD texture_stage_state[8][HIGHEST_TEXTURE_STAGE_STATE];
   DWORD light_state[HIGHEST_LIGHT_STATE];
} STATEBLOCK;

/*****************************************************************************
 * IDirect3D implementation structure.
 * This is common for interfaces 1, 2, 3 and 7.
 */
struct IDirect3DImpl
{
    ICOM_VFIELD_MULTI(IDirect3D7);
    ICOM_VFIELD_MULTI(IDirect3D3);
    ICOM_VFIELD_MULTI(IDirect3D2);
    ICOM_VFIELD_MULTI(IDirect3D);
    DWORD                   ref;
    /* IDirect3D fields */
    IDirectDrawImpl*	ddraw;

    /* Used as a callback function to create a texture */
    HRESULT (*create_texture)(IDirect3DImpl *d3d, IDirectDrawSurfaceImpl *tex, BOOLEAN at_creation, IDirectDrawSurfaceImpl *main);

    /* Used as a callback for Devices to tell to the D3D object it's been created */
    HRESULT (*added_device)(IDirect3DImpl *d3d, IDirect3DDeviceImpl *device);
    HRESULT (*removed_device)(IDirect3DImpl *d3d, IDirect3DDeviceImpl *device);

    /* This is needed for delayed texture creation and Z buffer blits */
    IDirect3DDeviceImpl *current_device;
};

/*****************************************************************************
 * IDirect3DLight implementation structure
 */
struct IDirect3DLightImpl
{
    ICOM_VFIELD_MULTI(IDirect3DLight);
    DWORD ref;
    /* IDirect3DLight fields */
    IDirect3DImpl *d3d;

    D3DLIGHT2 light;

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
    DWORD  ref;
    /* IDirect3DMaterial2 fields */
    IDirect3DImpl *d3d;
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
    DWORD ref;
    /* IDirect3DViewport fields */
    IDirect3DImpl *d3d;
    /* If this viewport is active for one device, put the device here */
    IDirect3DDeviceImpl *active_device;

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
    DWORD ref;
    /* IDirect3DExecuteBuffer fields */
    IDirect3DImpl *d3d;
    IDirect3DDeviceImpl* d3ddev;

    D3DEXECUTEBUFFERDESC desc;
    D3DEXECUTEDATA data;

    /* This buffer will store the transformed vertices */
    void* vertex_data;
    D3DVERTEXTYPE vertex_type;

    /* This flags is set to TRUE if we allocated ourselves the
       data buffer */
    BOOL need_free;

    void (*execute)(IDirect3DExecuteBufferImpl* this,
                    IDirect3DDeviceImpl* dev,
                    IDirect3DViewportImpl* vp);
};

/*****************************************************************************
 * IDirect3DDevice implementation structure
 */

#define MAX_TEXTURES 8
#define MAX_LIGHTS  16

#define WORLDMAT_CHANGED (0x00000001 << 0)
#define VIEWMAT_CHANGED  (0x00000001 << 1)
#define PROJMAT_CHANGED  (0x00000001 << 2)

struct IDirect3DDeviceImpl
{
    ICOM_VFIELD_MULTI(IDirect3DDevice7);
    ICOM_VFIELD_MULTI(IDirect3DDevice3);
    ICOM_VFIELD_MULTI(IDirect3DDevice2);
    ICOM_VFIELD_MULTI(IDirect3DDevice);
    DWORD  ref;
    /* IDirect3DDevice fields */
    IDirect3DImpl *d3d;
    IDirectDrawSurfaceImpl *surface;

    IDirect3DViewportImpl *viewport_list;
    IDirect3DViewportImpl *current_viewport;
    D3DVIEWPORT7 active_viewport;

    IDirectDrawSurfaceImpl *current_texture[MAX_TEXTURES];

    /* Current transformation matrices */
    D3DMATRIX *world_mat;
    D3DMATRIX *view_mat;
    D3DMATRIX *proj_mat;

    /* Current material used in D3D7 mode */
    D3DMATERIAL7 current_material;

    /* Light parameters */
    DWORD active_lights, set_lights;
    D3DLIGHT7 light_parameters[MAX_LIGHTS];

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

    STATEBLOCK state_block;
};

/*****************************************************************************
 * IDirect3DVertexBuffer implementation structure
 */
struct IDirect3DVertexBufferImpl
{
    ICOM_VFIELD_MULTI(IDirect3DVertexBuffer7);
    ICOM_VFIELD_MULTI(IDirect3DVertexBuffer);
    DWORD ref;
    IDirect3DImpl *d3d;
    D3DVERTEXBUFFERDESC desc;
    LPVOID *vertices;
    DWORD vertex_buffer_size;

    BOOLEAN processed;
};

/* Various dump and helper functions */
extern const char *_get_renderstate(D3DRENDERSTATETYPE type);
extern void dump_D3DMATERIAL7(LPD3DMATERIAL7 lpMat);
extern void dump_D3DCOLORVALUE(D3DCOLORVALUE *lpCol);
extern void dump_D3DLIGHT7(LPD3DLIGHT7 lpLight);
extern void dump_DPFLAGS(DWORD dwFlags);
extern void dump_D3DMATRIX(D3DMATRIX *mat);
extern void dump_D3DVECTOR(D3DVECTOR *lpVec);
extern void dump_flexible_vertex(DWORD d3dvtVertexType);
extern DWORD get_flexible_vertex_size(DWORD d3dvtVertexType);
extern void convert_FVF_to_strided_data(DWORD d3dvtVertexType, LPVOID lpvVertices, D3DDRAWPRIMITIVESTRIDEDDATA *strided);
extern void dump_D3DVOP(DWORD dwVertexOp);
extern void dump_D3DPV(DWORD dwFlags);

extern const float id_mat[16];

#endif /* __GRAPHICS_WINE_D3D_PRIVATE_H */
