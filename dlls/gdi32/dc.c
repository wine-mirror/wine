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

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "gdi_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);

static const WCHAR displayW[] = { 'd','i','s','p','l','a','y',0 };

static BOOL DC_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs dc_funcs =
{
    NULL,             /* pSelectObject */
    NULL,             /* pGetObjectA */
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
    case OBJ_DC:
    case OBJ_MEMDC:
    case OBJ_METADC:
    case OBJ_ENHMETADC:
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
    dc->wnd_org.x           = 0;
    dc->wnd_org.y           = 0;
    dc->wnd_ext.cx          = 1;
    dc->wnd_ext.cy          = 1;
    dc->vport_org.x         = 0;
    dc->vport_org.y         = 0;
    dc->vport_ext.cx        = 1;
    dc->vport_ext.cy        = 1;
    dc->miterLimit          = 10.0f; /* 10.0 is the default, from MSDN */
    dc->layout              = 0;
    dc->font_code_page      = CP_ACP;
    dc->ROPmode             = R2_COPYPEN;
    dc->polyFillMode        = ALTERNATE;
    dc->stretchBltMode      = BLACKONWHITE;
    dc->relAbsMode          = ABSOLUTE;
    dc->backgroundMode      = OPAQUE;
    dc->backgroundColor     = RGB( 255, 255, 255 );
    dc->dcBrushColor        = RGB( 255, 255, 255 );
    dc->dcPenColor          = RGB( 0, 0, 0 );
    dc->textColor           = RGB( 0, 0, 0 );
    dc->brush_org.x         = 0;
    dc->brush_org.y         = 0;
    dc->mapperFlags         = 0;
    dc->textAlign           = TA_LEFT | TA_TOP | TA_NOUPDATECP;
    dc->charExtra           = 0;
    dc->breakExtra          = 0;
    dc->breakRem            = 0;
    dc->MapMode             = MM_TEXT;
    dc->GraphicsMode        = GM_COMPATIBLE;
    dc->cur_pos.x           = 0;
    dc->cur_pos.y           = 0;
    dc->ArcDirection        = AD_COUNTERCLOCKWISE;
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

    dc->nulldrv.funcs       = &null_driver;
    dc->physDev             = &dc->nulldrv;
    dc->thread              = GetCurrentThreadId();
    dc->refcount            = 1;
    dc->hPen                = GDI_inc_ref_count( GetStockObject( BLACK_PEN ));
    dc->hBrush              = GDI_inc_ref_count( GetStockObject( WHITE_BRUSH ));
    dc->hFont               = GDI_inc_ref_count( GetStockObject( SYSTEM_FONT ));
    dc->hPalette            = GetStockObject( DEFAULT_PALETTE );

    set_initial_dc_state( dc );

    if (!(dc->hSelf = alloc_gdi_handle( dc, magic, &dc_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, dc );
        return NULL;
    }
    dc->nulldrv.hdc = dc->hSelf;

    if (font_driver && !font_driver->pCreateDC( &dc->physDev, NULL, NULL, NULL, NULL ))
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
    HeapFree( GetProcessHeap(), 0, dc->font_gamma_ramp );
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
    if (dc->disabled)
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
    SetTextColor( dc->hSelf, dc->textColor );
    SetBkColor( dc->hSelf, dc->backgroundColor );
    SelectObject( dc->hSelf, dc->hPen );
    SelectObject( dc->hSelf, dc->hBrush );
    SelectObject( dc->hSelf, dc->hFont );
    update_dc_clipping( dc );
    SetVirtualResolution( dc->hSelf, 0, 0, 0, 0 );
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
    scaleX = (double)dc->vport_ext.cx / (double)dc->wnd_ext.cx;
    scaleY = (double)dc->vport_ext.cy / (double)dc->wnd_ext.cy;

    if (dc->layout & LAYOUT_RTL) scaleX = -scaleX;
    xform->eM11 = scaleX;
    xform->eM12 = 0.0;
    xform->eM21 = 0.0;
    xform->eM22 = scaleY;
    xform->eDx  = (double)dc->vport_org.x - scaleX * (double)dc->wnd_org.x;
    xform->eDy  = (double)dc->vport_org.y - scaleY * (double)dc->wnd_org.y;
    if (dc->layout & LAYOUT_RTL) xform->eDx = dc->vis_rect.right - dc->vis_rect.left - 1 - xform->eDx;
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
        !GdiIsMetaFileDC(dc->hSelf))
    {
        SelectObject(dc->hSelf, dc->hFont);
        SelectObject(dc->hSelf, dc->hPen);
    }
}


