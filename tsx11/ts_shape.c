/*
 * Thread safe wrappers around shape calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXSHAPE
#include <X11/IntrinsicP.h>

#include <X11/extensions/shape.h>

#include "ts_shape.h"


void  TSXShapeCombineRectangles(Display* a0, Window a1, int a2, int a3, int a4, XRectangle* a5, int a6, int a7, int a8)
{
  wine_tsx11_lock();
  XShapeCombineRectangles(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  wine_tsx11_unlock();
}

void  TSXShapeCombineMask(Display* a0, Window a1, int a2, int a3, int a4, Pixmap a5, int a6)
{
  wine_tsx11_lock();
  XShapeCombineMask(a0, a1, a2, a3, a4, a5, a6);
  wine_tsx11_unlock();
}

#endif /* defined(HAVE_LIBXSHAPE) */

