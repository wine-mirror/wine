/*
 * Thread safe wrappers around xpm calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXXPM

#include <X11/xpm.h>

#include "ts_xpm.h"


int TSXpmCreatePixmapFromData(Display *a0, Drawable a1, char **a2, Pixmap *a3, Pixmap *a4, XpmAttributes *a5)
{
  int r;
  wine_tsx11_lock();
  r = XpmCreatePixmapFromData(a0, a1, a2, a3, a4, a5);
  wine_tsx11_unlock();
  return r;
}

int TSXpmAttributesSize(void)
{
  int r;
  wine_tsx11_lock();
  r = XpmAttributesSize();
  wine_tsx11_unlock();
  return r;
}

#endif /* defined(HAVE_LIBXXPM) */

