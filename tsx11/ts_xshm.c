/*
 * Thread safe wrappers around XShm calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXXSHM

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

#include "ts_xshm.h"


Bool TSXShmQueryExtension(Display *a0)
{
  Bool r;
  wine_tsx11_lock();
  r = XShmQueryExtension(a0);
  wine_tsx11_unlock();
  return r;
}

Bool TSXShmQueryVersion(Display *a0, int *a1, int *a2, Bool *a3)
{
  Bool r;
  wine_tsx11_lock();
  r = XShmQueryVersion(a0, a1, a2, a3);
  wine_tsx11_unlock();
  return r;
}

int TSXShmPixmapFormat(Display *a0)
{
  int r;
  wine_tsx11_lock();
  r = XShmPixmapFormat(a0);
  wine_tsx11_unlock();
  return r;
}

Status TSXShmAttach(Display *a0, XShmSegmentInfo *a1)
{
  Status r;
  wine_tsx11_lock();
  r = XShmAttach(a0, a1);
  wine_tsx11_unlock();
  return r;
}

Status TSXShmDetach(Display *a0, XShmSegmentInfo *a1)
{
  Status r;
  wine_tsx11_lock();
  r = XShmDetach(a0, a1);
  wine_tsx11_unlock();
  return r;
}

Status TSXShmPutImage(Display *a0, Drawable a1, GC a2, XImage *a3, int a4, int a5, int a6, int a7, unsigned int a8, unsigned int a9, Bool a10)
{
  Status r;
  wine_tsx11_lock();
  r = XShmPutImage(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  wine_tsx11_unlock();
  return r;
}

Status TSXShmGetImage(Display *a0, Drawable a1, XImage *a2, int a3, int a4, unsigned long a5)
{
  Status r;
  wine_tsx11_lock();
  r = XShmGetImage(a0, a1, a2, a3, a4, a5);
  wine_tsx11_unlock();
  return r;
}

XImage * TSXShmCreateImage(Display *a0, Visual *a1, unsigned int a2, int a3, char *a4, XShmSegmentInfo *a5, unsigned int a6, unsigned int a7)
{
  XImage * r;
  wine_tsx11_lock();
  r = XShmCreateImage(a0, a1, a2, a3, a4, a5, a6, a7);
  wine_tsx11_unlock();
  return r;
}

Pixmap TSXShmCreatePixmap(Display *a0, Drawable a1, char *a2, XShmSegmentInfo *a3, unsigned int a4, unsigned int a5, unsigned int a6)
{
  Pixmap r;
  wine_tsx11_lock();
  r = XShmCreatePixmap(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
  return r;
}

#endif /* defined(HAVE_LIBXXSHM) */

