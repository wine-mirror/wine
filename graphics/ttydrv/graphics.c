/*
 * TTY DC graphics
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "config.h"

#include "dc.h"
#include "debugtools.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *		TTYDRV_DC_Arc
 */
BOOL TTYDRV_DC_Arc(DC *dc, INT left, INT top, INT right, INT bottom,
		   INT xstart, INT ystart, INT xend, INT yend)
{
  FIXME("(%p, %d, %d, %d, %d, %d, %d, %d, %d): stub\n",
	dc, left, top, right, bottom, xstart, ystart, xend, yend);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Chord
 */
BOOL TTYDRV_DC_Chord(DC *dc, INT left, INT top, INT right, INT bottom,
		     INT xstart, INT ystart, INT xend, INT yend)
{
  FIXME("(%p, %d, %d, %d, %d, %d, %d, %d, %d): stub\n",
	dc, left, top, right, bottom, xstart, ystart, xend, yend);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Ellipse
 */
BOOL TTYDRV_DC_Ellipse(DC *dc, INT left, INT top, INT right, INT bottom)
{
  FIXME("(%p, %d, %d, %d, %d): stub\n",
	dc, left, top, right, bottom);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_ExtFloodFill
 */
BOOL TTYDRV_DC_ExtFloodFill(DC *dc, INT x, INT y,
			    COLORREF color, UINT fillType)
{
  FIXME("(%p, %d, %d, 0x%08lx, %u): stub\n", dc, x, y, color, fillType);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_GetPixel
 */
COLORREF TTYDRV_DC_GetPixel(DC *dc, INT x, INT y)
{
  FIXME("(%p, %d, %d): stub\n", dc, x, y);

  return RGB(0,0,0); /* FIXME: Always returns black */
}

/***********************************************************************
 *		TTYDRV_DC_LineTo
 */
BOOL TTYDRV_DC_LineTo(DC *dc, INT x, INT y)
{
#ifdef HAVE_LIBCURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row1, col1, row2, col2;

  TRACE("(%p, %d, %d)\n", dc, x, y);

  row1 = (dc->w.DCOrgY + XLPTODP(dc, dc->w.CursPosY)) / physDev->cellHeight;
  col1 = (dc->w.DCOrgX + XLPTODP(dc, dc->w.CursPosX)) / physDev->cellWidth;
  row2 = (dc->w.DCOrgY + XLPTODP(dc, y)) / physDev->cellHeight;
  col2 = (dc->w.DCOrgX + XLPTODP(dc, x)) / physDev->cellWidth;

  if(row1 > row2) {
    INT tmp = row1;
    row1 = row2;
    row2 = tmp; 
  } 

  if(col1 > col2) {
    INT tmp = col1;
    col1 = col2;
    col2 = tmp; 
  } 

  wmove(physDev->window, row1, col1);
  if(col1 == col2) {
    wvline(physDev->window, '|', row2-row1);
  } else if(row1 == row2) {
    whline(physDev->window, '-', col2-col1);
  } else {
    FIXME("Diagonal line drawing not yet supported\n");
  }
  wrefresh(physDev->window);

  return TRUE;
#else /* defined(HAVE_LIBCURSES) */
  FIXME("(%p, %d, %d): stub\n", dc, x, y);

  return TRUE;
#endif /* defined(HAVE_LIBCURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_PaintRgn
 */
BOOL TTYDRV_DC_PaintRgn(DC *dc, HRGN hrgn)
{
  FIXME("(%p, 0x%04x): stub\n", dc, hrgn);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Pie
 */
BOOL TTYDRV_DC_Pie(DC *dc, INT left, INT top, INT right, INT bottom,
		   INT xstart, INT ystart, INT xend, INT yend)
{
  FIXME("(%p, %d, %d, %d, %d, %d, %d, %d, %d): stub\n",
	dc, left, top, right, bottom, xstart, ystart, xend, yend);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_PolyBezier
 */
BOOL TTYDRV_DC_PolyBezier(DC *dc, const POINT* BezierPoints, DWORD count)
{
  FIXME("(%p, %p, %lu): stub\n", dc, BezierPoints, count);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Polygon
 */
BOOL TTYDRV_DC_Polygon(DC *dc, const POINT* pt, INT count)
{
  FIXME("(%p, %p, %d): stub\n", dc, pt, count);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Polyline
 */
BOOL TTYDRV_DC_Polyline(DC *dc, const POINT* pt, INT count)
{
  FIXME("(%p, %p, %d): stub\n", dc, pt, count);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_PolyPolygon
 */
BOOL TTYDRV_DC_PolyPolygon(DC *dc, const POINT* pt, const INT* counts, UINT polygons)
{
  FIXME("(%p, %p, %p, %u): stub\n", dc, pt, counts, polygons);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_PolyPolyline
 */
BOOL TTYDRV_DC_PolyPolyline(DC *dc, const POINT* pt, const DWORD* counts, DWORD polylines)
{
  FIXME("(%p, %p, %p, %lu): stub\n", dc, pt, counts, polylines);
  
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Rectangle
 */
BOOL TTYDRV_DC_Rectangle(DC *dc, INT left, INT top, INT right, INT bottom)
{
#ifdef HAVE_LIBCURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row1, col1, row2, col2;

  TRACE("(%p, %d, %d, %d, %d)\n", dc, left, top, right, bottom);

  row1 = (dc->w.DCOrgY + XLPTODP(dc, top)) / physDev->cellHeight;
  col1 = (dc->w.DCOrgX + XLPTODP(dc, left)) / physDev->cellWidth;
  row2 = (dc->w.DCOrgY + XLPTODP(dc, bottom)) / physDev->cellHeight;
  col2 = (dc->w.DCOrgX + XLPTODP(dc, right)) / physDev->cellWidth;

  if(row1 > row2) {
    INT tmp = row1;
    row1 = row2;
    row2 = tmp; 
  } 
  if(col1 > col2) {
    INT tmp = col1;
    col1 = col2;
    col2 = tmp; 
  } 

  wmove(physDev->window, row1, col1);
  whline(physDev->window, '-', col2-col1);

  wmove(physDev->window, row1, col2);
  wvline(physDev->window, '|', row2-row1);

  wmove(physDev->window, row2, col1);
  whline(physDev->window, '-', col2-col1);

  wmove(physDev->window, row1, col1);
  wvline(physDev->window, '|', row2-row1);

  mvwaddch(physDev->window, row1, col1, '+');
  mvwaddch(physDev->window, row1, col2, '+');
  mvwaddch(physDev->window, row2, col2, '+');
  mvwaddch(physDev->window, row2, col1, '+');

  wrefresh(physDev->window);

  return TRUE;
#else /* defined(HAVE_LIBCURSES) */
  FIXME("(%p, %d, %d, %d, %d): stub\n", dc, left, top, right, bottom);

  return TRUE;
#endif /* defined(HAVE_LIBCURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_RoundRect
 */
BOOL TTYDRV_DC_RoundRect(DC *dc, INT left, INT top, INT right,
			 INT bottom, INT ell_width, INT ell_height)
{
  FIXME("(%p, %d, %d, %d, %d, %d, %d): stub\n", 
	dc, left, top, right, bottom, ell_width, ell_height);
  
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_SetBkColor
 */
COLORREF TTYDRV_DC_SetBkColor(DC *dc, COLORREF color)
{
  COLORREF oldColor;

  TRACE("(%p, 0x%08lx)\n", dc, color);  

  oldColor = dc->w.backgroundColor;
  dc->w.backgroundColor = color;

  return oldColor;
}

/***********************************************************************
 *		TTYDRV_DC_SetPixel
 */
COLORREF TTYDRV_DC_SetPixel(DC *dc, INT x, INT y, COLORREF color)
{
#ifdef HAVE_LIBCURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row, col;

  TRACE("(%p, %d, %d, 0x%08lx)\n", dc, x, y, color);

  row = (dc->w.DCOrgY + XLPTODP(dc, y)) / physDev->cellHeight;
  col = (dc->w.DCOrgX + XLPTODP(dc, x)) / physDev->cellWidth;

  mvwaddch(physDev->window, row, col, '.');
  wrefresh(physDev->window);

  return RGB(0,0,0); /* FIXME: Always returns black */
#else /* defined(HAVE_LIBCURSES) */
  FIXME("(%p, %d, %d, 0x%08lx): stub\n", dc, x, y, color);

  return RGB(0,0,0); /* FIXME: Always returns black */
#endif /* defined(HAVE_LIBCURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_SetTextColor
 */
COLORREF TTYDRV_DC_SetTextColor(DC *dc, COLORREF color)
{
  COLORREF oldColor;

  TRACE("(%p, 0x%08lx)\n", dc, color);
  
  oldColor = dc->w.textColor;
  dc->w.textColor = color;
  
  return oldColor;
}

