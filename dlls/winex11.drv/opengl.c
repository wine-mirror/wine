/*
 * X11DRV OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
 * Copyright 2005 Alex Woods
 * Copyright 2005 Raphael Junqueira
 * Copyright 2006 Roderick Colenbrander
 * Copyright 2006 Tomas Carnecky
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

#include "x11drv.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);
WINE_DECLARE_DEBUG_CHANNEL(opengl);

#if defined(HAVE_GL_GL_H) && defined(HAVE_GL_GLX_H)

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#endif
#ifdef HAVE_GL_GLX_H
# include <GL/glx.h>
#endif
#ifdef HAVE_GL_GLEXT_H
# include <GL/glext.h>
#endif

#include "wine/wgl.h"

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI


WINE_DECLARE_DEBUG_CHANNEL(fps);

typedef struct wine_glcontext {
    HDC hdc;
    Display *display;
    XVisualInfo *vis;
    GLXFBConfig fb_conf;
    GLXContext ctx;
    BOOL do_escape;
    struct wine_glcontext *next;
    struct wine_glcontext *prev;
} Wine_GLContext;

typedef struct wine_glpbuffer {
    Drawable   drawable;
    Display*   display;
    int        pixelFormat;
    int        width;
    int        height;
    int*       attribList;
    HDC        hdc;

    int        use_render_texture;
    GLuint     texture_target;
    GLuint     texture_bind_target;
    GLuint     texture;
    int        texture_level;
    HDC        prev_hdc;
    HGLRC      prev_ctx;
    HDC        render_hdc;
    HGLRC      render_ctx;
} Wine_GLPBuffer;

typedef struct wine_glextension {
    const char *extName;
    struct {
        const char *funcName;
        void *funcAddress;
    } extEntryPoints[8];
} WineGLExtension;

struct WineGLInfo {
    const char *glVersion;
    const char *glExtensions;

    int glxVersion[2];

    const char *glxServerVersion;
    const char *glxServerExtensions;

    const char *glxClientVersion;
    const char *glxClientExtensions;

    const char *glxExtensions;

    BOOL glxDirect;
    char wglExtensions[4096];
};

static Wine_GLContext *context_list;
static struct WineGLInfo WineGLInfo = { 0 };
static int use_render_texture_emulation = 0;
static int use_render_texture_ati = 0;
static int swap_interval = 1;

#define MAX_EXTENSIONS 16
static const WineGLExtension *WineGLExtensionList[MAX_EXTENSIONS];
static int WineGLExtensionListSize;

static void X11DRV_WineGL_LoadExtensions(void);

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

/* No need to load any other libraries as according to the ABI, libGL should be self-sufficient and
   include all dependencies
*/
#ifndef SONAME_LIBGL
#define SONAME_LIBGL "libGL.so"
#endif

#define PUSH1(attribs,att)        do { attribs[nAttribs++] = (att); } while (0)
#define PUSH2(attribs,att,value)  do { attribs[nAttribs++] = (att); attribs[nAttribs++] = (value); } while(0)

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
/* GLX 1.0 */
MAKE_FUNCPTR(glXChooseVisual)
MAKE_FUNCPTR(glXCreateContext)
MAKE_FUNCPTR(glXCreateGLXPixmap)
MAKE_FUNCPTR(glXGetCurrentContext)
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
MAKE_FUNCPTR(glXDestroyPbuffer)
MAKE_FUNCPTR(glXGetFBConfigAttrib)
MAKE_FUNCPTR(glXGetVisualFromFBConfig)
MAKE_FUNCPTR(glXMakeContextCurrent)
MAKE_FUNCPTR(glXQueryDrawable)
MAKE_FUNCPTR(glXGetCurrentReadDrawable)

/* GLX Extensions */
static void* (*pglXGetProcAddressARB)(const GLubyte *);
static BOOL  (*pglXBindTexImageARB)(Display *dpy, GLXPbuffer pbuffer, int buffer);
static BOOL  (*pglXReleaseTexImageARB)(Display *dpy, GLXPbuffer pbuffer, int buffer);
static BOOL  (*pglXDrawableAttribARB)(Display *dpy, GLXDrawable draw, const int *attribList);
static int   (*pglXSwapIntervalSGI)(int);

/* Standard OpenGL */
MAKE_FUNCPTR(glBindTexture)
MAKE_FUNCPTR(glCopyTexSubImage1D)
MAKE_FUNCPTR(glCopyTexSubImage2D)
MAKE_FUNCPTR(glDrawBuffer)
MAKE_FUNCPTR(glGetError)
MAKE_FUNCPTR(glGetIntegerv)
MAKE_FUNCPTR(glGetString)
#undef MAKE_FUNCPTR

BOOL X11DRV_WineGL_InitOpenglInfo()
{
    static BOOL infoInitialized = FALSE;

    int screen = DefaultScreen(gdi_display);
    Window win = RootWindow(gdi_display, screen);
    Visual *visual;
    XVisualInfo template;
    XVisualInfo *vis;
    int num;
    GLXContext ctx = NULL;

    if (infoInitialized)
        return TRUE;
    infoInitialized = TRUE;

    wine_tsx11_lock();

    visual = DefaultVisual(gdi_display, screen);
    template.visualid = XVisualIDFromVisual(visual);
    vis = XGetVisualInfo(gdi_display, VisualIDMask, &template, &num);
    if (vis) {
        /* Create a GLX Context. Without one we can't query GL information */
        ctx = pglXCreateContext(gdi_display, vis, None, GL_TRUE);
    }

    if (ctx) {
        pglXMakeCurrent(gdi_display, win, ctx);
    } else {
        ERR(" couldn't initialize OpenGL, expect problems\n");
        return FALSE;
    }

    WineGLInfo.glVersion = (const char *) pglGetString(GL_VERSION);
    WineGLInfo.glExtensions = (const char *) pglGetString(GL_EXTENSIONS);

    /* Get the common GLX version supported by GLX client and server ( major/minor) */
    pglXQueryVersion(gdi_display, &WineGLInfo.glxVersion[0], &WineGLInfo.glxVersion[1]);

    WineGLInfo.glxServerVersion = pglXQueryServerString(gdi_display, screen, GLX_VERSION);
    WineGLInfo.glxServerExtensions = pglXQueryServerString(gdi_display, screen, GLX_EXTENSIONS);

    WineGLInfo.glxClientVersion = pglXGetClientString(gdi_display, GLX_VERSION);
    WineGLInfo.glxClientExtensions = pglXGetClientString(gdi_display, GLX_EXTENSIONS);

    WineGLInfo.glxExtensions = pglXQueryExtensionsString(gdi_display, screen);
    WineGLInfo.glxDirect = pglXIsDirect(gdi_display, ctx);

    TRACE("GL version             : %s.\n", WineGLInfo.glVersion);
    TRACE("GLX version            : %d.%d.\n", WineGLInfo.glxVersion[0], WineGLInfo.glxVersion[1]);
    TRACE("Server GLX version     : %s.\n", WineGLInfo.glxServerVersion);
    TRACE("Client GLX version     : %s.\n", WineGLInfo.glxClientVersion);
    TRACE("Direct rendering enabled: %s\n", WineGLInfo.glxDirect ? "True" : "False");

    wine_tsx11_unlock();
    if(vis) XFree(vis);
    if(ctx) pglXDestroyContext(gdi_display, ctx);
    return TRUE;
}

static BOOL has_opengl(void)
{
    static int init_done;
    static void *opengl_handle;
    const char *glx_extensions;

    int error_base, event_base;

    if (init_done) return (opengl_handle != NULL);
    init_done = 1;

    opengl_handle = wine_dlopen(SONAME_LIBGL, RTLD_NOW|RTLD_GLOBAL, NULL, 0);
    if (opengl_handle == NULL) return FALSE;

    pglXGetProcAddressARB = wine_dlsym(opengl_handle, "glXGetProcAddressARB", NULL, 0);
    if (pglXGetProcAddressARB == NULL) {
        ERR("could not find glXGetProcAddressARB in libGL.\n");
        return FALSE;
    }

#define LOAD_FUNCPTR(f) if((p##f = (void*)pglXGetProcAddressARB((unsigned char*)#f)) == NULL) goto sym_not_found;
/* GLX 1.0 */
LOAD_FUNCPTR(glXChooseVisual)
LOAD_FUNCPTR(glXCreateContext)
LOAD_FUNCPTR(glXCreateGLXPixmap)
LOAD_FUNCPTR(glXGetCurrentContext)
LOAD_FUNCPTR(glXDestroyContext)
LOAD_FUNCPTR(glXDestroyGLXPixmap)
LOAD_FUNCPTR(glXGetConfig)
LOAD_FUNCPTR(glXIsDirect)
LOAD_FUNCPTR(glXMakeCurrent)
LOAD_FUNCPTR(glXSwapBuffers)
LOAD_FUNCPTR(glXQueryExtension)
LOAD_FUNCPTR(glXQueryVersion)

/* GLX 1.1 */
LOAD_FUNCPTR(glXGetClientString)
LOAD_FUNCPTR(glXQueryExtensionsString)
LOAD_FUNCPTR(glXQueryServerString)

/* GLX 1.3 */
LOAD_FUNCPTR(glXCreatePbuffer)
LOAD_FUNCPTR(glXDestroyPbuffer)
LOAD_FUNCPTR(glXMakeContextCurrent)
LOAD_FUNCPTR(glXGetCurrentReadDrawable)
LOAD_FUNCPTR(glXGetFBConfigs)

/* Standard OpenGL calls */
LOAD_FUNCPTR(glBindTexture)
LOAD_FUNCPTR(glCopyTexSubImage1D)
LOAD_FUNCPTR(glCopyTexSubImage2D)
LOAD_FUNCPTR(glDrawBuffer)
LOAD_FUNCPTR(glGetError)
LOAD_FUNCPTR(glGetIntegerv)
LOAD_FUNCPTR(glGetString)
#undef LOAD_FUNCPTR

    if(!X11DRV_WineGL_InitOpenglInfo()) {
        ERR("Intialization of OpenGL info failed, disabling OpenGL!\n");
        wine_dlclose(opengl_handle, NULL, 0);
        opengl_handle = NULL;
        return FALSE;
    }

    wine_tsx11_lock();
    if (pglXQueryExtension(gdi_display, &error_base, &event_base) == True) {
        TRACE("GLX is up and running error_base = %d\n", error_base);
    } else {
        wine_dlclose(opengl_handle, NULL, 0);
        opengl_handle = NULL;
    }

    /* In case of GLX you have direct and indirect rendering. Most of the time direct rendering is used
     * as in general only that is hardware accelerated. In some cases like in case of remote X indirect
     * rendering is used.
     *
     * The main problem for our OpenGL code is that we need certain GLX calls but their presence
     * depends on the reported GLX client / server version and on the client / server extension list.
     * Those don't have to be the same.
     *
     * In general the server GLX information should be used in case of indirect rendering. When direct
     * rendering is used, the OpenGL client library is responsible for which GLX calls are available.
     * Nvidia's OpenGL drivers are the best in terms of GLX features. At the moment of writing their
     * 8762 drivers support 1.3 for the server and 1.4 for the client and they support lots of extensions.
     * Unfortunately it is much more complicated for Mesa/DRI-based drivers and ATI's drivers.
     * Both sets of drivers report a server version of 1.2 and the client version can be 1.3 or 1.4.
     * Further in case of atleast ATI's drivers one crucial extension needed for our pixel format code
     * is only available in the list of server extensions and not in the client list.
     *
     * The versioning checks below try to take into account the comments from above.
     */

    /* Depending on the use of direct or indirect rendering we need either the list of extensions
     * exported by the client or by the server.
     */
    if(WineGLInfo.glxDirect)
        glx_extensions = WineGLInfo.glxClientExtensions;
    else
        glx_extensions = WineGLInfo.glxServerExtensions;

    /* Based on the default opengl context we decide whether direct or indirect rendering is used.
     * In case of indirect rendering we check if the GLX version of the server is 1.2 and else
     * the client version is checked.
     */
    if ((!WineGLInfo.glxDirect && !strcmp("1.2", WineGLInfo.glxServerVersion)) ||
        (WineGLInfo.glxDirect && !strcmp("1.2", WineGLInfo.glxClientVersion)))
    {
        pglXChooseFBConfig = (void*)pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfig");
        pglXGetFBConfigAttrib = (void*)pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttrib");
        pglXGetVisualFromFBConfig = (void*)pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfig");
    } else {
        if (NULL != strstr(glx_extensions, "GLX_SGIX_fbconfig")) {
            pglXChooseFBConfig = (void*)pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfigSGIX");
            pglXGetFBConfigAttrib = (void*)pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttribSGIX");
            pglXGetVisualFromFBConfig = (void*)pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfigSGIX");
        } else {
            ERR(" glx_version as %s and GLX_SGIX_fbconfig extension is unsupported. Expect problems.\n", WineGLInfo.glxClientVersion);
        }
    }

    /* The mesa libGL client library seems to forward glXQueryDrawable to the Xserver, so only
     * enable this function when the Xserver understand GLX 1.3 or newer
     */
    if (!strcmp("1.2", WineGLInfo.glxServerVersion))
        pglXQueryDrawable = NULL;
    else
        pglXQueryDrawable = wine_dlsym(RTLD_DEFAULT, "glXQueryDrawable", NULL, 0);

    if (NULL != strstr(glx_extensions, "GLX_ATI_render_texture")) {
        pglXBindTexImageARB = (void*)pglXGetProcAddressARB((const GLubyte *) "glXBindTexImageARB");
        pglXReleaseTexImageARB = (void*)pglXGetProcAddressARB((const GLubyte *) "glXReleaseTexImageARB");
        pglXDrawableAttribARB = (void*)pglXGetProcAddressARB((const GLubyte *) "glXDrawableAttribARB");
    }

    X11DRV_WineGL_LoadExtensions();

    wine_tsx11_unlock();
    return (opengl_handle != NULL);

sym_not_found:
    wine_dlclose(opengl_handle, NULL, 0);
    opengl_handle = NULL;
    return FALSE;
}

