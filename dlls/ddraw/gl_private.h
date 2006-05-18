/* GL 'hack' private include file
 * Copyright (c) 2003 Lionel Ulmer / Mike McCormack
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

#ifndef __GRAPHICS_WINE_GL_PRIVATE_H
#define __GRAPHICS_WINE_GL_PRIVATE_H

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

#define GL_API_FUNCTION(f) extern typeof(f) * p##f;
#include "gl_api.h"
#undef GL_API_FUNCTION

/* This is also where I store our private extension defines...
   I know that Raphael won't like it, but well, I prefer doing that than battling 10 different headers :-)

   Note: this is perfectly 'legal' as the three variants of the enum have exactly the same value
*/
#define GL_MIRRORED_REPEAT_WINE                 0x8370
#define GL_TEXTURE_FILTER_CONTROL_WINE          0x8500
#define GL_TEXTURE_LOD_BIAS_WINE                0x8501
#define GL_TEXTURE0_WINE                        0x84C0
#define GL_TEXTURE1_WINE                        0x84C1
#define GL_TEXTURE2_WINE                        0x84C2
#define GL_TEXTURE3_WINE                        0x84C3
#define GL_TEXTURE4_WINE                        0x84C4
#define GL_TEXTURE5_WINE                        0x84C5
#define GL_TEXTURE6_WINE                        0x84C6
#define GL_TEXTURE7_WINE                        0x84C7
#define GL_MAX_TEXTURE_UNITS_WINE               0x84E2

#ifndef GLPRIVATE_NO_REDEFINE

#define glAlphaFunc pglAlphaFunc
#define glBegin pglBegin
#define glBindTexture pglBindTexture
#define glBlendFunc pglBlendFunc
#define glClear pglClear
#define glClearColor pglClearColor
#define glClearDepth pglClearDepth
#define glClearStencil pglClearStencil
#define glClipPlane pglClipPlane
#define glColor3f pglColor3f
#define glColor3ub pglColor3ub
#define glColor4ub pglColor4ub
#define glColorMask pglColorMask
#define glColorPointer pglColorPointer
#define glCopyPixels pglCopyPixels
#define glCopyTexSubImage2D pglCopyTexSubImage2D
#define glColorMaterial pglColorMaterial
#define glCullFace pglCullFace
#define glDeleteTextures pglDeleteTextures
#define glDepthFunc pglDepthFunc
#define glDepthMask pglDepthMask
#define glDepthRange pglDepthRange
#define glDisable pglDisable
#define glDisableClientState pglDisableClientState
#define glDrawArrays pglDrawArrays
#define glDrawBuffer pglDrawBuffer
#define glDrawElements pglDrawElements
#define glDrawPixels pglDrawPixels
#define glEnable pglEnable
#define glEnableClientState pglEnableClientState
#define glEnd pglEnd
#define glFlush pglFlush
#define glFogf pglFogf
#define glFogfv pglFogfv
#define glFogi pglFogi
#define glFrontFace pglFrontFace
#define glGenTextures pglGenTextures
#define glGetBooleanv pglGetBooleanv
#define glGetError pglGetError
#define glGetFloatv pglGetFloatv
#define glGetIntegerv pglGetIntegerv
#define glGetString pglGetString
#define glGetTexEnviv pglGetTexEnviv
#define glGetTexParameteriv pglGetTexParameteriv
#define glHint pglHint
#define glLightModelfv pglLightModelfv
#define glLightModeli pglLightModeli
#define glLightfv pglLightfv
#define glLoadIdentity pglLoadIdentity
#define glLoadMatrixf pglLoadMatrixf
#define glMaterialf pglMaterialf
#define glMaterialfv pglMaterialfv
#define glMatrixMode pglMatrixMode
#define glMultMatrixf pglMultMatrixf
#define glNormal3f pglNormal3f
#define glNormal3fv pglNormal3fv
#define glNormalPointer pglNormalPointer
#define glOrtho pglOrtho
#define glPixelStorei pglPixelStorei
#define glPolygonMode pglPolygonMode
#define glPolygonOffset pglPolygonOffset
#define glPopMatrix pglPopMatrix
#define glPushMatrix pglPushMatrix
#define glRasterPos2i pglRasterPos2i
#define glRasterPos3d pglRasterPos3d
#define glReadBuffer pglReadBuffer
#define glReadPixels pglReadPixels
#define glScissor pglScissor
#define glShadeModel pglShadeModel
#define glStencilFunc pglStencilFunc
#define glStencilMask pglStencilMask
#define glStencilOp pglStencilOp
#define glTexCoord1fv pglTexCoord1fv
#define glTexCoord2f pglTexCoord2f
#define glTexCoord2fv pglTexCoord2fv
#define glTexCoord3fv pglTexCoord3fv
#define glTexCoord4fv pglTexCoord4fv
#define glTexCoordPointer pglTexCoordPointer
#define glTexEnvf pglTexEnvf
#define glTexEnvfv pglTexEnvfv
#define glTexEnvi pglTexEnvi
#define glTexImage2D pglTexImage2D
#define glTexParameteri pglTexParameteri
#define glTexParameterfv pglTexParameterfv
#define glTexSubImage2D pglTexSubImage2D
#define glTranslatef pglTranslatef
#define glVertex3d pglVertex3d
#define glVertex3f pglVertex3f
#define glVertex3fv pglVertex3fv
#define glVertex4f pglVertex4f
#define glVertexPointer pglVertexPointer
#define glViewport pglViewport
#define glXCreateContext pglXCreateContext
#define glXDestroyContext pglXDestroyContext
#define glXMakeCurrent pglXMakeCurrent
#define glXQueryExtensionsString pglXQueryExtensionsString
#define glXSwapBuffers pglXSwapBuffers

#endif /* GLPRIVATE_NO_REDEFINE */

#endif /* HAVE_OPENGL */

#endif /* __GRAPHICS_WINE_GL_PRIVATE_H */
