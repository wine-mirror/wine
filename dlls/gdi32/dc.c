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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winnls.h"
#include "winternl.h"
#include "winerror.h"
#include "ntgdi_private.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);

static BOOL DC_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs dc_funcs =
{
    NULL,             /* pGetObjectW */
    NULL,             /* pUnrealizeObject */
    DC_DeleteObject   /* pDeleteObject */
};


static inline DC *get_dc_obj( HDC hdc )
{
    WORD type;
    DC *dc = get_any_obj_ptr( hdc, &type );
    if (!dc) return NULL;

    switch (type)
    {
    case NTGDI_OBJ_DC:
    case NTGDI_OBJ_MEMDC:
    case NTGDI_OBJ_METADC:
    case NTGDI_OBJ_ENHMETADC:
        return dc;
    default:
        GDI_ReleaseObj( hdc );
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
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
    dc->font_code_page      = CP_ACP;
    dc->attr->rop_mode      = R2_COPYPEN;
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
DC *alloc_dc_ptr( WORD magic )
{
    DC *dc;

    if (!(dc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dc) ))) return NULL;
    if (!(dc->attr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dc->attr))))
    {
        HeapFree( GetProcessHeap(), 0, dc->attr );
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
        HeapFree( GetProcessHeap(), 0, dc->attr );
        HeapFree( GetProcessHeap(), 0, dc );
        return NULL;
    }
    dc->attr->hdc = dc->nulldrv.hdc = dc->hSelf;
    set_gdi_client_ptr( dc->hSelf, dc->attr );

    if (!font_driver.pCreateDC( &dc->physDev, NULL, NULL, NULL, NULL ))
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
    if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
    if (dc->hMetaRgn) DeleteObject( dc->hMetaRgn );
    if (dc->hVisRgn) DeleteObject( dc->hVisRgn );
    if (dc->region) DeleteObject( dc->region );
    if (dc->path) free_gdi_path( dc->path );
    HeapFree( GetProcessHeap(), 0, dc->attr );
    HeapFree( GetProcessHeap(), 0, dc );
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
    if (dc->hBitmap) GDI_dec_ref_count( dc->hBitmap );
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


/***********************************************************************
 *           update_dc
 *
 * Make sure the DC vis region is up to date.
 * This function may call up to USER so the GDI lock should _not_
 * be held when calling it.
 */
void update_dc( DC *dc )
{
    if (InterlockedExchange( &dc->dirty, 0 ) && dc->hookProc)
        dc->hookProc( dc->hSelf, DCHC_INVALIDVISRGN, dc->dwHookData, 0 );
}


/***********************************************************************
 *           DC_DeleteObject
 */
static BOOL DC_DeleteObject( HGDIOBJ handle )
{
    return DeleteDC( handle );
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
    SetTextColor( dc->hSelf, dc->attr->text_color );
    SetBkColor( dc->hSelf, dc->attr->background_color );
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
    CombineTransform( &dc->xformWorld2Vport, &dc->xformWorld2Wnd,
        &xformWnd2Vport );

    /* Create inverse of world-to-viewport transformation */
    dc->vport2WorldValid = DC_InvertXform( &dc->xformWorld2Vport,
        &dc->xformVport2World );

    /* Reselect the font and pen back into the dc so that the size
       gets updated. */
    if (linear_xform_cmp( &oldworld2vport, &dc->xformWorld2Vport ) &&
        GetObjectType( dc->hSelf ) != OBJ_METADC)
    {
        NtGdiSelectFont(dc->hSelf, dc->hFont);
        NtGdiSelectPen(dc->hSelf, dc->hPen);
    }
}


/***********************************************************************
 *           nulldrv_RestoreDC
 */