static inline Wine_GLContext *alloc_context(void)
{
    Wine_GLContext *ret;

    if ((ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Wine_GLContext))))
    {
        ret->next = context_list;
        if (context_list) context_list->prev = ret;
        context_list = ret;
    }
    return ret;
}

static inline void free_context(Wine_GLContext *context)
{
    if (context->next != NULL) context->next->prev = context->prev;
    if (context->prev != NULL) context->prev->next = context->next;
    else context_list = context->next;

    HeapFree(GetProcessHeap(), 0, context);
}

static inline Wine_GLContext *get_context_from_GLXContext(GLXContext ctx)
{
    Wine_GLContext *ret;
    if (!ctx) return NULL;
    for (ret = context_list; ret; ret = ret->next) if (ctx == ret->ctx) break;
    return ret;
}

/* retrieve the GLX drawable to use on a given DC */
inline static Drawable get_drawable( HDC hdc )
{
    GLXDrawable drawable;
    enum x11drv_escape_codes escape = X11DRV_GET_GLX_DRAWABLE;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(drawable), (LPSTR)&drawable )) drawable = 0;
    return drawable;
}

inline static void set_drawable( HDC hdc, Drawable drawable )
{
    struct x11drv_escape_set_drawable escape;

    escape.code = X11DRV_SET_DRAWABLE;
    escape.drawable = drawable;
    escape.mode = IncludeInferiors;
    escape.org.x = escape.org.y = 0;
    escape.drawable_org.x = escape.drawable_org.y = 0;

    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape, 0, NULL );
}

/** for use of wglGetCurrentReadDCARB */
inline static HDC get_hdc_from_Drawable(GLXDrawable d)
{
    Wine_GLContext *ret;
    for (ret = context_list; ret; ret = ret->next) {
        if (d == get_drawable( ret->hdc )) {
            return ret->hdc;
        }
    }
    return NULL;
}

inline static BOOL is_valid_context( Wine_GLContext *ctx )
{
    Wine_GLContext *ptr;
    for (ptr = context_list; ptr; ptr = ptr->next) if (ptr == ctx) break;
    return (ptr != NULL);
}

static int describeContext(Wine_GLContext* ctx) {
    int tmp;
    int ctx_vis_id;
    TRACE(" Context %p have (vis:%p):\n", ctx, ctx->vis);
    pglXGetFBConfigAttrib(ctx->display, ctx->fb_conf, GLX_FBCONFIG_ID, &tmp);
    TRACE(" - FBCONFIG_ID 0x%x\n", tmp);
    pglXGetFBConfigAttrib(ctx->display, ctx->fb_conf, GLX_VISUAL_ID, &tmp);
    TRACE(" - VISUAL_ID 0x%x\n", tmp);
    ctx_vis_id = tmp;
    return ctx_vis_id;
}

static int describeDrawable(Wine_GLContext* ctx, Drawable drawable) {
    int tmp;
    int nElements;
    int attribList[3] = { GLX_FBCONFIG_ID, 0, None };
    GLXFBConfig *fbCfgs;

    if (pglXQueryDrawable == NULL)  {
        /** glXQueryDrawable not available so returns not supported */
        return -1;
    }

    TRACE(" Drawable %p have :\n", (void*) drawable);
    pglXQueryDrawable(ctx->display, drawable, GLX_WIDTH, (unsigned int*) &tmp);
    TRACE(" - WIDTH as %d\n", tmp);
    pglXQueryDrawable(ctx->display, drawable, GLX_HEIGHT, (unsigned int*) &tmp);
    TRACE(" - HEIGHT as %d\n", tmp);
    pglXQueryDrawable(ctx->display, drawable, GLX_FBCONFIG_ID, (unsigned int*) &tmp);
    TRACE(" - FBCONFIG_ID as 0x%x\n", tmp);

    attribList[1] = tmp;
    fbCfgs = pglXChooseFBConfig(ctx->display, DefaultScreen(ctx->display), attribList, &nElements);
    if (fbCfgs == NULL) {
        return -1;
    }

    pglXGetFBConfigAttrib(ctx->display, fbCfgs[0], GLX_VISUAL_ID, &tmp);
    TRACE(" - VISUAL_ID as 0x%x\n", tmp);

    XFree(fbCfgs);

    return tmp;
}

static int ConvertAttribWGLtoGLX(const int* iWGLAttr, int* oGLXAttr, Wine_GLPBuffer* pbuf) {
  int nAttribs = 0;
  unsigned cur = 0; 
  int pop;
  int isColor = 0;
  int wantColorBits = 0;
  int sz_alpha = 0;

  while (0 != iWGLAttr[cur]) {
    TRACE("pAttr[%d] = %x\n", cur, iWGLAttr[cur]);

    switch (iWGLAttr[cur]) {
    case WGL_COLOR_BITS_ARB:
      pop = iWGLAttr[++cur];
      wantColorBits = pop; /** see end */
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
      sz_alpha = pop;
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

    case WGL_PIXEL_TYPE_ARB:
      pop = iWGLAttr[++cur];
      switch (pop) {
      case WGL_TYPE_COLORINDEX_ARB: pop = GLX_COLOR_INDEX_BIT; isColor = 1; break ;
      case WGL_TYPE_RGBA_ARB: pop = GLX_RGBA_BIT; break ;
      case WGL_TYPE_RGBA_FLOAT_ATI: pop = GLX_RGBA_FLOAT_ATI_BIT; break ;
      default:
        ERR("unexpected PixelType(%x)\n", pop);	
        pop = 0;
      }
      PUSH2(oGLXAttr, GLX_RENDER_TYPE, pop);
      TRACE("pAttr[%d] = GLX_RENDER_TYPE: %d\n", cur, pop);
      break;

    case WGL_SUPPORT_GDI_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_X_RENDERABLE, pop);
      TRACE("pAttr[%d] = GLX_RENDERABLE: %d\n", cur, pop);
      break;

    case WGL_DRAW_TO_BITMAP_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_X_RENDERABLE, pop);
      TRACE("pAttr[%d] = GLX_RENDERABLE: %d\n", cur, pop);
      if (pop) {
        PUSH2(oGLXAttr, GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT);
        TRACE("pAttr[%d] = GLX_DRAWABLE_TYPE: GLX_PIXMAP_BIT\n", cur);
      }
      break;

    case WGL_DRAW_TO_WINDOW_ARB:
      pop = iWGLAttr[++cur];
      if (pop) {
        PUSH2(oGLXAttr, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT);
        TRACE("pAttr[%d] = GLX_DRAWABLE_TYPE: GLX_WINDOW_BIT\n", cur);
      }
      break;

    case WGL_DRAW_TO_PBUFFER_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_X_RENDERABLE, pop);
      TRACE("pAttr[%d] = GLX_RENDERABLE: %d\n", cur, pop);
      if (pop) {
        PUSH2(oGLXAttr, GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT);
        TRACE("pAttr[%d] = GLX_DRAWABLE_TYPE: GLX_PBUFFER_BIT\n", cur);
      }
      break;

    case WGL_ACCELERATION_ARB:
    case WGL_SUPPORT_OPENGL_ARB:
      pop = iWGLAttr[++cur];
      /** nothing to do, if we are here, supposing support Accelerated OpenGL */
      TRACE("pAttr[%d] = WGL_SUPPORT_OPENGL_ARB: %d\n", cur, pop);
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
      if (use_render_texture_ati) {
        /** nothing to do here */
      }
      else if (!use_render_texture_emulation) {
        if (WGL_NO_TEXTURE_ARB != pop) {
          ERR("trying to use WGL_render_texture Attributes without support (was %x)\n", iWGLAttr[cur]);
          return -1; /** error: don't support it */
        } else {
          PUSH2(oGLXAttr, GLX_X_RENDERABLE, pop);
          PUSH2(oGLXAttr, GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT);
        }
      }
      break ;

    case WGL_BIND_TO_TEXTURE_RGB_ARB:
    case WGL_BIND_TO_TEXTURE_RGBA_ARB:
      pop = iWGLAttr[++cur];
      /** cannot be converted, see direct handling on 
       *   - wglGetPixelFormatAttribivARB
       *  TODO: wglChoosePixelFormat
       */
      break ;

    default:
      FIXME("unsupported %x WGL Attribute\n", iWGLAttr[cur]);
      break;
    }
    ++cur;
  }

  /**
   * Trick as WGL_COLOR_BITS_ARB != GLX_BUFFER_SIZE
   *    WGL_COLOR_BITS_ARB + WGL_ALPHA_BITS_ARB == GLX_BUFFER_SIZE
   *
   *  WGL_COLOR_BITS_ARB
   *     The number of color bitplanes in each color buffer. For RGBA
   *     pixel types, it is the size of the color buffer, excluding the
   *     alpha bitplanes. For color-index pixels, it is the size of the
   *     color index buffer.
   *
   *  GLX_BUFFER_SIZE   
   *     This attribute defines the number of bits per color buffer. 
   *     For GLX FBConfigs that correspond to a PseudoColor or StaticColor visual, 
   *     this is equal to the depth value reported in the X11 visual. 
   *     For GLX FBConfigs that correspond to TrueColor or DirectColor visual, 
   *     this is the sum of GLX_RED_SIZE, GLX_GREEN_SIZE, GLX_BLUE_SIZE, and GLX_ALPHA_SIZE.
   * 
   */
  if (0 < wantColorBits) {
    if (!isColor) { 
      wantColorBits += sz_alpha; 
    }
    if (32 < wantColorBits) {
      ERR("buggy %d GLX_BUFFER_SIZE default to 32\n", wantColorBits);
      wantColorBits = 32;
    }
    PUSH2(oGLXAttr, GLX_BUFFER_SIZE, wantColorBits);
    TRACE("pAttr[%d] = WGL_COLOR_BITS_ARB: %d\n", cur, wantColorBits);
  }

  return nAttribs;
}

