/* Wrapper for OpenGL includes...
 * Copyright 1998 - Lionel Ulmer
 *
 * This wrapper is needed because Mesa uses also the CALLBACK / WINAPI
 * constants.
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

#ifndef __WINE_WINE_GL_H
#define __WINE_WINE_GL_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#if defined(HAVE_OPENGL)

#include "ts_xlib.h"

/* As GLX relies on X, this is needed */
#define ENTER_GL() wine_tsx11_lock()
#define LEAVE_GL() wine_tsx11_unlock()

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#define XMD_H /* This is to prevent the Xmd.h inclusion bug to happen :-/ */
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

#endif /* HAVE_OPENGL */

#endif /* __WINE_WINE_GL_H */
