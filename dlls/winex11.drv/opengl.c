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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#endif
#ifdef HAVE_GL_GLX_H
# include <GL/glx.h>
#endif
#undef APIENTRY
#undef GLAPI

#include "x11drv.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);

#ifdef SONAME_LIBGL

WINE_DECLARE_DEBUG_CHANNEL(winediag);
WINE_DECLARE_DEBUG_CHANNEL(fps);

#include "wine/wglext.h"

/* For compatibility with old Mesa headers */
#ifndef GLX_SAMPLE_BUFFERS_ARB
# define GLX_SAMPLE_BUFFERS_ARB           100000
#endif
#ifndef GLX_SAMPLES_ARB
# define GLX_SAMPLES_ARB                  100001
#endif
#ifndef GL_TEXTURE_CUBE_MAP
# define GL_TEXTURE_CUBE_MAP              0x8513
#endif
#ifndef GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT
# define GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20B2
#endif
#ifndef GLX_EXT_fbconfig_packed_float
# define GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT 0x20B1
# define GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT  0x00000008
#endif
#ifndef GLX_ARB_create_context
# define GLX_CONTEXT_MAJOR_VERSION_ARB    0x2091
# define GLX_CONTEXT_MINOR_VERSION_ARB    0x2092
# define GLX_CONTEXT_FLAGS_ARB            0x2094
#endif
#ifndef GLX_ARB_create_context_profile
# define GLX_CONTEXT_PROFILE_MASK_ARB     0x9126
#endif
/** GLX_ATI_pixel_format_float */
#define GLX_RGBA_FLOAT_ATI_BIT            0x00000100
/** GLX_ARB_pixel_format_float */
#define GLX_RGBA_FLOAT_BIT                0x00000004
#define GLX_RGBA_FLOAT_TYPE               0x20B9
/** GL_NV_float_buffer */
#define GL_FLOAT_R_NV                     0x8880
#define GL_FLOAT_RG_NV                    0x8881
#define GL_FLOAT_RGB_NV                   0x8882
#define GL_FLOAT_RGBA_NV                  0x8883
#define GL_FLOAT_R16_NV                   0x8884
#define GL_FLOAT_R32_NV                   0x8885
#define GL_FLOAT_RG16_NV                  0x8886
#define GL_FLOAT_RG32_NV                  0x8887
#define GL_FLOAT_RGB16_NV                 0x8888
#define GL_FLOAT_RGB32_NV                 0x8889
#define GL_FLOAT_RGBA16_NV                0x888A
#define GL_FLOAT_RGBA32_NV                0x888B
#define GL_TEXTURE_FLOAT_COMPONENTS_NV    0x888C
#define GL_FLOAT_CLEAR_COLOR_VALUE_NV     0x888D
#define GL_FLOAT_RGBA_MODE_NV             0x888E
/** GLX_NV_float_buffer */
#define GLX_FLOAT_COMPONENTS_NV           0x20B0


typedef struct wine_glextension {
    const char *extName;
    struct {
        const char *funcName;
        void *funcAddress;
    } extEntryPoints[9];
} WineGLExtension;

struct WineGLInfo {
    const char *glVersion;
    char *glExtensions;

    int glxVersion[2];

    const char *glxServerVersion;
    const char *glxServerVendor;
    const char *glxServerExtensions;

    const char *glxClientVersion;
    const char *glxClientVendor;
    const char *glxClientExtensions;

    const char *glxExtensions;

    BOOL glxDirect;
    char wglExtensions[4096];
};

typedef struct wine_glpixelformat {
    int         iPixelFormat;
    GLXFBConfig fbconfig;
    int         fmt_id;
    int         render_type;
    BOOL        offscreenOnly;
    DWORD       dwFlags; /* We store some PFD_* flags in here for emulated bitmap formats */
} WineGLPixelFormat;

struct wgl_context
{
    HDC hdc;
    BOOL has_been_current;
    BOOL sharing;
    BOOL gl3_context;
    XVisualInfo *vis;
    WineGLPixelFormat *fmt;
    int numAttribs; /* This is needed for delaying wglCreateContextAttribsARB */
    int attribList[16]; /* This is needed for delaying wglCreateContextAttribsARB */
    GLXContext ctx;
    HDC read_hdc;
    Drawable drawables[2];
    BOOL refresh_drawables;
    Pixmap pixmap;            /* pixmap for memory DCs */
    GLXPixmap glxpixmap;      /* GLX pixmap for memory DCs */
    SIZE pixmap_size;         /* pixmap size for memory DCs */
    struct list entry;
};

typedef struct wine_glpbuffer {
    Drawable   drawable;
    Display*   display;
    WineGLPixelFormat* fmt;
    int        width;
    int        height;
    int*       attribList;
    HDC        hdc;

    int        use_render_texture; /* This is also the internal texture format */
    int        texture_bind_target;
    int        texture_bpp;
    GLint      texture_format;
    GLuint     texture_target;
    GLenum     texture_type;
    GLuint     texture;
    int        texture_level;
} Wine_GLPBuffer;

struct glx_physdev
{
    struct gdi_physdev dev;
    X11DRV_PDEVICE    *x11dev;
    enum dc_gl_type    type;          /* type of GL device context */
    int                pixel_format;
    Drawable           drawable;
    Pixmap             pixmap;        /* pixmap for a DL_GL_PIXMAP_WIN drawable */
};

static const struct wgl_funcs glxdrv_wgl_funcs;
static const struct gdi_dc_funcs glxdrv_funcs;

static inline struct glx_physdev *get_glxdrv_dev( PHYSDEV dev )
{
    return (struct glx_physdev *)dev;
}

static struct list context_list = LIST_INIT( context_list );
static struct WineGLInfo WineGLInfo = { 0 };
static int use_render_texture_emulation = 1;
static BOOL has_swap_control;
static int swap_interval = 1;

#define MAX_EXTENSIONS 16
static const WineGLExtension *WineGLExtensionList[MAX_EXTENSIONS];
static int WineGLExtensionListSize;

static void X11DRV_WineGL_LoadExtensions(void);
static WineGLPixelFormat* ConvertPixelFormatWGLtoGLX(Display *display, int iPixelFormat, BOOL AllowOffscreen, int *fmt_count);
static BOOL glxRequireVersion(int requiredVersion);
static BOOL glxRequireExtension(const char *requiredExtension);

static void dump_PIXELFORMATDESCRIPTOR(const PIXELFORMATDESCRIPTOR *ppfd) {
  TRACE("  - size / version : %d / %d\n", ppfd->nSize, ppfd->nVersion);
  TRACE("  - dwFlags : ");
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

  TRACE("  - iPixelType : ");
  switch (ppfd->iPixelType) {
  case PFD_TYPE_RGBA: TRACE("PFD_TYPE_RGBA"); break;
  case PFD_TYPE_COLORINDEX: TRACE("PFD_TYPE_COLORINDEX"); break;
  }
  TRACE("\n");

  TRACE("  - Color   : %d\n", ppfd->cColorBits);
  TRACE("  - Red     : %d\n", ppfd->cRedBits);
  TRACE("  - Green   : %d\n", ppfd->cGreenBits);
  TRACE("  - Blue    : %d\n", ppfd->cBlueBits);
  TRACE("  - Alpha   : %d\n", ppfd->cAlphaBits);
  TRACE("  - Accum   : %d\n", ppfd->cAccumBits);
  TRACE("  - Depth   : %d\n", ppfd->cDepthBits);
  TRACE("  - Stencil : %d\n", ppfd->cStencilBits);
  TRACE("  - Aux     : %d\n", ppfd->cAuxBuffers);

  TRACE("  - iLayerType : ");
  switch (ppfd->iLayerType) {
  case PFD_MAIN_PLANE: TRACE("PFD_MAIN_PLANE"); break;
  case PFD_OVERLAY_PLANE: TRACE("PFD_OVERLAY_PLANE"); break;
  case (BYTE)PFD_UNDERLAY_PLANE: TRACE("PFD_UNDERLAY_PLANE"); break;
  }
  TRACE("\n");
}

#define PUSH1(attribs,att)        do { attribs[nAttribs++] = (att); } while (0)
#define PUSH2(attribs,att,value)  do { attribs[nAttribs++] = (att); attribs[nAttribs++] = (value); } while(0)

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
/* GLX 1.0 */
MAKE_FUNCPTR(glXChooseVisual)
MAKE_FUNCPTR(glXCopyContext)
MAKE_FUNCPTR(glXCreateContext)
MAKE_FUNCPTR(glXCreateGLXPixmap)
MAKE_FUNCPTR(glXGetCurrentContext)
MAKE_FUNCPTR(glXGetCurrentDrawable)
MAKE_FUNCPTR(glXDestroyContext)
MAKE_FUNCPTR(glXDestroyGLXPixmap)
MAKE_FUNCPTR(glXGetConfig)
MAKE_FUNCPTR(glXIsDirect)
MAKE_FUNCPTR(glXMakeCurrent)
MAKE_FUNCPTR(glXSwapBuffers)
MAKE_FUNCPTR(glXQueryExtension)
MAKE_FUNCPTR(glXQueryVersion)

/* GLX 1.1 */
MAKE_FUNCPTR(glXGetClientString)
MAKE_FUNCPTR(glXQueryExtensionsString)
MAKE_FUNCPTR(glXQueryServerString)

/* GLX 1.3 */
MAKE_FUNCPTR(glXGetFBConfigs)
MAKE_FUNCPTR(glXChooseFBConfig)
MAKE_FUNCPTR(glXCreatePbuffer)
MAKE_FUNCPTR(glXCreateNewContext)
MAKE_FUNCPTR(glXDestroyPbuffer)
MAKE_FUNCPTR(glXGetFBConfigAttrib)
MAKE_FUNCPTR(glXGetVisualFromFBConfig)
MAKE_FUNCPTR(glXMakeContextCurrent)
MAKE_FUNCPTR(glXQueryDrawable)
MAKE_FUNCPTR(glXGetCurrentReadDrawable)

/* GLX Extensions */
static GLXContext (*pglXCreateContextAttribsARB)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
static void* (*pglXGetProcAddressARB)(const GLubyte *);
static int   (*pglXSwapIntervalSGI)(int);

/* NV GLX Extension */
static void* (*pglXAllocateMemoryNV)(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
static void  (*pglXFreeMemoryNV)(GLvoid *pointer);

/* MESA GLX Extensions */
static void (*pglXCopySubBufferMESA)(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height);

/* Standard OpenGL */
MAKE_FUNCPTR(glBindTexture)
MAKE_FUNCPTR(glBitmap)
MAKE_FUNCPTR(glCopyTexSubImage1D)
MAKE_FUNCPTR(glCopyTexImage2D)
MAKE_FUNCPTR(glCopyTexSubImage2D)
MAKE_FUNCPTR(glDrawBuffer)
MAKE_FUNCPTR(glEndList)
MAKE_FUNCPTR(glGetError)
MAKE_FUNCPTR(glGetIntegerv)
MAKE_FUNCPTR(glGetString)
MAKE_FUNCPTR(glNewList)
MAKE_FUNCPTR(glPixelStorei)
MAKE_FUNCPTR(glReadPixels)
MAKE_FUNCPTR(glTexImage2D)
MAKE_FUNCPTR(glFinish)
MAKE_FUNCPTR(glFlush)
#undef MAKE_FUNCPTR

static int GLXErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    /* In the future we might want to find the exact X or GLX error to report back to the app */
    return 1;
}

