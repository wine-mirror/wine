/*
 * TTY bitmap driver
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "dc.h"
#include "bitmap.h"
#include "ttydrv.h"

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
