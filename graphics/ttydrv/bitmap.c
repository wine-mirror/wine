/*
 * TTY bitmap driver
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "bitmap.h"
#include "dc.h"
#include "debugtools.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/**********************************************************************
 *		TTYDRV_BITMAP_CreateDIBSection
 */
HBITMAP TTYDRV_BITMAP_CreateDIBSection(
  DC *dc, BITMAPINFO *bmi, UINT usage, 
  LPVOID *bits, HANDLE section, DWORD offset)
{
  return (HBITMAP) NULL;
}

/**********************************************************************
 *		TTYDRV_BITMAP_CreateDIBSection16
 */
HBITMAP16 TTYDRV_DIB_CreateDIBSection16(
  DC *dc, BITMAPINFO *bmi, UINT16 usage,
  SEGPTR *bits, HANDLE section, DWORD offset)
{
  return (HBITMAP16) NULL;
}

/**********************************************************************
 *		TTYDRV_BITMAP_SetDIBits
 */
INT TTYDRV_BITMAP_SetDIBits(
  BITMAPOBJ *bmp, DC *dc, UINT startscan, UINT lines, 
  LPCVOID bits, const BITMAPINFO *info, UINT coloruse, HBITMAP hbitmap)
{
  return 0;
}

/**********************************************************************
 *		TTYDRV_BITMAP_GetDIBits
 */
INT TTYDRV_BITMAP_GetDIBits(
  BITMAPOBJ *bmp, DC *dc, UINT startscan, UINT lines, 
  LPVOID bits, BITMAPINFO *info, UINT coloruse, HBITMAP hbitmap)
{
  return 0;
}

/**********************************************************************
 *		TTYDRV_BITMAP_DeleteDIBSection
 */
void TTYDRV_BITMAP_DeleteDIBSection(BITMAPOBJ *bmp)
{
}

/***********************************************************************
 *		TTYDRV_DC_CreateBitmap
 */
BOOL TTYDRV_DC_CreateBitmap(HBITMAP hbitmap)
{
  FIXME("(0x%04x): stub\n", hbitmap);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_BITMAP_DeleteObject
 */
BOOL TTYDRV_DC_BITMAP_DeleteObject(HBITMAP hbitmap, BITMAPOBJ *bitmap)
{
  FIXME("(0x%04x, %p): stub\n", hbitmap, bitmap);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_BITMAP_SelectObject
 */
HBITMAP TTYDRV_DC_BITMAP_SelectObject(DC *dc, HBITMAP hbitmap, BITMAPOBJ *bitmap)
{
  FIXME("(%p, 0x%04x, %p): stub\n", dc, hbitmap, bitmap);

  if(!(dc->w.flags & DC_MEMORY)) 
    return NULL;

  return NULL;
}

