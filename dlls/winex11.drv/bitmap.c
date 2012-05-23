/*
 * X11DRV bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1999 Noel Borthwick
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "x11drv.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

  /* GCs used for B&W and color bitmap operations */
static GC bitmap_gc[32];
X_PHYSBITMAP BITMAP_stock_phys_bitmap = { 0 };  /* phys bitmap for the default stock bitmap */

static XContext bitmap_context;  /* X context to associate a phys bitmap to a handle */

GC get_bitmap_gc(int depth)
{
    if(depth < 1 || depth > 32)
        return 0;

    return bitmap_gc[depth-1];
}

/***********************************************************************
 *           X11DRV_BITMAP_Init
 */
void X11DRV_BITMAP_Init(void)
{
    int depth_count, index, i;
    int *depth_list;
    Pixmap tmpPixmap;

    wine_tsx11_lock();
    bitmap_context = XUniqueContext();
    BITMAP_stock_phys_bitmap.depth = 1;
    BITMAP_stock_phys_bitmap.pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 1 );
    bitmap_gc[0] = XCreateGC( gdi_display, BITMAP_stock_phys_bitmap.pixmap, 0, NULL );
    XSetGraphicsExposures( gdi_display, bitmap_gc[0], False );
    XSetSubwindowMode( gdi_display, bitmap_gc[0], IncludeInferiors );

    /* Create a GC for all available depths. GCs at depths other than 1-bit/screen_depth are for use
     * in combination with XRender which allows us to create dibsections at more depths.
     */
    depth_list = XListDepths(gdi_display, DefaultScreen(gdi_display), &depth_count);
    for (i = 0; i < depth_count; i++)
    {
        index = depth_list[i] - 1;
        if (bitmap_gc[index]) continue;
        if ((tmpPixmap = XCreatePixmap( gdi_display, root_window, 1, 1, depth_list[i])))
        {
            if ((bitmap_gc[index] = XCreateGC( gdi_display, tmpPixmap, 0, NULL )))
            {
                XSetGraphicsExposures( gdi_display, bitmap_gc[index], False );
                XSetSubwindowMode( gdi_display, bitmap_gc[index], IncludeInferiors );
            }
            XFreePixmap( gdi_display, tmpPixmap );
        }
    }
    XFree( depth_list );

    wine_tsx11_unlock();
}

/***********************************************************************
 *           SelectBitmap   (X11DRV.@)
 */
HBITMAP X11DRV_SelectBitmap( PHYSDEV dev, HBITMAP hbitmap )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    X_PHYSBITMAP *physBitmap;
    BITMAP bitmap;

    if (!GetObjectW( hbitmap, sizeof(bitmap), &bitmap )) return 0;

    if (hbitmap == BITMAP_stock_phys_bitmap.hbitmap) physBitmap = &BITMAP_stock_phys_bitmap;
    else if (!(physBitmap = X11DRV_get_phys_bitmap( hbitmap ))) return 0;

    physDev->bitmap = physBitmap;
    physDev->drawable = physBitmap->pixmap;
    physDev->color_shifts = physBitmap->trueColor ? &physBitmap->color_shifts : NULL;
    SetRect( &physDev->drawable_rect, 0, 0, bitmap.bmWidth, bitmap.bmHeight );
    physDev->dc_rect = physDev->drawable_rect;

      /* Change GC depth if needed */

    if (physDev->depth != physBitmap->depth)
    {
        physDev->depth = physBitmap->depth;
        wine_tsx11_lock();
        XFreeGC( gdi_display, physDev->gc );
        physDev->gc = XCreateGC( gdi_display, physDev->drawable, 0, NULL );
        XSetGraphicsExposures( gdi_display, physDev->gc, False );
        XSetSubwindowMode( gdi_display, physDev->gc, IncludeInferiors );
        XFlush( gdi_display );
        wine_tsx11_unlock();
    }

    return hbitmap;
}


/***********************************************************************
 *           X11DRV_create_phys_bitmap
 */
