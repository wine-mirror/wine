/* Support for window-specific OpenGL extensions.
 *
 * Copyright (c) 2004 Lionel Ulmer
 * Copyright (c) 2005 Alex Woods
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"

#include "wgl.h"
#include "wgl_ext.h"
#include "opengl_ext.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);


/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,   /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,  /* get current drawable for a DC */
    X11DRV_GET_FONT,      /* get current X font for a DC */
    X11DRV_SET_DRAWABLE,     /* set current drawable for a DC */
};
struct x11drv_escape_set_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_SET_DRAWABLE) */
    Drawable                 drawable;     /* X drawable */
    int                      mode;         /* ClipByChildren or IncludeInferiors */
    POINT                    org;          /* origin of DC relative to drawable */
    POINT                    drawable_org; /* origin of drawable relative to screen */
};

/* retrieve the X display to use on a given DC */
inline static Display *get_display( HDC hdc )
{
    Display *display;
    enum x11drv_escape_codes escape = X11DRV_GET_DISPLAY;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(display), (LPSTR)&display )) display = NULL;
    return display;
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

/* Some WGL extensions... */
static const char *WGL_extensions_base = "WGL_ARB_extensions_string WGL_EXT_extensions_string";
static char *WGL_extensions = NULL;

/**
 * Extensions-query functions 
 *
 * @TODO: use a struct to handle parameters 
 */
BOOL query_function_multisample(glXGetProcAddressARB_t proc, const char *gl_version, const char *gl_extensions, 
				const char* glx_version, const char *glx_extensions,
				const char *server_glx_extensions, const char *client_glx_extensions)
{
  return NULL != strstr("GLX_ARB_multisample", glx_extensions);
}

BOOL query_function_pbuffer(glXGetProcAddressARB_t proc, const char *gl_version, const char *gl_extensions, 
			    const char* glx_version, const char *glx_extensions,
			    const char *server_glx_extensions, const char *client_glx_extensions)
{
  FIXME("gl_version is: \"%s\"\n", gl_version);
  FIXME("glx_exts is: \"%s\"\n", glx_extensions);

  return 0 <= strcmp("1.3", glx_version) || NULL != strstr(glx_extensions, "GLX_SGIX_pbuffer");
}

BOOL query_function_pixel_format(glXGetProcAddressARB_t proc, const char *gl_version, const char *gl_extensions, 
				 const char* glx_version, const char *glx_extensions,
				 const char *server_glx_extensions, const char *client_glx_extensions)
{
  return TRUE;
}

/** GLX_ARB_render_texture */
Bool (*p_glXBindTexImageARB)(Display *dpy, GLXPbuffer pbuffer, int buffer);
Bool (*p_glXReleaseTexImageARB)(Display *dpy, GLXPbuffer pbuffer, int buffer);
Bool (*p_glXDrawableAttribARB)(Display *dpy, GLXDrawable draw, const int *attribList);

BOOL query_function_render_texture(glXGetProcAddressARB_t proc, const char *gl_version, const char *gl_extensions, 
				   const char* glx_version, const char *glx_extensions,
				   const char *server_glx_extensions, const char *client_glx_extensions)
{
  BOOL bTest = (0 <= strcmp("1.3", glx_version) || NULL != strstr(glx_extensions, "GLX_SGIX_pbuffer") || NULL != strstr(glx_extensions, "GLX_ARB_render_texture"));
  if (bTest) {
    p_glXBindTexImageARB = proc("glXBindTexImageARB");
    p_glXReleaseTexImageARB = proc("glXReleaseTexImageARB");
    p_glXDrawableAttribARB = proc("glXDrawableAttribARB");
    bTest = (NULL != p_glXBindTexImageARB && NULL != p_glXReleaseTexImageARB && NULL != p_glXDrawableAttribARB);
  }
  return bTest;
}

/***********************************************************************
 *              wglGetExtensionsStringEXT(OPENGL32.@)
 */
const char * WINAPI wglGetExtensionsStringEXT(void) {
    TRACE("() returning \"%s\"\n", WGL_extensions);

    return WGL_extensions;
}

/***********************************************************************
 *              wglGetExtensionsStringARB(OPENGL32.@)
 */
const char * WINAPI wglGetExtensionsStringARB(HDC hdc) {
    TRACE("() returning \"%s\"\n", WGL_extensions);

    return WGL_extensions;    
}

static int swap_interval = 1;

/***********************************************************************
 *              wglSwapIntervalEXT(OPENGL32.@)
 */
BOOL WINAPI wglSwapIntervalEXT(int interval) {
    FIXME("(%d),stub!\n", interval);

    swap_interval = interval;
    return TRUE;
}

/***********************************************************************
 *              wglGetSwapIntervalEXT(OPENGL32.@)
 */
