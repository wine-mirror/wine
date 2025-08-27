/*
 * X11DRV OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
 * Copyright 2005 Alex Woods
 * Copyright 2005 Raphael Junqueira
 * Copyright 2006-2009 Roderick Colenbrander
 * Copyright 2006 Tomas Carnecky
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "x11drv.h"
#include "xcomposite.h"
#include "winternl.h"
#include "wine/debug.h"

#ifdef SONAME_LIBGL

WINE_DEFAULT_DEBUG_CHANNEL(wgl);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#include "wine/opengl_driver.h"

typedef struct __GLXcontextRec *GLXContext;
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef XID GLXFBConfigID;
typedef XID GLXContextID;
typedef XID GLXWindow;
typedef XID GLXPbuffer;

#define GLX_USE_GL                        1
#define GLX_BUFFER_SIZE                   2
#define GLX_LEVEL                         3
#define GLX_RGBA                          4
#define GLX_DOUBLEBUFFER                  5
#define GLX_STEREO                        6
#define GLX_AUX_BUFFERS                   7
#define GLX_RED_SIZE                      8
#define GLX_GREEN_SIZE                    9
#define GLX_BLUE_SIZE                     10
#define GLX_ALPHA_SIZE                    11
#define GLX_DEPTH_SIZE                    12
#define GLX_STENCIL_SIZE                  13
#define GLX_ACCUM_RED_SIZE                14
#define GLX_ACCUM_GREEN_SIZE              15
#define GLX_ACCUM_BLUE_SIZE               16
#define GLX_ACCUM_ALPHA_SIZE              17

#define GLX_BAD_SCREEN                    1
#define GLX_BAD_ATTRIBUTE                 2
#define GLX_NO_EXTENSION                  3
#define GLX_BAD_VISUAL                    4
#define GLX_BAD_CONTEXT                   5
#define GLX_BAD_VALUE                     6
#define GLX_BAD_ENUM                      7

#define GLX_VENDOR                        1
#define GLX_VERSION                       2
#define GLX_EXTENSIONS                    3

#define GLX_CONFIG_CAVEAT                 0x20
#define GLX_DONT_CARE                     0xFFFFFFFF
#define GLX_X_VISUAL_TYPE                 0x22
#define GLX_TRANSPARENT_TYPE              0x23
#define GLX_TRANSPARENT_INDEX_VALUE       0x24
#define GLX_TRANSPARENT_RED_VALUE         0x25
#define GLX_TRANSPARENT_GREEN_VALUE       0x26
#define GLX_TRANSPARENT_BLUE_VALUE        0x27
#define GLX_TRANSPARENT_ALPHA_VALUE       0x28
#define GLX_WINDOW_BIT                    0x00000001
#define GLX_PIXMAP_BIT                    0x00000002
#define GLX_PBUFFER_BIT                   0x00000004
#define GLX_AUX_BUFFERS_BIT               0x00000010
#define GLX_FRONT_LEFT_BUFFER_BIT         0x00000001
#define GLX_FRONT_RIGHT_BUFFER_BIT        0x00000002
#define GLX_BACK_LEFT_BUFFER_BIT          0x00000004
#define GLX_BACK_RIGHT_BUFFER_BIT         0x00000008
#define GLX_DEPTH_BUFFER_BIT              0x00000020
#define GLX_STENCIL_BUFFER_BIT            0x00000040
#define GLX_ACCUM_BUFFER_BIT              0x00000080
#define GLX_NONE                          0x8000
#define GLX_SLOW_CONFIG                   0x8001
#define GLX_TRUE_COLOR                    0x8002
#define GLX_DIRECT_COLOR                  0x8003
#define GLX_PSEUDO_COLOR                  0x8004
#define GLX_STATIC_COLOR                  0x8005
#define GLX_GRAY_SCALE                    0x8006
#define GLX_STATIC_GRAY                   0x8007
#define GLX_TRANSPARENT_RGB               0x8008
#define GLX_TRANSPARENT_INDEX             0x8009
#define GLX_VISUAL_ID                     0x800B
#define GLX_SCREEN                        0x800C
#define GLX_NON_CONFORMANT_CONFIG         0x800D
#define GLX_DRAWABLE_TYPE                 0x8010
#define GLX_RENDER_TYPE                   0x8011
#define GLX_X_RENDERABLE                  0x8012
#define GLX_FBCONFIG_ID                   0x8013
#define GLX_RGBA_TYPE                     0x8014
#define GLX_COLOR_INDEX_TYPE              0x8015
#define GLX_MAX_PBUFFER_WIDTH             0x8016
#define GLX_MAX_PBUFFER_HEIGHT            0x8017
#define GLX_MAX_PBUFFER_PIXELS            0x8018
#define GLX_PRESERVED_CONTENTS            0x801B
#define GLX_LARGEST_PBUFFER               0x801C
#define GLX_WIDTH                         0x801D
#define GLX_HEIGHT                        0x801E
#define GLX_EVENT_MASK                    0x801F
#define GLX_DAMAGED                       0x8020
#define GLX_SAVED                         0x8021
#define GLX_WINDOW                        0x8022
#define GLX_PBUFFER                       0x8023
#define GLX_PBUFFER_HEIGHT                0x8040
#define GLX_PBUFFER_WIDTH                 0x8041
#define GLX_SWAP_METHOD_OML               0x8060
#define GLX_SWAP_EXCHANGE_OML             0x8061
#define GLX_SWAP_COPY_OML                 0x8062
#define GLX_SWAP_UNDEFINED_OML            0x8063
#define GLX_RGBA_BIT                      0x00000001
#define GLX_COLOR_INDEX_BIT               0x00000002
#define GLX_PBUFFER_CLOBBER_MASK          0x08000000

/** GLX_ARB_multisample */
#define GLX_SAMPLE_BUFFERS_ARB            100000
#define GLX_SAMPLES_ARB                   100001
/** GLX_ARB_framebuffer_sRGB */
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT  0x20B2
/** GLX_EXT_fbconfig_packed_float */
#define GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT  0x20B1
#define GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT   0x00000008
/** GLX_ARB_create_context */
#define GLX_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB     0x2092
#define GLX_CONTEXT_FLAGS_ARB             0x2094
/** GLX_ARB_create_context_no_error */
#define GLX_CONTEXT_OPENGL_NO_ERROR_ARB   0x31B3
/** GLX_ARB_create_context_profile */
#define GLX_CONTEXT_PROFILE_MASK_ARB      0x9126
/** GLX_ATI_pixel_format_float */
#define GLX_RGBA_FLOAT_ATI_BIT            0x00000100
/** GLX_ARB_pixel_format_float */
#define GLX_RGBA_FLOAT_BIT                0x00000004
#define GLX_RGBA_FLOAT_TYPE               0x20B9
/** GLX_MESA_query_renderer */
#define GLX_RENDERER_ID_MESA              0x818E
/** GLX_NV_float_buffer */
#define GLX_FLOAT_COMPONENTS_NV           0x20B0


static const char *glExtensions;
static const char *glxExtensions;
static char wglExtensions[4096];
static int glxVersion[2];
static int glx_opcode;

struct glx_pixel_format
{
    GLXFBConfig fbconfig;
    XVisualInfo *visual;
    int         fmt_id;
    int         render_type;
    DWORD       dwFlags; /* We store some PFD_* flags in here for emulated bitmap formats */
};

struct gl_drawable
{
    struct opengl_drawable         base;
    GLXDrawable                    drawable;     /* drawable for rendering with GL */
    Colormap                       colormap;     /* colormap for the client window */
};

static struct gl_drawable *impl_from_opengl_drawable( struct opengl_drawable *base )
{
    return CONTAINING_RECORD( base, struct gl_drawable, base );
}

enum glx_swap_control_method
{
    GLX_SWAP_CONTROL_NONE,
    GLX_SWAP_CONTROL_EXT,
    GLX_SWAP_CONTROL_SGI,
    GLX_SWAP_CONTROL_MESA
};

static struct glx_pixel_format *pixel_formats;
static int nb_pixel_formats, nb_onscreen_formats;
static const struct egl_platform *egl;

/* Selects the preferred GLX swap control method for use by wglSwapIntervalEXT */
static enum glx_swap_control_method swap_control_method = GLX_SWAP_CONTROL_NONE;
/* Set when GLX_EXT_swap_control_tear is supported, requires GLX_SWAP_CONTROL_EXT */
static BOOL has_swap_control_tear = FALSE;
static BOOL has_swap_method = FALSE;

