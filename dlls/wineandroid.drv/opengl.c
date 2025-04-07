/*
 * Android OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
 * Copyright 2005 Alex Woods
 * Copyright 2005 Raphael Junqueira
 * Copyright 2006-2009 Roderick Colenbrander
 * Copyright 2006 Tomas Carnecky
 * Copyright 2013 Matteo Bruni
 * Copyright 2012, 2013, 2014, 2017 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#ifdef HAVE_EGL_EGL_H
#include <EGL/egl.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "android.h"
#include "winternl.h"

#include "wine/opengl_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);

#define DECL_FUNCPTR(f) typeof(f) * p_##f = NULL
DECL_FUNCPTR( eglCreateContext );
DECL_FUNCPTR( eglCreateWindowSurface );
DECL_FUNCPTR( eglCreatePbufferSurface );
DECL_FUNCPTR( eglDestroyContext );
DECL_FUNCPTR( eglDestroySurface );
DECL_FUNCPTR( eglGetConfigAttrib );
DECL_FUNCPTR( eglGetConfigs );
DECL_FUNCPTR( eglGetDisplay );
DECL_FUNCPTR( eglGetProcAddress );
DECL_FUNCPTR( eglInitialize );
DECL_FUNCPTR( eglMakeCurrent );
DECL_FUNCPTR( eglSwapBuffers );
DECL_FUNCPTR( eglSwapInterval );
#undef DECL_FUNCPTR

static const int egl_client_version = 2;

struct egl_pixel_format
{
    EGLConfig config;
};

struct android_context
{
    struct list entry;
    EGLConfig  config;
    EGLContext context;
    EGLSurface surface;
    HWND       hwnd;
    BOOL       refresh;
};

struct gl_drawable
{
    struct list     entry;
    HWND            hwnd;
    HDC             hdc;
    int             format;
    ANativeWindow  *window;
    EGLSurface      surface;
    EGLSurface      pbuffer;
    int             swap_interval;
};

static void *egl_handle;
static void *opengl_handle;
static struct egl_pixel_format *pixel_formats;
static int nb_pixel_formats, nb_onscreen_formats;
static EGLDisplay display;
static char wgl_extensions[4096];
static struct opengl_funcs egl_funcs;

static struct list gl_contexts = LIST_INIT( gl_contexts );
static struct list gl_drawables = LIST_INIT( gl_drawables );

pthread_mutex_t drawable_mutex;

static struct gl_drawable *create_gl_drawable( HWND hwnd, HDC hdc, int format )
{
    static const int attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    struct gl_drawable *gl = malloc( sizeof(*gl) );

    gl->hwnd   = hwnd;
    gl->hdc    = hdc;
    gl->format = format;
    gl->window = create_ioctl_window( hwnd, TRUE, 1.0f );
    gl->surface = 0;
    gl->pbuffer = p_eglCreatePbufferSurface( display, pixel_formats[gl->format - 1].config, attribs );
    pthread_mutex_lock( &drawable_mutex );
    list_add_head( &gl_drawables, &gl->entry );
    return gl;
}

static struct gl_drawable *get_gl_drawable( HWND hwnd, HDC hdc )
{
    struct gl_drawable *gl;

    pthread_mutex_lock( &drawable_mutex );
    LIST_FOR_EACH_ENTRY( gl, &gl_drawables, struct gl_drawable, entry )
    {
        if (hwnd && gl->hwnd == hwnd) return gl;
        if (hdc && gl->hdc == hdc) return gl;
    }
    pthread_mutex_unlock( &drawable_mutex );
    return NULL;
}

static void release_gl_drawable( struct gl_drawable *gl )
{
    if (gl) pthread_mutex_unlock( &drawable_mutex );
}

void destroy_gl_drawable( HWND hwnd )
{
    struct gl_drawable *gl;

    pthread_mutex_lock( &drawable_mutex );
    LIST_FOR_EACH_ENTRY( gl, &gl_drawables, struct gl_drawable, entry )
    {
        if (gl->hwnd != hwnd) continue;
        list_remove( &gl->entry );
        if (gl->surface) p_eglDestroySurface( display, gl->surface );
        if (gl->pbuffer) p_eglDestroySurface( display, gl->pbuffer );
        release_ioctl_window( gl->window );
        free( gl );
        break;
    }
    pthread_mutex_unlock( &drawable_mutex );
}

static BOOL refresh_context( struct android_context *ctx )
{
    BOOL ret = InterlockedExchange( &ctx->refresh, FALSE );

    if (ret)
    {
        TRACE( "refreshing hwnd %p context %p surface %p\n", ctx->hwnd, ctx->context, ctx->surface );
        p_eglMakeCurrent( display, ctx->surface, ctx->surface, ctx->context );
        NtUserRedrawWindow( ctx->hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE );
    }
    return ret;
}

void update_gl_drawable( HWND hwnd )
{
    struct gl_drawable *gl;
    struct android_context *ctx;

    if ((gl = get_gl_drawable( hwnd, 0 )))
    {
        if (!gl->surface &&
            (gl->surface = p_eglCreateWindowSurface( display, pixel_formats[gl->format - 1].config, gl->window, NULL )))
        {
            LIST_FOR_EACH_ENTRY( ctx, &gl_contexts, struct android_context, entry )
            {
                if (ctx->hwnd != hwnd) continue;
                TRACE( "hwnd %p refreshing %p %scurrent\n", hwnd, ctx, NtCurrentTeb()->glReserved2 == ctx ? "" : "not " );
                ctx->surface = gl->surface;
                if (NtCurrentTeb()->glReserved2 == ctx)
                    p_eglMakeCurrent( display, ctx->surface, ctx->surface, ctx->context );
                else
                    InterlockedExchange( &ctx->refresh, TRUE );
            }
        }
        release_gl_drawable( gl );
        NtUserRedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE );
    }
}

static BOOL android_set_pixel_format( HWND hwnd, int old_format, int new_format, BOOL internal )
{
    struct gl_drawable *gl;

    TRACE( "hwnd %p, old_format %d, new_format %d, internal %u\n", hwnd, old_format, new_format, internal );

    if ((gl = get_gl_drawable( hwnd, 0 )))
    {
        if (internal)
        {
            EGLint pf;
            p_eglGetConfigAttrib( display, pixel_formats[new_format - 1].config, EGL_NATIVE_VISUAL_ID, &pf );
            gl->window->perform( gl->window, NATIVE_WINDOW_SET_BUFFERS_FORMAT, pf );
            gl->format = new_format;
        }
    }
    else gl = create_gl_drawable( hwnd, 0, new_format );
    release_gl_drawable( gl );

    return TRUE;
}

static BOOL android_context_create( HDC hdc, int format, void *share, const int *attribs, void **private )
{
    struct android_context *ctx, *shared_ctx = share;
    int count = 0, egl_attribs[3];
    BOOL opengl_es = FALSE;

    if (!attribs) opengl_es = TRUE;
    while (attribs && *attribs && count < 2)
    {
        switch (*attribs)
        {
        case WGL_CONTEXT_PROFILE_MASK_ARB:
            if (attribs[1] == WGL_CONTEXT_ES2_PROFILE_BIT_EXT)
                opengl_es = TRUE;
            break;
        case WGL_CONTEXT_MAJOR_VERSION_ARB:
            egl_attribs[count++] = EGL_CONTEXT_CLIENT_VERSION;
            egl_attribs[count++] = attribs[1];
            break;
        default:
            FIXME("Unhandled attributes: %#x %#x\n", attribs[0], attribs[1]);
        }
        attribs += 2;
    }
    if (!opengl_es)
    {
        WARN("Requested creation of an OpenGL (non ES) context, that's not supported.\n");
        return FALSE;
    }
    if (!count)  /* FIXME: force version if not specified */
    {
        egl_attribs[count++] = EGL_CONTEXT_CLIENT_VERSION;
        egl_attribs[count++] = egl_client_version;
    }
    egl_attribs[count] = EGL_NONE;
    attribs = egl_attribs;

    ctx = malloc( sizeof(*ctx) );

    ctx->config  = pixel_formats[format - 1].config;
    ctx->surface = 0;
    ctx->refresh = FALSE;
    ctx->context = p_eglCreateContext( display, ctx->config, shared_ctx ? shared_ctx->context : EGL_NO_CONTEXT, attribs );
    TRACE( "%p fmt %d ctx %p\n", hdc, format, ctx->context );
    list_add_head( &gl_contexts, &ctx->entry );

    *private = ctx;
    return TRUE;
}