/***********************************************************************
 *           nulldrv_SaveDC
 */
INT CDECL nulldrv_SaveDC( PHYSDEV dev )
{
    DC *newdc, *dc = get_nulldrv_dc( dev );

    if (!(newdc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*newdc )))) return 0;
    newdc->layout           = dc->layout;
    newdc->hPen             = dc->hPen;
    newdc->hBrush           = dc->hBrush;
    newdc->hFont            = dc->hFont;
    newdc->hBitmap          = dc->hBitmap;
    newdc->hPalette         = dc->hPalette;
    newdc->ROPmode          = dc->ROPmode;
    newdc->polyFillMode     = dc->polyFillMode;
    newdc->stretchBltMode   = dc->stretchBltMode;
    newdc->relAbsMode       = dc->relAbsMode;
    newdc->backgroundMode   = dc->backgroundMode;
    newdc->backgroundColor  = dc->backgroundColor;
    newdc->textColor        = dc->textColor;
    newdc->dcBrushColor     = dc->dcBrushColor;
    newdc->dcPenColor       = dc->dcPenColor;
    newdc->brush_org        = dc->brush_org;
    newdc->mapperFlags      = dc->mapperFlags;
    newdc->textAlign        = dc->textAlign;
    newdc->charExtra        = dc->charExtra;
    newdc->breakExtra       = dc->breakExtra;
    newdc->breakRem         = dc->breakRem;
    newdc->MapMode          = dc->MapMode;
    newdc->GraphicsMode     = dc->GraphicsMode;
    newdc->cur_pos          = dc->cur_pos;
    newdc->ArcDirection     = dc->ArcDirection;
    newdc->xformWorld2Wnd   = dc->xformWorld2Wnd;
    newdc->xformWorld2Vport = dc->xformWorld2Vport;
    newdc->xformVport2World = dc->xformVport2World;
    newdc->vport2WorldValid = dc->vport2WorldValid;
    newdc->wnd_org          = dc->wnd_org;
    newdc->wnd_ext          = dc->wnd_ext;
    newdc->vport_org        = dc->vport_org;
    newdc->vport_ext        = dc->vport_ext;
    newdc->virtual_res      = dc->virtual_res;
    newdc->virtual_size     = dc->virtual_size;

    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

    if (dc->hClipRgn)
    {
        newdc->hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( newdc->hClipRgn, dc->hClipRgn, 0, RGN_COPY );
    }
    if (dc->hMetaRgn)
    {
        newdc->hMetaRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( newdc->hMetaRgn, dc->hMetaRgn, 0, RGN_COPY );
    }

    if (!PATH_SavePath( newdc, dc ))
    {
        release_dc_ptr( dc );
        free_dc_state( newdc );
	return 0;
    }

    newdc->saved_dc = dc->saved_dc;
    dc->saved_dc = newdc;
    return ++dc->saveLevel;
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

    dc->layout           = dcs->layout;
    dc->ROPmode          = dcs->ROPmode;
    dc->polyFillMode     = dcs->polyFillMode;
    dc->stretchBltMode   = dcs->stretchBltMode;
    dc->relAbsMode       = dcs->relAbsMode;
    dc->backgroundMode   = dcs->backgroundMode;
    dc->backgroundColor  = dcs->backgroundColor;
    dc->textColor        = dcs->textColor;
    dc->dcBrushColor     = dcs->dcBrushColor;
    dc->dcPenColor       = dcs->dcPenColor;
    dc->brush_org        = dcs->brush_org;
    dc->mapperFlags      = dcs->mapperFlags;
    dc->textAlign        = dcs->textAlign;
    dc->charExtra        = dcs->charExtra;
    dc->breakExtra       = dcs->breakExtra;
    dc->breakRem         = dcs->breakRem;
    dc->MapMode          = dcs->MapMode;
    dc->GraphicsMode     = dcs->GraphicsMode;
    dc->cur_pos          = dcs->cur_pos;
    dc->ArcDirection     = dcs->ArcDirection;
    dc->xformWorld2Wnd   = dcs->xformWorld2Wnd;
    dc->xformWorld2Vport = dcs->xformWorld2Vport;
    dc->xformVport2World = dcs->xformVport2World;
    dc->vport2WorldValid = dcs->vport2WorldValid;
    dc->wnd_org          = dcs->wnd_org;
    dc->wnd_ext          = dcs->wnd_ext;
    dc->vport_org        = dcs->vport_org;
    dc->vport_ext        = dcs->vport_ext;
    dc->virtual_res      = dcs->virtual_res;
    dc->virtual_size     = dcs->virtual_size;

    if (dcs->hClipRgn)
    {
        if (!dc->hClipRgn) dc->hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( dc->hClipRgn, dcs->hClipRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
        dc->hClipRgn = 0;
    }
    if (dcs->hMetaRgn)
    {
        if (!dc->hMetaRgn) dc->hMetaRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( dc->hMetaRgn, dcs->hMetaRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hMetaRgn) DeleteObject( dc->hMetaRgn );
        dc->hMetaRgn = 0;
    }
    DC_UpdateXforms( dc );
    update_dc_clipping( dc );

    SelectObject( dev->hdc, dcs->hBitmap );
    SelectObject( dev->hdc, dcs->hBrush );
    SelectObject( dev->hdc, dcs->hFont );
    SelectObject( dev->hdc, dcs->hPen );
    SetBkColor( dev->hdc, dcs->backgroundColor);
    SetTextColor( dev->hdc, dcs->textColor);
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
    SelectObject( hdc, GetStockObject( WHITE_BRUSH ));
    SelectObject( hdc, GetStockObject( SYSTEM_FONT ));
    SelectObject( hdc, GetStockObject( BLACK_PEN ));
    SetVirtualResolution( hdc, 0, 0, 0, 0 );
    GDISelectPalette( hdc, GetStockObject( DEFAULT_PALETTE ), FALSE );
    SetBoundsRect( hdc, NULL, DCB_DISABLE );
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
 *           SaveDC    (GDI32.@)
 */
INT WINAPI SaveDC( HDC hdc )
{
    DC * dc;
    INT ret = 0;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSaveDC );
        ret = physdev->funcs->pSaveDC( physdev );
        release_dc_ptr( dc );
    }
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
        strcpyW(buf, driver);
    }

    if (!(funcs = DRIVER_load_driver( buf )))
    {
        ERR( "no driver found for %s\n", debugstr_w(buf) );
        return 0;
    }
    if (!(dc = alloc_dc_ptr( OBJ_DC ))) return 0;
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

    dc->vis_rect.left   = 0;
    dc->vis_rect.top    = 0;
    dc->vis_rect.right  = GetDeviceCaps( hdc, DESKTOPHORZRES );
    dc->vis_rect.bottom = GetDeviceCaps( hdc, DESKTOPVERTRES );

    DC_InitDC( dc );
    release_dc_ptr( dc );
    return hdc;
}


