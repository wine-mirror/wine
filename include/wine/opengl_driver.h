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

/* Wine internal opengl driver version, needs to be bumped upon opengl_funcs changes. */
#define WINE_OPENGL_DRIVER_VERSION 36

struct wgl_context;
struct wgl_pbuffer;

/* interface between opengl32 and win32u */
struct opengl_funcs
{
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
    EGLDisplay  display;
    UINT        config_count;
    EGLConfig  *configs;
    BOOL        has_EGL_EXT_present_opaque;
    BOOL        has_EGL_EXT_pixel_format_float;
};

/* interface between win32u and the user drivers */
struct opengl_driver_funcs
{
    GLenum (*p_init_egl_platform)(const struct egl_platform*,EGLNativeDisplayType*);
    void *(*p_get_proc_address)(const char *);
    UINT (*p_init_pixel_formats)(UINT*);
    BOOL (*p_describe_pixel_format)(int,struct wgl_pixel_format*);
    const char *(*p_init_wgl_extensions)(struct opengl_funcs *funcs);
    BOOL (*p_set_pixel_format)(HWND,int,int,BOOL);
    BOOL (*p_swap_buffers)(void*,HWND,HDC,int);
    BOOL (*p_context_create)(HDC,int,void*,const int*,void**);
    BOOL (*p_context_destroy)(void*);
    BOOL (*p_context_flush)(void*,HWND,HDC,int,void(*)(void));
    BOOL (*p_context_make_current)(HDC,HDC,void*);
    BOOL (*p_pbuffer_create)(HDC,int,BOOL,GLenum,GLenum,GLint,GLsizei*,GLsizei*,void **);
    BOOL (*p_pbuffer_destroy)(HDC,void*);
    BOOL (*p_pbuffer_updated)(HDC,void*,GLenum,GLint);
    UINT (*p_pbuffer_bind)(HDC,void*,GLenum);
};

#endif /* WINE_UNIX_LIB */

#endif /* __WINE_OPENGL_DRIVER_H */