static const BOOL is_win64 = sizeof(void *) > sizeof(int);

static BOOL glxRequireVersion(int requiredVersion);

static void dump_PIXELFORMATDESCRIPTOR(const PIXELFORMATDESCRIPTOR *ppfd) {
  TRACE( "size %u version %u flags %u type %u color %u %u,%u,%u,%u "
         "accum %u depth %u stencil %u aux %u ",
         ppfd->nSize, ppfd->nVersion, ppfd->dwFlags, ppfd->iPixelType,
         ppfd->cColorBits, ppfd->cRedBits, ppfd->cGreenBits, ppfd->cBlueBits, ppfd->cAlphaBits,
         ppfd->cAccumBits, ppfd->cDepthBits, ppfd->cStencilBits, ppfd->cAuxBuffers );
#define TEST_AND_DUMP(t,tv) if ((t) & (tv)) TRACE(#tv " ")
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DEPTH_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DOUBLEBUFFER);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DOUBLEBUFFER_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DRAW_TO_WINDOW);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DRAW_TO_BITMAP);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_GENERIC_ACCELERATED);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_GENERIC_FORMAT);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_NEED_PALETTE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_NEED_SYSTEM_PALETTE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_STEREO);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_STEREO_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SUPPORT_GDI);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SUPPORT_OPENGL);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_COPY);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_EXCHANGE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_LAYER_BUFFERS);
  /* PFD_SUPPORT_COMPOSITION is new in Vista, it is similar to composition
   * under X e.g. COMPOSITE + GLX_EXT_TEXTURE_FROM_PIXMAP. */
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SUPPORT_COMPOSITION);
#undef TEST_AND_DUMP
  TRACE("\n");
}