/***********************************************************************
 *           CreateDCA    (GDI32.@)
 */
HDC WINAPI CreateDCA( LPCSTR driver, LPCSTR device, LPCSTR output,
                      const DEVMODEA *initData )
{
    UNICODE_STRING driverW, deviceW, outputW;
    DEVMODEW *initDataW;
    HDC ret;

    if (driver) RtlCreateUnicodeStringFromAsciiz(&driverW, driver);
    else driverW.Buffer = NULL;

    if (device) RtlCreateUnicodeStringFromAsciiz(&deviceW, device);
    else deviceW.Buffer = NULL;

    if (output) RtlCreateUnicodeStringFromAsciiz(&outputW, output);
    else outputW.Buffer = NULL;

    initDataW = NULL;
    if (initData)
    {
        /* don't convert initData for DISPLAY driver, it's not used */
        if (!driverW.Buffer || strcmpiW( driverW.Buffer, displayW ))
            initDataW = GdiConvertToDevmodeW(initData);
    }

    ret = CreateDCW( driverW.Buffer, deviceW.Buffer, outputW.Buffer, initDataW );

    RtlFreeUnicodeString(&driverW);
    RtlFreeUnicodeString(&deviceW);
    RtlFreeUnicodeString(&outputW);
    HeapFree(GetProcessHeap(), 0, initDataW);
    return ret;
}