static BOOL android_describe_pixel_format( int format, struct wgl_pixel_format *desc )
{
    struct egl_pixel_format *fmt = pixel_formats + format - 1;
    PIXELFORMATDESCRIPTOR *pfd = &desc->pfd;
    EGLint val;
    EGLConfig config = fmt->config;

    if (format <= 0 || format > nb_pixel_formats) return FALSE;

    memset( pfd, 0, sizeof(*pfd) );
    pfd->nSize = sizeof(*pfd);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SUPPORT_COMPOSITION;
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->iLayerType = PFD_MAIN_PLANE;

    p_eglGetConfigAttrib( display, config, EGL_BUFFER_SIZE, &val );
    pfd->cColorBits = val;
    p_eglGetConfigAttrib( display, config, EGL_RED_SIZE, &val );
    pfd->cRedBits = val;
    p_eglGetConfigAttrib( display, config, EGL_GREEN_SIZE, &val );
    pfd->cGreenBits = val;
    p_eglGetConfigAttrib( display, config, EGL_BLUE_SIZE, &val );
    pfd->cBlueBits = val;
    p_eglGetConfigAttrib( display, config, EGL_ALPHA_SIZE, &val );
    pfd->cAlphaBits = val;
    p_eglGetConfigAttrib( display, config, EGL_DEPTH_SIZE, &val );
    pfd->cDepthBits = val;
    p_eglGetConfigAttrib( display, config, EGL_STENCIL_SIZE, &val );
    pfd->cStencilBits = val;

    pfd->cAlphaShift = 0;
    pfd->cBlueShift = pfd->cAlphaShift + pfd->cAlphaBits;
    pfd->cGreenShift = pfd->cBlueShift + pfd->cBlueBits;
    pfd->cRedShift = pfd->cGreenShift + pfd->cGreenBits;
    return TRUE;
}

