/*
 * TTY DC OEM bitmap
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "bitmap.h"
#include "debugtools.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/**********************************************************************
 *		TTYDRV_DC_LoadOEMResource
 */
HANDLE TTYDRV_DC_LoadOEMResource(WORD resid, WORD type)
{
  HBITMAP hbitmap;
  BITMAPOBJ *bmpObjPtr;

  FIXME("(%d, %d): semistub\n", resid, type);
 
  if(!(hbitmap = GDI_AllocObject(sizeof(BITMAPOBJ), BITMAP_MAGIC)))
    return (HANDLE) NULL;
  
  bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_LOCK(hbitmap);
  bmpObjPtr->size.cx = 0;
  bmpObjPtr->size.cy = 0;
  bmpObjPtr->bitmap.bmType       = 0;
  bmpObjPtr->bitmap.bmWidth      = 0;
  bmpObjPtr->bitmap.bmHeight     = 0;
  bmpObjPtr->bitmap.bmWidthBytes = 0;
  bmpObjPtr->bitmap.bmPlanes     = 0;
  bmpObjPtr->bitmap.bmBitsPixel  = 0;
  bmpObjPtr->bitmap.bmBits       = NULL;
  bmpObjPtr->dib                 = NULL;
  
  GDI_HEAP_UNLOCK( hbitmap );
  return hbitmap;
}
