/*
 * TTY bitmap driver
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

#include <string.h>

#include "bitmap.h"
#include "gdi.h"
#include "ttydrv.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

extern const DC_FUNCTIONS *TTYDRV_DC_Funcs;  /* hack */

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
  bitmap->funcs = TTYDRV_DC_Funcs;

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
 *		TTYDRV_SelectBitmap   (TTYDRV.@)
 */
HBITMAP TTYDRV_SelectBitmap(TTYDRV_PDEVICE *physDev, HBITMAP hbitmap)
{
  DC *dc = physDev->dc;
  BITMAPOBJ *bitmap;

  TRACE("(%p, 0x%04x)\n", dc, hbitmap);

  if(!(dc->flags & DC_MEMORY)) 
    return 0;

  if (!(bitmap = GDI_GetObjPtr( hbitmap, BITMAP_MAGIC ))) return 0;
  /* Assure that the bitmap device dependent */
  if(!bitmap->physBitmap && !TTYDRV_DC_CreateBitmap(hbitmap))
  {
      GDI_ReleaseObj( hbitmap );
      return 0;
  }

  if(bitmap->funcs != dc->funcs) {
    ERR("Trying to select a non-TTY DDB into a TTY DC\n");
    GDI_ReleaseObj( hbitmap );
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
    {
        GDI_ReleaseObj( hbitmap );
        return 0;
    }
    dc->hVisRgn = hrgn;
  }
  GDI_ReleaseObj( hbitmap );
  return hbitmap;
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
  TTYDRV_PDEVICE *physDev, BITMAPINFO *bmi, UINT usage,
  LPVOID *bits, HANDLE section, DWORD offset)
{
  FIXME("(%x, %p, %u, %p, 0x%04x, %ld): stub\n",
	physDev->hdc, bmi, usage, bits, section, offset);

  return (HBITMAP) NULL;
}

/***********************************************************************
 *		TTYDRV_DC_SetDIBitsToDevice
 */
INT TTYDRV_DC_SetDIBitsToDevice(TTYDRV_PDEVICE *physDev, INT xDest, INT yDest, DWORD cx,
				DWORD cy, INT xSrc, INT ySrc,
				UINT startscan, UINT lines, LPCVOID bits,
				const BITMAPINFO *info, UINT coloruse)
{
  FIXME("(%x, %d, %d, %ld, %ld, %d, %d, %u, %u, %p, %p, %u): stub\n",
	physDev->hdc, xDest, yDest, cx, cy, xSrc, ySrc, startscan, lines, bits, info, coloruse);

  return 0;
}