static BOOL android_context_make_current( HDC draw_hdc, HDC read_hdc, void *private )
{
    struct android_context *ctx = private;
    BOOL ret = FALSE;
    struct gl_drawable *draw_gl, *read_gl = NULL;
    EGLSurface draw_surface, read_surface;
    HWND draw_hwnd;

    TRACE( "%p %p %p\n", draw_hdc, read_hdc, ctx );

    if (!private)
    {
        p_eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
        NtCurrentTeb()->glReserved2 = NULL;
        return TRUE;
    }

    draw_hwnd = NtUserWindowFromDC( draw_hdc );
    if ((draw_gl = get_gl_drawable( draw_hwnd, draw_hdc )))
    {
        read_gl = get_gl_drawable( NtUserWindowFromDC( read_hdc ), read_hdc );
        draw_surface = draw_gl->surface ? draw_gl->surface : draw_gl->pbuffer;
        read_surface = read_gl->surface ? read_gl->surface : read_gl->pbuffer;
        TRACE( "%p/%p context %p surface %p/%p\n",
               draw_hdc, read_hdc, ctx->context, draw_surface, read_surface );
        ret = p_eglMakeCurrent( display, draw_surface, read_surface, ctx->context );
        if (ret)
        {
            ctx->surface = draw_gl->surface;
            ctx->hwnd    = draw_hwnd;
            ctx->refresh = FALSE;
            NtCurrentTeb()->glReserved2 = ctx;
            goto done;
        }
    }
    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );

done:
    release_gl_drawable( read_gl );
    release_gl_drawable( draw_gl );
    return ret;
}

/***********************************************************************
 *		android_wglCopyContext
 */
static BOOL android_context_copy( void *src, void *dst, UINT mask )
{
    FIXME( "%p -> %p mask %#x unsupported\n", src, dst, mask );
    return FALSE;
}

/***********************************************************************
 *		android_wglDeleteContext
 */
static BOOL android_context_destroy( void *private )
{
    struct android_context *ctx = private;
    pthread_mutex_lock( &drawable_mutex );
    list_remove( &ctx->entry );
    pthread_mutex_unlock( &drawable_mutex );
    p_eglDestroyContext( display, ctx->context );
    free( ctx );
    return TRUE;
}

static void *android_get_proc_address( const char *name )
{
    void *ptr;
    if ((ptr = dlsym( opengl_handle, name ))) return ptr;
    return p_eglGetProcAddress( name );
}

static BOOL android_context_share( void *org, void *dest )
{
    FIXME( "%p %p\n", org, dest );
    return FALSE;
}

static void set_swap_interval( struct gl_drawable *gl, int interval )
{
    if (interval < 0) interval = -interval;
    if (gl->swap_interval == interval) return;
    p_eglSwapInterval( display, interval );
    gl->swap_interval = interval;
}

static BOOL android_swap_buffers( void *private, HWND hwnd, HDC hdc, int interval )
{
    struct android_context *ctx = private;
    struct gl_drawable *gl;

    if (!ctx) return FALSE;

    TRACE( "%p hwnd %p context %p surface %p\n", hdc, ctx->hwnd, ctx->context, ctx->surface );

    if (refresh_context( ctx )) return TRUE;

    if ((gl = get_gl_drawable( hwnd, hdc )))
    {
        set_swap_interval( gl, interval );
        release_gl_drawable( gl );
    }

    if (ctx->surface) p_eglSwapBuffers( display, ctx->surface );
    return TRUE;
}

static BOOL android_context_flush( void *private, HWND hwnd, HDC hdc, int interval, BOOL finish )
{
    struct android_context *ctx = private;
    struct gl_drawable *gl;

    TRACE( "hwnd %p context %p\n", ctx->hwnd, ctx->context );
    refresh_context( ctx );

    if ((gl = get_gl_drawable( hwnd, hdc )))
    {
        set_swap_interval( gl, interval );
        release_gl_drawable( gl );
    }
    return FALSE;
}

static void register_extension( const char *ext )
{
    if (wgl_extensions[0]) strcat( wgl_extensions, " " );
    strcat( wgl_extensions, ext );
    TRACE( "%s\n", ext );
}

static const char *android_init_wgl_extensions(void)
{
    register_extension("WGL_EXT_framebuffer_sRGB");
    return wgl_extensions;
}

