/* Typedefs for extensions loading

     Copyright (c) 2000 Lionel Ulmer
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
#ifndef __DLLS_OPENGL32_OPENGL_EXT_H
#define __DLLS_OPENGL32_OPENGL_EXT_H

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#define XMD_H /* This is to prevent the Xmd.h inclusion bug :-/ */
#define GL_GLEXT_PROTOTYPES
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

/* For compatibility with old Mesa headers */
#ifndef GLX_SAMPLE_BUFFERS_ARB
# define GLX_SAMPLE_BUFFERS_ARB             100000
#endif
#ifndef GLX_SAMPLES_ARB
# define GLX_SAMPLES_ARB                    100001
#endif
#ifndef GL_TEXTURE_CUBE_MAP
# define GL_TEXTURE_CUBE_MAP 0x8513
#endif

/* X11 locking */

extern void (*wine_tsx11_lock_ptr)(void);
extern void (*wine_tsx11_unlock_ptr)(void);

/* As GLX relies on X, this is needed */
void enter_gl(void);
#define ENTER_GL() enter_gl()
#define LEAVE_GL() wine_tsx11_unlock_ptr()


typedef struct {
  const char  *name;     /* name of the extension */
  const char  *glx_name; /* name used on Unix's libGL */
  void  *func;     /* pointer to the Wine function for this extension */
  void **func_ptr; /* where to store the value of glXGetProcAddressARB */
} OpenGL_extension;

extern const OpenGL_extension extension_registry[];
extern const int extension_registry_size;

const GLubyte* internal_glGetString(GLenum name);
void internal_glGetIntegerv(GLenum pname, GLint* params);

#endif /* __DLLS_OPENGL32_OPENGL_EXT_H */
