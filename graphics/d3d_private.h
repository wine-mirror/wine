/* Direct3D private include file
   (c) 1998 Lionel ULMER
   
   This files contains all the structure that are not exported
   through d3d.h and all common macros. */

#ifndef __GRAPHICS_WINE_D3D_PRIVATE_H
#define __GRAPHICS_WINE_D3D_PRIVATE_H

#include "wine_gl.h"
#include "d3d.h"

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirect3DImpl IDirect3DImpl;
typedef struct IDirect3D2Impl IDirect3D2Impl;
typedef struct IDirect3DLightImpl IDirect3DLightImpl;
typedef struct IDirect3DMaterial2Impl IDirect3DMaterial2Impl;
typedef struct IDirect3DTexture2Impl IDirect3DTexture2Impl;
typedef struct IDirect3DViewport2Impl IDirect3DViewport2Impl;
typedef struct IDirect3DExecuteBufferImpl IDirect3DExecuteBufferImpl;
typedef struct IDirect3DDeviceImpl IDirect3DDeviceImpl;
typedef struct IDirect3DDevice2Impl IDirect3DDevice2Impl;

#include "ddraw_private.h"

/*****************************************************************************
 * IDirect3D implementation structure
 */
struct IDirect3DImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3D)* lpvtbl;
    DWORD                   ref;
    /* IDirect3D fields */
    IDirectDrawImpl* ddraw;
};

/*****************************************************************************
 * IDirect3D2 implementation structure
 */
struct IDirect3D2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3D2)* lpvtbl;
    DWORD                    ref;
    /* IDirect3D2 fields */
    IDirectDrawImpl* ddraw;
};

/*****************************************************************************
 * IDirect3DLight implementation structure
 */
struct IDirect3DLightImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3DLight)* lpvtbl;
    DWORD                        ref;
    /* IDirect3DLight fields */
    union {
        IDirect3DImpl*  d3d1;
        IDirect3D2Impl* d3d2;
    } d3d;
    int                 type;
  
    D3DLIGHT2           light;

    /* Chained list used for adding / removing from viewports */
    IDirect3DLightImpl *next, *prev;

    /* Activation function */
    void (*activate)(IDirect3DLightImpl*);
    int                 is_active;
  
    /* Awful OpenGL code !!! */
#ifdef HAVE_MESAGL
    GLenum              light_num;
#endif
};

/*****************************************************************************
 * IDirect3DMaterial2 implementation structure
 */
struct IDirect3DMaterial2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3DMaterial2)* lpvtbl;
    DWORD                            ref;
    /* IDirect3DMaterial2 fields */
    union {
        IDirect3DImpl*        d3d1;
        IDirect3D2Impl*       d3d2;
    } d3d;
    union {
        IDirect3DDeviceImpl*  active_device1;
        IDirect3DDevice2Impl* active_device2;
    } device;
    int                       use_d3d2;

    D3DMATERIAL               mat;

    void (*activate)(IDirect3DMaterial2Impl* this);
};

/*****************************************************************************
 * IDirect3DTexture2 implementation structure
 */
struct IDirect3DTexture2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3DTexture2)* lpvtbl;
    DWORD                           ref;
    /* IDirect3DTexture2 fields */
    void*                    D3Ddevice; /* I put (void *) to use the same pointer for both
		                           Direct3D and Direct3D2 */
#ifdef HAVE_MESAGL
    GLuint                   tex_name;
#endif  
    IDirectDrawSurface4Impl* surface;
};

/*****************************************************************************
 * IDirect3DViewport2 implementation structure
 */
struct IDirect3DViewport2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3DViewport2)* lpvtbl;
    DWORD                            ref;
    /* IDirect3DViewport2 fields */
    union {
        IDirect3DImpl*        d3d1;
        IDirect3D2Impl*       d3d2;
    } d3d;
    /* If this viewport is active for one device, put the device here */
    union {
        IDirect3DDeviceImpl*  active_device1;
        IDirect3DDevice2Impl* active_device2;
    } device;
    int                       use_d3d2;

    union {
        D3DVIEWPORT           vp1;
        D3DVIEWPORT2          vp2;
    } viewport;
    int                       use_vp2;

  /* Activation function */
  void (*activate)(IDirect3DViewport2Impl*);
  
  /* Field used to chain viewports together */
  IDirect3DViewport2Impl*     next;

  /* Lights list */
  IDirect3DLightImpl*         lights;

  /* OpenGL code */
#ifdef HAVE_MESAGL
  GLenum                      nextlight;
#endif
};

/*****************************************************************************
 * IDirect3DExecuteBuffer implementation structure
 */
