/*
 * GDI Device Context functions
 *
 * Copyright 1993 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winnls.h"
#include "winternl.h"
#include "winerror.h"
#include "ntgdi_private.h"

#include "wine/opengl_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);

static pthread_mutex_t dc_attr_lock = PTHREAD_MUTEX_INITIALIZER;

struct dc_attr_bucket
{
    struct list entry;
    DC_ATTR *entries;
    DC_ATTR *next_free;
    DC_ATTR *next_unused;
};

static struct list dc_attr_buckets = LIST_INIT( dc_attr_buckets );

static BOOL DC_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs dc_funcs =
{
    NULL,             /* pGetObjectW */
    NULL,             /* pUnrealizeObject */
    DC_DeleteObject   /* pDeleteObject */
};


static inline DC *get_dc_obj( HDC hdc )
{
    DWORD type;
    DC *dc = get_any_obj_ptr( hdc, &type );
    if (!dc) return NULL;

    switch (type)
    {
    case NTGDI_OBJ_DC:
    case NTGDI_OBJ_MEMDC:
    case NTGDI_OBJ_ENHMETADC:
        return dc;
    default:
        GDI_ReleaseObj( hdc );
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return NULL;
    }
}

/* alloc DC_ATTR from a pool of memory accessible from client */
static DC_ATTR *alloc_dc_attr(void)
{
    struct dc_attr_bucket *bucket;
    DC_ATTR *dc_attr = NULL;

    pthread_mutex_lock( &dc_attr_lock );

    LIST_FOR_EACH_ENTRY( bucket, &dc_attr_buckets, struct dc_attr_bucket, entry )
    {
        if (bucket->next_free)
        {
            dc_attr = bucket->next_free;
            bucket->next_free = *(void **)bucket->next_free;
            break;
        }
        if ((char *)bucket->next_unused - (char *)bucket->entries + sizeof(*dc_attr) <=
            system_info.AllocationGranularity)
        {
            dc_attr = bucket->next_unused++;
            break;
        }
    }

    if (!dc_attr && (bucket = malloc( sizeof(*bucket) )))
    {
        SIZE_T size = system_info.AllocationGranularity;
        bucket->entries = NULL;
        if (!NtAllocateVirtualMemory( GetCurrentProcess(), (void **)&bucket->entries, zero_bits,
                                      &size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ))
        {
            bucket->next_free = NULL;
            bucket->next_unused = bucket->entries + 1;
            dc_attr = bucket->entries;
            list_add_head( &dc_attr_buckets, &bucket->entry );
        }
        else free( bucket );
    }

    if (dc_attr) memset( dc_attr, 0, sizeof( *dc_attr ));

    pthread_mutex_unlock( &dc_attr_lock );

    return dc_attr;
}


static void free_dc_attr( DC_ATTR *dc_attr )
{
    struct dc_attr_bucket *bucket;

    pthread_mutex_lock( &dc_attr_lock );

    LIST_FOR_EACH_ENTRY( bucket, &dc_attr_buckets, struct dc_attr_bucket, entry )
    {
        if (bucket->entries > dc_attr || dc_attr >= bucket->next_unused) continue;
        *(void **)dc_attr = bucket->next_free;
        bucket->next_free = dc_attr;
        break;
    }

    pthread_mutex_unlock( &dc_attr_lock );
}


/***********************************************************************
 *           set_initial_dc_state
 */
static void set_initial_dc_state( DC *dc )
{
    dc->attr->wnd_org.x     = 0;
    dc->attr->wnd_org.y     = 0;
    dc->attr->wnd_ext.cx    = 1;
    dc->attr->wnd_ext.cy    = 1;
    dc->attr->vport_org.x   = 0;
    dc->attr->vport_org.y   = 0;
    dc->attr->vport_ext.cx  = 1;
    dc->attr->vport_ext.cy  = 1;
    dc->attr->miter_limit   = 10.0f; /* 10.0 is the default, from MSDN */
    dc->attr->layout        = 0;
    dc->attr->rop_mode      = R2_COPYPEN;
    dc->attr->font_code_page   = CP_ACP;
    dc->attr->poly_fill_mode   = ALTERNATE;
    dc->attr->stretch_blt_mode = BLACKONWHITE;
    dc->attr->rel_abs_mode     = ABSOLUTE;
    dc->attr->background_mode  = OPAQUE;
    dc->attr->background_color = RGB( 255, 255, 255 );
    dc->attr->brush_color      = RGB( 255, 255, 255 );
    dc->attr->pen_color        = RGB( 0, 0, 0 );
    dc->attr->text_color    = RGB( 0, 0, 0 );
    dc->attr->brush_org.x   = 0;
    dc->attr->brush_org.y   = 0;
    dc->attr->mapper_flags  = 0;
    dc->attr->text_align    = TA_LEFT | TA_TOP | TA_NOUPDATECP;
    dc->attr->char_extra    = 0;
    dc->breakExtra          = 0;
    dc->breakRem            = 0;
    dc->attr->map_mode      = MM_TEXT;
    dc->attr->graphics_mode = GM_COMPATIBLE;
    dc->attr->cur_pos.x     = 0;
    dc->attr->cur_pos.y     = 0;
    dc->attr->arc_direction = AD_COUNTERCLOCKWISE;
    dc->xformWorld2Wnd.eM11 = 1.0f;
    dc->xformWorld2Wnd.eM12 = 0.0f;
    dc->xformWorld2Wnd.eM21 = 0.0f;
    dc->xformWorld2Wnd.eM22 = 1.0f;
    dc->xformWorld2Wnd.eDx  = 0.0f;
    dc->xformWorld2Wnd.eDy  = 0.0f;
    dc->xformWorld2Vport    = dc->xformWorld2Wnd;
    dc->xformVport2World    = dc->xformWorld2Wnd;
    dc->vport2WorldValid    = TRUE;

    reset_bounds( &dc->bounds );
}

