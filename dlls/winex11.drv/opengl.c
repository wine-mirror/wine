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

#include "x11drv.h"
#include "xcomposite.h"
#include "winternl.h"
#include "wine/debug.h"

#ifdef SONAME_LIBGL

WINE_DEFAULT_DEBUG_CHANNEL(wgl);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#include "wine/wgl.h"
#include "wine/wgl_driver.h"

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


static char *glExtensions;
static const char *glxExtensions;
static char wglExtensions[4096];
static int glxVersion[2];
static int glx_opcode;

struct wgl_pixel_format
{
    GLXFBConfig fbconfig;
    XVisualInfo *visual;
    int         fmt_id;
    int         render_type;
    DWORD       dwFlags; /* We store some PFD_* flags in here for emulated bitmap formats */
};

struct wgl_context
{
    HDC hdc;
    BOOL has_been_current;
    BOOL sharing;
    BOOL gl3_context;
    const struct wgl_pixel_format *fmt;
    int numAttribs; /* This is needed for delaying wglCreateContextAttribsARB */
    int attribList[16]; /* This is needed for delaying wglCreateContextAttribsARB */
    GLXContext ctx;
    struct gl_drawable *drawables[2];
    struct gl_drawable *new_drawables[2];
    BOOL refresh_drawables;
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
    GLXDrawable                    drawable;     /* drawable for rendering with GL */
    Window                         window;       /* window if drawable is a GLXWindow */
    Pixmap                         pixmap;       /* base pixmap if drawable is a GLXPixmap */
    const struct wgl_pixel_format *format;       /* pixel format for the drawable */
    SIZE                           pixmap_size;  /* pixmap size for GLXPixmap drawables */
    int                            swap_interval;
    BOOL                           refresh_swap_interval;
    BOOL                           mutable_pf;
};

