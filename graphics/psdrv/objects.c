/*
 *	PostScript driver object handling
 *
 *	Copyright 1998  Huw D M Davies
 *
 */

#include "windows.h"
#include "psdrv.h"
#include "font.h"
#include "pen.h"
#include "brush.h"
#include "bitmap.h"
#include "debug.h"

/***********************************************************************
 *           PSDRV_BITMAP_SelectObject
 */
static HBITMAP16 PSDRV_BITMAP_SelectObject( DC * dc, HBITMAP16 hbitmap,
                                            BITMAPOBJ * bmp )
{
    FIXME(psdrv, "stub\n");
    return 0;
}


/***********************************************************************
 *           PSDRV_BRUSH_SelectObject
 */
static HBRUSH32 PSDRV_BRUSH_SelectObject( DC * dc, HBRUSH32 hbrush,
                                          BRUSHOBJ * brush )
{
    FIXME(psdrv, "stub\n");
    return 0;
}


/***********************************************************************
 *           PSDRV_PEN_SelectObject
 */
static HPEN32 PSDRV_PEN_SelectObject( DC * dc, HPEN32 hpen, PENOBJ * pen )
{
    FIXME(psdrv, "stub\n");
    return 0;
}


/***********************************************************************
 *           PSDRV_SelectObject
 */
HGDIOBJ32 PSDRV_SelectObject( DC *dc, HGDIOBJ32 handle )
{
    GDIOBJHDR * ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    HGDIOBJ32 ret = 0;

    if (!ptr) return 0;
    TRACE(psdrv, "hdc=%04x %04x\n", dc->hSelf, handle );
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  ret = PSDRV_PEN_SelectObject( dc, handle, (PENOBJ *)ptr );
	  break;
      case BRUSH_MAGIC:
	  ret = PSDRV_BRUSH_SelectObject( dc, handle, (BRUSHOBJ *)ptr );
	  break;
      case BITMAP_MAGIC:
	  ret = PSDRV_BITMAP_SelectObject( dc, handle, (BITMAPOBJ *)ptr );
	  break;
      case FONT_MAGIC:
	  ret = PSDRV_FONT_SelectObject( dc, handle, (FONTOBJ *)ptr );	  
	  break;
      case REGION_MAGIC:
	  ret = (HGDIOBJ16)SelectClipRgn16( dc->hSelf, handle );
	  break;
      default:
	  ERR(psdrv, "Unknown object magic %04x\n", ptr->wMagic);
	  break;
    }
    GDI_HEAP_UNLOCK( handle );
    return ret;
}
