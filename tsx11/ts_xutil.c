/*
 * Thread safe wrappers around Xutil calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"


#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "ts_xutil.h"


XClassHint * TSXAllocClassHint(void)
{
  XClassHint * r;
  wine_tsx11_lock();
  r = XAllocClassHint();
  wine_tsx11_unlock();
  return r;
}

XSizeHints * TSXAllocSizeHints(void)
{
  XSizeHints * r;
  wine_tsx11_lock();
  r = XAllocSizeHints();
  wine_tsx11_unlock();
  return r;
}

XWMHints * TSXAllocWMHints(void)
{
  XWMHints * r;
  wine_tsx11_lock();
  r = XAllocWMHints();
  wine_tsx11_unlock();
  return r;
}

int  TSXClipBox(Region a0, XRectangle* a1)
{
  int  r;
  wine_tsx11_lock();
  r = XClipBox(a0, a1);
  wine_tsx11_unlock();
  return r;
}

Region  TSXCreateRegion(void)
{
  Region  r;
  wine_tsx11_lock();
  r = XCreateRegion();
  wine_tsx11_unlock();
  return r;
}

int  TSXDeleteContext(Display* a0, XID a1, XContext a2)
{
  int  r;
  wine_tsx11_lock();
  r = XDeleteContext(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXDestroyRegion(Region a0)
{
  int  r;
  wine_tsx11_lock();
  r = XDestroyRegion(a0);
  wine_tsx11_unlock();
  return r;
}

int  TSXEmptyRegion(Region a0)
{
  int  r;
  wine_tsx11_lock();
  r = XEmptyRegion(a0);
  wine_tsx11_unlock();
  return r;
}

int  TSXEqualRegion(Region a0, Region a1)
{
  int  r;
  wine_tsx11_lock();
  r = XEqualRegion(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int  TSXFindContext(Display* a0, XID a1, XContext a2, XPointer* a3)
{
  int  r;
  wine_tsx11_lock();
  r = XFindContext(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

XVisualInfo * TSXGetVisualInfo(Display* a0, long a1, XVisualInfo* a2, int* a3)
{
  XVisualInfo * r;
  wine_tsx11_lock();
  r = XGetVisualInfo(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

XWMHints * TSXGetWMHints(Display* a0, Window a1)
{
  XWMHints * r;
  wine_tsx11_lock();
  r = XGetWMHints(a0, a1);
  wine_tsx11_unlock();
  return r;
}

int   TSXGetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, long* a3, Atom a4)
{
  int   r;
  wine_tsx11_lock();
  r = XGetWMSizeHints(a0, a1, a2, a3, a4);
  wine_tsx11_unlock();
  return r;
}

int  TSXIntersectRegion(Region a0, Region a1, Region a2)
{
  int  r;
  wine_tsx11_lock();
  r = XIntersectRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXLookupString(XKeyEvent* a0, char* a1, int a2, KeySym* a3, XComposeStatus* a4)
{
  int  r;
  wine_tsx11_lock();
  r = XLookupString(a0, a1, a2, a3, a4);
  wine_tsx11_unlock();
  return r;
}

int  TSXOffsetRegion(Region a0, int a1, int a2)
{
  int  r;
  wine_tsx11_lock();
  r = XOffsetRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int   TSXPointInRegion(Region a0, int a1, int a2)
{
  int   r;
  wine_tsx11_lock();
  r = XPointInRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

Region  TSXPolygonRegion(XPoint* a0, int a1, int a2)
{
  Region  r;
  wine_tsx11_lock();
  r = XPolygonRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXRectInRegion(Region a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  int  r;
  wine_tsx11_lock();
  r = XRectInRegion(a0, a1, a2, a3, a4);
  wine_tsx11_unlock();
  return r;
}

int  TSXSaveContext(Display* a0, XID a1, XContext a2, const  char* a3)
{
  int  r;
  wine_tsx11_lock();
  r = XSaveContext(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetClassHint(Display* a0, Window a1, XClassHint* a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetClassHint(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXSetWMHints(Display* a0, Window a1, XWMHints* a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetWMHints(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

void  TSXSetWMProperties(Display* a0, Window a1, XTextProperty* a2, XTextProperty* a3, char** a4, int a5, XSizeHints* a6, XWMHints* a7, XClassHint* a8)
{
  wine_tsx11_lock();
  XSetWMProperties(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
}

void  TSXSetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, Atom a3)
{
  wine_tsx11_lock();
  XSetWMSizeHints(a0, a1, a2, a3);
  wine_tsx11_unlock();
}

int  TSXSetRegion(Display* a0, GC a1, Region a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXShrinkRegion(Region a0, int a1, int a2)
{
  int  r;
  wine_tsx11_lock();
  r = XShrinkRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int   TSXStringListToTextProperty(char** a0, int a1, XTextProperty* a2)
{
  int   r;
  wine_tsx11_lock();
  r = XStringListToTextProperty(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXSubtractRegion(Region a0, Region a1, Region a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSubtractRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXUnionRectWithRegion(XRectangle* a0, Region a1, Region a2)
{
  int  r;
  wine_tsx11_lock();
  r = XUnionRectWithRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXUnionRegion(Region a0, Region a1, Region a2)
{
  int  r;
  wine_tsx11_lock();
  r = XUnionRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int  TSXXorRegion(Region a0, Region a1, Region a2)
{
  int  r;
  wine_tsx11_lock();
  r = XXorRegion(a0, a1, a2);
  wine_tsx11_unlock();
  return r;
}

int TSXDestroyImage(struct _XImage *a0)
{
  int r;
  wine_tsx11_lock();
  r = XDestroyImage(a0);
  wine_tsx11_unlock();
  return r;
}

struct _XImage * TSXSubImage(struct _XImage *a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  struct _XImage * r;
  wine_tsx11_lock();
  r = XSubImage(a0, a1, a2, a3, a4);
  wine_tsx11_unlock();
  return r;
}

int TSXAddPixel(struct _XImage *a0, long a1)
{
  int r;
  wine_tsx11_lock();
  r = XAddPixel(a0, a1);
  wine_tsx11_unlock();
  return r;
}

XContext TSXUniqueContext(void)
{
  XContext r;
  wine_tsx11_lock();
  r = XUniqueContext();
  wine_tsx11_unlock();
  return r;
}