/***********************************************************************
 *           CreateICA    (GDI32.@)
 */
HDC WINAPI CreateICA( LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODEA* initData )
{
      /* Nothing special yet for ICs */
    return CreateDCA( driver, device, output, initData );
}


/***********************************************************************
 *           CreateICW    (GDI32.@)
 */
HDC WINAPI CreateICW( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                          const DEVMODEW* initData )
{
      /* Nothing special yet for ICs */
    return CreateDCW( driver, device, output, initData );
}


/***********************************************************************
 *           CreateCompatibleDC   (GDI32.@)
 */
HDC WINAPI CreateCompatibleDC( HDC hdc )
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
    else funcs = DRIVER_load_driver( displayW );

    if (!(dc = alloc_dc_ptr( OBJ_MEMDC ))) return 0;

    TRACE("(%p): returning %p\n", hdc, dc->hSelf );

    dc->hBitmap = GDI_inc_ref_count( GetStockObject( DEFAULT_BITMAP ));
    dc->vis_rect.left   = 0;
    dc->vis_rect.top    = 0;
    dc->vis_rect.right  = 1;
    dc->vis_rect.bottom = 1;
    dc->device_rect = dc->vis_rect;

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
            dc->vis_rect.left   = 0;
            dc->vis_rect.top    = 0;
            dc->vis_rect.right  = GetDeviceCaps( hdc, DESKTOPHORZRES );
            dc->vis_rect.bottom = GetDeviceCaps( hdc, DESKTOPVERTRES );
            if (dc->hVisRgn) DeleteObject( dc->hVisRgn );
            dc->hVisRgn = 0;
            update_dc_clipping( dc );
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           ResetDCA    (GDI32.@)
 */
HDC WINAPI ResetDCA( HDC hdc, const DEVMODEA *devmode )
{
    DEVMODEW *devmodeW;
    HDC ret;

    if (devmode) devmodeW = GdiConvertToDevmodeW(devmode);
    else devmodeW = NULL;

    ret = ResetDCW(hdc, devmodeW);

    HeapFree(GetProcessHeap(), 0, devmodeW);
    return ret;
}


/***********************************************************************
 *           GetDeviceCaps    (GDI32.@)
 */
INT WINAPI GetDeviceCaps( HDC hdc, INT cap )
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
 *		GetBkColor (GDI32.@)
 */
