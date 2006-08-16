/* Support for window-specific OpenGL extensions.
 *
 * Copyright (c) 2004 Lionel Ulmer
 * Copyright (c) 2005 Raphael Junqueira
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

#ifndef __DLLS_OPENGL32_WGL_EXT_H
#define __DLLS_OPENGL32_WGL_EXT_H

#include "opengl_ext.h"

typedef void* (*glXGetProcAddressARB_t)(const GLubyte *);

/* Used to initialize the WGL extension string at DLL loading */
void wgl_ext_initialize_extensions(Display *display, int screen, glXGetProcAddressARB_t proc, const char* disabled_extensions);
void wgl_ext_finalize_extensions(void);

/* Used to convert between WGL and GLX pixel formats in wglMakeCurrent and some WGL extensions */
BOOL ConvertPixelFormatWGLtoGLX(Display *display, int iPixelFormat, int *fmt_index, int *fmt_count);

typedef struct {
    const char *func_name;
    void *func_address;
    const char *(*func_init)(void *(*p_glXGetProcAddressARB)(const GLubyte *), void *context);
    void *context;
} WGL_extension;

extern WGL_extension wgl_extension_registry[];
extern int wgl_extension_registry_size;

#endif /* __DLLS_OPENGL32_WGL_EXT_H */