BOOL CDECL nulldrv_RestoreDC( PHYSDEV dev, INT level )
{
    DC *dcs, *first_dcs, *dc = get_nulldrv_dc( dev );
    INT save_level;

    /* find the state level to restore */

    if (abs(level) > dc->saveLevel || level == 0) return FALSE;
    if (level < 0) level = dc->saveLevel + level + 1;
    first_dcs = dc->saved_dc;
    for (dcs = first_dcs, save_level = dc->saveLevel; save_level > level; save_level--)
        dcs = dcs->saved_dc;

    /* restore the state */

    if (!PATH_RestorePath( dc, dcs )) return FALSE;

    dc->attr->layout     = dcs->attr->layout;
    dc->attr->rop_mode   = dcs->attr->rop_mode;
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
    dc->breakExtra       = dcs->breakExtra;
    dc->breakRem         = dcs->breakRem;
    dc->attr->map_mode         = dcs->attr->map_mode;
    dc->attr->graphics_mode    = dcs->attr->graphics_mode;
    dc->attr->cur_pos          = dcs->attr->cur_pos;
    dc->attr->arc_direction    = dcs->attr->arc_direction;
    dc->xformWorld2Wnd   = dcs->xformWorld2Wnd;
    dc->xformWorld2Vport = dcs->xformWorld2Vport;
    dc->xformVport2World = dcs->xformVport2World;
    dc->vport2WorldValid = dcs->vport2WorldValid;
    dc->attr->wnd_org          = dcs->attr->wnd_org;
    dc->attr->wnd_ext          = dcs->attr->wnd_ext;
    dc->attr->vport_org        = dcs->attr->vport_org;
    dc->attr->vport_ext  = dcs->attr->vport_ext;
    dc->attr->virtual_res      = dcs->attr->virtual_res;
    dc->attr->virtual_size     = dcs->attr->virtual_size;

    if (dcs->hClipRgn)
    {
        if (!dc->hClipRgn) dc->hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtGdiCombineRgn( dc->hClipRgn, dcs->hClipRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
        dc->hClipRgn = 0;
    }
    if (dcs->hMetaRgn)
    {
        if (!dc->hMetaRgn) dc->hMetaRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtGdiCombineRgn( dc->hMetaRgn, dcs->hMetaRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hMetaRgn) DeleteObject( dc->hMetaRgn );
        dc->hMetaRgn = 0;
    }
    DC_UpdateXforms( dc );
    update_dc_clipping( dc );

    NtGdiSelectBitmap( dev->hdc, dcs->hBitmap );
    NtGdiSelectBrush( dev->hdc, dcs->hBrush );
    NtGdiSelectFont( dev->hdc, dcs->hFont );
    NtGdiSelectPen( dev->hdc, dcs->hPen );
    SetBkColor( dev->hdc, dcs->attr->background_color);
    SetTextColor( dev->hdc, dcs->attr->text_color);
    GDISelectPalette( dev->hdc, dcs->hPalette, FALSE );

    dc->saved_dc  = dcs->saved_dc;
    dcs->saved_dc = 0;
    dc->saveLevel = save_level - 1;

    /* now destroy all the saved DCs */

    while (first_dcs)
    {
        DC *next = first_dcs->saved_dc;
        free_dc_state( first_dcs );
        first_dcs = next;
    }
    return TRUE;
}


/***********************************************************************
 *           reset_dc_state
 */
static BOOL reset_dc_state( HDC hdc )
{
    DC *dc, *dcs, *next;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;

    set_initial_dc_state( dc );
    SetBkColor( hdc, RGB( 255, 255, 255 ));
    SetTextColor( hdc, RGB( 0, 0, 0 ));
    NtGdiSelectBrush( hdc, GetStockObject( WHITE_BRUSH ));
    NtGdiSelectFont( hdc, GetStockObject( SYSTEM_FONT ));
    NtGdiSelectPen( hdc, GetStockObject( BLACK_PEN ));
    NtGdiSetVirtualResolution( hdc, 0, 0, 0, 0 );
    GDISelectPalette( hdc, GetStockObject( DEFAULT_PALETTE ), FALSE );
    NtGdiSetBoundsRect( hdc, NULL, DCB_DISABLE );
    AbortPath( hdc );

    if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
    if (dc->hMetaRgn) DeleteObject( dc->hMetaRgn );
    dc->hClipRgn = 0;
    dc->hMetaRgn = 0;
    update_dc_clipping( dc );

    for (dcs = dc->saved_dc; dcs; dcs = next)
    {
        next = dcs->saved_dc;
        free_dc_state( dcs );
    }
    dc->saved_dc = NULL;
    dc->saveLevel = 0;
    release_dc_ptr( dc );
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

    if (!(newdc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*newdc ))))
    {
        release_dc_ptr( dc );
        return 0;
    }
    if (!(newdc->attr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*newdc->attr) )))
    {
        HeapFree( GetProcessHeap(), 0, newdc );
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
    ret = ++dc->saveLevel;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           RestoreDC    (GDI32.@)
 */
BOOL WINAPI RestoreDC( HDC hdc, INT level )
{
    PHYSDEV physdev;
    DC *dc;
    BOOL success = FALSE;

    TRACE("%p %d\n", hdc, level );
    if ((dc = get_dc_ptr( hdc )))
    {
        update_dc( dc );
        physdev = GET_DC_PHYSDEV( dc, pRestoreDC );
        success = physdev->funcs->pRestoreDC( physdev, level );
        release_dc_ptr( dc );
    }
    return success;
}


/***********************************************************************
 *           CreateDCW    (GDI32.@)
 */
HDC WINAPI CreateDCW( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                      const DEVMODEW *initData )
{
    const WCHAR *display, *p;
    HDC hdc;
    DC * dc;
    const struct gdi_dc_funcs *funcs;
    WCHAR buf[300];

    GDI_CheckNotLock();

    if (!device || !DRIVER_GetDriverName( device, buf, 300 ))
    {
        if (!driver)
        {
            ERR( "no device found for %s\n", debugstr_w(device) );
            return 0;
        }
        lstrcpyW(buf, driver);
    }

    if (!(funcs = DRIVER_load_driver( buf )))
    {
        ERR( "no driver found for %s\n", debugstr_w(buf) );
        return 0;
    }
    if (!(dc = alloc_dc_ptr( NTGDI_OBJ_DC ))) return 0;
    hdc = dc->hSelf;

    dc->hBitmap = GDI_inc_ref_count( GetStockObject( DEFAULT_BITMAP ));

    TRACE("(driver=%s, device=%s, output=%s): returning %p\n",
          debugstr_w(driver), debugstr_w(device), debugstr_w(output), dc->hSelf );

    if (funcs->pCreateDC)
    {
        if (!funcs->pCreateDC( &dc->physDev, buf, device, output, initData ))
        {
            WARN("creation aborted by device\n" );
            free_dc_ptr( dc );
            return 0;
        }
    }

    if (is_display_device( driver ))
        display = driver;
    else if (is_display_device( device ))
        display = device;
    else
        display = NULL;

    if (display)
    {
        /* Copy only the display name. For example, \\.\DISPLAY1 in \\.\DISPLAY1\Monitor0 */
        p = display + 12;
        while (iswdigit( *p ))
            ++p;
        lstrcpynW( dc->display, display, p - display + 1 );
        dc->display[p - display] = '\0';
    }

    dc->attr->vis_rect.left   = 0;
    dc->attr->vis_rect.top    = 0;
    dc->attr->vis_rect.right  = GetDeviceCaps( hdc, DESKTOPHORZRES );
    dc->attr->vis_rect.bottom = GetDeviceCaps( hdc, DESKTOPVERTRES );

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

    GDI_CheckNotLock();

    if (hdc)
    {
        if (!(origDC = get_dc_ptr( hdc ))) return 0;
        physDev = GET_DC_PHYSDEV( origDC, pCreateCompatibleDC );
        funcs = physDev->funcs;
        release_dc_ptr( origDC );
    }
    else funcs = DRIVER_load_driver( L"display" );

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

    if (!dib_driver.pCreateDC( &dc->physDev, NULL, NULL, NULL, NULL ))
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
 *           DeleteDC    (GDI32.@)
 */
BOOL WINAPI DeleteDC( HDC hdc )
{
    DC * dc;

    TRACE("%p\n", hdc );

    GDI_CheckNotLock();

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (dc->refcount != 1)
    {
        FIXME( "not deleting busy DC %p refcount %u\n", dc->hSelf, dc->refcount );
        release_dc_ptr( dc );
        return FALSE;
    }

    /* Call hook procedure to check whether is it OK to delete this DC */
    if (dc->hookProc && !dc->hookProc( dc->hSelf, DCHC_DELETEDC, dc->dwHookData, 0 ))
    {
        release_dc_ptr( dc );
        return TRUE;
    }
    reset_dc_state( hdc );
    free_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           ResetDCW    (GDI32.@)
 */
HDC WINAPI ResetDCW( HDC hdc, const DEVMODEW *devmode )
{
    DC *dc;
    HDC ret = 0;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pResetDC );
        ret = physdev->funcs->pResetDC( physdev, devmode );
        if (ret)  /* reset the visible region */
        {
            dc->dirty = 0;
            dc->attr->vis_rect.left   = 0;
            dc->attr->vis_rect.top    = 0;
            dc->attr->vis_rect.right  = GetDeviceCaps( hdc, DESKTOPHORZRES );
            dc->attr->vis_rect.bottom = GetDeviceCaps( hdc, DESKTOPVERTRES );
            if (dc->hVisRgn) DeleteObject( dc->hVisRgn );
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


/***********************************************************************
 *           SetBkColor    (GDI32.@)
 */
COLORREF WINAPI SetBkColor( HDC hdc, COLORREF color )
{
    COLORREF ret = CLR_INVALID;
    DC * dc = get_dc_ptr( hdc );

    TRACE("hdc=%p color=0x%08x\n", hdc, color);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetBkColor );
        ret = dc->attr->background_color;
        dc->attr->background_color = physdev->funcs->pSetBkColor( physdev, color );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetTextColor    (GDI32.@)
 */
COLORREF WINAPI SetTextColor( HDC hdc, COLORREF color )
{
    COLORREF ret = CLR_INVALID;
    DC * dc = get_dc_ptr( hdc );

    TRACE(" hdc=%p color=0x%08x\n", hdc, color);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetTextColor );
        ret = dc->attr->text_color;
        dc->attr->text_color = physdev->funcs->pSetTextColor( physdev, color );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           NtGdiGetAndSetDCDword    (win32u.@)
 */
BOOL WINAPI NtGdiGetAndSetDCDword( HDC hdc, UINT method, DWORD value, DWORD *prev_value )
{
    BOOL ret;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    switch (method)
    {
    case NtGdiSetMapMode:
        *prev_value = dc->attr->map_mode;
        ret = set_map_mode( dc, value );
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
 *           SetGraphicsMode    (GDI32.@)
 */
INT WINAPI SetGraphicsMode( HDC hdc, INT mode )
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hdc );

    /* One would think that setting the graphics mode to GM_COMPATIBLE
     * would also reset the world transformation matrix to the unity
     * matrix. However, in Windows, this is not the case. This doesn't
     * make a lot of sense to me, but that's the way it is.
     */
    if (!dc) return 0;
    if ((mode > 0) && (mode <= GM_LAST))
    {
        ret = dc->attr->graphics_mode;
        dc->attr->graphics_mode = mode;
    }
    /* font metrics depend on the graphics mode */
    if (ret != mode) NtGdiSelectFont(dc->hSelf, dc->hFont);
    release_dc_ptr( dc );
    return ret;
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


/****************************************************************************
 * CombineTransform [GDI32.@]
 * Combines two transformation matrices.
 *
 * PARAMS
 *    xformResult [O] Stores the result of combining the two matrices
 *    xform1      [I] Specifies the first matrix to apply
 *    xform2      [I] Specifies the second matrix to apply
 *
 * REMARKS
 *    The same matrix can be passed in for more than one of the parameters.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI CombineTransform( LPXFORM xformResult, const XFORM *xform1,
    const XFORM *xform2 )
{
    XFORM xformTemp;

    /* Check for illegal parameters */
    if (!xformResult || !xform1 || !xform2)
        return FALSE;

    /* Create the result in a temporary XFORM, since xformResult may be
     * equal to xform1 or xform2 */
    xformTemp.eM11 = xform1->eM11 * xform2->eM11 +
                     xform1->eM12 * xform2->eM21;
    xformTemp.eM12 = xform1->eM11 * xform2->eM12 +
                     xform1->eM12 * xform2->eM22;
    xformTemp.eM21 = xform1->eM21 * xform2->eM11 +
                     xform1->eM22 * xform2->eM21;
    xformTemp.eM22 = xform1->eM21 * xform2->eM12 +
                     xform1->eM22 * xform2->eM22;
    xformTemp.eDx  = xform1->eDx  * xform2->eM11 +
                     xform1->eDy  * xform2->eM21 +
                     xform2->eDx;
    xformTemp.eDy  = xform1->eDx  * xform2->eM12 +
                     xform1->eDy  * xform2->eM22 +
                     xform2->eDy;

    /* Copy the result to xformResult */
    *xformResult = xformTemp;

    return TRUE;
}


/***********************************************************************
 *           SetDCHook   (GDI32.@)
 *
 * Note: this doesn't exist in Win32, we add it here because user32 needs it.
 */
BOOL WINAPI SetDCHook( HDC hdc, DCHOOKPROC hookProc, DWORD_PTR dwHookData )
{
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;

    dc->dwHookData = dwHookData;
    dc->hookProc = hookProc;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           GetDCHook   (GDI32.@)
 *
 * Note: this doesn't exist in Win32, we add it here because user32 needs it.
 */
DWORD_PTR WINAPI GetDCHook( HDC hdc, DCHOOKPROC *proc )
{
    DC *dc = get_dc_ptr( hdc );
    DWORD_PTR ret;

    if (!dc) return 0;
    if (proc) *proc = dc->hookProc;
    ret = dc->dwHookData;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetHookFlags   (GDI32.@)
 *
 * Note: this doesn't exist in Win32, we add it here because user32 needs it.
 */
WORD WINAPI SetHookFlags( HDC hdc, WORD flags )
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
        if (GetObjectType( hdc ) != OBJ_MEMDC)
        {
            PHYSDEV physdev = GET_DC_PHYSDEV( dc, pGetDeviceGammaRamp );
            ret = physdev->funcs->pGetDeviceGammaRamp( physdev, ptr );
        }
        else SetLastError( ERROR_INVALID_PARAMETER );
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
        if (GetObjectType( hdc ) != OBJ_MEMDC)
        {
            PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetDeviceGammaRamp );

            if (check_gamma_ramps(ptr))
                ret = physdev->funcs->pSetDeviceGammaRamp( physdev, ptr );
        }
        else SetLastError( ERROR_INVALID_PARAMETER );
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
        if (is_rect_empty( &dc->bounds ))
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
          (is_rect_empty( &dc->bounds ) ? ret & DCB_SET : DCB_SET);

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
    DWORD old_layout = GDI_ERROR;
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
    }

    TRACE("hdc : %p, old layout : %08x, new layout : %08x\n", hdc, old_layout, layout);

    return old_layout;
}

/***********************************************************************
 *           SetDCBrushColor    (GDI32.@)
 */
COLORREF WINAPI SetDCBrushColor(HDC hdc, COLORREF crColor)
{
    DC *dc;
    COLORREF oldClr = CLR_INVALID;

    TRACE("hdc(%p) crColor(%08x)\n", hdc, crColor);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetDCBrushColor );
        crColor = physdev->funcs->pSetDCBrushColor( physdev, crColor );
        if (crColor != CLR_INVALID)
        {
            oldClr = dc->attr->brush_color;
            dc->attr->brush_color = crColor;
        }
        release_dc_ptr( dc );
    }

    return oldClr;
}

/***********************************************************************
 *           SetDCPenColor    (GDI32.@)
 */
COLORREF WINAPI SetDCPenColor(HDC hdc, COLORREF crColor)
{
    DC *dc;
    COLORREF oldClr = CLR_INVALID;

    TRACE("hdc(%p) crColor(%08x)\n", hdc, crColor);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetDCPenColor );
        crColor = physdev->funcs->pSetDCPenColor( physdev, crColor );
        if (crColor != CLR_INVALID)
        {
            oldClr = dc->attr->pen_color;
            dc->attr->pen_color = crColor;
        }
        release_dc_ptr( dc );
    }

    return oldClr;
}