/* GLX can advertise dozens of different pixelformats including offscreen and onscreen ones.
 * In our WGL implementation we only support a subset of these formats namely the format of
 * Wine's main visual and offscreen formats (if they are available).
 * This function converts a WGL format to its corresponding GLX one. It returns the index (zero-based)
 * into the GLX FB config table and it returns the number of supported WGL formats in fmt_count.
 */
static BOOL ConvertPixelFormatWGLtoGLX(Display *display, int iPixelFormat, int *fmt_index, int *fmt_count)
{
  int res = FALSE;
  int i = 0;
  GLXFBConfig* cfgs = NULL;
  int nCfgs = 0;
  int tmp_fmt_id = 0;
  int tmp_vis_id = 0;
  int nFormats = 1; /* Start at 1 as we always have a main visual */
  VisualID visualid = 0;

  /* Request to look up the format of the main visual when iPixelFormat = 1 */
  if(iPixelFormat == 1) visualid = XVisualIDFromVisual(visual);

  /* As mentioned in various parts of the code only the format of the main visual can be used for onscreen rendering.
   * Next to this format there are also so called offscreen rendering formats (used for pbuffers) which can be supported
   * because they don't need a visual. Below we use glXGetFBConfigs instead of glXChooseFBConfig to enumerate the fb configurations
   * bas this call lists both types of formats instead of only onscreen ones. */
  cfgs = pglXGetFBConfigs(display, DefaultScreen(display), &nCfgs);
  if (NULL == cfgs || 0 == nCfgs) {
    ERR("glXChooseFBConfig returns NULL\n");
    if(cfgs != NULL) XFree(cfgs);
    return FALSE;
  }

  /* Find the requested offscreen format and count the number of offscreen formats */
  for(i=0; i<nCfgs; i++) {
    pglXGetFBConfigAttrib(display, cfgs[i], GLX_VISUAL_ID, &tmp_vis_id);
    pglXGetFBConfigAttrib(display, cfgs[i], GLX_FBCONFIG_ID, &tmp_fmt_id);

    /* We are looking up the GLX index of our main visual and have found it :) */
    if(iPixelFormat == 1 && visualid == tmp_vis_id) {
      *fmt_index = i;
      TRACE("Found FBCONFIG_ID 0x%x at index %d for VISUAL_ID 0x%x\n", tmp_fmt_id, *fmt_index, tmp_vis_id);
      res = TRUE;
    }
    /* We found an offscreen rendering format :) */
    else if(tmp_vis_id == 0) {
      nFormats++;
      TRACE("Checking offscreen format FBCONFIG_ID 0x%x at index %d\n", tmp_fmt_id, i);

      if(iPixelFormat == nFormats) {
        *fmt_index = i;
        TRACE("Found offscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", tmp_fmt_id, iPixelFormat, i);
        res = TRUE;
      }
    }
  }
  *fmt_count = nFormats;
  TRACE("Number of offscreen formats: %d; returning index: %d\n", *fmt_count, *fmt_index);

  if(cfgs != NULL) XFree(cfgs);

  if(res == FALSE && iPixelFormat == 1)
    ERR("Can't find a matching FBCONFIG_ID for VISUAL_ID 0x%lx!\n", visualid);

  return res;
}

/**
 * X11DRV_ChoosePixelFormat
 *
 * Equivalent of glXChooseVisual
 */
int X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev, 
			     const PIXELFORMATDESCRIPTOR *ppfd) {
  GLXFBConfig* cfgs = NULL;
  int ret = 0;
  int nCfgs = 0;
  int value = 0;
  int fmt_index = 0;

  if (!has_opengl()) {
    ERR("No libGL on this box - disabling OpenGL support !\n");
    return 0;
  }

  if (TRACE_ON(opengl)) {
    TRACE("(%p,%p)\n", physDev, ppfd);

    dump_PIXELFORMATDESCRIPTOR((const PIXELFORMATDESCRIPTOR *) ppfd);
  }

  wine_tsx11_lock(); 
  if(!visual) {
    ERR("Can't get an opengl visual!\n");
    goto choose_exit;
  }

  /* Get a list containing all supported FB configurations */
  cfgs = pglXChooseFBConfig(gdi_display, DefaultScreen(gdi_display), NULL, &nCfgs);
  if (NULL == cfgs || 0 == nCfgs) {
    ERR("glXChooseFBConfig returns NULL (glError: %d)\n", pglGetError());
    goto choose_exit;
  }

  /* In case an fbconfig was found, check if it matches to the requirements of the ppfd */
  if(!ConvertPixelFormatWGLtoGLX(gdi_display, 1 /* main visual */, &fmt_index, &value)) {
    ERR("Can't find a matching FBCONFIG_ID for VISUAL_ID 0x%lx!\n", visual->visualid);
  } else {
    int dwFlags = 0;
    int iPixelType = 0;
    int value = 0;

    /* Pixel type */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_RENDER_TYPE, &value);
    if (value & GLX_RGBA_BIT)
      iPixelType = PFD_TYPE_RGBA;
    else
      iPixelType = PFD_TYPE_COLORINDEX;

    if (ppfd->iPixelType != iPixelType) {
      goto choose_exit;
    }

    /* Doublebuffer */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_DOUBLEBUFFER, &value); if (value) dwFlags |= PFD_DOUBLEBUFFER;
    if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE)) {
      if ((ppfd->dwFlags & PFD_DOUBLEBUFFER) != (dwFlags & PFD_DOUBLEBUFFER)) {
        goto choose_exit;
      }
    }

    /* Stereo */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_STEREO, &value); if (value) dwFlags |= PFD_STEREO;
    if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE)) {
      if ((ppfd->dwFlags & PFD_STEREO) != (dwFlags & PFD_STEREO)) {
        goto choose_exit;
      }
    }

    /* Alpha bits */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_ALPHA_SIZE, &value);
    if (ppfd->iPixelType==PFD_TYPE_RGBA && ppfd->cAlphaBits && !value) {
      goto choose_exit;
    }

    /* Depth bits */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_DEPTH_SIZE, &value);
    if (ppfd->cDepthBits && !value) {
      goto choose_exit;
    }

    /* Stencil bits */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_STENCIL_SIZE, &value);
    if (ppfd->cStencilBits && !value) {
      goto choose_exit;
    }

    /* Aux buffers */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_AUX_BUFFERS, &value);
    if (ppfd->cAuxBuffers && !value) {
      goto choose_exit;
    }

    /* When we pass all the checks we have found a matching format :) */
    ret = 1;
    TRACE("Successfully found a matching mode, returning index: %d\n", ret);
  }

choose_exit:
  if(!ret)
    TRACE("No matching mode was found returning 0\n");

  if (NULL != cfgs) XFree(cfgs);
  wine_tsx11_unlock();
  return ret;
}

/**
 * X11DRV_DescribePixelFormat
 *
 * Get the pixel-format descriptor associated to the given id
 */
int X11DRV_DescribePixelFormat(X11DRV_PDEVICE *physDev,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd) {
  /*XVisualInfo *vis;*/
  int value;
  int rb,gb,bb,ab;

  GLXFBConfig* cfgs = NULL;
  GLXFBConfig cur;
  int nCfgs = 0;
  int ret = 0;
  int fmt_index = 0;

  if (!has_opengl()) {
    ERR("No libGL on this box - disabling OpenGL support !\n");
    return 0;
  }
  
  TRACE("(%p,%d,%d,%p)\n", physDev, iPixelFormat, nBytes, ppfd);

  wine_tsx11_lock();
  cfgs = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs);
  wine_tsx11_unlock();

  if (NULL == cfgs || 0 == nCfgs) {
    ERR("unexpected iPixelFormat(%d), returns NULL\n", iPixelFormat);
    return 0; /* unespected error */
  }

  /* This function always reports the total number of supported pixel formats.
   * At the moment we only support the pixel format corresponding to the main
   * visual which got created at x11drv initialization. More formats could be
   * supported if there was a way to recreate x11 windows in x11drv. 
   * Because we only support one format nCfgs needs to be set to 1.
   */
  nCfgs = 1;

  if (ppfd == NULL) {
    /* The application is only querying the number of visuals */
    wine_tsx11_lock();
    if (NULL != cfgs) XFree(cfgs);
    wine_tsx11_unlock();
    return nCfgs;
  }

  if (nBytes < sizeof(PIXELFORMATDESCRIPTOR)) {
    ERR("Wrong structure size !\n");
    /* Should set error */
    return 0;
  }

  if (nCfgs < iPixelFormat || 1 > iPixelFormat) {
    WARN("unexpected iPixelFormat(%d): not >=1 and <=nFormats(%d), returning NULL\n", iPixelFormat, nCfgs);
    return 0;
  }

  /* Retrieve the index in the FBConfig table corresponding to the visual ID from the main visual */
  if(!ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, &fmt_index, &value)) {
      ERR("Can't find a valid pixel format index from the main visual, expect problems!\n");
      return 0;
  }

  ret = nCfgs;
  cur = cfgs[fmt_index];

  memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
  ppfd->nVersion = 1;

  /* These flags are always the same... */
  ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
  /* Now the flags extracted from the Visual */

  wine_tsx11_lock();

  pglXGetFBConfigAttrib(gdi_display, cur, GLX_CONFIG_CAVEAT, &value);
  if(value == GLX_SLOW_CONFIG)
      ppfd->dwFlags |= PFD_GENERIC_ACCELERATED;

  pglXGetFBConfigAttrib(gdi_display, cur, GLX_DOUBLEBUFFER, &value); if (value) ppfd->dwFlags |= PFD_DOUBLEBUFFER;
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_STEREO, &value); if (value) ppfd->dwFlags |= PFD_STEREO;

  /* Pixel type */
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_RENDER_TYPE, &value);
  if (value & GLX_RGBA_BIT)
    ppfd->iPixelType = PFD_TYPE_RGBA;
  else
    ppfd->iPixelType = PFD_TYPE_COLORINDEX;

  /* Color bits */
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_BUFFER_SIZE, &value);
  ppfd->cColorBits = value;

  /* Red, green, blue and alpha bits / shifts */
  if (ppfd->iPixelType == PFD_TYPE_RGBA) {
    pglXGetFBConfigAttrib(gdi_display, cur, GLX_RED_SIZE, &rb);
    pglXGetFBConfigAttrib(gdi_display, cur, GLX_GREEN_SIZE, &gb);
    pglXGetFBConfigAttrib(gdi_display, cur, GLX_BLUE_SIZE, &bb);
    pglXGetFBConfigAttrib(gdi_display, cur, GLX_ALPHA_SIZE, &ab);

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
  /* Accums : to do ... */

  /* Depth bits */
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_DEPTH_SIZE, &value);
  ppfd->cDepthBits = value;

  /* stencil bits */
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_STENCIL_SIZE, &value);
  ppfd->cStencilBits = value;

  wine_tsx11_unlock();

  /* Aux : to do ... */

  ppfd->iLayerType = PFD_MAIN_PLANE;

  if (TRACE_ON(opengl)) {
    dump_PIXELFORMATDESCRIPTOR(ppfd);
  }

  wine_tsx11_lock();
  if (NULL != cfgs) XFree(cfgs);
  wine_tsx11_unlock();

  return ret;
}