struct wgl_pbuffer
{
    struct gl_drawable *gl;
    const struct wgl_pixel_format* fmt;
    int        width;
    int        height;
    int*       attribList;
    int        use_render_texture; /* This is also the internal texture format */
    int        texture_bind_target;
    int        texture_bpp;
    GLint      texture_format;
    GLuint     texture_target;
    GLenum     texture_type;
    GLuint     texture;
    int        texture_level;
    GLXContext tmp_context;
    GLXContext prev_context;
    struct list entry;
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
static struct list pbuffer_list = LIST_INIT( pbuffer_list );
static struct wgl_pixel_format *pixel_formats;
static int nb_pixel_formats, nb_onscreen_formats;
static BOOL use_render_texture_emulation = TRUE;

/* Selects the preferred GLX swap control method for use by wglSwapIntervalEXT */
static enum glx_swap_control_method swap_control_method = GLX_SWAP_CONTROL_NONE;
/* Set when GLX_EXT_swap_control_tear is supported, requires GLX_SWAP_CONTROL_EXT */
static BOOL has_swap_control_tear = FALSE;
static BOOL has_swap_method = FALSE;

static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;

static const BOOL is_win64 = sizeof(void *) > sizeof(int);

static struct opengl_funcs opengl_funcs;

#define USE_GL_FUNC(name) #name,
static const char *opengl_func_names[] = { ALL_WGL_FUNCS };
#undef USE_GL_FUNC

static void X11DRV_WineGL_LoadExtensions(void);
static void init_pixel_formats( Display *display );
static BOOL glxRequireVersion(int requiredVersion);

static void dump_PIXELFORMATDESCRIPTOR(const PIXELFORMATDESCRIPTOR *ppfd) {
  TRACE( "size %u version %u flags %u type %u color %u %u,%u,%u,%u "
         "accum %u depth %u stencil %u aux %u ",
         ppfd->nSize, ppfd->nVersion, (int)ppfd->dwFlags, ppfd->iPixelType,
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

#define PUSH1(attribs,att)        do { attribs[nAttribs++] = (att); } while (0)
#define PUSH2(attribs,att,value)  do { attribs[nAttribs++] = (att); attribs[nAttribs++] = (value); } while(0)

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

static void wglFinish(void);
static void wglFlush(void);
static const GLubyte *wglGetString(GLenum name);

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
    static const char legacy_extensions[] = " WGL_EXT_extensions_string WGL_EXT_swap_control";

    int screen = DefaultScreen(gdi_display);
    Window win = 0, root = 0;
    const char *gl_version;
    const char *gl_renderer;
    const char* str;
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
    gl_renderer = (const char *)opengl_funcs.gl.p_glGetString(GL_RENDERER);
    gl_version  = (const char *)opengl_funcs.gl.p_glGetString(GL_VERSION);
    str = (const char *) opengl_funcs.gl.p_glGetString(GL_EXTENSIONS);
    glExtensions = malloc( strlen(str) + sizeof(legacy_extensions) );
    strcpy(glExtensions, str);
    strcat(glExtensions, legacy_extensions);

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

static void init_opengl(void)
{
    int error_base, event_base;
    unsigned int i;

    /* No need to load any other libraries as according to the ABI, libGL should be self-sufficient
       and include all dependencies */
    opengl_handle = dlopen( SONAME_LIBGL, RTLD_NOW | RTLD_GLOBAL );
    if (opengl_handle == NULL)
    {
        ERR( "Failed to load libGL: %s\n", dlerror() );
        ERR( "OpenGL support is disabled.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE( opengl_func_names ); i++)
    {
        if (!(((void **)&opengl_funcs.gl)[i] = dlsym( opengl_handle, opengl_func_names[i] )))
        {
            ERR( "%s not found in libGL, disabling OpenGL.\n", opengl_func_names[i] );
            goto failed;
        }
    }

    /* redirect some standard OpenGL functions */
#define REDIRECT(func) \
    do { p##func = opengl_funcs.gl.p_##func; opengl_funcs.gl.p_##func = w##func; } while(0)
    REDIRECT( glFinish );
    REDIRECT( glFlush );
    REDIRECT( glGetString );
#undef REDIRECT

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

    X11DRV_WineGL_LoadExtensions();
    init_pixel_formats( gdi_display );
    return;

failed:
    dlclose(opengl_handle);
    opengl_handle = NULL;
}

static BOOL has_opengl(void)
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;

    return !pthread_once( &init_once, init_opengl );
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

static int ConvertAttribWGLtoGLX(const int* iWGLAttr, int* oGLXAttr, struct wgl_pbuffer* pbuf) {
  int nAttribs = 0;
  unsigned cur = 0; 
  int attr, pop;
  int drawattrib = 0;
  int nvfloatattrib = GLX_DONT_CARE;
  int pixelattrib = GLX_DONT_CARE;

  /* The list of WGL attributes is allowed to be NULL. We don't return here for NULL
   * because we need to do fixups for GLX_DRAWABLE_TYPE/GLX_RENDER_TYPE/GLX_FLOAT_COMPONENTS_NV. */
  while (iWGLAttr && 0 != iWGLAttr[cur]) {
    attr = iWGLAttr[cur];
    TRACE("pAttr[%d] = %x\n", cur, attr);
    pop = iWGLAttr[++cur];

    switch (attr) {
    case WGL_AUX_BUFFERS_ARB:
      PUSH2(oGLXAttr, GLX_AUX_BUFFERS, pop);
      TRACE("pAttr[%d] = GLX_AUX_BUFFERS: %d\n", cur, pop);
      break;
    case WGL_COLOR_BITS_ARB:
      PUSH2(oGLXAttr, GLX_BUFFER_SIZE, pop);
      TRACE("pAttr[%d] = GLX_BUFFER_SIZE: %d\n", cur, pop);
      break;
    case WGL_BLUE_BITS_ARB:
      PUSH2(oGLXAttr, GLX_BLUE_SIZE, pop);
      TRACE("pAttr[%d] = GLX_BLUE_SIZE: %d\n", cur, pop);
      break;
    case WGL_RED_BITS_ARB:
      PUSH2(oGLXAttr, GLX_RED_SIZE, pop);
      TRACE("pAttr[%d] = GLX_RED_SIZE: %d\n", cur, pop);
      break;
    case WGL_GREEN_BITS_ARB:
      PUSH2(oGLXAttr, GLX_GREEN_SIZE, pop);
      TRACE("pAttr[%d] = GLX_GREEN_SIZE: %d\n", cur, pop);
      break;
    case WGL_ALPHA_BITS_ARB:
      PUSH2(oGLXAttr, GLX_ALPHA_SIZE, pop);
      TRACE("pAttr[%d] = GLX_ALPHA_SIZE: %d\n", cur, pop);
      break;
    case WGL_DEPTH_BITS_ARB:
      PUSH2(oGLXAttr, GLX_DEPTH_SIZE, pop);
      TRACE("pAttr[%d] = GLX_DEPTH_SIZE: %d\n", cur, pop);
      break;
    case WGL_STENCIL_BITS_ARB:
      PUSH2(oGLXAttr, GLX_STENCIL_SIZE, pop);
      TRACE("pAttr[%d] = GLX_STENCIL_SIZE: %d\n", cur, pop);
      break;
    case WGL_DOUBLE_BUFFER_ARB:
      PUSH2(oGLXAttr, GLX_DOUBLEBUFFER, pop);
      TRACE("pAttr[%d] = GLX_DOUBLEBUFFER: %d\n", cur, pop);
      break;
    case WGL_STEREO_ARB:
      PUSH2(oGLXAttr, GLX_STEREO, pop);
      TRACE("pAttr[%d] = GLX_STEREO: %d\n", cur, pop);
      break;

    case WGL_PIXEL_TYPE_ARB:
      TRACE("pAttr[%d] = WGL_PIXEL_TYPE_ARB: %d\n", cur, pop);
      switch (pop) {
      case WGL_TYPE_COLORINDEX_ARB: pixelattrib = GLX_COLOR_INDEX_BIT; break ;
      case WGL_TYPE_RGBA_ARB: pixelattrib = GLX_RGBA_BIT; break ;
      /* This is the same as WGL_TYPE_RGBA_FLOAT_ATI but the GLX constants differ, only the ARB GLX one is widely supported so use that */
      case WGL_TYPE_RGBA_FLOAT_ATI: pixelattrib = GLX_RGBA_FLOAT_BIT; break ;
      case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT: pixelattrib = GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT; break ;
      default:
        ERR("unexpected PixelType(%x)\n", pop);	
      }
      break;

    case WGL_SUPPORT_GDI_ARB:
      /* This flag is set in a pixel format */
      TRACE("pAttr[%d] = WGL_SUPPORT_GDI_ARB: %d\n", cur, pop);
      break;

    case WGL_DRAW_TO_BITMAP_ARB:
      /* This flag is set in a pixel format */
      TRACE("pAttr[%d] = WGL_DRAW_TO_BITMAP_ARB: %d\n", cur, pop);
      break;

    case WGL_DRAW_TO_WINDOW_ARB:
      TRACE("pAttr[%d] = WGL_DRAW_TO_WINDOW_ARB: %d\n", cur, pop);
      /* GLX_DRAWABLE_TYPE flags need to be OR'd together. See below. */
      if (pop) {
        drawattrib |= GLX_WINDOW_BIT;
      }
      break;

    case WGL_DRAW_TO_PBUFFER_ARB:
      TRACE("pAttr[%d] = WGL_DRAW_TO_PBUFFER_ARB: %d\n", cur, pop);
      /* GLX_DRAWABLE_TYPE flags need to be OR'd together. See below. */
      if (pop) {
        drawattrib |= GLX_PBUFFER_BIT;
      }
      break;

    case WGL_ACCELERATION_ARB:
      /* This flag is set in a pixel format */
      TRACE("pAttr[%d] = WGL_ACCELERATION_ARB: %d\n", cur, pop);
      break;

    case WGL_SUPPORT_OPENGL_ARB:
      /** nothing to do, if we are here, supposing support Accelerated OpenGL */
      TRACE("pAttr[%d] = WGL_SUPPORT_OPENGL_ARB: %d\n", cur, pop);
      break;

    case WGL_SWAP_METHOD_ARB:
      TRACE("pAttr[%d] = WGL_SWAP_METHOD_ARB: %#x\n", cur, pop);
      if (has_swap_method)
      {
          switch (pop)
          {
          case WGL_SWAP_EXCHANGE_ARB:
              pop = GLX_SWAP_EXCHANGE_OML;
              break;
          case WGL_SWAP_COPY_ARB:
              pop = GLX_SWAP_COPY_OML;
              break;
          case WGL_SWAP_UNDEFINED_ARB:
              pop = GLX_SWAP_UNDEFINED_OML;
              break;
          default:
              ERR("Unexpected swap method %#x.\n", pop);
              pop = GLX_DONT_CARE;
          }
          PUSH2(oGLXAttr, GLX_SWAP_METHOD_OML, pop);
      }
      else
      {
          WARN("GLX_OML_swap_method not supported, ignoring attribute.\n");
      }
      break;

    case WGL_PBUFFER_LARGEST_ARB:
      PUSH2(oGLXAttr, GLX_LARGEST_PBUFFER, pop);
      TRACE("pAttr[%d] = GLX_LARGEST_PBUFFER: %x\n", cur, pop);
      break;

    case WGL_SAMPLE_BUFFERS_ARB:
      PUSH2(oGLXAttr, GLX_SAMPLE_BUFFERS_ARB, pop);
      TRACE("pAttr[%d] = GLX_SAMPLE_BUFFERS_ARB: %x\n", cur, pop);
      break;

    case WGL_SAMPLES_ARB:
      PUSH2(oGLXAttr, GLX_SAMPLES_ARB, pop);
      TRACE("pAttr[%d] = GLX_SAMPLES_ARB: %x\n", cur, pop);
      break;

    case WGL_TEXTURE_FORMAT_ARB:
    case WGL_TEXTURE_TARGET_ARB:
    case WGL_MIPMAP_TEXTURE_ARB:
      TRACE("WGL_render_texture Attributes: %x as %x\n", iWGLAttr[cur - 1], iWGLAttr[cur]);
      if (NULL == pbuf) {
        ERR("trying to use GLX_Pbuffer Attributes without Pbuffer (was %x)\n", iWGLAttr[cur]);
      }
      if (!use_render_texture_emulation) {
        if (WGL_NO_TEXTURE_ARB != pop) {
          ERR("trying to use WGL_render_texture Attributes without support (was %x)\n", iWGLAttr[cur]);
          return -1; /** error: don't support it */
        } else {
          drawattrib |= GLX_PBUFFER_BIT;
        }
      }
      break ;
    case WGL_FLOAT_COMPONENTS_NV:
      nvfloatattrib = pop;
      TRACE("pAttr[%d] = WGL_FLOAT_COMPONENTS_NV: %x\n", cur, nvfloatattrib);
      break ;
    case WGL_BIND_TO_TEXTURE_DEPTH_NV:
    case WGL_BIND_TO_TEXTURE_RGB_ARB:
    case WGL_BIND_TO_TEXTURE_RGBA_ARB:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV:
      /** cannot be converted, see direct handling on 
       *   - wglGetPixelFormatAttribivARB
       *  TODO: wglChoosePixelFormat
       */
      break ;
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT:
      PUSH2(oGLXAttr, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, pop);
      TRACE("pAttr[%d] = GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT: %x\n", cur, pop);
      break ;

    case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT:
      PUSH2(oGLXAttr, GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT, pop);
      TRACE("pAttr[%d] = GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT: %x\n", cur, pop);
      break ;
    default:
      FIXME("unsupported %x WGL Attribute\n", attr);
      break;
    }
    ++cur;
  }

  /* By default glXChooseFBConfig defaults to GLX_WINDOW_BIT. wglChoosePixelFormatARB searches through
   * all formats. Unless drawattrib is set to a non-zero value override it with GLX_DONT_CARE, so that
   * pixmap and pbuffer formats appear as well. */
  if (!drawattrib) drawattrib = GLX_DONT_CARE;
  PUSH2(oGLXAttr, GLX_DRAWABLE_TYPE, drawattrib);
  TRACE("pAttr[?] = GLX_DRAWABLE_TYPE: %#x\n", drawattrib);

  /* By default glXChooseFBConfig uses GLX_RGBA_BIT as the default value. Since wglChoosePixelFormatARB
   * searches in all formats we have to do the same. For this reason we set GLX_RENDER_TYPE to
   * GLX_DONT_CARE unless it is overridden. */
  PUSH2(oGLXAttr, GLX_RENDER_TYPE, pixelattrib);
  TRACE("pAttr[?] = GLX_RENDER_TYPE: %#x\n", pixelattrib);

  /* Set GLX_FLOAT_COMPONENTS_NV all the time */
  if (has_extension(glxExtensions, "GLX_NV_float_buffer")) {
    PUSH2(oGLXAttr, GLX_FLOAT_COMPONENTS_NV, nvfloatattrib);
    TRACE("pAttr[?] = GLX_FLOAT_COMPONENTS_NV: %#x\n", nvfloatattrib);
  }

  return nAttribs;
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

static void init_pixel_formats( Display *display )
{
    struct wgl_pixel_format *list;
    int size = 0, onscreen_size = 0;
    int fmt_id, nCfgs, i, run, bmp_formats;
    GLXFBConfig* cfgs;
    XVisualInfo *visinfo;

    cfgs = pglXGetFBConfigs(display, DefaultScreen(display), &nCfgs);
    if (NULL == cfgs || 0 == nCfgs) {
        if(cfgs != NULL) XFree(cfgs);
        ERR("glXChooseFBConfig returns NULL\n");
        return;
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
        if(check_fbconfig_bitmap_capability(display, cfgs[i]))
            bmp_formats++;
    }
    TRACE("Found %d bitmap capable fbconfigs\n", bmp_formats);

    list = calloc( 1, (nCfgs + bmp_formats) * sizeof(*list) );

    /* Fill the pixel format list. Put onscreen formats at the top and offscreen ones at the bottom.
     * Do this as GLX doesn't guarantee that the list is sorted */
    for(run=0; run < 2; run++)
    {
        for(i=0; i<nCfgs; i++) {
            pglXGetFBConfigAttrib(display, cfgs[i], GLX_FBCONFIG_ID, &fmt_id);
            visinfo = pglXGetVisualFromFBConfig(display, cfgs[i]);

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
                list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                list[size].dwFlags = 0;
                size++;
                onscreen_size++;

                /* Clone a format if it is bitmap capable for indirect rendering to bitmaps */
                if(check_fbconfig_bitmap_capability(display, cfgs[i]))
                {
                    TRACE("Found bitmap capable format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                    list[size].fbconfig = cfgs[i];
                    list[size].visual = visinfo;
                    list[size].fmt_id = fmt_id;
                    list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
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
                list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
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
}

static inline BOOL is_valid_pixel_format( int format )
{
    return format > 0 && format <= nb_pixel_formats;
}

static inline BOOL is_onscreen_pixel_format( int format )
{
    return format > 0 && format <= nb_onscreen_formats;
}

static inline int pixel_format_index( const struct wgl_pixel_format *format )
{
    return format - pixel_formats + 1;
}

/* GLX can advertise dozens of different pixelformats including offscreen and onscreen ones.
 * In our WGL implementation we only support a subset of these formats namely the format of
 * Wine's main visual and offscreen formats (if they are available).
 * This function converts a WGL format to its corresponding GLX one.
 */
static const struct wgl_pixel_format *get_pixel_format(Display *display, int iPixelFormat, BOOL AllowOffscreen)
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
        XDestroyWindow( gdi_display, gl->window );
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
    free( gl );
}

/* Mark any allocated context using the glx drawable 'old' to use 'new' */
static void mark_drawable_dirty( struct gl_drawable *old, struct gl_drawable *new )
{
    struct wgl_context *ctx;

    pthread_mutex_lock( &context_mutex );
    LIST_FOR_EACH_ENTRY( ctx, &context_list, struct wgl_context, entry )
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
static inline void sync_context(struct wgl_context *context)
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

static BOOL set_swap_interval(GLXDrawable drawable, int interval)
{
    BOOL ret = TRUE;

    switch (swap_control_method)
    {
    case GLX_SWAP_CONTROL_EXT:
        X11DRV_expect_error(gdi_display, GLXErrorHandler, NULL);
        pglXSwapIntervalEXT(gdi_display, drawable, interval);
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

static GLXContext create_glxcontext(Display *display, struct wgl_context *context, GLXContext shareList)
{
    GLXContext ctx;

    if(context->gl3_context)
    {
        if(context->numAttribs)
            ctx = pglXCreateContextAttribsARB(gdi_display, context->fmt->fbconfig, shareList, GL_TRUE, context->attribList);
        else
            ctx = pglXCreateContextAttribsARB(gdi_display, context->fmt->fbconfig, shareList, GL_TRUE, NULL);
    }
    else if(context->fmt->visual)
        ctx = pglXCreateContext(gdi_display, context->fmt->visual, shareList, GL_TRUE);
    else /* Create a GLX Context for a pbuffer */
        ctx = pglXCreateNewContext(gdi_display, context->fmt->fbconfig, context->fmt->render_type, shareList, TRUE);

    return ctx;
}


/***********************************************************************
 *              create_gl_drawable
 */
static struct gl_drawable *create_gl_drawable( HWND hwnd, const struct wgl_pixel_format *format, BOOL known_child,
                                               BOOL mutable_pf )
{
    struct gl_drawable *gl, *prev;
    XVisualInfo *visual = format->visual;
    RECT rect;
    int width, height;

    NtUserGetClientRect( hwnd, &rect );
    width  = min( max( 1, rect.right ), 65535 );
    height = min( max( 1, rect.bottom ), 65535 );

    if (!(gl = calloc( 1, sizeof(*gl) ))) return NULL;

    /* Default GLX and WGL swap interval is 1, but in case of glXSwapIntervalSGI
     * there is no way to query it, so we have to store it here.
     */
    gl->swap_interval = 1;
    gl->refresh_swap_interval = TRUE;
    gl->format = format;
    gl->ref = 1;
    gl->mutable_pf = mutable_pf;

    if (!known_child && !NtUserGetWindowRelative( hwnd, GW_CHILD ) &&
        NtUserGetAncestor( hwnd, GA_PARENT ) == NtUserGetDesktopWindow())  /* childless top-level window */
    {
        gl->type = DC_GL_WINDOW;
        gl->window = create_client_window( hwnd, visual );
        if (gl->window)
            gl->drawable = pglXCreateWindow( gdi_display, gl->format->fbconfig, gl->window, NULL );
        TRACE( "%p created client %lx drawable %lx\n", hwnd, gl->window, gl->drawable );
    }
#ifdef SONAME_LIBXCOMPOSITE
    else if(usexcomposite)
    {
        gl->type = DC_GL_CHILD_WIN;
        gl->window = create_client_window( hwnd, visual );
        if (gl->window)
        {
            gl->drawable = pglXCreateWindow( gdi_display, gl->format->fbconfig, gl->window, NULL );
            pXCompositeRedirectWindow( gdi_display, gl->window, CompositeRedirectManual );
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
            gl->pixmap_size.cx = width;
            gl->pixmap_size.cy = height;
        }
    }

    if (!gl->drawable)
    {
        free( gl );
        return NULL;
    }

    pthread_mutex_lock( &context_mutex );
    if (!XFindContext( gdi_display, (XID)hwnd, gl_hwnd_context, (char **)&prev ))
    {
        gl->swap_interval = prev->swap_interval;
        release_gl_drawable( prev );
    }
    XSaveContext( gdi_display, (XID)hwnd, gl_hwnd_context, (char *)grab_gl_drawable(gl) );
    pthread_mutex_unlock( &context_mutex );
    return gl;
}


/***********************************************************************
 *              set_win_format
 */
static BOOL set_win_format( HWND hwnd, const struct wgl_pixel_format *format, BOOL internal )
{
    struct gl_drawable *old, *gl;

    if (!format->visual) return FALSE;

    old = get_gl_drawable( hwnd, 0 );

    if (!(gl = create_gl_drawable( hwnd, format, FALSE, internal )))
    {
        release_gl_drawable( old );
        return FALSE;
    }

    TRACE( "created GL drawable %lx for win %p %s\n",
           gl->drawable, hwnd, debugstr_fbconfig( format->fbconfig ));

    if (old)
        mark_drawable_dirty( old, gl );

    XFlush( gdi_display );
    release_gl_drawable( gl );
    release_gl_drawable( old );

    win32u_set_window_pixel_format( hwnd, pixel_format_index( format ), internal );
    return TRUE;
}


static BOOL set_pixel_format( HDC hdc, int format, BOOL internal )
{
    const struct wgl_pixel_format *fmt;
    int value;
    HWND hwnd = NtUserWindowFromDC( hdc );
    int prev;

    TRACE("(%p,%d)\n", hdc, format);

    if (!hwnd || hwnd == NtUserGetDesktopWindow())
    {
        WARN( "not a valid window DC %p/%p\n", hdc, hwnd );
        return FALSE;
    }

    fmt = get_pixel_format(gdi_display, format, FALSE /* Offscreen */);
    if (!fmt)
    {
        ERR( "Invalid format %d\n", format );
        return FALSE;
    }

    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &value);
    if (!(value & GLX_WINDOW_BIT))
    {
        WARN( "Pixel format %d is not compatible for window rendering\n", format );
        return FALSE;
    }

    /* Even for internal pixel format fail setting it if the app has already set a
     * different pixel format. Let wined3d create a backup GL context instead.
     * Switching pixel format involves drawable recreation and is much more expensive
     * than blitting from backup context. */
    if ((prev = win32u_get_window_pixel_format( hwnd )))
        return prev == format;

    return set_win_format( hwnd, fmt, internal );
}


/***********************************************************************
 *              sync_gl_drawable
 */
void sync_gl_drawable( HWND hwnd, BOOL known_child )
{
    struct gl_drawable *old, *new;

    if (!(old = get_gl_drawable( hwnd, 0 ))) return;

    switch (old->type)
    {
    case DC_GL_WINDOW:
        if (!known_child) break; /* Still a childless top-level window */
        /* fall through */
    case DC_GL_PIXMAP_WIN:
        if (!(new = create_gl_drawable( hwnd, old->format, known_child, old->mutable_pf ))) break;
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

    if ((new = create_gl_drawable( hwnd, old->format, FALSE, old->mutable_pf )))
    {
        mark_drawable_dirty( old, new );
        release_gl_drawable( new );
    }
    else
    {
        destroy_gl_drawable( hwnd );
        win32u_set_window_pixel_format( hwnd, 0, FALSE );
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


/**
 * glxdrv_DescribePixelFormat
 *
 * Get the pixel-format descriptor associated to the given id
 */
static int describe_pixel_format( int iPixelFormat, PIXELFORMATDESCRIPTOR *ppfd, BOOL allow_offscreen )
{
  /*XVisualInfo *vis;*/
  int value;
  int rb,gb,bb,ab;
  const struct wgl_pixel_format *fmt;

  if (!has_opengl()) return 0;

  /* Look for the iPixelFormat in our list of supported formats. If it is supported we get the index in the FBConfig table and the number of supported formats back */
  fmt = get_pixel_format(gdi_display, iPixelFormat, allow_offscreen);
  if (!fmt) {
      WARN("unexpected format %d\n", iPixelFormat);
      return 0;
  }

  memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
  ppfd->nVersion = 1;

  /* These flags are always the same... */
  ppfd->dwFlags = PFD_SUPPORT_OPENGL;
  /* Now the flags extracted from the Visual */

  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &value);
  if(value & GLX_WINDOW_BIT)
      ppfd->dwFlags |= PFD_DRAW_TO_WINDOW;

  /* On Windows bitmap rendering is only offered using the GDI Software renderer. We reserve some formats (see get_formats for more info)
   * for bitmap rendering since we require indirect rendering for this. Further pixel format logs of a GeforceFX, Geforce8800GT, Radeon HD3400 and a
   * Radeon 9000 indicated that all bitmap formats have PFD_SUPPORT_GDI. Except for 2 formats on the Radeon 9000 none of the hw accelerated formats
   * offered the GDI bit either. */
  ppfd->dwFlags |= fmt->dwFlags & (PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI);

  /* PFD_GENERIC_FORMAT - gdi software rendering
   * PFD_GENERIC_ACCELERATED - some parts are accelerated by a display driver (MCD e.g. 3dfx minigl)
   * none set - full hardware accelerated by a ICD
   *
   * We only set PFD_GENERIC_FORMAT on bitmap formats (see get_formats) as that's what ATI and Nvidia Windows drivers do  */
  ppfd->dwFlags |= fmt->dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED);

  if (!(ppfd->dwFlags & PFD_GENERIC_FORMAT))
    ppfd->dwFlags |= PFD_SUPPORT_COMPOSITION;

  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DOUBLEBUFFER, &value);
  if (value) {
      ppfd->dwFlags |= PFD_DOUBLEBUFFER;
      ppfd->dwFlags &= ~PFD_SUPPORT_GDI;
  }
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_STEREO, &value); if (value) ppfd->dwFlags |= PFD_STEREO;

  /* Pixel type */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_RENDER_TYPE, &value);
  if (value & GLX_RGBA_BIT)
    ppfd->iPixelType = PFD_TYPE_RGBA;
  else
    ppfd->iPixelType = PFD_TYPE_COLORINDEX;

  /* Color bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_BUFFER_SIZE, &value);
  ppfd->cColorBits = value;

  /* Red, green, blue and alpha bits / shifts */
  if (ppfd->iPixelType == PFD_TYPE_RGBA) {
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_RED_SIZE, &rb);
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_GREEN_SIZE, &gb);
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_BLUE_SIZE, &bb);
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ALPHA_SIZE, &ab);

    ppfd->cBlueBits = bb;
    ppfd->cBlueShift = 0;
    ppfd->cGreenBits = gb;
    ppfd->cGreenShift = bb;
    ppfd->cRedBits = rb;
    ppfd->cRedShift = gb + bb;
    ppfd->cAlphaBits = ab;
    if (ab)
        ppfd->cAlphaShift = rb + gb + bb;
    else
        ppfd->cAlphaShift = 0;
  } else {
    ppfd->cRedBits = 0;
    ppfd->cRedShift = 0;
    ppfd->cBlueBits = 0;
    ppfd->cBlueShift = 0;
    ppfd->cGreenBits = 0;
    ppfd->cGreenShift = 0;
    ppfd->cAlphaBits = 0;
    ppfd->cAlphaShift = 0;
  }

  /* Accum RGBA bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_RED_SIZE, &rb);
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_GREEN_SIZE, &gb);
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_BLUE_SIZE, &bb);
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_ALPHA_SIZE, &ab);

  ppfd->cAccumBits = rb+gb+bb+ab;
  ppfd->cAccumRedBits = rb;
  ppfd->cAccumGreenBits = gb;
  ppfd->cAccumBlueBits = bb;
  ppfd->cAccumAlphaBits = ab;

  /* Aux bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_AUX_BUFFERS, &value);
  ppfd->cAuxBuffers = value;

  /* Depth bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DEPTH_SIZE, &value);
  ppfd->cDepthBits = value;

  /* stencil bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_STENCIL_SIZE, &value);
  ppfd->cStencilBits = value;

  ppfd->iLayerType = PFD_MAIN_PLANE;

  if (TRACE_ON(wgl)) {
    dump_PIXELFORMATDESCRIPTOR(ppfd);
  }

  return nb_onscreen_formats;
}

/**
 * glxdrv_DescribePixelFormat
 *
 * Get the pixel-format descriptor associated to the given id
 */
static int glxdrv_wglDescribePixelFormat( HDC hdc, int iPixelFormat,
                                          UINT nBytes, PIXELFORMATDESCRIPTOR *ppfd)
{
  TRACE("(%p,%d,%d,%p)\n", hdc, iPixelFormat, nBytes, ppfd);

  if (!ppfd) return nb_onscreen_formats;

  if (nBytes < sizeof(PIXELFORMATDESCRIPTOR))
  {
    ERR("Wrong structure size !\n");
    /* Should set error */
    return 0;
  }

  return describe_pixel_format(iPixelFormat, ppfd, FALSE);
}

/***********************************************************************
 *		glxdrv_wglGetPixelFormat
 */
static int glxdrv_wglGetPixelFormat( HDC hdc )
{
    struct gl_drawable *gl;
    int ret = 0;
    HWND hwnd;

    if ((hwnd = NtUserWindowFromDC( hdc )))
        return win32u_get_window_pixel_format( hwnd );

    if ((gl = get_gl_drawable( NULL, hdc )))
    {
        ret = pixel_format_index( gl->format );
        /* Offscreen formats can't be used with traditional WGL calls.
         * As has been verified on Windows GetPixelFormat doesn't fail but returns iPixelFormat=1. */
        if (!is_onscreen_pixel_format( ret )) ret = 1;
        release_gl_drawable( gl );
    }
    TRACE( "%p -> %d\n", hdc, ret );
    return ret;
}

/***********************************************************************
 *		glxdrv_wglSetPixelFormat
 */
static BOOL glxdrv_wglSetPixelFormat( HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd )
{
    return set_pixel_format(hdc, iPixelFormat, FALSE);
}

/***********************************************************************
 *		glxdrv_wglCopyContext
 */
static BOOL glxdrv_wglCopyContext(struct wgl_context *src, struct wgl_context *dst, UINT mask)
{
    TRACE("%p -> %p mask %#x\n", src, dst, mask);

    X11DRV_expect_error( gdi_display, GLXErrorHandler, NULL );
    pglXCopyContext( gdi_display, src->ctx, dst->ctx, mask );
    XSync( gdi_display, False );
    if (X11DRV_check_error())
    {
        static unsigned int once;

        if (!once++)
        {
            ERR("glXCopyContext failed. glXCopyContext() for direct rendering contexts not "
                "implemented in the host graphics driver?\n");
        }
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		glxdrv_wglCreateContext
 */
static struct wgl_context *glxdrv_wglCreateContext( HDC hdc )
{
    struct wgl_context *ret;
    struct gl_drawable *gl;

    if (!(gl = get_gl_drawable( NtUserWindowFromDC( hdc ), hdc )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return NULL;
    }

    if ((ret = calloc( 1, sizeof(*ret) )))
    {
        ret->hdc = hdc;
        ret->fmt = gl->format;
        ret->ctx = create_glxcontext(gdi_display, ret, NULL);
        pthread_mutex_lock( &context_mutex );
        list_add_head( &context_list, &ret->entry );
        pthread_mutex_unlock( &context_mutex );
    }
    release_gl_drawable( gl );
    TRACE( "%p -> %p\n", hdc, ret );
    return ret;
}

/***********************************************************************
 *		glxdrv_wglDeleteContext
 */
static BOOL glxdrv_wglDeleteContext(struct wgl_context *ctx)
{
    struct wgl_pbuffer *pb;

    TRACE("(%p)\n", ctx);

    pthread_mutex_lock( &context_mutex );
    list_remove( &ctx->entry );
    LIST_FOR_EACH_ENTRY( pb, &pbuffer_list, struct wgl_pbuffer, entry )
    {
        if (pb->prev_context == ctx->ctx) {
            pglXDestroyContext(gdi_display, pb->tmp_context);
            pb->prev_context = pb->tmp_context = NULL;
        }
    }
    pthread_mutex_unlock( &context_mutex );

    if (ctx->ctx) pglXDestroyContext( gdi_display, ctx->ctx );
    release_gl_drawable( ctx->drawables[0] );
    release_gl_drawable( ctx->drawables[1] );
    release_gl_drawable( ctx->new_drawables[0] );
    release_gl_drawable( ctx->new_drawables[1] );
    free( ctx );
    return TRUE;
}

/***********************************************************************
 *		glxdrv_wglGetProcAddress
 */
static PROC glxdrv_wglGetProcAddress(LPCSTR lpszProc)
{
    if (!strncmp(lpszProc, "wgl", 3)) return NULL;
    return pglXGetProcAddressARB((const GLubyte*)lpszProc);
}

static void set_context_drawables( struct wgl_context *ctx, struct gl_drawable *draw,
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

/***********************************************************************
 *		glxdrv_wglMakeCurrent
 */
static BOOL glxdrv_wglMakeCurrent(HDC hdc, struct wgl_context *ctx)
{
    BOOL ret = FALSE;
    struct gl_drawable *gl;

    TRACE("(%p,%p)\n", hdc, ctx);

    if (!ctx)
    {
        pglXMakeCurrent(gdi_display, None, NULL);
        NtCurrentTeb()->glContext = NULL;
        return TRUE;
    }

    if ((gl = get_gl_drawable( NtUserWindowFromDC( hdc ), hdc )))
    {
        if (ctx->fmt != gl->format)
        {
            WARN( "mismatched pixel format hdc %p %p ctx %p %p\n", hdc, gl->format, ctx, ctx->fmt );
            RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
            goto done;
        }

        TRACE("hdc %p drawable %lx fmt %p ctx %p %s\n", hdc, gl->drawable, gl->format, ctx->ctx,
              debugstr_fbconfig( gl->format->fbconfig ));

        pthread_mutex_lock( &context_mutex );
        ret = pglXMakeCurrent(gdi_display, gl->drawable, ctx->ctx);
        if (ret)
        {
            NtCurrentTeb()->glContext = ctx;
            ctx->has_been_current = TRUE;
            ctx->hdc = hdc;
            set_context_drawables( ctx, gl, gl );
            ctx->refresh_drawables = FALSE;
            pthread_mutex_unlock( &context_mutex );
            goto done;
        }
        pthread_mutex_unlock( &context_mutex );
    }
    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );

done:
    release_gl_drawable( gl );
    TRACE( "%p,%p returning %d\n", hdc, ctx, ret );
    return ret;
}

/***********************************************************************
 *		X11DRV_wglMakeContextCurrentARB
 */
static BOOL X11DRV_wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, struct wgl_context *ctx )
{
    BOOL ret = FALSE;
    struct gl_drawable *draw_gl, *read_gl = NULL;

    TRACE("(%p,%p,%p)\n", draw_hdc, read_hdc, ctx);

    if (!ctx)
    {
        pglXMakeCurrent(gdi_display, None, NULL);
        NtCurrentTeb()->glContext = NULL;
        return TRUE;
    }

    if (!pglXMakeContextCurrent) return FALSE;

    if ((draw_gl = get_gl_drawable( NtUserWindowFromDC( draw_hdc ), draw_hdc )))
    {
        read_gl = get_gl_drawable( NtUserWindowFromDC( read_hdc ), read_hdc );

        pthread_mutex_lock( &context_mutex );
        ret = pglXMakeContextCurrent(gdi_display, draw_gl->drawable,
                                     read_gl ? read_gl->drawable : 0, ctx->ctx);
        if (ret)
        {
            ctx->has_been_current = TRUE;
            ctx->hdc = draw_hdc;
            set_context_drawables( ctx, draw_gl, read_gl );
            ctx->refresh_drawables = FALSE;
            NtCurrentTeb()->glContext = ctx;
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

/***********************************************************************
 *		glxdrv_wglShareLists
 */
static BOOL glxdrv_wglShareLists(struct wgl_context *org, struct wgl_context *dest)
{
    struct wgl_context *keep, *clobber;

    TRACE("(%p, %p)\n", org, dest);

    /* Sharing of display lists works differently in GLX and WGL. In case of GLX it is done
     * at context creation time but in case of WGL it is done using wglShareLists.
     * In the past we tried to emulate wglShareLists by delaying GLX context creation until
     * either a wglMakeCurrent or wglShareLists. This worked fine for most apps but it causes
     * issues for OpenGL 3 because there wglCreateContextAttribsARB can fail in a lot of cases,
     * so there delaying context creation doesn't work.
     *
     * The new approach is to create a GLX context in wglCreateContext / wglCreateContextAttribsARB
     * and when a program requests sharing we recreate the destination or source context if it
     * hasn't been made current and it hasn't shared display lists before.
     */

    if (!dest->has_been_current && !dest->sharing)
    {
        keep = org;
        clobber = dest;
    }
    else if (!org->has_been_current && !org->sharing)
    {
        keep = dest;
        clobber = org;
    }
    else
    {
        ERR("Could not share display lists because both of the contexts have already been current or shared\n");
        return FALSE;
    }

    pglXDestroyContext(gdi_display, clobber->ctx);
    clobber->ctx = create_glxcontext(gdi_display, clobber, keep->ctx);
    TRACE("re-created context (%p) for Wine context %p (%s) sharing lists with ctx %p (%s)\n",
          clobber->ctx, clobber, debugstr_fbconfig(clobber->fmt->fbconfig),
          keep->ctx, debugstr_fbconfig(keep->fmt->fbconfig));

    org->sharing = TRUE;
    dest->sharing = TRUE;
    return TRUE;
}

static void wglFinish(void)
{
    struct x11drv_escape_flush_gl_drawable escape;
    struct gl_drawable *gl;
    struct wgl_context *ctx = NtCurrentTeb()->glContext;

    escape.code = X11DRV_FLUSH_GL_DRAWABLE;
    escape.gl_drawable = 0;
    escape.flush = FALSE;

    if ((gl = get_gl_drawable( NtUserWindowFromDC( ctx->hdc ), 0 )))
    {
        switch (gl->type)
        {
        case DC_GL_PIXMAP_WIN: escape.gl_drawable = gl->pixmap; break;
        case DC_GL_CHILD_WIN:  escape.gl_drawable = gl->window; break;
        default: break;
        }
        sync_context(ctx);
        release_gl_drawable( gl );
    }

    pglFinish();
    if (escape.gl_drawable)
        NtGdiExtEscape( ctx->hdc, NULL, 0, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

static void wglFlush(void)
{
    struct x11drv_escape_flush_gl_drawable escape;
    struct gl_drawable *gl;
    struct wgl_context *ctx = NtCurrentTeb()->glContext;

    escape.code = X11DRV_FLUSH_GL_DRAWABLE;
    escape.gl_drawable = 0;
    escape.flush = FALSE;

    if ((gl = get_gl_drawable( NtUserWindowFromDC( ctx->hdc ), 0 )))
    {
        switch (gl->type)
        {
        case DC_GL_PIXMAP_WIN: escape.gl_drawable = gl->pixmap; break;
        case DC_GL_CHILD_WIN:  escape.gl_drawable = gl->window; break;
        default: break;
        }
        sync_context(ctx);
        release_gl_drawable( gl );
    }

    pglFlush();
    if (escape.gl_drawable)
        NtGdiExtEscape( ctx->hdc, NULL, 0, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

static const GLubyte *wglGetString(GLenum name)
{
    if (name == GL_EXTENSIONS && glExtensions) return (const GLubyte *)glExtensions;
    return pglGetString(name);
}

/***********************************************************************
 *		X11DRV_wglCreateContextAttribsARB
 */
static struct wgl_context *X11DRV_wglCreateContextAttribsARB( HDC hdc, struct wgl_context *hShareContext,
                                                              const int* attribList )
{
    struct wgl_context *ret;
    struct gl_drawable *gl;
    int err = 0;

    TRACE("(%p %p %p)\n", hdc, hShareContext, attribList);

    if (!(gl = get_gl_drawable( NtUserWindowFromDC( hdc ), hdc )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return NULL;
    }

    if ((ret = calloc( 1, sizeof(*ret) )))
    {
        ret->hdc = hdc;
        ret->fmt = gl->format;
        ret->gl3_context = TRUE;
        if (attribList)
        {
            int *pContextAttribList = &ret->attribList[0];
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
                    ret->numAttribs++;
                    break;
                case WGL_CONTEXT_MINOR_VERSION_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_MINOR_VERSION_ARB;
                    pContextAttribList[1] = attribList[1];
                    pContextAttribList += 2;
                    ret->numAttribs++;
                    break;
                case WGL_CONTEXT_LAYER_PLANE_ARB:
                    break;
                case WGL_CONTEXT_FLAGS_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_FLAGS_ARB;
                    pContextAttribList[1] = attribList[1];
                    pContextAttribList += 2;
                    ret->numAttribs++;
                    break;
                case WGL_CONTEXT_OPENGL_NO_ERROR_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_OPENGL_NO_ERROR_ARB;
                    pContextAttribList[1] = attribList[1];
                    pContextAttribList += 2;
                    ret->numAttribs++;
                    break;
                case WGL_CONTEXT_PROFILE_MASK_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_PROFILE_MASK_ARB;
                    pContextAttribList[1] = attribList[1];
                    pContextAttribList += 2;
                    ret->numAttribs++;
                    break;
                case WGL_RENDERER_ID_WINE:
                    pContextAttribList[0] = GLX_RENDERER_ID_MESA;
                    pContextAttribList[1] = attribList[1];
                    pContextAttribList += 2;
                    ret->numAttribs++;
                    break;
                default:
                    ERR("Unhandled attribList pair: %#x %#x\n", attribList[0], attribList[1]);
                }
                attribList += 2;
            }
        }

        X11DRV_expect_error(gdi_display, GLXErrorHandler, NULL);
        ret->ctx = create_glxcontext(gdi_display, ret, hShareContext ? hShareContext->ctx : NULL);
        XSync(gdi_display, False);
        if ((err = X11DRV_check_error()) || !ret->ctx)
        {
            /* In the future we should convert the GLX error to a win32 one here if needed */
            WARN("Context creation failed (error %#x).\n", err);
            free( ret );
            ret = NULL;
        }
        else
        {
            pthread_mutex_lock( &context_mutex );
            list_add_head( &context_list, &ret->entry );
            pthread_mutex_unlock( &context_mutex );
        }
    }

    release_gl_drawable( gl );
    TRACE( "%p -> %p\n", hdc, ret );
    return ret;
}

/**
 * X11DRV_wglGetExtensionsStringARB
 *
 * WGL_ARB_extensions_string: wglGetExtensionsStringARB
 */
static const char *X11DRV_wglGetExtensionsStringARB(HDC hdc)
{
    TRACE("() returning \"%s\"\n", wglExtensions);
    return wglExtensions;
}

/**
 * X11DRV_wglCreatePbufferARB
 *
 * WGL_ARB_pbuffer: wglCreatePbufferARB
 */
static struct wgl_pbuffer *X11DRV_wglCreatePbufferARB( HDC hdc, int iPixelFormat, int iWidth, int iHeight,
                                                       const int *piAttribList )
{
    struct wgl_pbuffer* object;
    const struct wgl_pixel_format *fmt;
    int attribs[256];
    int nAttribs = 0;

    TRACE("(%p, %d, %d, %d, %p)\n", hdc, iPixelFormat, iWidth, iHeight, piAttribList);

    /* Convert the WGL pixelformat to a GLX format, if it fails then the format is invalid */
    fmt = get_pixel_format(gdi_display, iPixelFormat, TRUE /* Offscreen */);
    if(!fmt) {
        ERR("(%p): invalid pixel format %d\n", hdc, iPixelFormat);
        RtlSetLastWin32Error(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    object = calloc( 1, sizeof(*object) );
    if (NULL == object) {
        RtlSetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return NULL;
    }
    object->width = iWidth;
    object->height = iHeight;
    object->fmt = fmt;

    PUSH2(attribs, GLX_PBUFFER_WIDTH,  iWidth);
    PUSH2(attribs, GLX_PBUFFER_HEIGHT, iHeight); 
    while (piAttribList && 0 != *piAttribList) {
        int attr_v;
        switch (*piAttribList) {
            case WGL_PBUFFER_LARGEST_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_LARGEST_PBUFFER_ARB = %d\n", attr_v);
                PUSH2(attribs, GLX_LARGEST_PBUFFER, attr_v);
                break;
            }

            case WGL_TEXTURE_FORMAT_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_TEXTURE_FORMAT_ARB as %x\n", attr_v);
                if (WGL_NO_TEXTURE_ARB == attr_v) {
                    object->use_render_texture = 0;
                } else {
                    if (!use_render_texture_emulation) {
                        RtlSetLastWin32Error(ERROR_INVALID_DATA);
                        goto create_failed;
                    }
                    switch (attr_v) {
                        case WGL_TEXTURE_RGB_ARB:
                            object->use_render_texture = GL_RGB;
                            object->texture_bpp = 3;
                            object->texture_format = GL_RGB;
                            object->texture_type = GL_UNSIGNED_BYTE;
                            break;
                        case WGL_TEXTURE_RGBA_ARB:
                            object->use_render_texture = GL_RGBA;
                            object->texture_bpp = 4;
                            object->texture_format = GL_RGBA;
                            object->texture_type = GL_UNSIGNED_BYTE;
                            break;

                        /* WGL_FLOAT_COMPONENTS_NV */
                        case WGL_TEXTURE_FLOAT_R_NV:
                            object->use_render_texture = GL_FLOAT_R_NV;
                            object->texture_bpp = 4;
                            object->texture_format = GL_RED;
                            object->texture_type = GL_FLOAT;
                            break;
                        case WGL_TEXTURE_FLOAT_RG_NV:
                            object->use_render_texture = GL_FLOAT_RG_NV;
                            object->texture_bpp = 8;
                            object->texture_format = GL_LUMINANCE_ALPHA;
                            object->texture_type = GL_FLOAT;
                            break;
                        case WGL_TEXTURE_FLOAT_RGB_NV:
                            object->use_render_texture = GL_FLOAT_RGB_NV;
                            object->texture_bpp = 12;
                            object->texture_format = GL_RGB;
                            object->texture_type = GL_FLOAT;
                            break;
                        case WGL_TEXTURE_FLOAT_RGBA_NV:
                            object->use_render_texture = GL_FLOAT_RGBA_NV;
                            object->texture_bpp = 16;
                            object->texture_format = GL_RGBA;
                            object->texture_type = GL_FLOAT;
                            break;
                        default:
                            ERR("Unknown texture format: %x\n", attr_v);
                            RtlSetLastWin32Error(ERROR_INVALID_DATA);
                            goto create_failed;
                    }
                }
                break;
            }

            case WGL_TEXTURE_TARGET_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_TEXTURE_TARGET_ARB as %x\n", attr_v);
                if (WGL_NO_TEXTURE_ARB == attr_v) {
                    object->texture_target = 0;
                } else {
                    if (!use_render_texture_emulation) {
                        RtlSetLastWin32Error(ERROR_INVALID_DATA);
                        goto create_failed;
                    }
                    switch (attr_v) {
                        case WGL_TEXTURE_CUBE_MAP_ARB: {
                            if (iWidth != iHeight) {
                                RtlSetLastWin32Error(ERROR_INVALID_DATA);
                                goto create_failed;
                            }
                            object->texture_target = GL_TEXTURE_CUBE_MAP;
                            object->texture_bind_target = GL_TEXTURE_BINDING_CUBE_MAP;
                           break;
                        }
                        case WGL_TEXTURE_1D_ARB: {
                            if (1 != iHeight) {
                                RtlSetLastWin32Error(ERROR_INVALID_DATA);
                                goto create_failed;
                            }
                            object->texture_target = GL_TEXTURE_1D;
                            object->texture_bind_target = GL_TEXTURE_BINDING_1D;
                            break;
                        }
                        case WGL_TEXTURE_2D_ARB: {
                            object->texture_target = GL_TEXTURE_2D;
                            object->texture_bind_target = GL_TEXTURE_BINDING_2D;
                            break;
                        }
                        case WGL_TEXTURE_RECTANGLE_NV: {
                            object->texture_target = GL_TEXTURE_RECTANGLE_NV;
                            object->texture_bind_target = GL_TEXTURE_BINDING_RECTANGLE_NV;
                            break;
                        }
                        default:
                            ERR("Unknown texture target: %x\n", attr_v);
                            RtlSetLastWin32Error(ERROR_INVALID_DATA);
                            goto create_failed;
                    }
                }
                break;
            }

            case WGL_MIPMAP_TEXTURE_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_MIPMAP_TEXTURE_ARB as %x\n", attr_v);
                if (!use_render_texture_emulation) {
                    RtlSetLastWin32Error(ERROR_INVALID_DATA);
                    goto create_failed;
                }
                break;
            }
        }
        ++piAttribList;
    }

    PUSH1(attribs, None);
    if (!(object->gl = calloc( 1, sizeof(*object->gl) )))
    {
        RtlSetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        goto create_failed;
    }
    object->gl->type = DC_GL_PBUFFER;
    object->gl->format = object->fmt;
    object->gl->ref = 1;

    object->gl->drawable = pglXCreatePbuffer(gdi_display, fmt->fbconfig, attribs);
    TRACE("new Pbuffer drawable as %p (%lx)\n", object->gl, object->gl->drawable);
    if (!object->gl->drawable) {
        free( object->gl );
        RtlSetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        goto create_failed; /* unexpected error */
    }
    pthread_mutex_lock( &context_mutex );
    list_add_head( &pbuffer_list, &object->entry );
    pthread_mutex_unlock( &context_mutex );
    TRACE("->(%p)\n", object);
    return object;

create_failed:
    free( object );
    TRACE("->(FAILED)\n");
    return NULL;
}

/**
 * X11DRV_wglDestroyPbufferARB
 *
 * WGL_ARB_pbuffer: wglDestroyPbufferARB
 */
static BOOL X11DRV_wglDestroyPbufferARB( struct wgl_pbuffer *object )
{
    TRACE("(%p)\n", object);

    pthread_mutex_lock( &context_mutex );
    list_remove( &object->entry );
    pthread_mutex_unlock( &context_mutex );
    release_gl_drawable( object->gl );
    if (object->tmp_context)
        pglXDestroyContext(gdi_display, object->tmp_context);
    free( object );
    return GL_TRUE;
}

/**
 * X11DRV_wglGetPbufferDCARB
 *
 * WGL_ARB_pbuffer: wglGetPbufferDCARB
 */
static HDC X11DRV_wglGetPbufferDCARB( struct wgl_pbuffer *object )
{
    struct x11drv_escape_set_drawable escape;
    struct gl_drawable *prev;
    HDC hdc;

    hdc = NtGdiOpenDCW( NULL, NULL, NULL, 0, TRUE, NULL, NULL, NULL );
    if (!hdc) return 0;

    pthread_mutex_lock( &context_mutex );
    if (!XFindContext( gdi_display, (XID)hdc, gl_pbuffer_context, (char **)&prev ))
        release_gl_drawable( prev );
    grab_gl_drawable( object->gl );
    XSaveContext( gdi_display, (XID)hdc, gl_pbuffer_context, (char *)object->gl );
    pthread_mutex_unlock( &context_mutex );

    escape.code = X11DRV_SET_DRAWABLE;
    escape.drawable = object->gl->drawable;
    escape.mode = IncludeInferiors;
    SetRect( &escape.dc_rect, 0, 0, object->width, object->height );
    NtGdiExtEscape( hdc, NULL, 0, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );

    TRACE( "(%p)->(%p)\n", object, hdc );
    return hdc;
}

/**
 * X11DRV_wglQueryPbufferARB
 *
 * WGL_ARB_pbuffer: wglQueryPbufferARB
 */
static BOOL X11DRV_wglQueryPbufferARB( struct wgl_pbuffer *object, int iAttribute, int *piValue )
{
    TRACE("(%p, 0x%x, %p)\n", object, iAttribute, piValue);

    switch (iAttribute) {
        case WGL_PBUFFER_WIDTH_ARB:
            pglXQueryDrawable(gdi_display, object->gl->drawable, GLX_WIDTH, (unsigned int*) piValue);
            break;
        case WGL_PBUFFER_HEIGHT_ARB:
            pglXQueryDrawable(gdi_display, object->gl->drawable, GLX_HEIGHT, (unsigned int*) piValue);
            break;

        case WGL_PBUFFER_LOST_ARB:
            /* GLX Pbuffers cannot be lost by default. We can support this by
             * setting GLX_PRESERVED_CONTENTS to False and using glXSelectEvent
             * to receive pixel buffer clobber events, however that may or may
             * not give any benefit */
            *piValue = GL_FALSE;
            break;

        case WGL_TEXTURE_FORMAT_ARB:
            if (!object->use_render_texture) {
                *piValue = WGL_NO_TEXTURE_ARB;
            } else {
                if (!use_render_texture_emulation) {
                    RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
                    return GL_FALSE;
                }
                switch(object->use_render_texture) {
                    case GL_RGB:
                        *piValue = WGL_TEXTURE_RGB_ARB;
                        break;
                    case GL_RGBA:
                        *piValue = WGL_TEXTURE_RGBA_ARB;
                        break;
                    /* WGL_FLOAT_COMPONENTS_NV */
                    case GL_FLOAT_R_NV:
                        *piValue = WGL_TEXTURE_FLOAT_R_NV;
                        break;
                    case GL_FLOAT_RG_NV:
                        *piValue = WGL_TEXTURE_FLOAT_RG_NV;
                        break;
                    case GL_FLOAT_RGB_NV:
                        *piValue = WGL_TEXTURE_FLOAT_RGB_NV;
                        break;
                    case GL_FLOAT_RGBA_NV:
                        *piValue = WGL_TEXTURE_FLOAT_RGBA_NV;
                        break;
                    default:
                        ERR("Unknown texture format: %x\n", object->use_render_texture);
                }
            }
            break;

        case WGL_TEXTURE_TARGET_ARB:
            if (!object->texture_target){
                *piValue = WGL_NO_TEXTURE_ARB;
            } else {
                if (!use_render_texture_emulation) {
                    RtlSetLastWin32Error(ERROR_INVALID_DATA);
                    return GL_FALSE;
                }
                switch (object->texture_target) {
                    case GL_TEXTURE_1D:       *piValue = WGL_TEXTURE_1D_ARB; break;
                    case GL_TEXTURE_2D:       *piValue = WGL_TEXTURE_2D_ARB; break;
                    case GL_TEXTURE_CUBE_MAP: *piValue = WGL_TEXTURE_CUBE_MAP_ARB; break;
                    case GL_TEXTURE_RECTANGLE_NV: *piValue = WGL_TEXTURE_RECTANGLE_NV; break;
                }
            }
            break;

        case WGL_MIPMAP_TEXTURE_ARB:
            *piValue = GL_FALSE; /** don't support that */
            FIXME("unsupported WGL_ARB_render_texture attribute query for 0x%x\n", iAttribute);
            break;

        default:
            FIXME("unexpected attribute %x\n", iAttribute);
            break;
    }

    return GL_TRUE;
}

/**
 * X11DRV_wglReleasePbufferDCARB
 *
 * WGL_ARB_pbuffer: wglReleasePbufferDCARB
 */
static int X11DRV_wglReleasePbufferDCARB( struct wgl_pbuffer *object, HDC hdc )
{
    struct gl_drawable *gl;

    TRACE("(%p, %p)\n", object, hdc);

    pthread_mutex_lock( &context_mutex );

    if (!XFindContext( gdi_display, (XID)hdc, gl_pbuffer_context, (char **)&gl ))
    {
        XDeleteContext( gdi_display, (XID)hdc, gl_pbuffer_context );
        release_gl_drawable( gl );
    }
    else hdc = 0;

    pthread_mutex_unlock( &context_mutex );

    return hdc && NtGdiDeleteObjectApp(hdc);
}

/**
 * X11DRV_wglSetPbufferAttribARB
 *
 * WGL_ARB_pbuffer: wglSetPbufferAttribARB
 */
static BOOL X11DRV_wglSetPbufferAttribARB( struct wgl_pbuffer *object, const int *piAttribList )
{
    GLboolean ret = GL_FALSE;

    WARN("(%p, %p): alpha-testing, report any problem\n", object, piAttribList);

    if (!object->use_render_texture) {
        RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (use_render_texture_emulation) {
        return GL_TRUE;
    }
    return ret;
}

struct choose_pixel_format_arb_format
{
    int format;
    int original_index;
    PIXELFORMATDESCRIPTOR pfd;
    int depth, stencil;
};

static int compare_formats(const void *a, const void *b)
{
    /* Order formats so that onscreen formats go first. Then, if no depth bits requested,
     * prioritize formats with smaller depth within the original sort order with respect to
     * other attributes. */
    const struct choose_pixel_format_arb_format *fmt_a = a, *fmt_b = b;
    BOOL offscreen_a, offscreen_b;

    offscreen_a = fmt_a->format > nb_onscreen_formats;
    offscreen_b = fmt_b->format > nb_onscreen_formats;

    if (offscreen_a != offscreen_b)
        return offscreen_a - offscreen_b;
    if (memcmp(&fmt_a->pfd, &fmt_b->pfd, sizeof(fmt_a->pfd)))
        return fmt_a->original_index - fmt_b->original_index;
    if (fmt_a->depth != fmt_b->depth)
        return fmt_a->depth - fmt_b->depth;
    if (fmt_a->stencil != fmt_b->stencil)
        return fmt_a->stencil - fmt_b->stencil;

    return fmt_a->original_index - fmt_b->original_index;
}

/**
 * X11DRV_wglChoosePixelFormatARB
 *
 * WGL_ARB_pixel_format: wglChoosePixelFormatARB
 */
static BOOL X11DRV_wglChoosePixelFormatARB( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList,
                                            UINT nMaxFormats, int *piFormats, UINT *nNumFormats )
{
    struct choose_pixel_format_arb_format *formats;
    int it, i, format_count;
    BYTE depth_bits = 0;
    GLXFBConfig* cfgs;
    DWORD dwFlags = 0;
    int attribs[256];
    int nAttribs = 0;
    int nCfgs = 0;
    int fmt_id;

    TRACE("(%p, %p, %p, %d, %p, %p): hackish\n", hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats);
    if (NULL != pfAttribFList) {
        FIXME("unused pfAttribFList\n");
    }

    nAttribs = ConvertAttribWGLtoGLX(piAttribIList, attribs, NULL);
    if (-1 == nAttribs) {
        WARN("Cannot convert WGL to GLX attributes\n");
        return GL_FALSE;
    }
    PUSH1(attribs, None);

    /* There is no 1:1 mapping between GLX and WGL formats because we duplicate some GLX formats for bitmap rendering (see get_formats).
     * Flags like PFD_SUPPORT_GDI, PFD_DRAW_TO_BITMAP and others are a property of the pixel format. We don't query these attributes
     * using glXChooseFBConfig but we filter the result of glXChooseFBConfig later on.
     */
    for(i=0; piAttribIList[i] != 0; i+=2)
    {
        switch(piAttribIList[i])
        {
            case WGL_DRAW_TO_BITMAP_ARB:
                if(piAttribIList[i+1])
                    dwFlags |= PFD_DRAW_TO_BITMAP;
                break;
            case WGL_ACCELERATION_ARB:
                switch(piAttribIList[i+1])
                {
                    case WGL_NO_ACCELERATION_ARB:
                        dwFlags |= PFD_GENERIC_FORMAT;
                        break;
                    case WGL_GENERIC_ACCELERATION_ARB:
                        dwFlags |= PFD_GENERIC_ACCELERATED;
                        break;
                    case WGL_FULL_ACCELERATION_ARB:
                        /* Nothing to do */
                        break;
                }
                break;
            case WGL_SUPPORT_GDI_ARB:
                if(piAttribIList[i+1])
                    dwFlags |= PFD_SUPPORT_GDI;
                break;
            case WGL_DEPTH_BITS_ARB:
                depth_bits = piAttribIList[i+1];
                break;

        }
    }

    /* Search for FB configurations matching the requirements in attribs */
    cfgs = pglXChooseFBConfig(gdi_display, DefaultScreen(gdi_display), attribs, &nCfgs);
    if (NULL == cfgs) {
        WARN("Compatible Pixel Format not found\n");
        return GL_FALSE;
    }

    if (!(formats = malloc( nCfgs * sizeof(*formats) )))
    {
        ERR("No memory.\n");
        XFree(cfgs);
        return GL_FALSE;
    }

    format_count = 0;
    for (it = 0; it < nCfgs; ++it)
    {
        struct choose_pixel_format_arb_format *format;

        if (pglXGetFBConfigAttrib(gdi_display, cfgs[it], GLX_FBCONFIG_ID, &fmt_id))
        {
            ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
            continue;
        }

        for (i = 0; i < nb_pixel_formats; ++i)
            if (pixel_formats[i].fmt_id == fmt_id)
                break;

        if (i == nb_pixel_formats)
            continue;
        if ((pixel_formats[i].dwFlags & dwFlags) != dwFlags)
            continue;

        format = &formats[format_count];
        format->format = i + 1;
        format->original_index = it;

        memset(&format->pfd, 0, sizeof(format->pfd));
        if (!describe_pixel_format(format->format, &format->pfd, TRUE))
            ERR("describe_pixel_format failed, format %d.\n", format->format);

        format->depth = format->pfd.cDepthBits;
        format->stencil = format->pfd.cStencilBits;
        if (!depth_bits && !(format->pfd.dwFlags & PFD_GENERIC_FORMAT))
        {
            format->pfd.cDepthBits = 0;
            format->pfd.cStencilBits = 0;
        }

        ++format_count;
    }

    qsort(formats, format_count, sizeof(*formats), compare_formats);

    *nNumFormats = min(nMaxFormats, format_count);
    for (i = 0; i < *nNumFormats; ++i)
        piFormats[i] = formats[i].format;

    free( formats );
    XFree(cfgs);
    return GL_TRUE;
}

/**
 * X11DRV_wglGetPixelFormatAttribivARB
 *
 * WGL_ARB_pixel_format: wglGetPixelFormatAttribivARB
 */
static BOOL X11DRV_wglGetPixelFormatAttribivARB( HDC hdc, int iPixelFormat, int iLayerPlane,
                                                 UINT nAttributes, const int *piAttributes, int *piValues )
{
    UINT i;
    const struct wgl_pixel_format *fmt;
    int hTest;
    int tmp;
    int curGLXAttr = 0;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);

    if (0 < iLayerPlane) {
        FIXME("unsupported iLayerPlane(%d) > 0, returns FALSE\n", iLayerPlane);
        return GL_FALSE;
    }

    /* Convert the WGL pixelformat to a GLX one, if this fails then most likely the iPixelFormat isn't supported.
    * We don't have to fail yet as a program can specify an invalid iPixelFormat (lets say 0) if it wants to query
    * the number of supported WGL formats. Whether the iPixelFormat is valid is handled in the for-loop below. */
    fmt = get_pixel_format(gdi_display, iPixelFormat, TRUE /* Offscreen */);
    if(!fmt) {
        WARN("Unable to convert iPixelFormat %d to a GLX one!\n", iPixelFormat);
    }

    for (i = 0; i < nAttributes; ++i) {
        const int curWGLAttr = piAttributes[i];
        TRACE("pAttr[%d] = %x\n", i, curWGLAttr);

        switch (curWGLAttr) {
            case WGL_NUMBER_PIXEL_FORMATS_ARB:
                piValues[i] = nb_pixel_formats;
                continue;

            case WGL_SUPPORT_OPENGL_ARB:
                piValues[i] = GL_TRUE; 
                continue;

            case WGL_ACCELERATION_ARB:
                curGLXAttr = GLX_CONFIG_CAVEAT;
                if (!fmt) goto pix_error;
                if(fmt->dwFlags & PFD_GENERIC_FORMAT)
                    piValues[i] = WGL_NO_ACCELERATION_ARB;
                else if(fmt->dwFlags & PFD_GENERIC_ACCELERATED)
                    piValues[i] = WGL_GENERIC_ACCELERATION_ARB;
                else
                    piValues[i] = WGL_FULL_ACCELERATION_ARB;
                continue;

            case WGL_TRANSPARENT_ARB:
                curGLXAttr = GLX_TRANSPARENT_TYPE;
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                piValues[i] = GL_FALSE;
                if (GLX_NONE != tmp) piValues[i] = GL_TRUE;
                continue;

            case WGL_PIXEL_TYPE_ARB:
                curGLXAttr = GLX_RENDER_TYPE;
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                TRACE("WGL_PIXEL_TYPE_ARB: GLX_RENDER_TYPE = 0x%x\n", tmp);
                if      (tmp & GLX_RGBA_BIT)           { piValues[i] = WGL_TYPE_RGBA_ARB; }
                else if (tmp & GLX_COLOR_INDEX_BIT)    { piValues[i] = WGL_TYPE_COLORINDEX_ARB; }
                else if (tmp & GLX_RGBA_FLOAT_BIT)     { piValues[i] = WGL_TYPE_RGBA_FLOAT_ATI; }
                else if (tmp & GLX_RGBA_FLOAT_ATI_BIT) { piValues[i] = WGL_TYPE_RGBA_FLOAT_ATI; }
                else if (tmp & GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT) { piValues[i] = WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT; }
                else {
                    ERR("unexpected RenderType(%x)\n", tmp);
                    piValues[i] = WGL_TYPE_RGBA_ARB;
                }
                continue;

            case WGL_COLOR_BITS_ARB:
                curGLXAttr = GLX_BUFFER_SIZE;
                break;

            case WGL_BIND_TO_TEXTURE_RGB_ARB:
            case WGL_BIND_TO_TEXTURE_RGBA_ARB:
                if (!use_render_texture_emulation) {
                    piValues[i] = GL_FALSE;
                    continue;	
                }
                curGLXAttr = GLX_RENDER_TYPE;
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                if (GLX_COLOR_INDEX_BIT == tmp) {
                    piValues[i] = GL_FALSE;  
                    continue;
                }
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &tmp);
                if (hTest) goto get_error;
                piValues[i] = (tmp & GLX_PBUFFER_BIT) ? GL_TRUE : GL_FALSE;
                continue;

            case WGL_BLUE_BITS_ARB:
                curGLXAttr = GLX_BLUE_SIZE;
                break;
            case WGL_RED_BITS_ARB:
                curGLXAttr = GLX_RED_SIZE;
                break;
            case WGL_GREEN_BITS_ARB:
                curGLXAttr = GLX_GREEN_SIZE;
                break;
            case WGL_ALPHA_BITS_ARB:
                curGLXAttr = GLX_ALPHA_SIZE;
                break;
            case WGL_DEPTH_BITS_ARB:
                curGLXAttr = GLX_DEPTH_SIZE;
                break;
            case WGL_STENCIL_BITS_ARB:
                curGLXAttr = GLX_STENCIL_SIZE;
                break;
            case WGL_DOUBLE_BUFFER_ARB:
                curGLXAttr = GLX_DOUBLEBUFFER;
                break;
            case WGL_STEREO_ARB:
                curGLXAttr = GLX_STEREO;
                break;
            case WGL_AUX_BUFFERS_ARB:
                curGLXAttr = GLX_AUX_BUFFERS;
                break;

            case WGL_SUPPORT_GDI_ARB:
                if (!fmt) goto pix_error;
                piValues[i] = (fmt->dwFlags & PFD_SUPPORT_GDI) != 0;
                continue;

            case WGL_DRAW_TO_BITMAP_ARB:
                if (!fmt) goto pix_error;
                piValues[i] = (fmt->dwFlags & PFD_DRAW_TO_BITMAP) != 0;
                continue;

            case WGL_DRAW_TO_WINDOW_ARB:
            case WGL_DRAW_TO_PBUFFER_ARB:
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &tmp);
                if (hTest) goto get_error;
                if((curWGLAttr == WGL_DRAW_TO_WINDOW_ARB && (tmp&GLX_WINDOW_BIT)) ||
                   (curWGLAttr == WGL_DRAW_TO_PBUFFER_ARB && (tmp&GLX_PBUFFER_BIT)))
                    piValues[i] = GL_TRUE;
                else
                    piValues[i] = GL_FALSE;
                continue;

            case WGL_SWAP_METHOD_ARB:
                if (has_swap_method)
                {
                    hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_SWAP_METHOD_OML, &tmp);
                    if (hTest) goto get_error;
                    switch (tmp)
                    {
                    case GLX_SWAP_EXCHANGE_OML:
                        piValues[i] = WGL_SWAP_EXCHANGE_ARB;
                        break;
                    case GLX_SWAP_COPY_OML:
                        piValues[i] = WGL_SWAP_COPY_ARB;
                        break;
                    case GLX_SWAP_UNDEFINED_OML:
                        piValues[i] = WGL_SWAP_UNDEFINED_ARB;
                        break;
                    default:
                        ERR("Unexpected swap method %x.\n", tmp);
                    }
                }
                else
                {
                    WARN("GLX_OML_swap_method not supported, returning WGL_SWAP_EXCHANGE_ARB.\n");
                    piValues[i] = WGL_SWAP_EXCHANGE_ARB;
                }
                continue;

            case WGL_PBUFFER_LARGEST_ARB:
                curGLXAttr = GLX_LARGEST_PBUFFER;
                break;

            case WGL_SAMPLE_BUFFERS_ARB:
                curGLXAttr = GLX_SAMPLE_BUFFERS_ARB;
                break;

            case WGL_SAMPLES_ARB:
                curGLXAttr = GLX_SAMPLES_ARB;
                break;

            case WGL_FLOAT_COMPONENTS_NV:
                curGLXAttr = GLX_FLOAT_COMPONENTS_NV;
                break;

            case WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT:
                curGLXAttr = GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT;
                break;

            case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT:
                curGLXAttr = GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT;
                break;

            case WGL_ACCUM_RED_BITS_ARB:
                curGLXAttr = GLX_ACCUM_RED_SIZE;
                break;
            case WGL_ACCUM_GREEN_BITS_ARB:
                curGLXAttr = GLX_ACCUM_GREEN_SIZE;
                break;
            case WGL_ACCUM_BLUE_BITS_ARB:
                curGLXAttr = GLX_ACCUM_BLUE_SIZE;
                break;
            case WGL_ACCUM_ALPHA_BITS_ARB:
                curGLXAttr = GLX_ACCUM_ALPHA_SIZE;
                break;
            case WGL_ACCUM_BITS_ARB:
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_RED_SIZE, &tmp);
                if (hTest) goto get_error;
                piValues[i] = tmp;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_GREEN_SIZE, &tmp);
                if (hTest) goto get_error;
                piValues[i] += tmp;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_BLUE_SIZE, &tmp);
                if (hTest) goto get_error;
                piValues[i] += tmp;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_ALPHA_SIZE, &tmp);
                if (hTest) goto get_error;
                piValues[i] += tmp;
                continue;

            default:
                FIXME("unsupported %x WGL Attribute\n", curWGLAttr);
        }

        /* Retrieve a GLX FBConfigAttrib when the attribute to query is valid and
         * iPixelFormat != 0. When iPixelFormat is 0 the only value which makes
         * sense to query is WGL_NUMBER_PIXEL_FORMATS_ARB.
         *
         * TODO: properly test the behavior of wglGetPixelFormatAttrib*v on Windows
         *       and check which options can work using iPixelFormat=0 and which not.
         *       A problem would be that this function is an extension. This would
         *       mean that the behavior could differ between different vendors (ATI, Nvidia, ..).
         */
        if (0 != curGLXAttr && iPixelFormat != 0) {
            if (!fmt) goto pix_error;
            hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, curGLXAttr, piValues + i);
            if (hTest) goto get_error;
            curGLXAttr = 0;
        } else { 
            piValues[i] = GL_FALSE; 
        }
    }
    return GL_TRUE;

get_error:
    ERR("(%p): unexpected failure on GetFBConfigAttrib(%x) returns FALSE\n", hdc, curGLXAttr);
    return GL_FALSE;

pix_error:
    ERR("(%p): unexpected iPixelFormat(%d) vs nFormats(%d), returns FALSE\n", hdc, iPixelFormat, nb_pixel_formats);
    return GL_FALSE;
}

/**
 * X11DRV_wglGetPixelFormatAttribfvARB
 *
 * WGL_ARB_pixel_format: wglGetPixelFormatAttribfvARB
 */
static BOOL X11DRV_wglGetPixelFormatAttribfvARB( HDC hdc, int iPixelFormat, int iLayerPlane,
                                                 UINT nAttributes, const int *piAttributes, FLOAT *pfValues )
{
    int *attr;
    int ret;
    UINT i;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);

    /* Allocate a temporary array to store integer values */
    attr = malloc( nAttributes * sizeof(int) );
    if (!attr) {
        ERR("couldn't allocate %d array\n", nAttributes);
        return GL_FALSE;
    }

    /* Piggy-back on wglGetPixelFormatAttribivARB */
    ret = X11DRV_wglGetPixelFormatAttribivARB(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, attr);
    if (ret) {
        /* Convert integer values to float. Should also check for attributes
           that can give decimal values here */
        for (i=0; i<nAttributes;i++) {
            pfValues[i] = attr[i];
        }
    }

    free( attr );
    return ret;
}

/**
 * X11DRV_wglBindTexImageARB
 *
 * WGL_ARB_render_texture: wglBindTexImageARB
 */
static BOOL X11DRV_wglBindTexImageARB( struct wgl_pbuffer *object, int iBuffer )
{
    GLboolean ret = GL_FALSE;

    TRACE("(%p, %d)\n", object, iBuffer);

    if (!object->use_render_texture) {
        RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }

    if (use_render_texture_emulation) {
        static BOOL initialized = FALSE;
        int prev_binded_texture = 0;
        GLXContext prev_context;
        GLXDrawable prev_drawable;

        prev_context = pglXGetCurrentContext();
        prev_drawable = pglXGetCurrentDrawable();

        /* Our render_texture emulation is basic and lacks some features (1D/Cube support).
           This is mostly due to lack of demos/games using them. Further the use of glReadPixels
           isn't ideal performance wise but I wasn't able to get other ways working.
        */
        if(!initialized) {
            initialized = TRUE; /* Only show the FIXME once for performance reasons */
            FIXME("partial stub!\n");
        }

        TRACE("drawable=%p (%lx), context=%p\n", object->gl, object->gl->drawable, prev_context);
        if (!object->tmp_context || object->prev_context != prev_context) {
            if (object->tmp_context)
                pglXDestroyContext(gdi_display, object->tmp_context);
            object->tmp_context = pglXCreateNewContext(gdi_display, object->fmt->fbconfig, object->fmt->render_type, prev_context, True);
            object->prev_context = prev_context;
        }

        opengl_funcs.gl.p_glGetIntegerv(object->texture_bind_target, &prev_binded_texture);

        /* Switch to our pbuffer */
        pglXMakeCurrent(gdi_display, object->gl->drawable, object->tmp_context);

        /* Make sure that the prev_binded_texture is set as the current texture state isn't shared between contexts.
         * After that copy the pbuffer texture data. */
        opengl_funcs.gl.p_glBindTexture(object->texture_target, prev_binded_texture);
        opengl_funcs.gl.p_glCopyTexImage2D(object->texture_target, 0, object->use_render_texture, 0, 0, object->width, object->height, 0);

        /* Switch back to the original drawable and context */
        pglXMakeCurrent(gdi_display, prev_drawable, prev_context);
        return GL_TRUE;
    }

    return ret;
}

/**
 * X11DRV_wglReleaseTexImageARB
 *
 * WGL_ARB_render_texture: wglReleaseTexImageARB
 */
static BOOL X11DRV_wglReleaseTexImageARB( struct wgl_pbuffer *object, int iBuffer )
{
    GLboolean ret = GL_FALSE;

    TRACE("(%p, %d)\n", object, iBuffer);

    if (!object->use_render_texture) {
        RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (use_render_texture_emulation) {
        return GL_TRUE;
    }
    return ret;
}

/**
 * X11DRV_wglGetExtensionsStringEXT
 *
 * WGL_EXT_extensions_string: wglGetExtensionsStringEXT
 */
static const char *X11DRV_wglGetExtensionsStringEXT(void)
{
    TRACE("() returning \"%s\"\n", wglExtensions);
    return wglExtensions;
}

/**
 * X11DRV_wglGetSwapIntervalEXT
 *
 * WGL_EXT_swap_control: wglGetSwapIntervalEXT
 */
static int X11DRV_wglGetSwapIntervalEXT(void)
{
    struct wgl_context *ctx = NtCurrentTeb()->glContext;
    struct gl_drawable *gl;
    int swap_interval;

    TRACE("()\n");

    if (!(gl = get_gl_drawable( NtUserWindowFromDC( ctx->hdc ), ctx->hdc )))
    {
        /* This can't happen because a current WGL context is required to get
         * here. Likely the application is buggy.
         */
        WARN("No GL drawable found, returning swap interval 0\n");
        return 0;
    }

    swap_interval = gl->swap_interval;
    release_gl_drawable(gl);

    return swap_interval;
}

/**
 * X11DRV_wglSwapIntervalEXT
 *
 * WGL_EXT_swap_control: wglSwapIntervalEXT
 */
static BOOL X11DRV_wglSwapIntervalEXT(int interval)
{
    struct wgl_context *ctx = NtCurrentTeb()->glContext;
    struct gl_drawable *gl;
    BOOL ret;

    TRACE("(%d)\n", interval);

    /* Without WGL/GLX_EXT_swap_control_tear a negative interval
     * is invalid.
     */
    if (interval < 0 && !has_swap_control_tear)
    {
        RtlSetLastWin32Error(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (!(gl = get_gl_drawable( NtUserWindowFromDC( ctx->hdc ), ctx->hdc )))
    {
        RtlSetLastWin32Error(ERROR_DC_NOT_FOUND);
        return FALSE;
    }

    pthread_mutex_lock( &context_mutex );
    ret = set_swap_interval(gl->drawable, interval);
    gl->refresh_swap_interval = FALSE;
    if (ret)
        gl->swap_interval = interval;
    else
        RtlSetLastWin32Error(ERROR_DC_NOT_FOUND);

    pthread_mutex_unlock( &context_mutex );
    release_gl_drawable(gl);

    return ret;
}

/**
 * X11DRV_wglSetPixelFormatWINE
 *
 * WGL_WINE_pixel_format_passthrough: wglSetPixelFormatWINE
 * This is a WINE-specific wglSetPixelFormat which can set the pixel format multiple times.
 */
static BOOL X11DRV_wglSetPixelFormatWINE(HDC hdc, int format)
{
    return set_pixel_format(hdc, format, TRUE);
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

/**
 * X11DRV_WineGL_LoadExtensions
 */
static void X11DRV_WineGL_LoadExtensions(void)
{
    wglExtensions[0] = 0;

    /* ARB Extensions */

    if (has_extension( glxExtensions, "GLX_ARB_create_context"))
    {
        register_extension( "WGL_ARB_create_context" );
        opengl_funcs.ext.p_wglCreateContextAttribsARB = X11DRV_wglCreateContextAttribsARB;

        if (has_extension( glxExtensions, "GLX_ARB_create_context_no_error" ))
            register_extension( "WGL_ARB_create_context_no_error" );
        if (has_extension( glxExtensions, "GLX_ARB_create_context_profile"))
            register_extension("WGL_ARB_create_context_profile");
    }


    register_extension( "WGL_ARB_extensions_string" );
    opengl_funcs.ext.p_wglGetExtensionsStringARB = X11DRV_wglGetExtensionsStringARB;

    if (glxRequireVersion(3))
    {
        register_extension( "WGL_ARB_make_current_read" );
        opengl_funcs.ext.p_wglGetCurrentReadDCARB   = (void *)1;  /* never called */
        opengl_funcs.ext.p_wglMakeContextCurrentARB = X11DRV_wglMakeContextCurrentARB;
    }

    if (has_extension( glxExtensions, "GLX_ARB_multisample")) register_extension( "WGL_ARB_multisample" );

    if (glxRequireVersion(3))
    {
        register_extension( "WGL_ARB_pbuffer" );
        opengl_funcs.ext.p_wglCreatePbufferARB    = X11DRV_wglCreatePbufferARB;
        opengl_funcs.ext.p_wglDestroyPbufferARB   = X11DRV_wglDestroyPbufferARB;
        opengl_funcs.ext.p_wglGetPbufferDCARB     = X11DRV_wglGetPbufferDCARB;
        opengl_funcs.ext.p_wglQueryPbufferARB     = X11DRV_wglQueryPbufferARB;
        opengl_funcs.ext.p_wglReleasePbufferDCARB = X11DRV_wglReleasePbufferDCARB;
        opengl_funcs.ext.p_wglSetPbufferAttribARB = X11DRV_wglSetPbufferAttribARB;
    }

    register_extension( "WGL_ARB_pixel_format" );
    opengl_funcs.ext.p_wglChoosePixelFormatARB      = X11DRV_wglChoosePixelFormatARB;
    opengl_funcs.ext.p_wglGetPixelFormatAttribfvARB = X11DRV_wglGetPixelFormatAttribfvARB;
    opengl_funcs.ext.p_wglGetPixelFormatAttribivARB = X11DRV_wglGetPixelFormatAttribivARB;

    if (has_extension( glxExtensions, "GLX_ARB_fbconfig_float"))
    {
        register_extension("WGL_ARB_pixel_format_float");
        register_extension("WGL_ATI_pixel_format_float");
    }

    /* Support WGL_ARB_render_texture when there's support or pbuffer based emulation */
    if (has_extension( glxExtensions, "GLX_ARB_render_texture") ||
        (glxRequireVersion(3) && use_render_texture_emulation))
    {
        register_extension( "WGL_ARB_render_texture" );
        opengl_funcs.ext.p_wglBindTexImageARB    = X11DRV_wglBindTexImageARB;
        opengl_funcs.ext.p_wglReleaseTexImageARB = X11DRV_wglReleaseTexImageARB;

        /* The WGL version of GLX_NV_float_buffer requires render_texture */
        if (has_extension( glxExtensions, "GLX_NV_float_buffer"))
            register_extension("WGL_NV_float_buffer");

        /* Again there's no GLX equivalent for this extension, so depend on the required GL extension */
        if (has_extension(glExtensions, "GL_NV_texture_rectangle"))
            register_extension("WGL_NV_render_texture_rectangle");
    }

    /* EXT Extensions */

    register_extension( "WGL_EXT_extensions_string" );
    opengl_funcs.ext.p_wglGetExtensionsStringEXT = X11DRV_wglGetExtensionsStringEXT;

    /* Load this extension even when it isn't backed by a GLX extension because it is has been around for ages.
     * Games like Call of Duty and K.O.T.O.R. rely on it. Further our emulation is good enough. */
    register_extension( "WGL_EXT_swap_control" );
    opengl_funcs.ext.p_wglSwapIntervalEXT = X11DRV_wglSwapIntervalEXT;
    opengl_funcs.ext.p_wglGetSwapIntervalEXT = X11DRV_wglGetSwapIntervalEXT;

    if (has_extension( glxExtensions, "GLX_EXT_framebuffer_sRGB"))
        register_extension("WGL_EXT_framebuffer_sRGB");

    if (has_extension( glxExtensions, "GLX_EXT_fbconfig_packed_float"))
        register_extension("WGL_EXT_pixel_format_packed_float");

    if (has_extension( glxExtensions, "GLX_EXT_swap_control"))
    {
        swap_control_method = GLX_SWAP_CONTROL_EXT;
        if (has_extension( glxExtensions, "GLX_EXT_swap_control_tear"))
        {
            register_extension("WGL_EXT_swap_control_tear");
            has_swap_control_tear = TRUE;
        }
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
        opengl_funcs.ext.p_wglAllocateMemoryNV = pglXAllocateMemoryNV;
        opengl_funcs.ext.p_wglFreeMemoryNV = pglXFreeMemoryNV;
    }

    if (has_extension(glxExtensions, "GLX_OML_swap_method"))
        has_swap_method = TRUE;

    /* WINE-specific WGL Extensions */

    /* In WineD3D we need the ability to set the pixel format more than once (e.g. after a device reset).
     * The default wglSetPixelFormat doesn't allow this, so add our own which allows it.
     */
    register_extension( "WGL_WINE_pixel_format_passthrough" );
    opengl_funcs.ext.p_wglSetPixelFormatWINE = X11DRV_wglSetPixelFormatWINE;

    if (has_extension( glxExtensions, "GLX_MESA_query_renderer" ))
    {
        register_extension( "WGL_WINE_query_renderer" );
        opengl_funcs.ext.p_wglQueryCurrentRendererIntegerWINE = X11DRV_wglQueryCurrentRendererIntegerWINE;
        opengl_funcs.ext.p_wglQueryCurrentRendererStringWINE = X11DRV_wglQueryCurrentRendererStringWINE;
        opengl_funcs.ext.p_wglQueryRendererIntegerWINE = X11DRV_wglQueryRendererIntegerWINE;
        opengl_funcs.ext.p_wglQueryRendererStringWINE = X11DRV_wglQueryRendererStringWINE;
    }
}


/**
 * glxdrv_SwapBuffers
 *
 * Swap the buffers of this DC
 */
static BOOL glxdrv_wglSwapBuffers( HDC hdc )
{
    struct x11drv_escape_flush_gl_drawable escape;
    struct gl_drawable *gl;
    struct wgl_context *ctx = NtCurrentTeb()->glContext;
    INT64 ust, msc, sbc, target_sbc = 0;

    TRACE("(%p)\n", hdc);

    escape.code = X11DRV_FLUSH_GL_DRAWABLE;
    escape.gl_drawable = 0;
    escape.flush = !pglXWaitForSbcOML;

    if (!(gl = get_gl_drawable( NtUserWindowFromDC( hdc ), hdc )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    pthread_mutex_lock( &context_mutex );
    if (gl->refresh_swap_interval)
    {
        set_swap_interval(gl->drawable, gl->swap_interval);
        gl->refresh_swap_interval = FALSE;
    }
    pthread_mutex_unlock( &context_mutex );

    switch (gl->type)
    {
    case DC_GL_PIXMAP_WIN:
        if (ctx) sync_context( ctx );
        escape.gl_drawable = gl->pixmap;
        if (ctx && pglXCopySubBufferMESA) {
            /* (glX)SwapBuffers has an implicit glFlush effect, however
             * GLX_MESA_copy_sub_buffer doesn't. Make sure GL is flushed before
             * copying */
            pglFlush();
            pglXCopySubBufferMESA( gdi_display, gl->drawable, 0, 0,
                                   gl->pixmap_size.cx, gl->pixmap_size.cy );
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
        if (gl->type == DC_GL_CHILD_WIN) escape.gl_drawable = gl->window;
        /* fall through */
    default:
        if (ctx && escape.gl_drawable && pglXSwapBuffersMscOML)
        {
            pglFlush();
            target_sbc = pglXSwapBuffersMscOML( gdi_display, gl->drawable, 0, 0, 0 );
            break;
        }
        pglXSwapBuffers(gdi_display, gl->drawable);
        break;
    }

    if (ctx && escape.gl_drawable && pglXWaitForSbcOML)
        pglXWaitForSbcOML( gdi_display, gl->drawable, target_sbc, &ust, &msc, &sbc );

    release_gl_drawable( gl );

    if (escape.gl_drawable)
        NtGdiExtEscape( ctx ? ctx->hdc : hdc, NULL, 0, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
    return TRUE;
}

static struct opengl_funcs opengl_funcs =
{
    {
        glxdrv_wglCopyContext,              /* p_wglCopyContext */
        glxdrv_wglCreateContext,            /* p_wglCreateContext */
        glxdrv_wglDeleteContext,            /* p_wglDeleteContext */
        glxdrv_wglDescribePixelFormat,      /* p_wglDescribePixelFormat */
        glxdrv_wglGetPixelFormat,           /* p_wglGetPixelFormat */
        glxdrv_wglGetProcAddress,           /* p_wglGetProcAddress */
        glxdrv_wglMakeCurrent,              /* p_wglMakeCurrent */
        glxdrv_wglSetPixelFormat,           /* p_wglSetPixelFormat */
        glxdrv_wglShareLists,               /* p_wglShareLists */
        glxdrv_wglSwapBuffers,              /* p_wglSwapBuffers */
    }
};

struct opengl_funcs *get_glx_driver( UINT version )
{
    if (version != WINE_WGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but driver has %u\n", version, WINE_WGL_DRIVER_VERSION );
        return NULL;
    }
    if (has_opengl()) return &opengl_funcs;
    return NULL;
}

#else  /* no OpenGL includes */

struct opengl_funcs *get_glx_driver( UINT version )
{
    return NULL;
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