/* GLX 1.0 */
static XVisualInfo* (*pglXChooseVisual)( Display *dpy, int screen, int *attribList );
static GLXContext (*pglXCreateContext)( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
static void (*pglXDestroyContext)( Display *dpy, GLXContext ctx );
static Bool (*pglXMakeCurrent)( Display *dpy, GLXDrawable drawable, GLXContext ctx);
static void (*pglXCopyContext)( Display *dpy, GLXContext src, GLXContext dst, unsigned long mask );
static void (*pglXSwapBuffers)( Display *dpy, GLXDrawable drawable );
static Bool (*pglXQueryVersion)( Display *dpy, int *maj, int *min );
static Bool (*pglXIsDirect)( Display *dpy, GLXContext ctx );
static GLXContext (*pglXGetCurrentContext)( void );
static GLXDrawable (*pglXGetCurrentDrawable)( void );

/* GLX 1.1 */
static const char *(*pglXQueryExtensionsString)( Display *dpy, int screen );
static const char *(*pglXQueryServerString)( Display *dpy, int screen, int name );
static const char *(*pglXGetClientString)( Display *dpy, int name );

/* GLX 1.3 */
static GLXFBConfig *(*pglXChooseFBConfig)( Display *dpy, int screen, const int *attribList, int *nitems );
static int (*pglXGetFBConfigAttrib)( Display *dpy, GLXFBConfig config, int attribute, int *value );
static GLXFBConfig *(*pglXGetFBConfigs)( Display *dpy, int screen, int *nelements );
static XVisualInfo *(*pglXGetVisualFromFBConfig)( Display *dpy, GLXFBConfig config );
static GLXPbuffer (*pglXCreatePbuffer)( Display *dpy, GLXFBConfig config, const int *attribList );
static void (*pglXDestroyPbuffer)( Display *dpy, GLXPbuffer pbuf );
static void (*pglXQueryDrawable)( Display *dpy, GLXDrawable draw, int attribute, unsigned int *value );
static GLXContext (*pglXCreateNewContext)( Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct );
static Bool (*pglXMakeContextCurrent)( Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx );
static GLXPixmap (*pglXCreatePixmap)( Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list );
static void (*pglXDestroyPixmap)( Display *dpy, GLXPixmap pixmap );
static GLXWindow (*pglXCreateWindow)( Display *dpy, GLXFBConfig config, Window win, const int *attrib_list );
static void (*pglXDestroyWindow)( Display *dpy, GLXWindow win );

/* GLX Extensions */
static GLXContext (*pglXCreateContextAttribsARB)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
static void* (*pglXGetProcAddressARB)(const GLubyte *);
static void (*pglXSwapIntervalEXT)(Display *dpy, GLXDrawable drawable, int interval);
static int   (*pglXSwapIntervalSGI)(int);

/* NV GLX Extension */
static void* (*pglXAllocateMemoryNV)(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
static void  (*pglXFreeMemoryNV)(GLvoid *pointer);

/* MESA GLX Extensions */
static void (*pglXCopySubBufferMESA)(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height);
static int (*pglXSwapIntervalMESA)(unsigned int interval);
static Bool (*pglXQueryCurrentRendererIntegerMESA)(int attribute, unsigned int *value);
static const char *(*pglXQueryCurrentRendererStringMESA)(int attribute);
static Bool (*pglXQueryRendererIntegerMESA)(Display *dpy, int screen, int renderer, int attribute, unsigned int *value);
static const char *(*pglXQueryRendererStringMESA)(Display *dpy, int screen, int renderer, int attribute);

/* OpenML GLX Extensions */
static Bool (*pglXWaitForSbcOML)( Display *dpy, GLXDrawable drawable,
        INT64 target_sbc, INT64 *ust, INT64 *msc, INT64 *sbc );
static INT64 (*pglXSwapBuffersMscOML)( Display *dpy, GLXDrawable drawable,
        INT64 target_msc, INT64 divisor, INT64 remainder );

/* Standard OpenGL */
static const GLubyte *(*pglGetString)(GLenum name);

static void *opengl_handle;
static const struct opengl_funcs *funcs;
static struct opengl_driver_funcs x11drv_driver_funcs;
static const struct opengl_drawable_funcs x11drv_surface_funcs;
static const struct opengl_drawable_funcs x11drv_pbuffer_funcs;
static const struct opengl_drawable_funcs x11drv_egl_surface_funcs;

/* check if the extension is present in the list */
static BOOL has_extension( const char *list, const char *ext )
{
    size_t len = strlen( ext );

    while (list)
    {
        while (*list == ' ') list++;
        if (!strncmp( list, ext, len ) && (!list[len] || list[len] == ' ')) return TRUE;
        list = strchr( list, ' ' );
    }
    return FALSE;
}

static int GLXErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    /* In the future we might want to find the exact X or GLX error to report back to the app */
    if (event->request_code != glx_opcode)
        return 0;
    return 1;
}

static BOOL X11DRV_WineGL_InitOpenglInfo(void)
{
    int screen = DefaultScreen(gdi_display);
    Window win = 0, root = 0;
    const char *gl_version;
    const char *gl_renderer;
    BOOL glx_direct;
    XVisualInfo *vis;
    GLXContext ctx = NULL;
    XSetWindowAttributes attr;
    BOOL ret = FALSE;
    int attribList[] = {GLX_RGBA, GLX_DOUBLEBUFFER, None};

    attr.override_redirect = True;
    attr.colormap = None;
    attr.border_pixel = 0;

    vis = pglXChooseVisual(gdi_display, screen, attribList);
    if (vis) {
#ifdef __i386__
        WORD old_fs, new_fs;
        __asm__( "mov %%fs,%0" : "=r" (old_fs) );
        /* Create a GLX Context. Without one we can't query GL information */
        ctx = pglXCreateContext(gdi_display, vis, None, GL_TRUE);
        __asm__( "mov %%fs,%0" : "=r" (new_fs) );
        __asm__( "mov %0,%%fs" :: "r" (old_fs) );
        if (old_fs != new_fs)
        {
            ERR( "%%fs register corrupted, probably broken ATI driver, disabling OpenGL.\n" );
            ERR( "You need to set the \"UseFastTls\" option to \"2\" in your X config file.\n" );
            goto done;
        }
#else
        ctx = pglXCreateContext(gdi_display, vis, None, GL_TRUE);
#endif
    }
    if (!ctx) goto done;

    root = RootWindow( gdi_display, vis->screen );
    if (vis->visual != DefaultVisual( gdi_display, vis->screen ))
        attr.colormap = XCreateColormap( gdi_display, root, vis->visual, AllocNone );
    if ((win = XCreateWindow( gdi_display, root, -1, -1, 1, 1, 0, vis->depth, InputOutput,
                              vis->visual, CWBorderPixel | CWOverrideRedirect | CWColormap, &attr )))
        XMapWindow( gdi_display, win );
    else
        win = root;

    if(pglXMakeCurrent(gdi_display, win, ctx) == 0)
    {
        ERR_(winediag)( "Unable to activate OpenGL context, most likely your %s OpenGL drivers haven't been "
                        "installed correctly\n", is_win64 ? "64-bit" : "32-bit" );
        goto done;
    }
    gl_renderer = (const char *)pglGetString(GL_RENDERER);
    gl_version  = (const char *)pglGetString(GL_VERSION);
    glExtensions = (const char *) pglGetString(GL_EXTENSIONS);

    /* Get the common GLX version supported by GLX client and server ( major/minor) */
    pglXQueryVersion(gdi_display, &glxVersion[0], &glxVersion[1]);

    glxExtensions = pglXQueryExtensionsString(gdi_display, screen);
    glx_direct = pglXIsDirect(gdi_display, ctx);

    TRACE("GL version             : %s.\n", gl_version);
    TRACE("GL renderer            : %s.\n", gl_renderer);
    TRACE("GLX version            : %d.%d.\n", glxVersion[0], glxVersion[1]);
    TRACE("Server GLX version     : %s.\n", pglXQueryServerString(gdi_display, screen, GLX_VERSION));
    TRACE("Server GLX vendor:     : %s.\n", pglXQueryServerString(gdi_display, screen, GLX_VENDOR));
    TRACE("Client GLX version     : %s.\n", pglXGetClientString(gdi_display, GLX_VERSION));
    TRACE("Client GLX vendor:     : %s.\n", pglXGetClientString(gdi_display, GLX_VENDOR));
    TRACE("Direct rendering enabled: %s\n", glx_direct ? "True" : "False");

    if(!glx_direct)
    {
        int fd = ConnectionNumber(gdi_display);
        struct sockaddr_un uaddr;
        unsigned int uaddrlen = sizeof(struct sockaddr_un);

        /* In general indirect rendering on a local X11 server indicates a driver problem.
         * Detect a local X11 server by checking whether the X11 socket is a Unix socket.
         */
        if(!getsockname(fd, (struct sockaddr *)&uaddr, &uaddrlen) && uaddr.sun_family == AF_UNIX)
            ERR_(winediag)("Direct rendering is disabled, most likely your %s OpenGL drivers "
                           "haven't been installed correctly (using GL renderer %s, version %s).\n",
                           is_win64 ? "64-bit" : "32-bit", debugstr_a(gl_renderer),
                           debugstr_a(gl_version));
    }
    else
    {
        /* In general you would expect that if direct rendering is returned, that you receive hardware
         * accelerated OpenGL rendering. The definition of direct rendering is that rendering is performed
         * client side without sending all GL commands to X using the GLX protocol. When Mesa falls back to
         * software rendering, it shows direct rendering.
         *
         * Depending on the cause of software rendering a different rendering string is shown. In case Mesa fails
         * to load a DRI module 'Software Rasterizer' is returned. When Mesa is compiled as a OpenGL reference driver
         * it shows 'Mesa X11'.
         */
        if(!strcmp(gl_renderer, "Software Rasterizer") || !strcmp(gl_renderer, "Mesa X11"))
            ERR_(winediag)("The Mesa OpenGL driver is using software rendering, most likely your %s OpenGL "
                           "drivers haven't been installed correctly (using GL renderer %s, version %s).\n",
                           is_win64 ? "64-bit" : "32-bit", debugstr_a(gl_renderer),
                           debugstr_a(gl_version));
    }
    ret = TRUE;

done:
    if(vis) XFree(vis);
    if(ctx) {
        pglXMakeCurrent(gdi_display, None, NULL);    
        pglXDestroyContext(gdi_display, ctx);
    }
    if (win != root) XDestroyWindow( gdi_display, win );
    if (attr.colormap) XFreeColormap( gdi_display, attr.colormap );
    if (!ret) ERR(" couldn't initialize OpenGL, expect problems\n");
    return ret;
}

static void x11drv_init_egl_platform( struct egl_platform *platform )
{
    platform->type = EGL_PLATFORM_X11_KHR;
    platform->native_display = gdi_display;
    egl = platform;
}

static inline EGLConfig egl_config_for_format(int format)
{
    assert(format > 0 && format <= 2 * egl->config_count);
    if (format <= egl->config_count) return egl->configs[format - 1];
    return egl->configs[format - egl->config_count - 1];
}

static BOOL x11drv_egl_surface_create( HWND hwnd, int format, struct opengl_drawable **drawable )
{
    struct opengl_drawable *previous;
    struct client_surface *client;
    struct gl_drawable *gl;
    Window window;
    RECT rect;

    if ((previous = *drawable) && previous->format == format) return TRUE;
    NtUserGetClientRect( hwnd, &rect, NtUserGetDpiForWindow( hwnd ) );

    if (!(window = x11drv_client_surface_create( hwnd, &default_visual, default_colormap, &client ))) return FALSE;
    gl = opengl_drawable_create( sizeof(*gl), &x11drv_egl_surface_funcs, format, client );
    client_surface_release( client );
    if (!gl) return FALSE;

    if (!(gl->base.surface = funcs->p_eglCreateWindowSurface( egl->display, egl_config_for_format( format ),
                                                              (void *)window, NULL )))
    {
        opengl_drawable_release( &gl->base );
        return FALSE;
    }

    TRACE( "Created drawable %s with client window %lx\n", debugstr_opengl_drawable( &gl->base ), window );
    XFlush( gdi_display );

    if (previous) opengl_drawable_release( previous );
    *drawable = &gl->base;
    return TRUE;
}

/**********************************************************************
 *           X11DRV_OpenglInit
 */
UINT X11DRV_OpenGLInit( UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs )
{
    int error_base, event_base;

    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but driver has %u\n", version, WINE_OPENGL_DRIVER_VERSION );
        return STATUS_INVALID_PARAMETER;
    }
    funcs = opengl_funcs;

    if (use_egl)
    {
        if (!opengl_funcs->egl_handle) return STATUS_NOT_SUPPORTED;
        WARN( "Using experimental EGL OpenGL backend\n" );
        x11drv_driver_funcs = **driver_funcs;
        x11drv_driver_funcs.p_init_egl_platform = x11drv_init_egl_platform;
        x11drv_driver_funcs.p_surface_create = x11drv_egl_surface_create;
        *driver_funcs = &x11drv_driver_funcs;
        return STATUS_SUCCESS;
    }

    /* No need to load any other libraries as according to the ABI, libGL should be self-sufficient
       and include all dependencies */
    opengl_handle = dlopen( SONAME_LIBGL, RTLD_NOW | RTLD_GLOBAL );
    if (opengl_handle == NULL)
    {
        ERR( "Failed to load libGL: %s\n", dlerror() );
        ERR( "OpenGL support is disabled.\n");
        return STATUS_NOT_SUPPORTED;
    }

    /* redirect some standard OpenGL functions */
#define LOAD_FUNCPTR(func) \
        if (!(p##func = dlsym( opengl_handle, #func ))) \
        { \
            ERR( "%s not found in libGL, disabling OpenGL.\n", #func ); \
            goto failed; \
        }
    LOAD_FUNCPTR( glGetString );
#undef LOAD_FUNCPTR

    pglXGetProcAddressARB = dlsym(opengl_handle, "glXGetProcAddressARB");
    if (pglXGetProcAddressARB == NULL) {
        ERR("Could not find glXGetProcAddressARB in libGL, disabling OpenGL.\n");
        goto failed;
    }

#define LOAD_FUNCPTR(f) do if((p##f = (void*)pglXGetProcAddressARB((const unsigned char*)#f)) == NULL) \
    { \
        ERR( "%s not found in libGL, disabling OpenGL.\n", #f ); \
        goto failed; \
    } while(0)

    /* GLX 1.0 */
    LOAD_FUNCPTR(glXChooseVisual);
    LOAD_FUNCPTR(glXCopyContext);
    LOAD_FUNCPTR(glXCreateContext);
    LOAD_FUNCPTR(glXGetCurrentContext);
    LOAD_FUNCPTR(glXGetCurrentDrawable);
    LOAD_FUNCPTR(glXDestroyContext);
    LOAD_FUNCPTR(glXIsDirect);
    LOAD_FUNCPTR(glXMakeCurrent);
    LOAD_FUNCPTR(glXSwapBuffers);
    LOAD_FUNCPTR(glXQueryVersion);

    /* GLX 1.1 */
    LOAD_FUNCPTR(glXGetClientString);
    LOAD_FUNCPTR(glXQueryExtensionsString);
    LOAD_FUNCPTR(glXQueryServerString);

    /* GLX 1.3 */
    LOAD_FUNCPTR(glXCreatePbuffer);
    LOAD_FUNCPTR(glXCreateNewContext);
    LOAD_FUNCPTR(glXDestroyPbuffer);
    LOAD_FUNCPTR(glXMakeContextCurrent);
    LOAD_FUNCPTR(glXGetFBConfigs);
    LOAD_FUNCPTR(glXCreatePixmap);
    LOAD_FUNCPTR(glXDestroyPixmap);
    LOAD_FUNCPTR(glXCreateWindow);
    LOAD_FUNCPTR(glXDestroyWindow);
#undef LOAD_FUNCPTR

/* It doesn't matter if these fail. They'll only be used if the driver reports
   the associated extension is available (and if a driver reports the extension
   is available but fails to provide the functions, it's quite broken) */
#define LOAD_FUNCPTR(f) p##f = pglXGetProcAddressARB((const GLubyte *)#f)
    /* ARB GLX Extension */
    LOAD_FUNCPTR(glXCreateContextAttribsARB);
    /* EXT GLX Extension */
    LOAD_FUNCPTR(glXSwapIntervalEXT);
    /* MESA GLX Extension */
    LOAD_FUNCPTR(glXSwapIntervalMESA);
    /* SGI GLX Extension */
    LOAD_FUNCPTR(glXSwapIntervalSGI);
    /* NV GLX Extension */
    LOAD_FUNCPTR(glXAllocateMemoryNV);
    LOAD_FUNCPTR(glXFreeMemoryNV);
#undef LOAD_FUNCPTR

    if(!X11DRV_WineGL_InitOpenglInfo()) goto failed;

    if (XQueryExtension( gdi_display, "GLX", &glx_opcode, &event_base, &error_base ))
    {
        TRACE("GLX is up and running error_base = %d\n", error_base);
    } else {
        ERR( "GLX extension is missing, disabling OpenGL.\n" );
        goto failed;
    }

    /* In case of GLX you have direct and indirect rendering. Most of the time direct rendering is used
     * as in general only that is hardware accelerated. In some cases like in case of remote X indirect
     * rendering is used.
     *
     * The main problem for our OpenGL code is that we need certain GLX calls but their presence
     * depends on the reported GLX client / server version and on the client / server extension list.
     * Those don't have to be the same.
     *
     * In general the server GLX information lists the capabilities in case of indirect rendering.
     * When direct rendering is used, the OpenGL client library is responsible for which GLX calls are
     * available and in that case the client GLX informat can be used.
     * OpenGL programs should use the 'intersection' of both sets of information which is advertised
     * in the GLX version/extension list. When a program does this it works for certain for both
     * direct and indirect rendering.
     *
     * The problem we are having in this area is that ATI's Linux drivers are broken. For some reason
     * they haven't added some very important GLX extensions like GLX_SGIX_fbconfig to their client
     * extension list which causes this extension not to be listed. (Wine requires this extension).
     * ATI advertises a GLX client version of 1.3 which implies that this fbconfig extension among
     * pbuffers is around.
     *
     * In order to provide users of Ati's proprietary drivers with OpenGL support, we need to detect
     * the ATI drivers and from then on use GLX client information for them.
     */

    if(glxRequireVersion(3)) {
        pglXChooseFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfig");
        pglXGetFBConfigAttrib = pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttrib");
        pglXGetVisualFromFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfig");
        pglXQueryDrawable = pglXGetProcAddressARB((const GLubyte *) "glXQueryDrawable");
    } else if (has_extension( glxExtensions, "GLX_SGIX_fbconfig")) {
        pglXChooseFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfigSGIX");
        pglXGetFBConfigAttrib = pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttribSGIX");
        pglXGetVisualFromFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfigSGIX");

        /* The mesa libGL client library seems to forward glXQueryDrawable to the Xserver, so only
         * enable this function when the Xserver understand GLX 1.3 or newer
         */
        pglXQueryDrawable = NULL;
    } else if(strcmp("ATI", pglXGetClientString(gdi_display, GLX_VENDOR)) == 0) {
        TRACE("Overriding ATI GLX capabilities!\n");
        pglXChooseFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfig");
        pglXGetFBConfigAttrib = pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttrib");
        pglXGetVisualFromFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfig");
        pglXQueryDrawable = pglXGetProcAddressARB((const GLubyte *) "glXQueryDrawable");

        /* Use client GLX information in case of the ATI drivers. We override the
         * capabilities over here and not somewhere else as ATI might better their
         * life in the future. In case they release proper drivers this block of
         * code won't be called. */
        glxExtensions = pglXGetClientString(gdi_display, GLX_EXTENSIONS);
    } else {
        ERR(" glx_version is %s and GLX_SGIX_fbconfig extension is unsupported. Expect problems.\n",
            pglXQueryServerString(gdi_display, DefaultScreen(gdi_display), GLX_VERSION));
    }

    if (has_extension( glxExtensions, "GLX_MESA_copy_sub_buffer")) {
        pglXCopySubBufferMESA = pglXGetProcAddressARB((const GLubyte *) "glXCopySubBufferMESA");
    }

    if (has_extension( glxExtensions, "GLX_MESA_query_renderer" ))
    {
        pglXQueryCurrentRendererIntegerMESA = pglXGetProcAddressARB(
                (const GLubyte *)"glXQueryCurrentRendererIntegerMESA" );
        pglXQueryCurrentRendererStringMESA = pglXGetProcAddressARB(
                (const GLubyte *)"glXQueryCurrentRendererStringMESA" );
        pglXQueryRendererIntegerMESA = pglXGetProcAddressARB( (const GLubyte *)"glXQueryRendererIntegerMESA" );
        pglXQueryRendererStringMESA = pglXGetProcAddressARB( (const GLubyte *)"glXQueryRendererStringMESA" );
    }

    if (has_extension( glxExtensions, "GLX_OML_sync_control" ))
    {
        pglXWaitForSbcOML = pglXGetProcAddressARB( (const GLubyte *)"glXWaitForSbcOML" );
        pglXSwapBuffersMscOML = pglXGetProcAddressARB( (const GLubyte *)"glXSwapBuffersMscOML" );
    }

    *driver_funcs = &x11drv_driver_funcs;
    return STATUS_SUCCESS;

failed:
    dlclose(opengl_handle);
    opengl_handle = NULL;
    return STATUS_NOT_SUPPORTED;
}

static int get_render_type_from_fbconfig(Display *display, GLXFBConfig fbconfig)
{
    int render_type, render_type_bit;
    pglXGetFBConfigAttrib(display, fbconfig, GLX_RENDER_TYPE, &render_type_bit);
    switch(render_type_bit)
    {
        case GLX_RGBA_BIT:
            render_type = GLX_RGBA_TYPE;
            break;
        case GLX_COLOR_INDEX_BIT:
            render_type = GLX_COLOR_INDEX_TYPE;
            break;
        case GLX_RGBA_FLOAT_BIT:
            render_type = GLX_RGBA_FLOAT_TYPE;
            break;
        case GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT:
            render_type = GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT;
            break;
        default:
            ERR("Unknown render_type: %x\n", render_type_bit);
            render_type = 0;
    }
    return render_type;
}

/* Check whether a fbconfig is suitable for Windows-style bitmap rendering */
static BOOL check_fbconfig_bitmap_capability( GLXFBConfig fbconfig, const XVisualInfo *vis )
{
    int dbuf, value;

    pglXGetFBConfigAttrib( gdi_display, fbconfig, GLX_BUFFER_SIZE, &value );
    if (vis && value != vis->depth) return FALSE;

    pglXGetFBConfigAttrib( gdi_display, fbconfig, GLX_DOUBLEBUFFER, &dbuf );
    pglXGetFBConfigAttrib(gdi_display, fbconfig, GLX_DRAWABLE_TYPE, &value);

    /* Windows only supports bitmap rendering on single buffered formats, further the fbconfig needs to have
     * the GLX_PIXMAP_BIT set. */
    return !dbuf && (value & GLX_PIXMAP_BIT);
}

static UINT x11drv_init_pixel_formats( UINT *onscreen_count )
{
    struct glx_pixel_format *list;
    int size = 0, onscreen_size = 0;
    int fmt_id, nCfgs, i, run;
    GLXFBConfig* cfgs;
    XVisualInfo *visinfo;

    cfgs = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs);
    if (NULL == cfgs || 0 == nCfgs) {
        if(cfgs != NULL) XFree(cfgs);
        ERR("glXChooseFBConfig returns NULL\n");
        return 0;
    }

    list = calloc( 1, (nCfgs * 2) * sizeof(*list) );

    /* Fill the pixel format list. Put onscreen formats at the top and offscreen ones at the bottom.
     * Do this as GLX doesn't guarantee that the list is sorted */
    for(run=0; run < 2; run++)
    {
        for(i=0; i<nCfgs; i++) {
            pglXGetFBConfigAttrib(gdi_display, cfgs[i], GLX_FBCONFIG_ID, &fmt_id);
            visinfo = pglXGetVisualFromFBConfig(gdi_display, cfgs[i]);

            /* The first run we only add onscreen formats (ones which have an associated X Visual).
             * The second run we only set offscreen formats. */
            if(!run && visinfo)
            {
                /* We implement child window rendering using offscreen buffers (using composite or an XPixmap).
                 * The contents is copied to the destination using XCopyArea. For the copying to work
                 * the depth of the source and destination window should be the same. In general this should
                 * not be a problem for OpenGL as drivers only advertise formats with a similar depth (or no depth).
                 * As of the introduction of composition managers at least Nvidia now also offers ARGB visuals
                 * with a depth of 32 in addition to the default 24 bit. In order to prevent BadMatch errors we only
                 * list formats with the same depth. */
                if(visinfo->depth != default_visual.depth)
                {
                    XFree(visinfo);
                    continue;
                }

                TRACE("Found onscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                list[size].fbconfig = cfgs[i];
                list[size].visual = visinfo;
                list[size].fmt_id = fmt_id;
                list[size].render_type = get_render_type_from_fbconfig(gdi_display, cfgs[i]);
                list[size].dwFlags = 0;
                size++;
                onscreen_size++;

                /* Clone a format if it is bitmap capable for indirect rendering to bitmaps */
                if (check_fbconfig_bitmap_capability( cfgs[i], visinfo ))
                {
                    TRACE("Found bitmap capable format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                    list[size].fbconfig = cfgs[i];
                    list[size].visual = visinfo;
                    list[size].fmt_id = fmt_id;
                    list[size].render_type = get_render_type_from_fbconfig(gdi_display, cfgs[i]);
                    list[size].dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_GENERIC_FORMAT;
                    size++;
                    onscreen_size++;
                }
            } else if(run && !visinfo) {
                int window_drawable=0;
                pglXGetFBConfigAttrib(gdi_display, cfgs[i], GLX_DRAWABLE_TYPE, &window_drawable);

                /* Recent Nvidia drivers and DRI drivers offer window drawable formats without a visual.
                 * This are formats like 16-bit rgb on a 24-bit desktop. In order to support these formats
                 * onscreen we would have to use glXCreateWindow instead of XCreateWindow. Further it will
                 * likely make our child window opengl rendering more complicated since likely you can't use
                 * XCopyArea on a GLX Window.
                 * For now ignore fbconfigs which are window drawable but lack a visual. */
                if(window_drawable & GLX_WINDOW_BIT)
                {
                    TRACE("Skipping FBCONFIG_ID 0x%x as an offscreen format because it is window_drawable\n", fmt_id);
                    continue;
                }

                TRACE("Found offscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                list[size].fbconfig = cfgs[i];
                list[size].fmt_id = fmt_id;
                list[size].render_type = get_render_type_from_fbconfig(gdi_display, cfgs[i]);
                if (!check_fbconfig_bitmap_capability( cfgs[i], NULL )) list[size].dwFlags = 0;
                else list[size].dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_GENERIC_FORMAT;
                size++;
            }
            else if (visinfo) XFree(visinfo);
        }
    }

    XFree(cfgs);

    pixel_formats = list;
    nb_pixel_formats = size;
    nb_onscreen_formats = onscreen_size;

    *onscreen_count = onscreen_size;
    return size;
}

static struct glx_pixel_format *glx_pixel_format_from_format( int format )
{
    assert( format > 0 && format <= nb_pixel_formats );
    return &pixel_formats[format - 1];
}

static void x11drv_surface_destroy( struct opengl_drawable *base )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );

    TRACE( "drawable %s\n", debugstr_opengl_drawable( base ) );

    if (gl->drawable) pglXDestroyWindow( gdi_display, gl->drawable );
    if (gl->colormap) XFreeColormap( gdi_display, gl->colormap );
}

static BOOL set_swap_interval( struct gl_drawable *gl, int interval )
{
    BOOL ret = TRUE;

    if (interval < 0 && !has_swap_control_tear) interval = -interval;

    switch (swap_control_method)
    {
    case GLX_SWAP_CONTROL_EXT:
        X11DRV_expect_error(gdi_display, GLXErrorHandler, NULL);
        pglXSwapIntervalEXT( gdi_display, gl->drawable, interval );
        XSync(gdi_display, False);
        ret = !X11DRV_check_error();
        break;

    case GLX_SWAP_CONTROL_MESA:
        ret = !pglXSwapIntervalMESA(interval);
        break;

    case GLX_SWAP_CONTROL_SGI:
        /* wglSwapIntervalEXT considers an interval value of zero to mean that
         * vsync should be disabled, but glXSwapIntervalSGI considers such a
         * value to be an error. Just silently ignore the request for now.
         */
        if (!interval)
            WARN("Request to disable vertical sync is not handled\n");
        else
            ret = !pglXSwapIntervalSGI(interval);
        break;

    case GLX_SWAP_CONTROL_NONE:
        /* Unlikely to happen on modern GLX implementations */
        WARN("Request to adjust swap interval is not handled\n");
        break;
    }

    return ret;
}

static GLXContext create_glxcontext( int format, GLXContext share, const int *attribs )
{
    struct glx_pixel_format *fmt = glx_pixel_format_from_format( format );
    GLXContext ctx;

    if (attribs) ctx = pglXCreateContextAttribsARB( gdi_display, fmt->fbconfig, share, TRUE, attribs );
    else if (fmt->visual) ctx = pglXCreateContext( gdi_display, fmt->visual, share, TRUE );
    else ctx = pglXCreateNewContext( gdi_display, fmt->fbconfig, fmt->render_type, share, TRUE );

    return ctx;
}

static BOOL x11drv_surface_create( HWND hwnd, int format, struct opengl_drawable **drawable )
{
    struct glx_pixel_format *fmt = glx_pixel_format_from_format( format );
    struct opengl_drawable *previous;
    struct client_surface *client;
    struct gl_drawable *gl;
    Colormap colormap;
    Window window;
    RECT rect;

    if ((previous = *drawable) && previous->format == format) return TRUE;
    NtUserGetClientRect( hwnd, &rect, NtUserGetDpiForWindow( hwnd ) );

    colormap = XCreateColormap( gdi_display, get_dummy_parent(), fmt->visual->visual,
                                (fmt->visual->class == PseudoColor || fmt->visual->class == GrayScale ||
                                 fmt->visual->class == DirectColor) ? AllocAll : AllocNone );
    if (!colormap) return FALSE;

    if (!(window = x11drv_client_surface_create( hwnd, fmt->visual, colormap, &client ))) goto failed;
    gl = opengl_drawable_create( sizeof(*gl), &x11drv_surface_funcs, format, client );
    client_surface_release( client );
    if (!gl) goto failed;
    gl->colormap = colormap;

    if (!(gl->drawable = pglXCreateWindow( gdi_display, fmt->fbconfig, window, NULL )))
    {
        opengl_drawable_release( &gl->base );
        return FALSE;
    }

    TRACE( "Created drawable %s with client window %lx\n", debugstr_opengl_drawable( &gl->base ), window );
    XFlush( gdi_display );

    if (previous) opengl_drawable_release( previous );
    *drawable = &gl->base;
    return TRUE;

failed:
    XFreeColormap( gdi_display, colormap );
    return FALSE;
}

static BOOL x11drv_describe_pixel_format( int format, struct wgl_pixel_format *pf )
{
    int value, drawable_type = 0, render_type = 0;
    struct glx_pixel_format *fmt = glx_pixel_format_from_format( format );
    int rb, gb, bb, ab;

    /* If we can't get basic information, there is no point continuing */
    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &drawable_type )) return 0;
    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_RENDER_TYPE, &render_type )) return 0;

    memset( pf, 0, sizeof(*pf) );
    pf->pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pf->pfd.nVersion = 1;

    /* These flags are always the same... */
    pf->pfd.dwFlags = PFD_SUPPORT_OPENGL;
    /* Now the flags extracted from the Visual */

    if (drawable_type & GLX_WINDOW_BIT) pf->pfd.dwFlags |= PFD_DRAW_TO_WINDOW;

    /* On Windows bitmap rendering is only offered using the GDI Software
     * renderer. We reserve some formats (see get_formats for more info) for
     * bitmap rendering since we require indirect rendering for this. Further
     * pixel format logs of a GeforceFX, Geforce8800GT, Radeon HD3400 and a
     * Radeon 9000 indicated that all bitmap formats have PFD_SUPPORT_GDI.
     * Except for 2 formats on the Radeon 9000 none of the hw accelerated
     * formats offered the GDI bit either. */
    pf->pfd.dwFlags |= fmt->dwFlags & (PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI);

    /* PFD_GENERIC_FORMAT - gdi software rendering
     * PFD_GENERIC_ACCELERATED - some parts are accelerated by a display driver
     * (MCD e.g. 3dfx minigl) none set - full hardware accelerated by a ICD
     *
     * We only set PFD_GENERIC_FORMAT on bitmap formats (see get_formats) as
     * that's what ATI and Nvidia Windows drivers do  */
    pf->pfd.dwFlags |= fmt->dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED);

    if (!(pf->pfd.dwFlags & PFD_GENERIC_FORMAT)) pf->pfd.dwFlags |= PFD_SUPPORT_COMPOSITION;

    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_DOUBLEBUFFER, &value );
    if (value)
    {
        pf->pfd.dwFlags |= PFD_DOUBLEBUFFER;
        pf->pfd.dwFlags &= ~PFD_SUPPORT_GDI;
    }
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_STEREO, &value );
    if (value) pf->pfd.dwFlags |= PFD_STEREO;

    /* Pixel type */
    if (render_type & GLX_RGBA_BIT) pf->pfd.iPixelType = PFD_TYPE_RGBA;
    else pf->pfd.iPixelType = PFD_TYPE_COLORINDEX;

    /* Color bits */
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_BUFFER_SIZE, &value );
    pf->pfd.cColorBits = value;

    /* Red, green, blue and alpha bits / shifts */
    if (pf->pfd.iPixelType == PFD_TYPE_RGBA)
    {
        pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_RED_SIZE, &rb );
        pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_GREEN_SIZE, &gb );
        pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_BLUE_SIZE, &bb );
        pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_ALPHA_SIZE, &ab );

        pf->pfd.cBlueBits = bb;
        pf->pfd.cBlueShift = 0;
        pf->pfd.cGreenBits = gb;
        pf->pfd.cGreenShift = bb;
        pf->pfd.cRedBits = rb;
        pf->pfd.cRedShift = gb + bb;
        pf->pfd.cAlphaBits = ab;
        if (ab) pf->pfd.cAlphaShift = rb + gb + bb;
        else pf->pfd.cAlphaShift = 0;
    }
    else
    {
        pf->pfd.cRedBits = 0;
        pf->pfd.cRedShift = 0;
        pf->pfd.cBlueBits = 0;
        pf->pfd.cBlueShift = 0;
        pf->pfd.cGreenBits = 0;
        pf->pfd.cGreenShift = 0;
        pf->pfd.cAlphaBits = 0;
        pf->pfd.cAlphaShift = 0;
    }

    /* Accum RGBA bits */
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_ACCUM_RED_SIZE, &rb );
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_ACCUM_GREEN_SIZE, &gb );
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_ACCUM_BLUE_SIZE, &bb );
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_ACCUM_ALPHA_SIZE, &ab );

    pf->pfd.cAccumBits = rb + gb + bb + ab;
    pf->pfd.cAccumRedBits = rb;
    pf->pfd.cAccumGreenBits = gb;
    pf->pfd.cAccumBlueBits = bb;
    pf->pfd.cAccumAlphaBits = ab;

    /* Aux bits */
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_AUX_BUFFERS, &value );
    pf->pfd.cAuxBuffers = value;

    /* Depth bits */
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_DEPTH_SIZE, &value );
    pf->pfd.cDepthBits = value;

    /* stencil bits */
    pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_STENCIL_SIZE, &value );
    pf->pfd.cStencilBits = value;

    pf->pfd.iLayerType = PFD_MAIN_PLANE;

    if (!has_swap_method) pf->swap_method = WGL_SWAP_EXCHANGE_ARB;
    else if (!pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_SWAP_METHOD_OML, &value ))
    {
        switch (value) {
        case GLX_SWAP_EXCHANGE_OML: pf->swap_method = WGL_SWAP_EXCHANGE_ARB; break;
        case GLX_SWAP_COPY_OML: pf->swap_method = WGL_SWAP_COPY_ARB; break;
        case GLX_SWAP_UNDEFINED_OML: pf->swap_method = WGL_SWAP_UNDEFINED_ARB; break;
        default: { ERR( "Unexpected swap method %x.\n", value ); pf->swap_method = -1; break; }
        }
    }
    else pf->swap_method = -1;

    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_TRANSPARENT_TYPE, &value) ) pf->transparent = -1;
    else pf->transparent = value != GLX_NONE;

    if (render_type & GLX_RGBA_BIT) pf->pixel_type = WGL_TYPE_RGBA_ARB;
    else if (render_type & GLX_COLOR_INDEX_BIT) pf->pixel_type = WGL_TYPE_COLORINDEX_ARB;
    else if (render_type & GLX_RGBA_FLOAT_BIT) pf->pixel_type = WGL_TYPE_RGBA_FLOAT_ATI;
    else if (render_type & GLX_RGBA_FLOAT_ATI_BIT) pf->pixel_type = WGL_TYPE_RGBA_FLOAT_ATI;
    else if (render_type & GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT) pf->pixel_type = WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT;
    else { ERR( "unexpected RenderType(%x)\n", render_type ); pf->pixel_type = -1; }

    pf->draw_to_pbuffer = (drawable_type & GLX_PBUFFER_BIT) ? GL_TRUE : GL_FALSE;
    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_MAX_PBUFFER_PIXELS, &value )) value = -1;
    pf->max_pbuffer_pixels = value;
    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_MAX_PBUFFER_WIDTH, &value )) value = -1;
    pf->max_pbuffer_width = value;
    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_MAX_PBUFFER_HEIGHT, &value )) value = -1;
    pf->max_pbuffer_height = value;

    if (!pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_TRANSPARENT_RED_VALUE, &value ))
    {
        pf->transparent_red_value_valid = GL_TRUE;
        pf->transparent_red_value = value;
    }
    if (!pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_TRANSPARENT_GREEN_VALUE, &value ))
    {
        pf->transparent_green_value_valid = GL_TRUE;
        pf->transparent_green_value = value;
    }
    if (!pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_TRANSPARENT_BLUE_VALUE, &value ))
    {
        pf->transparent_blue_value_valid = GL_TRUE;
        pf->transparent_blue_value = value;
    }
    if (!pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_TRANSPARENT_ALPHA_VALUE, &value ))
    {
        pf->transparent_alpha_value_valid = GL_TRUE;
        pf->transparent_alpha_value = value;
    }
    if (!pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_TRANSPARENT_INDEX_VALUE, &value ))
    {
        pf->transparent_index_value_valid = GL_TRUE;
        pf->transparent_index_value = value;
    }

    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_SAMPLE_BUFFERS_ARB, &value )) value = -1;
    pf->sample_buffers = value;
    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_SAMPLES_ARB, &value )) value = -1;
    pf->samples = value;

    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, &value )) value = -1;
    pf->framebuffer_srgb_capable = value;

    pf->bind_to_texture_rgb = pf->bind_to_texture_rgba = render_type != GLX_COLOR_INDEX_BIT && (drawable_type & GLX_PBUFFER_BIT);
    pf->bind_to_texture_rectangle_rgb = pf->bind_to_texture_rectangle_rgba = GL_FALSE;

    if (pglXGetFBConfigAttrib( gdi_display, fmt->fbconfig, GLX_FLOAT_COMPONENTS_NV, &value )) value = -1;
    pf->float_components = value;

    if (TRACE_ON(wgl)) dump_PIXELFORMATDESCRIPTOR( &pf->pfd );

    return TRUE;
}

