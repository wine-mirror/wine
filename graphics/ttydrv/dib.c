/*
 * TTY DC dib
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "bitmap.h"
#include "dc.h"
#include "ttydrv.h"
#include "winbase.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *		TTYDRV_BITMAP_CreateDIBSection
 */
HBITMAP TTYDRV_BITMAP_CreateDIBSection(
  DC *dc, BITMAPINFO *bmi, UINT usage,
  LPVOID *bits, HANDLE section, DWORD offset)
{
  FIXME("(%p, %p, %u, %p, 0x%04x, %ld): stub\n",
	dc, bmi, usage, bits, section, offset);

  return (HBITMAP) NULL;
}

/**********************************************************************
 *		TTYDRV_BITMAP_CreateDIBSection16
 */
HBITMAP16 TTYDRV_DIB_CreateDIBSection16(
  DC *dc, BITMAPINFO *bmi, UINT16 usage,
  SEGPTR *bits, HANDLE section, DWORD offset)
{
  FIXME("(%p, %p, %u, %p, 0x%04x, %ld): stub\n",
	dc, bmi, usage, bits, section, offset);

  return (HBITMAP16) NULL;
}

/***********************************************************************
 *		TTYDRV_BITMAP_DeleteDIBSection
 */
void TTYDRV_BITMAP_DeleteDIBSection(BITMAPOBJ *bmp)
{
  FIXME("(%p): stub\n", bmp);
}

/***********************************************************************
 *		TTYDRV_BITMAP_GetDIBits
 */
INT TTYDRV_BITMAP_GetDIBits(
  BITMAPOBJ *bmp, DC *dc, UINT startscan, UINT lines, 
  LPVOID bits, BITMAPINFO *info, UINT coloruse, HBITMAP hbitmap)
{
  FIXME("(%p, %p, %u, %u, %p, %p, %u, 0x%04x): stub\n",
	bmp, dc, startscan, lines, bits, info, coloruse, hbitmap);

  return 0;
}


/***********************************************************************
 *		TTYDRV_BITMAP_SetDIBits
 */
INT TTYDRV_BITMAP_SetDIBits(
  BITMAPOBJ *bmp, DC *dc, UINT startscan, UINT lines, 
  LPCVOID bits, const BITMAPINFO *info, UINT coloruse, HBITMAP hbitmap)
{
  FIXME("(%p, %p, %u, %u, %p, %p, %u, 0x%04x): stub\n",
	bmp, dc, startscan, lines, bits, info, coloruse, hbitmap);

  return 0;
}

/***********************************************************************
 *		TTYDRV_DC_SetDIBitsToDevice
 */
INT TTYDRV_DC_SetDIBitsToDevice(DC *dc, INT xDest, INT yDest, DWORD cx,
				DWORD cy, INT xSrc, INT ySrc,
				UINT startscan, UINT lines, LPCVOID bits,
				const BITMAPINFO *info, UINT coloruse)
{
  FIXME("(%p, %d, %d, %ld, %ld, %d, %d, %u, %u, %p, %p, %u): stub\n",
	dc, xDest, yDest, cx, cy, xSrc, ySrc, startscan, lines, bits, info, coloruse);

  return 0;
}

