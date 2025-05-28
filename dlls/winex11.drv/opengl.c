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

struct x11drv_context
{
    HDC hdc;
    BOOL gl3_context;
    const struct glx_pixel_format *fmt;
    GLXContext ctx;
    struct gl_drawable *drawables[2];
    struct gl_drawable *new_drawables[2];
    struct list entry;
};

enum dc_gl_type
{
    DC_GL_NONE,       /* no GL support (pixel format not set yet) */
    DC_GL_WINDOW,     /* normal top-level window */
    DC_GL_CHILD_WIN,  /* child window using XComposite */
    DC_GL_PIXMAP_WIN, /* child window using intermediate pixmap */
    DC_GL_PBUFFER     /* pseudo memory DC using a PBuffer */
};

struct gl_drawable
{
    LONG                           ref;          /* reference count */
    enum dc_gl_type                type;         /* type of GL surface */
    HWND                           hwnd;
    RECT                           rect;         /* current size of the GL drawable */
    GLXDrawable                    drawable;     /* drawable for rendering with GL */
    Window                         window;       /* window if drawable is a GLXWindow */
    Colormap                       colormap;     /* colormap for the client window */
    Pixmap                         pixmap;       /* base pixmap if drawable is a GLXPixmap */
    const struct glx_pixel_format *format;       /* pixel format for the drawable */
    int                            swap_interval;
    HDC                            hdc_src;
    HDC                            hdc_dst;
};

enum glx_swap_control_method
{
    GLX_SWAP_CONTROL_NONE,
    GLX_SWAP_CONTROL_EXT,
    GLX_SWAP_CONTROL_SGI,
    GLX_SWAP_CONTROL_MESA
};

/* X context to associate a struct gl_drawable to an hwnd */
static XContext gl_hwnd_context;
/* X context to associate a struct gl_drawable to a pbuffer hdc */
static XContext gl_pbuffer_context;

static struct list context_list = LIST_INIT( context_list );
static struct glx_pixel_format *pixel_formats;
static int nb_pixel_formats, nb_onscreen_formats;

/* Selects the preferred GLX swap control method for use by wglSwapIntervalEXT */
static enum glx_swap_control_method swap_control_method = GLX_SWAP_CONTROL_NONE;
/* Set when GLX_EXT_swap_control_tear is supported, requires GLX_SWAP_CONTROL_EXT */
static BOOL has_swap_control_tear = FALSE;
static BOOL has_swap_method = FALSE;

static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;

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
static void (*pglFinish)(void);
static void (*pglFlush)(void);
static const GLubyte *(*pglGetString)(GLenum name);

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

static void *opengl_handle;
static const struct opengl_driver_funcs x11drv_driver_funcs;

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
    LOAD_FUNCPTR( glFinish );
    LOAD_FUNCPTR( glFlush );
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
    gl_hwnd_context = XUniqueContext();
    gl_pbuffer_context = XUniqueContext();

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