/***********************************************************************
 *           alloc_dc_ptr
 */
DC *alloc_dc_ptr( DWORD magic )
{
    DC *dc;

    if (!(dc = calloc( 1, sizeof(*dc) ))) return NULL;
    if (!(dc->attr = alloc_dc_attr()))
    {
        free( dc );
        return NULL;
    }

    dc->nulldrv.funcs       = &null_driver;
    dc->physDev             = &dc->nulldrv;
    dc->thread              = GetCurrentThreadId();
    dc->refcount            = 1;
    dc->hPen                = GDI_inc_ref_count( GetStockObject( BLACK_PEN ));
    dc->hBrush              = GDI_inc_ref_count( GetStockObject( WHITE_BRUSH ));
    dc->hFont               = GDI_inc_ref_count( GetStockObject( SYSTEM_FONT ));
    dc->hPalette            = GetStockObject( DEFAULT_PALETTE );

    set_initial_dc_state( dc );

    if (!(dc->hSelf = alloc_gdi_handle( &dc->obj, magic, &dc_funcs )))
    {
        free_dc_attr( dc->attr );
        free( dc );
        return NULL;
    }
    dc->nulldrv.hdc = dc->hSelf;
    dc->attr->hdc = HandleToUlong( dc->hSelf );
    set_gdi_client_ptr( dc->hSelf, dc->attr );

    if (!font_driver.pCreateDC( &dc->physDev, NULL, NULL, NULL ))
    {
        free_dc_ptr( dc );
        return NULL;
    }
    return dc;
}



/***********************************************************************
 *           free_dc_state
 */
static void free_dc_state( DC *dc )
{
    if (dc->opengl_drawable) opengl_drawable_release( dc->opengl_drawable );
    if (dc->hClipRgn) NtGdiDeleteObjectApp( dc->hClipRgn );
    if (dc->hMetaRgn) NtGdiDeleteObjectApp( dc->hMetaRgn );
    if (dc->hVisRgn) NtGdiDeleteObjectApp( dc->hVisRgn );
    if (dc->region) NtGdiDeleteObjectApp( dc->region );
    if (dc->path) free_gdi_path( dc->path );
    free_dc_attr( dc->attr );
    free( dc );
}


/***********************************************************************
 *           free_dc_ptr
 */
void free_dc_ptr( DC *dc )
{
    assert( dc->refcount == 1 );

    while (dc->physDev != &dc->nulldrv)
    {
        PHYSDEV physdev = dc->physDev;
        dc->physDev = physdev->next;
        physdev->funcs->pDeleteDC( physdev );
    }
    GDI_dec_ref_count( dc->hPen );
    GDI_dec_ref_count( dc->hBrush );
    GDI_dec_ref_count( dc->hFont );
    if (dc->hBitmap && !dc->is_display) GDI_dec_ref_count( dc->hBitmap );
    free_gdi_handle( dc->hSelf );
    free_dc_state( dc );
}


/***********************************************************************
 *           get_dc_ptr
 *
 * Retrieve a DC pointer but release the GDI lock.
 */
DC *get_dc_ptr( HDC hdc )
{
    DC *dc = get_dc_obj( hdc );
    if (!dc) return NULL;
    if (dc->attr->disabled)
    {
        GDI_ReleaseObj( hdc );
        return NULL;
    }

    if (!InterlockedCompareExchange( &dc->refcount, 1, 0 ))
    {
        dc->thread = GetCurrentThreadId();
    }
    else if (dc->thread != GetCurrentThreadId())
    {
        WARN( "dc %p belongs to thread %04x\n", hdc, dc->thread );
        GDI_ReleaseObj( hdc );
        return NULL;
    }
    else InterlockedIncrement( &dc->refcount );

    GDI_ReleaseObj( hdc );
    return dc;
}


/***********************************************************************
 *           release_dc_ptr
 */
void release_dc_ptr( DC *dc )
{
    LONG ref;

    dc->thread = 0;
    ref = InterlockedDecrement( &dc->refcount );
    assert( ref >= 0 );
    if (ref) dc->thread = GetCurrentThreadId();  /* we still own it */
}


static void set_bk_color( DC *dc, COLORREF color )
{
    PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetBkColor );
    dc->attr->background_color = physdev->funcs->pSetBkColor( physdev, color );
}


static void set_text_color( DC *dc, COLORREF color )
{
    PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetTextColor );
    dc->attr->text_color = physdev->funcs->pSetTextColor( physdev, color );
}


/***********************************************************************
 *           DC_InitDC
 *
 * Setup device-specific DC values for a newly created DC.
 */
void DC_InitDC( DC* dc )
{
    PHYSDEV physdev = GET_DC_PHYSDEV( dc, pRealizeDefaultPalette );
    physdev->funcs->pRealizeDefaultPalette( physdev );
    set_text_color( dc, dc->attr->text_color );
    set_bk_color( dc, dc->attr->background_color );
    NtGdiSelectPen( dc->hSelf, dc->hPen );
    NtGdiSelectBrush( dc->hSelf, dc->hBrush );
    NtGdiSelectFont( dc->hSelf, dc->hFont );
    update_dc_clipping( dc );
    NtGdiSetVirtualResolution( dc->hSelf, 0, 0, 0, 0 );
    physdev = GET_DC_PHYSDEV( dc, pSetBoundsRect );
    physdev->funcs->pSetBoundsRect( physdev, &dc->bounds, dc->bounds_enabled ? DCB_ENABLE : DCB_DISABLE );
}


/***********************************************************************
 *           DC_InvertXform
 *
 * Computes the inverse of the transformation xformSrc and stores it to
 * xformDest. Returns TRUE if successful or FALSE if the xformSrc matrix
 * is singular.
 */
