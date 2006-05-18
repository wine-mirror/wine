/* GL API list
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

/* Note : this file is NOT protected against double-inclusion for pretty good
          reasons :-) */

#ifndef GL_API_FUNCTION
#error "This file should be included with GL_API_FUNCTION defined !"
#endif

GL_API_FUNCTION(glAlphaFunc)
GL_API_FUNCTION(glBegin)
GL_API_FUNCTION(glBindTexture)
GL_API_FUNCTION(glBlendFunc)
GL_API_FUNCTION(glClear)
GL_API_FUNCTION(glClearColor)
GL_API_FUNCTION(glClearDepth)
GL_API_FUNCTION(glClearStencil)
GL_API_FUNCTION(glClipPlane)
GL_API_FUNCTION(glColor3f)
GL_API_FUNCTION(glColor3ub)
GL_API_FUNCTION(glColor4ub)
GL_API_FUNCTION(glColorMask)
GL_API_FUNCTION(glColorMaterial)
GL_API_FUNCTION(glColorPointer)
GL_API_FUNCTION(glCopyPixels)
GL_API_FUNCTION(glCopyTexSubImage2D)
GL_API_FUNCTION(glCullFace)
GL_API_FUNCTION(glDeleteTextures)
GL_API_FUNCTION(glDepthFunc)
GL_API_FUNCTION(glDepthMask)
GL_API_FUNCTION(glDepthRange)
GL_API_FUNCTION(glDisable)
GL_API_FUNCTION(glDisableClientState)
GL_API_FUNCTION(glDrawArrays)
GL_API_FUNCTION(glDrawBuffer)
GL_API_FUNCTION(glDrawElements)
GL_API_FUNCTION(glDrawPixels)
GL_API_FUNCTION(glEnable)
GL_API_FUNCTION(glEnableClientState)
GL_API_FUNCTION(glEnd)
GL_API_FUNCTION(glFlush)
GL_API_FUNCTION(glFogf)
GL_API_FUNCTION(glFogfv)
GL_API_FUNCTION(glFogi)
GL_API_FUNCTION(glFrontFace)
GL_API_FUNCTION(glGenTextures)
GL_API_FUNCTION(glGetBooleanv)
GL_API_FUNCTION(glGetError)
GL_API_FUNCTION(glGetFloatv)
GL_API_FUNCTION(glGetIntegerv)
GL_API_FUNCTION(glGetString)
GL_API_FUNCTION(glGetTexEnviv)
GL_API_FUNCTION(glGetTexParameteriv)
GL_API_FUNCTION(glHint)
GL_API_FUNCTION(glLightModelfv)
GL_API_FUNCTION(glLightModeli)
GL_API_FUNCTION(glLightfv)
GL_API_FUNCTION(glLoadIdentity)
GL_API_FUNCTION(glLoadMatrixf)
GL_API_FUNCTION(glMaterialf)
GL_API_FUNCTION(glMaterialfv)
GL_API_FUNCTION(glMatrixMode)
GL_API_FUNCTION(glMultMatrixf)
GL_API_FUNCTION(glNormal3f)
GL_API_FUNCTION(glNormal3fv)
GL_API_FUNCTION(glNormalPointer)
GL_API_FUNCTION(glOrtho)
GL_API_FUNCTION(glPixelStorei)
GL_API_FUNCTION(glPolygonMode)
GL_API_FUNCTION(glPolygonOffset)
GL_API_FUNCTION(glPopMatrix)
GL_API_FUNCTION(glPushMatrix)
GL_API_FUNCTION(glRasterPos2i)
GL_API_FUNCTION(glRasterPos3d)
GL_API_FUNCTION(glReadBuffer)
GL_API_FUNCTION(glReadPixels)
GL_API_FUNCTION(glScissor)
GL_API_FUNCTION(glShadeModel)
GL_API_FUNCTION(glStencilFunc)
GL_API_FUNCTION(glStencilMask)
GL_API_FUNCTION(glStencilOp)
GL_API_FUNCTION(glTexCoord1fv)
GL_API_FUNCTION(glTexCoord2f)
GL_API_FUNCTION(glTexCoord2fv)
GL_API_FUNCTION(glTexCoord3fv)
GL_API_FUNCTION(glTexCoord4fv)
GL_API_FUNCTION(glTexCoordPointer)
GL_API_FUNCTION(glTexEnvf)
GL_API_FUNCTION(glTexEnvfv)
GL_API_FUNCTION(glTexEnvi)
GL_API_FUNCTION(glTexImage2D)
GL_API_FUNCTION(glTexParameteri)
GL_API_FUNCTION(glTexParameterfv)
GL_API_FUNCTION(glTexSubImage2D)
GL_API_FUNCTION(glTranslatef)
GL_API_FUNCTION(glVertex3d)
GL_API_FUNCTION(glVertex3f)
GL_API_FUNCTION(glVertex3fv)
GL_API_FUNCTION(glVertex4f)
GL_API_FUNCTION(glVertexPointer)
GL_API_FUNCTION(glViewport)
GL_API_FUNCTION(glXCreateContext)
GL_API_FUNCTION(glXDestroyContext)
GL_API_FUNCTION(glXMakeCurrent)
GL_API_FUNCTION(glXQueryExtensionsString)
GL_API_FUNCTION(glXSwapBuffers)
