/*
 * TTY DC graphics
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "config.h"

#include "wine/winestring.h"
#include "gdi.h"
#include "heap.h"
#include "debugtools.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);

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
#ifdef WINE_CURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row1, col1, row2, col2;

  TRACE("(%p, %d, %d)\n", dc, x, y);

  if(!physDev->window)
    return FALSE;

  row1 = (dc->DCOrgY + XLPTODP(dc, dc->CursPosY)) / physDev->cellHeight;
  col1 = (dc->DCOrgX + XLPTODP(dc, dc->CursPosX)) / physDev->cellWidth;
  row2 = (dc->DCOrgY + XLPTODP(dc, y)) / physDev->cellHeight;
  col2 = (dc->DCOrgX + XLPTODP(dc, x)) / physDev->cellWidth;

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
    wvline(physDev->window, ACS_VLINE, row2-row1);
  } else if(row1 == row2) {
    whline(physDev->window, ACS_HLINE, col2-col1);
  } else {
    FIXME("Diagonal line drawing not yet supported\n");
  }
  wrefresh(physDev->window);

  return TRUE;
#else /* defined(WINE_CURSES) */
  FIXME("(%p, %d, %d): stub\n", dc, x, y);

  return TRUE;
#endif /* defined(WINE_CURSES) */
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
#ifdef WINE_CURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row1, col1, row2, col2;

  TRACE("(%p, %d, %d, %d, %d)\n", dc, left, top, right, bottom);

  if(!physDev->window)
    return FALSE;

  row1 = (dc->DCOrgY + XLPTODP(dc, top)) / physDev->cellHeight;
  col1 = (dc->DCOrgX + XLPTODP(dc, left)) / physDev->cellWidth;
  row2 = (dc->DCOrgY + XLPTODP(dc, bottom)) / physDev->cellHeight;
  col2 = (dc->DCOrgX + XLPTODP(dc, right)) / physDev->cellWidth;

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
  whline(physDev->window, ACS_HLINE, col2-col1);

  wmove(physDev->window, row1, col2);
  wvline(physDev->window, ACS_VLINE, row2-row1);

  wmove(physDev->window, row2, col1);
  whline(physDev->window, ACS_HLINE, col2-col1);

  wmove(physDev->window, row1, col1);
  wvline(physDev->window, ACS_VLINE, row2-row1);

  mvwaddch(physDev->window, row1, col1, ACS_ULCORNER);
  mvwaddch(physDev->window, row1, col2, ACS_URCORNER);
  mvwaddch(physDev->window, row2, col2, ACS_LRCORNER);
  mvwaddch(physDev->window, row2, col1, ACS_LLCORNER);

  wrefresh(physDev->window);

  return TRUE;
#else /* defined(WINE_CURSES) */
  FIXME("(%p, %d, %d, %d, %d): stub\n", dc, left, top, right, bottom);

  return TRUE;
#endif /* defined(WINE_CURSES) */
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

  oldColor = dc->backgroundColor;
  dc->backgroundColor = color;

  return oldColor;
}

/***********************************************************************
 *		TTYDRV_DC_SetPixel
 */