/***********************************************************************
 *		glxdrv_wglDeleteContext
 */
static BOOL x11drv_context_destroy( void *context )
{
    TRACE("(%p)\n", context);
    pglXDestroyContext( gdi_display, context );
    return TRUE;
}

static void *x11drv_get_proc_address( const char *name )
{
    void *ptr;
    if ((ptr = dlsym( opengl_handle, name ))) return ptr;
    return pglXGetProcAddressARB( (const GLubyte *)name );
}

static BOOL x11drv_make_current( struct opengl_drawable *draw_base, struct opengl_drawable *read_base, void *context )
{
    struct gl_drawable *draw = impl_from_opengl_drawable( draw_base ), *read = impl_from_opengl_drawable( read_base );
    BOOL ret;

    TRACE( "draw %s, read %s, context %p\n", debugstr_opengl_drawable( draw_base ), debugstr_opengl_drawable( read_base ), context );

    if (!pglXMakeContextCurrent || !context) ret = pglXMakeCurrent( gdi_display, context ? draw->drawable : None, context );
    else ret = pglXMakeContextCurrent( gdi_display, draw->drawable, read->drawable, context );
    if (ret) NtCurrentTeb()->glReserved2 = context;
    return ret;
}

static void x11drv_surface_flush( struct opengl_drawable *base, UINT flags )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );

    TRACE( "%s flags %#x\n", debugstr_opengl_drawable( base ), flags );

    if (flags & GL_FLUSH_INTERVAL) set_swap_interval( gl, base->interval );

    if (InterlockedCompareExchange( &base->client->offscreen, 0, 0 ))
    {
        if (!(flags & GL_FLUSH_FINISHED)) funcs->p_glFinish();
        XFlush( gdi_display );
        client_surface_present( base->client );
    }
}

