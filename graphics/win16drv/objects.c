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


extern HBITMAP32 WIN16DRV_BITMAP_SelectObject( DC * dc, HBITMAP32 hbitmap,
                                             BITMAPOBJ * bmp );
extern HBRUSH32 WIN16DRV_BRUSH_SelectObject( DC * dc, HBRUSH32 hbrush,
                                           BRUSHOBJ * brush );
extern HFONT32 WIN16DRV_FONT_SelectObject( DC * dc, HFONT32 hfont,
                                         FONTOBJ * font );
extern HPEN32 WIN16DRV_PEN_SelectObject( DC * dc, HPEN32 hpen, PENOBJ * pen );


/***********************************************************************
 *           WIN16DRV_SelectObject
 */
HGDIOBJ32 WIN16DRV_SelectObject( DC *dc, HGDIOBJ32 handle )
{
    GDIOBJHDR *ptr = GDI_GetObjPtr( handle, MAGIC_DONTCARE );
    HGDIOBJ32 ret = 0;

    if (!ptr) return 0;
    TRACE(gdi, "hdc=%04x %04x\n", dc->hSelf, handle );
    
    switch(ptr->wMagic)
    {
    case PEN_MAGIC:
        ret = WIN16DRV_PEN_SelectObject( dc, handle, (PENOBJ *)ptr );	  
        break;
    case BRUSH_MAGIC:
        ret = WIN16DRV_BRUSH_SelectObject( dc, handle, (BRUSHOBJ *)ptr );	  
        break;
    case BITMAP_MAGIC:
        FIXME(gdi, "WIN16DRV_SelectObject for BITMAP not implemented\n");
        ret = 1;
        break;
    case FONT_MAGIC:
        ret = WIN16DRV_FONT_SelectObject( dc, handle, (FONTOBJ *)ptr );	  
	break;
    case REGION_MAGIC:
	ret = (HGDIOBJ16)SelectClipRgn16( dc->hSelf, handle );
	break;
    }
    GDI_HEAP_UNLOCK( handle );
    return ret;
}
