/* Direct3D private include file
   (c) 1998 Lionel ULMER
   
   This files contains all the structure that are not exported
   through d3d.h and all common macros. */

#ifndef _WINE_D3D_PRIVATE_H
#define _WINE_D3D_PRIVATE_H

#ifdef HAVE_MESAGL

#include "d3d.h"
#include "wine_gl.h"

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
  IDirect3DDevice2 common;
  
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
  IDirect3DDevice common;
  
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
extern LPDIRECT3DTEXTURE2 d3dtexture2_create(LPDIRECTDRAWSURFACE3 surf) ;
extern LPDIRECT3DTEXTURE d3dtexture_create(LPDIRECTDRAWSURFACE3 surf) ;

extern LPDIRECT3DLIGHT d3dlight_create_dx3(LPDIRECT3D d3d) ;
extern LPDIRECT3DLIGHT d3dlight_create(LPDIRECT3D2 d3d) ;

extern LPDIRECT3DEXECUTEBUFFER d3dexecutebuffer_create(LPDIRECT3DDEVICE d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc) ;

extern LPDIRECT3DMATERIAL d3dmaterial_create(LPDIRECT3D d3d) ;
extern LPDIRECT3DMATERIAL2 d3dmaterial2_create(LPDIRECT3D2 d3d) ;

extern LPDIRECT3DVIEWPORT d3dviewport_create(LPDIRECT3D d3d) ;
extern LPDIRECT3DVIEWPORT2 d3dviewport2_create(LPDIRECT3D2 d3d) ;

extern int is_OpenGL_dx3(REFCLSID rguid, LPDIRECTDRAWSURFACE surface, LPDIRECT3DDEVICE *device) ;
extern int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) ;
extern int d3d_OpenGL(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) ;
extern int is_OpenGL(REFCLSID rguid, LPDIRECTDRAWSURFACE surface, LPDIRECT3DDEVICE2 *device, LPDIRECT3D2 d3d) ;

#endif /* _WINE_D3D_PRIVATE_H */