static BOOL infoInitialized = FALSE;
static BOOL X11DRV_WineGL_InitOpenglInfo(void)
{
    int screen = DefaultScreen(gdi_display);
    Window win = 0, root = 0;
    const char *gl_renderer;
    const char* str;
    XVisualInfo *vis;
    GLXContext ctx = NULL;
    XSetWindowAttributes attr;
    BOOL ret = FALSE;
    int attribList[] = {GLX_RGBA, GLX_DOUBLEBUFFER, None};

    if (infoInitialized)
        return TRUE;
    infoInitialized = TRUE;

    attr.override_redirect = True;
    attr.colormap = None;
    attr.border_pixel = 0;

    wine_tsx11_lock();

    vis = pglXChooseVisual(gdi_display, screen, attribList);
    if (vis) {
#ifdef __i386__
        WORD old_fs = wine_get_fs();
        /* Create a GLX Context. Without one we can't query GL information */
        ctx = pglXCreateContext(gdi_display, vis, None, GL_TRUE);
        if (wine_get_fs() != old_fs)
        {
            wine_set_fs( old_fs );
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
        ERR_(winediag)( "Unable to activate OpenGL context, most likely your OpenGL drivers haven't been installed correctly\n" );
        goto done;
    }
    gl_renderer = (const char *)pglGetString(GL_RENDERER);
    WineGLInfo.glVersion = (const char *) pglGetString(GL_VERSION);
    str = (const char *) pglGetString(GL_EXTENSIONS);
    WineGLInfo.glExtensions = HeapAlloc(GetProcessHeap(), 0, strlen(str)+1);
    strcpy(WineGLInfo.glExtensions, str);

    /* Get the common GLX version supported by GLX client and server ( major/minor) */
    pglXQueryVersion(gdi_display, &WineGLInfo.glxVersion[0], &WineGLInfo.glxVersion[1]);

    WineGLInfo.glxServerVersion = pglXQueryServerString(gdi_display, screen, GLX_VERSION);
    WineGLInfo.glxServerVendor = pglXQueryServerString(gdi_display, screen, GLX_VENDOR);
    WineGLInfo.glxServerExtensions = pglXQueryServerString(gdi_display, screen, GLX_EXTENSIONS);

    WineGLInfo.glxClientVersion = pglXGetClientString(gdi_display, GLX_VERSION);
    WineGLInfo.glxClientVendor = pglXGetClientString(gdi_display, GLX_VENDOR);
    WineGLInfo.glxClientExtensions = pglXGetClientString(gdi_display, GLX_EXTENSIONS);

    WineGLInfo.glxExtensions = pglXQueryExtensionsString(gdi_display, screen);
    WineGLInfo.glxDirect = pglXIsDirect(gdi_display, ctx);

    TRACE("GL version             : %s.\n", WineGLInfo.glVersion);
    TRACE("GL renderer            : %s.\n", gl_renderer);
    TRACE("GLX version            : %d.%d.\n", WineGLInfo.glxVersion[0], WineGLInfo.glxVersion[1]);
    TRACE("Server GLX version     : %s.\n", WineGLInfo.glxServerVersion);
    TRACE("Server GLX vendor:     : %s.\n", WineGLInfo.glxServerVendor);
    TRACE("Client GLX version     : %s.\n", WineGLInfo.glxClientVersion);
    TRACE("Client GLX vendor:     : %s.\n", WineGLInfo.glxClientVendor);
    TRACE("Direct rendering enabled: %s\n", WineGLInfo.glxDirect ? "True" : "False");

    if(!WineGLInfo.glxDirect)
    {
        int fd = ConnectionNumber(gdi_display);
        struct sockaddr_un uaddr;
        unsigned int uaddrlen = sizeof(struct sockaddr_un);

        /* In general indirect rendering on a local X11 server indicates a driver problem.
         * Detect a local X11 server by checking whether the X11 socket is a Unix socket.
         */
        if(!getsockname(fd, (struct sockaddr *)&uaddr, &uaddrlen) && uaddr.sun_family == AF_UNIX)
            ERR_(winediag)("Direct rendering is disabled, most likely your OpenGL drivers "
                           "haven't been installed correctly (using GL renderer %s, version %s).\n",
                           debugstr_a(gl_renderer), debugstr_a(WineGLInfo.glVersion));
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
            ERR_(winediag)("The Mesa OpenGL driver is using software rendering, most likely your OpenGL "
                           "drivers haven't been installed correctly (using GL renderer %s, version %s).\n",
                           debugstr_a(gl_renderer), debugstr_a(WineGLInfo.glVersion));
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
    wine_tsx11_unlock();
    if (!ret) ERR(" couldn't initialize OpenGL, expect problems\n");
    return ret;
}

static BOOL has_opengl(void)
{
    static int init_done;
    static void *opengl_handle;

    char buffer[200];
    int error_base, event_base;

    if (init_done) return (opengl_handle != NULL);
    init_done = 1;

    /* No need to load any other libraries as according to the ABI, libGL should be self-sufficient
       and include all dependencies */
    opengl_handle = wine_dlopen(SONAME_LIBGL, RTLD_NOW|RTLD_GLOBAL, buffer, sizeof(buffer));
    if (opengl_handle == NULL)
    {
        ERR( "Failed to load libGL: %s\n", buffer );
        ERR( "OpenGL support is disabled.\n");
        return FALSE;
    }

    pglXGetProcAddressARB = wine_dlsym(opengl_handle, "glXGetProcAddressARB", NULL, 0);
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
    LOAD_FUNCPTR(glXCreateGLXPixmap);
    LOAD_FUNCPTR(glXGetCurrentContext);
    LOAD_FUNCPTR(glXGetCurrentDrawable);
    LOAD_FUNCPTR(glXDestroyContext);
    LOAD_FUNCPTR(glXDestroyGLXPixmap);
    LOAD_FUNCPTR(glXGetConfig);
    LOAD_FUNCPTR(glXIsDirect);
    LOAD_FUNCPTR(glXMakeCurrent);
    LOAD_FUNCPTR(glXSwapBuffers);
    LOAD_FUNCPTR(glXQueryExtension);
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
    LOAD_FUNCPTR(glXGetCurrentReadDrawable);
    LOAD_FUNCPTR(glXGetFBConfigs);

    /* Standard OpenGL calls */
    LOAD_FUNCPTR(glBindTexture);
    LOAD_FUNCPTR(glBitmap);
    LOAD_FUNCPTR(glCopyTexSubImage1D);
    LOAD_FUNCPTR(glCopyTexImage2D);
    LOAD_FUNCPTR(glCopyTexSubImage2D);
    LOAD_FUNCPTR(glDrawBuffer);
    LOAD_FUNCPTR(glEndList);
    LOAD_FUNCPTR(glGetError);
    LOAD_FUNCPTR(glGetIntegerv);
    LOAD_FUNCPTR(glGetString);
    LOAD_FUNCPTR(glNewList);
    LOAD_FUNCPTR(glPixelStorei);
    LOAD_FUNCPTR(glReadPixels);
    LOAD_FUNCPTR(glTexImage2D);
    LOAD_FUNCPTR(glFinish);
    LOAD_FUNCPTR(glFlush);
#undef LOAD_FUNCPTR

/* It doesn't matter if these fail. They'll only be used if the driver reports
   the associated extension is available (and if a driver reports the extension
   is available but fails to provide the functions, it's quite broken) */
#define LOAD_FUNCPTR(f) p##f = pglXGetProcAddressARB((const GLubyte *)#f)
    /* ARB GLX Extension */
    LOAD_FUNCPTR(glXCreateContextAttribsARB);
    /* SGI GLX Extension */
    LOAD_FUNCPTR(glXSwapIntervalSGI);
    /* NV GLX Extension */
    LOAD_FUNCPTR(glXAllocateMemoryNV);
    LOAD_FUNCPTR(glXFreeMemoryNV);
#undef LOAD_FUNCPTR

    if(!X11DRV_WineGL_InitOpenglInfo()) goto failed;

    wine_tsx11_lock();
    if (pglXQueryExtension(gdi_display, &error_base, &event_base)) {
        TRACE("GLX is up and running error_base = %d\n", error_base);
    } else {
        wine_tsx11_unlock();
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
    } else if(glxRequireExtension("GLX_SGIX_fbconfig")) {
        pglXChooseFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfigSGIX");
        pglXGetFBConfigAttrib = pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttribSGIX");
        pglXGetVisualFromFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfigSGIX");

        /* The mesa libGL client library seems to forward glXQueryDrawable to the Xserver, so only
         * enable this function when the Xserver understand GLX 1.3 or newer
         */
        pglXQueryDrawable = NULL;
     } else if(strcmp("ATI", WineGLInfo.glxClientVendor) == 0) {
        TRACE("Overriding ATI GLX capabilities!\n");
        pglXChooseFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfig");
        pglXGetFBConfigAttrib = pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttrib");
        pglXGetVisualFromFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfig");
        pglXQueryDrawable = pglXGetProcAddressARB((const GLubyte *) "glXQueryDrawable");

        /* Use client GLX information in case of the ATI drivers. We override the
         * capabilities over here and not somewhere else as ATI might better their
         * life in the future. In case they release proper drivers this block of
         * code won't be called. */
        WineGLInfo.glxExtensions = WineGLInfo.glxClientExtensions;
    } else {
         ERR(" glx_version is %s and GLX_SGIX_fbconfig extension is unsupported. Expect problems.\n", WineGLInfo.glxServerVersion);
    }

    if(glxRequireExtension("GLX_MESA_copy_sub_buffer")) {
        pglXCopySubBufferMESA = pglXGetProcAddressARB((const GLubyte *) "glXCopySubBufferMESA");
    }

    X11DRV_WineGL_LoadExtensions();

    wine_tsx11_unlock();
    return TRUE;

failed:
    wine_dlclose(opengl_handle, NULL, 0);
    opengl_handle = NULL;
    return FALSE;
}

static int describeContext( struct wgl_context *ctx ) {
    int tmp;
    int ctx_vis_id;
    TRACE(" Context %p have (vis:%p):\n", ctx, ctx->vis);
    pglXGetFBConfigAttrib(gdi_display, ctx->fmt->fbconfig, GLX_FBCONFIG_ID, &tmp);
    TRACE(" - FBCONFIG_ID 0x%x\n", tmp);
    pglXGetFBConfigAttrib(gdi_display, ctx->fmt->fbconfig, GLX_VISUAL_ID, &tmp);
    TRACE(" - VISUAL_ID 0x%x\n", tmp);
    ctx_vis_id = tmp;
    return ctx_vis_id;
}

static int ConvertAttribWGLtoGLX(const int* iWGLAttr, int* oGLXAttr, Wine_GLPBuffer* pbuf) {
  int nAttribs = 0;
  unsigned cur = 0; 
  int pop;
  int drawattrib = 0;
  int nvfloatattrib = GLX_DONT_CARE;
  int pixelattrib = GLX_DONT_CARE;

  /* The list of WGL attributes is allowed to be NULL. We don't return here for NULL
   * because we need to do fixups for GLX_DRAWABLE_TYPE/GLX_RENDER_TYPE/GLX_FLOAT_COMPONENTS_NV. */
  while (iWGLAttr && 0 != iWGLAttr[cur]) {
    TRACE("pAttr[%d] = %x\n", cur, iWGLAttr[cur]);

    switch (iWGLAttr[cur]) {
    case WGL_AUX_BUFFERS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_AUX_BUFFERS, pop);
      TRACE("pAttr[%d] = GLX_AUX_BUFFERS: %d\n", cur, pop);
      break;
    case WGL_COLOR_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_BUFFER_SIZE, pop);
      TRACE("pAttr[%d] = GLX_BUFFER_SIZE: %d\n", cur, pop);
      break;
    case WGL_BLUE_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_BLUE_SIZE, pop);
      TRACE("pAttr[%d] = GLX_BLUE_SIZE: %d\n", cur, pop);
      break;
    case WGL_RED_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_RED_SIZE, pop);
      TRACE("pAttr[%d] = GLX_RED_SIZE: %d\n", cur, pop);
      break;
    case WGL_GREEN_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_GREEN_SIZE, pop);
      TRACE("pAttr[%d] = GLX_GREEN_SIZE: %d\n", cur, pop);
      break;
    case WGL_ALPHA_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_ALPHA_SIZE, pop);
      TRACE("pAttr[%d] = GLX_ALPHA_SIZE: %d\n", cur, pop);
      break;
    case WGL_DEPTH_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_DEPTH_SIZE, pop);
      TRACE("pAttr[%d] = GLX_DEPTH_SIZE: %d\n", cur, pop);
      break;
    case WGL_STENCIL_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_STENCIL_SIZE, pop);
      TRACE("pAttr[%d] = GLX_STENCIL_SIZE: %d\n", cur, pop);
      break;
    case WGL_DOUBLE_BUFFER_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_DOUBLEBUFFER, pop);
      TRACE("pAttr[%d] = GLX_DOUBLEBUFFER: %d\n", cur, pop);
      break;
    case WGL_STEREO_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_STEREO, pop);
      TRACE("pAttr[%d] = GLX_STEREO: %d\n", cur, pop);
      break;

    case WGL_PIXEL_TYPE_ARB:
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_PIXEL_TYPE_ARB: %d\n", cur, pop);
      switch (pop) {
      case WGL_TYPE_COLORINDEX_ARB: pixelattrib = GLX_COLOR_INDEX_BIT; break ;
      case WGL_TYPE_RGBA_ARB: pixelattrib = GLX_RGBA_BIT; break ;
      /* This is the same as WGL_TYPE_RGBA_FLOAT_ATI but the GLX constants differ, only the ARB GLX one is widely supported so use that */
      case WGL_TYPE_RGBA_FLOAT_ATI: pixelattrib = GLX_RGBA_FLOAT_BIT; break ;
      case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT: pixelattrib = GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT; break ;
      default:
        ERR("unexpected PixelType(%x)\n", pop);	
        pop = 0;
      }
      break;

    case WGL_SUPPORT_GDI_ARB:
      /* This flag is set in a WineGLPixelFormat */
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_SUPPORT_GDI_ARB: %d\n", cur, pop);
      break;

    case WGL_DRAW_TO_BITMAP_ARB:
      /* This flag is set in a WineGLPixelFormat */
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_DRAW_TO_BITMAP_ARB: %d\n", cur, pop);
      break;

    case WGL_DRAW_TO_WINDOW_ARB:
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_DRAW_TO_WINDOW_ARB: %d\n", cur, pop);
      /* GLX_DRAWABLE_TYPE flags need to be OR'd together. See below. */
      if (pop) {
        drawattrib |= GLX_WINDOW_BIT;
      }
      break;

    case WGL_DRAW_TO_PBUFFER_ARB:
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_DRAW_TO_PBUFFER_ARB: %d\n", cur, pop);
      /* GLX_DRAWABLE_TYPE flags need to be OR'd together. See below. */
      if (pop) {
        drawattrib |= GLX_PBUFFER_BIT;
      }
      break;

    case WGL_ACCELERATION_ARB:
      /* This flag is set in a WineGLPixelFormat */
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_ACCELERATION_ARB: %d\n", cur, pop);
      break;

    case WGL_SUPPORT_OPENGL_ARB:
      pop = iWGLAttr[++cur];
      /** nothing to do, if we are here, supposing support Accelerated OpenGL */
      TRACE("pAttr[%d] = WGL_SUPPORT_OPENGL_ARB: %d\n", cur, pop);
      break;

    case WGL_SWAP_METHOD_ARB:
      pop = iWGLAttr[++cur];
      /* For now we ignore this and just return SWAP_EXCHANGE */
      TRACE("pAttr[%d] = WGL_SWAP_METHOD_ARB: %#x\n", cur, pop);
      break;

    case WGL_PBUFFER_LARGEST_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_LARGEST_PBUFFER, pop);
      TRACE("pAttr[%d] = GLX_LARGEST_PBUFFER: %x\n", cur, pop);
      break;

    case WGL_SAMPLE_BUFFERS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_SAMPLE_BUFFERS_ARB, pop);
      TRACE("pAttr[%d] = GLX_SAMPLE_BUFFERS_ARB: %x\n", cur, pop);
      break;

    case WGL_SAMPLES_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_SAMPLES_ARB, pop);
      TRACE("pAttr[%d] = GLX_SAMPLES_ARB: %x\n", cur, pop);
      break;

    case WGL_TEXTURE_FORMAT_ARB:
    case WGL_TEXTURE_TARGET_ARB:
    case WGL_MIPMAP_TEXTURE_ARB:
      TRACE("WGL_render_texture Attributes: %x as %x\n", iWGLAttr[cur], iWGLAttr[cur + 1]);
      pop = iWGLAttr[++cur];
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
      nvfloatattrib = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_FLOAT_COMPONENTS_NV: %x\n", cur, nvfloatattrib);
      break ;
    case WGL_BIND_TO_TEXTURE_DEPTH_NV:
    case WGL_BIND_TO_TEXTURE_RGB_ARB:
    case WGL_BIND_TO_TEXTURE_RGBA_ARB:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV:
      pop = iWGLAttr[++cur];
      /** cannot be converted, see direct handling on 
       *   - wglGetPixelFormatAttribivARB
       *  TODO: wglChoosePixelFormat
       */
      break ;
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, pop);
      TRACE("pAttr[%d] = GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT: %x\n", cur, pop);
      break ;

    case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT, pop);
      TRACE("pAttr[%d] = GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT: %x\n", cur, pop);
      break ;
    default:
      FIXME("unsupported %x WGL Attribute\n", iWGLAttr[cur]);
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
  if(strstr(WineGLInfo.glxExtensions, "GLX_NV_float_buffer")) {
    PUSH2(oGLXAttr, GLX_FLOAT_COMPONENTS_NV, nvfloatattrib);
    TRACE("pAttr[?] = GLX_FLOAT_COMPONENTS_NV: %#x\n", nvfloatattrib);
  }

  return nAttribs;
}

