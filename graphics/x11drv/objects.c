/*
 * GDI objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include "bitmap.h"
#include "brush.h"
#include "font.h"
#include "pen.h"
#include "debug.h"


extern HBITMAP32 X11DRV_BITMAP_SelectObject( DC * dc, HBITMAP32 hbitmap,
                                             BITMAPOBJ * bmp );
extern HBRUSH32 X11DRV_BRUSH_SelectObject( DC * dc, HBRUSH32 hbrush,
                                           BRUSHOBJ * brush );
extern HFONT32 X11DRV_FONT_SelectObject( DC * dc, HFONT32 hfont,
                                         FONTOBJ * font );
extern HPEN32 X11DRV_PEN_SelectObject( DC * dc, HPEN32 hpen, PENOBJ * pen );

extern BOOL32 X11DRV_BITMAP_DeleteObject( HBITMAP32 hbitmap, BITMAPOBJ *bmp );

/***********************************************************************
 *           X11DRV_SelectObject
 */
HGDIOBJ32 X11DRV_SelectObject( DC *dc, HGDIOBJ32 handle )
{
    GDIOBJHDR *ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    HGDIOBJ32 ret = 0;

    if (!ptr) return 0;
    TRACE(gdi, "hdc=%04x %04x\n", dc->hSelf, handle );
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  ret = X11DRV_PEN_SelectObject( dc, handle, (PENOBJ *)ptr );
	  break;
      case BRUSH_MAGIC:
	  ret = X11DRV_BRUSH_SelectObject( dc, handle, (BRUSHOBJ *)ptr );
	  break;
      case BITMAP_MAGIC:
	  ret = X11DRV_BITMAP_SelectObject( dc, handle, (BITMAPOBJ *)ptr );
	  break;
      case FONT_MAGIC:
	  ret = X11DRV_FONT_SelectObject( dc, handle, (FONTOBJ *)ptr );	  
	  break;
      case REGION_MAGIC:
	  ret = (HGDIOBJ16)SelectClipRgn16( dc->hSelf, handle );
	  break;
    }
    GDI_HEAP_UNLOCK( handle );
    return ret;
}


/***********************************************************************
 *           X11DRV_DeleteObject
 */
BOOL32 X11DRV_DeleteObject( HGDIOBJ32 handle )
{
    GDIOBJHDR *ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    BOOL32 ret = 0;

    if (!ptr) return FALSE;
     
    switch(ptr->wMagic) {
    case BITMAP_MAGIC:
        ret = X11DRV_BITMAP_DeleteObject( handle, (BITMAPOBJ *)ptr );
	break;

    default:
        ERR(gdi, "Shouldn't be here!\n");
	ret = FALSE;
	break;
    }
    GDI_HEAP_UNLOCK( handle );
    return ret;
}
