/*
 * Thread safe wrappers around shape calls.
 * This file was generated automatically by tools/make_X11wrappers
 * DO NOT EDIT!
 */

#include "config.h"

#ifdef HAVE_LIBXSHAPE
#include <X11/IntrinsicP.h>

#include <X11/extensions/shape.h>

#include "debugtools.h"
#include "ts_shape.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(x11)

void  TSXShapeCombineRectangles(Display* a0, Window a1, int a2, int a3, int a4, XRectangle* a5, int a6, int a7, int a8)
{
  TRACE("Call XShapeCombineRectangles\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XShapeCombineRectangles(a0, a1, a2, a3, a4, a5, a6, a7, a8);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XShapeCombineRectangles\n");
}

void  TSXShapeCombineMask(Display* a0, Window a1, int a2, int a3, int a4, Pixmap a5, int a6)
{
  TRACE("Call XShapeCombineMask\n");
  EnterCriticalSection( &X11DRV_CritSection );
  XShapeCombineMask(a0, a1, a2, a3, a4, a5, a6);
  LeaveCriticalSection( &X11DRV_CritSection );
  TRACE("Ret XShapeCombineMask\n");
}

#endif /* defined(HAVE_LIBXSHAPE) */