/***********************************************************************
 *		X11DRV_wglCreateContextAttribsARB
 */
static BOOL x11drv_context_create( int format, void *share, const int *attribList, void **context )
{
    int glx_attribs[16] = {0}, *pContextAttribList = glx_attribs;
    int err = 0;

    TRACE("(%d %p %p)\n", format, share, attribList);

    if (attribList)
    {
        /* attribList consists of pairs {token, value] terminated with 0 */
        while(attribList[0] != 0)
        {
            TRACE("%#x %#x\n", attribList[0], attribList[1]);
            switch(attribList[0])
            {
            case WGL_CONTEXT_MAJOR_VERSION_ARB:
                pContextAttribList[0] = GLX_CONTEXT_MAJOR_VERSION_ARB;
                pContextAttribList[1] = attribList[1];
                pContextAttribList += 2;
                break;
            case WGL_CONTEXT_MINOR_VERSION_ARB:
                pContextAttribList[0] = GLX_CONTEXT_MINOR_VERSION_ARB;
                pContextAttribList[1] = attribList[1];
                pContextAttribList += 2;
                break;
            case WGL_CONTEXT_LAYER_PLANE_ARB:
                break;
            case WGL_CONTEXT_FLAGS_ARB:
                pContextAttribList[0] = GLX_CONTEXT_FLAGS_ARB;
                pContextAttribList[1] = attribList[1];
                pContextAttribList += 2;
                break;
            case WGL_CONTEXT_OPENGL_NO_ERROR_ARB:
                pContextAttribList[0] = GLX_CONTEXT_OPENGL_NO_ERROR_ARB;
                pContextAttribList[1] = attribList[1];
                pContextAttribList += 2;
                break;
            case WGL_CONTEXT_PROFILE_MASK_ARB:
                pContextAttribList[0] = GLX_CONTEXT_PROFILE_MASK_ARB;
                pContextAttribList[1] = attribList[1];
                pContextAttribList += 2;
                break;
            case WGL_RENDERER_ID_WINE:
                pContextAttribList[0] = GLX_RENDERER_ID_MESA;
                pContextAttribList[1] = attribList[1];
                pContextAttribList += 2;
                break;
            default:
                ERR("Unhandled attribList pair: %#x %#x\n", attribList[0], attribList[1]);
            }
            attribList += 2;
        }
    }

    X11DRV_expect_error(gdi_display, GLXErrorHandler, NULL);
    *context = create_glxcontext( format, share, attribList ? glx_attribs : NULL );
    XSync(gdi_display, False);
    if ((err = X11DRV_check_error()) || !*context)
    {
        /* In the future we should convert the GLX error to a win32 one here if needed */
        WARN("Context creation failed (error %#x).\n", err);
        return FALSE;
    }

    TRACE( "-> %p\n", *context );
    return TRUE;
}