/**
 * X11DRV_GetPixelFormat
 *
 * Get the pixel-format id used by this DC
 */
int X11DRV_GetPixelFormat(X11DRV_PDEVICE *physDev) {
  TRACE("(%p): returns %d\n", physDev, physDev->current_pf);

  return physDev->current_pf;
}

/**
 * X11DRV_SetPixelFormat
 *
 * Set the pixel-format id used by this DC
 */
BOOL X11DRV_SetPixelFormat(X11DRV_PDEVICE *physDev,
			   int iPixelFormat,
			   const PIXELFORMATDESCRIPTOR *ppfd) {
  TRACE("(%p,%d,%p)\n", physDev, iPixelFormat, ppfd);

  /* At the moment we only support the pixelformat corresponding to the main
   * x11drv visual which got created at x11drv initialization. More formats
   * can be supported if there was a way to recreate x11 windows in x11drv
   */
  if(iPixelFormat != 1) {
    TRACE("Invalid iPixelFormat: %d\n", iPixelFormat);
    return 0;
  }

  physDev->current_pf = iPixelFormat;

  if (TRACE_ON(opengl)) {
    int nCfgs_fmt = 0;
    GLXFBConfig* cfgs_fmt = NULL;
    GLXFBConfig cur_cfg;
    int value;
    int gl_test = 0;
    int fmt_index = 0;

    if(!ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, &fmt_index, &value)) {
      ERR("Can't find a valid pixel format index from the main visual, expect problems!\n");
      return TRUE; /* Return true because the SetPixelFormat stuff itself passed */
    }

    /*
     * How to test if hdc current drawable is compatible (visual/FBConfig) ?
     *
     * in case of root window created HDCs we crash here :(
     *
    Drawable drawable =  get_drawable( physDev->hdc );
    TRACE(" drawable (%p,%p) have :\n", drawable, root_window);
    pglXQueryDrawable(gdi_display, drawable, GLX_FBCONFIG_ID, (unsigned int*) &value);
    TRACE(" - FBCONFIG_ID as 0x%x\n", tmp);
    pglXQueryDrawable(gdi_display, drawable, GLX_VISUAL_ID, (unsigned int*) &value);
    TRACE(" - VISUAL_ID as 0x%x\n", tmp);
    pglXQueryDrawable(gdi_display, drawable, GLX_WIDTH, (unsigned int*) &value);
    TRACE(" - WIDTH as %d\n", tmp);
    pglXQueryDrawable(gdi_display, drawable, GLX_HEIGHT, (unsigned int*) &value);
    TRACE(" - HEIGHT as %d\n", tmp);
    */
    cfgs_fmt = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs_fmt);
    cur_cfg = cfgs_fmt[fmt_index];
    gl_test = pglXGetFBConfigAttrib(gdi_display, cur_cfg, GLX_FBCONFIG_ID, &value);
    if (gl_test) {
      ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
    } else {
      TRACE(" FBConfig have :\n");
      TRACE(" - FBCONFIG_ID   0x%x\n", value);
      pglXGetFBConfigAttrib(gdi_display, cur_cfg, GLX_VISUAL_ID, &value);
      TRACE(" - VISUAL_ID     0x%x\n", value);
      pglXGetFBConfigAttrib(gdi_display, cur_cfg, GLX_DRAWABLE_TYPE, &value);
      TRACE(" - DRAWABLE_TYPE 0x%x\n", value);
    }
    XFree(cfgs_fmt);
  }
  return TRUE;
}