static int get_render_type_from_fbconfig(Display *display, GLXFBConfig fbconfig)
{
    int render_type=0, render_type_bit;
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

static WineGLPixelFormat *get_formats(Display *display, int *size_ret, int *onscreen_size_ret)
{
    static WineGLPixelFormat *list;
    static int size, onscreen_size;

    int fmt_id, nCfgs, i, run, bmp_formats;
    GLXFBConfig* cfgs;
    XVisualInfo *visinfo;

    wine_tsx11_lock();
    if (list) goto done;

    cfgs = pglXGetFBConfigs(display, DefaultScreen(display), &nCfgs);
    if (NULL == cfgs || 0 == nCfgs) {
        if(cfgs != NULL) XFree(cfgs);
        wine_tsx11_unlock();
        ERR("glXChooseFBConfig returns NULL\n");
        return NULL;
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

    list = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nCfgs + bmp_formats)*sizeof(WineGLPixelFormat));

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
                if(visinfo->depth != screen_depth)
                {
                    XFree(visinfo);
                    continue;
                }

                TRACE("Found onscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                list[size].iPixelFormat = size+1; /* The index starts at 1 */
                list[size].fbconfig = cfgs[i];
                list[size].fmt_id = fmt_id;
                list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                list[size].offscreenOnly = FALSE;
                list[size].dwFlags = 0;
                size++;
                onscreen_size++;

                /* Clone a format if it is bitmap capable for indirect rendering to bitmaps */
                if(check_fbconfig_bitmap_capability(display, cfgs[i]))
                {
                    TRACE("Found bitmap capable format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                    list[size].iPixelFormat = size+1; /* The index starts at 1 */
                    list[size].fbconfig = cfgs[i];
                    list[size].fmt_id = fmt_id;
                    list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                    list[size].offscreenOnly = FALSE;
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
                list[size].iPixelFormat = size+1; /* The index starts at 1 */
                list[size].fbconfig = cfgs[i];
                list[size].fmt_id = fmt_id;
                list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                list[size].offscreenOnly = TRUE;
                list[size].dwFlags = 0;
                size++;
            }

            if (visinfo) XFree(visinfo);
        }
    }

    XFree(cfgs);

done:
    if (size_ret) *size_ret = size;
    if (onscreen_size_ret) *onscreen_size_ret = onscreen_size;
    wine_tsx11_unlock();
    return list;
}

/* GLX can advertise dozens of different pixelformats including offscreen and onscreen ones.
 * In our WGL implementation we only support a subset of these formats namely the format of
 * Wine's main visual and offscreen formats (if they are available).
 * This function converts a WGL format to its corresponding GLX one. It returns a WineGLPixelFormat
 * and it returns the number of supported WGL formats in fmt_count.
 */
static WineGLPixelFormat* ConvertPixelFormatWGLtoGLX(Display *display, int iPixelFormat, BOOL AllowOffscreen, int *fmt_count)
{
    WineGLPixelFormat *list, *res = NULL;
    int size, onscreen_size;

    if (!(list = get_formats(display, &size, &onscreen_size ))) return NULL;

    /* Check if the pixelformat is valid. Note that it is legal to pass an invalid
     * iPixelFormat in case of probing the number of pixelformats.
     */
    if((iPixelFormat > 0) && (iPixelFormat <= size) &&
       (!list[iPixelFormat-1].offscreenOnly || AllowOffscreen)) {
        res = &list[iPixelFormat-1];
        TRACE("Returning fmt_id=%#x for iPixelFormat=%d\n", res->fmt_id, iPixelFormat);
    }

    if(AllowOffscreen)
        *fmt_count = size;
    else
        *fmt_count = onscreen_size;

    TRACE("Number of returned pixelformats=%d\n", *fmt_count);

    return res;
}

/* Search our internal pixelformat list for the WGL format corresponding to the given fbconfig */
static WineGLPixelFormat* ConvertPixelFormatGLXtoWGL(Display *display, int fmt_id, DWORD dwFlags)
{
    WineGLPixelFormat *list;
    int i, size;

    if (!(list = get_formats(display, &size, NULL ))) return NULL;

    for(i=0; i<size; i++) {
        /* A GLX format can appear multiple times in the pixel format list due to fake formats for bitmap rendering.
         * Fake formats might get selected when the user passes the proper flags using the dwFlags parameter. */
        if( (list[i].fmt_id == fmt_id) && ((list[i].dwFlags & dwFlags) == dwFlags) ) {
            TRACE("Returning iPixelFormat %d for fmt_id 0x%x\n", list[i].iPixelFormat, fmt_id);
            return &list[i];
        }
    }
    TRACE("No compatible format found for fmt_id 0x%x\n", fmt_id);
    return NULL;
}

static int pixelformat_from_fbconfig_id(XID fbconfig_id)
{
    WineGLPixelFormat *fmt;

    if (!fbconfig_id) return 0;

    fmt = ConvertPixelFormatGLXtoWGL(gdi_display, fbconfig_id, 0 /* no flags */);
    if(fmt)
        return fmt->iPixelFormat;
    /* This will happen on hwnds without a pixel format set; it's ok */
    return 0;
}


/* Mark any allocated context using the glx drawable 'old' to use 'new' */
void mark_drawable_dirty(Drawable old, Drawable new)
{
    struct wgl_context *ctx;
    LIST_FOR_EACH_ENTRY( ctx, &context_list, struct wgl_context, entry )
    {
        if (old == ctx->drawables[0]) {
            ctx->drawables[0] = new;
            ctx->refresh_drawables = TRUE;
        }
        if (old == ctx->drawables[1]) {
            ctx->drawables[1] = new;
            ctx->refresh_drawables = TRUE;
        }
    }
}

/* Given the current context, make sure its drawable is sync'd */
static inline void sync_context(struct wgl_context *context)
{
    if(context && context->refresh_drawables) {
        if (glxRequireVersion(3))
            pglXMakeContextCurrent(gdi_display, context->drawables[0],
                                   context->drawables[1], context->ctx);
        else
            pglXMakeCurrent(gdi_display, context->drawables[0], context->ctx);
        context->refresh_drawables = FALSE;
    }
}


static GLXContext create_glxcontext(Display *display, struct wgl_context *context, GLXContext shareList)
{
    GLXContext ctx;

    /* We use indirect rendering for rendering to bitmaps. See get_formats for a comment about this. */
    BOOL indirect = (context->fmt->dwFlags & PFD_DRAW_TO_BITMAP) ? FALSE : TRUE;

    if(context->gl3_context)
    {
        if(context->numAttribs)
            ctx = pglXCreateContextAttribsARB(gdi_display, context->fmt->fbconfig, shareList, indirect, context->attribList);
        else
            ctx = pglXCreateContextAttribsARB(gdi_display, context->fmt->fbconfig, shareList, indirect, NULL);
    }
    else if(context->vis)
        ctx = pglXCreateContext(gdi_display, context->vis, shareList, indirect);
    else /* Create a GLX Context for a pbuffer */
        ctx = pglXCreateNewContext(gdi_display, context->fmt->fbconfig, context->fmt->render_type, shareList, TRUE);

    return ctx;
}


Drawable create_glxpixmap(Display *display, XVisualInfo *vis, Pixmap parent)
{
    return pglXCreateGLXPixmap(display, vis, parent);
}


/**
 * glxdrv_DescribePixelFormat
 *
 * Get the pixel-format descriptor associated to the given id
 */