static void init_opengl_funcs(void)
{
    void *ptr;

    /* load standard functions and extensions exported from the OpenGL library */

#define USE_GL_FUNC(func) if ((ptr = dlsym( opengl_handle, #func ))) egl_funcs.p_##func = ptr;
    ALL_GL_FUNCS
#undef USE_GL_FUNC

#define LOAD_FUNCPTR(func) egl_funcs.p_##func = dlsym( opengl_handle, #func )
    LOAD_FUNCPTR( glActiveShaderProgram );
    LOAD_FUNCPTR( glActiveTexture );
    LOAD_FUNCPTR( glAttachShader );
    LOAD_FUNCPTR( glBeginQuery );
    LOAD_FUNCPTR( glBeginTransformFeedback );
    LOAD_FUNCPTR( glBindAttribLocation );
    LOAD_FUNCPTR( glBindBuffer );
    LOAD_FUNCPTR( glBindBufferBase );
    LOAD_FUNCPTR( glBindBufferRange );
    LOAD_FUNCPTR( glBindFramebuffer );
    LOAD_FUNCPTR( glBindImageTexture );
    LOAD_FUNCPTR( glBindProgramPipeline );
    LOAD_FUNCPTR( glBindRenderbuffer );
    LOAD_FUNCPTR( glBindSampler );
    LOAD_FUNCPTR( glBindTransformFeedback );
    LOAD_FUNCPTR( glBindVertexArray );
    LOAD_FUNCPTR( glBindVertexBuffer );
    LOAD_FUNCPTR( glBlendBarrierKHR );
    LOAD_FUNCPTR( glBlendColor );
    LOAD_FUNCPTR( glBlendEquation );
    LOAD_FUNCPTR( glBlendEquationSeparate );
    LOAD_FUNCPTR( glBlendFuncSeparate );
    LOAD_FUNCPTR( glBlitFramebuffer );
    LOAD_FUNCPTR( glBufferData );
    LOAD_FUNCPTR( glBufferSubData );
    LOAD_FUNCPTR( glCheckFramebufferStatus );
    LOAD_FUNCPTR( glClearBufferfi );
    LOAD_FUNCPTR( glClearBufferfv );
    LOAD_FUNCPTR( glClearBufferiv );
    LOAD_FUNCPTR( glClearBufferuiv );
    LOAD_FUNCPTR( glClearDepthf );
    LOAD_FUNCPTR( glClientWaitSync );
    LOAD_FUNCPTR( glCompileShader );
    LOAD_FUNCPTR( glCompressedTexImage2D );
    LOAD_FUNCPTR( glCompressedTexImage3D );
    LOAD_FUNCPTR( glCompressedTexSubImage2D );
    LOAD_FUNCPTR( glCompressedTexSubImage3D );
    LOAD_FUNCPTR( glCopyBufferSubData );
    LOAD_FUNCPTR( glCopyTexSubImage3D );
    LOAD_FUNCPTR( glCreateProgram );
    LOAD_FUNCPTR( glCreateShader );
    LOAD_FUNCPTR( glCreateShaderProgramv );
    LOAD_FUNCPTR( glDeleteBuffers );
    LOAD_FUNCPTR( glDeleteFramebuffers );
    LOAD_FUNCPTR( glDeleteProgram );
    LOAD_FUNCPTR( glDeleteProgramPipelines );
    LOAD_FUNCPTR( glDeleteQueries );
    LOAD_FUNCPTR( glDeleteRenderbuffers );
    LOAD_FUNCPTR( glDeleteSamplers );
    LOAD_FUNCPTR( glDeleteShader );
    LOAD_FUNCPTR( glDeleteSync );
    LOAD_FUNCPTR( glDeleteTransformFeedbacks );
    LOAD_FUNCPTR( glDeleteVertexArrays );
    LOAD_FUNCPTR( glDepthRangef );
    LOAD_FUNCPTR( glDetachShader );
    LOAD_FUNCPTR( glDisableVertexAttribArray );
    LOAD_FUNCPTR( glDispatchCompute );
    LOAD_FUNCPTR( glDispatchComputeIndirect );
    LOAD_FUNCPTR( glDrawArraysIndirect );
    LOAD_FUNCPTR( glDrawArraysInstanced );
    LOAD_FUNCPTR( glDrawBuffers );
    LOAD_FUNCPTR( glDrawElementsIndirect );
    LOAD_FUNCPTR( glDrawElementsInstanced );
    LOAD_FUNCPTR( glDrawRangeElements );
    LOAD_FUNCPTR( glEnableVertexAttribArray );
    LOAD_FUNCPTR( glEndQuery );
    LOAD_FUNCPTR( glEndTransformFeedback );
    LOAD_FUNCPTR( glFenceSync );
    LOAD_FUNCPTR( glFlushMappedBufferRange );
    LOAD_FUNCPTR( glFramebufferParameteri );
    LOAD_FUNCPTR( glFramebufferRenderbuffer );
    LOAD_FUNCPTR( glFramebufferTexture2D );
    LOAD_FUNCPTR( glFramebufferTextureEXT );
    LOAD_FUNCPTR( glFramebufferTextureLayer );
    LOAD_FUNCPTR( glGenBuffers );
    LOAD_FUNCPTR( glGenFramebuffers );
    LOAD_FUNCPTR( glGenProgramPipelines );
    LOAD_FUNCPTR( glGenQueries );
    LOAD_FUNCPTR( glGenRenderbuffers );
    LOAD_FUNCPTR( glGenSamplers );
    LOAD_FUNCPTR( glGenTransformFeedbacks );
    LOAD_FUNCPTR( glGenVertexArrays );
    LOAD_FUNCPTR( glGenerateMipmap );
    LOAD_FUNCPTR( glGetActiveAttrib );
    LOAD_FUNCPTR( glGetActiveUniform );
    LOAD_FUNCPTR( glGetActiveUniformBlockName );
    LOAD_FUNCPTR( glGetActiveUniformBlockiv );
    LOAD_FUNCPTR( glGetActiveUniformsiv );
    LOAD_FUNCPTR( glGetAttachedShaders );
    LOAD_FUNCPTR( glGetAttribLocation );
    LOAD_FUNCPTR( glGetBooleani_v );
    LOAD_FUNCPTR( glGetBufferParameteri64v );
    LOAD_FUNCPTR( glGetBufferParameteriv );
    LOAD_FUNCPTR( glGetBufferPointerv );
    LOAD_FUNCPTR( glGetFragDataLocation );
    LOAD_FUNCPTR( glGetFramebufferAttachmentParameteriv );
    LOAD_FUNCPTR( glGetFramebufferParameteriv );
    LOAD_FUNCPTR( glGetInteger64i_v );
    LOAD_FUNCPTR( glGetInteger64v );
    LOAD_FUNCPTR( glGetIntegeri_v );
    LOAD_FUNCPTR( glGetInternalformativ );
    LOAD_FUNCPTR( glGetMultisamplefv );
    LOAD_FUNCPTR( glGetProgramBinary );
    LOAD_FUNCPTR( glGetProgramInfoLog );
    LOAD_FUNCPTR( glGetProgramInterfaceiv );
    LOAD_FUNCPTR( glGetProgramPipelineInfoLog );
    LOAD_FUNCPTR( glGetProgramPipelineiv );
    LOAD_FUNCPTR( glGetProgramResourceIndex );
    LOAD_FUNCPTR( glGetProgramResourceLocation );
    LOAD_FUNCPTR( glGetProgramResourceName );
    LOAD_FUNCPTR( glGetProgramResourceiv );
    LOAD_FUNCPTR( glGetProgramiv );
    LOAD_FUNCPTR( glGetQueryObjectuiv );
    LOAD_FUNCPTR( glGetQueryiv );
    LOAD_FUNCPTR( glGetRenderbufferParameteriv );
    LOAD_FUNCPTR( glGetSamplerParameterfv );
    LOAD_FUNCPTR( glGetSamplerParameteriv );
    LOAD_FUNCPTR( glGetShaderInfoLog );
    LOAD_FUNCPTR( glGetShaderPrecisionFormat );
    LOAD_FUNCPTR( glGetShaderSource );
    LOAD_FUNCPTR( glGetShaderiv );
    LOAD_FUNCPTR( glGetStringi );
    LOAD_FUNCPTR( glGetSynciv );
    LOAD_FUNCPTR( glGetTexParameterIivEXT );
    LOAD_FUNCPTR( glGetTexParameterIuivEXT );
    LOAD_FUNCPTR( glGetTransformFeedbackVarying );
    LOAD_FUNCPTR( glGetUniformBlockIndex );
    LOAD_FUNCPTR( glGetUniformIndices );
    LOAD_FUNCPTR( glGetUniformLocation );
    LOAD_FUNCPTR( glGetUniformfv );
    LOAD_FUNCPTR( glGetUniformiv );
    LOAD_FUNCPTR( glGetUniformuiv );
    LOAD_FUNCPTR( glGetVertexAttribIiv );
    LOAD_FUNCPTR( glGetVertexAttribIuiv );
    LOAD_FUNCPTR( glGetVertexAttribPointerv );
    LOAD_FUNCPTR( glGetVertexAttribfv );
    LOAD_FUNCPTR( glGetVertexAttribiv );
    LOAD_FUNCPTR( glInvalidateFramebuffer );
    LOAD_FUNCPTR( glInvalidateSubFramebuffer );
    LOAD_FUNCPTR( glIsBuffer );
    LOAD_FUNCPTR( glIsFramebuffer );
    LOAD_FUNCPTR( glIsProgram );
    LOAD_FUNCPTR( glIsProgramPipeline );
    LOAD_FUNCPTR( glIsQuery );
    LOAD_FUNCPTR( glIsRenderbuffer );
    LOAD_FUNCPTR( glIsSampler );
    LOAD_FUNCPTR( glIsShader );
    LOAD_FUNCPTR( glIsSync );
    LOAD_FUNCPTR( glIsTransformFeedback );
    LOAD_FUNCPTR( glIsVertexArray );
    LOAD_FUNCPTR( glLinkProgram );
    LOAD_FUNCPTR( glMapBufferRange );
    LOAD_FUNCPTR( glMemoryBarrier );
    LOAD_FUNCPTR( glMemoryBarrierByRegion );
    LOAD_FUNCPTR( glPauseTransformFeedback );
    LOAD_FUNCPTR( glProgramBinary );
    LOAD_FUNCPTR( glProgramParameteri );
    LOAD_FUNCPTR( glProgramUniform1f );
    LOAD_FUNCPTR( glProgramUniform1fv );
    LOAD_FUNCPTR( glProgramUniform1i );
    LOAD_FUNCPTR( glProgramUniform1iv );
    LOAD_FUNCPTR( glProgramUniform1ui );
    LOAD_FUNCPTR( glProgramUniform1uiv );
    LOAD_FUNCPTR( glProgramUniform2f );
    LOAD_FUNCPTR( glProgramUniform2fv );
    LOAD_FUNCPTR( glProgramUniform2i );
    LOAD_FUNCPTR( glProgramUniform2iv );
    LOAD_FUNCPTR( glProgramUniform2ui );
    LOAD_FUNCPTR( glProgramUniform2uiv );
    LOAD_FUNCPTR( glProgramUniform3f );
    LOAD_FUNCPTR( glProgramUniform3fv );
    LOAD_FUNCPTR( glProgramUniform3i );
    LOAD_FUNCPTR( glProgramUniform3iv );
    LOAD_FUNCPTR( glProgramUniform3ui );
    LOAD_FUNCPTR( glProgramUniform3uiv );
    LOAD_FUNCPTR( glProgramUniform4f );
    LOAD_FUNCPTR( glProgramUniform4fv );
    LOAD_FUNCPTR( glProgramUniform4i );
    LOAD_FUNCPTR( glProgramUniform4iv );
    LOAD_FUNCPTR( glProgramUniform4ui );
    LOAD_FUNCPTR( glProgramUniform4uiv );
    LOAD_FUNCPTR( glProgramUniformMatrix2fv );
    LOAD_FUNCPTR( glProgramUniformMatrix2x3fv );
    LOAD_FUNCPTR( glProgramUniformMatrix2x4fv );
    LOAD_FUNCPTR( glProgramUniformMatrix3fv );
    LOAD_FUNCPTR( glProgramUniformMatrix3x2fv );
    LOAD_FUNCPTR( glProgramUniformMatrix3x4fv );
    LOAD_FUNCPTR( glProgramUniformMatrix4fv );
    LOAD_FUNCPTR( glProgramUniformMatrix4x2fv );
    LOAD_FUNCPTR( glProgramUniformMatrix4x3fv );
    LOAD_FUNCPTR( glReleaseShaderCompiler );
    LOAD_FUNCPTR( glRenderbufferStorage );
    LOAD_FUNCPTR( glRenderbufferStorageMultisample );
    LOAD_FUNCPTR( glResumeTransformFeedback );
    LOAD_FUNCPTR( glSampleCoverage );
    LOAD_FUNCPTR( glSampleMaski );
    LOAD_FUNCPTR( glSamplerParameterf );
    LOAD_FUNCPTR( glSamplerParameterfv );
    LOAD_FUNCPTR( glSamplerParameteri );
    LOAD_FUNCPTR( glSamplerParameteriv );
    LOAD_FUNCPTR( glShaderBinary );
    LOAD_FUNCPTR( glShaderSource );
    LOAD_FUNCPTR( glStencilFuncSeparate );
    LOAD_FUNCPTR( glStencilMaskSeparate );
    LOAD_FUNCPTR( glStencilOpSeparate );
    LOAD_FUNCPTR( glTexBufferEXT );
    LOAD_FUNCPTR( glTexImage3D );
    LOAD_FUNCPTR( glTexParameterIivEXT );
    LOAD_FUNCPTR( glTexParameterIuivEXT );
    LOAD_FUNCPTR( glTexStorage2D );
    LOAD_FUNCPTR( glTexStorage2DMultisample );
    LOAD_FUNCPTR( glTexStorage3D );
    LOAD_FUNCPTR( glTexSubImage3D );
    LOAD_FUNCPTR( glTransformFeedbackVaryings );
    LOAD_FUNCPTR( glUniform1f );
    LOAD_FUNCPTR( glUniform1fv );
    LOAD_FUNCPTR( glUniform1i );
    LOAD_FUNCPTR( glUniform1iv );
    LOAD_FUNCPTR( glUniform1ui );
    LOAD_FUNCPTR( glUniform1uiv );
    LOAD_FUNCPTR( glUniform2f );
    LOAD_FUNCPTR( glUniform2fv );
    LOAD_FUNCPTR( glUniform2i );
    LOAD_FUNCPTR( glUniform2iv );
    LOAD_FUNCPTR( glUniform2ui );
    LOAD_FUNCPTR( glUniform2uiv );
    LOAD_FUNCPTR( glUniform3f );
    LOAD_FUNCPTR( glUniform3fv );
    LOAD_FUNCPTR( glUniform3i );
    LOAD_FUNCPTR( glUniform3iv );
    LOAD_FUNCPTR( glUniform3ui );
    LOAD_FUNCPTR( glUniform3uiv );
    LOAD_FUNCPTR( glUniform4f );
    LOAD_FUNCPTR( glUniform4fv );
    LOAD_FUNCPTR( glUniform4i );
    LOAD_FUNCPTR( glUniform4iv );
    LOAD_FUNCPTR( glUniform4ui );
    LOAD_FUNCPTR( glUniform4uiv );
    LOAD_FUNCPTR( glUniformBlockBinding );
    LOAD_FUNCPTR( glUniformMatrix2fv );
    LOAD_FUNCPTR( glUniformMatrix2x3fv );
    LOAD_FUNCPTR( glUniformMatrix2x4fv );
    LOAD_FUNCPTR( glUniformMatrix3fv );
    LOAD_FUNCPTR( glUniformMatrix3x2fv );
    LOAD_FUNCPTR( glUniformMatrix3x4fv );
    LOAD_FUNCPTR( glUniformMatrix4fv );
    LOAD_FUNCPTR( glUniformMatrix4x2fv );
    LOAD_FUNCPTR( glUniformMatrix4x3fv );
    LOAD_FUNCPTR( glUnmapBuffer );
    LOAD_FUNCPTR( glUseProgram );
    LOAD_FUNCPTR( glUseProgramStages );
    LOAD_FUNCPTR( glValidateProgram );
    LOAD_FUNCPTR( glValidateProgramPipeline );
    LOAD_FUNCPTR( glVertexAttrib1f );
    LOAD_FUNCPTR( glVertexAttrib1fv );
    LOAD_FUNCPTR( glVertexAttrib2f );
    LOAD_FUNCPTR( glVertexAttrib2fv );
    LOAD_FUNCPTR( glVertexAttrib3f );
    LOAD_FUNCPTR( glVertexAttrib3fv );
    LOAD_FUNCPTR( glVertexAttrib4f );
    LOAD_FUNCPTR( glVertexAttrib4fv );
    LOAD_FUNCPTR( glVertexAttribBinding );
    LOAD_FUNCPTR( glVertexAttribDivisor );
    LOAD_FUNCPTR( glVertexAttribFormat );
    LOAD_FUNCPTR( glVertexAttribI4i );
    LOAD_FUNCPTR( glVertexAttribI4iv );
    LOAD_FUNCPTR( glVertexAttribI4ui );
    LOAD_FUNCPTR( glVertexAttribI4uiv );
    LOAD_FUNCPTR( glVertexAttribIFormat );
    LOAD_FUNCPTR( glVertexAttribIPointer );
    LOAD_FUNCPTR( glVertexAttribPointer );
    LOAD_FUNCPTR( glVertexBindingDivisor );
    LOAD_FUNCPTR( glWaitSync );
#undef LOAD_FUNCPTR
}

