/*
 * Thread safe wrappers around shape calls.
 * Always include this file instead of <X11/shape.h>.
 * This file was generated automatically by tools/make_X11wrappers
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TS_SHAPE_H
#define __WINE_TS_SHAPE_H

#include "config.h"

#ifdef HAVE_LIBXSHAPE
#include <X11/IntrinsicP.h>

#include <X11/extensions/shape.h>

extern void  TSXShapeCombineRectangles(Display*, Window, int, int, int, XRectangle*, int, int, int);
extern void  TSXShapeCombineMask(Display*, Window, int, int, int, Pixmap, int);

#endif /* defined(HAVE_LIBXSHAPE) */

#endif /* __WINE_TS_SHAPE_H */
