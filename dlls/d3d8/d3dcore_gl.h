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

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI



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
typedef void (APIENTRY * PFNGLSECONDARYCOLOR3FEXTPROC) (GLfloat red, GLfloat green, GLfloat blue);
typedef void (APIENTRY * PFNGLSECONDARYCOLOR3FVEXTPROC) (const GLfloat *v);
typedef void (APIENTRY * PFNGLSECONDARYCOLOR3UBEXTPROC) (GLubyte red, GLubyte green, GLubyte blue);
typedef void (APIENTRY * PFNGLSECONDARYCOLORPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
#endif
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
typedef void (APIENTRY * PFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
#endif
/* GL_EXT_point_parameters */
#ifndef GL_EXT_point_parameters
#define GL_EXT_point_parameters 1
#define GL_POINT_SIZE_MIN_EXT             0x8126
#define GL_POINT_SIZE_MAX_EXT             0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT  0x8128
#define GL_DISTANCE_ATTENUATION_EXT       0x8129
typedef void (APIENTRY * PFNGLPOINTPARAMETERFEXTPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLPOINTPARAMETERFVEXTPROC) (GLenum pname, const GLfloat *params);
#endif


typedef enum _GL_SupportedExt {
  /* ARB */
  ARB_FRAGMENT_PROGRAM,
  ARB_MULTISAMPLE,
  ARB_MULTITEXTURE,
  ARB_POINT_PARAMETERS,
  ARB_TEXTURE_COMPRESSION,
  ARB_TEXTURE_CUBE_MAP,
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
    USE_GL_FUNC(PFNGLCOLORTABLEEXTPROC,            glColorTableEXT); \
    /* GL_EXT_point_parameters */ \
    USE_GL_FUNC(PFNGLPOINTPARAMETERFEXTPROC,       glPointParameterfEXT); \
    USE_GL_FUNC(PFNGLPOINTPARAMETERFVEXTPROC,      glPointParameterfvEXT); \
    /* GL_EXT_secondary_color */ \
    USE_GL_FUNC(PFNGLSECONDARYCOLOR3UBEXTPROC,     glSecondaryColor3ubEXT); \
    USE_GL_FUNC(PFNGLSECONDARYCOLOR3FEXTPROC,      glSecondaryColor3fEXT); \
    USE_GL_FUNC(PFNGLSECONDARYCOLOR3FVEXTPROC,     glSecondaryColor3fvEXT); \
    USE_GL_FUNC(PFNGLSECONDARYCOLORPOINTEREXTPROC, glSecondaryColorPointerEXT); \

#endif  /* __WINE_D3DCORE_GL_H */
