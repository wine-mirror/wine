/*
 * DIB driver OpenGL support
 *
 * Copyright 2012 Alexandre Julliard
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

#include <sys/types.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "dibdrv.h"
#include "wine/wgl.h"
#include "wine/wgl_driver.h"

#ifdef SONAME_LIBOSMESA

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

#define OSMESA_COLOR_INDEX	GL_COLOR_INDEX
#define OSMESA_RGBA		GL_RGBA
#define OSMESA_BGRA		0x1
#define OSMESA_ARGB		0x2
#define OSMESA_RGB		GL_RGB
#define OSMESA_BGR		0x4
#define OSMESA_RGB_565		0x5
#define OSMESA_ROW_LENGTH	0x10
#define OSMESA_Y_UP		0x11

typedef struct osmesa_context *OSMesaContext;

struct wgl_context
{
    OSMesaContext context;
    UINT          format;
};

static struct opengl_funcs opengl_funcs;

#define USE_GL_FUNC(name) #name,
static const char *opengl_func_names[] = { ALL_WGL_FUNCS };
#undef USE_GL_FUNC

static OSMesaContext (*pOSMesaCreateContextExt)( GLenum format, GLint depthBits, GLint stencilBits,
                                                 GLint accumBits, OSMesaContext sharelist );
static void (*pOSMesaDestroyContext)( OSMesaContext ctx );
static void * (*pOSMesaGetProcAddress)( const char *funcName );
static GLboolean (*pOSMesaMakeCurrent)( OSMesaContext ctx, void *buffer, GLenum type,
                                        GLsizei width, GLsizei height );
static void (*pOSMesaPixelStore)( GLint pname, GLint value );

static BOOL init_opengl(void)
{
    static BOOL init_done = FALSE;
    static void *osmesa_handle;
    unsigned int i;

    if (init_done) return (osmesa_handle != NULL);
    init_done = TRUE;

    osmesa_handle = dlopen( SONAME_LIBOSMESA, RTLD_NOW );
    if (osmesa_handle == NULL)
    {
        ERR( "Failed to load OSMesa: %s\n", dlerror() );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) do if (!(p##f = dlsym( osmesa_handle, #f ))) \
    { \
        ERR( "%s not found in %s (%s), disabling.\n", #f, SONAME_LIBOSMESA, dlerror() ); \
        goto failed; \
    } while(0)

    LOAD_FUNCPTR(OSMesaCreateContextExt);
    LOAD_FUNCPTR(OSMesaDestroyContext);
    LOAD_FUNCPTR(OSMesaGetProcAddress);
    LOAD_FUNCPTR(OSMesaMakeCurrent);
    LOAD_FUNCPTR(OSMesaPixelStore);
#undef LOAD_FUNCPTR

    for (i = 0; i < ARRAY_SIZE( opengl_func_names ); i++)
    {
        if (!(((void **)&opengl_funcs.gl)[i] = pOSMesaGetProcAddress( opengl_func_names[i] )))
        {
            ERR( "%s not found in %s, disabling.\n", opengl_func_names[i], SONAME_LIBOSMESA );
            goto failed;
        }
    }

    return TRUE;

failed:
    dlclose( osmesa_handle );
    osmesa_handle = NULL;
    return FALSE;
}

/***********************************************************************
 *		osmesa_get_gl_funcs
 */
static void osmesa_get_gl_funcs( struct opengl_funcs *funcs )
{
    funcs->gl = opengl_funcs.gl;
}

/***********************************************************************
 *		osmesa_create_context
 */
static struct wgl_context * osmesa_create_context( HDC hdc, const PIXELFORMATDESCRIPTOR *descr )
{
    struct wgl_context *context;
    UINT gl_format;

    switch (descr->cColorBits)
    {
    case 32:
        if (descr->cRedShift == 8) gl_format = OSMESA_ARGB;
        else if (descr->cRedShift == 16) gl_format = OSMESA_BGRA;
        else gl_format = OSMESA_RGBA;
        break;
    case 24:
        gl_format = descr->cRedShift == 16 ? OSMESA_BGR : OSMESA_RGB;
        break;
    case 16:
        gl_format = OSMESA_RGB_565;
        break;
    default:
        return NULL;
    }
    if (!(context = malloc( sizeof( *context )))) return NULL;
    context->format = gl_format;
    if (!(context->context = pOSMesaCreateContextExt( gl_format, descr->cDepthBits, descr->cStencilBits,
                                                      descr->cAccumBits, 0 )))
    {
        free( context );
        return NULL;
    }
    return context;
}

/***********************************************************************
 *		osmesa_delete_context
 */
static BOOL osmesa_delete_context( struct wgl_context *context )
{
    pOSMesaDestroyContext( context->context );
    free( context );
    return TRUE;
}

/***********************************************************************
 *		osmesa_get_proc_address
 */
static PROC osmesa_get_proc_address( const char *proc )
{
    return (PROC)pOSMesaGetProcAddress( proc );
}

/***********************************************************************
 *		osmesa_make_current
 */
static BOOL osmesa_make_current( struct wgl_context *context, void *bits,
                                 int width, int height, int bpp, int stride )
{
    BOOL ret;
    GLenum type;

    if (!context)
    {
        pOSMesaMakeCurrent( NULL, NULL, GL_UNSIGNED_BYTE, 0, 0 );
        return TRUE;
    }

    type = context->format == OSMESA_RGB_565 ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE;
    ret = pOSMesaMakeCurrent( context->context, bits, type, width, height );
    if (ret)
    {
        pOSMesaPixelStore( OSMESA_ROW_LENGTH, abs( stride ) * 8 / bpp );
        pOSMesaPixelStore( OSMESA_Y_UP, 1 );  /* Windows seems to assume bottom-up */
    }
    return ret;
}

static const struct osmesa_funcs osmesa_funcs =
{
    osmesa_get_gl_funcs,
    osmesa_create_context,
    osmesa_delete_context,
    osmesa_get_proc_address,
    osmesa_make_current
};

const struct osmesa_funcs *init_opengl_lib(void)
{
    if (!init_opengl()) return NULL;
    return &osmesa_funcs;
}

#else  /* SONAME_LIBOSMESA */

const struct osmesa_funcs *init_opengl_lib(void)
{
    return NULL;
}

#endif  /* SONAME_LIBOSMESA */