static int glxdrv_DescribePixelFormat(PHYSDEV dev, int iPixelFormat,
                                      UINT nBytes, PIXELFORMATDESCRIPTOR *ppfd)
{
  /*XVisualInfo *vis;*/
  int value;
  int rb,gb,bb,ab;
  WineGLPixelFormat *fmt;
  int ret = 0;
  int fmt_count = 0;

  if (!has_opengl()) return 0;

  TRACE("(%p,%d,%d,%p)\n", dev->hdc, iPixelFormat, nBytes, ppfd);

  /* Look for the iPixelFormat in our list of supported formats. If it is supported we get the index in the FBConfig table and the number of supported formats back */
  fmt = ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, FALSE /* Offscreen */, &fmt_count);
  if (ppfd == NULL) {
      /* The application is only querying the number of pixelformats */
      return fmt_count;
  } else if(fmt == NULL) {
      WARN("unexpected iPixelFormat(%d): not >=1 and <=nFormats(%d), returning NULL!\n", iPixelFormat, fmt_count);
      return 0;
  }

  if (nBytes < sizeof(PIXELFORMATDESCRIPTOR)) {
    ERR("Wrong structure size !\n");
    /* Should set error */
    return 0;
  }

  ret = fmt_count;

  memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
  ppfd->nVersion = 1;

  /* These flags are always the same... */
  ppfd->dwFlags = PFD_SUPPORT_OPENGL;
  /* Now the flags extracted from the Visual */

  wine_tsx11_lock();

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

    ppfd->cRedBits = rb;
    ppfd->cRedShift = gb + bb + ab;
    ppfd->cBlueBits = bb;
    ppfd->cBlueShift = ab;
    ppfd->cGreenBits = gb;
    ppfd->cGreenShift = bb + ab;
    ppfd->cAlphaBits = ab;
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

  wine_tsx11_unlock();

  ppfd->iLayerType = PFD_MAIN_PLANE;

  if (TRACE_ON(wgl)) {
    dump_PIXELFORMATDESCRIPTOR(ppfd);
  }

  return ret;
}

/***********************************************************************
 *		glxdrv_GetPixelFormat
 */
static int glxdrv_GetPixelFormat( HDC hdc )
{
    struct x11drv_escape_get_drawable escape;
    WineGLPixelFormat *fmt;
    int tmp;

    TRACE( "(%p)\n", hdc );

    escape.code = X11DRV_GET_DRAWABLE;
    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape.code), (LPCSTR)&escape.code,
                    sizeof(escape), (LPSTR)&escape ))
        return 0;

    if (!escape.pixel_format) return 0;  /* not set yet */

    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, escape.pixel_format, TRUE, &tmp);
    if (!fmt)
    {
        ERR("Unable to find a WineGLPixelFormat for iPixelFormat=%d\n", escape.pixel_format);
        return 0;
    }
    if (fmt->offscreenOnly)
    {
        /* Offscreen formats can't be used with traditional WGL calls.
         * As has been verified on Windows GetPixelFormat doesn't fail but returns iPixelFormat=1. */
        TRACE("Returning iPixelFormat=1 for offscreen format: %d\n", fmt->iPixelFormat);
        return 1;
    }
    TRACE("(%p): returns %d\n", hdc, escape.pixel_format);
    return escape.pixel_format;
}

/***********************************************************************
 *		glxdrv_SetPixelFormat
 */
static BOOL glxdrv_SetPixelFormat(PHYSDEV dev, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{
    struct glx_physdev *physdev = get_glxdrv_dev( dev );
    WineGLPixelFormat *fmt;
    int value;
    HWND hwnd;

    TRACE("(%p,%d,%p)\n", dev->hdc, iPixelFormat, ppfd);

    if (!has_opengl()) return FALSE;

    if(physdev->pixel_format)  /* cannot change it if already set */
        return (physdev->pixel_format == iPixelFormat);

    /* SetPixelFormat is not allowed on the X root_window e.g. GetDC(0) */
    if(physdev->x11dev->drawable == root_window)
    {
        ERR("Invalid operation on root_window\n");
        return FALSE;
    }

    /* Check if iPixelFormat is in our list of supported formats to see if it is supported. */
    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, FALSE /* Offscreen */, &value);
    if(!fmt) {
        ERR("Invalid iPixelFormat: %d\n", iPixelFormat);
        return FALSE;
    }

    wine_tsx11_lock();
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &value);
    wine_tsx11_unlock();

    hwnd = WindowFromDC(physdev->dev.hdc);
    if(hwnd) {
        if(!(value&GLX_WINDOW_BIT)) {
            WARN("Pixel format %d is not compatible for window rendering\n", iPixelFormat);
            return FALSE;
        }

        if(!SendMessageW(hwnd, WM_X11DRV_SET_WIN_FORMAT, fmt->fmt_id, 0)) {
            ERR("Couldn't set format of the window, returning failure\n");
            return FALSE;
        }
        /* physDev->current_pf will be set by the DCE update */
    }
    else if (GetObjectType( physdev->dev.hdc ) == OBJ_MEMDC) {
        if(!(value&GLX_PIXMAP_BIT)) {
            WARN("Pixel format %d is not compatible for bitmap rendering\n", iPixelFormat);
            return FALSE;
        }

        physdev->pixel_format = iPixelFormat;
        physdev->type = DC_GL_BITMAP;
    }
    else {
        FIXME("called on a non-window, non-bitmap object?\n");
    }

    if (TRACE_ON(wgl)) {
        int gl_test = 0;

        wine_tsx11_lock();
        gl_test = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_FBCONFIG_ID, &value);
        if (gl_test) {
           ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
        } else {
            TRACE(" FBConfig have :\n");
            TRACE(" - FBCONFIG_ID   0x%x\n", value);
            pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_VISUAL_ID, &value);
            TRACE(" - VISUAL_ID     0x%x\n", value);
            pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &value);
            TRACE(" - DRAWABLE_TYPE 0x%x\n", value);
        }
        wine_tsx11_unlock();
    }
    return TRUE;
}

/***********************************************************************
 *		glxdrv_wglCopyContext
 */
static BOOL glxdrv_wglCopyContext(struct wgl_context *src, struct wgl_context *dst, UINT mask)
{
    TRACE("%p -> %p mask %#x\n", src, dst, mask);

    wine_tsx11_lock();
    pglXCopyContext(gdi_display, src->ctx, dst->ctx, mask);
    wine_tsx11_unlock();

    /* As opposed to wglCopyContext, glXCopyContext doesn't return anything, so hopefully we passed */
    return TRUE;
}

/***********************************************************************
 *		glxdrv_wglCreateContext
 */
static struct wgl_context *glxdrv_wglCreateContext( HDC hdc )
{
    struct x11drv_escape_get_drawable escape;
    struct wgl_context *ret;
    WineGLPixelFormat *fmt;
    int fmt_count = 0;

    TRACE( "(%p)\n", hdc );

    escape.code = X11DRV_GET_DRAWABLE;
    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape.code), (LPCSTR)&escape.code,
                    sizeof(escape), (LPSTR)&escape ))
        return 0;

    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, escape.pixel_format, TRUE /* Offscreen */, &fmt_count);
    /* We can render using the iPixelFormat (1) of Wine's Main visual AND using some offscreen formats.
     * Note that standard WGL-calls don't recognize offscreen-only formats. For that reason pbuffers
     * use a sort of 'proxy' HDC (wglGetPbufferDCARB).
     * If this fails something is very wrong on the system. */
    if(!fmt) {
        ERR("Cannot get FB Config for iPixelFormat %d, expect problems!\n", escape.pixel_format);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    if (!(ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret)))) return 0;

    ret->hdc = hdc;
    ret->fmt = fmt;
    ret->has_been_current = FALSE;
    ret->sharing = FALSE;

    wine_tsx11_lock();
    ret->vis = pglXGetVisualFromFBConfig(gdi_display, fmt->fbconfig);
    ret->ctx = create_glxcontext(gdi_display, ret, NULL);
    list_add_head( &context_list, &ret->entry );
    wine_tsx11_unlock();

    TRACE(" creating context %p (GL context creation delayed)\n", ret);
    return ret;
}

/***********************************************************************
 *		glxdrv_wglDeleteContext
 */
static void glxdrv_wglDeleteContext(struct wgl_context *ctx)
{
    TRACE("(%p)\n", ctx);

    wine_tsx11_lock();
    list_remove( &ctx->entry );
    if (ctx->ctx) pglXDestroyContext( gdi_display, ctx->ctx );
    if (ctx->glxpixmap) pglXDestroyGLXPixmap( gdi_display, ctx->glxpixmap );
    if (ctx->pixmap) XFreePixmap( gdi_display, ctx->pixmap );
    if (ctx->vis) XFree( ctx->vis );
    wine_tsx11_unlock();

    HeapFree( GetProcessHeap(), 0, ctx );
}

/**
 * X11DRV_wglGetCurrentReadDCARB
 *
 * For OpenGL32 wglGetCurrentReadDCARB.
 */
static HDC WINAPI X11DRV_wglGetCurrentReadDCARB(void) 
{
    HDC ret = 0;
    struct wgl_context *ctx = NtCurrentTeb()->glContext;

    if (ctx) ret = ctx->read_hdc;

    TRACE(" returning %p (GL drawable %lu)\n", ret, ctx ? ctx->drawables[1] : 0);
    return ret;
}

/***********************************************************************
 *		glxdrv_wglGetProcAddress
 */
static PROC glxdrv_wglGetProcAddress(LPCSTR lpszProc)
{
    int i, j;
    const WineGLExtension *ext;

    int padding = 32 - strlen(lpszProc);
    if (padding < 0)
        padding = 0;

    /* Check the table of WGL extensions to see if we need to return a WGL extension
     * or a function pointer to a native OpenGL function. */
    if(strncmp(lpszProc, "wgl", 3) != 0) {
        return pglXGetProcAddressARB((const GLubyte*)lpszProc);
    } else {
        TRACE("('%s'):%*s", lpszProc, padding, " ");
        for (i = 0; i < WineGLExtensionListSize; ++i) {
            ext = WineGLExtensionList[i];
            for (j = 0; ext->extEntryPoints[j].funcName; ++j) {
                if (strcmp(ext->extEntryPoints[j].funcName, lpszProc) == 0) {
                    TRACE("(%p) - WineGL\n", ext->extEntryPoints[j].funcAddress);
                    return ext->extEntryPoints[j].funcAddress;
                }
            }
        }
    }

    WARN("(%s) - not found\n", lpszProc);
    return NULL;
}

static GLXPixmap get_context_pixmap( HDC hdc, struct wgl_context *ctx )
{
    if (!ctx->pixmap)
    {
        BITMAP bmp;

        GetObjectW( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bmp), &bmp );

        wine_tsx11_lock();
        ctx->pixmap = XCreatePixmap( gdi_display, root_window,
                                     bmp.bmWidth, bmp.bmHeight, ctx->vis->depth );
        ctx->glxpixmap = pglXCreateGLXPixmap( gdi_display, ctx->vis, ctx->pixmap );
        wine_tsx11_unlock();
        ctx->pixmap_size.cx = bmp.bmWidth;
        ctx->pixmap_size.cy = bmp.bmHeight;
    }
    return ctx->glxpixmap;
}

/***********************************************************************
 *		glxdrv_wglMakeCurrent
 */