/* OpenGL32 wglCreateContext */
HGLRC WINAPI X11DRV_wglCreateContext(HDC hdc)
{
    Wine_GLContext *ret;
    GLXFBConfig* cfgs_fmt = NULL;
    GLXFBConfig cur_cfg;
    int hdcPF = 1; /* We can only use the Wine's main visual which has an index of 1 */
    int tmp = 0;
    int fmt_index = 0;
    int nCfgs_fmt = 0;
    int value = 0;
    int gl_test = 0;

    TRACE("(%p)->(PF:%d)\n", hdc, hdcPF);

    /* First, get the visual in use by the X11DRV */
    if (!gdi_display) return 0;

    /* We can only render using the iPixelFormat (1) of Wine's Main visual, we need to get the correspondig GLX format.
    * If this fails something is very wrong on the system. */
    if(!ConvertPixelFormatWGLtoGLX(gdi_display, hdcPF, &tmp, &fmt_index)) {
        ERR("Cannot get FB Config for main iPixelFormat 1, expect problems!\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    cfgs_fmt = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs_fmt);
    if (NULL == cfgs_fmt || 0 == nCfgs_fmt) {
        ERR("Cannot get FB Configs, expect problems.\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    if (nCfgs_fmt < fmt_index) {
        ERR("(%p): unexpected pixelFormat(%d) > nFormats(%d), returns NULL\n", hdc, fmt_index, nCfgs_fmt);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    cur_cfg = cfgs_fmt[fmt_index];
    gl_test = pglXGetFBConfigAttrib(gdi_display, cur_cfg, GLX_FBCONFIG_ID, &value);
    if (gl_test) {
        ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }
    XFree(cfgs_fmt);

    /* The context will be allocated in the wglMakeCurrent call */
    wine_tsx11_lock();
    ret = alloc_context();
    wine_tsx11_unlock();
    ret->hdc = hdc;
    ret->display = gdi_display;
    ret->fb_conf = cur_cfg;
    /*ret->vis = vis;*/
    ret->vis = pglXGetVisualFromFBConfig(gdi_display, cur_cfg);

    TRACE(" creating context %p (GL context creation delayed)\n", ret);
    return (HGLRC) ret;
}

/* OpenGL32 wglDeleteContext */
BOOL WINAPI X11DRV_wglDeleteContext(HGLRC hglrc)
{
    Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
    BOOL ret = TRUE;

    TRACE("(%p)\n", hglrc);

    wine_tsx11_lock();
    /* A game (Half Life not to name it) deletes twice the same context,
    * so make sure it is valid first */
    if (is_valid_context( ctx ))
    {
        if (ctx->ctx) pglXDestroyContext(ctx->display, ctx->ctx);
        free_context(ctx);
    }
    else
    {
        WARN("Error deleting context !\n");
        SetLastError(ERROR_INVALID_HANDLE);
        ret = FALSE;
    }
    wine_tsx11_unlock();

    return ret;
}

/* OpenGL32 wglGetCurrentContext() */
HGLRC WINAPI X11DRV_wglGetCurrentContext(void) {
    GLXContext gl_ctx;
    Wine_GLContext *ret;

    TRACE("()\n");

    wine_tsx11_lock();
    gl_ctx = pglXGetCurrentContext();
    ret = get_context_from_GLXContext(gl_ctx);
    wine_tsx11_unlock();

    TRACE(" returning %p (GL context %p)\n", ret, gl_ctx);

    return (HGLRC)ret;
}

/* OpenGL32 wglGetCurrentDC */
HDC WINAPI X11DRV_wglGetCurrentDC(void) {
    GLXContext gl_ctx;
    Wine_GLContext *ret;

    TRACE("()\n");

    wine_tsx11_lock();
    gl_ctx = pglXGetCurrentContext();
    ret = get_context_from_GLXContext(gl_ctx);
    wine_tsx11_unlock();

    if (ret) {
        TRACE(" returning %p (GL context %p - Wine context %p)\n", ret->hdc, gl_ctx, ret);
        return ret->hdc;
    } else {
        TRACE(" no Wine context found for GLX context %p\n", gl_ctx);
        return 0;
    }
}

/* OpenGL32 wglGetCurrentReadDCARB */
HDC WINAPI X11DRV_wglGetCurrentReadDCARB(void) 
{
    GLXDrawable gl_d;
    HDC ret;

    TRACE("()\n");

    wine_tsx11_lock();
    gl_d = pglXGetCurrentReadDrawable();
    ret = get_hdc_from_Drawable(gl_d);
    wine_tsx11_unlock();

    TRACE(" returning %p (GL drawable %lu)\n", ret, gl_d);
    return ret;
}

/* OpenGL32: wglGetProcAddress */
PROC X11DRV_wglGetProcAddress(LPCSTR lpszProc)
{
    int i, j;
    const WineGLExtension *ext;

    int padding = 32 - strlen(lpszProc);
    if (padding < 0)
        padding = 0;

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

    ERR("(%s) - not found\n", lpszProc);

    return NULL;
}


/* OpenGL32 wglMakeCurrent */
BOOL WINAPI X11DRV_wglMakeCurrent(HDC hdc, HGLRC hglrc) {
    BOOL ret;
    DWORD type = GetObjectType(hdc);

    TRACE("(%p,%p)\n", hdc, hglrc);

    wine_tsx11_lock();
    if (hglrc == NULL) {
        ret = pglXMakeCurrent(gdi_display, None, NULL);
        NtCurrentTeb()->glContext = NULL;
    } else {
        Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
        Drawable drawable = get_drawable( hdc );
        if (ctx->ctx == NULL) {
            int draw_vis_id, ctx_vis_id;
            VisualID visualid = (VisualID)GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );
            TRACE(" Wine desktop VISUAL_ID is 0x%x\n", (unsigned int) visualid);
            draw_vis_id = describeDrawable(ctx, drawable);
            ctx_vis_id = describeContext(ctx);

            if (-1 == draw_vis_id || (draw_vis_id == visualid && draw_vis_id != ctx_vis_id)) {
                /**
                * Inherits from root window so reuse desktop visual
                */
                XVisualInfo template;
                XVisualInfo *vis;
                int num;
                template.visualid = visualid;
                vis = XGetVisualInfo(ctx->display, VisualIDMask, &template, &num);

                TRACE(" Creating GLX Context\n");
                ctx->ctx = pglXCreateContext(ctx->display, vis, NULL, type == OBJ_MEMDC ? False : True);
            } else {
                TRACE(" Creating GLX Context\n");
                ctx->ctx = pglXCreateContext(ctx->display, ctx->vis, NULL, type == OBJ_MEMDC ? False : True);
            }
            TRACE(" created a delayed OpenGL context (%p)\n", ctx->ctx);
        }
        TRACE(" make current for dis %p, drawable %p, ctx %p\n", ctx->display, (void*) drawable, ctx->ctx);
        ret = pglXMakeCurrent(ctx->display, drawable, ctx->ctx);
        NtCurrentTeb()->glContext = ctx;
        if(ret && type == OBJ_MEMDC)
        {
            ctx->do_escape = TRUE;
            pglDrawBuffer(GL_FRONT_LEFT);
        }
    }
    wine_tsx11_unlock();
    TRACE(" returning %s\n", (ret ? "True" : "False"));
    return ret;
}

/* OpenGL32 wglMakeContextCurrentARB */
BOOL WINAPI X11DRV_wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc) 
{
    BOOL ret;
    TRACE("(%p,%p,%p)\n", hDrawDC, hReadDC, hglrc);

    wine_tsx11_lock();
    if (hglrc == NULL) {
        ret = pglXMakeCurrent(gdi_display, None, NULL);
    } else {
        if (NULL == pglXMakeContextCurrent) {
            ret = FALSE;
        } else {
            Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
            Drawable d_draw = get_drawable( hDrawDC );
            Drawable d_read = get_drawable( hReadDC );

            if (ctx->ctx == NULL) {
                ctx->ctx = pglXCreateContext(ctx->display, ctx->vis, NULL, GetObjectType(hDrawDC) == OBJ_MEMDC ? False : True);
                TRACE(" created a delayed OpenGL context (%p)\n", ctx->ctx);
            }
            ret = pglXMakeContextCurrent(ctx->display, d_draw, d_read, ctx->ctx);
        }
    }
    wine_tsx11_unlock();

    TRACE(" returning %s\n", (ret ? "True" : "False"));
    return ret;
}

/* OpenGL32 wglShaderLists */
BOOL WINAPI X11DRV_wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
    Wine_GLContext *org  = (Wine_GLContext *) hglrc1;
    Wine_GLContext *dest = (Wine_GLContext *) hglrc2;

    TRACE("(%p, %p)\n", org, dest);

    if (NULL != dest && dest->ctx != NULL) {
        ERR("Could not share display lists, context already created !\n");
        return FALSE;
    } else {
        if (org->ctx == NULL) {
            wine_tsx11_lock();
            describeContext(org);
            org->ctx = pglXCreateContext(org->display, org->vis, NULL, GetObjectType(org->hdc) == OBJ_MEMDC ? False : True);
            wine_tsx11_unlock();
            TRACE(" created a delayed OpenGL context (%p) for Wine context %p\n", org->ctx, org);
        }
        if (NULL != dest) {
            wine_tsx11_lock();
            describeContext(dest);
            /* Create the destination context with display lists shared */
            dest->ctx = pglXCreateContext(org->display, dest->vis, org->ctx, GetObjectType(org->hdc) == OBJ_MEMDC ? False : True);
            wine_tsx11_unlock();
            TRACE(" created a delayed OpenGL context (%p) for Wine context %p sharing lists with OpenGL ctx %p\n", dest->ctx, dest, org->ctx);
            return TRUE;
        }
    }
    return FALSE;
}


/* WGL helper function which handles differences in glGetIntegerv from WGL and GLX */ 
void X11DRV_wglGetIntegerv(GLenum pname, GLint* params) {
    TRACE("pname: 0x%x, params: %p\n", pname, params);
    if (pname == GL_DEPTH_BITS) { 
        GLXContext gl_ctx = pglXGetCurrentContext();
        Wine_GLContext* ret = get_context_from_GLXContext(gl_ctx);
        /*TRACE("returns Wine Ctx as %p\n", ret);*/
        /** 
        * if we cannot find a Wine Context
        * we only have the default wine desktop context, 
        * so if we have only a 24 depth say we have 32
        */
        if (NULL == ret && 24 == *params) { 
            *params = 32;
        }
        TRACE("returns GL_DEPTH_BITS as '%d'\n", *params);
    }
    if (pname == GL_ALPHA_BITS) {
        GLint tmp;
        GLXContext gl_ctx = pglXGetCurrentContext();
        Wine_GLContext* ret = get_context_from_GLXContext(gl_ctx);
        pglXGetFBConfigAttrib(ret->display, ret->fb_conf, GLX_ALPHA_SIZE, &tmp);
        TRACE("returns GL_ALPHA_BITS as '%d'\n", tmp);
        *params = tmp;
    }
}

/* WGL_ARB_extensions_string: wglGetExtensionsStringARB */
static const char * WINAPI X11DRV_wglGetExtensionsStringARB(HDC hdc) {
    TRACE("() returning \"%s\"\n", WineGLInfo.wglExtensions);
    return WineGLInfo.wglExtensions;
}

/* WGL_ARB_pbuffer: wglCreatePbufferARB */
static HPBUFFERARB WINAPI X11DRV_wglCreatePbufferARB(HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList)
{
    Wine_GLPBuffer* object = NULL;
    GLXFBConfig* cfgs = NULL;
    int nCfgs = 0;
    int attribs[256];
    unsigned nAttribs = 0;
    int fmt_index = 0;

    TRACE("(%p, %d, %d, %d, %p)\n", hdc, iPixelFormat, iWidth, iHeight, piAttribList);

    if (0 >= iPixelFormat) {
        ERR("(%p): unexpected iPixelFormat(%d) <= 0, returns NULL\n", hdc, iPixelFormat);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL; /* unexpected error */
    }

    cfgs = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs);

    if (NULL == cfgs || 0 == nCfgs) {
        ERR("(%p): Cannot get FB Configs for iPixelFormat(%d), returns NULL\n", hdc, iPixelFormat);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL; /* unexpected error */
    }

    /* Convert the WGL pixelformat to a GLX format, if it fails then the format is invalid */
    if(!ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, &fmt_index, &nCfgs)) {
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

    nAttribs = ConvertAttribWGLtoGLX(piAttribList, attribs, object);
    if (-1 == nAttribs) {
        WARN("Cannot convert WGL to GLX attributes\n");
        goto create_failed;
    }
    PUSH2(attribs, GLX_PBUFFER_WIDTH,  iWidth);
    PUSH2(attribs, GLX_PBUFFER_HEIGHT, iHeight); 
    while (0 != *piAttribList) {
        int attr_v;
        switch (*piAttribList) {
            case WGL_TEXTURE_FORMAT_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_TEXTURE_FORMAT_ARB as %x\n", attr_v);
                if (use_render_texture_ati) {
                    int type = 0;
                    switch (attr_v) {
                        case WGL_NO_TEXTURE_ARB: type = GLX_NO_TEXTURE_ATI; break ;
                        case WGL_TEXTURE_RGB_ARB: type = GLX_TEXTURE_RGB_ATI; break ;
                        case WGL_TEXTURE_RGBA_ARB: type = GLX_TEXTURE_RGBA_ATI; break ;
                        default:
                            SetLastError(ERROR_INVALID_DATA);
                            goto create_failed;
                    }
                    object->use_render_texture = 1;
                    PUSH2(attribs, GLX_TEXTURE_FORMAT_ATI, type);
                } else {
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
                                break;
                            case WGL_TEXTURE_RGBA_ARB:
                                object->use_render_texture = GL_RGBA;
                                break;
                            default:
                                SetLastError(ERROR_INVALID_DATA);
                                goto create_failed;
                        }
                    }
                }
                break;
            }

            case WGL_TEXTURE_TARGET_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_TEXTURE_TARGET_ARB as %x\n", attr_v);
                if (use_render_texture_ati) {
                    int type = 0;
                    switch (attr_v) {
                        case WGL_NO_TEXTURE_ARB: type = GLX_NO_TEXTURE_ATI; break ;
                        case WGL_TEXTURE_CUBE_MAP_ARB: type = GLX_TEXTURE_CUBE_MAP_ATI; break ;
                        case WGL_TEXTURE_1D_ARB: type = GLX_TEXTURE_1D_ATI; break ;
                        case WGL_TEXTURE_2D_ARB: type = GLX_TEXTURE_2D_ATI; break ;
                        default:
                            SetLastError(ERROR_INVALID_DATA);
                            goto create_failed;
                    }
                    PUSH2(attribs, GLX_TEXTURE_TARGET_ATI, type);
                } else {
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
                                object->texture_bind_target = GL_TEXTURE_CUBE_MAP;
                               break;
                            }
                            case WGL_TEXTURE_1D_ARB: {
                                if (1 != iHeight) {
                                    SetLastError(ERROR_INVALID_DATA);
                                    goto create_failed;
                                }
                                object->texture_target = GL_TEXTURE_1D;
                                object->texture_bind_target = GL_TEXTURE_1D;
                                break;
                            }
                            case WGL_TEXTURE_2D_ARB: {
                                object->texture_target = GL_TEXTURE_2D;
                                object->texture_bind_target = GL_TEXTURE_2D;
                                break;
                            }
                            default:
                                SetLastError(ERROR_INVALID_DATA);
                                goto create_failed;
                        }
                    }
                }
                break;
            }

            case WGL_MIPMAP_TEXTURE_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_MIPMAP_TEXTURE_ARB as %x\n", attr_v);
                if (use_render_texture_ati) {
                    PUSH2(attribs, GLX_MIPMAP_TEXTURE_ATI, attr_v);
                } else {
                    if (!use_render_texture_emulation) {
                        SetLastError(ERROR_INVALID_DATA);
                        goto create_failed;
                    }
                }
                break;
            }
        }
        ++piAttribList;
    }

    PUSH1(attribs, None);
    object->drawable = pglXCreatePbuffer(gdi_display, cfgs[fmt_index], attribs);
    TRACE("new Pbuffer drawable as %p\n", (void*) object->drawable);
    if (!object->drawable) {
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        goto create_failed; /* unexpected error */
    }
    TRACE("->(%p)\n", object);

    /** free list */
    XFree(cfgs);
    return (HPBUFFERARB) object;

create_failed:
    if (NULL != cfgs) XFree(cfgs);
    HeapFree(GetProcessHeap(), 0, object);
    TRACE("->(FAILED)\n");
    return (HPBUFFERARB) NULL;
}

/* WGL_ARB_pbuffer: wglDestroyPbufferARB */
static GLboolean WINAPI X11DRV_wglDestroyPbufferARB(HPBUFFERARB hPbuffer)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
    TRACE("(%p)\n", hPbuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    pglXDestroyPbuffer(object->display, object->drawable);
    HeapFree(GetProcessHeap(), 0, object);
    return GL_TRUE;
}

