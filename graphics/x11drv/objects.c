/*
 * GDI objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <stdlib.h>
#include <stdio.h>
#include "bitmap.h"
#include "brush.h"
#include "font.h"
#include "pen.h"
#include "local.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(gdi)


extern HBITMAP X11DRV_BITMAP_SelectObject( DC * dc, HBITMAP hbitmap,
                                             BITMAPOBJ * bmp );
extern HBRUSH X11DRV_BRUSH_SelectObject( DC * dc, HBRUSH hbrush,
                                           BRUSHOBJ * brush );
extern HFONT X11DRV_FONT_SelectObject( DC * dc, HFONT hfont,
                                         FONTOBJ * font );
extern HPEN X11DRV_PEN_SelectObject( DC * dc, HPEN hpen, PENOBJ * pen );

extern BOOL X11DRV_BITMAP_DeleteObject( HBITMAP hbitmap, BITMAPOBJ *bmp );

/***********************************************************************
 *           X11DRV_SelectObject
 */
HGDIOBJ X11DRV_SelectObject( DC *dc, HGDIOBJ handle )
{
    GDIOBJHDR *ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    HGDIOBJ ret = 0;

    if (!ptr) return 0;
    TRACE("hdc=%04x %04x\n", dc->hSelf, handle );
    
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
BOOL X11DRV_DeleteObject( HGDIOBJ handle )
{
    GDIOBJHDR *ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    BOOL ret = 0;

    if (!ptr) return FALSE;
     
    switch(ptr->wMagic) {
    case BITMAP_MAGIC:
        ret = X11DRV_BITMAP_DeleteObject( handle, (BITMAPOBJ *)ptr );
	break;

    default:
        ERR("Shouldn't be here!\n");
	ret = FALSE;
	break;
    }
    GDI_HEAP_UNLOCK( handle );
    return ret;
}

#endif /* !defined(X_DISPLAY_MISSING) */
