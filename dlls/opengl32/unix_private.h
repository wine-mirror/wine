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
#ifndef __WINE_OPENGL32_UNIX_PRIVATE_H
#define __WINE_OPENGL32_UNIX_PRIVATE_H

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "ntgdi.h"

#include "wine/opengl_driver.h"
#include "unix_thunks.h"

struct registry_entry
{
    const char *name;      /* name of the extension */
    const char *extension; /* name of the GL/WGL extension */
    size_t offset;         /* offset in the opengl_funcs table */
};

extern const struct registry_entry extension_registry[];
extern const int extension_registry_size;

extern struct opengl_funcs null_opengl_funcs;

static inline const struct opengl_funcs *get_dc_funcs( HDC hdc )
{
    const struct opengl_funcs *funcs = __wine_get_wgl_driver( hdc, WINE_OPENGL_DRIVER_VERSION, &null_opengl_funcs );
    if (!funcs) RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
    return funcs;
}

static inline void *copy_wow64_ptr32s( UINT_PTR address, ULONG count )
{
    ULONG *ptrs = (ULONG *)address;
    void **tmp;

    if (!ptrs || !(tmp = calloc( count, sizeof(*tmp) ))) return NULL;
    while (count--) tmp[count] = ULongToPtr(ptrs[count]);
    return tmp;
}

static inline TEB *get_teb64( ULONG teb32 )
{
    TEB32 *teb32_ptr = ULongToPtr( teb32 );
    return (TEB *)((char *)teb32_ptr + teb32_ptr->WowTebOffset);
}

extern pthread_mutex_t wgl_lock;

extern NTSTATUS process_attach( void *args );
extern NTSTATUS thread_attach( void *args );
extern NTSTATUS process_detach( void *args );
extern NTSTATUS get_pixel_formats( void *args );

extern BOOL wrap_wglCopyContext( TEB *teb, HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask );
extern HGLRC wrap_wglCreateContext( TEB *teb, HDC hDc );
extern BOOL wrap_wglDeleteContext( TEB *teb, HGLRC oldContext );
extern PROC wrap_wglGetProcAddress( TEB *teb, LPCSTR lpszProc );
extern BOOL wrap_wglMakeCurrent( TEB *teb, HDC hDc, HGLRC newContext );
extern void wrap_glFinish( TEB *teb );
extern void wrap_glFlush( TEB *teb );
extern void wrap_glClear( TEB *teb, GLbitfield mask );
extern void wrap_glDrawPixels( TEB *teb, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
extern void wrap_glReadPixels( TEB *teb, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels );
extern void wrap_glViewport( TEB *teb, GLint x, GLint y, GLsizei width, GLsizei height );
extern BOOL wrap_wglSwapBuffers( TEB *teb, HDC hdc );
extern BOOL wrap_wglShareLists( TEB *teb, HGLRC hrcSrvShare, HGLRC hrcSrvSource );
extern void wrap_glGetIntegerv( TEB *teb, GLenum pname, GLint *data );
extern const GLubyte * wrap_glGetString( TEB *teb, GLenum name );
extern void wrap_glDebugMessageCallback( TEB *teb, GLDEBUGPROC callback, const void *userParam );
extern void wrap_glDebugMessageCallbackAMD( TEB *teb, GLDEBUGPROCAMD callback, void *userParam );
extern void wrap_glDebugMessageCallbackARB( TEB *teb, GLDEBUGPROCARB callback, const void *userParam );
extern const GLubyte * wrap_glGetStringi( TEB *teb, GLenum name, GLuint index );
extern BOOL wrap_wglBindTexImageARB( TEB *teb, HPBUFFERARB hPbuffer, int iBuffer );
extern HGLRC wrap_wglCreateContextAttribsARB( TEB *teb, HDC hDC, HGLRC hShareContext, const int *attribList );
extern HPBUFFERARB wrap_wglCreatePbufferARB( TEB *teb, HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList );
extern BOOL wrap_wglDestroyPbufferARB( TEB *teb, HPBUFFERARB hPbuffer );
extern HDC wrap_wglGetPbufferDCARB( TEB *teb, HPBUFFERARB hPbuffer );
extern BOOL wrap_wglMakeContextCurrentARB( TEB *teb, HDC hDrawDC, HDC hReadDC, HGLRC hglrc );
extern BOOL wrap_wglQueryPbufferARB( TEB *teb, HPBUFFERARB hPbuffer, int iAttribute, int *piValue );
extern int wrap_wglReleasePbufferDCARB( TEB *teb, HPBUFFERARB hPbuffer, HDC hDC );
extern BOOL wrap_wglReleaseTexImageARB( TEB *teb, HPBUFFERARB hPbuffer, int iBuffer );
extern BOOL wrap_wglSetPbufferAttribARB( TEB *teb, HPBUFFERARB hPbuffer, const int *piAttribList );

extern void set_context_attribute( TEB *teb, GLenum name, const void *value, size_t size );

#endif /* __WINE_OPENGL32_UNIX_PRIVATE_H */
