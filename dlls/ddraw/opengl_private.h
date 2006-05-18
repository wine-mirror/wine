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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __GRAPHICS_WINE_MESA_PRIVATE_H
#define __GRAPHICS_WINE_MESA_PRIVATE_H

#include "d3d_private.h"

#ifdef HAVE_OPENGL

#include "gl_private.h"

/* X11 locking */

extern void (*wine_tsx11_lock_ptr)(void);
extern void (*wine_tsx11_unlock_ptr)(void);

/* As GLX relies on X, this is needed */
#define ENTER_GL() wine_tsx11_lock_ptr()
#define LEAVE_GL() wine_tsx11_unlock_ptr()

extern const GUID IID_D3DDEVICE_OpenGL;

typedef enum {
    SURFACE_GL,
    SURFACE_MEMORY,
    SURFACE_MEMORY_DIRTY
} SURFACE_STATE;

/* This structure is used for the 'd3d_private' field of the IDirectDraw structure */
typedef struct IDirect3DGLImpl
{
    int dummy; /* Empty for the moment */
} IDirect3DGLImpl;

/* This structure is used for the 'private' field of the IDirectDrawSurfaceImpl structure */
typedef struct IDirect3DTextureGLImpl
{
    GLuint tex_name;
    BOOLEAN loaded; /* For the moment, this is here.. Should be part of surface management though */
    IDirectDrawSurfaceImpl *main; /* Pointer to the 'main' surface of the mip-map set */
    
    /* Texture upload management */
    BOOLEAN initial_upload_done;
    SURFACE_STATE dirty_flag;

    /* This is used to optimize dirty checking in case of mipmapping.
       Note that a bitmap could have been used but it was not worth the pain as it will be very rare
       to have only one mipmap level change...

       The __global_dirty_flag will only be set for the main mipmap level.
    */
    SURFACE_STATE __global_dirty_flag;
    SURFACE_STATE *global_dirty_flag;
    
    /* This is to optimize the 'per-texture' parameters. */
    DWORD *tex_parameters;
    
    /* Surface optimization */
    void *surface_ptr;

    /* Used to detect a change in internal format when going from non-CK texture to CK-ed texture */
    GLenum current_internal_format;
    
    /* This is for now used to override 'standard' surface stuff to be as transparent as possible */
    void (*final_release)(struct IDirectDrawSurfaceImpl *This);
    void (*lock_update)(IDirectDrawSurfaceImpl* This, LPCRECT pRect, DWORD dwFlags);
    void (*unlock_update)(IDirectDrawSurfaceImpl* This, LPCRECT pRect);
    void (*set_palette)(IDirectDrawSurfaceImpl* This, IDirectDrawPaletteImpl* pal);
} IDirect3DTextureGLImpl;

typedef enum {
    GL_TRANSFORM_NONE = 0,
    GL_TRANSFORM_ORTHO,
    GL_TRANSFORM_NORMAL,
    GL_TRANSFORM_VERTEXBUFFER
} GL_TRANSFORM_STATE;

typedef enum {
    WINE_GL_BUFFER_BACK = 0,
    WINE_GL_BUFFER_FRONT
} WINE_GL_BUFFER_TYPE;

typedef struct IDirect3DDeviceGLImpl
{
    struct IDirect3DDeviceImpl parent;
    
    GLXContext gl_context;

    /* This stores the textures which are actually bound to the GL context */
    IDirectDrawSurfaceImpl *current_bound_texture[MAX_TEXTURES];

    /* The last type of vertex drawn */
    GL_TRANSFORM_STATE transform_state;

    /* Used to handle fogging faster... */
    BYTE fog_table[3 * 0x10000]; /* 3 is for R, G and B
				    0x10000 is 0xFF for the vertex color and
				               0xFF for the fog intensity */
    
    Display  *display;
    Drawable drawable;

    /* Variables used for the flush to frame-buffer code using the texturing code */
    GLuint unlock_tex;
    void *surface_ptr;
    GLenum current_internal_format;

    /* 0 is back-buffer, 1 is front-buffer */
    SURFACE_STATE state[2];
    IDirectDrawSurfaceImpl *lock_surf[2];
    RECT lock_rect[2];
    /* This is just here to print-out a nice warning if we have two successive locks */
    BOOLEAN lock_rect_valid[2];

    /* This is used to optimize some stuff */
    DWORD prev_clear_color;
    DWORD prev_clear_stencil;
    D3DVALUE prev_clear_Z;
    BOOLEAN depth_mask, depth_test, alpha_test, stencil_test, cull_face, lighting, blending, fogging;
    GLenum current_alpha_test_func;
    GLclampf current_alpha_test_ref;
    GLenum current_tex_env;
    GLenum current_active_tex_unit;
} IDirect3DDeviceGLImpl;

