/*
 * TTY font driver
 *
 * Copyright 1999 Patrik Stridvall
 */
#include "dc.h"
#include "debugtools.h"
#include "font.h"
#include "ttydrv.h"
#include "wingdi.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

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

/***********************************************************************
 *		TTYDRV_DC_FONT_SelectObject
 */
HFONT TTYDRV_DC_FONT_SelectObject(DC* dc, HFONT hfont, FONTOBJ *font)
{
  HFONT hPreviousFont;

  TRACE("(%p, 0x%04x, %p)\n", dc, hfont, font);

  hPreviousFont = dc->w.hFont;
  dc->w.hFont = hfont;

  return hPreviousFont;
}