static const char *debugstr_fbconfig( GLXFBConfig fbconfig )
{
    int id, visual, drawable;

    if (pglXGetFBConfigAttrib( gdi_display, fbconfig, GLX_FBCONFIG_ID, &id ))
        return "*** invalid fbconfig";
    pglXGetFBConfigAttrib( gdi_display, fbconfig, GLX_VISUAL_ID, &visual );
    pglXGetFBConfigAttrib( gdi_display, fbconfig, GLX_DRAWABLE_TYPE, &drawable );
    return wine_dbg_sprintf( "fbconfig %#x visual id %#x drawable type %#x", id, visual, drawable );
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
static BOOL check_fbconfig_bitmap_capability(Display *display, GLXFBConfig fbconfig)
{
    int dbuf, value;
    pglXGetFBConfigAttrib(display, fbconfig, GLX_DOUBLEBUFFER, &dbuf);
    pglXGetFBConfigAttrib(gdi_display, fbconfig, GLX_DRAWABLE_TYPE, &value);

    /* Windows only supports bitmap rendering on single buffered formats, further the fbconfig needs to have
     * the GLX_PIXMAP_BIT set. */
    return !dbuf && (value & GLX_PIXMAP_BIT);
}

static UINT x11drv_init_pixel_formats( UINT *onscreen_count )
{
    struct glx_pixel_format *list;
    int size = 0, onscreen_size = 0;
    int fmt_id, nCfgs, i, run, bmp_formats;
    GLXFBConfig* cfgs;
    XVisualInfo *visinfo;

    cfgs = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs);
    if (NULL == cfgs || 0 == nCfgs) {
        if(cfgs != NULL) XFree(cfgs);
        ERR("glXChooseFBConfig returns NULL\n");
        return 0;
    }

    /* Bitmap rendering on Windows implies the use of the Microsoft GDI software renderer.
     * Further most GLX drivers only offer pixmap rendering using indirect rendering (except for modern drivers which support 'AIGLX' / composite).
     * Indirect rendering can indicate software rendering (on Nvidia it is hw accelerated)
     * Since bitmap rendering implies the use of software rendering we can safely use indirect rendering for bitmaps.
     *
     * Below we count the number of formats which are suitable for bitmap rendering. Windows restricts bitmap rendering to single buffered formats.
     */
    for(i=0, bmp_formats=0; i<nCfgs; i++)
    {
        if(check_fbconfig_bitmap_capability(gdi_display, cfgs[i]))
            bmp_formats++;
    }
    TRACE("Found %d bitmap capable fbconfigs\n", bmp_formats);

    list = calloc( 1, (nCfgs + bmp_formats) * sizeof(*list) );

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
                if(check_fbconfig_bitmap_capability(gdi_display, cfgs[i]))
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
                list[size].dwFlags = 0;
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

static inline BOOL is_valid_pixel_format( int format )
{
    return format > 0 && format <= nb_pixel_formats;
}

static inline BOOL is_onscreen_pixel_format( int format )
{
    return format > 0 && format <= nb_onscreen_formats;
}

/* GLX can advertise dozens of different pixelformats including offscreen and onscreen ones.
 * In our WGL implementation we only support a subset of these formats namely the format of
 * Wine's main visual and offscreen formats (if they are available).
 * This function converts a WGL format to its corresponding GLX one.
 */
static const struct glx_pixel_format *get_pixel_format(Display *display, int iPixelFormat, BOOL AllowOffscreen)
{
    /* Check if the pixelformat is valid. Note that it is legal to pass an invalid
     * iPixelFormat in case of probing the number of pixelformats.
     */
    if (is_valid_pixel_format( iPixelFormat ) &&
        (is_onscreen_pixel_format( iPixelFormat ) || AllowOffscreen)) {
        TRACE("Returning fmt_id=%#x for iPixelFormat=%d\n",
              pixel_formats[iPixelFormat-1].fmt_id, iPixelFormat);
        return &pixel_formats[iPixelFormat-1];
    }
    return NULL;
}

static struct gl_drawable *grab_gl_drawable( struct gl_drawable *gl )
{
    InterlockedIncrement( &gl->ref );
    return gl;
}

static void release_gl_drawable( struct gl_drawable *gl )
{
    if (!gl) return;
    if (InterlockedDecrement( &gl->ref )) return;
    switch (gl->type)
    {
    case DC_GL_WINDOW:
    case DC_GL_CHILD_WIN:
        TRACE( "destroying %lx drawable %lx\n", gl->window, gl->drawable );
        pglXDestroyWindow( gdi_display, gl->drawable );
        destroy_client_window( gl->hwnd, gl->window );
        XFreeColormap( gdi_display, gl->colormap );
        break;
    case DC_GL_PIXMAP_WIN:
        TRACE( "destroying pixmap %lx drawable %lx\n", gl->pixmap, gl->drawable );
        pglXDestroyPixmap( gdi_display, gl->drawable );
        XFreePixmap( gdi_display, gl->pixmap );
        break;
    case DC_GL_PBUFFER:
        TRACE( "destroying pbuffer drawable %lx\n", gl->drawable );
        pglXDestroyPbuffer( gdi_display, gl->drawable );
        break;
    default:
        break;
    }
    if (gl->hdc_src) NtGdiDeleteObjectApp( gl->hdc_src );
    if (gl->hdc_dst) NtGdiDeleteObjectApp( gl->hdc_dst );
    free( gl );
}

/* Mark any allocated context using the glx drawable 'old' to use 'new' */
static void mark_drawable_dirty( struct gl_drawable *old, struct gl_drawable *new )
{
    struct x11drv_context *ctx;

    pthread_mutex_lock( &context_mutex );
    LIST_FOR_EACH_ENTRY( ctx, &context_list, struct x11drv_context, entry )
    {
        if (old == ctx->drawables[0] || old == ctx->new_drawables[0])
        {
            release_gl_drawable( ctx->new_drawables[0] );
            ctx->new_drawables[0] = grab_gl_drawable( new );
        }
        if (old == ctx->drawables[1] || old == ctx->new_drawables[1])
        {
            release_gl_drawable( ctx->new_drawables[1] );
            ctx->new_drawables[1] = grab_gl_drawable( new );
        }
    }
    pthread_mutex_unlock( &context_mutex );
}

/* Given the current context, make sure its drawable is sync'd */
static inline void sync_context(struct x11drv_context *context)
{
    BOOL refresh = FALSE;
    struct gl_drawable *old[2] = { NULL };

    pthread_mutex_lock( &context_mutex );
    if (context->new_drawables[0])
    {
        old[0] = context->drawables[0];
        context->drawables[0] = context->new_drawables[0];
        context->new_drawables[0] = NULL;
        refresh = TRUE;
    }
    if (context->new_drawables[1])
    {
        old[1] = context->drawables[1];
        context->drawables[1] = context->new_drawables[1];
        context->new_drawables[1] = NULL;
        refresh = TRUE;
    }
    if (refresh)
    {
        if (glxRequireVersion(3))
            pglXMakeContextCurrent(gdi_display, context->drawables[0]->drawable,
                                   context->drawables[1]->drawable, context->ctx);
        else
            pglXMakeCurrent(gdi_display, context->drawables[0]->drawable, context->ctx);
        release_gl_drawable( old[0] );
        release_gl_drawable( old[1] );
    }
    pthread_mutex_unlock( &context_mutex );
}

static BOOL set_swap_interval( struct gl_drawable *gl, int interval )
{
    BOOL ret = TRUE;

    if (interval < 0 && !has_swap_control_tear) interval = -interval;
    if (gl->swap_interval == interval) return TRUE;

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

static struct gl_drawable *get_gl_drawable( HWND hwnd, HDC hdc )
{
    struct gl_drawable *gl;

    pthread_mutex_lock( &context_mutex );
    if (hwnd && !XFindContext( gdi_display, (XID)hwnd, gl_hwnd_context, (char **)&gl ))
        gl = grab_gl_drawable( gl );
    else if (hdc && !XFindContext( gdi_display, (XID)hdc, gl_pbuffer_context, (char **)&gl ))
        gl = grab_gl_drawable( gl );
    else
        gl = NULL;
    pthread_mutex_unlock( &context_mutex );
    return gl;
}

static GLXContext create_glxcontext(Display *display, struct x11drv_context *context, GLXContext shareList, const int *attribs)
{
    GLXContext ctx;

    if(context->gl3_context)
        ctx = pglXCreateContextAttribsARB(gdi_display, context->fmt->fbconfig, shareList, GL_TRUE, attribs);
    else if(context->fmt->visual)
        ctx = pglXCreateContext(gdi_display, context->fmt->visual, shareList, GL_TRUE);
    else /* Create a GLX Context for a pbuffer */
        ctx = pglXCreateNewContext(gdi_display, context->fmt->fbconfig, context->fmt->render_type, shareList, TRUE);

    return ctx;
}

/***********************************************************************
 *              create_gl_drawable
 */
static struct gl_drawable *create_gl_drawable( HWND hwnd, const struct glx_pixel_format *format, BOOL known_child )
{
    static const WCHAR displayW[] = {'D','I','S','P','L','A','Y'};
    UNICODE_STRING device_str = RTL_CONSTANT_STRING(displayW);
    struct gl_drawable *gl, *prev;
    XVisualInfo *visual = format->visual;
    RECT rect;
    int width, height;

    NtUserGetClientRect( hwnd, &rect, NtUserGetDpiForWindow( hwnd ) );
    width  = min( max( 1, rect.right ), 65535 );
    height = min( max( 1, rect.bottom ), 65535 );

    if (!(gl = calloc( 1, sizeof(*gl) ))) return NULL;

    /* Default GLX and WGL swap interval is 1, but in case of glXSwapIntervalSGI there is no way to query it. */
    gl->swap_interval = INT_MIN;
    gl->format = format;
    gl->ref = 1;
    gl->hwnd = hwnd;
    gl->rect = rect;

    if (!needs_offscreen_rendering( hwnd, known_child ))
    {
        gl->type = DC_GL_WINDOW;
        gl->colormap = XCreateColormap( gdi_display, get_dummy_parent(), visual->visual,
                                        (visual->class == PseudoColor || visual->class == GrayScale ||
                                         visual->class == DirectColor) ? AllocAll : AllocNone );
        gl->window = create_client_window( hwnd, visual, gl->colormap );
        if (gl->window)
            gl->drawable = pglXCreateWindow( gdi_display, gl->format->fbconfig, gl->window, NULL );
        TRACE( "%p created client %lx drawable %lx\n", hwnd, gl->window, gl->drawable );
    }
#ifdef SONAME_LIBXCOMPOSITE
    else if(usexcomposite)
    {
        gl->type = DC_GL_CHILD_WIN;
        gl->colormap = XCreateColormap( gdi_display, get_dummy_parent(), visual->visual,
                                        (visual->class == PseudoColor || visual->class == GrayScale ||
                                         visual->class == DirectColor) ? AllocAll : AllocNone );
        gl->window = create_client_window( hwnd, visual, gl->colormap );
        if (gl->window)
        {
            struct x11drv_win_data *data;

            gl->drawable = pglXCreateWindow( gdi_display, gl->format->fbconfig, gl->window, NULL );
            pXCompositeRedirectWindow( gdi_display, gl->window, CompositeRedirectManual );

            if ((data = get_win_data( hwnd )))
            {
                detach_client_window( data, gl->window );
                release_win_data( data );
            }

            gl->hdc_dst = NtGdiOpenDCW( &device_str, NULL, NULL, 0, TRUE, NULL, NULL, NULL );
            gl->hdc_src = NtGdiOpenDCW( &device_str, NULL, NULL, 0, TRUE, NULL, NULL, NULL );
            set_dc_drawable( gl->hdc_src, gl->window, &gl->rect, IncludeInferiors );
        }

        TRACE( "%p created child %lx drawable %lx\n", hwnd, gl->window, gl->drawable );
    }
#endif
    else
    {
        static unsigned int once;

        if (!once++)
            ERR_(winediag)("XComposite is not available, using GLXPixmap hack.\n");
        WARN("XComposite is not available, using GLXPixmap hack.\n");

        gl->type = DC_GL_PIXMAP_WIN;
        gl->pixmap = XCreatePixmap( gdi_display, root_window, width, height, visual->depth );
        if (gl->pixmap)
        {
            gl->drawable = pglXCreatePixmap( gdi_display, gl->format->fbconfig, gl->pixmap, NULL );
            if (!gl->drawable) XFreePixmap( gdi_display, gl->pixmap );

            gl->hdc_dst = NtGdiOpenDCW( &device_str, NULL, NULL, 0, TRUE, NULL, NULL, NULL );
            gl->hdc_src = NtGdiOpenDCW( &device_str, NULL, NULL, 0, TRUE, NULL, NULL, NULL );
            set_dc_drawable( gl->hdc_src, gl->pixmap, &gl->rect, IncludeInferiors );
        }
    }

    if (!gl->drawable)
    {
        free( gl );
        return NULL;
    }

    pthread_mutex_lock( &context_mutex );
    if (!XFindContext( gdi_display, (XID)hwnd, gl_hwnd_context, (char **)&prev ))
        release_gl_drawable( prev );
    XSaveContext( gdi_display, (XID)hwnd, gl_hwnd_context, (char *)grab_gl_drawable(gl) );
    pthread_mutex_unlock( &context_mutex );
    return gl;
}

static BOOL x11drv_set_pixel_format( HWND hwnd, int old_format, int new_format, BOOL internal )
{
    const struct glx_pixel_format *fmt;
    struct gl_drawable *old, *gl;

    /* Even for internal pixel format fail setting it if the app has already set a
     * different pixel format. Let wined3d create a backup GL context instead.
     * Switching pixel format involves drawable recreation and is much more expensive
     * than blitting from backup context. */
    if (old_format) return old_format == new_format;

    if (!(fmt = get_pixel_format(gdi_display, new_format, FALSE /* Offscreen */)))
    {
        ERR( "Invalid format %d\n", new_format );
        return FALSE;
    }

    if (!(old = get_gl_drawable( hwnd, 0 )) || old->format != fmt)
    {
        if (!(gl = create_gl_drawable( hwnd, fmt, FALSE )))
        {
            release_gl_drawable( old );
            return FALSE;
        }

        TRACE( "created GL drawable %lx for win %p %s\n",
               gl->drawable, hwnd, debugstr_fbconfig( fmt->fbconfig ));

        if (old)
            mark_drawable_dirty( old, gl );

        XFlush( gdi_display );
        release_gl_drawable( gl );
    }

    release_gl_drawable( old );
    return TRUE;
}

static void update_gl_drawable_size( struct gl_drawable *gl )
{
    struct gl_drawable *new_gl;
    XWindowChanges changes;
    RECT rect;

    NtUserGetClientRect( gl->hwnd, &rect, NtUserGetDpiForWindow( gl->hwnd ) );
    if (EqualRect( &rect, &gl->rect )) return;

    changes.width  = min( max( 1, rect.right ), 65535 );
    changes.height = min( max( 1, rect.bottom ), 65535 );

    switch (gl->type)
    {
    case DC_GL_WINDOW:
    case DC_GL_CHILD_WIN:
        gl->rect = rect;
        XConfigureWindow( gdi_display, gl->window, CWWidth | CWHeight, &changes );
        set_dc_drawable( gl->hdc_src, gl->window, &gl->rect, IncludeInferiors );
        break;
    case DC_GL_PIXMAP_WIN:
        new_gl = create_gl_drawable( gl->hwnd, gl->format, TRUE );
        mark_drawable_dirty( gl, new_gl );
        release_gl_drawable( new_gl );
    default:
        break;
    }
}

/***********************************************************************
 *              sync_gl_drawable
 */
void sync_gl_drawable( HWND hwnd, BOOL known_child )
{
    struct gl_drawable *old, *new;
    BOOL is_offscreen;

    if (!(old = get_gl_drawable( hwnd, 0 ))) return;

    switch (old->type)
    {
    case DC_GL_WINDOW:
    case DC_GL_CHILD_WIN:
        is_offscreen = old->type == DC_GL_CHILD_WIN;
        if (is_offscreen == needs_offscreen_rendering( hwnd, known_child ))
        {
            update_gl_drawable_size( old );
            break;
        }
        /* fall through */
    case DC_GL_PIXMAP_WIN:
        if (!(new = create_gl_drawable( hwnd, old->format, known_child ))) break;
        mark_drawable_dirty( old, new );
        XFlush( gdi_display );
        TRACE( "Recreated GL drawable %lx to replace %lx\n", new->drawable, old->drawable );
        release_gl_drawable( new );
        break;
    default:
        break;
    }
    release_gl_drawable( old );
}


/***********************************************************************
 *              set_gl_drawable_parent
 */
void set_gl_drawable_parent( HWND hwnd, HWND parent )
{
    struct gl_drawable *old, *new;

    if (!(old = get_gl_drawable( hwnd, 0 ))) return;

    TRACE( "setting drawable %lx parent %p\n", old->drawable, parent );

    switch (old->type)
    {
    case DC_GL_WINDOW:
        break;
    case DC_GL_CHILD_WIN:
    case DC_GL_PIXMAP_WIN:
        if (parent == NtUserGetDesktopWindow()) break;
        /* fall through */
    default:
        release_gl_drawable( old );
        return;
    }

    if ((new = create_gl_drawable( hwnd, old->format, FALSE )))
    {
        mark_drawable_dirty( old, new );
        release_gl_drawable( new );
    }
    release_gl_drawable( old );
}


/***********************************************************************
 *              destroy_gl_drawable
 */
void destroy_gl_drawable( HWND hwnd )
{
    struct gl_drawable *gl;

    pthread_mutex_lock( &context_mutex );
    if (!XFindContext( gdi_display, (XID)hwnd, gl_hwnd_context, (char **)&gl ))
    {
        XDeleteContext( gdi_display, (XID)hwnd, gl_hwnd_context );
        release_gl_drawable( gl );
    }
    pthread_mutex_unlock( &context_mutex );
}


static BOOL x11drv_describe_pixel_format( int iPixelFormat, struct wgl_pixel_format *pf )
{
    int value, drawable_type = 0, render_type = 0;
    int rb, gb, bb, ab;
    const struct glx_pixel_format *fmt;

    /* Look for the iPixelFormat in our list of supported formats. If it is
     * supported we get the index in the FBConfig table and the number of
     * supported formats back */
    fmt = get_pixel_format( gdi_display, iPixelFormat, TRUE /* Offscreen */);
    if (!fmt)
    {
        WARN( "unexpected format %d\n", iPixelFormat );
        return FALSE;
    }

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
static BOOL x11drv_context_destroy(void *private)
{
    struct x11drv_context *ctx = private;

    TRACE("(%p)\n", ctx);

    pthread_mutex_lock( &context_mutex );
    list_remove( &ctx->entry );
    pthread_mutex_unlock( &context_mutex );

    if (ctx->ctx) pglXDestroyContext( gdi_display, ctx->ctx );
    release_gl_drawable( ctx->drawables[0] );
    release_gl_drawable( ctx->drawables[1] );
    release_gl_drawable( ctx->new_drawables[0] );
    release_gl_drawable( ctx->new_drawables[1] );
    free( ctx );
    return TRUE;
}

static void *x11drv_get_proc_address( const char *name )
{
    void *ptr;
    if ((ptr = dlsym( opengl_handle, name ))) return ptr;
    return pglXGetProcAddressARB( (const GLubyte *)name );
}

static void set_context_drawables( struct x11drv_context *ctx, struct gl_drawable *draw,
                                   struct gl_drawable *read )
{
    struct gl_drawable *prev[4];
    int i;

    prev[0] = ctx->drawables[0];
    prev[1] = ctx->drawables[1];
    prev[2] = ctx->new_drawables[0];
    prev[3] = ctx->new_drawables[1];
    ctx->drawables[0] = grab_gl_drawable( draw );
    ctx->drawables[1] = read ? grab_gl_drawable( read ) : NULL;
    ctx->new_drawables[0] = ctx->new_drawables[1] = NULL;
    for (i = 0; i < 4; i++) release_gl_drawable( prev[i] );
}

static BOOL x11drv_context_make_current( HDC draw_hdc, HDC read_hdc, void *private )
{
    struct x11drv_context *ctx = private;
    BOOL ret = FALSE;
    struct gl_drawable *draw_gl, *read_gl = NULL;

    TRACE("(%p,%p,%p)\n", draw_hdc, read_hdc, ctx);

    if (!private)
    {
        pglXMakeCurrent( gdi_display, None, NULL );
        NtCurrentTeb()->glReserved2 = NULL;
        return TRUE;
    }

    if ((draw_gl = get_gl_drawable( NtUserWindowFromDC( draw_hdc ), draw_hdc )))
    {
        read_gl = get_gl_drawable( NtUserWindowFromDC( read_hdc ), read_hdc );

        pthread_mutex_lock( &context_mutex );
        if (!pglXMakeContextCurrent) ret = pglXMakeCurrent( gdi_display, draw_gl->drawable, ctx->ctx );
        else ret = pglXMakeContextCurrent( gdi_display, draw_gl->drawable, read_gl ? read_gl->drawable : 0, ctx->ctx );
        if (ret)
        {
            ctx->hdc = draw_hdc;
            set_context_drawables( ctx, draw_gl, read_gl );
            NtCurrentTeb()->glReserved2 = ctx;
            pthread_mutex_unlock( &context_mutex );
            goto done;
        }
        pthread_mutex_unlock( &context_mutex );
    }
    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
done:
    release_gl_drawable( read_gl );
    release_gl_drawable( draw_gl );
    TRACE( "%p,%p,%p returning %d\n", draw_hdc, read_hdc, ctx, ret );
    return ret;
}

static void present_gl_drawable( HWND hwnd, HDC hdc, struct gl_drawable *gl, BOOL flush, BOOL gl_finish )
{
    HWND toplevel = NtUserGetAncestor( hwnd, GA_ROOT );
    struct x11drv_win_data *data;
    Drawable window, drawable;
    RECT rect_dst, rect;
    HRGN region;

    if (!gl) return;
    switch (gl->type)
    {
    case DC_GL_PIXMAP_WIN: drawable = gl->pixmap; break;
    case DC_GL_CHILD_WIN: drawable = gl->window; break;
    default: drawable = 0; break;
    }
    if (!drawable) return;
    window = get_dc_drawable( hdc, &rect );
    region = get_dc_monitor_region( hwnd, hdc );

    if (gl_finish) pglFinish();
    if (flush) XFlush( gdi_display );

    NtUserGetClientRect( hwnd, &rect_dst, NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) );
    NtUserMapWindowPoints( hwnd, toplevel, (POINT *)&rect_dst, 2, NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) );

    if ((data = get_win_data( toplevel )))
    {
        OffsetRect( &rect_dst, data->rects.client.left - data->rects.visible.left,
                    data->rects.client.top - data->rects.visible.top );
        release_win_data( data );
    }

    if (get_dc_drawable( gl->hdc_dst, &rect ) != window || !EqualRect( &rect, &rect_dst ))
        set_dc_drawable( gl->hdc_dst, window, &rect_dst, IncludeInferiors );
    if (region) NtGdiExtSelectClipRgn( gl->hdc_dst, region, RGN_COPY );

    NtGdiStretchBlt( gl->hdc_dst, 0, 0, rect_dst.right - rect_dst.left, rect_dst.bottom - rect_dst.top,
                     gl->hdc_src, 0, 0, gl->rect.right, gl->rect.bottom, SRCCOPY, 0 );

    if (region) NtGdiDeleteObjectApp( region );
}

static BOOL x11drv_context_flush( void *private, HWND hwnd, HDC hdc, int interval, BOOL finish )
{
    struct gl_drawable *gl;
    struct x11drv_context *ctx = private;

    if (!(gl = get_gl_drawable( hwnd, 0 ))) return FALSE;
    sync_context( ctx );

    pthread_mutex_lock( &context_mutex );
    set_swap_interval( gl, interval );
    pthread_mutex_unlock( &context_mutex );

    if (finish) pglFinish();
    else pglFlush();

    present_gl_drawable( hwnd, ctx->hdc, gl, TRUE, !finish );
    release_gl_drawable( gl );
    return TRUE;
}

/***********************************************************************
 *		X11DRV_wglCreateContextAttribsARB
 */
static BOOL x11drv_context_create( HDC hdc, int format, void *share_private, const int *attribList, void **private )
{
    struct x11drv_context *ret, *hShareContext = share_private;
    int glx_attribs[16] = {0}, *pContextAttribList = glx_attribs;
    int err = 0;

    TRACE("(%p %d %p %p)\n", hdc, format, hShareContext, attribList);

    if ((ret = calloc( 1, sizeof(*ret) )))
    {
        ret->hdc = hdc;
        ret->fmt = &pixel_formats[format - 1];
        if (attribList)
        {
            ret->gl3_context = TRUE;
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
        ret->ctx = create_glxcontext( gdi_display, ret, hShareContext ? hShareContext->ctx : NULL,
                                      attribList ? glx_attribs : NULL );
        XSync(gdi_display, False);
        if ((err = X11DRV_check_error()) || !ret->ctx)
        {
            /* In the future we should convert the GLX error to a win32 one here if needed */
            WARN("Context creation failed (error %#x).\n", err);
            free( ret );
            return FALSE;
        }

        pthread_mutex_lock( &context_mutex );
        list_add_head( &context_list, &ret->entry );
        pthread_mutex_unlock( &context_mutex );
    }

    TRACE( "%p -> %p\n", hdc, ret );
    *private = ret;
    return TRUE;
}

static BOOL x11drv_pbuffer_create( HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                   GLint max_level, GLsizei *width, GLsizei *height, void **private )
{
    const struct glx_pixel_format *fmt;
    int glx_attribs[7], count = 0;
    struct gl_drawable *surface;
    RECT rect;

    TRACE( "hdc %p, format %d, largest %u, texture_format %#x, texture_target %#x, max_level %#x, width %d, height %d, private %p\n",
           hdc, format, largest, texture_format, texture_target, max_level, *width, *height, private );

    /* Convert the WGL pixelformat to a GLX format, if it fails then the format is invalid */
    if (!(fmt = get_pixel_format( gdi_display, format, TRUE /* Offscreen */ )))
    {
        ERR( "(%p): invalid pixel format %d\n", hdc, format );
        return FALSE;
    }

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

    if (!(surface = calloc( 1, sizeof(*surface) ))) return FALSE;
    surface->type = DC_GL_PBUFFER;
    surface->format = fmt;
    surface->ref = 1;

    surface->drawable = pglXCreatePbuffer( gdi_display, fmt->fbconfig, glx_attribs );
    TRACE( "new Pbuffer drawable as %p (%lx)\n", surface, surface->drawable );
    if (!surface->drawable)
    {
        free( surface );
        return FALSE;
    }
    pglXQueryDrawable( gdi_display, surface->drawable, GLX_WIDTH, (unsigned int *)width );
    pglXQueryDrawable( gdi_display, surface->drawable, GLX_HEIGHT, (unsigned int *)height );
    SetRect( &rect, 0, 0, *width, *height );
    set_dc_drawable( hdc, surface->drawable, &rect, IncludeInferiors );

    pthread_mutex_lock( &context_mutex );
    XSaveContext( gdi_display, (XID)hdc, gl_pbuffer_context, (char *)surface );
    pthread_mutex_unlock( &context_mutex );

    *private = surface;
    return TRUE;
}

static BOOL x11drv_pbuffer_destroy( HDC hdc, void *private )
{
    struct gl_drawable *surface = private;

    TRACE( "hdc %p, private %p\n", hdc, surface );

    pthread_mutex_lock( &context_mutex );
    XDeleteContext( gdi_display, (XID)hdc, gl_pbuffer_context );
    pthread_mutex_unlock( &context_mutex );
    release_gl_drawable( surface );

    return GL_TRUE;
}

static BOOL x11drv_pbuffer_updated( HDC hdc, void *private, GLenum cube_face, GLint mipmap_level )
{
    TRACE( "hdc %p, private %p, cube_face %#x, mipmap_level %d\n", hdc, private, cube_face, mipmap_level );
    return GL_TRUE;
}

static UINT x11drv_pbuffer_bind( HDC hdc, void *private, GLenum buffer )
{
    TRACE( "hdc %p, private %p, buffer %#x\n", hdc, private, buffer );
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

/**
 * glxdrv_SwapBuffers
 *
 * Swap the buffers of this DC
 */
static BOOL x11drv_swap_buffers( void *private, HWND hwnd, HDC hdc, int interval )
{
    struct gl_drawable *gl;
    struct x11drv_context *ctx = private;
    INT64 ust, msc, sbc, target_sbc = 0;
    Drawable drawable = 0;

    TRACE("(%p)\n", hdc);

    if (!(gl = get_gl_drawable( hwnd, hdc )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    pthread_mutex_lock( &context_mutex );
    set_swap_interval( gl, interval );
    pthread_mutex_unlock( &context_mutex );

    switch (gl->type)
    {
    case DC_GL_PIXMAP_WIN:
        if (ctx) sync_context( ctx );
        drawable = gl->pixmap;
        if (ctx && pglXCopySubBufferMESA) {
            /* (glX)SwapBuffers has an implicit glFlush effect, however
             * GLX_MESA_copy_sub_buffer doesn't. Make sure GL is flushed before
             * copying */
            pglFlush();
            pglXCopySubBufferMESA( gdi_display, gl->drawable, 0, 0,
                                   gl->rect.right, gl->rect.bottom );
            break;
        }
        if (ctx && pglXSwapBuffersMscOML)
        {
            pglFlush();
            target_sbc = pglXSwapBuffersMscOML( gdi_display, gl->drawable, 0, 0, 0 );
            break;
        }
        pglXSwapBuffers(gdi_display, gl->drawable);
        break;
    case DC_GL_WINDOW:
    case DC_GL_CHILD_WIN:
        if (ctx) sync_context( ctx );
        if (gl->type == DC_GL_CHILD_WIN) drawable = gl->window;
        /* fall through */
    default:
        if (ctx && drawable && pglXSwapBuffersMscOML)
        {
            pglFlush();
            target_sbc = pglXSwapBuffersMscOML( gdi_display, gl->drawable, 0, 0, 0 );
            break;
        }
        pglXSwapBuffers(gdi_display, gl->drawable);
        break;
    }

    if (ctx && drawable && pglXWaitForSbcOML)
        pglXWaitForSbcOML( gdi_display, gl->drawable, target_sbc, &ust, &msc, &sbc );

    present_gl_drawable( hwnd, ctx ? ctx->hdc : hdc, gl, !pglXWaitForSbcOML, FALSE );
    update_gl_drawable_size( gl );
    release_gl_drawable( gl );
    return TRUE;
}

static const struct opengl_driver_funcs x11drv_driver_funcs =
{
    .p_get_proc_address = x11drv_get_proc_address,
    .p_init_pixel_formats = x11drv_init_pixel_formats,
    .p_describe_pixel_format = x11drv_describe_pixel_format,
    .p_init_wgl_extensions = x11drv_init_wgl_extensions,
    .p_set_pixel_format = x11drv_set_pixel_format,
    .p_swap_buffers = x11drv_swap_buffers,
    .p_context_create = x11drv_context_create,
    .p_context_destroy = x11drv_context_destroy,
    .p_context_flush = x11drv_context_flush,
    .p_context_make_current = x11drv_context_make_current,
    .p_pbuffer_create = x11drv_pbuffer_create,
    .p_pbuffer_destroy = x11drv_pbuffer_destroy,
    .p_pbuffer_updated = x11drv_pbuffer_updated,
    .p_pbuffer_bind = x11drv_pbuffer_bind,
};

#else  /* no OpenGL includes */

/**********************************************************************
 *           X11DRV_OpenglInit
 */
UINT X11DRV_OpenGLInit( UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs )
{
    return STATUS_NOT_IMPLEMENTED;
}

void sync_gl_drawable( HWND hwnd, BOOL known_child )
{
}

void set_gl_drawable_parent( HWND hwnd, HWND parent )
{
}

void destroy_gl_drawable( HWND hwnd )
{
}

#endif /* defined(SONAME_LIBGL) */