static BOOL glxdrv_wglMakeCurrent(HDC hdc, struct wgl_context *ctx)
{
    BOOL ret;
    struct x11drv_escape_get_drawable escape;

    TRACE("(%p,%p)\n", hdc, ctx);

    if (!ctx)
    {
        wine_tsx11_lock();
        ret = pglXMakeCurrent(gdi_display, None, NULL);
        wine_tsx11_unlock();
        NtCurrentTeb()->glContext = NULL;
        return TRUE;
    }

    escape.code = X11DRV_GET_DRAWABLE;
    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape.code), (LPCSTR)&escape.code,
                    sizeof(escape), (LPSTR)&escape ))
        return FALSE;

    if (!escape.pixel_format)
    {
        WARN("Trying to use an invalid drawable\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (ctx->fmt->iPixelFormat != escape.pixel_format)
    {
        WARN( "mismatched pixel format hdc %p %u ctx %p %u\n",
              hdc, escape.pixel_format, ctx, ctx->fmt->iPixelFormat );
        SetLastError( ERROR_INVALID_PIXEL_FORMAT );
        return FALSE;
    }
    else
    {
        if (escape.gl_type == DC_GL_BITMAP) escape.gl_drawable = get_context_pixmap( hdc, ctx );

        wine_tsx11_lock();

        if (TRACE_ON(wgl)) {
            int vis_id;
            pglXGetFBConfigAttrib(gdi_display, ctx->fmt->fbconfig, GLX_VISUAL_ID, &vis_id);
            describeContext(ctx);
            TRACE("hdc %p drawable %lx fmt %u vis %x ctx %p\n", hdc,
                  escape.gl_drawable, escape.pixel_format, vis_id, ctx->ctx);
        }

        ret = pglXMakeCurrent(gdi_display, escape.gl_drawable, ctx->ctx);

        if (ret)
        {
            NtCurrentTeb()->glContext = ctx;

            ctx->has_been_current = TRUE;
            ctx->hdc = hdc;
            ctx->read_hdc = hdc;
            ctx->drawables[0] = escape.gl_drawable;
            ctx->drawables[1] = escape.gl_drawable;
            ctx->refresh_drawables = FALSE;

            if (escape.gl_type == DC_GL_BITMAP) pglDrawBuffer(GL_FRONT_LEFT);
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
        wine_tsx11_unlock();
    }
    TRACE(" returning %s\n", (ret ? "True" : "False"));
    return ret;
}

/***********************************************************************
 *		glxdrv_wglMakeContextCurrentARB
 */
static BOOL glxdrv_wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, struct wgl_context *ctx )
{
    struct x11drv_escape_get_drawable escape_draw, escape_read;
    BOOL ret;

    TRACE("(%p,%p,%p)\n", draw_hdc, read_hdc, ctx);

    if (!ctx)
    {
        wine_tsx11_lock();
        ret = pglXMakeCurrent(gdi_display, None, NULL);
        wine_tsx11_unlock();
        NtCurrentTeb()->glContext = NULL;
        return TRUE;
    }

    escape_draw.code = X11DRV_GET_DRAWABLE;
    if (!ExtEscape( draw_hdc, X11DRV_ESCAPE, sizeof(escape_draw.code), (LPCSTR)&escape_draw.code,
                    sizeof(escape_draw), (LPSTR)&escape_draw ))
        return FALSE;

    escape_read.code = X11DRV_GET_DRAWABLE;
    if (!ExtEscape( read_hdc, X11DRV_ESCAPE, sizeof(escape_read.code), (LPCSTR)&escape_read.code,
                    sizeof(escape_read), (LPSTR)&escape_read ))
        return FALSE;

    if (!escape_draw.pixel_format)
    {
        WARN("Trying to use an invalid drawable\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    else
    {
        if (!pglXMakeContextCurrent) return FALSE;

        if (escape_draw.gl_type == DC_GL_BITMAP) escape_draw.gl_drawable = get_context_pixmap( draw_hdc, ctx );
        if (escape_read.gl_type == DC_GL_BITMAP) escape_read.gl_drawable = get_context_pixmap( read_hdc, ctx );

        wine_tsx11_lock();
        ret = pglXMakeContextCurrent(gdi_display, escape_draw.gl_drawable, escape_read.gl_drawable, ctx->ctx);
        if (ret)
        {
            ctx->has_been_current = TRUE;
            ctx->hdc = draw_hdc;
            ctx->read_hdc = read_hdc;
            ctx->drawables[0] = escape_draw.gl_drawable;
            ctx->drawables[1] = escape_read.gl_drawable;
            ctx->refresh_drawables = FALSE;
            NtCurrentTeb()->glContext = ctx;
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
        wine_tsx11_unlock();
    }

    TRACE(" returning %s\n", (ret ? "True" : "False"));
    return ret;
}

/***********************************************************************
 *		glxdrv_wglShareLists
 */
static BOOL glxdrv_wglShareLists(struct wgl_context *org, struct wgl_context *dest)
{
    TRACE("(%p, %p)\n", org, dest);

    /* Sharing of display lists works differently in GLX and WGL. In case of GLX it is done
     * at context creation time but in case of WGL it is done using wglShareLists.
     * In the past we tried to emulate wglShareLists by delaying GLX context creation until
     * either a wglMakeCurrent or wglShareLists. This worked fine for most apps but it causes
     * issues for OpenGL 3 because there wglCreateContextAttribsARB can fail in a lot of cases,
     * so there delaying context creation doesn't work.
     *
     * The new approach is to create a GLX context in wglCreateContext / wglCreateContextAttribsARB
     * and when a program requests sharing we recreate the destination context if it hasn't been made
     * current or when it hasn't shared display lists before.
     */

    if((org->has_been_current && dest->has_been_current) || dest->has_been_current)
    {
        ERR("Could not share display lists, one of the contexts has been current already !\n");
        return FALSE;
    }
    else if(dest->sharing)
    {
        ERR("Could not share display lists because hglrc2 has already shared lists before\n");
        return FALSE;
    }
    else
    {
        if((GetObjectType(org->hdc) == OBJ_MEMDC) ^ (GetObjectType(dest->hdc) == OBJ_MEMDC))
        {
            WARN("Attempting to share a context between a direct and indirect rendering context, expect issues!\n");
        }

        wine_tsx11_lock();
        describeContext(org);
        describeContext(dest);

        /* Re-create the GLX context and share display lists */
        pglXDestroyContext(gdi_display, dest->ctx);
        dest->ctx = create_glxcontext(gdi_display, dest, org->ctx);
        wine_tsx11_unlock();
        TRACE(" re-created an OpenGL context (%p) for Wine context %p sharing lists with OpenGL ctx %p\n", dest->ctx, dest, org->ctx);

        org->sharing = TRUE;
        dest->sharing = TRUE;
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *		glxdrv_wglGetCurrentDC
 */
static HDC glxdrv_wglGetCurrentDC( struct wgl_context *ctx )
{
    TRACE("hdc %p\n", ctx->hdc);
    return ctx->hdc;
}

/* WGL helper function which handles differences in glGetIntegerv from WGL and GLX */
static void WINAPI X11DRV_wglGetIntegerv(GLenum pname, GLint* params)
{
    wine_tsx11_lock();
    switch(pname)
    {
    case GL_DEPTH_BITS:
        {
            struct wgl_context *ctx = NtCurrentTeb()->glContext;

            pglGetIntegerv(pname, params);
            /**
             * if we cannot find a Wine Context
             * we only have the default wine desktop context,
             * so if we have only a 24 depth say we have 32
             */
            if (!ctx && *params == 24) {
                *params = 32;
            }
            TRACE("returns GL_DEPTH_BITS as '%d'\n", *params);
            break;
        }
    case GL_ALPHA_BITS:
        {
            struct wgl_context *ctx = NtCurrentTeb()->glContext;

            pglXGetFBConfigAttrib(gdi_display, ctx->fmt->fbconfig, GLX_ALPHA_SIZE, params);
            TRACE("returns GL_ALPHA_BITS as '%d'\n", *params);
            break;
        }
    default:
        pglGetIntegerv(pname, params);
        break;
    }
    wine_tsx11_unlock();
}

static void flush_pixmap( struct wgl_context *ctx )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct gdi_image_bits bits;

    if (!get_pixmap_image( ctx->pixmap, ctx->pixmap_size.cx, ctx->pixmap_size.cy, ctx->vis, info, &bits ))
    {
        HBITMAP bitmap = GetCurrentObject( ctx->hdc, OBJ_BITMAP );
        SetDIBits( 0, bitmap, 0, ctx->pixmap_size.cy, bits.ptr, info, DIB_RGB_COLORS );
        if (bits.free) bits.free( &bits );
    }
}

static void flush_gl_drawable( struct glx_physdev *physdev )
{
    RECT rect;
    int w = physdev->x11dev->dc_rect.right - physdev->x11dev->dc_rect.left;
    int h = physdev->x11dev->dc_rect.bottom - physdev->x11dev->dc_rect.top;
    Drawable src = physdev->drawable;

    if (w <= 0 || h <= 0) return;

    switch (physdev->type)
    {
    case DC_GL_PIXMAP_WIN:
        src = physdev->pixmap;
        /* fall through */
    case DC_GL_CHILD_WIN:
        /* The GL drawable may be lagged behind if we don't flush first, so
         * flush the display make sure we copy up-to-date data */
        wine_tsx11_lock();
        XFlush(gdi_display);
        XSetFunction(gdi_display, physdev->x11dev->gc, GXcopy);
        XCopyArea(gdi_display, src, physdev->x11dev->drawable, physdev->x11dev->gc, 0, 0, w, h,
                  physdev->x11dev->dc_rect.left, physdev->x11dev->dc_rect.top);
        wine_tsx11_unlock();
        SetRect( &rect, 0, 0, w, h );
        add_device_bounds( physdev->x11dev, &rect );
    default:
        break;
    }
}


static void WINAPI X11DRV_wglFinish(void)
{
    struct wgl_context *ctx = NtCurrentTeb()->glContext;
    enum x11drv_escape_codes code = X11DRV_FLUSH_GL_DRAWABLE;

    wine_tsx11_lock();
    sync_context(ctx);
    pglFinish();
    wine_tsx11_unlock();
    if (ctx)
    {
        if (ctx->pixmap) flush_pixmap( ctx );
        else ExtEscape( ctx->hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );
    }
}

static void WINAPI X11DRV_wglFlush(void)
{
    struct wgl_context *ctx = NtCurrentTeb()->glContext;
    enum x11drv_escape_codes code = X11DRV_FLUSH_GL_DRAWABLE;

    wine_tsx11_lock();
    sync_context(ctx);
    pglFlush();
    wine_tsx11_unlock();
    if (ctx)
    {
        if (ctx->pixmap) flush_pixmap( ctx );
        else ExtEscape( ctx->hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );
    }
}

/***********************************************************************
 *		glxdrv_wglCreateContextAttribsARB
 */
static struct wgl_context *glxdrv_wglCreateContextAttribsARB( HDC hdc, struct wgl_context *hShareContext,
                                                              const int* attribList )
{
    struct x11drv_escape_get_drawable escape;
    struct wgl_context *ret;
    WineGLPixelFormat *fmt;
    int fmt_count = 0;

    TRACE("(%p %p %p)\n", hdc, hShareContext, attribList);

    escape.code = X11DRV_GET_DRAWABLE;
    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape.code), (LPCSTR)&escape.code,
                    sizeof(escape), (LPSTR)&escape ))
        return 0;

    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, escape.pixel_format, TRUE /* Offscreen */, &fmt_count);
    /* wglCreateContextAttribsARB supports ALL pixel formats, so also offscreen ones.
     * If this fails something is very wrong on the system. */
    if(!fmt)
    {
        ERR("Cannot get FB Config for iPixelFormat %d, expect problems!\n", escape.pixel_format);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    if (!(ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret)))) return 0;

    ret->hdc = hdc;
    ret->fmt = fmt;
    ret->vis = NULL; /* glXCreateContextAttribsARB requires a fbconfig instead of a visual */
    ret->gl3_context = TRUE;

    ret->numAttribs = 0;
    if(attribList)
    {
        int *pAttribList = (int*)attribList;
        int *pContextAttribList = &ret->attribList[0];
        /* attribList consists of pairs {token, value] terminated with 0 */
        while(pAttribList[0] != 0)
        {
            TRACE("%#x %#x\n", pAttribList[0], pAttribList[1]);
            switch(pAttribList[0])
            {
                case WGL_CONTEXT_MAJOR_VERSION_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_MAJOR_VERSION_ARB;
                    pContextAttribList[1] = pAttribList[1];
                    break;
                case WGL_CONTEXT_MINOR_VERSION_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_MINOR_VERSION_ARB;
                    pContextAttribList[1] = pAttribList[1];
                    break;
                case WGL_CONTEXT_LAYER_PLANE_ARB:
                    break;
                case WGL_CONTEXT_FLAGS_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_FLAGS_ARB;
                    pContextAttribList[1] = pAttribList[1];
                    break;
                case WGL_CONTEXT_PROFILE_MASK_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_PROFILE_MASK_ARB;
                    pContextAttribList[1] = pAttribList[1];
                    break;
                default:
                    ERR("Unhandled attribList pair: %#x %#x\n", pAttribList[0], pAttribList[1]);
            }

            ret->numAttribs++;
            pAttribList += 2;
            pContextAttribList += 2;
        }
    }

    wine_tsx11_lock();
    X11DRV_expect_error(gdi_display, GLXErrorHandler, NULL);
    ret->ctx = create_glxcontext(gdi_display, ret, NULL);

    XSync(gdi_display, False);
    if(X11DRV_check_error() || !ret->ctx)
    {
        /* In the future we should convert the GLX error to a win32 one here if needed */
        ERR("Context creation failed\n");
        HeapFree( GetProcessHeap(), 0, ret );
        wine_tsx11_unlock();
        return NULL;
    }

    list_add_head( &context_list, &ret->entry );
    wine_tsx11_unlock();
    TRACE(" creating context %p\n", ret);
    return ret;
}

/**
 * X11DRV_wglGetExtensionsStringARB
 *
 * WGL_ARB_extensions_string: wglGetExtensionsStringARB
 */
static const char * WINAPI X11DRV_wglGetExtensionsStringARB(HDC hdc) {
    TRACE("() returning \"%s\"\n", WineGLInfo.wglExtensions);
    return WineGLInfo.wglExtensions;
}

/**
 * X11DRV_wglCreatePbufferARB
 *
 * WGL_ARB_pbuffer: wglCreatePbufferARB
 */