int WINAPI wglGetSwapIntervalEXT(VOID) {
    FIXME("(),stub!\n");
    return swap_interval;
}

typedef struct wine_glpbuffer {
  Drawable   drawable;
  Display*   display;
  int        pixelFormat;
  int        width;
  int        height;
  int*       attribList;
  HDC        hdc;
} Wine_GLPBuffer;

#define PUSH1(attribs,att)        attribs[nAttribs++] = (att); 
#define PUSH2(attribs,att,value)  attribs[nAttribs++] = (att); attribs[nAttribs++] = (value);

#define WGL_NUMBER_PIXEL_FORMATS_ARB		0x2000
#define WGL_DRAW_TO_WINDOW_ARB			0x2001
#define WGL_DRAW_TO_BITMAP_ARB			0x2002
#define WGL_ACCELERATION_ARB			0x2003
#define WGL_NEED_PALETTE_ARB			0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB		0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB		0x2006
#define WGL_SWAP_METHOD_ARB			0x2007
#define WGL_NUMBER_OVERLAYS_ARB			0x2008
#define WGL_NUMBER_UNDERLAYS_ARB		0x2009
#define WGL_TRANSPARENT_ARB			0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB		0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB		0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB		0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB		0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB		0x203B
#define WGL_SHARE_DEPTH_ARB			0x200C
#define WGL_SHARE_STENCIL_ARB			0x200D
#define WGL_SHARE_ACCUM_ARB			0x200E
#define WGL_SUPPORT_GDI_ARB			0x200F
#define WGL_SUPPORT_OPENGL_ARB			0x2010
#define WGL_DOUBLE_BUFFER_ARB			0x2011
#define WGL_STEREO_ARB				0x2012
#define WGL_PIXEL_TYPE_ARB			0x2013
#define WGL_COLOR_BITS_ARB			0x2014
#define WGL_RED_BITS_ARB			0x2015
#define WGL_RED_SHIFT_ARB			0x2016
#define WGL_GREEN_BITS_ARB			0x2017
#define WGL_GREEN_SHIFT_ARB			0x2018
#define WGL_BLUE_BITS_ARB			0x2019
#define WGL_BLUE_SHIFT_ARB			0x201A
#define WGL_ALPHA_BITS_ARB			0x201B
#define WGL_ALPHA_SHIFT_ARB			0x201C
#define WGL_ACCUM_BITS_ARB			0x201D
#define WGL_ACCUM_RED_BITS_ARB			0x201E
#define WGL_ACCUM_GREEN_BITS_ARB		0x201F
#define WGL_ACCUM_BLUE_BITS_ARB			0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB		0x2021
#define WGL_DEPTH_BITS_ARB			0x2022
#define WGL_STENCIL_BITS_ARB			0x2023
#define WGL_AUX_BUFFERS_ARB			0x2024

#define WGL_PBUFFER_WIDTH_ARB                0x2034
#define WGL_PBUFFER_HEIGHT_ARB               0x2035
#define WGL_PBUFFER_LOST_ARB                 0x2036

#if 0 /* not used yet */
static unsigned ConvertAttribGLXtoWGL(const int* iWGLAttr, int* oGLXAttr) {
  unsigned nAttribs = 0;
  FIXME("not yet implemented!\n");
  return nAttribs;
}
#endif

static unsigned ConvertAttribWGLtoGLX(const int* iWGLAttr, int* oGLXAttr) {
  unsigned nAttribs = 0;
  unsigned cur = 0; 
  int pop;

  while (0 != iWGLAttr[cur]) {
    switch (iWGLAttr[cur]) {
    case WGL_COLOR_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_BUFFER_SIZE, pop);
      break;
    case WGL_ALPHA_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_ALPHA_SIZE, pop);
      break;
    case WGL_DEPTH_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_DEPTH_SIZE, pop);
      break;
    case WGL_STENCIL_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_STENCIL_SIZE, pop);
      break;
    case WGL_DOUBLE_BUFFER_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_DOUBLEBUFFER, pop);
      break;
    case WGL_DRAW_TO_WINDOW_ARB:
    case WGL_SUPPORT_OPENGL_ARB:
    case WGL_ACCELERATION_ARB:
    /*
    case WGL_SAMPLE_BUFFERS_ARB:
    case WGL_SAMPLES_ARB:
    */
    default:
      break;
    }
    ++cur;
  }
  return nAttribs;
}

GLboolean WINAPI wglGetPixelFormatAttribivARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues)
{
    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);
    return GL_TRUE;
}

GLboolean WINAPI wglGetPixelFormatAttribfvARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues)
{
    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);
    return GL_TRUE;
}

