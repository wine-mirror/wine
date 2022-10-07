/* Window-specific OpenGL functions implementation.
 *
 * Copyright (c) 1999 Lionel Ulmer
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntuser.h"

#include "opengl_ext.h"

#include "unixlib.h"

#include "wine/debug.h"

struct wgl_handle wgl_handles[MAX_WGL_HANDLES];
static struct wgl_handle *next_free;
static unsigned int handle_count;

static CRITICAL_SECTION wgl_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &wgl_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wgl_section") }
};
static CRITICAL_SECTION wgl_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static inline HANDLE next_handle( struct wgl_handle *ptr, enum wgl_handle_type type )
{
    WORD generation = HIWORD( ptr->handle ) + 1;
    if (!generation) generation++;
    ptr->handle = MAKELONG( ptr - wgl_handles, generation ) | type;
    return ULongToHandle( ptr->handle );
}

static struct wgl_handle *get_handle_ptr( HANDLE handle, enum wgl_handle_type type )
{
    unsigned int index = LOWORD( handle ) & ~HANDLE_TYPE_MASK;

    EnterCriticalSection( &wgl_section );
    if (index < handle_count && ULongToHandle(wgl_handles[index].handle) == handle)
        return &wgl_handles[index];

    LeaveCriticalSection( &wgl_section );
    SetLastError( ERROR_INVALID_HANDLE );
    return NULL;
}

static void release_handle_ptr( struct wgl_handle *ptr )
{
    if (ptr) LeaveCriticalSection( &wgl_section );
}

static HANDLE alloc_handle( enum wgl_handle_type type, struct opengl_funcs *funcs, void *user_ptr )
{
    HANDLE handle = 0;
    struct wgl_handle *ptr = NULL;

    EnterCriticalSection( &wgl_section );
    if ((ptr = next_free))
        next_free = next_free->u.next;
    else if (handle_count < MAX_WGL_HANDLES)
        ptr = &wgl_handles[handle_count++];

    if (ptr)
    {
        ptr->funcs = funcs;
        ptr->u.context = user_ptr;
        handle = next_handle( ptr, type );
    }
    else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    LeaveCriticalSection( &wgl_section );
    return handle;
}

static void free_handle_ptr( struct wgl_handle *ptr )
{
    ptr->handle |= 0xffff;
    ptr->u.next = next_free;
    ptr->funcs = NULL;
    next_free = ptr;
    LeaveCriticalSection( &wgl_section );
}

static BOOL wrap_wglCopyContext( HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask )
{
    struct wgl_handle *src, *dst;
    BOOL ret = FALSE;

    if (!(src = get_handle_ptr( hglrcSrc, HANDLE_CONTEXT ))) return FALSE;
    if ((dst = get_handle_ptr( hglrcDst, HANDLE_CONTEXT )))
    {
        if (src->funcs != dst->funcs) SetLastError( ERROR_INVALID_HANDLE );
        else ret = src->funcs->wgl.p_wglCopyContext( src->u.context->drv_ctx, dst->u.context->drv_ctx, mask );
    }
    release_handle_ptr( dst );
    release_handle_ptr( src );
    return ret;
}

static HGLRC wrap_wglCreateContext( HDC hdc )
{
    HGLRC ret = 0;
    struct wgl_context *drv_ctx;
    struct opengl_context *context;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs) return 0;
    if (!(drv_ctx = funcs->wgl.p_wglCreateContext( hdc ))) return 0;
    if ((context = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*context) )))
    {
        context->drv_ctx = drv_ctx;
        if (!(ret = alloc_handle( HANDLE_CONTEXT, funcs, context )))
            HeapFree( GetProcessHeap(), 0, context );
    }
    if (!ret) funcs->wgl.p_wglDeleteContext( drv_ctx );
    return ret;
}

static BOOL wrap_wglDeleteContext( HGLRC hglrc )
{
    struct wgl_handle *ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT );

    if (!ptr) return FALSE;

    if (ptr->u.context->tid && ptr->u.context->tid != GetCurrentThreadId())
    {
        SetLastError( ERROR_BUSY );
        release_handle_ptr( ptr );
        return FALSE;
    }
    if (hglrc == NtCurrentTeb()->glCurrentRC) wglMakeCurrent( 0, 0 );
    ptr->funcs->wgl.p_wglDeleteContext( ptr->u.context->drv_ctx );
    HeapFree( GetProcessHeap(), 0, ptr->u.context->disabled_exts );
    HeapFree( GetProcessHeap(), 0, ptr->u.context->extensions );
    HeapFree( GetProcessHeap(), 0, ptr->u.context );
    free_handle_ptr( ptr );
    return TRUE;
}

static BOOL wrap_wglMakeCurrent( HDC hdc, HGLRC hglrc )
{
    BOOL ret = TRUE;
    struct wgl_handle *ptr, *prev = get_current_context_ptr();

    if (hglrc)
    {
        if (!(ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT ))) return FALSE;
        if (!ptr->u.context->tid || ptr->u.context->tid == GetCurrentThreadId())
        {
            ret = ptr->funcs->wgl.p_wglMakeCurrent( hdc, ptr->u.context->drv_ctx );
            if (ret)
            {
                if (prev) prev->u.context->tid = 0;
                ptr->u.context->tid = GetCurrentThreadId();
                ptr->u.context->draw_dc = hdc;
                ptr->u.context->read_dc = hdc;
                NtCurrentTeb()->glCurrentRC = hglrc;
                NtCurrentTeb()->glTable = ptr->funcs;
            }
        }
        else
        {
            SetLastError( ERROR_BUSY );
            ret = FALSE;
        }
        release_handle_ptr( ptr );
    }
    else if (prev)
    {
        if (!prev->funcs->wgl.p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->u.context->tid = 0;
        NtCurrentTeb()->glCurrentRC = 0;
        NtCurrentTeb()->glTable = &null_opengl_funcs;
    }
    else if (!hdc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        ret = FALSE;
    }
    return ret;
}

static BOOL wrap_wglShareLists( HGLRC hglrcSrc, HGLRC hglrcDst )
{
    BOOL ret = FALSE;
    struct wgl_handle *src, *dst;

    if (!(src = get_handle_ptr( hglrcSrc, HANDLE_CONTEXT ))) return FALSE;
    if ((dst = get_handle_ptr( hglrcDst, HANDLE_CONTEXT )))
    {
        if (src->funcs != dst->funcs) SetLastError( ERROR_INVALID_HANDLE );
        else ret = src->funcs->wgl.p_wglShareLists( src->u.context->drv_ctx, dst->u.context->drv_ctx );
    }
    release_handle_ptr( dst );
    release_handle_ptr( src );
    return ret;
}

static BOOL wrap_wglBindTexImageARB( HPBUFFERARB handle, int buffer )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglBindTexImageARB( ptr->u.pbuffer, buffer );
    release_handle_ptr( ptr );
    return ret;
}

static HGLRC wrap_wglCreateContextAttribsARB( HDC hdc, HGLRC share, const int *attribs )
{
    HGLRC ret = 0;
    struct wgl_context *drv_ctx;
    struct wgl_handle *share_ptr = NULL;
    struct opengl_context *context;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs)
    {
        SetLastError( ERROR_DC_NOT_FOUND );
        return 0;
    }
    if (!funcs->ext.p_wglCreateContextAttribsARB) return 0;
    if (share && !(share_ptr = get_handle_ptr( share, HANDLE_CONTEXT )))
    {
        SetLastError( ERROR_INVALID_OPERATION );
        return 0;
    }
    if ((drv_ctx = funcs->ext.p_wglCreateContextAttribsARB( hdc, share_ptr ? share_ptr->u.context->drv_ctx : NULL, attribs )))
    {
        if ((context = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*context) )))
        {
            enum wgl_handle_type type = HANDLE_CONTEXT;

            if (attribs)
            {
                while (*attribs)
                {
                    if (attribs[0] == WGL_CONTEXT_MAJOR_VERSION_ARB)
                    {
                        if (attribs[1] >= 3) type = HANDLE_CONTEXT_V3;
                        break;
                    }
                    attribs += 2;
                }
            }

            context->drv_ctx = drv_ctx;
            if (!(ret = alloc_handle( type, funcs, context ))) HeapFree( GetProcessHeap(), 0, context );
        }
        if (!ret) funcs->wgl.p_wglDeleteContext( drv_ctx );
    }

    release_handle_ptr( share_ptr );
    return ret;
}

static HPBUFFERARB wrap_wglCreatePbufferARB( HDC hdc, int format, int width, int height, const int *attribs )
{
    HPBUFFERARB ret;
    struct wgl_pbuffer *pbuffer;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglCreatePbufferARB) return 0;
    if (!(pbuffer = funcs->ext.p_wglCreatePbufferARB( hdc, format, width, height, attribs ))) return 0;
    ret = alloc_handle( HANDLE_PBUFFER, funcs, pbuffer );
    if (!ret) funcs->ext.p_wglDestroyPbufferARB( pbuffer );
    return ret;
}

static BOOL wrap_wglDestroyPbufferARB( HPBUFFERARB handle )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );

    if (!ptr) return FALSE;
    ptr->funcs->ext.p_wglDestroyPbufferARB( ptr->u.pbuffer );
    free_handle_ptr( ptr );
    return TRUE;
}

static HDC wrap_wglGetPbufferDCARB( HPBUFFERARB handle )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    HDC ret;

    if (!ptr) return 0;
    ret = ptr->funcs->ext.p_wglGetPbufferDCARB( ptr->u.pbuffer );
    release_handle_ptr( ptr );
    return ret;
}

static BOOL wrap_wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, HGLRC hglrc )
{
    BOOL ret = TRUE;
    struct wgl_handle *ptr, *prev = get_current_context_ptr();

    if (hglrc)
    {
        if (!(ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT ))) return FALSE;
        if (!ptr->u.context->tid || ptr->u.context->tid == GetCurrentThreadId())
        {
            ret = (ptr->funcs->ext.p_wglMakeContextCurrentARB &&
                   ptr->funcs->ext.p_wglMakeContextCurrentARB( draw_hdc, read_hdc, ptr->u.context->drv_ctx ));
            if (ret)
            {
                if (prev) prev->u.context->tid = 0;
                ptr->u.context->tid = GetCurrentThreadId();
                ptr->u.context->draw_dc = draw_hdc;
                ptr->u.context->read_dc = read_hdc;
                NtCurrentTeb()->glCurrentRC = hglrc;
                NtCurrentTeb()->glTable = ptr->funcs;
            }
        }
        else
        {
            SetLastError( ERROR_BUSY );
            ret = FALSE;
        }
        release_handle_ptr( ptr );
    }
    else if (prev)
    {
        if (!prev->funcs->wgl.p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->u.context->tid = 0;
        NtCurrentTeb()->glCurrentRC = 0;
        NtCurrentTeb()->glTable = &null_opengl_funcs;
    }
    return ret;
}

static BOOL wrap_wglQueryPbufferARB( HPBUFFERARB handle, int attrib, int *value )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglQueryPbufferARB( ptr->u.pbuffer, attrib, value );
    release_handle_ptr( ptr );
    return ret;
}

static int wrap_wglReleasePbufferDCARB( HPBUFFERARB handle, HDC hdc )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglReleasePbufferDCARB( ptr->u.pbuffer, hdc );
    release_handle_ptr( ptr );
    return ret;
}

static BOOL wrap_wglReleaseTexImageARB( HPBUFFERARB handle, int buffer )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglReleaseTexImageARB( ptr->u.pbuffer, buffer );
    release_handle_ptr( ptr );
    return ret;
}

static BOOL wrap_wglSetPbufferAttribARB( HPBUFFERARB handle, const int *attribs )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglSetPbufferAttribARB( ptr->u.pbuffer, attribs );
    release_handle_ptr( ptr );
    return ret;
}

static void gl_debug_message_callback( GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar *message, const void *userParam )
{
    struct wine_gl_debug_message_params params =
    {
        .source = source,
        .type = type,
        .id = id,
        .severity = severity,
        .length = length,
        .message = message,
    };
    BOOL (WINAPI *callback)( struct wine_gl_debug_message_params *params, ULONG size );
    void **kernel_callback_table;

    struct wgl_handle *ptr = (struct wgl_handle *)userParam;
    if (!(params.user_callback = ptr->u.context->debug_callback)) return;
    params.user_data = ptr->u.context->debug_user;

    kernel_callback_table = NtCurrentTeb()->Peb->KernelCallbackTable;
    callback = kernel_callback_table[NtUserCallOpenGLDebugMessageCallback];
    callback( &params, sizeof(params) );
}

static void WINAPI wrap_glDebugMessageCallback( GLDEBUGPROC callback, const void *userParam )
{
    struct wgl_handle *ptr = get_current_context_ptr();
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_glDebugMessageCallback) return;

    ptr->u.context->debug_callback = callback;
    ptr->u.context->debug_user     = userParam;
    funcs->ext.p_glDebugMessageCallback( gl_debug_message_callback, ptr );
}

static void WINAPI wrap_glDebugMessageCallbackAMD( GLDEBUGPROCAMD callback, void *userParam )
{
    struct wgl_handle *ptr = get_current_context_ptr();
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_glDebugMessageCallbackAMD) return;

    ptr->u.context->debug_callback = callback;
    ptr->u.context->debug_user     = userParam;
    funcs->ext.p_glDebugMessageCallbackAMD( gl_debug_message_callback, ptr );
}

static void WINAPI wrap_glDebugMessageCallbackARB( GLDEBUGPROCARB callback, const void *userParam )
{
    struct wgl_handle *ptr = get_current_context_ptr();
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_glDebugMessageCallbackARB) return;

    ptr->u.context->debug_callback = callback;
    ptr->u.context->debug_user     = userParam;
    funcs->ext.p_glDebugMessageCallbackARB( gl_debug_message_callback, ptr );
}

NTSTATUS wgl_wglCopyContext( void *args )
{
    struct wglCopyContext_params *params = args;
    params->ret = wrap_wglCopyContext( params->hglrcSrc, params->hglrcDst, params->mask );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglCreateContext( void *args )
{
    struct wglCreateContext_params *params = args;
    params->ret = wrap_wglCreateContext( params->hDc );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglDeleteContext( void *args )
{
    struct wglDeleteContext_params *params = args;
    params->ret = wrap_wglDeleteContext( params->oldContext );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglMakeCurrent( void *args )
{
    struct wglMakeCurrent_params *params = args;
    params->ret = wrap_wglMakeCurrent( params->hDc, params->newContext );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglShareLists( void *args )
{
    struct wglShareLists_params *params = args;
    params->ret = wrap_wglShareLists( params->hrcSrvShare, params->hrcSrvSource );
    return STATUS_SUCCESS;
}

NTSTATUS ext_glDebugMessageCallback( void *args )
{
    struct glDebugMessageCallback_params *params = args;
    wrap_glDebugMessageCallback( params->callback, params->userParam );
    return STATUS_SUCCESS;
}
NTSTATUS ext_glDebugMessageCallbackAMD( void *args )
{
    struct glDebugMessageCallbackAMD_params *params = args;
    wrap_glDebugMessageCallbackAMD( params->callback, params->userParam );
    return STATUS_SUCCESS;
}
NTSTATUS ext_glDebugMessageCallbackARB( void *args )
{
    struct glDebugMessageCallbackARB_params *params = args;
    wrap_glDebugMessageCallbackARB( params->callback, params->userParam );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglBindTexImageARB( void *args )
{
    struct wglBindTexImageARB_params *params = args;
    params->ret = wrap_wglBindTexImageARB( params->hPbuffer, params->iBuffer );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglCreateContextAttribsARB( void *args )
{
    struct wglCreateContextAttribsARB_params *params = args;
    params->ret = wrap_wglCreateContextAttribsARB( params->hDC, params->hShareContext, params->attribList );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglCreatePbufferARB( void *args )
{
    struct wglCreatePbufferARB_params *params = args;
    params->ret = wrap_wglCreatePbufferARB( params->hDC, params->iPixelFormat, params->iWidth, params->iHeight, params->piAttribList );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglDestroyPbufferARB( void *args )
{
    struct wglDestroyPbufferARB_params *params = args;
    params->ret = wrap_wglDestroyPbufferARB( params->hPbuffer );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglGetPbufferDCARB( void *args )
{
    struct wglGetPbufferDCARB_params *params = args;
    params->ret = wrap_wglGetPbufferDCARB( params->hPbuffer );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglMakeContextCurrentARB( void *args )
{
    struct wglMakeContextCurrentARB_params *params = args;
    params->ret = wrap_wglMakeContextCurrentARB( params->hDrawDC, params->hReadDC, params->hglrc );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglQueryPbufferARB( void *args )
{
    struct wglQueryPbufferARB_params *params = args;
    params->ret = wrap_wglQueryPbufferARB( params->hPbuffer, params->iAttribute, params->piValue );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglReleasePbufferDCARB( void *args )
{
    struct wglReleasePbufferDCARB_params *params = args;
    params->ret = wrap_wglReleasePbufferDCARB( params->hPbuffer, params->hDC );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglReleaseTexImageARB( void *args )
{
    struct wglReleaseTexImageARB_params *params = args;
    params->ret = wrap_wglReleaseTexImageARB( params->hPbuffer, params->iBuffer );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglSetPbufferAttribARB( void *args )
{
    struct wglSetPbufferAttribARB_params *params = args;
    params->ret = wrap_wglSetPbufferAttribARB( params->hPbuffer, params->piAttribList );
    return STATUS_SUCCESS;
}