struct IDirect3DExecuteBufferImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3DExecuteBuffer)* lpvtbl;
    DWORD                                ref;
    /* IDirect3DExecuteBuffer fields */
    IDirect3DDeviceImpl* d3ddev;

    D3DEXECUTEBUFFERDESC desc;
    D3DEXECUTEDATA data;

    /* This buffer will store the transformed vertices */
    void* vertex_data;
    D3DVERTEXTYPE vertex_type;

    /* This flags is set to TRUE if we allocated ourselves the
       data buffer */
    BOOL need_free;

    void (*execute)(IDirect3DExecuteBuffer* this,
                    IDirect3DDevice* dev,
                    IDirect3DViewport2* vp);
};

/*****************************************************************************
 * IDirect3DDevice implementation structure
 */
struct IDirect3DDeviceImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3DDevice)* lpvtbl;
    DWORD                         ref;
    /* IDirect3DDevice fields */
    IDirect3DImpl*          d3d;
    IDirectDrawSurfaceImpl* surface;

    IDirect3DViewport2Impl*  viewport_list;
    IDirect3DViewport2Impl*  current_viewport;

    void (*set_context)(IDirect3DDeviceImpl*);
};

/*****************************************************************************
 * IDirect3DDevice2 implementation structure
 */
struct IDirect3DDevice2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirect3DDevice2)* lpvtbl;
    DWORD                          ref;
    /* IDirect3DDevice fields */
    IDirect3D2Impl*         d3d;
    IDirectDrawSurfaceImpl* surface;

    IDirect3DViewport2Impl* viewport_list;
    IDirect3DViewport2Impl* current_viewport;

    void (*set_context)(IDirect3DDevice2Impl*);
};



#ifdef HAVE_MESAGL


/* Matrix copy WITH transposition */
#define conv_mat2(mat,gl_mat)			\
{						\
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44); \
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
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44); \
  memcpy(gl_mat, (mat), 16 * sizeof(float));      \
};

#define dump_mat(mat) \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
  TRACE(ddraw, "%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44);

typedef struct render_state {
  /* This is used for the device mode */
  GLenum src, dst;
  /* This is used for textures */
  GLenum mag, min;
} RenderState;

typedef struct OpenGL_IDirect3DDevice2 {
  IDirect3DDevice2Impl common;
  
  /* These are the OpenGL-specific variables */
  OSMesaContext ctx;
  unsigned char *buffer;
  
  /* The current render state */
  RenderState rs;

  /* The last type of vertex drawn */
  D3DVERTEXTYPE vt;
  
  float world_mat[16];
  float view_mat[16];
  float proj_mat[16];
} OpenGL_IDirect3DDevice2;

typedef struct OpenGL_IDirect3DDevice {
  IDirect3DDeviceImpl common;
  
  /* These are the OpenGL-specific variables */
  OSMesaContext ctx;
  unsigned char *buffer;
  
  /* The current render state */
  RenderState rs;
  
  D3DMATRIX *world_mat;
  D3DMATRIX *view_mat;
  D3DMATRIX *proj_mat;
} OpenGL_IDirect3DDevice;

#define _dump_colorvalue(s,v)                      \
  TRACE(ddraw, " " s " : %f %f %f %f\n",           \
	(v).r.r, (v).g.g, (v).b.b, (v).a.a);

/* Common functions defined in d3dcommon.c */
void set_render_state(D3DRENDERSTATETYPE dwRenderStateType,
		      DWORD dwRenderState, RenderState *rs) ;

#endif /* HAVE_MESAGL */

/* All non-static functions 'exported' by various sub-objects */
extern LPDIRECT3DTEXTURE2 d3dtexture2_create(IDirectDrawSurface4Impl* surf);
extern LPDIRECT3DTEXTURE d3dtexture_create(IDirectDrawSurface4Impl* surf);

extern LPDIRECT3DLIGHT d3dlight_create_dx3(IDirect3DImpl* d3d1);
extern LPDIRECT3DLIGHT d3dlight_create(IDirect3D2Impl* d3d2);

extern LPDIRECT3DEXECUTEBUFFER d3dexecutebuffer_create(IDirect3DDeviceImpl* d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc);

extern LPDIRECT3DMATERIAL d3dmaterial_create(IDirect3DImpl* d3d1);
extern LPDIRECT3DMATERIAL2 d3dmaterial2_create(IDirect3D2Impl* d3d2);

extern LPDIRECT3DVIEWPORT d3dviewport_create(IDirect3DImpl* d3d1);
extern LPDIRECT3DVIEWPORT2 d3dviewport2_create(IDirect3D2Impl* d3d2);

extern int is_OpenGL_dx3(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDeviceImpl** device);
extern int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) ;
extern int d3d_OpenGL(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) ;
extern int is_OpenGL(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDevice2Impl** device, IDirect3D2Impl* d3d);


#endif /* __GRAPHICS_WINE_D3D_PRIVATE_H */
