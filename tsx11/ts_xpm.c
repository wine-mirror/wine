/*
 * Thread safe wrappers around xpm calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXXPM

#include <X11/xpm.h>

#include "debugtools.h"
#include "ts_xpm.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(x11)

int TSXpmCreatePixmapFromData(Display *a0, Drawable a1, char **a2, Pixmap *a3, Pixmap *a4, XpmAttributes *a5)
{
  int r;
  TRACE("Call XpmCreatePixmapFromData\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XpmCreatePixmapFromData(a0, a1, a2, a3, a4, a5);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XpmCreatePixmapFromData\n");
  return r;
}

int TSXpmAttributesSize(void)
{
  int r;
  TRACE("Call XpmAttributesSize\n");
  EnterCriticalSection( &X11DRV_CritSection );
  r = XpmAttributesSize();
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XpmAttributesSize\n");
  return r;
}

#endif /* defined(HAVE_LIBXXPM) */

