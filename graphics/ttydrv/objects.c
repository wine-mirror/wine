/*
 * TTY DC objects
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "bitmap.h"
#include "brush.h"
#include "dc.h"
#include "font.h"
#include "gdi.h"
#include "pen.h"
#include "ttydrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/**********************************************************************/

extern HBITMAP TTYDRV_DC_BITMAP_SelectObject(DC *dc, HBITMAP hbitmap, BITMAPOBJ *bitmap);
extern HBRUSH TTYDRV_DC_BRUSH_SelectObject(DC *dc, HBRUSH hbrush, BRUSHOBJ *brush);
extern HFONT TTYDRV_DC_FONT_SelectObject(DC* dc, HFONT hfont, FONTOBJ *font);
extern HPEN TTYDRV_DC_PEN_SelectObject(DC *dc, HBRUSH hpen, PENOBJ *pen);

extern BOOL TTYDRV_DC_BITMAP_DeleteObject(HBITMAP hbitmap, BITMAPOBJ *bitmap);

/***********************************************************************
 *		TTYDRV_DC_SelectObject
 */
HGDIOBJ TTYDRV_DC_SelectObject(DC *dc, HGDIOBJ handle)
{
  GDIOBJHDR *ptr = GDI_GetObjPtr(handle, MAGIC_DONTCARE);
  HGDIOBJ result = NULL;

  if(!ptr) return NULL;

  switch(ptr->wMagic) 
  {
    case BITMAP_MAGIC:
      result = TTYDRV_DC_BITMAP_SelectObject(dc, handle, (BITMAPOBJ *) ptr);
      break;
    case BRUSH_MAGIC:
      result = TTYDRV_DC_BRUSH_SelectObject(dc, handle, (BRUSHOBJ *) ptr);
      break;
    case FONT_MAGIC:
      result = TTYDRV_DC_FONT_SelectObject(dc, handle, (FONTOBJ *) ptr);	  
      break;
    case PEN_MAGIC:
      result = TTYDRV_DC_PEN_SelectObject(dc, handle, (PENOBJ *) ptr);
      break;
    case REGION_MAGIC:
      /* FIXME: Shouldn't be handled here */
      result = (HGDIOBJ) SelectClipRgn(dc->hSelf, handle);
      break;
    default:
      ERR("handle (0x%04x) has unknown magic (0x%04x)\n", handle, ptr->wMagic);
  }

  GDI_HEAP_UNLOCK(handle);
    
  return result;
}

/***********************************************************************
 *           TTYDRV_DC_DeleteObject
 */
BOOL TTYDRV_DC_DeleteObject(HGDIOBJ handle)
{
  GDIOBJHDR *ptr = GDI_GetObjPtr(handle, MAGIC_DONTCARE);
  BOOL result;
  
  if(!ptr) return FALSE;
     
  switch(ptr->wMagic)
  {
    case BITMAP_MAGIC:
      result = TTYDRV_DC_BITMAP_DeleteObject(handle, (BITMAPOBJ *) ptr);
      break;
    case BRUSH_MAGIC:
    case FONT_MAGIC:
    case PEN_MAGIC:
    case REGION_MAGIC:
      result = TRUE;
      break;
    default:
      ERR("handle (0x%04x) has unknown magic (0x%04x)\n", handle, ptr->wMagic);   
      result = FALSE;
  }

  GDI_HEAP_UNLOCK(handle);

  return result;
}