static BOOL x11drv_pbuffer_create( HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                   GLint max_level, GLsizei *width, GLsizei *height, struct opengl_drawable **drawable )
{
    const struct glx_pixel_format *fmt = glx_pixel_format_from_format( format );
    int glx_attribs[7], count = 0;
    struct gl_drawable *gl;
    RECT rect;

    TRACE( "hdc %p, format %d, largest %u, texture_format %#x, texture_target %#x, max_level %#x, width %d, height %d, drawable %p\n",
           hdc, format, largest, texture_format, texture_target, max_level, *width, *height, drawable );

    glx_attribs[count++] = GLX_PBUFFER_WIDTH;
    glx_attribs[count++] = *width;
    glx_attribs[count++] = GLX_PBUFFER_HEIGHT;
    glx_attribs[count++] = *height;
    if (largest)
    {
        glx_attribs[count++] = GLX_LARGEST_PBUFFER;
        glx_attribs[count++] = 1;
    }
    glx_attribs[count++] = 0;

    if (!(gl = opengl_drawable_create( sizeof(*gl), &x11drv_pbuffer_funcs, format, NULL ))) return FALSE;

    gl->drawable = pglXCreatePbuffer( gdi_display, fmt->fbconfig, glx_attribs );
    TRACE( "new Pbuffer drawable as %p (%lx)\n", gl, gl->drawable );
    if (!gl->drawable)
    {
        opengl_drawable_release( &gl->base );
        return FALSE;
    }
    pglXQueryDrawable( gdi_display, gl->drawable, GLX_WIDTH, (unsigned int *)width );
    pglXQueryDrawable( gdi_display, gl->drawable, GLX_HEIGHT, (unsigned int *)height );
    SetRect( &rect, 0, 0, *width, *height );
    set_dc_drawable( hdc, gl->drawable, &rect, IncludeInferiors );

    *drawable = &gl->base;
    return TRUE;
}