GLboolean WINAPI wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)
{
  Display* display = get_display( hdc );
  GLXFBConfig* cfgs = NULL;
  int nCfgs = 0;
  int attribs[256];
  unsigned nAttribs;
  UINT it;

  TRACE("(%p, %p, %p, %d, %p, %p): hackish\n", hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats);
  if (NULL != pfAttribFList) {
    FIXME("unused pfAttribFList\n");
  }

  nAttribs = ConvertAttribWGLtoGLX(piAttribIList, attribs);
  PUSH1(attribs, None);

  cfgs = glXChooseFBConfig(display, DefaultScreen(display), attribs, &nCfgs);
  for (it = 0; it < nMaxFormats && it < nCfgs; ++it) {
    piFormats[it] = it;
  }
  *nNumFormats = it;
  return GL_TRUE;
}

#define HPBUFFERARB void *
HPBUFFERARB WINAPI wglCreatePbufferARB(HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList)
{
  Wine_GLPBuffer* object = NULL;
  Display* display = get_display( hdc );
  GLXFBConfig* cfgs = NULL;
  int nCfgs = 0;
  int attribs[256];
  unsigned nAttribs;

  TRACE("(%p, %d, %d, %d, %p)\n", hdc, iPixelFormat, iWidth, iHeight, piAttribList);

  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Wine_GLPBuffer));
  object->hdc = hdc;
  object->display = display;
  object->width = iWidth;
  object->height = iHeight;
  object->pixelFormat = iPixelFormat;

  nAttribs = ConvertAttribWGLtoGLX(piAttribList, attribs);
  PUSH1(attribs, None);

  cfgs = glXChooseFBConfig(display, DefaultScreen(display), attribs, &nCfgs);
  if (nCfgs < iPixelFormat) return NULL; /* unespected error */

  --nAttribs; /** append more to attribs now we have fbConfig */
  PUSH2(attribs, GLX_PBUFFER_WIDTH,  iWidth);
  PUSH2(attribs, GLX_PBUFFER_HEIGHT, iHeight);
  PUSH1(attribs, None);

  object->drawable = glXCreatePbuffer(display, cfgs[iPixelFormat], attribs);
  TRACE("drawable as %p\n", (void*) object->drawable);

  TRACE("->(%p)\n", object);
  return (HPBUFFERARB) object;
}

HDC WINAPI wglGetPbufferDCARB(HPBUFFERARB hPbuffer)
{
  Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
  HDC hDC;
  hDC = CreateCompatibleDC(object->hdc);
  set_drawable(hDC, object->drawable); /* works ?? */
  TRACE("(%p)\n", hPbuffer);
  return hDC;
}

int WINAPI wglReleasePbufferDCARB(HPBUFFERARB hPbuffer, HDC hdc)
{
  TRACE("(%p, %p)\n", hPbuffer, hdc);
  DeleteDC(hdc);
  return 0;
}

GLboolean WINAPI wglDestroyPbufferARB(HPBUFFERARB hPbuffer)
{
  Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
  TRACE("(%p)\n", hPbuffer);

  glXDestroyPbuffer(object->display, object->drawable);
  HeapFree(GetProcessHeap(), 0, object);

  return GL_TRUE;
}

GLboolean WINAPI wglQueryPbufferARB(HPBUFFERARB hPbuffer, int iAttribute, int *piValue)
{
  Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
  TRACE("(%p, %d, %p)\n", hPbuffer, iAttribute, piValue);

  /*glXQueryDrawable(object->display, object->drawable);*/

  switch (iAttribute) {
  case WGL_PBUFFER_WIDTH_ARB:
    glXQueryDrawable(object->display, object->drawable, GLX_WIDTH, piValue);
    break;
  case WGL_PBUFFER_HEIGHT_ARB:
    glXQueryDrawable(object->display, object->drawable, GLX_HEIGHT, piValue);
    break;

  default:
    break;
  }

  return GL_TRUE;
}

GLboolean WINAPI wglBindTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
  Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
  TRACE("(%p, %d)\n", hPbuffer, iBuffer);
  return p_glXBindTexImageARB(object->display, object->drawable, iBuffer);
}

GLboolean WINAPI wglReleaseTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
  Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
  TRACE("(%p, %d)\n", hPbuffer, iBuffer);
  return p_glXReleaseTexImageARB(object->display, object->drawable, iBuffer);
}

GLboolean WINAPI wglSetPbufferAttribARB(HPBUFFERARB hPbuffer, const int *piAttribList)
{
  Wine_GLPBuffer* object = (Wine_GLPBuffer*) hPbuffer;
  WARN("(%p, %p): alpha-testing, report any problem\n", hPbuffer, piAttribList);
  return p_glXDrawableAttribARB(object->display, object->drawable, piAttribList);
}

