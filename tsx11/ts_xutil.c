/*
 * Thread safe wrappers around Xutil calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include "x11drv.h"
#include "stddebug.h"
#include "debug.h"

XClassHint * TSXAllocClassHint(void)
{
  XClassHint * r;
  dprintf_x11(stddeb, "Call XAllocClassHint\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocClassHint();
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XAllocClassHint\n");
  return r;
}

XSizeHints * TSXAllocSizeHints(void)
{
  XSizeHints * r;
  dprintf_x11(stddeb, "Call XAllocSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocSizeHints();
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XAllocSizeHints\n");
  return r;
}

XWMHints * TSXAllocWMHints(void)
{
  XWMHints * r;
  dprintf_x11(stddeb, "Call XAllocWMHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAllocWMHints();
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XAllocWMHints\n");
  return r;
}

int  TSXClipBox(Region a0, XRectangle* a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XClipBox\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XClipBox(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XClipBox\n");
  return r;
}

Region  TSXCreateRegion(void)
{
  Region  r;
  dprintf_x11(stddeb, "Call XCreateRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XCreateRegion();
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XCreateRegion\n");
  return r;
}

int  TSXDeleteContext(Display* a0, XID a1, XContext a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XDeleteContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDeleteContext(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XDeleteContext\n");
  return r;
}

int  TSXDestroyRegion(Region a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XDestroyRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDestroyRegion(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XDestroyRegion\n");
  return r;
}

int  TSXEmptyRegion(Region a0)
{
  int  r;
  dprintf_x11(stddeb, "Call XEmptyRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XEmptyRegion(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XEmptyRegion\n");
  return r;
}

int  TSXEqualRegion(Region a0, Region a1)
{
  int  r;
  dprintf_x11(stddeb, "Call XEqualRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XEqualRegion(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XEqualRegion\n");
  return r;
}

int  TSXFindContext(Display* a0, XID a1, XContext a2, XPointer* a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XFindContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XFindContext(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XFindContext\n");
  return r;
}

XVisualInfo * TSXGetVisualInfo(Display* a0, long a1, XVisualInfo* a2, int* a3)
{
  XVisualInfo * r;
  dprintf_x11(stddeb, "Call XGetVisualInfo\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetVisualInfo(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XGetVisualInfo\n");
  return r;
}

int   TSXGetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, long* a3, Atom a4)
{
  int   r;
  dprintf_x11(stddeb, "Call XGetWMSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XGetWMSizeHints(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XGetWMSizeHints\n");
  return r;
}

int  TSXIntersectRegion(Region a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XIntersectRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XIntersectRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XIntersectRegion\n");
  return r;
}

int  TSXLookupString(XKeyEvent* a0, char* a1, int a2, KeySym* a3, XComposeStatus* a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XLookupString\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XLookupString(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XLookupString\n");
  return r;
}

int  TSXOffsetRegion(Region a0, int a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XOffsetRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XOffsetRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XOffsetRegion\n");
  return r;
}

int   TSXPointInRegion(Region a0, int a1, int a2)
{
  int   r;
  dprintf_x11(stddeb, "Call XPointInRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPointInRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XPointInRegion\n");
  return r;
}

Region  TSXPolygonRegion(XPoint* a0, int a1, int a2)
{
  Region  r;
  dprintf_x11(stddeb, "Call XPolygonRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XPolygonRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XPolygonRegion\n");
  return r;
}

int  TSXRectInRegion(Region a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  int  r;
  dprintf_x11(stddeb, "Call XRectInRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XRectInRegion(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XRectInRegion\n");
  return r;
}

int  TSXSaveContext(Display* a0, XID a1, XContext a2, const  char* a3)
{
  int  r;
  dprintf_x11(stddeb, "Call XSaveContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSaveContext(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XSaveContext\n");
  return r;
}

void  TSXSetWMProperties(Display* a0, Window a1, XTextProperty* a2, XTextProperty* a3, char** a4, int a5, XSizeHints* a6, XWMHints* a7, XClassHint* a8)
{
  dprintf_x11(stddeb, "Call XSetWMProperties\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XSetWMProperties(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XSetWMProperties\n");
}

void  TSXSetWMSizeHints(Display* a0, Window a1, XSizeHints* a2, Atom a3)
{
  dprintf_x11(stddeb, "Call XSetWMSizeHints\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XSetWMSizeHints(a0, a1, a2, a3);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XSetWMSizeHints\n");
}

int  TSXSetRegion(Display* a0, GC a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSetRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSetRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XSetRegion\n");
  return r;
}

int  TSXShrinkRegion(Region a0, int a1, int a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XShrinkRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XShrinkRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XShrinkRegion\n");
  return r;
}

int   TSXStringListToTextProperty(char** a0, int a1, XTextProperty* a2)
{
  int   r;
  dprintf_x11(stddeb, "Call XStringListToTextProperty\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XStringListToTextProperty(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XStringListToTextProperty\n");
  return r;
}

int  TSXSubtractRegion(Region a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XSubtractRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSubtractRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XSubtractRegion\n");
  return r;
}

int  TSXUnionRectWithRegion(XRectangle* a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XUnionRectWithRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUnionRectWithRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XUnionRectWithRegion\n");
  return r;
}

int  TSXUnionRegion(Region a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XUnionRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUnionRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XUnionRegion\n");
  return r;
}

int  TSXXorRegion(Region a0, Region a1, Region a2)
{
  int  r;
  dprintf_x11(stddeb, "Call XXorRegion\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XXorRegion(a0, a1, a2);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XXorRegion\n");
  return r;
}

int TSXDestroyImage(struct _XImage *a0)
{
  int r;
  dprintf_x11(stddeb, "Call XDestroyImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XDestroyImage(a0);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XDestroyImage\n");
  return r;
}

struct _XImage * TSXSubImage(struct _XImage *a0, int a1, int a2, unsigned int a3, unsigned int a4)
{
  struct _XImage * r;
  dprintf_x11(stddeb, "Call XSubImage\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XSubImage(a0, a1, a2, a3, a4);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XSubImage\n");
  return r;
}

int TSXAddPixel(struct _XImage *a0, long a1)
{
  int r;
  dprintf_x11(stddeb, "Call XAddPixel\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XAddPixel(a0, a1);
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XAddPixel\n");
  return r;
}

XContext TSXUniqueContext(void)
{
  XContext r;
  dprintf_x11(stddeb, "Call XUniqueContext\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XUniqueContext();
  LeaveCriticalSection( &X11DRV_CritSection );
  dprintf_x11(stddeb, "Ret XUniqueContext\n");
  return r;
}
