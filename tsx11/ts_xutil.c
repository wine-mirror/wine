/*
 * Thread safe wrappers around Xutil calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"


#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "debugtools.h"
#include "ts_xutil.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(x11)

XClassHint * TSXAllocClassHint(void)
{
  XClassHint * r;
  TRACE("Call XAllocClassHint\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocClassHint();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XAllocClassHint\n");
  return r;
}

XSizeHints * TSXAllocSizeHints(void)
{
  XSizeHints * r;
  TRACE("Call XAllocSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocSizeHints();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XAllocSizeHints\n");
  return r;
}

XWMHints * TSXAllocWMHints(void)
{
  XWMHints * r;
  TRACE("Call XAllocWMHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocWMHints();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XAllocWMHints\n");
  return r;
}

int  TSXClipBox(Region a0, XRectangle* a1)
{
  int  r;
  TRACE("Call XClipBox\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XClipBox(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XClipBox\n");
  return r;
}

Region  TSXCreateRegion(void)
{
  Region  r;
  TRACE("Call XCreateRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateRegion();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XCreateRegion\n");
  return r;
}

int  TSXDeleteContext(Display* a0, XID a1, XContext a2)
{
  int  r;
  TRACE("Call XDeleteContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDeleteContext(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDeleteContext\n");
  return r;
}

int  TSXDestroyRegion(Region a0)
{
  int  r;
  TRACE("Call XDestroyRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDestroyRegion(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDestroyRegion\n");
  return r;
}

int  TSXEmptyRegion(Region a0)
{
  int  r;
  TRACE("Call XEmptyRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XEmptyRegion(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XEmptyRegion\n");
  return r;
}

int  TSXEqualRegion(Region a0, Region a1)
{
  int  r;
  TRACE("Call XEqualRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XEqualRegion(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XEqualRegion\n");
  return r;
}

int  TSXFindContext(Display* a0, XID a1, XContext a2, XPointer* a3)
{
  int  r;
  TRACE("Call XFindContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFindContext(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XFindContext\n");
  return r;
}

XVisualInfo * TSXGetVisualInfo(Display* a0, long a1, XVisualInfo* a2, int* a3)
{
  XVisualInfo * r;
  TRACE("Call XGetVisualInfo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetVisualInfo(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XGetVisualInfo\n");
  return r;
}

XWMHints * TSXGetWMHints(Display* a0, Window a1)
{
  XWMHints * r;
  TRACE("Call XGetWMHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetWMHints(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XGetWMHints\n");
  return r;
}

int   TSXGetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, long* a3, Atom a4)
{
  int   r;
  TRACE("Call XGetWMSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetWMSizeHints(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XGetWMSizeHints\n");
  return r;
}

int  TSXIntersectRegion(Region a0, Region a1, Region a2)
{
  int  r;
  TRACE("Call XIntersectRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XIntersectRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XIntersectRegion\n");
  return r;
}

int  TSXLookupString(XKeyEvent* a0, char* a1, int a2, KeySym* a3, XComposeStatus* a4)
{
  int  r;
  TRACE("Call XLookupString\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XLookupString(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XLookupString\n");
  return r;
}

int  TSXOffsetRegion(Region a0, int a1, int a2)
{
  int  r;
  TRACE("Call XOffsetRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XOffsetRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XOffsetRegion\n");
  return r;
}

int   TSXPointInRegion(Region a0, int a1, int a2)
{
  int   r;
  TRACE("Call XPointInRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPointInRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XPointInRegion\n");
  return r;
}

Region  TSXPolygonRegion(XPoint* a0, int a1, int a2)
{
  Region  r;
  TRACE("Call XPolygonRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPolygonRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XPolygonRegion\n");
  return r;
}

int  TSXRectInRegion(Region a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  int  r;
  TRACE("Call XRectInRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XRectInRegion(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XRectInRegion\n");
  return r;
}

int  TSXSaveContext(Display* a0, XID a1, XContext a2, const  char* a3)
{
  int  r;
  TRACE("Call XSaveContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSaveContext(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XSaveContext\n");
  return r;
}

int  TSXSetClassHint(Display* a0, Window a1, XClassHint* a2)
{
  int  r;
  TRACE("Call XSetClassHint\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetClassHint(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XSetClassHint\n");
  return r;
}

int  TSXSetWMHints(Display* a0, Window a1, XWMHints* a2)
{
  int  r;
  TRACE("Call XSetWMHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetWMHints(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XSetWMHints\n");
  return r;
}

void  TSXSetWMProperties(Display* a0, Window a1, XTextProperty* a2, XTextProperty* a3, char** a4, int a5, XSizeHints* a6, XWMHints* a7, XClassHint* a8)
{
  TRACE("Call XSetWMProperties\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XSetWMProperties(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XSetWMProperties\n");
}

void  TSXSetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, Atom a3)
{
  TRACE("Call XSetWMSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XSetWMSizeHints(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XSetWMSizeHints\n");
}

int  TSXSetRegion(Display* a0, GC a1, Region a2)
{
  int  r;
  TRACE("Call XSetRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XSetRegion\n");
  return r;
}

int  TSXShrinkRegion(Region a0, int a1, int a2)
{
  int  r;
  TRACE("Call XShrinkRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShrinkRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XShrinkRegion\n");
  return r;
}

int   TSXStringListToTextProperty(char** a0, int a1, XTextProperty* a2)
{
  int   r;
  TRACE("Call XStringListToTextProperty\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XStringListToTextProperty(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XStringListToTextProperty\n");
  return r;
}

int  TSXSubtractRegion(Region a0, Region a1, Region a2)
{
  int  r;
  TRACE("Call XSubtractRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSubtractRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XSubtractRegion\n");
  return r;
}

int  TSXUnionRectWithRegion(XRectangle* a0, Region a1, Region a2)
{
  int  r;
  TRACE("Call XUnionRectWithRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUnionRectWithRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XUnionRectWithRegion\n");
  return r;
}

int  TSXUnionRegion(Region a0, Region a1, Region a2)
{
  int  r;
  TRACE("Call XUnionRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUnionRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XUnionRegion\n");
  return r;
}

int  TSXXorRegion(Region a0, Region a1, Region a2)
{
  int  r;
  TRACE("Call XXorRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XXorRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XXorRegion\n");
  return r;
}

int TSXDestroyImage(struct _XImage *a0)
{
  int r;
  TRACE("Call XDestroyImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDestroyImage(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XDestroyImage\n");
  return r;
}

struct _XImage * TSXSubImage(struct _XImage *a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  struct _XImage * r;
  TRACE("Call XSubImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSubImage(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XSubImage\n");
  return r;
}

int TSXAddPixel(struct _XImage *a0, long a1)
{
  int r;
  TRACE("Call XAddPixel\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAddPixel(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XAddPixel\n");
  return r;
}

XContext TSXUniqueContext(void)
{
  XContext r;
  TRACE("Call XUniqueContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUniqueContext();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XUniqueContext\n");
  return r;
}


