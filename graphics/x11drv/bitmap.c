/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include "ts_xlib.h"
#include "ts_xutil.h"
#include "gdi.h"
#include "callback.h"
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "debug.h"

/***********************************************************************
 *           X11DRV_BITMAP_Init
 */
BOOL32 X11DRV_BITMAP_Init(void)
{
    Pixmap tmpPixmap;
    
      /* Create the necessary GCs */
    
    if ((tmpPixmap = TSXCreatePixmap( display, rootWindow, 1, 1, 1 )))
    {
	BITMAP_monoGC = TSXCreateGC( display, tmpPixmap, 0, NULL );
	TSXSetGraphicsExposures( display, BITMAP_monoGC, False );
	TSXFreePixmap( display, tmpPixmap );
    }

    if (screenDepth != 1)
    {
	if ((tmpPixmap = TSXCreatePixmap(display, rootWindow, 1,1,screenDepth)))
	{
	    BITMAP_colorGC = TSXCreateGC( display, tmpPixmap, 0, NULL );
	    TSXSetGraphicsExposures( display, BITMAP_colorGC, False );
	    TSXFreePixmap( display, tmpPixmap );
	}
    }
    return TRUE;
}

/***********************************************************************
 *           X11DRV_BITMAP_SelectObject
 */
HBITMAP32 X11DRV_BITMAP_SelectObject( DC * dc, HBITMAP32 hbitmap,
                                      BITMAPOBJ * bmp )
{
    HRGN32 hrgn;
    HBITMAP32 prevHandle = dc->w.hBitmap;
    
    if (!(dc->w.flags & DC_MEMORY)) return 0;

    if (dc->w.hVisRgn)
       SetRectRgn32( dc->w.hVisRgn, 0, 0,
                     bmp->bitmap.bmWidth, bmp->bitmap.bmHeight );
    else
    { 
       hrgn = CreateRectRgn32(0, 0, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight);
       if (!hrgn) return 0;
       dc->w.hVisRgn    = hrgn;
    }

    dc->u.x.drawable = bmp->pixmap;
    dc->w.hBitmap    = hbitmap;

      /* Change GC depth if needed */

    if (dc->w.bitsPerPixel != bmp->bitmap.bmBitsPixel)
    {
	TSXFreeGC( display, dc->u.x.gc );
	dc->u.x.gc = TSXCreateGC( display, dc->u.x.drawable, 0, NULL );
	dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;
        DC_InitDC( dc );
    }
    else CLIPPING_UpdateGCRegion( dc );  /* Just update GC clip region */
    return prevHandle;
}
