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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wgl.h"
#include "wine/wgl_driver.h"

typedef struct {
  const char  *name;     /* name of the extension */
  const char  *extension; /* name of the GL/WGL extension */
  void  *func;     /* pointer to the Wine function for this extension */
} OpenGL_extension;

extern const OpenGL_extension extension_registry[] DECLSPEC_HIDDEN;
extern const int extension_registry_size DECLSPEC_HIDDEN;
extern struct opengl_funcs null_opengl_funcs DECLSPEC_HIDDEN;

static inline struct opengl_funcs *get_dc_funcs( HDC hdc )
{
    struct opengl_funcs *funcs = __wine_get_wgl_driver( hdc, WINE_WGL_DRIVER_VERSION );
    if (!funcs) SetLastError( ERROR_INVALID_HANDLE );
    else if (funcs == (void *)-1) funcs = &null_opengl_funcs;
    return funcs;
}

#endif /* __DLLS_OPENGL32_OPENGL_EXT_H */