static UINT android_init_pixel_formats( UINT *onscreen_count )
{
    EGLConfig *configs;
    EGLint count, i, pass;

    p_eglGetConfigs( display, NULL, 0, &count );
    configs = malloc( count * sizeof(*configs) );
    pixel_formats = malloc( count * sizeof(*pixel_formats) );
    p_eglGetConfigs( display, configs, count, &count );
    if (!count || !configs || !pixel_formats)
    {
        free( configs );
        free( pixel_formats );
        ERR( "eglGetConfigs returned no configs\n" );
        return 0;
    }

    for (pass = 0; pass < 2; pass++)
    {
        for (i = 0; i < count; i++)
        {
            EGLint id, type, visual_id, native, render, color, r, g, b, d, s;

            p_eglGetConfigAttrib( display, configs[i], EGL_SURFACE_TYPE, &type );
            if (!(type & EGL_WINDOW_BIT) == !pass) continue;
            p_eglGetConfigAttrib( display, configs[i], EGL_RENDERABLE_TYPE, &render );
            if (egl_client_version == 2 && !(render & EGL_OPENGL_ES2_BIT)) continue;

            pixel_formats[nb_pixel_formats++].config = configs[i];

            p_eglGetConfigAttrib( display, configs[i], EGL_CONFIG_ID, &id );
            p_eglGetConfigAttrib( display, configs[i], EGL_NATIVE_VISUAL_ID, &visual_id );
            p_eglGetConfigAttrib( display, configs[i], EGL_NATIVE_RENDERABLE, &native );
            p_eglGetConfigAttrib( display, configs[i], EGL_COLOR_BUFFER_TYPE, &color );
            p_eglGetConfigAttrib( display, configs[i], EGL_RED_SIZE, &r );
            p_eglGetConfigAttrib( display, configs[i], EGL_GREEN_SIZE, &g );
            p_eglGetConfigAttrib( display, configs[i], EGL_BLUE_SIZE, &b );
            p_eglGetConfigAttrib( display, configs[i], EGL_DEPTH_SIZE, &d );
            p_eglGetConfigAttrib( display, configs[i], EGL_STENCIL_SIZE, &s );
            TRACE( "%u: config %u id %u type %x visual %u native %u render %x colortype %u rgb %u,%u,%u depth %u stencil %u\n",
                   nb_pixel_formats, i, id, type, visual_id, native, render, color, r, g, b, d, s );
        }
        if (!pass) nb_onscreen_formats = nb_pixel_formats;
    }

    *onscreen_count = nb_onscreen_formats;
    return nb_pixel_formats;
}

