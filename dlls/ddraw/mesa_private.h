/* MESA private include file
 * Copyright (c) 1998 Lionel ULMER
 *
 * This file contains all structures that are not exported
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

#ifndef __GRAPHICS_WINE_MESA_PRIVATE_H
#define __GRAPHICS_WINE_MESA_PRIVATE_H

#include "d3d_private.h"

#ifdef HAVE_OPENGL

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

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

extern const GUID IID_D3DDEVICE_OpenGL;
extern const GUID IID_D3DDEVICE2_OpenGL;
extern const GUID IID_D3DDEVICE3_OpenGL;
extern const GUID IID_D3DDEVICE7_OpenGL;
extern const GUID IID_D3DDEVICE_Default;

typedef struct render_state {
    /* This is used for the device mode */
    GLenum src, dst;
    /* This is used for textures */
    GLenum mag, min;
} RenderState;

/* Common functions defined in d3dcommon.c */
void set_render_state(D3DRENDERSTATETYPE dwRenderStateType,
		      DWORD dwRenderState, RenderState *rs) ;

typedef struct IDirect3DGLImpl
{
    struct IDirect3DImpl parent;
    int free_lights;
    void (*light_released)(IDirect3DImpl *, GLenum light_num);
} IDirect3DGLImpl;

typedef struct IDirect3DLightGLImpl
{
    struct IDirect3DLightImpl parent;
    GLenum light_num;
} IDirect3DLightGLImpl;

typedef struct IDirect3DTextureGLImpl
{
    struct IDirect3DTextureImpl parent;
    GLuint tex_name;
} IDirect3DTextureGLImpl;

typedef struct IDirect3DDeviceGLImpl
{
    struct IDirect3DDeviceImpl parent;
    
    GLXContext gl_context;

    /* The current render state */
    RenderState render_state;

    /* The last type of vertex drawn */
    D3DVERTEXTYPE vertex_type;

    D3DMATRIX *world_mat;
    D3DMATRIX *view_mat;
    D3DMATRIX *proj_mat;
    
    Display  *display;
    Drawable drawable;
} IDirect3DDeviceGLImpl;

/* All non-static functions 'exported' by various sub-objects */
extern HRESULT direct3d_create(IDirect3DImpl **obj, IDirectDrawImpl *ddraw);
extern HRESULT d3dtexture_create(IDirect3DTextureImpl **obj, IDirect3DImpl *d3d, IDirectDrawSurfaceImpl *surf);
extern HRESULT d3dlight_create(IDirect3DLightImpl **obj, IDirect3DImpl *d3d, GLenum light_num);
extern HRESULT d3dexecutebuffer_create(IDirect3DExecuteBufferImpl **obj, IDirect3DImpl *d3d, IDirect3DDeviceImpl *d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc);
extern HRESULT d3dmaterial_create(IDirect3DMaterialImpl **obj, IDirect3DImpl *d3d);
extern HRESULT d3dviewport_create(IDirect3DViewportImpl **obj, IDirect3DImpl *d3d);
extern HRESULT d3dvertexbuffer_create(IDirect3DVertexBufferImpl **obj, IDirect3DImpl *d3d, LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc);
extern HRESULT d3ddevice_create(IDirect3DDeviceImpl **obj, IDirect3DImpl *d3d, IDirectDrawSurfaceImpl *surface);

/* Used for Direct3D to request the device to enumerate itself */
extern HRESULT d3device_enumerate(LPD3DENUMDEVICESCALLBACK cb, LPVOID context, DWORD interface_version) ;

/* Matrix copy WITH transposition */
#define conv_mat2(mat,gl_mat)			\
{						\
    TRACE("%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
    TRACE("%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
    TRACE("%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
    TRACE("%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44); \
    (gl_mat)[ 0] = (mat)->_11;			\
    (gl_mat)[ 1] = (mat)->_21;			\
    (gl_mat)[ 2] = (mat)->_31;			\
    (gl_mat)[ 3] = (mat)->_41;			\
    (gl_mat)[ 4] = (mat)->_12;			\
    (gl_mat)[ 5] = (mat)->_22;			\
    (gl_mat)[ 6] = (mat)->_32;			\
    (gl_mat)[ 7] = (mat)->_42;			\
    (gl_mat)[ 8] = (mat)->_13;			\
    (gl_mat)[ 9] = (mat)->_23;			\
    (gl_mat)[10] = (mat)->_33;			\
    (gl_mat)[11] = (mat)->_43;			\
    (gl_mat)[12] = (mat)->_14;			\
    (gl_mat)[13] = (mat)->_24;			\
    (gl_mat)[14] = (mat)->_34;			\
    (gl_mat)[15] = (mat)->_44;			\
};

/* Matrix copy WITHOUT transposition */
#define conv_mat(mat,gl_mat)			\
{                                               \
    TRACE("%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
    TRACE("%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
    TRACE("%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
    TRACE("%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44); \
    memcpy(gl_mat, (mat), 16 * sizeof(float));      \
};

#define _dump_colorvalue(s,v)                \
    TRACE(" " s " : %f %f %f %f\n",          \
    (v).u1.r, (v).u2.g, (v).u3.b, (v).u4.a);

/* This structure contains all the function pointers to OpenGL extensions
   that are used by Wine */
typedef struct {
  void (*ptr_ColorTableEXT) (GLenum target, GLenum internalformat,
			     GLsizei width, GLenum format, GLenum type, const GLvoid *table);
} Mesa_DeviceCapabilities;

#endif /* HAVE_OPENGL */

#endif /* __GRAPHICS_WINE_MESA_PRIVATE_H */
