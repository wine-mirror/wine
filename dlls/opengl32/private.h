/*
 *    Copyright (c) 2000 Lionel Ulmer
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
#ifndef __WINE_OPENGL32_PRIVATE_H
#define __WINE_OPENGL32_PRIVATE_H

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "wine/opengl_driver.h"
#include "thunks.h"

struct registry_entry
{
    const char *name;      /* name of the extension */
    void *func;
    UINT16 major;
    UINT16 minor;
    enum opengl_extension extensions[4];
};

extern int WINAPI wglDescribePixelFormat( HDC hdc, int ipfd, UINT cjpfd, PIXELFORMATDESCRIPTOR *ppfd );
extern BOOL get_pbuffer_from_handle( HPBUFFERARB handle, HPBUFFERARB *obj );
extern BOOL get_context_from_handle( HGLRC handle, HGLRC *obj );
extern BOOL get_sync_from_handle( GLsync handle, GLsync *obj );
extern void set_gl_error( GLenum error );
extern struct registry_entry *get_function_entry( const char *name );
extern BOOL get_integer( GLenum pname, GLuint index, GLint value, GLint *data );

enum object_type
{
    OBJ_TYPE_BUFFER,
    OBJ_TYPE_FRAMEBUFFER,
    OBJ_TYPE_COUNT,
};

static inline GLuint *memdup_objects( UINT n, const GLuint *handles, GLuint *buf, UINT max )
{
    GLuint *tmp = buf;
    if (n > max && !(tmp = malloc( n * sizeof(*handles) ))) return NULL;
    memcpy( tmp, handles, n * sizeof(*handles) );
    return tmp;
}

extern void put_context_objects( enum object_type type, UINT n, GLuint *handles );
extern BOOL alloc_context_objects( enum object_type type, UINT n, const GLuint *handles, BOOL extension );
extern GLuint *del_context_objects( enum object_type type, UINT n, GLuint *handles );
extern GLuint *map_context_objects( enum object_type type, UINT n, GLuint *handles );

#endif /* __WINE_OPENGL32_PRIVATE_H */