COLORREF WINAPI GetBkColor( HDC hdc )
{
    COLORREF ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->backgroundColor;
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
        ret = dc->backgroundColor;
        dc->backgroundColor = physdev->funcs->pSetBkColor( physdev, color );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetTextColor (GDI32.@)
 */
COLORREF WINAPI GetTextColor( HDC hdc )
{
    COLORREF ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->textColor;
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
        ret = dc->textColor;
        dc->textColor = physdev->funcs->pSetTextColor( physdev, color );
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetTextAlign (GDI32.@)
 */
UINT WINAPI GetTextAlign( HDC hdc )
{
    UINT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->textAlign;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetTextAlign    (GDI32.@)
 */
UINT WINAPI SetTextAlign( HDC hdc, UINT align )
{
    UINT ret = GDI_ERROR;
    DC *dc = get_dc_ptr( hdc );

    TRACE("hdc=%p align=%d\n", hdc, align);

    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetTextAlign );
        align = physdev->funcs->pSetTextAlign( physdev, align );
        if (align != GDI_ERROR)
        {
            ret = dc->textAlign;
            dc->textAlign = align;
        }
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           GetDCOrgEx  (GDI32.@)
 */
BOOL WINAPI GetDCOrgEx( HDC hDC, LPPOINT lpp )
{
    DC * dc;

    if (!lpp) return FALSE;
    if (!(dc = get_dc_ptr( hDC ))) return FALSE;
    lpp->x = dc->vis_rect.left;
    lpp->y = dc->vis_rect.top;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetGraphicsMode (GDI32.@)
 */
INT WINAPI GetGraphicsMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->GraphicsMode;
        release_dc_ptr( dc );
    }
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
        ret = dc->GraphicsMode;
        dc->GraphicsMode = mode;
    }
    /* font metrics depend on the graphics mode */
    if (ret != mode) SelectObject(dc->hSelf, dc->hFont);
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetArcDirection (GDI32.@)
 */
INT WINAPI GetArcDirection( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->ArcDirection;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           SetArcDirection    (GDI32.@)
 */
INT WINAPI SetArcDirection( HDC hdc, INT dir )
{
    DC * dc;
    INT ret = 0;

    if (dir != AD_COUNTERCLOCKWISE && dir != AD_CLOCKWISE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetArcDirection );
        dir = physdev->funcs->pSetArcDirection( physdev, dir );
        if (dir)
        {
            ret = dc->ArcDirection;
            dc->ArcDirection = dir;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           GetWorldTransform    (GDI32.@)
 */
BOOL WINAPI GetWorldTransform( HDC hdc, LPXFORM xform )
{
    DC * dc;
    if (!xform) return FALSE;
    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    *xform = dc->xformWorld2Wnd;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           GetTransform    (GDI32.@)
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
BOOL WINAPI GetTransform( HDC hdc, DWORD which, XFORM *xform )
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
        ret = InterlockedExchange( &dc->disabled, 1 );
    else if (flags & DCHF_ENABLEDC)
        ret = InterlockedExchange( &dc->disabled, 0 );

    GDI_ReleaseObj( hdc );

    if (flags & DCHF_RESETDC) ret = reset_dc_state( hdc );
    return ret;
}

/***********************************************************************
 *           SetICMMode    (GDI32.@)
 */
INT WINAPI SetICMMode(HDC hdc, INT iEnableICM)
{
/*FIXME:  Assume that ICM is always off, and cannot be turned on */
    if (iEnableICM == ICM_OFF) return ICM_OFF;
    if (iEnableICM == ICM_ON) return 0;
    if (iEnableICM == ICM_QUERY) return ICM_OFF;
    return 0;
}

/***********************************************************************
 *           GetDeviceGammaRamp    (GDI32.@)
 */
BOOL WINAPI GetDeviceGammaRamp(HDC hDC, LPVOID ptr)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hDC );

    TRACE("%p, %p\n", hDC, ptr);
    if( dc )
    {
        if (GetObjectType( hDC ) != OBJ_MEMDC)
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
 *           SetDeviceGammaRamp    (GDI32.@)
 */
BOOL WINAPI SetDeviceGammaRamp(HDC hDC, LPVOID ptr)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hDC );

    TRACE("%p, %p\n", hDC, ptr);
    if( dc )
    {
        if (GetObjectType( hDC ) != OBJ_MEMDC)
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
 *           GetColorSpace    (GDI32.@)
 */
HCOLORSPACE WINAPI GetColorSpace(HDC hdc)
{
/*FIXME    Need to do whatever GetColorSpace actually does */
    return 0;
}

/***********************************************************************
 *           CreateColorSpaceA    (GDI32.@)
 */
HCOLORSPACE WINAPI CreateColorSpaceA( LPLOGCOLORSPACEA lpLogColorSpace )
{
  FIXME( "stub\n" );
  return 0;
}

/***********************************************************************
 *           CreateColorSpaceW    (GDI32.@)
 */
HCOLORSPACE WINAPI CreateColorSpaceW( LPLOGCOLORSPACEW lpLogColorSpace )
{
  FIXME( "stub\n" );
  return 0;
}

/***********************************************************************
 *           DeleteColorSpace     (GDI32.@)
 */
BOOL WINAPI DeleteColorSpace( HCOLORSPACE hColorSpace )
{
  FIXME( "stub\n" );

  return TRUE;
}

/***********************************************************************
 *           SetColorSpace     (GDI32.@)
 */
HCOLORSPACE WINAPI SetColorSpace( HDC hDC, HCOLORSPACE hColorSpace )
{
  FIXME( "stub\n" );

  return hColorSpace;
}

/***********************************************************************
 *           GetBoundsRect    (GDI32.@)
 */
UINT WINAPI GetBoundsRect(HDC hdc, LPRECT rect, UINT flags)
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
            rect->right  = min( rect->right, dc->vis_rect.right - dc->vis_rect.left );
            rect->bottom = min( rect->bottom, dc->vis_rect.bottom - dc->vis_rect.top );
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
 *           SetBoundsRect    (GDI32.@)
 */
UINT WINAPI SetBoundsRect(HDC hdc, const RECT* rect, UINT flags)
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
 *		GetRelAbs		(GDI32.@)
 */
INT WINAPI GetRelAbs( HDC hdc, DWORD dwIgnore )
{
    INT ret = 0;
    DC *dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->relAbsMode;
        release_dc_ptr( dc );
    }
    return ret;
}




/***********************************************************************
 *		GetBkMode (GDI32.@)
 */
INT WINAPI GetBkMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->backgroundMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetBkMode (GDI32.@)
 */
INT WINAPI SetBkMode( HDC hdc, INT mode )
{
    INT ret = 0;
    DC *dc;

    if ((mode <= 0) || (mode > BKMODE_LAST))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetBkMode );
        mode = physdev->funcs->pSetBkMode( physdev, mode );
        if (mode)
        {
            ret = dc->backgroundMode;
            dc->backgroundMode = mode;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetROP2 (GDI32.@)
 */
INT WINAPI GetROP2( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->ROPmode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetROP2 (GDI32.@)
 */
INT WINAPI SetROP2( HDC hdc, INT mode )
{
    INT ret = 0;
    DC *dc;

    if ((mode < R2_BLACK) || (mode > R2_WHITE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetROP2 );
        mode = physdev->funcs->pSetROP2( physdev, mode );
        if (mode)
        {
            ret = dc->ROPmode;
            dc->ROPmode = mode;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetRelAbs (GDI32.@)
 */
INT WINAPI SetRelAbs( HDC hdc, INT mode )
{
    INT ret = 0;
    DC *dc;

    if ((mode != ABSOLUTE) && (mode != RELATIVE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetRelAbs );
        mode = physdev->funcs->pSetRelAbs( physdev, mode );
        if (mode)
        {
            ret = dc->relAbsMode;
            dc->relAbsMode = mode;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetPolyFillMode (GDI32.@)
 */
INT WINAPI GetPolyFillMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->polyFillMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetPolyFillMode (GDI32.@)
 */
INT WINAPI SetPolyFillMode( HDC hdc, INT mode )
{
    INT ret = 0;
    DC *dc;

    if ((mode <= 0) || (mode > POLYFILL_LAST))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetPolyFillMode );
        mode = physdev->funcs->pSetPolyFillMode( physdev, mode );
        if (mode)
        {
            ret = dc->polyFillMode;
            dc->polyFillMode = mode;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetStretchBltMode (GDI32.@)
 */
INT WINAPI GetStretchBltMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->stretchBltMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		SetStretchBltMode (GDI32.@)
 */
INT WINAPI SetStretchBltMode( HDC hdc, INT mode )
{
    INT ret = 0;
    DC *dc;

    if ((mode <= 0) || (mode > MAXSTRETCHBLTMODE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetStretchBltMode );
        mode = physdev->funcs->pSetStretchBltMode( physdev, mode );
        if (mode)
        {
            ret = dc->stretchBltMode;
            dc->stretchBltMode = mode;
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetMapMode (GDI32.@)
 */
INT WINAPI GetMapMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        ret = dc->MapMode;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *		GetBrushOrgEx (GDI32.@)
 */
BOOL WINAPI GetBrushOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    *pt = dc->brush_org;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetCurrentPositionEx (GDI32.@)
 */
BOOL WINAPI GetCurrentPositionEx( HDC hdc, LPPOINT pt )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    *pt = dc->cur_pos;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetViewportExtEx (GDI32.@)
 */
BOOL WINAPI GetViewportExtEx( HDC hdc, LPSIZE size )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    *size = dc->vport_ext;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetViewportOrgEx (GDI32.@)
 */
BOOL WINAPI GetViewportOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    *pt = dc->vport_org;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetWindowExtEx (GDI32.@)
 */
BOOL WINAPI GetWindowExtEx( HDC hdc, LPSIZE size )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    *size = dc->wnd_ext;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *		GetWindowOrgEx (GDI32.@)
 */
BOOL WINAPI GetWindowOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    *pt = dc->wnd_org;
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           GetLayout    (GDI32.@)
 *
 * Gets left->right or right->left text layout flags of a dc.
 *
 */
DWORD WINAPI GetLayout(HDC hdc)
{
    DWORD layout = GDI_ERROR;

    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        layout = dc->layout;
        release_dc_ptr( dc );
    }

    TRACE("hdc : %p, layout : %08x\n", hdc, layout);

    return layout;
}

/***********************************************************************
 *           SetLayout    (GDI32.@)
 *
 * Sets left->right or right->left text layout flags of a dc.
 *
 */
DWORD WINAPI SetLayout(HDC hdc, DWORD layout)
{
    DWORD oldlayout = GDI_ERROR;

    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSetLayout );
        layout = physdev->funcs->pSetLayout( physdev, layout );
        if (layout != GDI_ERROR)
        {
            oldlayout = dc->layout;
            dc->layout = layout;
            if (layout != oldlayout)
            {
                if (layout & LAYOUT_RTL) dc->MapMode = MM_ANISOTROPIC;
                DC_UpdateXforms( dc );
            }
        }
        release_dc_ptr( dc );
    }

    TRACE("hdc : %p, old layout : %08x, new layout : %08x\n", hdc, oldlayout, layout);

    return oldlayout;
}

/***********************************************************************
 *           GetDCBrushColor    (GDI32.@)
 */
COLORREF WINAPI GetDCBrushColor(HDC hdc)
{
    DC *dc;
    COLORREF dcBrushColor = CLR_INVALID;

    TRACE("hdc(%p)\n", hdc);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        dcBrushColor = dc->dcBrushColor;
	release_dc_ptr( dc );
    }

    return dcBrushColor;
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
            oldClr = dc->dcBrushColor;
            dc->dcBrushColor = crColor;
        }
        release_dc_ptr( dc );
    }

    return oldClr;
}

/***********************************************************************
 *           GetDCPenColor    (GDI32.@)
 */
COLORREF WINAPI GetDCPenColor(HDC hdc)
{
    DC *dc;
    COLORREF dcPenColor = CLR_INVALID;

    TRACE("hdc(%p)\n", hdc);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        dcPenColor = dc->dcPenColor;
	release_dc_ptr( dc );
    }

    return dcPenColor;
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
            oldClr = dc->dcPenColor;
            dc->dcPenColor = crColor;
        }
        release_dc_ptr( dc );
    }

    return oldClr;
}

/***********************************************************************
 *           CancelDC    (GDI32.@)
 */
BOOL WINAPI CancelDC(HDC hdc)
{
    FIXME("stub\n");
    return TRUE;
}

/*******************************************************************
 *      GetMiterLimit [GDI32.@]
 *
 *
 */
BOOL WINAPI GetMiterLimit(HDC hdc, PFLOAT peLimit)
{
    BOOL bRet = FALSE;
    DC *dc;

    TRACE("(%p,%p)\n", hdc, peLimit);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        if (peLimit)
            *peLimit = dc->miterLimit;

        release_dc_ptr( dc );
        bRet = TRUE;
    }
    return bRet;
}

/*******************************************************************
 *      SetMiterLimit [GDI32.@]
 *
 *
 */
BOOL WINAPI SetMiterLimit(HDC hdc, FLOAT eNewLimit, PFLOAT peOldLimit)
{
    BOOL bRet = FALSE;
    DC *dc;

    TRACE("(%p,%f,%p)\n", hdc, eNewLimit, peOldLimit);

    dc = get_dc_ptr( hdc );
    if (dc)
    {
        if (peOldLimit)
            *peOldLimit = dc->miterLimit;
        dc->miterLimit = eNewLimit;
        release_dc_ptr( dc );
        bRet = TRUE;
    }
    return bRet;
}

/*******************************************************************
 *      GdiIsMetaPrintDC [GDI32.@]
 */
BOOL WINAPI GdiIsMetaPrintDC(HDC hdc)
{
    FIXME("%p\n", hdc);
    return FALSE;
}

/*******************************************************************
 *      GdiIsMetaFileDC [GDI32.@]
 */
BOOL WINAPI GdiIsMetaFileDC(HDC hdc)
{
    TRACE("%p\n", hdc);

    switch( GetObjectType( hdc ) )
    {
    case OBJ_METADC:
    case OBJ_ENHMETADC:
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************
 *      GdiIsPlayMetafileDC [GDI32.@]
 */
BOOL WINAPI GdiIsPlayMetafileDC(HDC hdc)
{
    FIXME("%p\n", hdc);
    return FALSE;
}
