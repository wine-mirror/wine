/*
 * Copyright 2025 Rémi Bernon for CodeWeavers
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

#ifndef __WINE_OPENGL_DRIVER_H
#define __WINE_OPENGL_DRIVER_H

#include <stdarg.h>
#include <stddef.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>

#include "wine/wgl.h"
#include "wine/debug.h"

struct wgl_pixel_format
{
    PIXELFORMATDESCRIPTOR pfd;
    int swap_method;
    int transparent;
    int pixel_type;
    int draw_to_pbuffer;
    int max_pbuffer_pixels;
    int max_pbuffer_width;
    int max_pbuffer_height;
    int transparent_red_value;
    int transparent_red_value_valid;
    int transparent_green_value;
    int transparent_green_value_valid;
    int transparent_blue_value;
    int transparent_blue_value_valid;
    int transparent_alpha_value;
    int transparent_alpha_value_valid;
    int transparent_index_value;
    int transparent_index_value_valid;
    int sample_buffers;
    int samples;
    int bind_to_texture_rgb;
    int bind_to_texture_rgba;
    int bind_to_texture_rectangle_rgb;
    int bind_to_texture_rectangle_rgba;
    int framebuffer_srgb_capable;
    int float_components;
};

#ifdef WINE_UNIX_LIB

#include "wine/gdi_driver.h"

/* Wine internal opengl driver version, needs to be bumped upon opengl_funcs changes. */
#define WINE_OPENGL_DRIVER_VERSION 37

struct opengl_drawable;
struct wgl_context;
struct wgl_pbuffer;

struct wgl_context
{
    void                   *driver_private;     /* driver context / private data */
    void                   *internal_context;   /* driver context for win32u internal use */
    int                     format;             /* pixel format of the context */
    struct opengl_drawable *draw;               /* currently bound draw surface */
    struct opengl_drawable *read;               /* currently bound read surface */
};

/* interface between opengl32 and win32u */
struct opengl_funcs
{
    BOOL       (*p_wgl_context_reset)( struct wgl_context *context, HDC hdc, struct wgl_context *share, const int *attribs );
    BOOL       (*p_wgl_context_flush)( struct wgl_context *context, void (*flush)(void) );
    BOOL       (*p_wglCopyContext)( struct wgl_context * hglrcSrc, struct wgl_context * hglrcDst, UINT mask );
    struct wgl_context * (*p_wglCreateContext)( HDC hDc );
    BOOL       (*p_wglDeleteContext)( struct wgl_context * oldContext );
    int        (*p_wglGetPixelFormat)( HDC hdc );
    PROC       (*p_wglGetProcAddress)( LPCSTR lpszProc );
    BOOL       (*p_wglMakeCurrent)( HDC hDc, struct wgl_context * newContext );
    BOOL       (*p_wglSetPixelFormat)( HDC hdc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd );
    BOOL       (*p_wglShareLists)( struct wgl_context * hrcSrvShare, struct wgl_context * hrcSrvSource );
    BOOL       (*p_wglSwapBuffers)( HDC hdc );
    void       (*p_get_pixel_formats)( struct wgl_pixel_format *formats, UINT max_formats, UINT *num_formats, UINT *num_onscreen_formats );
    void *     (*p_wglAllocateMemoryNV)( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority );
    BOOL       (*p_wglBindTexImageARB)( struct wgl_pbuffer * hPbuffer, int iBuffer );
    BOOL       (*p_wglChoosePixelFormatARB)( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats );
    struct wgl_context * (*p_wglCreateContextAttribsARB)( HDC hDC, struct wgl_context * hShareContext, const int *attribList );
    struct wgl_pbuffer * (*p_wglCreatePbufferARB)( HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList );
    BOOL       (*p_wglDestroyPbufferARB)( struct wgl_pbuffer * hPbuffer );
    void       (*p_wglFreeMemoryNV)( void *pointer );
    HDC        (*p_wglGetCurrentReadDCARB)(void);
    const char * (*p_wglGetExtensionsStringARB)( HDC hdc );
    const char * (*p_wglGetExtensionsStringEXT)(void);
    HDC        (*p_wglGetPbufferDCARB)( struct wgl_pbuffer * hPbuffer );
    BOOL       (*p_wglGetPixelFormatAttribfvARB)( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues );
    BOOL       (*p_wglGetPixelFormatAttribivARB)( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues );
    int        (*p_wglGetSwapIntervalEXT)(void);
    BOOL       (*p_wglMakeContextCurrentARB)( HDC hDrawDC, HDC hReadDC, struct wgl_context * hglrc );
    BOOL       (*p_wglQueryCurrentRendererIntegerWINE)( GLenum attribute, GLuint *value );
    const GLchar * (*p_wglQueryCurrentRendererStringWINE)( GLenum attribute );
    BOOL       (*p_wglQueryPbufferARB)( struct wgl_pbuffer * hPbuffer, int iAttribute, int *piValue );
    BOOL       (*p_wglQueryRendererIntegerWINE)( HDC dc, GLint renderer, GLenum attribute, GLuint *value );
    const GLchar * (*p_wglQueryRendererStringWINE)( HDC dc, GLint renderer, GLenum attribute );
    int        (*p_wglReleasePbufferDCARB)( struct wgl_pbuffer * hPbuffer, HDC hDC );
    BOOL       (*p_wglReleaseTexImageARB)( struct wgl_pbuffer * hPbuffer, int iBuffer );
    BOOL       (*p_wglSetPbufferAttribARB)( struct wgl_pbuffer * hPbuffer, const int *piAttribList );
    BOOL       (*p_wglSetPixelFormatWINE)( HDC hdc, int format );
    BOOL       (*p_wglSwapIntervalEXT)( int interval );
#define USE_GL_FUNC(x) PFN_##x p_##x;
    ALL_EGL_FUNCS
    ALL_GL_FUNCS
    ALL_GL_EXT_FUNCS
#undef USE_GL_FUNC

