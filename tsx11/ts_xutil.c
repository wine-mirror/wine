/*
 * Thread safe wrappers around Xutil calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_X11_XLIB_H

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "ts_xutil.h"


XWMHints * TSXAllocWMHints(void)
{
  XWMHints * r;
  wine_tsx11_lock();
  r = XAllocWMHints();
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

XWMHints * TSXGetWMHints(Display* a0, Window a1)
{
  XWMHints * r;
  wine_tsx11_lock();
  r = XGetWMHints(a0, a1);
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

int  TSXSetWMHints(Display* a0, Window a1, XWMHints* a2)
{
  int  r;
  wine_tsx11_lock();
  r = XSetWMHints(a0, a1, a2);
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

#endif /* defined(HAVE_X11_XLIB_H) */

