/*
 * TTY bitmap driver
 *
 * Copyright 1999 Patrik Stridvall
 */

#include <string.h>
#include "bitmap.h"
#include "gdi.h"
#include "ttydrv.h"
#include "winbase.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

static LONG TTYDRV_DC_GetBitmapBits(BITMAPOBJ *bitmap, void *bits, LONG count);
static LONG TTYDRV_DC_SetBitmapBits(BITMAPOBJ *bitmap, void *bits, LONG count);

/***********************************************************************
 *		TTYDRV_DC_AllocBitmap
 */
TTYDRV_PHYSBITMAP *TTYDRV_DC_AllocBitmap(BITMAPOBJ *bitmap)
{
  TTYDRV_PHYSBITMAP *physBitmap;
  
  if(!(physBitmap = HeapAlloc(GetProcessHeap(), 0, sizeof(TTYDRV_PHYSBITMAP)))) {
    ERR("Can't alloc TTYDRV_PHYSBITMAP\n");
    return NULL;
  }

  bitmap->physBitmap = physBitmap;
  bitmap->funcs = DRIVER_FindDriver("DISPLAY");

  return physBitmap;
}

/***********************************************************************
 *           TTYDRV_DC_BitmapBits
 */
LONG TTYDRV_DC_BitmapBits(HBITMAP hbitmap, void *bits, LONG count, WORD flags)
{
  BITMAPOBJ *bitmap;
  LONG result;

  if(!(bitmap = (BITMAPOBJ *) GDI_GetObjPtr(hbitmap, BITMAP_MAGIC)))
    return FALSE;
  
  if(flags == DDB_GET)
    result = TTYDRV_DC_GetBitmapBits(bitmap, bits, count);
  else if(flags == DDB_SET)
    result = TTYDRV_DC_SetBitmapBits(bitmap, bits, count);
  else {
    ERR("Unknown flags value %d\n", flags);
    result = 0;
  }
  
  GDI_ReleaseObj(hbitmap);
  return result;
}

/***********************************************************************
 *		TTYDRV_DC_CreateBitmap
 */
BOOL TTYDRV_DC_CreateBitmap(HBITMAP hbitmap)
{
  TTYDRV_PHYSBITMAP *physBitmap;
  BITMAPOBJ *bitmap;

  TRACE("(0x%04x)\n", hbitmap);

  if(!(bitmap = (BITMAPOBJ *) GDI_GetObjPtr(hbitmap, BITMAP_MAGIC)))
    return FALSE;
  
  if(!(physBitmap = TTYDRV_DC_AllocBitmap(bitmap))) {
    GDI_ReleaseObj(hbitmap);
    return FALSE;
  }
 
  /* Set bitmap bits */
  if(bitmap->bitmap.bmBits) { 
    TTYDRV_DC_BitmapBits(hbitmap, bitmap->bitmap.bmBits,
			 bitmap->bitmap.bmHeight * bitmap->bitmap.bmWidthBytes,
			 DDB_SET );
  }

  GDI_ReleaseObj(hbitmap);
  
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_BITMAP_DeleteObject
 */
BOOL TTYDRV_DC_BITMAP_DeleteObject(HBITMAP hbitmap, BITMAPOBJ *bitmap)
{
  TRACE("(0x%04x, %p)\n", hbitmap, bitmap);

  HeapFree(GetProcessHeap(), 0, bitmap->physBitmap);
  bitmap->physBitmap = NULL;
  bitmap->funcs = NULL;

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_GetBitmapBits
 */
static LONG TTYDRV_DC_GetBitmapBits(BITMAPOBJ *bitmap, void *bits, LONG count)
{
  FIXME("(%p, %p, %ld): stub\n", bitmap, bits, count);

  memset(bits, 0, count);

  return count;
}

/***********************************************************************
 *		TTYDRV_DC_BITMAP_SelectObject
 */
HBITMAP TTYDRV_DC_BITMAP_SelectObject(DC *dc, HBITMAP hbitmap, BITMAPOBJ *bitmap)
{
  HBITMAP hPreviousBitmap;

  TRACE("(%p, 0x%04x, %p)\n", dc, hbitmap, bitmap);

  if(!(dc->flags & DC_MEMORY)) 
    return 0;

  /* Assure that the bitmap device dependent */
  if(!bitmap->physBitmap && !TTYDRV_DC_CreateBitmap(hbitmap))
    return 0;

  if(bitmap->funcs != dc->funcs) {
    ERR("Trying to select a non-TTY DDB into a TTY DC\n");
    return 0;
  }

  dc->totalExtent.left   = 0;
  dc->totalExtent.top    = 0;
  dc->totalExtent.right  = bitmap->bitmap.bmWidth;
  dc->totalExtent.bottom = bitmap->bitmap.bmHeight;

  /* FIXME: Should be done in the common code instead */
  if(dc->hVisRgn) {
    SetRectRgn(dc->hVisRgn, 0, 0,
	       bitmap->bitmap.bmWidth, bitmap->bitmap.bmHeight);
  } else { 
    HRGN hrgn;

    if(!(hrgn = CreateRectRgn(0, 0, bitmap->bitmap.bmWidth, bitmap->bitmap.bmHeight)))
      return 0;

    dc->hVisRgn = hrgn;
  }

  hPreviousBitmap = dc->hBitmap;
  dc->hBitmap = hbitmap;

  return hPreviousBitmap;
}

/***********************************************************************
 *		TTYDRV_DC_SetBitmapBits
 */
static LONG TTYDRV_DC_SetBitmapBits(BITMAPOBJ *bitmap, void *bits, LONG count)
{
  FIXME("(%p, %p, %ld): semistub\n", bitmap, bits, count);

  return count;
}

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

/***********************************************************************
 *		TTYDRV_BITMAP_DeleteDIBSection
 */
void TTYDRV_BITMAP_DeleteDIBSection(BITMAPOBJ *bmp)
{
  FIXME("(%p): stub\n", bmp);
}

/***********************************************************************
 *		TTYDRV_BITMAP_SetDIBColorTable
 */
UINT TTYDRV_BITMAP_SetDIBColorTable(BITMAPOBJ *bmp, DC *dc, UINT start, UINT count, const RGBQUAD *colors)
{
  FIXME("(%p): stub\n", bmp);
  return 0;
}

/***********************************************************************
 *		TTYDRV_BITMAP_GetDIBColorTable
 */
UINT TTYDRV_BITMAP_GetDIBColorTable(BITMAPOBJ *bmp, DC *dc, UINT start, UINT count, RGBQUAD *colors)
{
  FIXME("(%p): stub\n", bmp);
  return 0;
}

/***********************************************************************
 *		TTYDRV_BITMAP_Lock
 */
INT TTYDRV_BITMAP_Lock(BITMAPOBJ *bmp, INT req, BOOL lossy)
{
  FIXME("(%p): stub\n", bmp);
  return DIB_Status_None;
}

/***********************************************************************
 *		TTYDRV_BITMAP_Unlock
 */
void TTYDRV_BITMAP_Unlock(BITMAPOBJ *bmp, BOOL commit)
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