    void       *egl_handle;
};

struct egl_platform
{
    EGLenum              type;
    EGLNativeDisplayType native_display;
    BOOL                 force_pbuffer_formats;

    /* filled by win32u after init_egl_platform */
    EGLDisplay  display;
    UINT        config_count;
    EGLConfig  *configs;
    BOOL        has_EGL_EXT_present_opaque;
    BOOL        has_EGL_EXT_pixel_format_float;
};

struct opengl_drawable_funcs
{
    void (*destroy)( struct opengl_drawable *iface );
    /* flush and update the drawable front buffer, called from render thread */
    void (*flush)( struct opengl_drawable *iface, UINT flags );
    /* swap and present the drawable buffers, called from render thread */
    BOOL (*swap)( struct opengl_drawable *iface );
    /* drawable is being unset or made current, called from render thread */
    void (*set_context)( struct opengl_drawable *iface, void *private );
};

/* flags for opengl_drawable flush */
#define GL_FLUSH_FINISHED      0x01
#define GL_FLUSH_INTERVAL      0x02
#define GL_FLUSH_UPDATED       0x04

/* a driver opengl drawable, either a client surface of a pbuffer */
struct opengl_drawable
{
    const struct opengl_drawable_funcs *funcs;
    LONG                                ref;            /* reference count */
    struct client_surface              *client;         /* underlying client surface */
    int                                 format;         /* pixel format of the drawable */
    int                                 interval;       /* last set surface swap interval */
    BOOL                                doublebuffer;   /* pixel format is double buffered */
    BOOL                                stereo;         /* pixel format is stereo buffered */
    EGLSurface                          surface;        /* surface for EGL based drivers */
    GLuint                              read_fbo;       /* default read FBO name when emulating framebuffer */
    GLuint                              draw_fbo;       /* default draw FBO name when emulating framebuffer */
};

static inline const char *debugstr_opengl_drawable( struct opengl_drawable *drawable )
{
    if (!drawable) return "(null)";
    return wine_dbg_sprintf( "%s/%p (format %u)", debugstr_client_surface( drawable->client ), drawable, drawable->format );
}

W32KAPI void *opengl_drawable_create( UINT size, const struct opengl_drawable_funcs *funcs, int format, struct client_surface *client );
W32KAPI void opengl_drawable_add_ref( struct opengl_drawable *drawable );
W32KAPI void opengl_drawable_release( struct opengl_drawable *drawable );

/* interface between win32u and the user drivers */
struct opengl_driver_funcs
{
    void (*p_init_egl_platform)(struct egl_platform*);
    void *(*p_get_proc_address)(const char *);
    UINT (*p_init_pixel_formats)(UINT*);
    BOOL (*p_describe_pixel_format)(int,struct wgl_pixel_format*);
    const char *(*p_init_wgl_extensions)(struct opengl_funcs *funcs);
    BOOL (*p_surface_create)( HWND hwnd, int format, struct opengl_drawable **drawable );
    BOOL (*p_context_create)( int format, void *share, const int *attribs, void **context );
    BOOL (*p_context_destroy)(void*);
    BOOL (*p_make_current)( struct opengl_drawable *draw, struct opengl_drawable *read, void *private );
    BOOL (*p_pbuffer_create)( HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                              GLint max_level, GLsizei *width, GLsizei *height, struct opengl_drawable **drawable );
    BOOL (*p_pbuffer_updated)( HDC hdc, struct opengl_drawable *drawable, GLenum cube_face, GLint mipmap_level );
    UINT (*p_pbuffer_bind)( HDC hdc, struct opengl_drawable *drawable, GLenum buffer );
};

#endif /* WINE_UNIX_LIB */

#endif /* __WINE_OPENGL_DRIVER_H */
