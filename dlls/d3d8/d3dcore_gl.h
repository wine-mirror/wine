/*
 * Direct3D gl definitions
 *
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

/* GL_ARB_vertex_blend */
#ifndef GL_ARB_vertex_blend
#define GL_ARB_vertex_blend 1
#define GL_MAX_VERTEX_UNITS_ARB           0x86A4
#define GL_ACTIVE_VERTEX_UNITS_ARB        0x86A5
#define GL_WEIGHT_SUM_UNITY_ARB           0x86A6
#define GL_VERTEX_BLEND_ARB               0x86A7
#define GL_CURRENT_WEIGHT_ARB             0x86A8
#define GL_WEIGHT_ARRAY_TYPE_ARB          0x86A9
#define GL_WEIGHT_ARRAY_STRIDE_ARB        0x86AA
#define GL_WEIGHT_ARRAY_SIZE_ARB          0x86AB
#define GL_WEIGHT_ARRAY_POINTER_ARB       0x86AC
#define GL_WEIGHT_ARRAY_ARB               0x86AD
#define GL_MODELVIEW0_ARB                 0x1700
#define GL_MODELVIEW1_ARB                 0x850A
#define GL_MODELVIEW2_ARB                 0x8722
#define GL_MODELVIEW3_ARB                 0x8723
#define GL_MODELVIEW4_ARB                 0x8724
#define GL_MODELVIEW5_ARB                 0x8725
#define GL_MODELVIEW6_ARB                 0x8726
#define GL_MODELVIEW7_ARB                 0x8727
#define GL_MODELVIEW8_ARB                 0x8728
#define GL_MODELVIEW9_ARB                 0x8729
#define GL_MODELVIEW10_ARB                0x872A
#define GL_MODELVIEW11_ARB                0x872B
#define GL_MODELVIEW12_ARB                0x872C
#define GL_MODELVIEW13_ARB                0x872D
#define GL_MODELVIEW14_ARB                0x872E
#define GL_MODELVIEW15_ARB                0x872F
#define GL_MODELVIEW16_ARB                0x8730
#define GL_MODELVIEW17_ARB                0x8731
#define GL_MODELVIEW18_ARB                0x8732
#define GL_MODELVIEW19_ARB                0x8733
#define GL_MODELVIEW20_ARB                0x8734
#define GL_MODELVIEW21_ARB                0x8735
#define GL_MODELVIEW22_ARB                0x8736
#define GL_MODELVIEW23_ARB                0x8737
#define GL_MODELVIEW24_ARB                0x8738
#define GL_MODELVIEW25_ARB                0x8739
#define GL_MODELVIEW26_ARB                0x873A
#define GL_MODELVIEW27_ARB                0x873B
#define GL_MODELVIEW28_ARB                0x873C
#define GL_MODELVIEW29_ARB                0x873D
#define GL_MODELVIEW30_ARB                0x873E
#define GL_MODELVIEW31_ARB                0x873F
#endif
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
/* GL_EXT_texture_env_combine */
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
/* GL_EXT_texture_lod_bias */
#ifndef GL_EXT_texture_lod_bias
#define GL_EXT_texture_lod_bias 1
#define GL_MAX_TEXTURE_LOD_BIAS_EXT       0x84FD
#define GL_TEXTURE_FILTER_CONTROL_EXT     0x8500
#define GL_TEXTURE_LOD_BIAS_EXT           0x8501
#endif
/* GL_ARB_texture_border_clamp */
#ifndef GL_ARB_texture_border_clamp
#define GL_ARB_texture_border_clamp 1
#define GL_CLAMP_TO_BORDER_ARB            0x812D
#endif
/* GL_ARB_texture_mirrored_repeat (full support GL1.4) */
#ifndef GL_ARB_texture_mirrored_repeat
#define GL_ARB_texture_mirrored_repeat 1
#define GL_MIRRORED_REPEAT_ARB            0x8370
#endif
/* GL_ATI_texture_mirror_once */
#ifndef GL_ATI_texture_mirror_once
#define GL_ATI_texture_mirror_once 1
#define GL_MIRROR_CLAMP_ATI               0x8742
#define GL_MIRROR_CLAMP_TO_EDGE_ATI       0x8743
#endif
/* GL_ARB_texture_env_dot3 */
#ifndef GL_ARB_texture_env_dot3
#define GL_ARB_texture_env_dot3 1
#define GL_DOT3_RGB_ARB                   0x86AE
#define GL_DOT3_RGBA_ARB                  0x86AF
#endif
/* GL_EXT_texture_env_dot3 */
#ifndef GL_EXT_texture_env_dot3
#define GL_EXT_texture_env_dot3 1
#define GL_DOT3_RGB_EXT                   0x8740
#define GL_DOT3_RGBA_EXT                  0x8741
#endif
/* GL_ARB_vertex_program */
#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
#define GL_VERTEX_PROGRAM_ARB             0x8620
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB  0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB    0x8643
#define GL_COLOR_SUM_ARB                  0x8458
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB   0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB      0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
#define GL_PROGRAM_LENGTH_ARB             0x8627
#define GL_PROGRAM_FORMAT_ARB             0x8876
#define GL_PROGRAM_BINDING_ARB            0x8677
#define GL_PROGRAM_INSTRUCTIONS_ARB       0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB   0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB        0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB    0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#define GL_PROGRAM_PARAMETERS_ARB         0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB     0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB  0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ATTRIBS_ARB            0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB        0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB     0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB  0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_PROGRAM_STRING_ARB             0x8628
#define GL_PROGRAM_ERROR_POSITION_ARB     0x864B
#define GL_CURRENT_MATRIX_ARB             0x8641
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB   0x88B7
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB 0x8640
#define GL_MAX_VERTEX_ATTRIBS_ARB         0x8869
#define GL_MAX_PROGRAM_MATRICES_ARB       0x862F
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#define GL_PROGRAM_ERROR_STRING_ARB       0x8874
#define GL_MATRIX0_ARB                    0x88C0
#define GL_MATRIX1_ARB                    0x88C1
#define GL_MATRIX2_ARB                    0x88C2
#define GL_MATRIX3_ARB                    0x88C3
#define GL_MATRIX4_ARB                    0x88C4
#define GL_MATRIX5_ARB                    0x88C5
#define GL_MATRIX6_ARB                    0x88C6
#define GL_MATRIX7_ARB                    0x88C7
#define GL_MATRIX8_ARB                    0x88C8
#define GL_MATRIX9_ARB                    0x88C9
#define GL_MATRIX10_ARB                   0x88CA
#define GL_MATRIX11_ARB                   0x88CB
#define GL_MATRIX12_ARB                   0x88CC
#define GL_MATRIX13_ARB                   0x88CD
#define GL_MATRIX14_ARB                   0x88CE
#define GL_MATRIX15_ARB                   0x88CF
#define GL_MATRIX16_ARB                   0x88D0
#define GL_MATRIX17_ARB                   0x88D1
#define GL_MATRIX18_ARB                   0x88D2
#define GL_MATRIX19_ARB                   0x88D3
#define GL_MATRIX20_ARB                   0x88D4
#define GL_MATRIX21_ARB                   0x88D5
#define GL_MATRIX22_ARB                   0x88D6
#define GL_MATRIX23_ARB                   0x88D7
#define GL_MATRIX24_ARB                   0x88D8
#define GL_MATRIX25_ARB                   0x88D9
#define GL_MATRIX26_ARB                   0x88DA
#define GL_MATRIX27_ARB                   0x88DB
#define GL_MATRIX28_ARB                   0x88DC
#define GL_MATRIX29_ARB                   0x88DD
#define GL_MATRIX30_ARB                   0x88DE
#define GL_MATRIX31_ARB                   0x88DF
#endif
typedef void (APIENTRY * PGLFNVERTEXATTRIB1DARBPROC) (GLuint index, GLdouble x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1FARBPROC) (GLuint index, GLfloat x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1SARBPROC) (GLuint index, GLshort x);
typedef void (APIENTRY * PGLFNVERTEXATTRIB1SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2DARBPROC) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2FARBPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2SARBPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * PGLFNVERTEXATTRIB2SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PGLFNVERTEXATTRIB3SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NBVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NIVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NSVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUBARBPROC) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4NUSVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4BVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4IVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4UBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4UIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIB4USVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PGLFNVERTEXATTRIBPOINTERARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PGLFNENABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRY * PGLFNDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRY * PGLFNPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
typedef void (APIENTRY * PGLFNBINDPROGRAMARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRY * PGLFNDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * PGLFNGENPROGRAMSARBPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * PGLFNPROGRAMENVPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PGLFNPROGRAMENVPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * PGLFNPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PGLFNPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * PGLFNPROGRAMLOCALPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PGLFNPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * PGLFNPROGRAMLOCALPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PGLFNPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * PGLFNGETPROGRAMENVPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * PGLFNGETPROGRAMENVPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * PGLFNGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * PGLFNGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * PGLFNGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETPROGRAMSTRINGARBPROC) (GLenum target, GLenum pname, GLvoid *string);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBDVARBPROC) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBFVARBPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBIVARBPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRY * PGLFNGETVERTEXATTRIBPOINTERVARBPROC) (GLuint index, GLenum pname, GLvoid* *pointer);
typedef GLboolean (APIENTRY * PGLFNISPROGRAMARBPROC) (GLuint program);

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
  ARB_TEXTURE_ENV_ADD,
  ARB_TEXTURE_ENV_COMBINE,
  ARB_TEXTURE_ENV_DOT3,
  ARB_TEXTURE_BORDER_CLAMP,
  ARB_TEXTURE_MIRRORED_REPEAT,
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
  EXT_TEXTURE_ENV_ADD,
  EXT_TEXTURE_ENV_COMBINE,
  EXT_TEXTURE_ENV_DOT3,
  EXT_VERTEX_WEIGHTING,
  /* NVIDIA */
  NV_TEXTURE_ENV_COMBINE4,
  NV_FRAGMENT_PROGRAM,
  NV_VERTEX_PROGRAM,
  /* ATI */
  ATI_TEXTURE_ENV_COMBINE3,
  ATI_TEXTURE_MIRROR_ONCE,
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
    /* GL_EXT_secondary_color */ \
    USE_GL_FUNC(PGLFNGENPROGRAMSARBPROC,              glGenProgramsARB); \
    USE_GL_FUNC(PGLFNBINDPROGRAMARBPROC,              glBindProgramARB); \
    USE_GL_FUNC(PGLFNPROGRAMSTRINGARBPROC,            glProgramStringARB); \
    USE_GL_FUNC(PGLFNDELETEPROGRAMSARBPROC,           glDeleteProgramsARB); \
    USE_GL_FUNC(PGLFNPROGRAMENVPARAMETER4FVARBPROC,   glProgramEnvParameter4fvARB); \
    USE_GL_FUNC(PGLFNVERTEXATTRIBPOINTERARBPROC,      glVertexAttribPointerARB); \
    USE_GL_FUNC(PGLFNENABLEVERTEXATTRIBARRAYARBPROC,  glEnableVertexAttribArrayARB); \
    USE_GL_FUNC(PGLFNDISABLEVERTEXATTRIBARRAYARBPROC, glDisableVertexAttribArrayARB); \

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
        } s;
        Direct3DStridedData input[16];  /* Indexed by constants in D3DVSDE_REGISTER */
    } u;
} Direct3DVertexStridedData;

#define USE_GL_FUNC(type, pfn) type pfn;
typedef struct _GL_Info {
  unsigned bIsFilled;
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
  
  BOOL supported[40];

  /** OpenGL EXT and ARB functions ptr */
  GL_EXT_FUNCS_GEN;
  /** OpenGL GLX functions ptr */
  GLX_EXT_FUNCS_GEN;
  /**/
} GL_Info;
#undef USE_GL_FUNC


#endif  /* __WINE_D3DCORE_GL_H */
