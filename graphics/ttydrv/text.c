/*
 * TTY DC text
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "config.h"

#include "wine/wingdi16.h"
#include "dc.h"
#include "debugtools.h"
#include "gdi.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *		TTYDRV_DC_ExtTextOut
 */
BOOL TTYDRV_DC_ExtTextOut(DC *dc, INT x, INT y, UINT flags,
			  const RECT *lpRect, LPCSTR str, UINT count,
			  const INT *lpDx)
{
#ifdef HAVE_LIBCURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row, col;

  TRACE("(%p, %d, %d, 0x%08x, %p, %s, %d, %p)\n",
	dc, x, y, flags, lpRect, debugstr_a(str), count, lpDx);

  if(!physDev->window)
    return FALSE;

  /* FIXME: Is this really correct? */
  if(dc->w.textAlign & TA_UPDATECP) {
    x = dc->w.CursPosX;
    y = dc->w.CursPosY;
  }

  x = XLPTODP(dc, x);
  y = YLPTODP(dc, y);
  
  row = (dc->w.DCOrgY + y) / physDev->cellHeight;
  col = (dc->w.DCOrgX + x) / physDev->cellWidth;

  mvwaddnstr(physDev->window, row, col, str, count);
  wrefresh(physDev->window);

  if(dc->w.textAlign & TA_UPDATECP) {
    dc->w.CursPosX += count * physDev->cellWidth;
    dc->w.CursPosY += count * physDev->cellHeight;
  }

  return TRUE;
#else /* defined(HAVE_LIBCURSES) */
  FIXME("(%p, %d, %d, 0x%08x, %p, %s, %d, %p): stub\n",
	dc, x, y, flags, lpRect, debugstr_a(str), count, lpDx);

  return TRUE;
#endif /* defined(HAVE_LIBCURSES) */
}