static HPBUFFERARB WINAPI X11DRV_wglCreatePbufferARB(HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList)
{
    Wine_GLPBuffer* object = NULL;
    WineGLPixelFormat *fmt = NULL;
    int nCfgs = 0;
    int attribs[256];
    int nAttribs = 0;

    TRACE("(%p, %d, %d, %d, %p)\n", hdc, iPixelFormat, iWidth, iHeight, piAttribList);

    if (0 >= iPixelFormat) {
        ERR("(%p): unexpected iPixelFormat(%d) <= 0, returns NULL\n", hdc, iPixelFormat);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL; /* unexpected error */
    }

    /* Convert the WGL pixelformat to a GLX format, if it fails then the format is invalid */
    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, TRUE /* Offscreen */, &nCfgs);
    if(!fmt) {
        ERR("(%p): unexpected iPixelFormat(%d) > nFormats(%d), returns NULL\n", hdc, iPixelFormat, nCfgs);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        goto create_failed; /* unexpected error */
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Wine_GLPBuffer));
    if (NULL == object) {
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        goto create_failed; /* unexpected error */
    }
    object->hdc = hdc;
    object->display = gdi_display;
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
                        SetLastError(ERROR_INVALID_DATA);
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
                            SetLastError(ERROR_INVALID_DATA);
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
                        SetLastError(ERROR_INVALID_DATA);
                        goto create_failed;
                    }
                    switch (attr_v) {
                        case WGL_TEXTURE_CUBE_MAP_ARB: {
                            if (iWidth != iHeight) {
                                SetLastError(ERROR_INVALID_DATA);
                                goto create_failed;
                            }
                            object->texture_target = GL_TEXTURE_CUBE_MAP;
                            object->texture_bind_target = GL_TEXTURE_BINDING_CUBE_MAP;
                           break;
                        }
                        case WGL_TEXTURE_1D_ARB: {
                            if (1 != iHeight) {
                                SetLastError(ERROR_INVALID_DATA);
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
                            SetLastError(ERROR_INVALID_DATA);
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
                    SetLastError(ERROR_INVALID_DATA);
                    goto create_failed;
                }
                break;
            }
        }
        ++piAttribList;
    }

    PUSH1(attribs, None);
    wine_tsx11_lock();
    object->drawable = pglXCreatePbuffer(gdi_display, fmt->fbconfig, attribs);
    wine_tsx11_unlock();
    TRACE("new Pbuffer drawable as %lx\n", object->drawable);
    if (!object->drawable) {
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        goto create_failed; /* unexpected error */
    }
    TRACE("->(%p)\n", object);
    return (HPBUFFERARB)object;

create_failed:
    HeapFree(GetProcessHeap(), 0, object);
    TRACE("->(FAILED)\n");
    return NULL;
}

/**
 * X11DRV_wglDestroyPbufferARB
 *
 * WGL_ARB_pbuffer: wglDestroyPbufferARB
 */
static GLboolean WINAPI X11DRV_wglDestroyPbufferARB(HPBUFFERARB hPbuffer)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer *)hPbuffer;
    TRACE("(%p)\n", hPbuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    wine_tsx11_lock();
    pglXDestroyPbuffer(object->display, object->drawable);
    wine_tsx11_unlock();
    HeapFree(GetProcessHeap(), 0, object);
    return GL_TRUE;
}

/**
 * X11DRV_wglGetPbufferDCARB
 *
 * WGL_ARB_pbuffer: wglGetPbufferDCARB
 */
static HDC WINAPI X11DRV_wglGetPbufferDCARB(HPBUFFERARB hPbuffer)
{
    struct x11drv_escape_set_drawable escape;
    Wine_GLPBuffer* object = (Wine_GLPBuffer *)hPbuffer;
    HDC hdc;

    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    hdc = CreateDCA( "DISPLAY", NULL, NULL, NULL );
    if (!hdc) return 0;

    escape.code = X11DRV_SET_DRAWABLE;
    escape.drawable = object->drawable;
    escape.mode = IncludeInferiors;
    SetRect( &escape.drawable_rect, 0, 0, object->width, object->height );
    escape.dc_rect = escape.drawable_rect;
    escape.fbconfig_id = object->fmt->fmt_id;
    escape.gl_drawable = object->drawable;
    escape.pixmap = 0;
    escape.gl_type = DC_GL_PBUFFER;
    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );

    TRACE( "(%p)->(%p)\n", hPbuffer, hdc );
    return hdc;
}

/**
 * X11DRV_wglQueryPbufferARB
 *
 * WGL_ARB_pbuffer: wglQueryPbufferARB
 */
static GLboolean WINAPI X11DRV_wglQueryPbufferARB(HPBUFFERARB hPbuffer, int iAttribute, int *piValue)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer *)hPbuffer;
    TRACE("(%p, 0x%x, %p)\n", hPbuffer, iAttribute, piValue);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    switch (iAttribute) {
        case WGL_PBUFFER_WIDTH_ARB:
            wine_tsx11_lock();
            pglXQueryDrawable(object->display, object->drawable, GLX_WIDTH, (unsigned int*) piValue);
            wine_tsx11_unlock();
            break;
        case WGL_PBUFFER_HEIGHT_ARB:
            wine_tsx11_lock();
            pglXQueryDrawable(object->display, object->drawable, GLX_HEIGHT, (unsigned int*) piValue);
            wine_tsx11_unlock();
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
                    SetLastError(ERROR_INVALID_HANDLE);
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
                    SetLastError(ERROR_INVALID_DATA);
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
static int WINAPI X11DRV_wglReleasePbufferDCARB(HPBUFFERARB hPbuffer, HDC hdc)
{
    TRACE("(%p, %p)\n", hPbuffer, hdc);
    return DeleteDC(hdc);
}

/**
 * X11DRV_wglSetPbufferAttribARB
 *
 * WGL_ARB_pbuffer: wglSetPbufferAttribARB
 */
static GLboolean WINAPI X11DRV_wglSetPbufferAttribARB(HPBUFFERARB hPbuffer, const int *piAttribList)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer *)hPbuffer;
    GLboolean ret = GL_FALSE;

    WARN("(%p, %p): alpha-testing, report any problem\n", hPbuffer, piAttribList);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (1 == use_render_texture_emulation) {
        return GL_TRUE;
    }
    return ret;
}

/**
 * X11DRV_wglChoosePixelFormatARB
 *
 * WGL_ARB_pixel_format: wglChoosePixelFormatARB
 */
static GLboolean WINAPI X11DRV_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)
{
    int gl_test = 0;
    int attribs[256];
    int nAttribs = 0;
    GLXFBConfig* cfgs = NULL;
    int nCfgs = 0;
    int it;
    int fmt_id;
    WineGLPixelFormat *fmt;
    UINT pfmt_it = 0;
    int run;
    int i;
    DWORD dwFlags = 0;

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
     * Flags like PFD_SUPPORT_GDI, PFD_DRAW_TO_BITMAP and others are a property of the WineGLPixelFormat. We don't query these attributes
     * using glXChooseFBConfig but we filter the result of glXChooseFBConfig later on by passing a dwFlags to 'ConvertPixelFormatGLXtoWGL'. */
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
        }
    }

    /* Search for FB configurations matching the requirements in attribs */
    wine_tsx11_lock();
    cfgs = pglXChooseFBConfig(gdi_display, DefaultScreen(gdi_display), attribs, &nCfgs);
    if (NULL == cfgs) {
        wine_tsx11_unlock();
        WARN("Compatible Pixel Format not found\n");
        return GL_FALSE;
    }

    /* Loop through all matching formats and check if they are suitable.
     * Note that this function should at max return nMaxFormats different formats */
    for(run=0; run < 2; run++)
    {
        for (it = 0; it < nCfgs && pfmt_it < nMaxFormats; ++it)
        {
            gl_test = pglXGetFBConfigAttrib(gdi_display, cfgs[it], GLX_FBCONFIG_ID, &fmt_id);
            if (gl_test) {
                ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
                continue;
            }

            /* Search for the format in our list of compatible formats */
            fmt = ConvertPixelFormatGLXtoWGL(gdi_display, fmt_id, dwFlags);
            if(!fmt)
                continue;

            /* During the first run we only want onscreen formats and during the second only offscreen 'XOR' */
            if( ((run == 0) && fmt->offscreenOnly) || ((run == 1) && !fmt->offscreenOnly) )
                continue;

            piFormats[pfmt_it] = fmt->iPixelFormat;
            TRACE("at %d/%d found FBCONFIG_ID 0x%x (%d)\n", it + 1, nCfgs, fmt_id, piFormats[pfmt_it]);
            pfmt_it++;
        }
    }

    *nNumFormats = pfmt_it;
    /** free list */
    XFree(cfgs);
    wine_tsx11_unlock();
    return GL_TRUE;
}

/**
 * X11DRV_wglGetPixelFormatAttribivARB
 *
 * WGL_ARB_pixel_format: wglGetPixelFormatAttribivARB
 */
static GLboolean WINAPI X11DRV_wglGetPixelFormatAttribivARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues)
{
    UINT i;
    WineGLPixelFormat *fmt = NULL;
    int hTest;
    int tmp;
    int curGLXAttr = 0;
    int nWGLFormats = 0;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);

    if (0 < iLayerPlane) {
        FIXME("unsupported iLayerPlane(%d) > 0, returns FALSE\n", iLayerPlane);
        return GL_FALSE;
    }

    /* Convert the WGL pixelformat to a GLX one, if this fails then most likely the iPixelFormat isn't supported.
    * We don't have to fail yet as a program can specify an invalid iPixelFormat (lets say 0) if it wants to query
    * the number of supported WGL formats. Whether the iPixelFormat is valid is handled in the for-loop below. */
    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, TRUE /* Offscreen */, &nWGLFormats);
    if(!fmt) {
        WARN("Unable to convert iPixelFormat %d to a GLX one!\n", iPixelFormat);
    }

    wine_tsx11_lock();
    for (i = 0; i < nAttributes; ++i) {
        const int curWGLAttr = piAttributes[i];
        TRACE("pAttr[%d] = %x\n", i, curWGLAttr);

        switch (curWGLAttr) {
            case WGL_NUMBER_PIXEL_FORMATS_ARB:
                piValues[i] = nWGLFormats; 
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
                piValues[i] = (fmt->dwFlags & PFD_SUPPORT_GDI) ? TRUE : FALSE;
                continue;

            case WGL_DRAW_TO_BITMAP_ARB:
                if (!fmt) goto pix_error;
                piValues[i] = (fmt->dwFlags & PFD_DRAW_TO_BITMAP) ? TRUE : FALSE;
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
                /* For now return SWAP_EXCHANGE_ARB which is the best type of buffer switch available.
                 * Later on we can also use GLX_OML_swap_method on drivers which support this. At this
                 * point only ATI offers this.
                 */
                piValues[i] = WGL_SWAP_EXCHANGE_ARB;
                break;

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
    wine_tsx11_unlock();
    return GL_TRUE;

get_error:
    wine_tsx11_unlock();
    ERR("(%p): unexpected failure on GetFBConfigAttrib(%x) returns FALSE\n", hdc, curGLXAttr);
    return GL_FALSE;

pix_error:
    wine_tsx11_unlock();
    ERR("(%p): unexpected iPixelFormat(%d) vs nFormats(%d), returns FALSE\n", hdc, iPixelFormat, nWGLFormats);
    return GL_FALSE;
}

/**
 * X11DRV_wglGetPixelFormatAttribfvARB
 *
 * WGL_ARB_pixel_format: wglGetPixelFormatAttribfvARB
 */
static GLboolean WINAPI X11DRV_wglGetPixelFormatAttribfvARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues)
{
    int *attr;
    int ret;
    UINT i;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);

    /* Allocate a temporary array to store integer values */
    attr = HeapAlloc(GetProcessHeap(), 0, nAttributes * sizeof(int));
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

    HeapFree(GetProcessHeap(), 0, attr);
    return ret;
}

/**
 * X11DRV_wglBindTexImageARB
 *
 * WGL_ARB_render_texture: wglBindTexImageARB
 */
