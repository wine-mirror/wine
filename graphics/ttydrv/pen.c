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
  HPEN hPreviousPen;

  TRACE("(%p, 0x%04x, %p)\n", dc, hpen, pen);

  hPreviousPen = dc->w.hPen;
  dc->w.hPen = hpen;

  return hPreviousPen;
}