static const struct opengl_driver_funcs android_driver_funcs =
{
    .p_get_proc_address = android_get_proc_address,
    .p_init_pixel_formats = android_init_pixel_formats,
    .p_describe_pixel_format = android_describe_pixel_format,
    .p_init_wgl_extensions = android_init_wgl_extensions,
    .p_set_pixel_format = android_set_pixel_format,
    .p_swap_buffers = android_swap_buffers,
    .p_context_create = android_context_create,
    .p_context_destroy = android_context_destroy,
    .p_context_copy = android_context_copy,
    .p_context_share = android_context_share,
    .p_context_flush = android_context_flush,
    .p_context_make_current = android_context_make_current,
};

/**********************************************************************
 *           ANDROID_OpenGLInit
 */
UINT ANDROID_OpenGLInit( UINT version, struct opengl_funcs **funcs, const struct opengl_driver_funcs **driver_funcs )
{
    EGLint major, minor;

    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but driver has %u\n", version, WINE_OPENGL_DRIVER_VERSION );
        return STATUS_INVALID_PARAMETER;
    }
    if (!(egl_handle = dlopen( SONAME_LIBEGL, RTLD_NOW|RTLD_GLOBAL )))
    {
        ERR( "failed to load %s: %s\n", SONAME_LIBEGL, dlerror() );
        return STATUS_NOT_SUPPORTED;
    }
    if (!(opengl_handle = dlopen( SONAME_LIBGLESV2, RTLD_NOW|RTLD_GLOBAL )))
    {
        ERR( "failed to load %s: %s\n", SONAME_LIBGLESV2, dlerror() );
        return STATUS_NOT_SUPPORTED;
    }