static GLboolean WINAPI X11DRV_wglBindTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer *)hPbuffer;
    GLboolean ret = GL_FALSE;

    TRACE("(%p, %d)\n", hPbuffer, iBuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }

    if (1 == use_render_texture_emulation) {
        static int init = 0;
        int prev_binded_texture = 0;
        GLXContext prev_context;
        Drawable prev_drawable;
        GLXContext tmp_context;

        wine_tsx11_lock();
        prev_context = pglXGetCurrentContext();
        prev_drawable = pglXGetCurrentDrawable();

        /* Our render_texture emulation is basic and lacks some features (1D/Cube support).
           This is mostly due to lack of demos/games using them. Further the use of glReadPixels
           isn't ideal performance wise but I wasn't able to get other ways working.
        */
        if(!init) {
            init = 1; /* Only show the FIXME once for performance reasons */
            FIXME("partial stub!\n");
        }

        TRACE("drawable=%lx, context=%p\n", object->drawable, prev_context);
        tmp_context = pglXCreateNewContext(gdi_display, object->fmt->fbconfig, object->fmt->render_type, prev_context, True);

        pglGetIntegerv(object->texture_bind_target, &prev_binded_texture);

        /* Switch to our pbuffer */
        pglXMakeCurrent(gdi_display, object->drawable, tmp_context);

        /* Make sure that the prev_binded_texture is set as the current texture state isn't shared between contexts.
         * After that upload the pbuffer texture data. */
        pglBindTexture(object->texture_target, prev_binded_texture);
        pglCopyTexImage2D(object->texture_target, 0, object->use_render_texture, 0, 0, object->width, object->height, 0);

        /* Switch back to the original drawable and upload the pbuffer-texture */
        pglXMakeCurrent(object->display, prev_drawable, prev_context);
        pglXDestroyContext(gdi_display, tmp_context);
        wine_tsx11_unlock();
        return GL_TRUE;
    }

    return ret;
}

/**
 * X11DRV_wglReleaseTexImageARB
 *
 * WGL_ARB_render_texture: wglReleaseTexImageARB
 */
static GLboolean WINAPI X11DRV_wglReleaseTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer *)hPbuffer;
    GLboolean ret = GL_FALSE;

    TRACE("(%p, %d)\n", hPbuffer, iBuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (1 == use_render_texture_emulation) {
        return GL_TRUE;
    }
    return ret;
}

/**
 * X11DRV_wglGetExtensionsStringEXT
 *
 * WGL_EXT_extensions_string: wglGetExtensionsStringEXT
 */
static const char * WINAPI X11DRV_wglGetExtensionsStringEXT(void) {
    TRACE("() returning \"%s\"\n", WineGLInfo.wglExtensions);
    return WineGLInfo.wglExtensions;
}

/**
 * X11DRV_wglGetSwapIntervalEXT
 *
 * WGL_EXT_swap_control: wglGetSwapIntervalEXT
 */
static int WINAPI X11DRV_wglGetSwapIntervalEXT(VOID) {
    /* GLX_SGI_swap_control doesn't have any provisions for getting the swap
     * interval, so the swap interval has to be tracked. */
    TRACE("()\n");
    return swap_interval;
}

/**
 * X11DRV_wglSwapIntervalEXT
 *
 * WGL_EXT_swap_control: wglSwapIntervalEXT
 */
static BOOL WINAPI X11DRV_wglSwapIntervalEXT(int interval) {
    BOOL ret = TRUE;

    TRACE("(%d)\n", interval);

    if (interval < 0)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    else if (!has_swap_control && interval == 0)
    {
        /* wglSwapIntervalEXT considers an interval value of zero to mean that
         * vsync should be disabled, but glXSwapIntervalSGI considers such a
         * value to be an error. Just silently ignore the request for now. */
        WARN("Request to disable vertical sync is not handled\n");
        swap_interval = 0;
    }
    else
    {
        if (pglXSwapIntervalSGI)
        {
            wine_tsx11_lock();
            ret = !pglXSwapIntervalSGI(interval);
            wine_tsx11_unlock();
        }
        else
            WARN("GLX_SGI_swap_control extension is not available\n");

        if (ret)
            swap_interval = interval;
        else
            SetLastError(ERROR_DC_NOT_FOUND);
    }

    return ret;
}

/**
 * X11DRV_wglAllocateMemoryNV
 *
 * WGL_NV_vertex_array_range: wglAllocateMemoryNV
 */
static void* WINAPI X11DRV_wglAllocateMemoryNV(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority) {
    void *ret = NULL;
    TRACE("(%d, %f, %f, %f)\n", size, readfreq, writefreq, priority );

    if (pglXAllocateMemoryNV)
    {
        wine_tsx11_lock();
        ret = pglXAllocateMemoryNV(size, readfreq, writefreq, priority);
        wine_tsx11_unlock();
    }
    return ret;
}

/**
 * X11DRV_wglFreeMemoryNV
 *
 * WGL_NV_vertex_array_range: wglFreeMemoryNV
 */
static void WINAPI X11DRV_wglFreeMemoryNV(GLvoid* pointer) {
    TRACE("(%p)\n", pointer);
    if (pglXFreeMemoryNV == NULL)
        return;

    wine_tsx11_lock();
    pglXFreeMemoryNV(pointer);
    wine_tsx11_unlock();
}

/**
 * X11DRV_wglSetPixelFormatWINE
 *
 * WGL_WINE_pixel_format_passthrough: wglSetPixelFormatWINE
 * This is a WINE-specific wglSetPixelFormat which can set the pixel format multiple times.
 */
static BOOL WINAPI X11DRV_wglSetPixelFormatWINE(HDC hdc, int format)
{
    WineGLPixelFormat *fmt;
    int value;
    HWND hwnd;

    TRACE("(%p,%d)\n", hdc, format);

    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, format, FALSE /* Offscreen */, &value);
    if (!fmt)
    {
        ERR( "Invalid format %d\n", format );
        return FALSE;
    }

    hwnd = WindowFromDC( hdc );
    if (!hwnd || hwnd == GetDesktopWindow())
    {
        ERR( "not a valid window DC %p\n", hdc );
        return FALSE;
    }

    wine_tsx11_lock();
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &value);
    wine_tsx11_unlock();

    if (!(value & GLX_WINDOW_BIT))
    {
        WARN( "Pixel format %d is not compatible for window rendering\n", format );
        return FALSE;
    }

    return SendMessageW(hwnd, WM_X11DRV_SET_WIN_FORMAT, fmt->fmt_id, 0);
    /* DC pixel format will be set by the DCE update */
}

/**
 * glxRequireVersion (internal)
 *
 * Check if the supported GLX version matches requiredVersion.
 */
static BOOL glxRequireVersion(int requiredVersion)
{
    /* Both requiredVersion and glXVersion[1] contains the minor GLX version */
    if(requiredVersion <= WineGLInfo.glxVersion[1])
        return TRUE;

    return FALSE;
}

static BOOL glxRequireExtension(const char *requiredExtension)
{
    if (strstr(WineGLInfo.glxExtensions, requiredExtension) == NULL) {
        return FALSE;
    }

    return TRUE;
}

static void register_extension_string(const char *ext)
{
    if (WineGLInfo.wglExtensions[0])
        strcat(WineGLInfo.wglExtensions, " ");
    strcat(WineGLInfo.wglExtensions, ext);

    TRACE("'%s'\n", ext);
}

static BOOL register_extension(const WineGLExtension * ext)
{
    int i;

    assert( WineGLExtensionListSize < MAX_EXTENSIONS );
    WineGLExtensionList[WineGLExtensionListSize++] = ext;

    register_extension_string(ext->extName);

    for (i = 0; ext->extEntryPoints[i].funcName; ++i)
        TRACE("    - '%s'\n", ext->extEntryPoints[i].funcName);

    return TRUE;
}

static const WineGLExtension WGL_internal_functions =
{
  "",
  {
    { "wglFinish", X11DRV_wglFinish },
    { "wglFlush", X11DRV_wglFlush },
    { "wglGetIntegerv", X11DRV_wglGetIntegerv },
  }
};


static const WineGLExtension WGL_ARB_create_context_extension =
{
  "WGL_ARB_create_context",
  {
    { "wglCreateContextAttribsARB", (void *)1 /* called through the glxdrv_wgl_funcs driver */ },
  }
};

static const WineGLExtension WGL_ARB_extensions_string_extension =
{
  "WGL_ARB_extensions_string",
  {
    { "wglGetExtensionsStringARB", X11DRV_wglGetExtensionsStringARB },
  }
};

static const WineGLExtension WGL_ARB_make_current_read_extension =
{
  "WGL_ARB_make_current_read",
  {
    { "wglGetCurrentReadDCARB", X11DRV_wglGetCurrentReadDCARB },
    { "wglMakeContextCurrentARB", (void *)1 /* called through the glxdrv_wgl_funcs driver */ },
  }
};

static const WineGLExtension WGL_ARB_multisample_extension =
{
  "WGL_ARB_multisample",
};

static const WineGLExtension WGL_ARB_pbuffer_extension =
{
  "WGL_ARB_pbuffer",
  {
    { "wglCreatePbufferARB", X11DRV_wglCreatePbufferARB },
    { "wglDestroyPbufferARB", X11DRV_wglDestroyPbufferARB },
    { "wglGetPbufferDCARB", X11DRV_wglGetPbufferDCARB },
    { "wglQueryPbufferARB", X11DRV_wglQueryPbufferARB },
    { "wglReleasePbufferDCARB", X11DRV_wglReleasePbufferDCARB },
    { "wglSetPbufferAttribARB", X11DRV_wglSetPbufferAttribARB },
  }
};

static const WineGLExtension WGL_ARB_pixel_format_extension =
{
  "WGL_ARB_pixel_format",
  {
    { "wglChoosePixelFormatARB", X11DRV_wglChoosePixelFormatARB },
    { "wglGetPixelFormatAttribfvARB", X11DRV_wglGetPixelFormatAttribfvARB },
    { "wglGetPixelFormatAttribivARB", X11DRV_wglGetPixelFormatAttribivARB },
  }
};

static const WineGLExtension WGL_ARB_render_texture_extension =
{
  "WGL_ARB_render_texture",
  {
    { "wglBindTexImageARB", X11DRV_wglBindTexImageARB },
    { "wglReleaseTexImageARB", X11DRV_wglReleaseTexImageARB },
  }
};

static const WineGLExtension WGL_EXT_extensions_string_extension =
{
  "WGL_EXT_extensions_string",
  {
    { "wglGetExtensionsStringEXT", X11DRV_wglGetExtensionsStringEXT },
  }
};

static const WineGLExtension WGL_EXT_swap_control_extension =
{
  "WGL_EXT_swap_control",
  {
    { "wglSwapIntervalEXT", X11DRV_wglSwapIntervalEXT },
    { "wglGetSwapIntervalEXT", X11DRV_wglGetSwapIntervalEXT },
  }
};

static const WineGLExtension WGL_NV_vertex_array_range_extension =
{
  "WGL_NV_vertex_array_range",
  {
    { "wglAllocateMemoryNV", X11DRV_wglAllocateMemoryNV },
    { "wglFreeMemoryNV", X11DRV_wglFreeMemoryNV },
  }
};

static const WineGLExtension WGL_WINE_pixel_format_passthrough_extension =
{
  "WGL_WINE_pixel_format_passthrough",
  {
    { "wglSetPixelFormatWINE", X11DRV_wglSetPixelFormatWINE },
  }
};

/**
 * X11DRV_WineGL_LoadExtensions
 */