static BOOL DC_InvertXform( const XFORM *xformSrc, XFORM *xformDest )
{
    double determinant;

    determinant = xformSrc->eM11*xformSrc->eM22 -
        xformSrc->eM12*xformSrc->eM21;
    if (determinant > -1e-12 && determinant < 1e-12)
        return FALSE;

    xformDest->eM11 =  xformSrc->eM22 / determinant;
    xformDest->eM12 = -xformSrc->eM12 / determinant;
    xformDest->eM21 = -xformSrc->eM21 / determinant;
    xformDest->eM22 =  xformSrc->eM11 / determinant;
    xformDest->eDx  = -xformSrc->eDx * xformDest->eM11 -
                       xformSrc->eDy * xformDest->eM21;
    xformDest->eDy  = -xformSrc->eDx * xformDest->eM12 -
                       xformSrc->eDy * xformDest->eM22;

    return TRUE;
}

/* Construct a transformation to do the window-to-viewport conversion */
static void construct_window_to_viewport(DC *dc, XFORM *xform)
{
    double scaleX, scaleY;
    scaleX = (double)dc->attr->vport_ext.cx / (double)dc->attr->wnd_ext.cx;
    scaleY = (double)dc->attr->vport_ext.cy / (double)dc->attr->wnd_ext.cy;

    if (dc->attr->layout & LAYOUT_RTL) scaleX = -scaleX;
    xform->eM11 = scaleX;
    xform->eM12 = 0.0;
    xform->eM21 = 0.0;
    xform->eM22 = scaleY;
    xform->eDx  = (double)dc->attr->vport_org.x - scaleX * (double)dc->attr->wnd_org.x;
    xform->eDy  = (double)dc->attr->vport_org.y - scaleY * (double)dc->attr->wnd_org.y;
    if (dc->attr->layout & LAYOUT_RTL)
        xform->eDx = dc->attr->vis_rect.right - dc->attr->vis_rect.left - 1 - xform->eDx;
}

/***********************************************************************
 *        linear_xform_cmp
 *
 * Compares the linear transform portion of two XFORMs (i.e. the 2x2 submatrix).
 * Returns 0 if they match.
 */
static inline int linear_xform_cmp( const XFORM *a, const XFORM *b )
{
    return memcmp( a, b, FIELD_OFFSET( XFORM, eDx ) );
}

/***********************************************************************
 *           DC_UpdateXforms
 *
 * Updates the xformWorld2Vport, xformVport2World and vport2WorldValid
 * fields of the specified DC by creating a transformation that
 * represents the current mapping mode and combining it with the DC's
 * world transform. This function should be called whenever the
 * parameters associated with the mapping mode (window and viewport
 * extents and origins) or the world transform change.
 */
void DC_UpdateXforms( DC *dc )
{
    XFORM xformWnd2Vport, oldworld2vport;

    construct_window_to_viewport(dc, &xformWnd2Vport);

    oldworld2vport = dc->xformWorld2Vport;
    /* Combine with the world transformation */
    combine_transform( &dc->xformWorld2Vport, &dc->xformWorld2Wnd, &xformWnd2Vport );

    /* Create inverse of world-to-viewport transformation */
    dc->vport2WorldValid = DC_InvertXform( &dc->xformWorld2Vport,
        &dc->xformVport2World );

    /* Reselect the font and pen back into the dc so that the size
       gets updated. */
    if (linear_xform_cmp( &oldworld2vport, &dc->xformWorld2Vport ) &&
        get_gdi_object_type( dc->hSelf ) != NTGDI_OBJ_METADC)
    {
        NtGdiSelectFont(dc->hSelf, dc->hFont);
        NtGdiSelectPen(dc->hSelf, dc->hPen);
    }
}


/***********************************************************************
 *           reset_dc_state
 */
static BOOL reset_dc_state( HDC hdc )
{
    DC *dc, *dcs, *next;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;

    set_initial_dc_state( dc );
    set_bk_color( dc, RGB( 255, 255, 255 ));
    set_text_color( dc, RGB( 0, 0, 0 ));
    NtGdiSelectBrush( hdc, GetStockObject( WHITE_BRUSH ));
    NtGdiSelectFont( hdc, GetStockObject( SYSTEM_FONT ));
    NtGdiSelectPen( hdc, GetStockObject( BLACK_PEN ));
    NtGdiSetVirtualResolution( hdc, 0, 0, 0, 0 );
    NtUserSelectPalette( hdc, GetStockObject( DEFAULT_PALETTE ), FALSE );
    NtGdiSetBoundsRect( hdc, NULL, DCB_DISABLE );
    NtGdiAbortPath( hdc );

    if (dc->hClipRgn) NtGdiDeleteObjectApp( dc->hClipRgn );
    if (dc->hMetaRgn) NtGdiDeleteObjectApp( dc->hMetaRgn );
    dc->hClipRgn = 0;
    dc->hMetaRgn = 0;
    update_dc_clipping( dc );

    for (dcs = dc->saved_dc; dcs; dcs = next)
    {
        next = dcs->saved_dc;
        free_dc_state( dcs );
    }
    dc->saved_dc = NULL;
    dc->attr->save_level = 0;
    release_dc_ptr( dc );

    return TRUE;
}


/***********************************************************************
 *           DC_DeleteObject
 */
