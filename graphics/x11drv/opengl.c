/* 
 * X11DRV OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
 *
 */

#include "config.h"

#include "ts_xlib.h"

#include <stdlib.h>
#include <string.h>

#include "debugtools.h"
#include "gdi.h"
#include "x11drv.h"
#include "wine_gl.h"

DEFAULT_DEBUG_CHANNEL(opengl)

#ifdef HAVE_OPENGL

static void dump_PIXELFORMATDESCRIPTOR(PIXELFORMATDESCRIPTOR *ppfd) {
  DPRINTF("  - size / version : %d / %d\n", ppfd->nSize, ppfd->nVersion);
  DPRINTF("  - dwFlags : ");
#define TEST_AND_DUMP(t,tv) if ((t) & (tv)) DPRINTF(#tv " ")
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
  DPRINTF("\n");
  
  DPRINTF("  - iPixelType : ");
  switch (ppfd->iPixelType) {
  case PFD_TYPE_RGBA: DPRINTF("PFD_TYPE_RGBA"); break;
  case PFD_TYPE_COLORINDEX: DPRINTF("PFD_TYPE_COLORINDEX"); break;
  }
  DPRINTF("\n");
  
  DPRINTF("  - Color   : %d\n", ppfd->cColorBits);
  DPRINTF("  - Alpha   : %d\n", ppfd->cAlphaBits);
  DPRINTF("  - Accum   : %d\n", ppfd->cAccumBits);
  DPRINTF("  - Depth   : %d\n", ppfd->cDepthBits);
  DPRINTF("  - Stencil : %d\n", ppfd->cStencilBits);
  DPRINTF("  - Aux     : %d\n", ppfd->cAuxBuffers);
  
  DPRINTF("  - iLayerType : ");
  switch (ppfd->iLayerType) {
  case PFD_MAIN_PLANE: DPRINTF("PFD_MAIN_PLANE"); break;
  case PFD_OVERLAY_PLANE: DPRINTF("PFD_OVERLAY_PLANE"); break;
  case PFD_UNDERLAY_PLANE: DPRINTF("PFD_UNDERLAY_PLANE"); break;
  }
  DPRINTF("\n");
}
     
/* X11DRV_ChoosePixelFormat

     Equivalent of glXChooseVisual
*/
int X11DRV_ChoosePixelFormat(DC *dc,
			     const PIXELFORMATDESCRIPTOR *ppfd) {
#define TEST_AND_ADD1(t,a) if (t) att_list[att_pos++] = a
#define TEST_AND_ADD2(t,a,b) if (t) { att_list[att_pos++] = a; att_list[att_pos++] = b; }
#define NULL_TEST_AND_ADD2(tv,a,b) att_list[att_pos++] = a; att_list[att_pos++] = ((tv) == 0 ? 0 : b)
#define ADD2(a,b) att_list[att_pos++] = a; att_list[att_pos++] = b
  
  X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
  int att_list[64];
  int att_pos = 0;
  XVisualInfo *vis;
  
  if (TRACE_ON(opengl)) {
    TRACE("(%p,%p)\n", dc, ppfd);
    
    dump_PIXELFORMATDESCRIPTOR((PIXELFORMATDESCRIPTOR *) ppfd);
  }

  /* For the moment, we are dumb : we always allocate a new XVisualInfo structure,
     we do not try to find an already found that could match */
  if (physDev->used_visuals == MAX_PIXELFORMATS) {
    ERR("Maximum number of visuals reached !\n");
    /* Should SetError here... */
    return 0;
  }

  if (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) {
    ERR("Flag not supported !\n");
    /* Should SetError here... */
    return 0;
  }

  /* Now, build the request to GLX */
  TEST_AND_ADD1(ppfd->dwFlags & PFD_DOUBLEBUFFER, GLX_DOUBLEBUFFER);
  TEST_AND_ADD1(ppfd->dwFlags & PFD_STEREO, GLX_STEREO);
  TEST_AND_ADD1(ppfd->iPixelType == PFD_TYPE_RGBA, GLX_RGBA);
  TEST_AND_ADD2(ppfd->iPixelType == PFD_TYPE_COLORINDEX, GLX_BUFFER_SIZE, ppfd->cColorBits);

  NULL_TEST_AND_ADD2(ppfd->cDepthBits, GLX_DEPTH_SIZE, 8);
  /* These flags are not supported yet...
     
     NULL_TEST_AND_ADD2(ppfd->cAlphaBits, GLX_ALPHA_SIZE, 8);
     ADD2(GLX_ACCUM_SIZE, ppfd->cAccumBits); 
     ADD2(GLX_STENCIL_SIZE, ppfd->cStencilBits);
     ADD2(GLX_AUX_BUFFERS, ppfd->cAuxBuffers); */
  att_list[att_pos] = None;
  
  ENTER_GL(); {
    /*
       This command cannot be used as we need to use the default visual...
       Let's hope it at least contains some OpenGL functionnalities
       
       vis = glXChooseVisual(display, DefaultScreen(display), att_list);
    */
    int num;
    XVisualInfo template;

    template.visualid = XVisualIDFromVisual(visual);
    vis = XGetVisualInfo(display, VisualIDMask, &template, &num);

    TRACE("Found visual : %p - returns %d\n", vis, physDev->used_visuals + 1);
  }
  LEAVE_GL();
  
  if (vis == NULL) {
    ERR("No visual found !\n");
    /* Should SetError here... */
    return 0;
  }
  physDev->visuals[physDev->used_visuals++] = vis;
  
  return physDev->used_visuals;
}

