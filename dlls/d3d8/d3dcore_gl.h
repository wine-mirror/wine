/*
 * Direct3D gl definitions
 *
 * Copyright 2003 Raphael Junqueira
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

#ifndef __WINE_D3DCORE_GL_H
#define __WINE_D3DCORE_GL_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#define XMD_H /* This is to prevent the Xmd.h inclusion bug :-/ */
#include <GL/gl.h>
#include <GL/glx.h>
#ifdef HAVE_GL_GLEXT_H
# include <GL/glext.h>
#endif
#undef  XMD_H

#undef  APIENTRY
#define APIENTRY

/**********************************
 * OpenGL Extensions (EXT and ARB)
 *  defines and functions pointer
 */

/* GL_EXT_secondary_color */
#ifndef GL_EXT_secondary_color
#define GL_EXT_secondary_color 1
#define GL_COLOR_SUM_EXT                     0x8458
#define GL_CURRENT_SECONDARY_COLOR_EXT       0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE_EXT    0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE_EXT    0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT  0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER_EXT 0x845D
#define GL_SECONDARY_COLOR_ARRAY_EXT         0x845E
#endif
typedef void (APIENTRY * PGLFNGLSECONDARYCOLOR3FEXTPROC) (GLfloat red, GLfloat green, GLfloat blue);
typedef void (APIENTRY * PGLFNGLSECONDARYCOLOR3FVEXTPROC) (const GLfloat *v);
typedef void (APIENTRY * PGLFNGLSECONDARYCOLOR3UBEXTPROC) (GLubyte red, GLubyte green, GLubyte blue);
typedef void (APIENTRY * PGLFNGLSECONDARYCOLORPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
/* GL_EXT_paletted_texture */
#ifndef GL_EXT_paletted_texture
#define GL_EXT_paletted_texture 1
#define GL_COLOR_INDEX1_EXT               0x80E2
#define GL_COLOR_INDEX2_EXT               0x80E3
#define GL_COLOR_INDEX4_EXT               0x80E4
#define GL_COLOR_INDEX8_EXT               0x80E5
#define GL_COLOR_INDEX12_EXT              0x80E6
#define GL_COLOR_INDEX16_EXT              0x80E7
#define GL_TEXTURE_INDEX_SIZE_EXT         0x80ED
#endif
typedef void (APIENTRY * PGLFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
/* GL_EXT_point_parameters */
#ifndef GL_EXT_point_parameters
#define GL_EXT_point_parameters 1
#define GL_POINT_SIZE_MIN_EXT             0x8126
#define GL_POINT_SIZE_MAX_EXT             0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT  0x8128
#define GL_DISTANCE_ATTENUATION_EXT       0x8129
#endif
typedef void (APIENTRY * PGLFNGLPOINTPARAMETERFEXTPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PGLFNGLPOINTPARAMETERFVEXTPROC) (GLenum pname, const GLfloat *params);
#ifndef GL_EXT_texture_env_combine
#define GL_EXT_texture_env_combine 1
#define GL_COMBINE_EXT                    0x8570
#define GL_COMBINE_RGB_EXT                0x8571
#define GL_COMBINE_ALPHA_EXT              0x8572
#define GL_RGB_SCALE_EXT                  0x8573
#define GL_ADD_SIGNED_EXT                 0x8574
#define GL_INTERPOLATE_EXT                0x8575
#define GL_SUBTRACT_EXT                   0x84E7
#define GL_CONSTANT_EXT                   0x8576
#define GL_PRIMARY_COLOR_EXT              0x8577
#define GL_PREVIOUS_EXT                   0x8578
#define GL_SOURCE0_RGB_EXT                0x8580
#define GL_SOURCE1_RGB_EXT                0x8581
#define GL_SOURCE2_RGB_EXT                0x8582
#define GL_SOURCE3_RGB_EXT                0x8583
#define GL_SOURCE4_RGB_EXT                0x8584
#define GL_SOURCE5_RGB_EXT                0x8585
#define GL_SOURCE6_RGB_EXT                0x8586
#define GL_SOURCE7_RGB_EXT                0x8587
#define GL_SOURCE0_ALPHA_EXT              0x8588
#define GL_SOURCE1_ALPHA_EXT              0x8589
#define GL_SOURCE2_ALPHA_EXT              0x858A
#define GL_SOURCE3_ALPHA_EXT              0x858B
#define GL_SOURCE4_ALPHA_EXT              0x858C
#define GL_SOURCE5_ALPHA_EXT              0x858D
#define GL_SOURCE6_ALPHA_EXT              0x858E
#define GL_SOURCE7_ALPHA_EXT              0x858F
#define GL_OPERAND0_RGB_EXT               0x8590
#define GL_OPERAND1_RGB_EXT               0x8591
#define GL_OPERAND2_RGB_EXT               0x8592
#define GL_OPERAND3_RGB_EXT               0x8593
#define GL_OPERAND4_RGB_EXT               0x8594
#define GL_OPERAND5_RGB_EXT               0x8595
#define GL_OPERAND6_RGB_EXT               0x8596
#define GL_OPERAND7_RGB_EXT               0x8597
#define GL_OPERAND0_ALPHA_EXT             0x8598
#define GL_OPERAND1_ALPHA_EXT             0x8599
#define GL_OPERAND2_ALPHA_EXT             0x859A
#define GL_OPERAND3_ALPHA_EXT             0x859B
#define GL_OPERAND4_ALPHA_EXT             0x859C
#define GL_OPERAND5_ALPHA_EXT             0x859D
#define GL_OPERAND6_ALPHA_EXT             0x859E
#define GL_OPERAND7_ALPHA_EXT             0x859F
#endif
/* GL_EXT_texture_env_dot3 */
#ifndef GL_EXT_texture_env_dot3
#define GL_EXT_texture_env_dot3 1
#define GL_DOT3_RGB_EXT			  0x8740
#define GL_DOT3_RGBA_EXT		  0x8741
#endif


/*******
 * OpenGL Official Version 
 *  defines 
 */

/* GL_VERSION_1_3 */
#if !defined(GL_DOT3_RGBA)
# define GL_DOT3_RGBA                     0x8741
#endif
#if !defined(GL_SUBTRACT)
# define GL_SUBTRACT                      0x84E7
#endif


/*********************************
 * OpenGL GLX Extensions
 *  defines and functions pointer
 */



/*********************************
 * OpenGL GLX Official Version
 *  defines and functions pointer
 */

/* GLX_VERSION_1_3 */
typedef GLXFBConfig * (APIENTRY * PGLXFNGLXGETFBCONFIGSPROC) (Display *dpy, int screen, int *nelements);
typedef GLXFBConfig * (APIENTRY * PGLXFNGLXCHOOSEFBCONFIGPROC) (Display *dpy, int screen, const int *attrib_list, int *nelements);
typedef int           (APIENTRY * PGLXFNGLXGETFBCONFIGATTRIBPROC) (Display *dpy, GLXFBConfig config, int attribute, int *value);
typedef XVisualInfo * (APIENTRY * PGLXFNGLXGETVISUALFROMFBCONFIGPROC) (Display *dpy, GLXFBConfig config);
typedef GLXWindow     (APIENTRY * PGLXFNGLXCREATEWINDOWPROC) (Display *dpy, GLXFBConfig config, Window win, const int *attrib_list);
typedef void          (APIENTRY * PGLXFNGLXDESTROYWINDOWPROC) (Display *dpy, GLXWindow win);
typedef GLXPixmap     (APIENTRY * PGLXFNGLXCREATEPIXMAPPROC) (Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list);
typedef void          (APIENTRY * PGLXFNGLXDESTROYPIXMAPPROC) (Display *dpy, GLXPixmap pixmap);
typedef GLXPbuffer    (APIENTRY * PGLXFNGLXCREATEPBUFFERPROC) (Display *dpy, GLXFBConfig config, const int *attrib_list);
typedef void          (APIENTRY * PGLXFNGLXDESTROYPBUFFERPROC) (Display *dpy, GLXPbuffer pbuf);
typedef void          (APIENTRY * PGLXFNGLXQUERYDRAWABLEPROC) (Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
typedef GLXContext    (APIENTRY * PGLXFNGLXCREATENEWCONTEXTPROC) (Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
typedef Bool          (APIENTRY * PGLXFNGLXMAKECONTEXTCURRENTPROC) (Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
typedef GLXDrawable   (APIENTRY * PGLXFNGLXGETCURRENTREADDRAWABLEPROC) (void);
typedef Display *     (APIENTRY * PGLXFNGLXGETCURRENTDISPLAYPROC) (void);
typedef int           (APIENTRY * PGLXFNGLXQUERYCONTEXTPROC) (Display *dpy, GLXContext ctx, int attribute, int *value);
typedef void          (APIENTRY * PGLXFNGLXSELECTEVENTPROC) (Display *dpy, GLXDrawable draw, unsigned long event_mask);
typedef void          (APIENTRY * PGLXFNGLXGETSELECTEDEVENTPROC) (Display *dpy, GLXDrawable draw, unsigned long *event_mask);


/********************************************
 * OpenGL Supported Extensions (ARB and EXT)
 */

typedef enum _GL_SupportedExt {
  /* ARB */
  ARB_FRAGMENT_PROGRAM,
  ARB_MULTISAMPLE,
  ARB_MULTITEXTURE,
  ARB_POINT_PARAMETERS,
  ARB_TEXTURE_COMPRESSION,
  ARB_TEXTURE_CUBE_MAP,
  ARB_TEXTURE_ENV_COMBINE,
  ARB_TEXTURE_ENV_DOT3,
  ARB_VERTEX_PROGRAM,
  ARB_VERTEX_BLEND,
  /* EXT */
  EXT_FOG_COORD,
  EXT_PALETTED_TEXTURE,
  EXT_POINT_PARAMETERS,
  EXT_SECONDARY_COLOR,
  EXT_TEXTURE_COMPRESSION_S3TC,
  EXT_TEXTURE_FILTER_ANISOTROPIC,
  EXT_TEXTURE_LOD,
  EXT_TEXTURE_LOD_BIAS,
  EXT_VERTEX_WEIGHTING,
  /* NVIDIA */
  NV_FRAGMENT_PROGRAM,
  NV_VERTEX_PROGRAM,
  /* ATI */
  EXT_VERTEX_SHADER,

  OPENGL_SUPPORTED_EXT_END
} GL_SupportedExt;

typedef enum _GL_VSVersion {
  VS_VERSION_NOT_SUPPORTED = 0x0,
  VS_VERSION_10 = 0x10,
  VS_VERSION_11 = 0x11,
  VS_VERSION_20 = 0x20,
  VS_VERSION_30 = 0x30,
  /*Force 32-bits*/
  VS_VERSION_FORCE_DWORD = 0x7FFFFFFF
} GL_VSVersion;

typedef enum _GL_PSVersion {
  PS_VERSION_NOT_SUPPORTED = 0x0,
  PS_VERSION_10 = 0x10,
  PS_VERSION_11 = 0x11,
  PS_VERSION_12 = 0x12,
  PS_VERSION_13 = 0x13,
  PS_VERSION_14 = 0x14,
  PS_VERSION_20 = 0x20,
  PS_VERSION_30 = 0x30,
  /*Force 32-bits*/
  PS_VERSION_FORCE_DWORD = 0x7FFFFFFF
} GL_PSVersion;


#define GL_EXT_FUNCS_GEN \
    /** EXT Extensions **/ \
    /* GL_EXT_fog_coord */ \
    /* GL_EXT_paletted_texture */ \
    USE_GL_FUNC(PGLFNGLCOLORTABLEEXTPROC,            glColorTableEXT); \
    /* GL_EXT_point_parameters */ \
    USE_GL_FUNC(PGLFNGLPOINTPARAMETERFEXTPROC,       glPointParameterfEXT); \
    USE_GL_FUNC(PGLFNGLPOINTPARAMETERFVEXTPROC,      glPointParameterfvEXT); \
    /* GL_EXT_secondary_color */ \
    USE_GL_FUNC(PGLFNGLSECONDARYCOLOR3UBEXTPROC,     glSecondaryColor3ubEXT); \
    USE_GL_FUNC(PGLFNGLSECONDARYCOLOR3FEXTPROC,      glSecondaryColor3fEXT); \
    USE_GL_FUNC(PGLFNGLSECONDARYCOLOR3FVEXTPROC,     glSecondaryColor3fvEXT); \
    USE_GL_FUNC(PGLFNGLSECONDARYCOLORPOINTEREXTPROC, glSecondaryColorPointerEXT); \

#define GLX_EXT_FUNCS_GEN \
    /** GLX_VERSION_1_3 **/ \
    USE_GL_FUNC(PGLXFNGLXCREATEPBUFFERPROC,          glXCreatePbuffer); \
    USE_GL_FUNC(PGLXFNGLXDESTROYPBUFFERPROC,         glXDestroyPbuffer); \
    USE_GL_FUNC(PGLXFNGLXCREATEPIXMAPPROC,           glXCreatePixmap); \
    USE_GL_FUNC(PGLXFNGLXDESTROYPIXMAPPROC,          glXDestroyPixmap); \
    USE_GL_FUNC(PGLXFNGLXCREATENEWCONTEXTPROC,       glXCreateNewContext); \
    USE_GL_FUNC(PGLXFNGLXMAKECONTEXTCURRENTPROC,     glXMakeContextCurrent); \
    USE_GL_FUNC(PGLXFNGLXCHOOSEFBCONFIGPROC,         glXChooseFBConfig); \

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI

/* Routine common to the draw primitive and draw indexed primitive routines */
void drawPrimitive(LPDIRECT3DDEVICE8 iface,
                    int PrimitiveType,
                    long NumPrimitives,

                    /* for Indexed: */
                    long  StartVertexIndex,
                    long  StartIdx,
                    short idxBytes,
                    const void *idxData,
                    int   minIndex);


/*****************************************
 * Structures required to draw primitives 
 */

typedef struct Direct3DStridedData {
    BYTE     *lpData;        /* Pointer to start of data               */
    DWORD     dwStride;      /* Stride between occurances of this data */
    DWORD     dwType;        /* Type (as in D3DVSDT_TYPE)              */
} Direct3DStridedData;

typedef struct Direct3DVertexStridedData {
    union {
        struct {
             Direct3DStridedData  position;
             Direct3DStridedData  blendWeights;
             Direct3DStridedData  blendMatrixIndices;
             Direct3DStridedData  normal;
             Direct3DStridedData  pSize;
             Direct3DStridedData  diffuse;
             Direct3DStridedData  specular;
             Direct3DStridedData  texCoords[8];
        } DUMMYSTRUCTNAME;
        Direct3DStridedData input[16];  /* Indexed by constants in D3DVSDE_REGISTER */
    } DUMMYUNIONNAME;
} Direct3DVertexStridedData;

#define USE_GL_FUNC(type, pfn) type pfn;
typedef struct _GL_Info {
  /** 
   * CAPS Constants 
   */
  UINT   max_lights;
  UINT   max_textures;
  UINT   max_clipplanes;

  GL_PSVersion ps_arb_version;
  GL_PSVersion ps_nv_version;

  GL_VSVersion vs_arb_version;
  GL_VSVersion vs_nv_version;
  GL_VSVersion vs_ati_version;
  
  BOOL supported[30];

  /** OpenGL EXT and ARB functions ptr */
  GL_EXT_FUNCS_GEN;
  /** OpenGL GLX functions ptr */
  GLX_EXT_FUNCS_GEN;
  /**/
} GL_Info;
#undef USE_GL_FUNC


#endif  /* __WINE_D3DCORE_GL_H */