static void x11drv_pbuffer_destroy( struct opengl_drawable *base )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );

    TRACE( "drawable %s\n", debugstr_opengl_drawable( base ) );

    if (gl->drawable) pglXDestroyPbuffer( gdi_display, gl->drawable );
}

static BOOL x11drv_pbuffer_updated( HDC hdc, struct opengl_drawable *base, GLenum cube_face, GLint mipmap_level )
{
    return GL_TRUE;
}

static UINT x11drv_pbuffer_bind( HDC hdc, struct opengl_drawable *base, GLenum buffer )
{
    return -1; /* use default implementation */
}

static BOOL X11DRV_wglQueryCurrentRendererIntegerWINE( GLenum attribute, GLuint *value )
{
    return pglXQueryCurrentRendererIntegerMESA( attribute, value );
}

static const char *X11DRV_wglQueryCurrentRendererStringWINE( GLenum attribute )
{
    return pglXQueryCurrentRendererStringMESA( attribute );
}

static BOOL X11DRV_wglQueryRendererIntegerWINE( HDC dc, GLint renderer, GLenum attribute, GLuint *value )
{
    return pglXQueryRendererIntegerMESA( gdi_display, DefaultScreen(gdi_display), renderer, attribute, value );
}

static const char *X11DRV_wglQueryRendererStringWINE( HDC dc, GLint renderer, GLenum attribute )
{
    return pglXQueryRendererStringMESA( gdi_display, DefaultScreen(gdi_display), renderer, attribute );
}