/* X11DRV_DescribePixelFormat

     Get the pixel-format descriptor associated to the given id
*/
int X11DRV_DescribePixelFormat(DC *dc,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd) {
  X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
  XVisualInfo *vis;
  int value;
  int rb,gb,bb,ab;
  
  TRACE("(%p,%d,%d,%p)\n", dc, iPixelFormat, nBytes, ppfd);
  
  if (nBytes < sizeof(PIXELFORMATDESCRIPTOR)) {
    ERR("Wrong structure size !\n");
    /* Should set error */
    return 0;
  }
  if ((iPixelFormat > MAX_PIXELFORMATS) ||
      (iPixelFormat > physDev->used_visuals + 1) ||
      (iPixelFormat <= 0)) {
    ERR("Wrong pixel format !\n");
    /* Should set error */
    return 0;
  }
      
  if (iPixelFormat == physDev->used_visuals + 1) {
    int dblBuf[]={GLX_RGBA,GLX_DEPTH_SIZE,16,GLX_DOUBLEBUFFER,None};

    /* Create a 'standard' X Visual */
    ENTER_GL();
    vis = glXChooseVisual(display,
			  DefaultScreen(display),
			  dblBuf);
    LEAVE_GL();
    
    WARN("Uninitialized Visual. Creating standard (%p) !\n", vis);

    if (vis == NULL) {
      ERR("Could not create standard visual !\n");
      /* Should set error */
      return 0;
    }
    
    physDev->visuals[physDev->used_visuals++] = vis;
  }
  vis = physDev->visuals[iPixelFormat - 1];

  memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
  ppfd->nVersion = 1;
  
  /* These flags are always the same... */
  ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED;
  /* Now the flags extraced from the Visual */
  ENTER_GL();
  glXGetConfig(display, vis, GLX_DOUBLEBUFFER, &value); if (value) ppfd->dwFlags |= PFD_DOUBLEBUFFER;
  glXGetConfig(display, vis, GLX_STEREO, &value); if (value) ppfd->dwFlags |= PFD_STEREO;

  /* Pixel type */
  glXGetConfig(display, vis, GLX_RGBA, &value);
  if (value)
    ppfd->iPixelType = PFD_TYPE_RGBA;
  else
    ppfd->iPixelType = PFD_TYPE_COLORINDEX;

  /* Color bits */
  glXGetConfig(display, vis, GLX_BUFFER_SIZE, &value);
  ppfd->cColorBits = value;

  /* Red, green, blue and alpha bits / shifts */
  if (ppfd->iPixelType == PFD_TYPE_RGBA) {
    glXGetConfig(display, vis, GLX_RED_SIZE, &rb);
    glXGetConfig(display, vis, GLX_GREEN_SIZE, &gb);
    glXGetConfig(display, vis, GLX_BLUE_SIZE, &bb);
    glXGetConfig(display, vis, GLX_ALPHA_SIZE, &ab);
    
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
  glXGetConfig(display, vis, GLX_DEPTH_SIZE, &value);
  ppfd->cDepthBits = value;
  LEAVE_GL();

  /* Aux, stencil : to do ... */

  ppfd->iLayerType = PFD_MAIN_PLANE;
  
  if (TRACE_ON(opengl)) {
    dump_PIXELFORMATDESCRIPTOR(ppfd);
  }
  
  return MAX_PIXELFORMATS;
}

/* X11DRV_GetPixelFormat

     Get the pixel-format id used by this DC
*/
int X11DRV_GetPixelFormat(DC *dc) {
  X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
  
  TRACE("(%p): returns %d\n", dc, physDev->current_pf);

  return physDev->current_pf;
}

/* X11DRV_SetPixelFormat

     Set the pixel-format id used by this DC
*/
BOOL X11DRV_SetPixelFormat(DC *dc,
			   int iPixelFormat,
			   const PIXELFORMATDESCRIPTOR *ppfd) {
  X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
  
  TRACE("(%p,%d,%p)\n", dc, iPixelFormat, ppfd);

  physDev->current_pf = iPixelFormat;
  
  return TRUE;
}

/* X11DRV_SwapBuffers

     Swap the buffers of this DC
*/
BOOL X11DRV_SwapBuffers(DC *dc) {
  X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
  
  TRACE("(%p)\n", dc);

  ENTER_GL();
  glXSwapBuffers(display, physDev->drawable);
  LEAVE_GL();
  
  return TRUE;
}

#else  /* defined(HAVE_OPENGL) */

int X11DRV_ChoosePixelFormat(DC *dc,
			     const PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");

  return 0;
}

int X11DRV_DescribePixelFormat(DC *dc,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");
  
  return 0;
}

int X11DRV_GetPixelFormat(DC *dc) {
  ERR("No OpenGL support compiled in.\n");
  
  return 0;
}

BOOL X11DRV_SetPixelFormat(DC *dc,
			   int iPixelFormat,
			   const PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");
  
  return FALSE;
}

BOOL X11DRV_SwapBuffers(DC *dc) {
  ERR("No OpenGL support compiled in.\n");
  
  return FALSE;
}

#endif /* defined(HAVE_OPENGL) */
