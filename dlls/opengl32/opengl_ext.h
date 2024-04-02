/* Typedefs for extensions loading

     Copyright (c) 2000 Lionel Ulmer
     Copyright     2019 Conor McCarthy for Codeweavers
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

#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/heap.h"
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

#ifdef __i386_on_x86_64__

static inline void *mirror_pointer_array(void **pointers, size_t count)
{
    void * HOSTPTR *ret = heap_alloc(count * sizeof(void * HOSTPTR));
    if (ret)
    {
      size_t i;
      for (i = 0; i < count; ++i)
          ret[i] = pointers[i];
    }
    return ret;
}

static inline WINEGLHOST(GLintptr) *mirror_intptr_array(const GLintptr *array, size_t count)
{
    WINEGLHOST(GLintptr) *ret = heap_alloc(count * sizeof(WINEGLHOST(GLintptr)));
    if (ret)
    {
      size_t i;
      for (i = 0; i < count; ++i)
          ret[i] = array[i];
    }
    return ret;
}

static inline GLchar *mirror_callback_string(const GLchar *message)
{
    return message ? heap_strdup(message) : NULL;
}

static inline void gl_temp_free(void *buffer)
{
    heap_free(buffer);
}

#else

static inline void *mirror_pointer_array(void **pointers, size_t count)
{
    (void)count;
    return pointers;
}

static inline GLintptr *mirror_intptr_array(const GLintptr *array, size_t count)
{
    (void)count;
    return (GLintptr*)array;
}

static inline GLchar *mirror_callback_string(const GLchar *message)
{
    return (GLchar*)message;
}

static inline void gl_temp_free(const void *buffer)
{
    (void)buffer;
}

#endif /* __i386_on_x86_64__ */

#endif /* __DLLS_OPENGL32_OPENGL_EXT_H */