/**
 * glxRequireVersion (internal)
 *
 * Check if the supported GLX version matches requiredVersion.
 */
static BOOL glxRequireVersion(int requiredVersion)
{
    /* Both requiredVersion and glXVersion[1] contains the minor GLX version */
    return (requiredVersion <= glxVersion[1]);
}

static void register_extension(const char *ext)
{
    if (wglExtensions[0])
        strcat(wglExtensions, " ");
    strcat(wglExtensions, ext);

    TRACE("'%s'\n", ext);
}

static const char *x11drv_init_wgl_extensions( struct opengl_funcs *funcs )
{
    wglExtensions[0] = 0;

    /* ARB Extensions */

    if (has_extension( glxExtensions, "GLX_ARB_multisample")) register_extension( "WGL_ARB_multisample" );

    if (has_extension( glxExtensions, "GLX_ARB_fbconfig_float"))
    {
        register_extension("WGL_ARB_pixel_format_float");
        register_extension("WGL_ATI_pixel_format_float");
    }

    /* Support WGL_ARB_render_texture when there's support or pbuffer based emulation */
    if (has_extension( glxExtensions, "GLX_ARB_render_texture" ) || glxRequireVersion( 3 ))
    {
        /* The WGL version of GLX_NV_float_buffer requires render_texture */
        if (has_extension( glxExtensions, "GLX_NV_float_buffer"))
            register_extension("WGL_NV_float_buffer");

        /* Again there's no GLX equivalent for this extension, so depend on the required GL extension */
        if (has_extension(glExtensions, "GL_NV_texture_rectangle"))
            register_extension("WGL_NV_render_texture_rectangle");
    }

    /* EXT Extensions */

    if (has_extension( glxExtensions, "GLX_EXT_framebuffer_sRGB"))
        register_extension("WGL_EXT_framebuffer_sRGB");

    if (has_extension( glxExtensions, "GLX_EXT_fbconfig_packed_float"))
        register_extension("WGL_EXT_pixel_format_packed_float");

    if (has_extension( glxExtensions, "GLX_EXT_swap_control"))
    {
        swap_control_method = GLX_SWAP_CONTROL_EXT;
        has_swap_control_tear = has_extension( glxExtensions, "GLX_EXT_swap_control_tear" );
    }
    else if (has_extension( glxExtensions, "GLX_MESA_swap_control"))
    {
        swap_control_method = GLX_SWAP_CONTROL_MESA;
    }
    else if (has_extension( glxExtensions, "GLX_SGI_swap_control"))
    {
        swap_control_method = GLX_SWAP_CONTROL_SGI;
    }

    /* The OpenGL extension GL_NV_vertex_array_range adds wgl/glX functions which aren't exported as 'real' wgl/glX extensions. */
    if (has_extension(glExtensions, "GL_NV_vertex_array_range"))
    {
        register_extension( "WGL_NV_vertex_array_range" );
        funcs->p_wglAllocateMemoryNV = pglXAllocateMemoryNV;
        funcs->p_wglFreeMemoryNV = pglXFreeMemoryNV;
    }

    if (has_extension(glxExtensions, "GLX_OML_swap_method"))
        has_swap_method = TRUE;

    /* WINE-specific WGL Extensions */

    if (has_extension( glxExtensions, "GLX_MESA_query_renderer" ))
    {
        register_extension( "WGL_WINE_query_renderer" );
        funcs->p_wglQueryCurrentRendererIntegerWINE = X11DRV_wglQueryCurrentRendererIntegerWINE;
        funcs->p_wglQueryCurrentRendererStringWINE = X11DRV_wglQueryCurrentRendererStringWINE;
        funcs->p_wglQueryRendererIntegerWINE = X11DRV_wglQueryRendererIntegerWINE;
        funcs->p_wglQueryRendererStringWINE = X11DRV_wglQueryRendererStringWINE;
    }

    return wglExtensions;
}

static BOOL x11drv_surface_swap( struct opengl_drawable *base )
{
    GLXContext ctx = NtCurrentTeb()->glReserved2;
    struct gl_drawable *gl = impl_from_opengl_drawable( base );
    INT64 ust, msc, sbc, target_sbc = 0;
    BOOL offscreen;

    TRACE( "drawable %s\n", debugstr_opengl_drawable( base ) );

    if ((offscreen = InterlockedCompareExchange( &base->client->offscreen, 0, 0 )) ||
        !ctx || !pglXSwapBuffersMscOML) pglXSwapBuffers( gdi_display, gl->drawable );
    else
    {
        funcs->p_glFlush();
        target_sbc = pglXSwapBuffersMscOML( gdi_display, gl->drawable, 0, 0, 0 );
        if (pglXWaitForSbcOML) pglXWaitForSbcOML( gdi_display, gl->drawable, target_sbc, &ust, &msc, &sbc );
    }

    if (offscreen)
    {
        if (!pglXWaitForSbcOML) XFlush( gdi_display );
        client_surface_present( base->client );
    }

    return TRUE;
}

static void x11drv_egl_surface_destroy( struct opengl_drawable *base )
{
    TRACE( "drawable %s\n", debugstr_opengl_drawable( base ) );
}

static void x11drv_egl_surface_flush( struct opengl_drawable *base, UINT flags )
{
    TRACE( "%s\n", debugstr_opengl_drawable( base ) );

    if (flags & GL_FLUSH_INTERVAL) funcs->p_eglSwapInterval( egl->display, abs( base->interval ) );

    if (InterlockedCompareExchange( &base->client->offscreen, 0, 0 ))
    {
        if (!(flags & GL_FLUSH_FINISHED)) funcs->p_glFinish();
        XFlush( gdi_display );
        client_surface_present( base->client );
    }
}

static BOOL x11drv_egl_surface_swap( struct opengl_drawable *base )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );

    TRACE( "%s\n", debugstr_opengl_drawable( base ) );

    funcs->p_eglSwapBuffers( egl->display, gl->base.surface );

    if (InterlockedCompareExchange( &base->client->offscreen, 0, 0 ))
    {
        XFlush( gdi_display );
        client_surface_present( base->client );
    }

    return TRUE;
}

static struct opengl_driver_funcs x11drv_driver_funcs =
{
    .p_get_proc_address = x11drv_get_proc_address,
    .p_init_pixel_formats = x11drv_init_pixel_formats,
    .p_describe_pixel_format = x11drv_describe_pixel_format,
    .p_init_wgl_extensions = x11drv_init_wgl_extensions,
    .p_surface_create = x11drv_surface_create,
    .p_context_create = x11drv_context_create,
    .p_context_destroy = x11drv_context_destroy,
    .p_make_current = x11drv_make_current,
    .p_pbuffer_create = x11drv_pbuffer_create,
    .p_pbuffer_updated = x11drv_pbuffer_updated,
    .p_pbuffer_bind = x11drv_pbuffer_bind,
};

static const struct opengl_drawable_funcs x11drv_surface_funcs =
{
    .destroy = x11drv_surface_destroy,
    .flush = x11drv_surface_flush,
    .swap = x11drv_surface_swap,
};

static const struct opengl_drawable_funcs x11drv_pbuffer_funcs =
{
    .destroy = x11drv_pbuffer_destroy,
};

static const struct opengl_drawable_funcs x11drv_egl_surface_funcs =
{
    .destroy = x11drv_egl_surface_destroy,
    .flush = x11drv_egl_surface_flush,
    .swap = x11drv_egl_surface_swap,
};

#else  /* no OpenGL includes */

/**********************************************************************
 *           X11DRV_OpenglInit
 */
UINT X11DRV_OpenGLInit( UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs )
{
    return STATUS_NOT_IMPLEMENTED;
}

void sync_gl_drawable( HWND hwnd )
{
}

void destroy_gl_drawable( HWND hwnd )
{
}

#endif /* defined(SONAME_LIBGL) */