/* WGL_ARB_pbuffer: wglGetPbufferDCARB */
static HDC WINAPI X11DRV_wglGetPbufferDCARB(HPBUFFERARB hPbuffer)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
    HDC hDC;
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    hDC = CreateCompatibleDC(object->hdc);

    /* The function wglGetPbufferDCARB returns a DC to which the pbuffer can be connected.
     * We only support one onscreen rendering format (the one from the main visual), so use that. */
    SetPixelFormat(hDC, 1, NULL);
    set_drawable(hDC, object->drawable); /* works ?? */
    TRACE("(%p)->(%p)\n", hPbuffer, hDC);
    return hDC;
}

/* WGL_ARB_pbuffer: wglQueryPbufferARB */
static GLboolean WINAPI X11DRV_wglQueryPbufferARB(HPBUFFERARB hPbuffer, int iAttribute, int *piValue)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
    TRACE("(%p, 0x%x, %p)\n", hPbuffer, iAttribute, piValue);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    switch (iAttribute) {
        case WGL_PBUFFER_WIDTH_ARB:
            pglXQueryDrawable(object->display, object->drawable, GLX_WIDTH, (unsigned int*) piValue);
            break;
        case WGL_PBUFFER_HEIGHT_ARB:
            pglXQueryDrawable(object->display, object->drawable, GLX_HEIGHT, (unsigned int*) piValue);
            break;

        case WGL_PBUFFER_LOST_ARB:
            FIXME("unsupported WGL_PBUFFER_LOST_ARB (need glXSelectEvent/GLX_DAMAGED work)\n");
            break;

        case WGL_TEXTURE_FORMAT_ARB:
            if (use_render_texture_ati) {
                unsigned int tmp;
                int type = WGL_NO_TEXTURE_ARB;
                pglXQueryDrawable(object->display, object->drawable, GLX_TEXTURE_FORMAT_ATI, &tmp);
                switch (tmp) {
                    case GLX_NO_TEXTURE_ATI: type = WGL_NO_TEXTURE_ARB; break ;
                    case GLX_TEXTURE_RGB_ATI: type = WGL_TEXTURE_RGB_ARB; break ;
                    case GLX_TEXTURE_RGBA_ATI: type = WGL_TEXTURE_RGBA_ARB; break ;
                }
                *piValue = type;
            } else {
                if (!object->use_render_texture) {
                    *piValue = WGL_NO_TEXTURE_ARB;
                } else {
                    if (!use_render_texture_emulation) {
                        SetLastError(ERROR_INVALID_HANDLE);
                        return GL_FALSE;
                    }
                    if (GL_RGBA == object->use_render_texture) {
                        *piValue = WGL_TEXTURE_RGBA_ARB;
                    } else {
                        *piValue = WGL_TEXTURE_RGB_ARB;
                    }
                }
            }
            break;

        case WGL_TEXTURE_TARGET_ARB:
            if (use_render_texture_ati) {
                unsigned int tmp;
                int type = WGL_NO_TEXTURE_ARB;
                pglXQueryDrawable(object->display, object->drawable, GLX_TEXTURE_TARGET_ATI, &tmp);
                switch (tmp) {
                    case GLX_NO_TEXTURE_ATI: type = WGL_NO_TEXTURE_ARB; break ;
                    case GLX_TEXTURE_CUBE_MAP_ATI: type = WGL_TEXTURE_CUBE_MAP_ARB; break ;
                    case GLX_TEXTURE_1D_ATI: type = WGL_TEXTURE_1D_ARB; break ;
                    case GLX_TEXTURE_2D_ATI: type = WGL_TEXTURE_2D_ARB; break ;
                }
                *piValue = type;
            } else {
            if (!object->texture_target) {
                *piValue = WGL_NO_TEXTURE_ARB;
            } else {
                if (!use_render_texture_emulation) {
                    SetLastError(ERROR_INVALID_DATA);      
                    return GL_FALSE;
                }
                switch (object->texture_target) {
                    case GL_TEXTURE_1D:       *piValue = WGL_TEXTURE_CUBE_MAP_ARB; break;
                    case GL_TEXTURE_2D:       *piValue = WGL_TEXTURE_1D_ARB; break;
                    case GL_TEXTURE_CUBE_MAP: *piValue = WGL_TEXTURE_2D_ARB; break;
                }
            }
        }
        break;

    case WGL_MIPMAP_TEXTURE_ARB:
        if (use_render_texture_ati) {
            pglXQueryDrawable(object->display, object->drawable, GLX_MIPMAP_TEXTURE_ATI, (unsigned int*) piValue);
        } else {
            *piValue = GL_FALSE; /** don't support that */
            FIXME("unsupported WGL_ARB_render_texture attribute query for 0x%x\n", iAttribute);
        }
        break;

    default:
        FIXME("unexpected attribute %x\n", iAttribute);
        break;
    }

    return GL_TRUE;
}

/* WGL_ARB_pbuffer: wglReleasePbufferDCARB */
static int WINAPI X11DRV_wglReleasePbufferDCARB(HPBUFFERARB hPbuffer, HDC hdc)
{
    TRACE("(%p, %p)\n", hPbuffer, hdc);
    DeleteDC(hdc);
    return 0;
}

/* WGL_ARB_pbuffer: wglSetPbufferAttribARB */
static GLboolean WINAPI X11DRV_wglSetPbufferAttribARB(HPBUFFERARB hPbuffer, const int *piAttribList)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
    WARN("(%p, %p): alpha-testing, report any problem\n", hPbuffer, piAttribList);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!use_render_texture_ati && 1 == use_render_texture_emulation) {
        return GL_TRUE;
    }
    if (NULL != pglXDrawableAttribARB) {
        if (use_render_texture_ati) {
            FIXME("Need conversion for GLX_ATI_render_texture\n");
        }
        return pglXDrawableAttribARB(object->display, object->drawable, piAttribList); 
    }
    return GL_FALSE;
}

/* WGL_ARB_pixel_format: wglChoosePixelFormatARB */
GLboolean WINAPI X11DRV_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)
{
    int gl_test = 0;
    int attribs[256];
    int nAttribs = 0;
    GLboolean res = FALSE;

    /* We need the visualid to check if the format is suitable */
    VisualID visualid = XVisualIDFromVisual(visual);

    GLXFBConfig* cfgs = NULL;
    int nCfgs = 0;
    UINT it;
    int fmt_id;

    GLXFBConfig* cfgs_fmt = NULL;
    int nCfgs_fmt = 0;
    UINT it_fmt;
    int tmp_fmt_id;
    int tmp_vis_id;

    int pfmt_it = 0;
    int offscreen_index = 1; /* Start at one because we allways have a main visual at iPixelFormat=1 */

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

    /* Search for FB configurations matching the requirements in attribs */
    cfgs = pglXChooseFBConfig(gdi_display, DefaultScreen(gdi_display), attribs, &nCfgs);
    if (NULL == cfgs) {
        WARN("Compatible Pixel Format not found\n");
        return GL_FALSE;
    }

    /* Get a list of all FB configurations */
    cfgs_fmt = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs_fmt);
    if (NULL == cfgs_fmt) {
        ERR("Failed to get All FB Configs\n");
        XFree(cfgs);
        return GL_FALSE;
    }

    /* Loop through all matching formats and check if they are suitable.
    * Note that this function should at max return nMaxFormats different formats */
    for (it = 0; pfmt_it < nMaxFormats && it < nCfgs; ++it) {
        gl_test = pglXGetFBConfigAttrib(gdi_display, cfgs[it], GLX_FBCONFIG_ID, &fmt_id);
        if (gl_test) {
            ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
            continue;
        }

        gl_test = pglXGetFBConfigAttrib(gdi_display, cfgs[it], GLX_VISUAL_ID, &tmp_vis_id);
        if (gl_test) {
            ERR("Failed to retrieve VISUAL_ID from GLXFBConfig, expect problems.\n");
            continue;
        }

        /* When the visualid of the GLXFBConfig matches the one of the main visual we have found our
        * only supported onscreen rendering format. This format has a WGL index of 1. */
        if(tmp_vis_id == visualid) {
            piFormats[pfmt_it] = 1;
            ++pfmt_it;
            res = GL_TRUE;
            TRACE("Found compatible GLXFBConfig 0x%x with WGL index 1\n", fmt_id);
            continue;
        }
        /* Only continue with this loop for offscreen rendering formats (visualid = 0) */
        else if(tmp_vis_id != 0) {
            TRACE("Discarded GLXFBConfig %0x with VisualID %x because the visualid is not the same as our main visual (%lx)\n", fmt_id, tmp_vis_id, visualid);
            continue;
        }

        /* Find the index of the found format in the whole format table */
        for (it_fmt = 0; it_fmt < nCfgs_fmt; ++it_fmt) {
            gl_test = pglXGetFBConfigAttrib(gdi_display, cfgs_fmt[it_fmt], GLX_FBCONFIG_ID, &tmp_fmt_id);
            if (gl_test) {
                ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
                continue;
            }
            gl_test = pglXGetFBConfigAttrib(gdi_display, cfgs[it], GLX_VISUAL_ID, &tmp_vis_id);
            if (gl_test) {
                ERR("Failed to retrieve VISUAL_ID from GLXFBConfig, expect problems.\n");
                continue;
            }
            /* The format of Wine's main visual is stored at index 1 of our WGL format table.
            * At higher indices we store offscreen rendering formats (visualid=0). Below we calculate
            * the index of the offscreen format. We do this by counting the number of offscreen formats
            * which we see upto reaching our target format. */ 
            if(tmp_vis_id == 0)
                offscreen_index++;

            /* We have found the format in the table (note the format is offscreen) */
            if (fmt_id == tmp_fmt_id) {
                int tmp;

                piFormats[pfmt_it] = offscreen_index + 1; /* Add 1 to get a one-based index */ 
                ++pfmt_it;
                pglXGetFBConfigAttrib(gdi_display, cfgs_fmt[it_fmt], GLX_ALPHA_SIZE, &tmp);
                TRACE("ALPHA_SIZE of FBCONFIG_ID(%d/%d) found as '%d'\n", it_fmt + 1, nCfgs_fmt, tmp);
                break;
            }
        }
        if (it_fmt == nCfgs_fmt) {
            ERR("Failed to get valid fmt for %d. Try next.\n", it);
            continue;
        }
        TRACE("at %d/%d found FBCONFIG_ID(%d/%d)\n", it + 1, nCfgs, piFormats[it], nCfgs_fmt);
    }

    *nNumFormats = pfmt_it;
    /** free list */
    XFree(cfgs);
    XFree(cfgs_fmt);
    return GL_TRUE;
}