/* This is for the OpenGL additions... */
typedef struct {
    struct IDirect3DVertexBufferImpl parent;

    DWORD dwVertexTypeDesc;
    D3DMATRIX world_mat, view_mat, proj_mat;
    LPVOID vertices;
} IDirect3DVertexBufferGLImpl;

/* This is for GL extension support.
   
   This can contain either only a boolean if no function pointer exists or a set
   of function pointers.
*/
typedef struct {
    /* Mirrored Repeat */
    BOOLEAN mirrored_repeat;
    /* Mipmap lod-bias */
    BOOLEAN mipmap_lodbias;
    /* Multi-texturing */
    GLint max_texture_units;
    void (*glActiveTexture)(GLenum texture);
    void (*glMultiTexCoord[4])(GLenum target, const GLfloat *v);
    void (*glClientActiveTexture)(GLenum texture);
    /* S3TC/DXTN compressed texture */
    BOOLEAN s3tc_compressed_texture;
    void (*glCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                 GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
    void (*glCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                    GLsizei width, GLsizei height, GLsizei imageSize, const GLvoid *data);
} GL_EXTENSIONS_LIST; 
extern GL_EXTENSIONS_LIST GL_extensions;

/* All non-static functions 'exported' by various sub-objects */
extern HRESULT direct3d_create(IDirectDrawImpl *This);
extern HRESULT d3dtexture_create(IDirectDrawImpl *d3d, IDirectDrawSurfaceImpl *surf, BOOLEAN at_creation, IDirectDrawSurfaceImpl *main_surf);
extern HRESULT d3dlight_create(IDirect3DLightImpl **obj, IDirectDrawImpl *d3d);
extern HRESULT d3dexecutebuffer_create(IDirect3DExecuteBufferImpl **obj, IDirectDrawImpl *d3d, IDirect3DDeviceImpl *d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc);
extern HRESULT d3dmaterial_create(IDirect3DMaterialImpl **obj, IDirectDrawImpl *d3d);
extern HRESULT d3dviewport_create(IDirect3DViewportImpl **obj, IDirectDrawImpl *d3d);
extern HRESULT d3dvertexbuffer_create(IDirect3DVertexBufferImpl **obj, IDirectDrawImpl *d3d, LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc, DWORD dwFlags);
extern HRESULT d3ddevice_create(IDirect3DDeviceImpl **obj, IDirectDrawImpl *d3d, IDirectDrawSurfaceImpl *surface, int version);

/* Used for Direct3D to request the device to enumerate itself */
extern HRESULT d3ddevice_enumerate(LPD3DENUMDEVICESCALLBACK cb, LPVOID context, DWORD version) ;
extern HRESULT d3ddevice_enumerate7(LPD3DENUMDEVICESCALLBACK7 cb, LPVOID context) ;
extern HRESULT d3ddevice_find(IDirectDrawImpl *d3d, LPD3DFINDDEVICESEARCH lpD3DDFS, LPD3DFINDDEVICERESULT lplpD3DDevice);

/* Used by the DLL init routine to set-up the GL context and stuff properly */
extern BOOL d3ddevice_init_at_startup(void *gl_handle);

/* Used to upload the texture */
extern HRESULT gltex_upload_texture(IDirectDrawSurfaceImpl *This, IDirect3DDeviceImpl *d3ddev, DWORD stage) ;

/* Used to get the texture name */
extern GLuint gltex_get_tex_name(IDirectDrawSurfaceImpl *This) ;

/* Used to set-up our orthographic projection */
extern void d3ddevice_set_ortho(IDirect3DDeviceImpl *This) ;

/* Rendering state management functions */
extern void set_render_state(IDirect3DDeviceImpl* This, D3DRENDERSTATETYPE dwRenderStateType, STATEBLOCK *lpStateBlock);
extern void store_render_state(IDirect3DDeviceImpl *This, D3DRENDERSTATETYPE dwRenderStateType, DWORD dwRenderState, STATEBLOCK* lpStateBlock);
extern void get_render_state(IDirect3DDeviceImpl *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState, STATEBLOCK* lpStateBlock);
extern void apply_render_state(IDirect3DDeviceImpl* This, STATEBLOCK* lpStateBlock);

/* Memory to texture conversion code. Split in three functions to do some optimizations. */
extern HRESULT upload_surface_to_tex_memory_init(IDirectDrawSurfaceImpl *surface, GLuint level, GLenum *prev_internal_format,
						 BOOLEAN need_to_alloc, BOOLEAN need_alpha_ck, DWORD tex_width, DWORD tex_height);
extern HRESULT upload_surface_to_tex_memory(RECT *rect, DWORD xoffset, DWORD yoffset, void **temp_buffer);
extern HRESULT upload_surface_to_tex_memory_release(void);

/* Some utilities functions needed to be shared.. */
extern GLenum convert_D3D_compare_to_GL(D3DCMPFUNC dwRenderState) ;

#endif /* HAVE_OPENGL */

#endif /* __GRAPHICS_WINE_MESA_PRIVATE_H */