X_PHYSBITMAP *X11DRV_create_phys_bitmap( HBITMAP hbitmap, const BITMAP *bitmap, int depth )
{
    X_PHYSBITMAP *physBitmap;

    if (bitmap->bmWidth > 65535 || bitmap->bmHeight > 65535 || bitmap->bmPlanes != 1 ||
        (bitmap->bmBitsPixel != 1 && bitmap->bmBitsPixel != screen_bpp))
    {
        WARN( "Trying to create invalid pixmap %dx%d planes %d bpp %d\n",
              bitmap->bmWidth, bitmap->bmHeight, bitmap->bmPlanes, bitmap->bmBitsPixel );
        return NULL;
    }

    if (!(physBitmap = X11DRV_init_phys_bitmap( hbitmap ))) return NULL;

    physBitmap->depth = depth;

    wine_tsx11_lock();
    physBitmap->pixmap = XCreatePixmap( gdi_display, root_window,
                                        bitmap->bmWidth, bitmap->bmHeight, physBitmap->depth );
    if (physBitmap->pixmap)
    {
        GC gc = get_bitmap_gc( depth );
        XSetFunction( gdi_display, gc, GXclear );
        XFillRectangle( gdi_display, physBitmap->pixmap, gc, 0, 0, bitmap->bmWidth, bitmap->bmHeight );
        XSetFunction( gdi_display, gc, GXcopy );
    }
    wine_tsx11_unlock();
    if (!physBitmap->pixmap)
    {
        WARN("Can't create Pixmap\n");
        HeapFree( GetProcessHeap(), 0, physBitmap );
        return NULL;
    }
    TRACE( "(%p) %dx%d %d bpp -> %lx\n",
           hbitmap, bitmap->bmWidth, bitmap->bmHeight, bitmap->bmBitsPixel, physBitmap->pixmap );
    return physBitmap;
}


/****************************************************************************
 *	  CreateBitmap   (X11DRV.@)
 *
 * Create a device dependent X11 bitmap
 *
 * Returns TRUE on success else FALSE
 */
BOOL X11DRV_CreateBitmap( PHYSDEV dev, HBITMAP hbitmap )
{
    BITMAP bitmap;
    X_PHYSBITMAP *phys_bitmap;

    if (!GetObjectW( hbitmap, sizeof(bitmap), &bitmap )) return FALSE;

    if (bitmap.bmBitsPixel == 1)
    {
        if (!(phys_bitmap = X11DRV_create_phys_bitmap( hbitmap, &bitmap, 1 ))) return FALSE;
        phys_bitmap->trueColor = FALSE;
    }
    else
    {
        if (!(phys_bitmap = X11DRV_create_phys_bitmap( hbitmap, &bitmap, screen_depth ))) return FALSE;
        phys_bitmap->trueColor = (visual->class == TrueColor || visual->class == DirectColor);
        phys_bitmap->color_shifts = X11DRV_PALETTE_default_shifts;
    }
    return TRUE;
}


/***********************************************************************
 *           DeleteBitmap   (X11DRV.@)
 */
BOOL X11DRV_DeleteBitmap( HBITMAP hbitmap )
{
    X_PHYSBITMAP *physBitmap = X11DRV_get_phys_bitmap( hbitmap );

    if (physBitmap)
    {
        wine_tsx11_lock();
        if (physBitmap->pixmap) XFreePixmap( gdi_display, physBitmap->pixmap );
        XDeleteContext( gdi_display, (XID)hbitmap, bitmap_context );
        wine_tsx11_unlock();
        HeapFree( GetProcessHeap(), 0, physBitmap );
    }
    return TRUE;
}


/***********************************************************************
 *           X11DRV_get_phys_bitmap
 *
 * Retrieve the X physical bitmap info.
 */
X_PHYSBITMAP *X11DRV_get_phys_bitmap( HBITMAP hbitmap )
{
    X_PHYSBITMAP *ret;

    wine_tsx11_lock();
    if (XFindContext( gdi_display, (XID)hbitmap, bitmap_context, (char **)&ret )) ret = NULL;
    wine_tsx11_unlock();
    return ret;
}


/***********************************************************************
 *           X11DRV_init_phys_bitmap
 *
 * Initialize the X physical bitmap info.
 */
X_PHYSBITMAP *X11DRV_init_phys_bitmap( HBITMAP hbitmap )
{
    X_PHYSBITMAP *ret;

    if ((ret = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret) )) != NULL)
    {
        ret->hbitmap = hbitmap;
        wine_tsx11_lock();
        XSaveContext( gdi_display, (XID)hbitmap, bitmap_context, (char *)ret );
        wine_tsx11_unlock();
    }
    return ret;
}


/***********************************************************************
 *           X11DRV_get_pixmap
 *
 * Retrieve the pixmap associated to a bitmap.
 */
Pixmap X11DRV_get_pixmap( HBITMAP hbitmap )
{
    X_PHYSBITMAP *physBitmap = X11DRV_get_phys_bitmap( hbitmap );

    if (!physBitmap) return 0;
    return physBitmap->pixmap;
}