/* WGL_ARB_pixel_format: wglGetPixelFormatAttribfvARB */
GLboolean WINAPI X11DRV_wglGetPixelFormatAttribfvARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues)
{
    FIXME("(%p, %d, %d, %d, %p, %p): stub\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);
    return GL_FALSE;
}

/* WGL_ARB_pixel_format: wglGetPixelFormatAttribivARB */
GLboolean WINAPI X11DRV_wglGetPixelFormatAttribivARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues)
{
    UINT i;
    GLXFBConfig* cfgs = NULL;
    GLXFBConfig  curCfg = NULL;
    int nCfgs = 0;
    int hTest;
    int tmp;
    int curGLXAttr = 0;
    int nWGLFormats = 0;
    int fmt_index = 0;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);

    if (0 < iLayerPlane) {
        FIXME("unsupported iLayerPlane(%d) > 0, returns FALSE\n", iLayerPlane);
        return GL_FALSE;
    }

    cfgs = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs);
    if (NULL == cfgs) {
        ERR("no FB Configs found for display(%p)\n", gdi_display);
        return GL_FALSE;
    }

    /* Convert the WGL pixelformat to a GLX one, if this fails then most likely the iPixelFormat isn't supoprted.
    * We don't have to fail yet as a program can specify an invaled iPixelFormat (lets say 0) if it wants to query
    * the number of supported WGL formats. Whether the iPixelFormat is valid is handled in the for-loop below. */
    if(!ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, &fmt_index, &nWGLFormats)) {
        ERR("Unable to convert iPixelFormat %d to a GLX one, expect problems!\n", iPixelFormat);
    }

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

                if (nCfgs < iPixelFormat || 0 >= iPixelFormat) goto pix_error;
                    curCfg = cfgs[iPixelFormat - 1];

                hTest = pglXGetFBConfigAttrib(gdi_display, curCfg, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                switch (tmp) {
                    case GLX_NONE: piValues[i] = WGL_FULL_ACCELERATION_ARB; break;
                    case GLX_SLOW_CONFIG: piValues[i] = WGL_NO_ACCELERATION_ARB; break;
                    case GLX_NON_CONFORMANT_CONFIG: piValues[i] = WGL_FULL_ACCELERATION_ARB; break;
                    default:
                        ERR("unexpected Config Caveat(%x)\n", tmp);
                        piValues[i] = WGL_NO_ACCELERATION_ARB;
                }
                continue;

            case WGL_TRANSPARENT_ARB:
                curGLXAttr = GLX_TRANSPARENT_TYPE;
                    /* Check if the format is supported by checking if iPixelFormat isn't larger than the max number of 
                     * supported WGLFormats and also check if the GLX fmt_index is valid. */
                if((iPixelFormat > nWGLFormats) || (fmt_index > nCfgs)) goto pix_error;
                    curCfg = cfgs[fmt_index];
                hTest = pglXGetFBConfigAttrib(gdi_display, curCfg, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                    piValues[i] = GL_FALSE;
                if (GLX_NONE != tmp) piValues[i] = GL_TRUE;
                    continue;

            case WGL_PIXEL_TYPE_ARB:
                curGLXAttr = GLX_RENDER_TYPE;
                /* Check if the format is supported by checking if iPixelFormat isn't larger than the max number of 
                * supported WGLFormats and also check if the GLX fmt_index is valid. */
                if((iPixelFormat > nWGLFormats) || (fmt_index > nCfgs)) goto pix_error;
                    curCfg = cfgs[fmt_index];
                hTest = pglXGetFBConfigAttrib(gdi_display, curCfg, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                TRACE("WGL_PIXEL_TYPE_ARB: GLX_RENDER_TYPE = 0x%x\n", tmp);
                if      (tmp & GLX_RGBA_BIT)           { piValues[i] = WGL_TYPE_RGBA_ARB; }
                else if (tmp & GLX_COLOR_INDEX_BIT)    { piValues[i] = WGL_TYPE_COLORINDEX_ARB; }
                else if (tmp & GLX_RGBA_FLOAT_BIT)     { piValues[i] = WGL_TYPE_RGBA_FLOAT_ATI; }
                else if (tmp & GLX_RGBA_FLOAT_ATI_BIT) { piValues[i] = WGL_TYPE_RGBA_FLOAT_ATI; }
                else {
                    ERR("unexpected RenderType(%x)\n", tmp);
                    piValues[i] = WGL_TYPE_RGBA_ARB;
                }
                continue;

            case WGL_COLOR_BITS_ARB:
                /** see ConvertAttribWGLtoGLX for explain */
                /* Check if the format is supported by checking if iPixelFormat isn't larger than the max number of 
                * supported WGLFormats and also check if the GLX fmt_index is valid. */
                if((iPixelFormat > nWGLFormats) || (fmt_index > nCfgs)) goto pix_error;
                    curCfg = cfgs[fmt_index];
                hTest = pglXGetFBConfigAttrib(gdi_display, curCfg, GLX_BUFFER_SIZE, piValues + i);
                if (hTest) goto get_error;
                TRACE("WGL_COLOR_BITS_ARB: GLX_BUFFER_SIZE = %d\n", piValues[i]);
                hTest = pglXGetFBConfigAttrib(gdi_display, curCfg, GLX_ALPHA_SIZE, &tmp);
                if (hTest) goto get_error;
                TRACE("WGL_COLOR_BITS_ARB: GLX_ALPHA_SIZE = %d\n", tmp);
                piValues[i] = piValues[i] - tmp;
                continue;

            case WGL_BIND_TO_TEXTURE_RGB_ARB:
                if (use_render_texture_ati) {
                    curGLXAttr = GLX_BIND_TO_TEXTURE_RGB_ATI;
                    break;
                }
            case WGL_BIND_TO_TEXTURE_RGBA_ARB:
                if (use_render_texture_ati) {
                    curGLXAttr = GLX_BIND_TO_TEXTURE_RGBA_ATI;
                    break;
                }
                if (!use_render_texture_emulation) {
                    piValues[i] = GL_FALSE;
                    continue;	
                }
                curGLXAttr = GLX_RENDER_TYPE;
                /* Check if the format is supported by checking if iPixelFormat isn't larger than the max number of 
                 * supported WGLFormats and also check if the GLX fmt_index is valid. */
                if((iPixelFormat > nWGLFormats) || (fmt_index > nCfgs)) goto pix_error;
                    curCfg = cfgs[fmt_index];
                hTest = pglXGetFBConfigAttrib(gdi_display, curCfg, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                if (GLX_COLOR_INDEX_BIT == tmp) {
                    piValues[i] = GL_FALSE;  
                    continue;
                }
                hTest = pglXGetFBConfigAttrib(gdi_display, curCfg, GLX_DRAWABLE_TYPE, &tmp);
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
            case WGL_DRAW_TO_WINDOW_ARB:
            case WGL_DRAW_TO_BITMAP_ARB:
            case WGL_DRAW_TO_PBUFFER_ARB:
                curGLXAttr = GLX_X_RENDERABLE;
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

            default:
                FIXME("unsupported %x WGL Attribute\n", curWGLAttr);
        }

        if (0 != curGLXAttr) {
            /* Check if the format is supported by checking if iPixelFormat isn't larger than the max number of 
            * supported WGLFormats and also check if the GLX fmt_index is valid. */
            if((iPixelFormat > 0) && ((iPixelFormat > nWGLFormats) || (fmt_index > nCfgs))) goto pix_error;
                curCfg = cfgs[fmt_index];
            hTest = pglXGetFBConfigAttrib(gdi_display, curCfg, curGLXAttr, piValues + i);
            if (hTest) goto get_error;
        } else { 
            piValues[i] = GL_FALSE; 
        }
    }
    return GL_TRUE;

get_error:
    ERR("(%p): unexpected failure on GetFBConfigAttrib(%x) returns FALSE\n", hdc, curGLXAttr);
    XFree(cfgs);
    return GL_FALSE;

pix_error:
    ERR("(%p): unexpected iPixelFormat(%d) vs nFormats(%d), returns FALSE\n", hdc, iPixelFormat, nCfgs);
    XFree(cfgs);
    return GL_FALSE;
}

/* WGL_ARB_render_texture: wglBindTexImageARB */
static GLboolean WINAPI X11DRV_wglBindTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
    TRACE("(%p, %d)\n", hPbuffer, iBuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!use_render_texture_ati && 1 == use_render_texture_emulation) {
        int do_init = 0;
        GLint prev_binded_tex;
        pglGetIntegerv(object->texture_target, &prev_binded_tex);
        if (NULL == object->render_ctx) {
            object->render_hdc = X11DRV_wglGetPbufferDCARB(hPbuffer);
            object->render_ctx = X11DRV_wglCreateContext(object->render_hdc);
            do_init = 1;
        }
        object->prev_hdc = X11DRV_wglGetCurrentDC();
        object->prev_ctx = X11DRV_wglGetCurrentContext();
        X11DRV_wglMakeCurrent(object->render_hdc, object->render_ctx);
        /*
        if (do_init) {
            glBindTexture(object->texture_target, object->texture);
            if (GL_RGBA == object->use_render_texture) {
                glTexImage2D(object->texture_target, 0, GL_RGBA8, object->width, object->height, 0, GL_RGBA, GL_FLOAT, NULL);
            } else {
                glTexImage2D(object->texture_target, 0, GL_RGB8, object->width, object->height, 0, GL_RGB, GL_FLOAT, NULL);
            }
        }
        */
        object->texture = prev_binded_tex;
        return GL_TRUE;
    }
    if (NULL != pglXBindTexImageARB) {
        return pglXBindTexImageARB(object->display, object->drawable, iBuffer);
    }
    return GL_FALSE;
}

/* WGL_ARB_render_texture: wglReleaseTexImageARB */
static GLboolean WINAPI X11DRV_wglReleaseTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
    Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
    TRACE("(%p, %d)\n", hPbuffer, iBuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!use_render_texture_ati && 1 == use_render_texture_emulation) {
    /*
        GLint prev_binded_tex;
        glGetIntegerv(object->texture_target, &prev_binded_tex);    
        if (GL_TEXTURE_1D == object->texture_target) {
            glCopyTexSubImage1D(object->texture_target, object->texture_level, 0, 0, 0, object->width);
        } else {
            glCopyTexSubImage2D(object->texture_target, object->texture_level, 0, 0, 0, 0, object->width,       object->height);
        }
        glBindTexture(object->texture_target, prev_binded_tex);
        SwapBuffers(object->render_hdc);
        */
        pglBindTexture(object->texture_target, object->texture);
        if (GL_TEXTURE_1D == object->texture_target) {
            pglCopyTexSubImage1D(object->texture_target, object->texture_level, 0, 0, 0, object->width);
        } else {
            pglCopyTexSubImage2D(object->texture_target, object->texture_level, 0, 0, 0, 0, object->width, object->height);
        }

        X11DRV_wglMakeCurrent(object->prev_hdc, object->prev_ctx);
        return GL_TRUE;
    }
    if (NULL != pglXReleaseTexImageARB) {
        return pglXReleaseTexImageARB(object->display, object->drawable, iBuffer);
    }
    return GL_FALSE;
}

/* WGL_EXT_extensions_string: wglGetExtensionsStringEXT */
const char * WINAPI X11DRV_wglGetExtensionsStringEXT(void) {
    TRACE("() returning \"%s\"\n", WineGLInfo.wglExtensions);
    return WineGLInfo.wglExtensions;
}

/* WGL_EXT_swap_control: wglGetSwapIntervalEXT */
int WINAPI X11DRV_wglGetSwapIntervalEXT(VOID) {
    FIXME("(),stub!\n");
    return swap_interval;
}

/* WGL_EXT_swap_control: wglSwapIntervalEXT */
BOOL WINAPI X11DRV_wglSwapIntervalEXT(int interval) {
    TRACE("(%d)\n", interval);
    swap_interval = interval;
    if (NULL != pglXSwapIntervalSGI) {
        return 0 == pglXSwapIntervalSGI(interval);
    }
    WARN("(): GLX_SGI_swap_control extension seems not supported\n");
    return TRUE;
}

