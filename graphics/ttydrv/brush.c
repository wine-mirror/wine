/*
 * TTY DC brush
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "brush.h"
#include "dc.h"
#include "debugtools.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *		TTYDRV_DC_BRUSH_SelectObject
 */
HBRUSH TTYDRV_DC_BRUSH_SelectObject(DC *dc, HBRUSH hbrush, BRUSHOBJ *brush)
{
  FIXME("(%p, 0x%08x, %p): stub\n", dc, hbrush, brush);

  return NULL;
}
