/* Support for window-specific OpenGL extensions.
 *
 * Copyright (c) 2004 Lionel Ulmer
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

#ifndef __DLLS_OPENGL32_WGL_EXT_H
#define __DLLS_OPENGL32_WGL_EXT_H

#include "opengl_ext.h"

/* Used to initialize the WGL extension string at DLL loading */
void wgl_ext_initialize_extensions(Display *display, int screen);
void wgl_ext_finalize_extensions(void);

typedef struct {
    const char *func_name;
    void *func_address;
    const char *(*func_init)(void *(*p_glXGetProcAddressARB)(const GLubyte *), void *context);
    void *context;
} WGL_extension;

extern WGL_extension wgl_extension_registry[];
extern int wgl_extension_registry_size;

#endif /* __DLLS_OPENGL32_WGL_EXT_H */
