/*
 * TTY DC pen
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "dc.h"
#include "debugtools.h"
#include "pen.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *		TTYDRV_DC_PEN_SelectObject
 */
HPEN TTYDRV_DC_PEN_SelectObject(DC *dc, HBRUSH hpen, PENOBJ *pen)
{
  FIXME("(%p, 0x%08x, %p): stub\n", dc, hpen, pen);

  return NULL;
}