static BOOL DC_DeleteObject( HGDIOBJ handle )
{
    DC *dc;

    TRACE( "%p\n", handle );

    if (!(dc = get_dc_ptr( handle ))) return FALSE;
    if (dc->refcount != 1)
    {
        FIXME( "not deleting busy DC %p refcount %u\n", dc->hSelf, dc->refcount );
        release_dc_ptr( dc );
        return FALSE;
    }

    /* Call hook procedure to check whether is it OK to delete this DC,
     * gdi_lock should not be locked */
    if (dc->dce && !delete_dce( dc->dce ))
    {
        release_dc_ptr( dc );
        return TRUE;
    }
    reset_dc_state( handle );
    free_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           NtGdiSaveDC    (win32u.@)
 */
INT WINAPI NtGdiSaveDC( HDC hdc )
{
    DC *dc, *newdc;
    INT ret;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if (!(newdc = calloc( 1, sizeof(*newdc ))))
    {
        release_dc_ptr( dc );
        return 0;
    }
    if (!(newdc->attr = alloc_dc_attr() ))
    {
        free( newdc );
        release_dc_ptr( dc );
        return 0;
    }

    *newdc->attr            = *dc->attr;
    newdc->hPen             = dc->hPen;
    newdc->hBrush           = dc->hBrush;
    newdc->hFont            = dc->hFont;
    newdc->hBitmap          = dc->hBitmap;
    newdc->hPalette         = dc->hPalette;
    newdc->breakExtra       = dc->breakExtra;
    newdc->breakRem         = dc->breakRem;
    newdc->xformWorld2Wnd   = dc->xformWorld2Wnd;
    newdc->xformWorld2Vport = dc->xformWorld2Vport;
    newdc->xformVport2World = dc->xformVport2World;
    newdc->vport2WorldValid = dc->vport2WorldValid;

    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

    if (dc->hClipRgn)
    {
        newdc->hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtGdiCombineRgn( newdc->hClipRgn, dc->hClipRgn, 0, RGN_COPY );
    }
    if (dc->hMetaRgn)
    {
        newdc->hMetaRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtGdiCombineRgn( newdc->hMetaRgn, dc->hMetaRgn, 0, RGN_COPY );
    }

    if (!PATH_SavePath( newdc, dc ))
    {
        release_dc_ptr( dc );
        free_dc_state( newdc );
        return 0;
    }

    newdc->saved_dc = dc->saved_dc;
    dc->saved_dc = newdc;
    ret = ++dc->attr->save_level;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiRestoreDC    (win32u.@)
 */
BOOL WINAPI NtGdiRestoreDC( HDC hdc, INT level )
{
    DC *dc, *dcs, *first_dcs;
    INT save_level;

    TRACE("%p %d\n", hdc, level );
    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    update_dc( dc );

    /* find the state level to restore */
    if (abs(level) > dc->attr->save_level || level == 0)
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    if (level < 0) level = dc->attr->save_level + level + 1;
    first_dcs = dc->saved_dc;
    for (dcs = first_dcs, save_level = dc->attr->save_level; save_level > level; save_level--)
        dcs = dcs->saved_dc;

    /* restore the state */

    if (!PATH_RestorePath( dc, dcs ))
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    dc->attr->layout           = dcs->attr->layout;
    dc->attr->rop_mode         = dcs->attr->rop_mode;
    dc->attr->poly_fill_mode   = dcs->attr->poly_fill_mode;
    dc->attr->stretch_blt_mode = dcs->attr->stretch_blt_mode;
    dc->attr->rel_abs_mode     = dcs->attr->rel_abs_mode;
    dc->attr->background_mode  = dcs->attr->background_mode;
    dc->attr->background_color = dcs->attr->background_color;
    dc->attr->text_color       = dcs->attr->text_color;
    dc->attr->brush_color      = dcs->attr->brush_color;
    dc->attr->pen_color        = dcs->attr->pen_color;
    dc->attr->brush_org        = dcs->attr->brush_org;
    dc->attr->mapper_flags     = dcs->attr->mapper_flags;
    dc->attr->text_align       = dcs->attr->text_align;
    dc->attr->char_extra       = dcs->attr->char_extra;
    dc->attr->map_mode         = dcs->attr->map_mode;
    dc->attr->graphics_mode    = dcs->attr->graphics_mode;
    dc->attr->cur_pos          = dcs->attr->cur_pos;
    dc->attr->arc_direction    = dcs->attr->arc_direction;
    dc->attr->wnd_org          = dcs->attr->wnd_org;
    dc->attr->wnd_ext          = dcs->attr->wnd_ext;
    dc->attr->vport_org        = dcs->attr->vport_org;
    dc->attr->vport_ext        = dcs->attr->vport_ext;
    dc->attr->virtual_res      = dcs->attr->virtual_res;
    dc->attr->virtual_size     = dcs->attr->virtual_size;

    dc->breakExtra       = dcs->breakExtra;
    dc->breakRem         = dcs->breakRem;
    dc->xformWorld2Wnd   = dcs->xformWorld2Wnd;
    dc->xformWorld2Vport = dcs->xformWorld2Vport;
    dc->xformVport2World = dcs->xformVport2World;
    dc->vport2WorldValid = dcs->vport2WorldValid;

    if (dcs->hClipRgn)
    {
        if (!dc->hClipRgn) dc->hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtGdiCombineRgn( dc->hClipRgn, dcs->hClipRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hClipRgn) NtGdiDeleteObjectApp( dc->hClipRgn );
        dc->hClipRgn = 0;
    }
    if (dcs->hMetaRgn)
    {
        if (!dc->hMetaRgn) dc->hMetaRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtGdiCombineRgn( dc->hMetaRgn, dcs->hMetaRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hMetaRgn) NtGdiDeleteObjectApp( dc->hMetaRgn );
        dc->hMetaRgn = 0;
    }
    DC_UpdateXforms( dc );
    update_dc_clipping( dc );

    NtGdiSelectBitmap( hdc, dcs->hBitmap );
    NtGdiSelectBrush( hdc, dcs->hBrush );
    NtGdiSelectFont( hdc, dcs->hFont );
    NtGdiSelectPen( hdc, dcs->hPen );
    set_bk_color( dc, dcs->attr->background_color);
    set_text_color( dc, dcs->attr->text_color);
    NtUserSelectPalette( hdc, dcs->hPalette, FALSE );

    dc->saved_dc  = dcs->saved_dc;
    dcs->saved_dc = 0;
    dc->attr->save_level = save_level - 1;

    /* now destroy all the saved DCs */

    while (first_dcs)
    {
        DC *next = first_dcs->saved_dc;
        free_dc_state( first_dcs );
        first_dcs = next;
    }
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           NtGdiOpenDCW    (win32u.@)
 */
HDC WINAPI NtGdiOpenDCW( UNICODE_STRING *device, const DEVMODEW *devmode, UNICODE_STRING *output,
                         ULONG type, BOOL is_display, HANDLE hspool, DRIVER_INFO_2W *driver_info,
                         void *pdev )
{
    const struct gdi_dc_funcs *funcs = NULL;
    HDC hdc;
    DC * dc;

    /* gdi_lock should not be locked */
    if (is_display)
        funcs = get_display_driver();
    else if (type != WINE_GDI_DRIVER_VERSION)
        ERR( "version mismatch: %u\n", type );
    else
        funcs = hspool;
    if (!funcs)
    {
        ERR( "no driver found\n" );
        return 0;
    }

    if (!(dc = alloc_dc_ptr( NTGDI_OBJ_DC ))) return 0;
    hdc = dc->hSelf;
    if (is_display)
        dc->hBitmap = get_display_bitmap();
    else
        dc->hBitmap = GDI_inc_ref_count( GetStockObject( DEFAULT_BITMAP ));

    TRACE("(device=%s, output=%s): returning %p\n",
          debugstr_us(device), debugstr_us(output), dc->hSelf );

    if (funcs->pCreateDC)
    {
        if (!funcs->pCreateDC( &dc->physDev, device ? device->Buffer : NULL,
                               output ? output->Buffer : NULL, devmode ))
        {
            WARN("creation aborted by device\n" );
            free_dc_ptr( dc );
            return 0;
        }
    }

    if (is_display && device)
    {
        memcpy( dc->display, device->Buffer, device->Length );
        dc->display[device->Length / sizeof(WCHAR)] = 0;

    }

    dc->attr->vis_rect.left   = 0;
    dc->attr->vis_rect.top    = 0;
    dc->attr->vis_rect.right  = NtGdiGetDeviceCaps( hdc, DESKTOPHORZRES );
    dc->attr->vis_rect.bottom = NtGdiGetDeviceCaps( hdc, DESKTOPVERTRES );
    dc->is_display            = !!is_display;

    DC_InitDC( dc );
    release_dc_ptr( dc );

    return hdc;
}


/***********************************************************************
 *           NtGdiCreateCompatibleDC   (win32u.@)
 */
HDC WINAPI NtGdiCreateCompatibleDC( HDC hdc )
{
    DC *dc, *origDC;
    HDC ret;
    const struct gdi_dc_funcs *funcs;
    PHYSDEV physDev = NULL;

    /* gdi_lock should not be locked */

    if (hdc)
    {
        if (!(origDC = get_dc_ptr( hdc ))) return 0;
        physDev = GET_DC_PHYSDEV( origDC, pCreateCompatibleDC );
        funcs = physDev->funcs;
        release_dc_ptr( origDC );
    }
    else funcs = get_display_driver();

    if (!(dc = alloc_dc_ptr( NTGDI_OBJ_MEMDC ))) return 0;

    TRACE("(%p): returning %p\n", hdc, dc->hSelf );

    dc->hBitmap = GDI_inc_ref_count( GetStockObject( DEFAULT_BITMAP ));
    dc->attr->vis_rect.left   = 0;
    dc->attr->vis_rect.top    = 0;
    dc->attr->vis_rect.right  = 1;
    dc->attr->vis_rect.bottom = 1;
    dc->device_rect = dc->attr->vis_rect;

    ret = dc->hSelf;

    if (funcs->pCreateCompatibleDC && !funcs->pCreateCompatibleDC( physDev, &dc->physDev ))
    {
        WARN("creation aborted by device\n");
        free_dc_ptr( dc );
        return 0;
    }

    if (!dib_driver.pCreateDC( &dc->physDev, NULL, NULL, NULL ))
    {
        free_dc_ptr( dc );
        return 0;
    }
    physDev = GET_DC_PHYSDEV( dc, pSelectBitmap );
    physDev->funcs->pSelectBitmap( physDev, dc->hBitmap );

    DC_InitDC( dc );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiResetDC    (win32u.@)
 */
BOOL WINAPI NtGdiResetDC( HDC hdc, const DEVMODEW *devmode, BOOL *banding,
                          DRIVER_INFO_2W *driver_info, void *dev )
{
    DC *dc;
    BOOL ret = FALSE;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pResetDC );
        ret = physdev->funcs->pResetDC( physdev, devmode ) != 0;
        if (ret)  /* reset the visible region */
        {
            dc->dirty = 0;
            dc->attr->vis_rect.left   = 0;
            dc->attr->vis_rect.top    = 0;
            dc->attr->vis_rect.right  = NtGdiGetDeviceCaps( hdc, DESKTOPHORZRES );
            dc->attr->vis_rect.bottom = NtGdiGetDeviceCaps( hdc, DESKTOPVERTRES );
            if (dc->hVisRgn) NtGdiDeleteObjectApp( dc->hVisRgn );
            dc->hVisRgn = 0;
            update_dc_clipping( dc );
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           NtGdiGetDeviceCaps    (win32u.@)
 */
INT WINAPI NtGdiGetDeviceCaps( HDC hdc, INT cap )
{
    DC *dc;
    INT ret = 0;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pGetDeviceCaps );
        ret = physdev->funcs->pGetDeviceCaps( physdev, cap );
        release_dc_ptr( dc );
    }
    return ret;
}


static BOOL set_graphics_mode( DC *dc, int mode )
{
    if (mode == dc->attr->graphics_mode) return TRUE;
    if (mode <= 0 || mode > GM_LAST) return FALSE;

    /* One would think that setting the graphics mode to GM_COMPATIBLE
     * would also reset the world transformation matrix to the unity
     * matrix. However, in Windows, this is not the case. This doesn't
     * make a lot of sense to me, but that's the way it is.
     */
    dc->attr->graphics_mode = mode;

    /* font metrics depend on the graphics mode */
    NtGdiSelectFont(dc->hSelf, dc->hFont);
    return TRUE;
}


DWORD set_stretch_blt_mode( HDC hdc, DWORD mode )
{
    DWORD ret;
    DC *dc;
    if (!(dc = get_dc_ptr( hdc ))) return 0;
    ret = dc->attr->stretch_blt_mode;
    dc->attr->stretch_blt_mode = mode;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiGetAndSetDCDword    (win32u.@)
 */
BOOL WINAPI NtGdiGetAndSetDCDword( HDC hdc, UINT method, DWORD value, DWORD *prev_value )
{
    PHYSDEV physdev;
    BOOL ret = TRUE;
    DWORD prev;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    switch (method)
    {
    case NtGdiSetMapMode:
        prev = dc->attr->map_mode;
        ret = set_map_mode( dc, value );
        break;

    case NtGdiSetBkColor:
        prev = dc->attr->background_color;
        set_bk_color( dc, value );
        break;

    case NtGdiSetBkMode:
        prev = dc->attr->background_mode;
        dc->attr->background_mode = value;
        break;

    case NtGdiSetTextColor:
        prev = dc->attr->text_color;
        set_text_color( dc, value );
        break;

    case NtGdiSetDCBrushColor:
        physdev = GET_DC_PHYSDEV( dc, pSetDCBrushColor );
        prev = dc->attr->brush_color;
        value = physdev->funcs->pSetDCBrushColor( physdev, value );
        if (value != CLR_INVALID) dc->attr->brush_color = value;
        break;

    case NtGdiSetDCPenColor:
        physdev = GET_DC_PHYSDEV( dc, pSetDCPenColor );
        prev = dc->attr->pen_color;
        value = physdev->funcs->pSetDCPenColor( physdev, value );
        if (value != CLR_INVALID) dc->attr->pen_color = value;
        break;

    case NtGdiSetGraphicsMode:
        prev = dc->attr->graphics_mode;
        ret = set_graphics_mode( dc, value );
        break;

    case NtGdiSetROP2:
        prev = dc->attr->rop_mode;
        dc->attr->rop_mode = value;
        break;

    case NtGdiSetTextAlign:
        prev = dc->attr->text_align;
        dc->attr->text_align = value;
        break;

    default:
        WARN( "unknown method %u\n", method );
        ret = FALSE;
        break;
    }

    release_dc_ptr( dc );
    if (!ret || !prev_value) return FALSE;
    *prev_value = prev;
    return TRUE;
}


/***********************************************************************
 *           NtGdiGetDCDword    (win32u.@)
 */
BOOL WINAPI NtGdiGetDCDword( HDC hdc, UINT method, DWORD *result )
{
    BOOL ret = TRUE;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    switch (method)
    {
    case NtGdiGetArcDirection:
        *result = dc->attr->arc_direction;
        break;

    case NtGdiGetBkColor:
        *result = dc->attr->background_color;
        break;

    case NtGdiGetBkMode:
        *result = dc->attr->background_mode;
        break;

    case NtGdiGetDCBrushColor:
        *result = dc->attr->brush_color;
        break;

    case NtGdiGetDCPenColor:
        *result = dc->attr->pen_color;
        break;

    case NtGdiGetGraphicsMode:
        *result = dc->attr->graphics_mode;
        break;

    case NtGdiGetLayout:
        *result = dc->attr->layout;
        break;

    case NtGdiGetPolyFillMode:
        *result = dc->attr->poly_fill_mode;
        break;

    case NtGdiGetROP2:
        *result = dc->attr->rop_mode;
        break;

    case NtGdiGetTextColor:
        *result = dc->attr->text_color;
        break;

    case NtGdiIsMemDC:
        *result = get_gdi_object_type( hdc ) == NTGDI_OBJ_MEMDC;
        break;

    default:
        WARN( "unknown method %u\n", method );
        ret = FALSE;
        break;
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiGetDCPoint    (win32u.@)
 */
BOOL WINAPI NtGdiGetDCPoint( HDC hdc, UINT method, POINT *result )
{
    BOOL ret = TRUE;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    switch (method)
    {
    case NtGdiGetBrushOrgEx:
        *result = dc->attr->brush_org;
        break;

    case NtGdiGetCurrentPosition:
        *result = dc->attr->cur_pos;
        break;

    case NtGdiGetDCOrg:
        result->x = dc->attr->vis_rect.left;
        result->y = dc->attr->vis_rect.top;
        break;

    default:
        WARN( "unknown method %u\n", method );
        ret = FALSE;
        break;
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiSetBrushOrg    (win32u.@)
 */
BOOL WINAPI NtGdiSetBrushOrg( HDC hdc, INT x, INT y, POINT *oldorg )
{
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (oldorg) *oldorg = dc->attr->brush_org;
    dc->attr->brush_org.x = x;
    dc->attr->brush_org.y = y;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           NtGdiGetMiterLimit  (win32u.@)
 */
BOOL WINAPI NtGdiGetMiterLimit( HDC hdc, FLOAT *limit )
{
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (limit) *limit = dc->attr->miter_limit;
    release_dc_ptr( dc );
    return TRUE;
}


/*******************************************************************
 *           NtGdiSetMiterLimit  (win32u.@)
 */
BOOL WINAPI NtGdiSetMiterLimit( HDC hdc, DWORD limit, FLOAT *old_limit )
{
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (old_limit) *old_limit = dc->attr->miter_limit;
    dc->attr->miter_limit = *(FLOAT *)&limit;
    release_dc_ptr( dc );
    return TRUE;
}


BOOL offset_viewport_org( HDC hdc, INT x, INT y, POINT *point )
{
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (point) *point = dc->attr->vport_org;
    dc->attr->vport_org.x += x;
    dc->attr->vport_org.y += y;
    release_dc_ptr( dc );
    return NtGdiComputeXformCoefficients( hdc );
}


BOOL set_viewport_org( HDC hdc, INT x, INT y, POINT *point )
{
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (point) *point = dc->attr->vport_org;
    dc->attr->vport_org.x = x;
    dc->attr->vport_org.y = y;
    release_dc_ptr( dc );
    return NtGdiComputeXformCoefficients( hdc );
}


/***********************************************************************
 *           NtGdiGetTransform    (win32u.@)
 *
 * Undocumented
 *
 * Returns one of the co-ordinate space transforms
 *
 * PARAMS
 *    hdc   [I] Device context.
 *    which [I] Which xform to return:
 *                  0x203 World -> Page transform (that set by SetWorldTransform).
 *                  0x304 Page -> Device transform (the mapping mode transform).
 *                  0x204 World -> Device transform (the combination of the above two).
 *                  0x402 Device -> World transform (the inversion of the above).
 *    xform [O] The xform.
 *
 */
BOOL WINAPI NtGdiGetTransform( HDC hdc, DWORD which, XFORM *xform )
{
    BOOL ret = TRUE;
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    switch(which)
    {
    case 0x203:
        *xform = dc->xformWorld2Wnd;
        break;

    case 0x304:
        construct_window_to_viewport(dc, xform);
        break;

    case 0x204:
        *xform = dc->xformWorld2Vport;
        break;

    case 0x402:
        *xform = dc->xformVport2World;
        break;

    default:
        FIXME("Unknown code %x\n", which);
        ret = FALSE;
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           set_dc_dce
 */
void set_dc_dce( HDC hdc, struct dce *dce )
{
    DC *dc;

    if (!(dc = get_dc_obj( hdc ))) return;
    if (dc->attr->disabled)
    {
        GDI_ReleaseObj( hdc );
        return;
    }
    dc->dce = dce;
    if (dce) dc->dirty = 1;
    GDI_ReleaseObj( hdc );
}


/***********************************************************************
 *           get_dc_dce
 */
struct dce *get_dc_dce( HDC hdc )
{
    DC *dc = get_dc_obj( hdc );
    struct dce *ret = NULL;

    if (!dc) return 0;
    if (!dc->attr->disabled) ret = dc->dce;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           set_dce_flags
 */
WORD set_dce_flags( HDC hdc, WORD flags )
{
    DC *dc = get_dc_obj( hdc );  /* not get_dc_ptr, this needs to work from any thread */
    LONG ret = 0;

    if (!dc) return 0;

    TRACE("hDC %p, flags %04x\n",hdc,flags);

    if (flags & DCHF_INVALIDATEVISRGN)
        ret = InterlockedExchange( &dc->dirty, 1 );
    else if (flags & DCHF_VALIDATEVISRGN || !flags)
        ret = InterlockedExchange( &dc->dirty, 0 );

    if (flags & DCHF_DISABLEDC)
        ret = InterlockedExchange( &dc->attr->disabled, 1 );
    else if (flags & DCHF_ENABLEDC)
        ret = InterlockedExchange( &dc->attr->disabled, 0 );

    GDI_ReleaseObj( hdc );

    if (flags & DCHF_RESETDC) ret = reset_dc_state( hdc );
    return ret;
}

/***********************************************************************
 *           NtGdiGetDeviceGammaRamp    (win32u.@)
 */
BOOL WINAPI NtGdiGetDeviceGammaRamp( HDC hdc, void *ptr )
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    TRACE("%p, %p\n", hdc, ptr);
    if( dc )
    {
        if (get_gdi_object_type( hdc ) != NTGDI_OBJ_MEMDC)
        {
            PHYSDEV physdev = GET_DC_PHYSDEV( dc, pGetDeviceGammaRamp );
            ret = physdev->funcs->pGetDeviceGammaRamp( physdev, ptr );
        }
        else RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
	release_dc_ptr( dc );
    }
    return ret;
}

static BOOL check_gamma_ramps(void *ptr)
{
    WORD *ramp = ptr;

    while (ramp < (WORD*)ptr + 3 * 256)
    {
        float r_x, r_y, r_lx, r_ly, r_d, r_v, r_e, g_avg, g_min, g_max;
        unsigned i, f, l, g_n, c;

        f = ramp[0];
        l = ramp[255];
        if (f >= l)
        {
            TRACE("inverted or flat gamma ramp (%d->%d), rejected\n", f, l);
            return FALSE;
        }
        r_d = l - f;
        g_min = g_max = g_avg = 0.0;

        /* check gamma ramp entries to estimate the gamma */
        TRACE("analyzing gamma ramp (%d->%d)\n", f, l);
        for (i=1, g_n=0; i<255; i++)
        {
            if (ramp[i] < f || ramp[i] > l)
            {
                TRACE("strange gamma ramp ([%d]=%d for %d->%d), rejected\n", i, ramp[i], f, l);
                return FALSE;
            }
            c = ramp[i] - f;
            if (!c) continue; /* avoid log(0) */

            /* normalize entry values into 0..1 range */
            r_x = i/255.0; r_y = c / r_d;
            /* compute logarithms of values */
            r_lx = log(r_x); r_ly = log(r_y);
            /* compute gamma for this entry */
            r_v = r_ly / r_lx;
            /* compute differential (error estimate) for this entry */
            /* some games use table-based logarithms that magnifies the error by 128 */
            r_e = -r_lx * 128 / (c * r_lx * r_lx);

            /* compute min & max while compensating for estimated error */
            if (!g_n || g_min > (r_v + r_e)) g_min = r_v + r_e;
            if (!g_n || g_max < (r_v - r_e)) g_max = r_v - r_e;

            /* add to average */
            g_avg += r_v;
            g_n++;
        }

        if (!g_n)
        {
            TRACE("no gamma data, shouldn't happen\n");
            return FALSE;
        }
        g_avg /= g_n;
        TRACE("low bias is %d, high is %d, gamma is %5.3f\n", f, 65535-l, g_avg);

        /* check that the gamma is reasonably uniform across the ramp */
        if (g_max - g_min > 12.8)
        {
            TRACE("ramp not uniform (max=%f, min=%f, avg=%f), rejected\n", g_max, g_min, g_avg);
            return FALSE;
        }

        /* check that the gamma is not too bright */
        if (g_avg < 0.2)
        {
            TRACE("too bright gamma ( %5.3f), rejected\n", g_avg);
            return FALSE;
        }

        ramp += 256;
    }

    return TRUE;
}

/***********************************************************************
 *           NtGdiSetDeviceGammaRamp    (win32u.@)
 */
BOOL WINAPI NtGdiSetDeviceGammaRamp( HDC hdc, void *ptr )
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    TRACE( "%p, %p\n", hdc, ptr );
    if( dc )
    {
        if (get_gdi_object_type( hdc ) != NTGDI_OBJ_MEMDC)
        {
            PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetDeviceGammaRamp );

            if (check_gamma_ramps(ptr))
                ret = physdev->funcs->pSetDeviceGammaRamp( physdev, ptr );
        }
        else RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
	release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           NtGdiGetBoundsRect    (win32u.@)
 */
UINT WINAPI NtGdiGetBoundsRect( HDC hdc, RECT *rect, UINT flags )
{
    PHYSDEV physdev;
    RECT device_rect;
    UINT ret;
    DC *dc = get_dc_ptr( hdc );

    if ( !dc ) return 0;

    physdev = GET_DC_PHYSDEV( dc, pGetBoundsRect );
    ret = physdev->funcs->pGetBoundsRect( physdev, &device_rect, DCB_RESET );
    if (!ret)
    {
        release_dc_ptr( dc );
        return 0;
    }
    if (dc->bounds_enabled && ret == DCB_SET) add_bounds_rect( &dc->bounds, &device_rect );

    if (rect)
    {
        if (IsRectEmpty( &dc->bounds ))
        {
            rect->left = rect->top = rect->right = rect->bottom = 0;
            ret = DCB_RESET;
        }
        else
        {
            *rect = dc->bounds;
            rect->left   = max( rect->left, 0 );
            rect->top    = max( rect->top, 0 );
            rect->right  = min( rect->right, dc->attr->vis_rect.right - dc->attr->vis_rect.left );
            rect->bottom = min( rect->bottom, dc->attr->vis_rect.bottom - dc->attr->vis_rect.top );
            ret = DCB_SET;
        }
        dp_to_lp( dc, (POINT *)rect, 2 );
    }
    else ret = 0;

    if (flags & DCB_RESET) reset_bounds( &dc->bounds );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiSetBoundsRect    (win32u.@)
 */
UINT WINAPI NtGdiSetBoundsRect( HDC hdc, const RECT *rect, UINT flags )
{
    PHYSDEV physdev;
    UINT ret;
    DC *dc;

    if ((flags & DCB_ENABLE) && (flags & DCB_DISABLE)) return 0;
    if (!(dc = get_dc_ptr( hdc ))) return 0;

    physdev = GET_DC_PHYSDEV( dc, pSetBoundsRect );
    ret = physdev->funcs->pSetBoundsRect( physdev, &dc->bounds, flags );
    if (!ret)
    {
        release_dc_ptr( dc );
        return 0;
    }

    ret = (dc->bounds_enabled ? DCB_ENABLE : DCB_DISABLE) |
          (IsRectEmpty( &dc->bounds ) ? ret & DCB_SET : DCB_SET);

    if (flags & DCB_RESET) reset_bounds( &dc->bounds );

    if ((flags & DCB_ACCUMULATE) && rect)
    {
        RECT rc = *rect;

        lp_to_dp( dc, (POINT *)&rc, 2 );
        add_bounds_rect( &dc->bounds, &rc );
    }

    if (flags & DCB_ENABLE) dc->bounds_enabled = TRUE;
    if (flags & DCB_DISABLE) dc->bounds_enabled = FALSE;

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           NtGdiSetLayout    (win32u.@)
 *
 * Sets left->right or right->left text layout flags of a dc.
 *
 */
DWORD WINAPI NtGdiSetLayout( HDC hdc, LONG wox, DWORD layout )
{
    UINT old_layout = GDI_ERROR;
    DC *dc;

    if ((dc = get_dc_ptr( hdc )))
    {
        old_layout = dc->attr->layout;
        dc->attr->layout = layout;
        if (layout != old_layout)
        {
            if (layout & LAYOUT_RTL) dc->attr->map_mode = MM_ANISOTROPIC;
            DC_UpdateXforms( dc );
        }
        release_dc_ptr( dc );
    }

    TRACE("hdc : %p, old layout : %08x, new layout : %08x\n", hdc, old_layout, layout);

    return old_layout;
}

/**********************************************************************
 *           __wine_get_icm_profile     (win32u.@)
 */
BOOL WINAPI __wine_get_icm_profile( HDC hdc, DWORD *size, WCHAR *filename )
{
    PHYSDEV physdev;
    DC *dc;
    BOOL ret;

    if (!(dc = get_dc_ptr(hdc))) return FALSE;

    physdev = GET_DC_PHYSDEV( dc, pGetICMProfile );
    ret = physdev->funcs->pGetICMProfile( physdev, size, filename );
    release_dc_ptr(dc);
    return ret;
}