static const struct {
    const char *name;
    BOOL (*query_function)(glXGetProcAddressARB_t proc, const char *gl_version, const char *gl_extensions,
			   const char *glx_version, const char *glx_extensions,
			   const char *server_glx_extensions, const char *client_glx_extensions);
} extension_list[] = {
  { "WGL_ARB_multisample", query_function_multisample },
  { "WGL_ARB_pbuffer", query_function_pbuffer },
  { "WGL_ARB_pixel_format" , query_function_pixel_format },
  { "WGL_ARB_render_texture", query_function_render_texture }
};

/* Used to initialize the WGL extension string at DLL loading */
void wgl_ext_initialize_extensions(Display *display, int screen, glXGetProcAddressARB_t proc)
{
    int size = strlen(WGL_extensions_base);
    const char *glx_extensions = glXQueryExtensionsString(display, screen);
    const char *server_glx_extensions = glXQueryServerString(display, screen, GLX_EXTENSIONS);
    const char *client_glx_extensions = glXGetClientString(display, GLX_EXTENSIONS);
    const char *gl_extensions = (const char *) glGetString(GL_EXTENSIONS);
    const char *gl_version = (const char *) glGetString(GL_VERSION);
    const char *glx_version = glXGetClientString(display, GLX_VERSION);
    int i;

    TRACE("GL version      : %s.\n", debugstr_a(gl_version));
    TRACE("GL exts         : %s.\n", debugstr_a(gl_extensions));
    TRACE("GLX exts        : %s.\n", debugstr_a(glx_extensions));
    TRACE("Server GLX exts : %s.\n", debugstr_a(server_glx_extensions));
    TRACE("Client GLX exts : %s.\n", debugstr_a(client_glx_extensions));

    for (i = 0; i < (sizeof(extension_list) / sizeof(extension_list[0])); i++) {
	if (extension_list[i].query_function(proc, gl_version, gl_extensions, 
					     glx_version, glx_extensions,
					     server_glx_extensions, client_glx_extensions)) {
	    size += strlen(extension_list[i].name) + 1;
	}
    }

    /* For the moment, only 'base' extensions are supported. */
    WGL_extensions = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 1);
    if (WGL_extensions == NULL) {
	WGL_extensions = (char *) WGL_extensions_base;
    } else {
	strcpy(WGL_extensions, WGL_extensions_base);
	for (i = 0; i < (sizeof(extension_list) / sizeof(extension_list[0])); i++) {
	    if (extension_list[i].query_function(proc, gl_version, gl_extensions, 
						 glx_version, glx_extensions,
						 server_glx_extensions, client_glx_extensions)) {
		strcat(WGL_extensions, " ");
		strcat(WGL_extensions, extension_list[i].name);
	    }
	}
    }

    TRACE("Supporting following WGL extensions : %s.\n", debugstr_a(WGL_extensions));
}

void wgl_ext_finalize_extensions(void)
{
    if (WGL_extensions != WGL_extensions_base) {
	HeapFree(GetProcessHeap(), 0, WGL_extensions);
    }
}

/*
 * Putting this at the end to prevent having to write the prototypes :-) 
 *
 * @WARNING: this list must be ordered by name
 */
WGL_extension wgl_extension_registry[] = {
    { "wglBindTexImageARB", (void *) wglBindTexImageARB, NULL, NULL},
    { "wglChoosePixelFormatARB", (void *) wglChoosePixelFormatARB, NULL, NULL},
    { "wglCreatePbufferARB", (void *) wglCreatePbufferARB, NULL, NULL},
    { "wglDestroyPbufferARB", (void *) wglDestroyPbufferARB, NULL, NULL},
    { "wglGetExtensionsStringARB", (void *) wglGetExtensionsStringARB, NULL, NULL},
    { "wglGetExtensionsStringEXT", (void *) wglGetExtensionsStringEXT, NULL, NULL},
    { "wglGetPbufferDCARB", (void *) wglGetPbufferDCARB, NULL, NULL},
    { "wglGetPixelFormatAttribfvARB", (void *) wglGetPixelFormatAttribfvARB, NULL, NULL},
    { "wglGetPixelFormatAttribivARB", (void *) wglGetPixelFormatAttribivARB, NULL, NULL},
    { "wglGetSwapIntervalEXT", (void *) wglGetSwapIntervalEXT, NULL, NULL},
    { "wglQueryPbufferARB", (void *) wglQueryPbufferARB, NULL, NULL},
    { "wglReleasePbufferDCARB", (void *) wglReleasePbufferDCARB, NULL, NULL},
    { "wglReleaseTexImageARB", (void *) wglReleaseTexImageARB, NULL, NULL},
    { "wglSetPbufferAttribARB", (void *) wglSetPbufferAttribARB, NULL, NULL},
    { "wglSwapIntervalEXT", (void *) wglSwapIntervalEXT, NULL, NULL}
};
int wgl_extension_registry_size = sizeof(wgl_extension_registry) / sizeof(wgl_extension_registry[0]);