/* Check if the supported GLX version matches requiredVersion */
static BOOL glxRequireVersion(int requiredVersion)
{
    /* Both requiredVersion and glXVersion[1] contains the minor GLX version */
    if(requiredVersion <= WineGLInfo.glxVersion[1])
        return TRUE;

    return FALSE;
}

static BOOL glxRequireExtension(const char *requiredExtension)
{
    if (strstr(WineGLInfo.glxClientExtensions, requiredExtension) == NULL) {
        return FALSE;
    }

    return TRUE;
}

static BOOL register_extension(const WineGLExtension * ext)
{
    int i;

    assert( WineGLExtensionListSize < MAX_EXTENSIONS );
    WineGLExtensionList[WineGLExtensionListSize++] = ext;

    strcat(WineGLInfo.wglExtensions, " ");
    strcat(WineGLInfo.wglExtensions, ext->extName);

    TRACE("'%s'\n", ext->extName);

    for (i = 0; ext->extEntryPoints[i].funcName; ++i)
        TRACE("    - '%s'\n", ext->extEntryPoints[i].funcName);

    return TRUE;
}

static const WineGLExtension WGL_ARB_extensions_string =
{
  "WGL_ARB_extensions_string",
  {
    { "wglGetExtensionsStringARB", X11DRV_wglGetExtensionsStringARB },
  }
};

static const WineGLExtension WGL_ARB_make_current_read =
{
  "WGL_ARB_make_current_read",
  {
    { "wglGetCurrentReadDCARB", X11DRV_wglGetCurrentReadDCARB },
    { "wglMakeContextCurrentARB", X11DRV_wglMakeContextCurrentARB },
  }
};

static const WineGLExtension WGL_ARB_multisample =
{
  "WGL_ARB_multisample",
};

static const WineGLExtension WGL_ARB_pbuffer =
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

static const WineGLExtension WGL_ARB_pixel_format =
{
  "WGL_ARB_pixel_format",
  {
    { "wglChoosePixelFormatARB", X11DRV_wglChoosePixelFormatARB },
    { "wglGetPixelFormatAttribfvARB", X11DRV_wglGetPixelFormatAttribfvARB },
    { "wglGetPixelFormatAttribivARB", X11DRV_wglGetPixelFormatAttribivARB },
  }
};

static const WineGLExtension WGL_ARB_render_texture =
{
  "WGL_ARB_render_texture",
  {
    { "wglBindTexImageARB", X11DRV_wglBindTexImageARB },
    { "wglReleaseTexImageARB", X11DRV_wglReleaseTexImageARB },
  }
};

static const WineGLExtension WGL_EXT_extensions_string =
{
  "WGL_EXT_extensions_string",
  {
    { "wglGetExtensionsStringEXT", X11DRV_wglGetExtensionsStringEXT },
  }
};

static const WineGLExtension WGL_EXT_swap_control =
{
  "WGL_EXT_swap_control",
  {
    { "wglSwapIntervalEXT", X11DRV_wglSwapIntervalEXT },
    { "wglGetSwapIntervalEXT", X11DRV_wglGetSwapIntervalEXT },
  }
};


/**
 * X11DRV_WineGL_LoadExtensions
 */
static void X11DRV_WineGL_LoadExtensions(void)
{
    WineGLInfo.wglExtensions[0] = 0;

    /* ARB Extensions */

    register_extension(&WGL_ARB_extensions_string);

    if (glxRequireVersion(3))
        register_extension(&WGL_ARB_make_current_read);

    if (glxRequireExtension("GLX_ARB_multisample"))
        register_extension(&WGL_ARB_multisample);

    if (glxRequireVersion(3) && glxRequireExtension("GLX_SGIX_pbuffer"))
        register_extension(&WGL_ARB_pbuffer);

    register_extension(&WGL_ARB_pixel_format);

    if (glxRequireExtension("GLX_ATI_render_texture") ||
        glxRequireExtension("GLX_ARB_render_texture"))
        register_extension(&WGL_ARB_render_texture);

    /* EXT Extensions */

    register_extension(&WGL_EXT_extensions_string);

    if (glxRequireExtension("GLX_SGI_swap_control"))
        register_extension(&WGL_EXT_swap_control);
}


static XID create_glxpixmap(X11DRV_PDEVICE *physDev)
{
    GLXPixmap ret;
    XVisualInfo *vis;
    XVisualInfo template;
    int num;

    wine_tsx11_lock();

    /* Retrieve the visualid from our main visual which is the only visual we can use */
    template.visualid =  XVisualIDFromVisual(visual);
    vis = XGetVisualInfo(gdi_display, VisualIDMask, &template, &num);

    ret = pglXCreateGLXPixmap(gdi_display, vis, physDev->bitmap->pixmap);
    XFree(vis);
    wine_tsx11_unlock(); 
    TRACE("return %lx\n", ret);
    return ret;
}

Drawable get_glxdrawable(X11DRV_PDEVICE *physDev)
{
    Drawable ret;

    if(physDev->bitmap)
    {
        if (physDev->bitmap->hbitmap == BITMAP_stock_phys_bitmap.hbitmap)
            ret = physDev->drawable; /* PBuffer */
        else
        {
            if(!physDev->bitmap->glxpixmap)
                physDev->bitmap->glxpixmap = create_glxpixmap(physDev);
            ret = physDev->bitmap->glxpixmap;
        }
    }
    else
        ret = physDev->drawable;
    return ret;
}

BOOL destroy_glxpixmap(XID glxpixmap)
{
    wine_tsx11_lock(); 
    pglXDestroyGLXPixmap(gdi_display, glxpixmap);
    wine_tsx11_unlock(); 
    return TRUE;
}

/**
 * X11DRV_SwapBuffers
 *
 * Swap the buffers of this DC
 */
BOOL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev)
{
  GLXDrawable drawable;
  if (!has_opengl()) {
    ERR("No libGL on this box - disabling OpenGL support !\n");
    return 0;
  }
  
  TRACE_(opengl)("(%p)\n", physDev);

  drawable = get_glxdrawable(physDev);
  wine_tsx11_lock();
  pglXSwapBuffers(gdi_display, drawable);
  wine_tsx11_unlock();

  /* FPS support */
  if (TRACE_ON(fps))
  {
      static long prev_time, frames;

      DWORD time = GetTickCount();
      frames++;
      /* every 1.5 seconds */
      if (time - prev_time > 1500) {
          TRACE_(fps)("@ approx %.2ffps\n", 1000.0*frames/(time - prev_time));
          prev_time = time;
          frames = 0;
      }
  }

  return TRUE;
}

/***********************************************************************
 *		X11DRV_setup_opengl_visual
 *
 * Setup the default visual used for OpenGL and Direct3D, and the desktop
 * window (if it exists).  If OpenGL isn't available, the visual is simply
 * set to the default visual for the display
 */
XVisualInfo *X11DRV_setup_opengl_visual( Display *display )
{
    XVisualInfo *visual = NULL;
    /* In order to support OpenGL or D3D, we require a double-buffered visual and stencil buffer support, */
    int dblBuf[] = {GLX_RGBA,GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DOUBLEBUFFER, None};
    if (!has_opengl()) return NULL;

    wine_tsx11_lock();
    visual = pglXChooseVisual(display, DefaultScreen(display), dblBuf);
    wine_tsx11_unlock();
    if (visual == NULL) {
        /* fallback to 16 bits depth, no alpha */
        int dblBuf2[] = {GLX_RGBA,GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None};
        WARN("Failed to get a visual with at least 24 bits depth\n");

        wine_tsx11_lock();
        visual = pglXChooseVisual(display, DefaultScreen(display), dblBuf2);
        wine_tsx11_unlock();
        if (visual == NULL) {
            /* fallback to no stencil */
            int dblBuf2[] = {GLX_RGBA,GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None};
            WARN("Failed to get a visual with at least 8 bits of stencil\n");

            wine_tsx11_lock();
            visual = pglXChooseVisual(display, DefaultScreen(display), dblBuf2);
            wine_tsx11_unlock();
            if (visual == NULL) {
                /* This should only happen if we cannot find a match with a depth size 16 */
                FIXME("Failed to find a suitable visual\n");
                return visual;
            }
        }
    }
    TRACE("Visual ID %lx Chosen\n",visual->visualid);
    return visual;
}

#else  /* no OpenGL includes */

void X11DRV_OpenGL_Init(Display *display)
{
}

/***********************************************************************
 *		ChoosePixelFormat (X11DRV.@)
 */
int X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev,
			     const PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");

  return 0;
}

/***********************************************************************
 *		DescribePixelFormat (X11DRV.@)
 */
int X11DRV_DescribePixelFormat(X11DRV_PDEVICE *physDev,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");

  return 0;
}

/***********************************************************************
 *		GetPixelFormat (X11DRV.@)
 */
int X11DRV_GetPixelFormat(X11DRV_PDEVICE *physDev) {
  ERR("No OpenGL support compiled in.\n");

  return 0;
}

/***********************************************************************
 *		SetPixelFormat (X11DRV.@)
 */
BOOL X11DRV_SetPixelFormat(X11DRV_PDEVICE *physDev,
			   int iPixelFormat,
			   const PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");

  return FALSE;
}

/***********************************************************************
 *		SwapBuffers (X11DRV.@)
 */
BOOL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev) {
  ERR_(opengl)("No OpenGL support compiled in.\n");

  return FALSE;
}

/* OpenGL32 wglCreateContext */
HGLRC WINAPI X11DRV_wglCreateContext(HDC hdc) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return NULL;
}

/* OpenGL32 wglDeleteContext */
BOOL WINAPI X11DRV_wglDeleteContext(HGLRC hglrc) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return FALSE;
}

/* OpenGL32 wglGetCurrentContext() */
HGLRC WINAPI X11DRV_wglGetCurrentContext(void) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return NULL;
}

/* OpenGL32 wglGetCurrentDC */
HDC WINAPI X11DRV_wglGetCurrentDC(void) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return 0;
}

/* OpenGL32 wglGetCurrentReadDCARB */
HDC WINAPI X11DRV_wglGetCurrentReadDCARB(void)
{
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return 0;
}

/* OpenGL32: wglGetProcAddress */
PROC X11DRV_wglGetProcAddress(LPCSTR lpszProc) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return NULL;
}

/* OpenGL32 wglMakeCurrent */
BOOL WINAPI X11DRV_wglMakeCurrent(HDC hdc, HGLRC hglrc) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return FALSE;
}

/* OpenGL32 wglMakeContextCurrentARB */
BOOL WINAPI X11DRV_wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc) 
{
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return FALSE;
}

/* OpenGL32 wglShaderLists */
BOOL WINAPI X11DRV_wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return FALSE;
}

/* WGL helper function which handles differences in glGetIntegerv from WGL and GLX */ 
void X11DRV_wglGetIntegerv(int pname, int* params) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
}

XVisualInfo *X11DRV_setup_opengl_visual( Display *display )
{
  return NULL;
}

Drawable get_glxdrawable(X11DRV_PDEVICE *physDev)
{
    return 0;
}

BOOL destroy_glxpixmap(XID glxpixmap)
{
    return FALSE;
}

#endif /* defined(HAVE_OPENGL) */
