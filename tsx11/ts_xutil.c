/*
 * Thread safe wrappers around Xutil calls.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include "tsx11defs.h"
#include "stddebug.h"
#include "debug.h"

XClassHint * TSXAllocClassHint(void)
{
  XClassHint * r;
  dprintf_x11(stddeb, "Call XAllocClassHint\n");
  X11_LOCK();
  r = XAllocClassHint();
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XAllocClassHint\n");
  return r;
}

XSizeHints * TSXAllocSizeHints(void)
{
  XSizeHints * r;
  dprintf_x11(stddeb, "Call XAllocSizeHints\n");
  X11_LOCK();
  r = XAllocSizeHints();
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XAllocSizeHints\n");
  return r;
}

XWMHints * TSXAllocWMHints(void)
{
  XWMHints * r;
  dprintf_x11(stddeb, "Call XAllocWMHints\n");
  X11_LOCK();
  r = XAllocWMHints();
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XAllocWMHints\n");
  return r;
}

int  TSXClipBox(Region a0, XRectangle* a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XClipBox\n");
  X11_LOCK();
  r = XClipBox(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XClipBox\n");
  return r;
}

Region  TSXCreateRegion(void)
{
  Region  r;
  dprintf_x11(stddeb, "Call XCreateRegion\n");
  X11_LOCK();
  r = XCreateRegion();
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XCreateRegion\n");
  return r;
}

int  TSXDeleteContext(Display* a0, XID a1, XContext a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XDeleteContext\n");
  X11_LOCK();
  r = XDeleteContext(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDeleteContext\n");
  return r;
}

int  TSXDestroyRegion(Region a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XDestroyRegion\n");
  X11_LOCK();
  r = XDestroyRegion(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDestroyRegion\n");
  return r;
}

int  TSXEmptyRegion(Region a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XEmptyRegion\n");
  X11_LOCK();
  r = XEmptyRegion(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XEmptyRegion\n");
  return r;
}

int  TSXEqualRegion(Region a0, Region a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XEqualRegion\n");
  X11_LOCK();
  r = XEqualRegion(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XEqualRegion\n");
  return r;
}

int  TSXFindContext(Display* a0, XID a1, XContext a2, XPointer* a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XFindContext\n");
  X11_LOCK();
  r = XFindContext(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XFindContext\n");
  return r;
}

int   TSXGetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, long* a3, Atom a4)
{
  int   r;
  dprintf_x11(stddeb, "Call XGetWMSizeHints\n");
  X11_LOCK();
  r = XGetWMSizeHints(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetWMSizeHints\n");
  return r;
}

int  TSXIntersectRegion(Region a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XIntersectRegion\n");
  X11_LOCK();
  r = XIntersectRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XIntersectRegion\n");
  return r;
}

int  TSXLookupString(XKeyEvent* a0, char* a1, int a2, KeySym* a3, XComposeStatus* a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XLookupString\n");
  X11_LOCK();
  r = XLookupString(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XLookupString\n");
  return r;
}

int  TSXOffsetRegion(Region a0, int a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XOffsetRegion\n");
  X11_LOCK();
  r = XOffsetRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XOffsetRegion\n");
  return r;
}

int   TSXPointInRegion(Region a0, int a1, int a2)
{
  int   r;
  dprintf_x11(stddeb, "Call XPointInRegion\n");
  X11_LOCK();
  r = XPointInRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XPointInRegion\n");
  return r;
}

Region  TSXPolygonRegion(XPoint* a0, int a1, int a2)
{
  Region  r;
  dprintf_x11(stddeb, "Call XPolygonRegion\n");
  X11_LOCK();
  r = XPolygonRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XPolygonRegion\n");
  return r;
}

int  TSXRectInRegion(Region a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XRectInRegion\n");
  X11_LOCK();
  r = XRectInRegion(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XRectInRegion\n");
  return r;
}

int  TSXSaveContext(Display* a0, XID a1, XContext a2, const  char* a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XSaveContext\n");
  X11_LOCK();
  r = XSaveContext(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSaveContext\n");
  return r;
}

void  TSXSetWMProperties(Display* a0, Window a1, XTextProperty* a2, XTextProperty* a3, char** a4, int a5, XSizeHints* a6, XWMHints* a7, XClassHint* a8)
{
  dprintf_x11(stddeb, "Call XSetWMProperties\n");
  X11_LOCK();
  XSetWMProperties(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetWMProperties\n");
}

void  TSXSetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, Atom a3)
{
  dprintf_x11(stddeb, "Call XSetWMSizeHints\n");
  X11_LOCK();
  XSetWMSizeHints(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetWMSizeHints\n");
}

int  TSXSetRegion(Display* a0, GC a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetRegion\n");
  X11_LOCK();
  r = XSetRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSetRegion\n");
  return r;
}

int  TSXShrinkRegion(Region a0, int a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XShrinkRegion\n");
  X11_LOCK();
  r = XShrinkRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XShrinkRegion\n");
  return r;
}

int   TSXStringListToTextProperty(char** a0, int a1, XTextProperty* a2)
{
  int   r;
  dprintf_x11(stddeb, "Call XStringListToTextProperty\n");
  X11_LOCK();
  r = XStringListToTextProperty(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XStringListToTextProperty\n");
  return r;
}

int  TSXSubtractRegion(Region a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSubtractRegion\n");
  X11_LOCK();
  r = XSubtractRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSubtractRegion\n");
  return r;
}

int  TSXUnionRectWithRegion(XRectangle* a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XUnionRectWithRegion\n");
  X11_LOCK();
  r = XUnionRectWithRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XUnionRectWithRegion\n");
  return r;
}

int  TSXUnionRegion(Region a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XUnionRegion\n");
  X11_LOCK();
  r = XUnionRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XUnionRegion\n");
  return r;
}

int  TSXXorRegion(Region a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XXorRegion\n");
  X11_LOCK();
  r = XXorRegion(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XXorRegion\n");
  return r;
}

int TSXDestroyImage(struct _XImage *a0)
{
  int r;
  dprintf_x11(stddeb, "Call XDestroyImage\n");
  X11_LOCK();
  r = XDestroyImage(a0);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XDestroyImage\n");
  return r;
}

unsigned long TSXGetPixel(struct _XImage *a0, int a1, int a2)
{
  unsigned long r;
  dprintf_x11(stddeb, "Call XGetPixel\n");
  X11_LOCK();
  r = XGetPixel(a0, a1, a2);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XGetPixel\n");
  return r;
}

int TSXPutPixel(struct _XImage *a0, int a1, int a2, unsigned long a3)
{
  int r;
  dprintf_x11(stddeb, "Call XPutPixel\n");
  X11_LOCK();
  r = XPutPixel(a0, a1, a2, a3);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XPutPixel\n");
  return r;
}

struct _XImage * TSXSubImage(struct _XImage *a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  struct _XImage * r;
  dprintf_x11(stddeb, "Call XSubImage\n");
  X11_LOCK();
  r = XSubImage(a0, a1, a2, a3, a4);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XSubImage\n");
  return r;
}

int TSXAddPixel(struct _XImage *a0, long a1)
{
  int r;
  dprintf_x11(stddeb, "Call XAddPixel\n");
  X11_LOCK();
  r = XAddPixel(a0, a1);
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XAddPixel\n");
  return r;
}

XContext TSXUniqueContext(void)
{
  XContext r;
  dprintf_x11(stddeb, "Call XUniqueContext\n");
  X11_LOCK();
  r = XUniqueContext();
  X11_UNLOCK();
  dprintf_x11(stddeb, "Ret XUniqueContext\n");
  return r;
}