COLORREF TTYDRV_DC_SetPixel(DC *dc, INT x, INT y, COLORREF color)
{
#ifdef WINE_CURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row, col;

  TRACE("(%p, %d, %d, 0x%08lx)\n", dc, x, y, color);

  if(!physDev->window)
    return FALSE;

  row = (dc->DCOrgY + XLPTODP(dc, y)) / physDev->cellHeight;
  col = (dc->DCOrgX + XLPTODP(dc, x)) / physDev->cellWidth;

  mvwaddch(physDev->window, row, col, ACS_BULLET);
  wrefresh(physDev->window);

  return RGB(0,0,0); /* FIXME: Always returns black */
#else /* defined(WINE_CURSES) */
  FIXME("(%p, %d, %d, 0x%08lx): stub\n", dc, x, y, color);

  return RGB(0,0,0); /* FIXME: Always returns black */
#endif /* defined(WINE_CURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_SetTextColor
 */
COLORREF TTYDRV_DC_SetTextColor(DC *dc, COLORREF color)
{
  COLORREF oldColor;

  TRACE("(%p, 0x%08lx)\n", dc, color);
  
  oldColor = dc->textColor;
  dc->textColor = color;
  
  return oldColor;
}


/***********************************************************************
 *		TTYDRV_DC_BitBlt
 */
BOOL TTYDRV_DC_BitBlt(DC *dcDst, INT xDst, INT yDst,
		      INT width, INT height, DC *dcSrc,
		      INT xSrc, INT ySrc, DWORD rop)
{
  FIXME("(%p, %d, %d, %d, %d, %p, %d, %d, %lu): stub\n",
	dcDst, xDst, yDst, width, height, 
        dcSrc, xSrc, ySrc, rop
  );

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_PatBlt
 */
BOOL TTYDRV_DC_PatBlt(DC *dc, INT left, INT top,
		      INT width, INT height, DWORD rop)
{
  FIXME("(%p, %d, %d, %d, %d, %lu): stub\n",
	dc, left, top, width, height, rop
  );


  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_StretchBlt
 */
BOOL TTYDRV_DC_StretchBlt(DC *dcDst, INT xDst, INT yDst,
			  INT widthDst, INT heightDst,
			  DC *dcSrc, INT xSrc, INT ySrc,
			  INT widthSrc, INT heightSrc, DWORD rop)
{
  FIXME("(%p, %d, %d, %d, %d, %p, %d, %d, %d, %d, %lu): stub\n",
	dcDst, xDst, yDst, widthDst, heightDst,
	dcSrc, xSrc, ySrc, widthSrc, heightSrc, rop
  );

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_ExtTextOut
 */
BOOL TTYDRV_DC_ExtTextOut(DC *dc, INT x, INT y, UINT flags,
			  const RECT *lpRect, LPCWSTR str, UINT count,
			  const INT *lpDx)
{
#ifdef WINE_CURSES
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;
  INT row, col;
  LPSTR ascii;

  TRACE("(%p, %d, %d, 0x%08x, %p, %s, %d, %p)\n",
	dc, x, y, flags, lpRect, debugstr_wn(str, count), count, lpDx);

  if(!physDev->window)
    return FALSE;

  /* FIXME: Is this really correct? */
  if(dc->textAlign & TA_UPDATECP) {
    x = dc->CursPosX;
    y = dc->CursPosY;
  }

  x = XLPTODP(dc, x);
  y = YLPTODP(dc, y);
  
  row = (dc->DCOrgY + y) / physDev->cellHeight;
  col = (dc->DCOrgX + x) / physDev->cellWidth;
  ascii = HeapAlloc( GetProcessHeap(), 0, count+1 );
  lstrcpynWtoA(ascii, str, count+1);
  mvwaddnstr(physDev->window, row, col, ascii, count);
  HeapFree( GetProcessHeap(), 0, ascii );
  wrefresh(physDev->window);

  if(dc->textAlign & TA_UPDATECP) {
    dc->CursPosX += count * physDev->cellWidth;
    dc->CursPosY += physDev->cellHeight;
  }

  return TRUE;
#else /* defined(WINE_CURSES) */
  FIXME("(%p, %d, %d, 0x%08x, %p, %s, %d, %p): stub\n",
	dc, x, y, flags, lpRect, debugstr_wn(str,count), count, lpDx);

  return TRUE;
#endif /* defined(WINE_CURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_GetCharWidth
 */
BOOL TTYDRV_DC_GetCharWidth(DC *dc, UINT firstChar, UINT lastChar,
			    LPINT buffer)
{
  UINT c;
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;

  FIXME("(%p, %u, %u, %p): semistub\n", dc, firstChar, lastChar, buffer);

  for(c=firstChar; c<=lastChar; c++) {
    buffer[c-firstChar] = physDev->cellWidth;
  }

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_GetTextExtentPoint
 */
BOOL TTYDRV_DC_GetTextExtentPoint(DC *dc, LPCWSTR str, INT count,
				  LPSIZE size)
{
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;

  TRACE("(%p, %s, %d, %p)\n", dc, debugstr_wn(str, count), count, size);

  size->cx = count * physDev->cellWidth;
  size->cy = physDev->cellHeight;

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_GetTextMetrics
 */
BOOL TTYDRV_DC_GetTextMetrics(DC *dc, LPTEXTMETRICA lptm)
{
  TTYDRV_PDEVICE *physDev = (TTYDRV_PDEVICE *) dc->physDev;

  TRACE("(%p, %p)\n", dc, lptm);

  lptm->tmHeight = physDev->cellHeight;
  lptm->tmAscent = 0;
  lptm->tmDescent = 0;
  lptm->tmInternalLeading = 0;
  lptm->tmExternalLeading = 0;
  lptm->tmAveCharWidth = physDev->cellWidth; 
  lptm->tmMaxCharWidth = physDev->cellWidth;
  lptm->tmWeight = FW_MEDIUM;
  lptm->tmOverhang = 0;
  lptm->tmDigitizedAspectX = physDev->cellWidth;
  lptm->tmDigitizedAspectY = physDev->cellHeight;
  lptm->tmFirstChar = 32;
  lptm->tmLastChar = 255;
  lptm->tmDefaultChar = 0;
  lptm->tmBreakChar = 32;
  lptm->tmItalic = FALSE;
  lptm->tmUnderlined = FALSE;
  lptm->tmStruckOut = FALSE;
  lptm->tmPitchAndFamily = TMPF_FIXED_PITCH|TMPF_DEVICE;
  lptm->tmCharSet = ANSI_CHARSET;

  return TRUE;
}