#define LOAD_FUNCPTR(func) do { \
        if (!(p_##func = dlsym( egl_handle, #func ))) \
        { ERR( "can't find symbol %s\n", #func); return FALSE; }    \
    } while(0)
    LOAD_FUNCPTR( eglCreateContext );
    LOAD_FUNCPTR( eglCreateWindowSurface );
    LOAD_FUNCPTR( eglCreatePbufferSurface );
    LOAD_FUNCPTR( eglDestroyContext );
    LOAD_FUNCPTR( eglDestroySurface );
    LOAD_FUNCPTR( eglGetConfigAttrib );
    LOAD_FUNCPTR( eglGetConfigs );
    LOAD_FUNCPTR( eglGetDisplay );
    LOAD_FUNCPTR( eglGetProcAddress );
    LOAD_FUNCPTR( eglInitialize );
    LOAD_FUNCPTR( eglMakeCurrent );
    LOAD_FUNCPTR( eglSwapBuffers );
    LOAD_FUNCPTR( eglSwapInterval );
#undef LOAD_FUNCPTR

    display = p_eglGetDisplay( EGL_DEFAULT_DISPLAY );
    if (!p_eglInitialize( display, &major, &minor )) return 0;
    TRACE( "display %p version %u.%u\n", display, major, minor );

    init_opengl_funcs();
    *funcs = &egl_funcs;
    *driver_funcs = &android_driver_funcs;
    return STATUS_SUCCESS;
}


/* generate stubs for GL functions that are not exported on Android */

#define USE_GL_FUNC(name) \
static void glstub_##name(void) \
{ \
    ERR( #name " called\n" ); \
    assert( 0 ); \
    ExitProcess( 1 ); \
}

ALL_GL_FUNCS
#undef USE_GL_FUNC

static struct opengl_funcs egl_funcs =
{
#define USE_GL_FUNC(name) .p_##name = (void *)glstub_##name,
    ALL_GL_FUNCS
#undef USE_GL_FUNC
};
