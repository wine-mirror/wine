/*
 * TTY DC graphics
 *
 * Copyright 1999 Patrik Stridvall
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "winnls.h"
#include "gdi.h"
#include "wine/debug.h"
#include "ttydrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

/***********************************************************************
 *		TTYDRV_DC_Arc
 */
BOOL TTYDRV_DC_Arc(TTYDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
		   INT xstart, INT ystart, INT xend, INT yend)
{
  FIXME("(%x, %d, %d, %d, %d, %d, %d, %d, %d): stub\n",
        physDev->hdc, left, top, right, bottom, xstart, ystart, xend, yend);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Chord
 */
BOOL TTYDRV_DC_Chord(TTYDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
		     INT xstart, INT ystart, INT xend, INT yend)
{
  FIXME("(%x, %d, %d, %d, %d, %d, %d, %d, %d): stub\n",
        physDev->hdc, left, top, right, bottom, xstart, ystart, xend, yend);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Ellipse
 */
BOOL TTYDRV_DC_Ellipse(TTYDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom)
{
  FIXME("(%x, %d, %d, %d, %d): stub\n", physDev->hdc, left, top, right, bottom);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_ExtFloodFill
 */
BOOL TTYDRV_DC_ExtFloodFill(TTYDRV_PDEVICE *physDev, INT x, INT y,
			    COLORREF color, UINT fillType)
{
  FIXME("(%x, %d, %d, 0x%08lx, %u): stub\n", physDev->hdc, x, y, color, fillType);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_GetPixel
 */
COLORREF TTYDRV_DC_GetPixel(TTYDRV_PDEVICE *physDev, INT x, INT y)
{
  FIXME("(%x, %d, %d): stub\n", physDev->hdc, x, y);
  return RGB(0,0,0); /* FIXME: Always returns black */
}

/***********************************************************************
 *		TTYDRV_DC_LineTo
 */
BOOL TTYDRV_DC_LineTo(TTYDRV_PDEVICE *physDev, INT x, INT y)
{
#ifdef WINE_CURSES
  INT row1, col1, row2, col2;
  DC *dc = physDev->dc;

  TRACE("(%x, %d, %d)\n", physDev->hdc, x, y);

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
  FIXME("(%x, %d, %d): stub\n", physDev->hdc, x, y);

  return TRUE;
#endif /* defined(WINE_CURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_PaintRgn
 */
BOOL TTYDRV_DC_PaintRgn(TTYDRV_PDEVICE *physDev, HRGN hrgn)
{
  FIXME("(%x, 0x%04x): stub\n", physDev->hdc, hrgn);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Pie
 */
BOOL TTYDRV_DC_Pie(TTYDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
		   INT xstart, INT ystart, INT xend, INT yend)
{
  FIXME("(%x, %d, %d, %d, %d, %d, %d, %d, %d): stub\n",
	physDev->hdc, left, top, right, bottom, xstart, ystart, xend, yend);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Polygon
 */
BOOL TTYDRV_DC_Polygon(TTYDRV_PDEVICE *physDev, const POINT* pt, INT count)
{
  FIXME("(%x, %p, %d): stub\n", physDev->hdc, pt, count);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Polyline
 */
BOOL TTYDRV_DC_Polyline(TTYDRV_PDEVICE *physDev, const POINT* pt, INT count)
{
  FIXME("(%x, %p, %d): stub\n", physDev->hdc, pt, count);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_PolyPolygon
 */
BOOL TTYDRV_DC_PolyPolygon(TTYDRV_PDEVICE *physDev, const POINT* pt, const INT* counts, UINT polygons)
{
  FIXME("(%x, %p, %p, %u): stub\n", physDev->hdc, pt, counts, polygons);
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_PolyPolyline
 */
BOOL TTYDRV_DC_PolyPolyline(TTYDRV_PDEVICE *physDev, const POINT* pt, const DWORD* counts, DWORD polylines)
{
  FIXME("(%x, %p, %p, %lu): stub\n", physDev->hdc, pt, counts, polylines);
  
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_Rectangle
 */
BOOL TTYDRV_DC_Rectangle(TTYDRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom)
{
#ifdef WINE_CURSES
  INT row1, col1, row2, col2;
  DC *dc = physDev->dc;

  TRACE("(%x, %d, %d, %d, %d)\n", physDev->hdc, left, top, right, bottom);

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
  FIXME("(%x, %d, %d, %d, %d): stub\n", physDev->hdc, left, top, right, bottom);

  return TRUE;
#endif /* defined(WINE_CURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_RoundRect
 */
BOOL TTYDRV_DC_RoundRect(TTYDRV_PDEVICE *physDev, INT left, INT top, INT right,
			 INT bottom, INT ell_width, INT ell_height)
{
  FIXME("(%x, %d, %d, %d, %d, %d, %d): stub\n", 
	physDev->hdc, left, top, right, bottom, ell_width, ell_height);
  
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_SetPixel
 */
COLORREF TTYDRV_DC_SetPixel(TTYDRV_PDEVICE *physDev, INT x, INT y, COLORREF color)
{
#ifdef WINE_CURSES
  INT row, col;
  DC *dc = physDev->dc;

  TRACE("(%x, %d, %d, 0x%08lx)\n", physDev->hdc, x, y, color);

  if(!physDev->window)
    return FALSE;

  row = (dc->DCOrgY + XLPTODP(dc, y)) / physDev->cellHeight;
  col = (dc->DCOrgX + XLPTODP(dc, x)) / physDev->cellWidth;

  mvwaddch(physDev->window, row, col, ACS_BULLET);
  wrefresh(physDev->window);

  return RGB(0,0,0); /* FIXME: Always returns black */
#else /* defined(WINE_CURSES) */
  FIXME("(%x, %d, %d, 0x%08lx): stub\n", physDev->hdc, x, y, color);

  return RGB(0,0,0); /* FIXME: Always returns black */
#endif /* defined(WINE_CURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_BitBlt
 */
BOOL TTYDRV_DC_BitBlt(TTYDRV_PDEVICE *physDevDst, INT xDst, INT yDst,
		      INT width, INT height, TTYDRV_PDEVICE *physDevSrc,
		      INT xSrc, INT ySrc, DWORD rop)
{
  FIXME("(%x, %d, %d, %d, %d, %x, %d, %d, %lu): stub\n",
        physDevDst->hdc, xDst, yDst, width, height, physDevSrc->hdc, xSrc, ySrc, rop );
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_PatBlt
 */
BOOL TTYDRV_DC_PatBlt(TTYDRV_PDEVICE *physDev, INT left, INT top,
		      INT width, INT height, DWORD rop)
{
  FIXME("(%x, %d, %d, %d, %d, %lu): stub\n", physDev->hdc, left, top, width, height, rop );
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_StretchBlt
 */
BOOL TTYDRV_DC_StretchBlt(TTYDRV_PDEVICE *physDevDst, INT xDst, INT yDst,
			  INT widthDst, INT heightDst,
			  TTYDRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc,
			  INT widthSrc, INT heightSrc, DWORD rop)
{
  FIXME("(%x, %d, %d, %d, %d, %x, %d, %d, %d, %d, %lu): stub\n",
        physDevDst->hdc, xDst, yDst, widthDst, heightDst,
        physDevSrc->hdc, xSrc, ySrc, widthSrc, heightSrc, rop );

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_ExtTextOut
 */
BOOL TTYDRV_DC_ExtTextOut(TTYDRV_PDEVICE *physDev, INT x, INT y, UINT flags,
			  const RECT *lpRect, LPCWSTR str, UINT count,
			  const INT *lpDx)
{
#ifdef WINE_CURSES
  INT row, col;
  LPSTR ascii;
  DWORD len;
  DC *dc = physDev->dc;

  TRACE("(%x, %d, %d, 0x%08x, %p, %s, %d, %p)\n",
        physDev->hdc, x, y, flags, lpRect, debugstr_wn(str, count), count, lpDx);

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
  len = WideCharToMultiByte( CP_ACP, 0, str, count, NULL, 0, NULL, NULL );
  ascii = HeapAlloc( GetProcessHeap(), 0, len );
  WideCharToMultiByte( CP_ACP, 0, str, count, ascii, len, NULL, NULL );
  mvwaddnstr(physDev->window, row, col, ascii, len);
  HeapFree( GetProcessHeap(), 0, ascii );
  wrefresh(physDev->window);

  if(dc->textAlign & TA_UPDATECP) {
    dc->CursPosX += count * physDev->cellWidth;
    dc->CursPosY += physDev->cellHeight;
  }

  return TRUE;
#else /* defined(WINE_CURSES) */
  FIXME("(%x, %d, %d, 0x%08x, %p, %s, %d, %p): stub\n",
        physDev->hdc, x, y, flags, lpRect, debugstr_wn(str,count), count, lpDx);

  return TRUE;
#endif /* defined(WINE_CURSES) */
}

/***********************************************************************
 *		TTYDRV_DC_GetCharWidth
 */
BOOL TTYDRV_DC_GetCharWidth(TTYDRV_PDEVICE *physDev, UINT firstChar, UINT lastChar,
			    LPINT buffer)
{
  UINT c;

  FIXME("(%x, %u, %u, %p): semistub\n", physDev->hdc, firstChar, lastChar, buffer);

  for(c=firstChar; c<=lastChar; c++) {
    buffer[c-firstChar] = physDev->cellWidth;
  }

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_GetTextExtentPoint
 */
BOOL TTYDRV_DC_GetTextExtentPoint(TTYDRV_PDEVICE *physDev, LPCWSTR str, INT count,
				  LPSIZE size)
{
  TRACE("(%x, %s, %d, %p)\n", physDev->hdc, debugstr_wn(str, count), count, size);

  size->cx = count * physDev->cellWidth;
  size->cy = physDev->cellHeight;

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_GetTextMetrics
 */
BOOL TTYDRV_DC_GetTextMetrics(TTYDRV_PDEVICE *physDev, LPTEXTMETRICW lptm)
{
  TRACE("(%x, %p)\n", physDev->hdc, lptm);

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