static void X11DRV_WineGL_LoadExtensions(void)
{
    WineGLInfo.wglExtensions[0] = 0;

    /* Load Wine internal functions */
    register_extension(&WGL_internal_functions);

    /* ARB Extensions */

    if(glxRequireExtension("GLX_ARB_create_context"))
    {
        register_extension(&WGL_ARB_create_context_extension);

        if(glxRequireExtension("GLX_ARB_create_context_profile"))
            register_extension_string("WGL_ARB_create_context_profile");
    }

    if(glxRequireExtension("GLX_ARB_fbconfig_float"))
    {
        register_extension_string("WGL_ARB_pixel_format_float");
        register_extension_string("WGL_ATI_pixel_format_float");
    }

    register_extension(&WGL_ARB_extensions_string_extension);

    if (glxRequireVersion(3))
        register_extension(&WGL_ARB_make_current_read_extension);

    if (glxRequireExtension("GLX_ARB_multisample"))
        register_extension(&WGL_ARB_multisample_extension);

    /* In general pbuffer functionality requires support in the X-server. The functionality is
     * available either when the GLX_SGIX_pbuffer is present or when the GLX server version is 1.3.
     */
    if ( glxRequireVersion(3) && glxRequireExtension("GLX_SGIX_pbuffer") )
        register_extension(&WGL_ARB_pbuffer_extension);

    register_extension(&WGL_ARB_pixel_format_extension);

    /* Support WGL_ARB_render_texture when there's support or pbuffer based emulation */
    if (glxRequireExtension("GLX_ARB_render_texture") ||
        (glxRequireVersion(3) && glxRequireExtension("GLX_SGIX_pbuffer") && use_render_texture_emulation))
    {
        register_extension(&WGL_ARB_render_texture_extension);

        /* The WGL version of GLX_NV_float_buffer requires render_texture */
        if(glxRequireExtension("GLX_NV_float_buffer"))
            register_extension_string("WGL_NV_float_buffer");

        /* Again there's no GLX equivalent for this extension, so depend on the required GL extension */
        if(strstr(WineGLInfo.glExtensions, "GL_NV_texture_rectangle") != NULL)
            register_extension_string("WGL_NV_texture_rectangle");
    }

    /* EXT Extensions */

    register_extension(&WGL_EXT_extensions_string_extension);

    /* Load this extension even when it isn't backed by a GLX extension because it is has been around for ages.
     * Games like Call of Duty and K.O.T.O.R. rely on it. Further our emulation is good enough. */
    register_extension(&WGL_EXT_swap_control_extension);

    if(glxRequireExtension("GLX_EXT_framebuffer_sRGB"))
        register_extension_string("WGL_EXT_framebuffer_sRGB");

    if(glxRequireExtension("GLX_EXT_fbconfig_packed_float"))
        register_extension_string("WGL_EXT_pixel_format_packed_float");

    if (glxRequireExtension("GLX_EXT_swap_control"))
        has_swap_control = TRUE;

    /* The OpenGL extension GL_NV_vertex_array_range adds wgl/glX functions which aren't exported as 'real' wgl/glX extensions. */
    if(strstr(WineGLInfo.glExtensions, "GL_NV_vertex_array_range") != NULL)
        register_extension(&WGL_NV_vertex_array_range_extension);

    /* WINE-specific WGL Extensions */

    /* In WineD3D we need the ability to set the pixel format more than once (e.g. after a device reset).
     * The default wglSetPixelFormat doesn't allow this, so add our own which allows it.
     */
    register_extension(&WGL_WINE_pixel_format_passthrough_extension);
}


BOOL destroy_glxpixmap(Display *display, XID glxpixmap)
{
    wine_tsx11_lock(); 
    pglXDestroyGLXPixmap(display, glxpixmap);
    wine_tsx11_unlock(); 
    return TRUE;
}

/**
 * glxdrv_SwapBuffers
 *
 * Swap the buffers of this DC
 */
static BOOL glxdrv_SwapBuffers(PHYSDEV dev)
{
  struct glx_physdev *physdev = get_glxdrv_dev( dev );
  struct wgl_context *ctx = NtCurrentTeb()->glContext;

  TRACE("(%p)\n", dev->hdc);

  if (!ctx)
  {
      WARN("Using a NULL context, skipping\n");
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
  }

  if (!physdev->drawable)
  {
      WARN("Using an invalid drawable, skipping\n");
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
  }

  wine_tsx11_lock();
  sync_context(ctx);
  switch (physdev->type)
  {
  case DC_GL_PIXMAP_WIN:
      if(pglXCopySubBufferMESA) {
          int w = physdev->x11dev->dc_rect.right - physdev->x11dev->dc_rect.left;
          int h = physdev->x11dev->dc_rect.bottom - physdev->x11dev->dc_rect.top;

          /* (glX)SwapBuffers has an implicit glFlush effect, however
           * GLX_MESA_copy_sub_buffer doesn't. Make sure GL is flushed before
           * copying */
          pglFlush();
          if(w > 0 && h > 0)
              pglXCopySubBufferMESA(gdi_display, physdev->drawable, 0, 0, w, h);
          break;
      }
      /* fall through */
  default:
      pglXSwapBuffers(gdi_display, physdev->drawable);
      break;
  }

  flush_gl_drawable( physdev );
  wine_tsx11_unlock();

  /* FPS support */
  if (TRACE_ON(fps))
  {
      static long prev_time, start_time;
      static unsigned long frames, frames_total;

      DWORD time = GetTickCount();
      frames++;
      frames_total++;
      /* every 1.5 seconds */
      if (time - prev_time > 1500) {
          TRACE_(fps)("@ approx %.2ffps, total %.2ffps\n",
                      1000.0*frames/(time - prev_time), 1000.0*frames_total/(time - start_time));
          prev_time = time;
          frames = 0;
          if(start_time == 0) start_time = time;
      }
  }

  return TRUE;
}

XVisualInfo *visual_from_fbconfig_id( XID fbconfig_id )
{
    WineGLPixelFormat *fmt;
    XVisualInfo *ret;

    fmt = ConvertPixelFormatGLXtoWGL(gdi_display, fbconfig_id, 0 /* no flags */);
    if(fmt == NULL)
        return NULL;

    wine_tsx11_lock();
    ret = pglXGetVisualFromFBConfig(gdi_display, fmt->fbconfig);
    wine_tsx11_unlock();
    return ret;
}

static BOOL create_glx_dc( PHYSDEV *pdev )
{
    /* assume that only the main x11 device implements GetDeviceCaps */
    X11DRV_PDEVICE *x11dev = get_x11drv_dev( GET_NEXT_PHYSDEV( *pdev, pGetDeviceCaps ));
    struct glx_physdev *physdev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physdev) );

    if (!physdev) return FALSE;
    physdev->x11dev = x11dev;
    push_dc_driver( pdev, &physdev->dev, &glxdrv_funcs );
    return TRUE;
}

/**********************************************************************
 *	     glxdrv_CreateDC
 */
static BOOL glxdrv_CreateDC( PHYSDEV *pdev, LPCWSTR driver, LPCWSTR device,
                             LPCWSTR output, const DEVMODEW* initData )
{
    return create_glx_dc( pdev );
}

/**********************************************************************
 *	     glxdrv_CreateCompatibleDC
 */
static BOOL glxdrv_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    if (orig)  /* chain to next driver first */
    {
        orig = GET_NEXT_PHYSDEV( orig, pCreateCompatibleDC );
        if (!orig->funcs->pCreateCompatibleDC( orig, pdev )) return FALSE;
    }
    /* otherwise we have been called by x11drv */
    return create_glx_dc( pdev );
}

/**********************************************************************
 *	     glxdrv_DeleteDC
 */
static BOOL glxdrv_DeleteDC( PHYSDEV dev )
{
    struct glx_physdev *physdev = get_glxdrv_dev( dev );
    HeapFree( GetProcessHeap(), 0, physdev );
    return TRUE;
}

/**********************************************************************
 *           glxdrv_ExtEscape
 */
static INT glxdrv_ExtEscape( PHYSDEV dev, INT escape, INT in_count, LPCVOID in_data,
                             INT out_count, LPVOID out_data )
{
    struct glx_physdev *physdev = get_glxdrv_dev( dev );

    dev = GET_NEXT_PHYSDEV( dev, pExtEscape );

    if (escape == X11DRV_ESCAPE && in_data && in_count >= sizeof(enum x11drv_escape_codes))
    {
        switch (*(const enum x11drv_escape_codes *)in_data)
        {
        case X11DRV_SET_DRAWABLE:
            if (in_count >= sizeof(struct x11drv_escape_set_drawable))
            {
                const struct x11drv_escape_set_drawable *data = in_data;
                physdev->pixel_format = pixelformat_from_fbconfig_id( data->fbconfig_id );
                physdev->type         = data->gl_type;
                physdev->drawable     = data->gl_drawable;
                physdev->pixmap       = data->pixmap;
                TRACE( "SET_DRAWABLE hdc %p drawable %lx pf %u type %u\n",
                       dev->hdc, physdev->drawable, physdev->pixel_format, physdev->type );
            }
            break;
        case X11DRV_GET_DRAWABLE:
            if (out_count >= sizeof(struct x11drv_escape_get_drawable))
            {
                struct x11drv_escape_get_drawable *data = out_data;
                data->pixel_format = physdev->pixel_format;
                data->gl_type      = physdev->type;
                data->gl_drawable  = physdev->drawable;
                data->pixmap       = physdev->pixmap;
            }
            break;
        case X11DRV_FLUSH_GL_DRAWABLE:
            flush_gl_drawable( physdev );
            return TRUE;
        default:
            break;
        }
    }
    return dev->funcs->pExtEscape( dev, escape, in_count, in_data, out_count, out_data );
}

/**********************************************************************
 *           glxdrv_wine_get_wgl_driver
 */
static const struct wgl_funcs * glxdrv_wine_get_wgl_driver( PHYSDEV dev, UINT version )
{
    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but driver has %u\n", version, WINE_GDI_DRIVER_VERSION );
        return NULL;
    }

    if (has_opengl()) return &glxdrv_wgl_funcs;

    dev = GET_NEXT_PHYSDEV( dev, wine_get_wgl_driver );
    return dev->funcs->wine_get_wgl_driver( dev, version );
}

static const struct gdi_dc_funcs glxdrv_funcs =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    NULL,                               /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    NULL,                               /* pArc */
    NULL,                               /* pArcTo */
    NULL,                               /* pBeginPath */
    NULL,                               /* pBlendImage */
    NULL,                               /* pChord */
    NULL,                               /* pCloseFigure */
    glxdrv_CreateCompatibleDC,          /* pCreateCompatibleDC */
    glxdrv_CreateDC,                    /* pCreateDC */
    glxdrv_DeleteDC,                    /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    glxdrv_DescribePixelFormat,         /* pDescribePixelFormat */
    NULL,                               /* pDeviceCapabilities */
    NULL,                               /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    glxdrv_ExtEscape,                   /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    NULL,                               /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGdiRealizationInfo */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    NULL,                               /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    NULL,                               /* pGradientFill */
    NULL,                               /* pIntersectClipRect */
    NULL,                               /* pInvertRgn */
    NULL,                               /* pLineTo */
    NULL,                               /* pModifyWorldTransform */
    NULL,                               /* pMoveTo */
    NULL,                               /* pOffsetClipRgn */
    NULL,                               /* pOffsetViewportOrg */
    NULL,                               /* pOffsetWindowOrg */
    NULL,                               /* pPaintRgn */
    NULL,                               /* pPatBlt */
    NULL,                               /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    NULL,                               /* pPolyPolygon */
    NULL,                               /* pPolyPolyline */
    NULL,                               /* pPolygon */
    NULL,                               /* pPolyline */
    NULL,                               /* pPolylineTo */
    NULL,                               /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    NULL,                               /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    NULL,                               /* pRoundRect */
    NULL,                               /* pSaveDC */
    NULL,                               /* pScaleViewportExt */
    NULL,                               /* pScaleWindowExt */
    NULL,                               /* pSelectBitmap */
    NULL,                               /* pSelectBrush */
    NULL,                               /* pSelectClipPath */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPalette */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetArcDirection */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBkMode */
    NULL,                               /* pSetBoundsRect */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    NULL,                               /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    NULL,                               /* pSetPixel */
    glxdrv_SetPixelFormat,              /* pSetPixelFormat */
    NULL,                               /* pSetPolyFillMode */
    NULL,                               /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    NULL,                               /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    glxdrv_SwapBuffers,                 /* pSwapBuffers */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    glxdrv_wine_get_wgl_driver,         /* wine_get_wgl_driver */
    GDI_PRIORITY_GRAPHICS_DRV + 20      /* priority */
};

static const struct wgl_funcs glxdrv_wgl_funcs =
{
    glxdrv_GetPixelFormat,              /* p_GetPixelFormat */
    glxdrv_wglCopyContext,              /* p_wglCopyContext */
    glxdrv_wglCreateContext,            /* p_wglCreateContext */
    glxdrv_wglCreateContextAttribsARB,  /* p_wglCreateContextAttribsARB */
    glxdrv_wglDeleteContext,            /* p_wglDeleteContext */
    glxdrv_wglGetCurrentDC,             /* p_wglGetCurrentDC */
    glxdrv_wglGetProcAddress,           /* p_wglGetProcAddress */
    glxdrv_wglMakeContextCurrentARB,    /* p_wglMakeContextCurrentARB */
    glxdrv_wglMakeCurrent,              /* p_wglMakeCurrent */
    glxdrv_wglShareLists,               /* p_wglShareLists */
};

const struct gdi_dc_funcs *get_glx_driver(void)
{
    return &glxdrv_funcs;
}

#else  /* no OpenGL includes */

const struct gdi_dc_funcs *get_glx_driver(void)
{
    return NULL;
}

void mark_drawable_dirty(Drawable old, Drawable new)
{
}

Drawable create_glxpixmap(Display *display, XVisualInfo *vis, Pixmap parent)
{
    return 0;
}


BOOL destroy_glxpixmap(Display *display, XID glxpixmap)
{
    return FALSE;
}

XVisualInfo *visual_from_fbconfig_id( XID fbconfig_id )
{
    return NULL;
}

#endif /* defined(SONAME_LIBGL) */
