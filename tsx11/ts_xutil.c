/*
 * Thread safe wrappers around Xutil calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING


#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include "debug.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(x11)

XClassHint * TSXAllocClassHint(void)
{
  XClassHint * r;
  TRACE(x11, "Call XAllocClassHint\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocClassHint();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XAllocClassHint\n");
  return r;
}

XSizeHints * TSXAllocSizeHints(void)
{
  XSizeHints * r;
  TRACE(x11, "Call XAllocSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocSizeHints();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XAllocSizeHints\n");
  return r;
}

XWMHints * TSXAllocWMHints(void)
{
  XWMHints * r;
  TRACE(x11, "Call XAllocWMHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocWMHints();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XAllocWMHints\n");
  return r;
}

int  TSXClipBox(Region a0, XRectangle* a1)
{
  int  r;
  TRACE(x11, "Call XClipBox\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XClipBox(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XClipBox\n");
  return r;
}

Region  TSXCreateRegion(void)
{
  Region  r;
  TRACE(x11, "Call XCreateRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateRegion();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XCreateRegion\n");
  return r;
}

int  TSXDeleteContext(Display* a0, XID a1, XContext a2)
{
  int  r;
  TRACE(x11, "Call XDeleteContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDeleteContext(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDeleteContext\n");
  return r;
}

int  TSXDestroyRegion(Region a0)
{
  int  r;
  TRACE(x11, "Call XDestroyRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDestroyRegion(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDestroyRegion\n");
  return r;
}

int  TSXEmptyRegion(Region a0)
{
  int  r;
  TRACE(x11, "Call XEmptyRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XEmptyRegion(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XEmptyRegion\n");
  return r;
}

int  TSXEqualRegion(Region a0, Region a1)
{
  int  r;
  TRACE(x11, "Call XEqualRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XEqualRegion(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XEqualRegion\n");
  return r;
}

int  TSXFindContext(Display* a0, XID a1, XContext a2, XPointer* a3)
{
  int  r;
  TRACE(x11, "Call XFindContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFindContext(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XFindContext\n");
  return r;
}

XVisualInfo * TSXGetVisualInfo(Display* a0, long a1, XVisualInfo* a2, int* a3)
{
  XVisualInfo * r;
  TRACE(x11, "Call XGetVisualInfo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetVisualInfo(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetVisualInfo\n");
  return r;
}

int   TSXGetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, long* a3, Atom a4)
{
  int   r;
  TRACE(x11, "Call XGetWMSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetWMSizeHints(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XGetWMSizeHints\n");
  return r;
}

int  TSXIntersectRegion(Region a0, Region a1, Region a2)
{
  int  r;
  TRACE(x11, "Call XIntersectRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XIntersectRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XIntersectRegion\n");
  return r;
}

int  TSXLookupString(XKeyEvent* a0, char* a1, int a2, KeySym* a3, XComposeStatus* a4)
{
  int  r;
  TRACE(x11, "Call XLookupString\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XLookupString(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XLookupString\n");
  return r;
}

int  TSXOffsetRegion(Region a0, int a1, int a2)
{
  int  r;
  TRACE(x11, "Call XOffsetRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XOffsetRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XOffsetRegion\n");
  return r;
}

int   TSXPointInRegion(Region a0, int a1, int a2)
{
  int   r;
  TRACE(x11, "Call XPointInRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPointInRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XPointInRegion\n");
  return r;
}

Region  TSXPolygonRegion(XPoint* a0, int a1, int a2)
{
  Region  r;
  TRACE(x11, "Call XPolygonRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPolygonRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XPolygonRegion\n");
  return r;
}

int  TSXRectInRegion(Region a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  int  r;
  TRACE(x11, "Call XRectInRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XRectInRegion(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XRectInRegion\n");
  return r;
}

int  TSXSaveContext(Display* a0, XID a1, XContext a2, const  char* a3)
{
  int  r;
  TRACE(x11, "Call XSaveContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSaveContext(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSaveContext\n");
  return r;
}

int  TSXSetClassHint(Display* a0, Window a1, XClassHint* a2)
{
  int  r;
  TRACE(x11, "Call XSetClassHint\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetClassHint(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetClassHint\n");
  return r;
}

int  TSXSetWMHints(Display* a0, Window a1, XWMHints* a2)
{
  int  r;
  TRACE(x11, "Call XSetWMHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetWMHints(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetWMHints\n");
  return r;
}

void  TSXSetWMProperties(Display* a0, Window a1, XTextProperty* a2, XTextProperty* a3, char** a4, int a5, XSizeHints* a6, XWMHints* a7, XClassHint* a8)
{
  TRACE(x11, "Call XSetWMProperties\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XSetWMProperties(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetWMProperties\n");
}

void  TSXSetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, Atom a3)
{
  TRACE(x11, "Call XSetWMSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XSetWMSizeHints(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetWMSizeHints\n");
}

int  TSXSetRegion(Display* a0, GC a1, Region a2)
{
  int  r;
  TRACE(x11, "Call XSetRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSetRegion\n");
  return r;
}

int  TSXShrinkRegion(Region a0, int a1, int a2)
{
  int  r;
  TRACE(x11, "Call XShrinkRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShrinkRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XShrinkRegion\n");
  return r;
}

int   TSXStringListToTextProperty(char** a0, int a1, XTextProperty* a2)
{
  int   r;
  TRACE(x11, "Call XStringListToTextProperty\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XStringListToTextProperty(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XStringListToTextProperty\n");
  return r;
}

int  TSXSubtractRegion(Region a0, Region a1, Region a2)
{
  int  r;
  TRACE(x11, "Call XSubtractRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSubtractRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSubtractRegion\n");
  return r;
}

int  TSXUnionRectWithRegion(XRectangle* a0, Region a1, Region a2)
{
  int  r;
  TRACE(x11, "Call XUnionRectWithRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUnionRectWithRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XUnionRectWithRegion\n");
  return r;
}

int  TSXUnionRegion(Region a0, Region a1, Region a2)
{
  int  r;
  TRACE(x11, "Call XUnionRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUnionRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XUnionRegion\n");
  return r;
}

int  TSXXorRegion(Region a0, Region a1, Region a2)
{
  int  r;
  TRACE(x11, "Call XXorRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XXorRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XXorRegion\n");
  return r;
}

int TSXDestroyImage(struct _XImage *a0)
{
  int r;
  TRACE(x11, "Call XDestroyImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDestroyImage(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XDestroyImage\n");
  return r;
}

struct _XImage * TSXSubImage(struct _XImage *a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  struct _XImage * r;
  TRACE(x11, "Call XSubImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSubImage(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XSubImage\n");
  return r;
}

int TSXAddPixel(struct _XImage *a0, long a1)
{
  int r;
  TRACE(x11, "Call XAddPixel\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAddPixel(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XAddPixel\n");
  return r;
}

XContext TSXUniqueContext(void)
{
  XContext r;
  TRACE(x11, "Call XUniqueContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUniqueContext();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE(x11, "Ret XUniqueContext\n");
  return r;
}


#endif /* !defined(X_DISPLAY_MISSING) */
