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
    HRESULT (*create_texture)(IDirect3DImpl *d3d, IDirectDrawSurfaceImpl *tex, BOOLEAN at_creation, IDirectDrawSurfaceImpl *main, DWORD mipmap_level);
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

    void (*set_context)(IDirect3DDeviceImpl*);
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
};

/* Various dump functions */
extern const char *_get_renderstate(D3DRENDERSTATETYPE type);

#define dump_mat(mat) \
    TRACE("%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
    TRACE("%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
    TRACE("%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
    TRACE("%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44);

#endif /* __GRAPHICS_WINE_D3D_PRIVATE_H */
