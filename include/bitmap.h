/*
 * GDI bitmap definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <X11/Xlib.h>
#include "windows.h"

  /* GCs used for B&W and color bitmap operations */
extern GC BITMAP_monoGC, BITMAP_colorGC;

#define BITMAP_GC(bmp) \
  (((bmp)->bitmap.bmBitsPixel == 1) ? BITMAP_monoGC : BITMAP_colorGC)

#endif  /* BITMAP_H */
