/*
 * TTY DC objects
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

#include "bitmap.h"
#include "gdi.h"
#include "ttydrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

/**********************************************************************/

extern BOOL TTYDRV_DC_BITMAP_DeleteObject(HBITMAP hbitmap, BITMAPOBJ *bitmap);


/***********************************************************************
 *		SelectFont   (TTYDRV.@)
 */
HFONT TTYDRV_SelectFont(TTYDRV_PDEVICE *physDev, HFONT hfont)
{
  TRACE("(%x, 0x%04x)\n", physDev->hdc, hfont);

  return TRUE; /* Use device font */
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
      ERR("handle (0x%04x) has unknown magic (0x%04x)\n", handle, GDIMAGIC(ptr->wMagic));
      result = FALSE;
  }

  GDI_ReleaseObj(handle);

  return result;
}
