/*
 * TTY bitmap driver
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "bitmap.h"
#include "dc.h"
#include "ttydrv.h"
#include "winbase.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/**********************************************************************/

static LONG TTYDRV_DC_GetBitmapBits(BITMAPOBJ *bitmap, void *bits, LONG count);
static LONG TTYDRV_DC_SetBitmapBits(BITMAPOBJ *bitmap, void *bits, LONG count);

/***********************************************************************
 *		TTYDRV_DC_AllocBitmap
 */
TTYDRV_PHYSBITMAP *TTYDRV_DC_AllocBitmap(BITMAPOBJ *bitmap)
{
  TTYDRV_PHYSBITMAP *physBitmap;
  
  if(!(bitmap->DDBitmap = HeapAlloc(GetProcessHeap(), 0, sizeof(DDBITMAP)))) {
    ERR("Can't alloc DDBITMAP\n");
    return NULL;
  }
 
  if(!(physBitmap = HeapAlloc(GetProcessHeap(), 0, sizeof(TTYDRV_PHYSBITMAP)))) {
    ERR("Can't alloc TTYDRV_PHYSBITMAP\n");
    HeapFree(GetProcessHeap(), 0, bitmap->DDBitmap);
    return NULL;
  }

  bitmap->DDBitmap->physBitmap = physBitmap;
  bitmap->DDBitmap->funcs = DRIVER_FindDriver("DISPLAY");

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
  
  GDI_HEAP_UNLOCK(hbitmap);
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
    GDI_HEAP_UNLOCK(hbitmap);
    return FALSE;
  }
 
  /* Set bitmap bits */
  if(bitmap->bitmap.bmBits) { 
    TTYDRV_DC_BitmapBits(hbitmap, bitmap->bitmap.bmBits,
			 bitmap->bitmap.bmHeight * bitmap->bitmap.bmWidthBytes,
			 DDB_SET );
  }

  GDI_HEAP_UNLOCK(hbitmap);
  
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_BITMAP_DeleteObject
 */
BOOL TTYDRV_DC_BITMAP_DeleteObject(HBITMAP hbitmap, BITMAPOBJ *bitmap)
{
  TRACE("(0x%04x, %p)\n", hbitmap, bitmap);

  HeapFree(GetProcessHeap(), 0, bitmap->DDBitmap->physBitmap);
  HeapFree(GetProcessHeap(), 0, bitmap->DDBitmap);
  bitmap->DDBitmap = NULL;
  
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

  if(!(dc->w.flags & DC_MEMORY)) 
    return NULL;

  /* Assure that the bitmap device dependent */
  if(!bitmap->DDBitmap && !TTYDRV_DC_CreateBitmap(hbitmap))
    return NULL;

  if(bitmap->DDBitmap->funcs != dc->funcs) {
    ERR("Trying to select a non-TTY DDB into a TTY DC\n");
    return NULL;
  }

  dc->w.totalExtent.left   = 0;
  dc->w.totalExtent.top    = 0;
  dc->w.totalExtent.right  = bitmap->bitmap.bmWidth;
  dc->w.totalExtent.bottom = bitmap->bitmap.bmHeight;

  /* FIXME: Should be done in the common code instead */
  if(dc->w.hVisRgn) {
    SetRectRgn(dc->w.hVisRgn, 0, 0,
	       bitmap->bitmap.bmWidth, bitmap->bitmap.bmHeight);
  } else { 
    HRGN hrgn;

    if(!(hrgn = CreateRectRgn(0, 0, bitmap->bitmap.bmWidth, bitmap->bitmap.bmHeight)))
      return NULL;

    dc->w.hVisRgn = hrgn;
  }

  hPreviousBitmap = dc->w.hBitmap;
  dc->w.hBitmap = hbitmap;

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
