/*
 * TTY DC text
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "config.h"

#include "windef.h"
#include "wingdi.h"
#include "dc.h"
#include "debugtools.h"
#include "gdi.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);

/***********************************************************************
 *		TTYDRV_DC_ExtTextOut
 */
BOOL TTYDRV_DC_ExtTextOut(DC *dc, INT x, INT y, UINT flags,
			  const RECT *lpRect, LPCWSTR str, UINT count,
			  const INT *lpDx)
{
#ifdef HAVE_LIBCURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row, col;
  LPSTR ascii;

  TRACE("(%p, %d, %d, 0x%08x, %p, %s, %d, %p)\n",
	dc, x, y, flags, lpRect, debugstr_wn(str, count), count, lpDx);

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
  ascii = HeapAlloc( GetProcessHeap(), 0, count+1 );
  lstrcpynWtoA(ascii, str, count+1);
  mvwaddnstr(physDev->window, row, col, ascii, count);
  HeapFree( GetProcessHeap(), 0, ascii );
  wrefresh(physDev->window);

  if(dc->w.textAlign & TA_UPDATECP) {
    dc->w.CursPosX += count * physDev->cellWidth;
    dc->w.CursPosY += physDev->cellHeight;
  }

  return TRUE;
#else /* defined(HAVE_LIBCURSES) */
  FIXME("(%p, %d, %d, 0x%08x, %p, %s, %d, %p): stub\n",
	dc, x, y, flags, lpRect, debugstr_wn(str,count), count, lpDx);

  return TRUE;
#endif /* defined(HAVE_LIBCURSES) */
}
