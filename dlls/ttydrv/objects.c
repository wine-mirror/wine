/*
 * TTY DC objects
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "bitmap.h"
#include "gdi.h"
#include "ttydrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

extern HBITMAP TTYDRV_DC_BITMAP_SelectObject(DC *dc, HBITMAP hbitmap, BITMAPOBJ *bitmap);
extern BOOL TTYDRV_DC_BITMAP_DeleteObject(HBITMAP hbitmap, BITMAPOBJ *bitmap);


/***********************************************************************
 *		TTYDRV_DC_BRUSH_SelectObject
 */
static HBRUSH TTYDRV_DC_BRUSH_SelectObject(DC *dc, HBRUSH hbrush)
{
  HBRUSH hPreviousBrush;

  TRACE("(%p, 0x%04x)\n", dc, hbrush);

  hPreviousBrush = dc->hBrush;
  dc->hBrush = hbrush;

  return hPreviousBrush;
}

/***********************************************************************
 *		TTYDRV_DC_FONT_SelectObject
 */
static HFONT TTYDRV_DC_FONT_SelectObject(DC* dc, HFONT hfont)
{
  HFONT hPreviousFont;

  TRACE("(%p, 0x%04x)\n", dc, hfont);

  hPreviousFont = dc->hFont;
  dc->hFont = hfont;

  return hPreviousFont;
}

/***********************************************************************
 *		TTYDRV_DC_PEN_SelectObject
 */
static HPEN TTYDRV_DC_PEN_SelectObject(DC *dc, HBRUSH hpen)
{
  HPEN hPreviousPen;

  TRACE("(%p, 0x%04x)\n", dc, hpen);

  hPreviousPen = dc->hPen;
  dc->hPen = hpen;

  return hPreviousPen;
}

/***********************************************************************
 *		TTYDRV_DC_SelectObject
 */
HGDIOBJ TTYDRV_DC_SelectObject(DC *dc, HGDIOBJ handle)
{
  GDIOBJHDR *ptr = GDI_GetObjPtr(handle, MAGIC_DONTCARE);
  HGDIOBJ result = 0;

  if(!ptr) return 0;

  switch(GDIMAGIC(ptr->wMagic))
  {
    case BITMAP_MAGIC:
      result = TTYDRV_DC_BITMAP_SelectObject(dc, handle, (BITMAPOBJ *) ptr);
      break;
    case BRUSH_MAGIC:
      result = TTYDRV_DC_BRUSH_SelectObject(dc, handle);
      break;
    case FONT_MAGIC:
      result = TTYDRV_DC_FONT_SelectObject(dc, handle);
      break;
    case PEN_MAGIC:
      result = TTYDRV_DC_PEN_SelectObject(dc, handle);
      break;
    case REGION_MAGIC:
      /* FIXME: Shouldn't be handled here */
      result = (HGDIOBJ) SelectClipRgn(dc->hSelf, handle);
      break;
    default:
      ERR("handle (0x%04x) has unknown magic (0x%04x)\n",
						handle, GDIMAGIC(ptr->wMagic));
  }

  GDI_ReleaseObj(handle);
    
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
     
  switch(GDIMAGIC(ptr->wMagic))
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
      ERR("handle (0x%04x) has unknown magic (0x%04x)\n",
						handle, GDIMAGIC(ptr->wMagic));   
      result = FALSE;
  }

  GDI_ReleaseObj(handle);

  return result;
}
