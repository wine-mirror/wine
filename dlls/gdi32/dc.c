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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "wownt32.h"
#include "wine/winuser16.h"
#include "gdi_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);

static const WCHAR displayW[] = { 'd','i','s','p','l','a','y',0 };

static BOOL DC_DeleteObject( HGDIOBJ handle, void *obj );

static const struct gdi_obj_funcs dc_funcs =
{
    NULL,             /* pSelectObject */
    NULL,             /* pGetObject16 */
    NULL,             /* pGetObjectA */
    NULL,             /* pGetObjectW */
    NULL,             /* pUnrealizeObject */
    DC_DeleteObject   /* pDeleteObject */
};

/***********************************************************************
 *           DC_AllocDC
 */
DC *DC_AllocDC( const DC_FUNCTIONS *funcs, WORD magic )
{
    HDC hdc;
    DC *dc;

    if (!(dc = GDI_AllocObject( sizeof(*dc), magic, (HGDIOBJ*)&hdc, &dc_funcs ))) return NULL;

    dc->hSelf               = hdc;
    dc->funcs               = funcs;
    dc->physDev             = NULL;
    dc->saveLevel           = 0;
    dc->saved_dc            = 0;
    dc->dwHookData          = 0;
    dc->hookProc            = NULL;
    dc->hookThunk           = NULL;
    dc->wndOrgX             = 0;
    dc->wndOrgY             = 0;
    dc->wndExtX             = 1;
    dc->wndExtY             = 1;
    dc->vportOrgX           = 0;
    dc->vportOrgY           = 0;
    dc->vportExtX           = 1;
    dc->vportExtY           = 1;
    dc->miterLimit          = 10.0f; /* 10.0 is the default, from MSDN */
    dc->flags               = 0;
    dc->layout              = 0;
    dc->hClipRgn            = 0;
    dc->hMetaRgn            = 0;
    dc->hMetaClipRgn        = 0;
    dc->hVisRgn             = 0;
    dc->hPen                = GetStockObject( BLACK_PEN );
    dc->hBrush              = GetStockObject( WHITE_BRUSH );
    dc->hFont               = GetStockObject( SYSTEM_FONT );
    dc->hBitmap             = 0;
    dc->hDevice             = 0;
    dc->hPalette            = GetStockObject( DEFAULT_PALETTE );
    dc->gdiFont             = 0;
    dc->ROPmode             = R2_COPYPEN;
    dc->polyFillMode        = ALTERNATE;
    dc->stretchBltMode      = BLACKONWHITE;
    dc->relAbsMode          = ABSOLUTE;
    dc->backgroundMode      = OPAQUE;
    dc->backgroundColor     = RGB( 255, 255, 255 );
    dc->dcBrushColor        = RGB( 255, 255, 255 );
    dc->dcPenColor          = RGB( 0, 0, 0 );
    dc->textColor           = RGB( 0, 0, 0 );
    dc->brushOrgX           = 0;
    dc->brushOrgY           = 0;
    dc->textAlign           = TA_LEFT | TA_TOP | TA_NOUPDATECP;
    dc->charExtra           = 0;
    dc->breakExtra          = 0;
    dc->breakRem            = 0;
    dc->MapMode             = MM_TEXT;
    dc->GraphicsMode        = GM_COMPATIBLE;
    dc->pAbortProc          = NULL;
    dc->CursPosX            = 0;
    dc->CursPosY            = 0;
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
    dc->BoundsRect.left     = 0;
    dc->BoundsRect.top      = 0;
    dc->BoundsRect.right    = 0;
    dc->BoundsRect.bottom   = 0;
    dc->saved_visrgn        = NULL;
    PATH_InitGdiPath(&dc->path);
    return dc;
}



/***********************************************************************
 *           DC_GetDCPtr
 */
DC *DC_GetDCPtr( HDC hdc )
{
    GDIOBJHDR *ptr = GDI_GetObjPtr( hdc, MAGIC_DONTCARE );
    if (!ptr) return NULL;
    if ((GDIMAGIC(ptr->wMagic) == DC_MAGIC) ||
        (GDIMAGIC(ptr->wMagic) == MEMORY_DC_MAGIC) ||
        (GDIMAGIC(ptr->wMagic) == METAFILE_DC_MAGIC) ||
        (GDIMAGIC(ptr->wMagic) == ENHMETAFILE_DC_MAGIC))
        return (DC *)ptr;
    GDI_ReleaseObj( hdc );
    SetLastError( ERROR_INVALID_HANDLE );
    return NULL;
}

/***********************************************************************
 *           DC_GetDCUpdate
 *
 * Retrieve a DC ptr while making sure the visRgn is updated.
 * This function may call up to USER so the GDI lock should _not_
 * be held when calling it.
 */
DC *DC_GetDCUpdate( HDC hdc )
{
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return NULL;
    while (dc->flags & DC_DIRTY)
    {
        DCHOOKPROC proc = dc->hookThunk;
        dc->flags &= ~DC_DIRTY;
        if (proc)
        {
            DWORD data = dc->dwHookData;
            GDI_ReleaseObj( hdc );
            proc( HDC_16(hdc), DCHC_INVALIDVISRGN, data, 0 );
            if (!(dc = DC_GetDCPtr( hdc ))) break;
            /* otherwise restart the loop in case it became dirty again in the meantime */
        }
    }
    return dc;
}


/***********************************************************************
 *           DC_DeleteObject
 */
static BOOL DC_DeleteObject( HGDIOBJ handle, void *obj )
{
    GDI_ReleaseObj( handle );
    return DeleteDC( handle );
}


/***********************************************************************
 *           DC_InitDC
 *
 * Setup device-specific DC values for a newly created DC.
 */
void DC_InitDC( DC* dc )
{
    if (dc->funcs->pRealizeDefaultPalette) dc->funcs->pRealizeDefaultPalette( dc->physDev );
    SetTextColor( dc->hSelf, dc->textColor );
    SetBkColor( dc->hSelf, dc->backgroundColor );
    SelectObject( dc->hSelf, dc->hPen );
    SelectObject( dc->hSelf, dc->hBrush );
    SelectObject( dc->hSelf, dc->hFont );
    CLIPPING_UpdateGCRegion( dc );
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
    FLOAT determinant;

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
    FLOAT scaleX, scaleY;

    /* Construct a transformation to do the window-to-viewport conversion */
    scaleX = (FLOAT)dc->vportExtX / (FLOAT)dc->wndExtX;
    scaleY = (FLOAT)dc->vportExtY / (FLOAT)dc->wndExtY;
    xformWnd2Vport.eM11 = scaleX;
    xformWnd2Vport.eM12 = 0.0;
    xformWnd2Vport.eM21 = 0.0;
    xformWnd2Vport.eM22 = scaleY;
    xformWnd2Vport.eDx  = (FLOAT)dc->vportOrgX -
        scaleX * (FLOAT)dc->wndOrgX;
    xformWnd2Vport.eDy  = (FLOAT)dc->vportOrgY -
        scaleY * (FLOAT)dc->wndOrgY;

    oldworld2vport = dc->xformWorld2Vport;
    /* Combine with the world transformation */
    CombineTransform( &dc->xformWorld2Vport, &dc->xformWorld2Wnd,
        &xformWnd2Vport );

    /* Create inverse of world-to-viewport transformation */
    dc->vport2WorldValid = DC_InvertXform( &dc->xformWorld2Vport,
        &dc->xformVport2World );

    /* Reselect the font and pen back into the dc so that the size
       gets updated. */
    if(memcmp(&oldworld2vport, &dc->xformWorld2Vport, sizeof(oldworld2vport)))
    {
        SelectObject(dc->hSelf, GetCurrentObject(dc->hSelf, OBJ_FONT));
        SelectObject(dc->hSelf, GetCurrentObject(dc->hSelf, OBJ_PEN));
    }
}


/***********************************************************************
 *           GetDCState   (Not a Windows API)
 */
HDC WINAPI GetDCState( HDC hdc )
{
    DC * newdc, * dc;
    HGDIOBJ handle;

    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    if (!(newdc = GDI_AllocObject( sizeof(DC), GDIMAGIC(dc->header.wMagic), &handle, &dc_funcs )))
    {
      GDI_ReleaseObj( hdc );
      return 0;
    }
    TRACE("(%p): returning %p\n", hdc, handle );

    newdc->flags            = dc->flags | DC_SAVED;
    newdc->layout           = dc->layout;
    newdc->hPen             = dc->hPen;
    newdc->hBrush           = dc->hBrush;
    newdc->hFont            = dc->hFont;
    newdc->hBitmap          = dc->hBitmap;
    newdc->hDevice          = dc->hDevice;
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
    newdc->brushOrgX        = dc->brushOrgX;
    newdc->brushOrgY        = dc->brushOrgY;
    newdc->textAlign        = dc->textAlign;
    newdc->charExtra        = dc->charExtra;
    newdc->breakExtra       = dc->breakExtra;
    newdc->breakRem         = dc->breakRem;
    newdc->MapMode          = dc->MapMode;
    newdc->GraphicsMode     = dc->GraphicsMode;
    newdc->CursPosX         = dc->CursPosX;
    newdc->CursPosY         = dc->CursPosY;
    newdc->ArcDirection     = dc->ArcDirection;
    newdc->xformWorld2Wnd   = dc->xformWorld2Wnd;
    newdc->xformWorld2Vport = dc->xformWorld2Vport;
    newdc->xformVport2World = dc->xformVport2World;
    newdc->vport2WorldValid = dc->vport2WorldValid;
    newdc->wndOrgX          = dc->wndOrgX;
    newdc->wndOrgY          = dc->wndOrgY;
    newdc->wndExtX          = dc->wndExtX;
    newdc->wndExtY          = dc->wndExtY;
    newdc->vportOrgX        = dc->vportOrgX;
    newdc->vportOrgY        = dc->vportOrgY;
    newdc->vportExtX        = dc->vportExtX;
    newdc->vportExtY        = dc->vportExtY;
    newdc->BoundsRect       = dc->BoundsRect;

    newdc->hSelf = (HDC)handle;
    newdc->saveLevel = 0;
    newdc->saved_dc  = 0;

    PATH_InitGdiPath( &newdc->path );

    newdc->pAbortProc = NULL;
    newdc->hookThunk  = NULL;
    newdc->hookProc   = 0;
    newdc->saved_visrgn = NULL;

    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

    newdc->hVisRgn      = 0;
    newdc->hClipRgn     = 0;
    newdc->hMetaRgn     = 0;
    newdc->hMetaClipRgn = 0;
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
    /* don't bother recomputing hMetaClipRgn, we'll do that in SetDCState */

    if(dc->gdiFont) {
	newdc->gdiFont = dc->gdiFont;
    } else
        newdc->gdiFont = 0;

    GDI_ReleaseObj( handle );
    GDI_ReleaseObj( hdc );
    return handle;
}


/***********************************************************************
 *           SetDCState   (Not a Windows API)
 */
void WINAPI SetDCState( HDC hdc, HDC hdcs )
{
    DC *dc, *dcs;

    if (!(dc = DC_GetDCUpdate( hdc ))) return;
    if (!(dcs = DC_GetDCPtr( hdcs )))
    {
      GDI_ReleaseObj( hdc );
      return;
    }
    if (!dcs->flags & DC_SAVED)
    {
      GDI_ReleaseObj( hdc );
      GDI_ReleaseObj( hdcs );
      return;
    }
    TRACE("%p %p\n", hdc, hdcs );

    dc->flags            = dcs->flags & ~(DC_SAVED | DC_DIRTY);
    dc->layout           = dcs->layout;
    dc->hDevice          = dcs->hDevice;
    dc->ROPmode          = dcs->ROPmode;
    dc->polyFillMode     = dcs->polyFillMode;
    dc->stretchBltMode   = dcs->stretchBltMode;
    dc->relAbsMode       = dcs->relAbsMode;
    dc->backgroundMode   = dcs->backgroundMode;
    dc->backgroundColor  = dcs->backgroundColor;
    dc->textColor        = dcs->textColor;
    dc->dcBrushColor     = dcs->dcBrushColor;
    dc->dcPenColor       = dcs->dcPenColor;
    dc->brushOrgX        = dcs->brushOrgX;
    dc->brushOrgY        = dcs->brushOrgY;
    dc->textAlign        = dcs->textAlign;
    dc->charExtra        = dcs->charExtra;
    dc->breakExtra       = dcs->breakExtra;
    dc->breakRem         = dcs->breakRem;
    dc->MapMode          = dcs->MapMode;
    dc->GraphicsMode     = dcs->GraphicsMode;
    dc->CursPosX         = dcs->CursPosX;
    dc->CursPosY         = dcs->CursPosY;
    dc->ArcDirection     = dcs->ArcDirection;
    dc->xformWorld2Wnd   = dcs->xformWorld2Wnd;
    dc->xformWorld2Vport = dcs->xformWorld2Vport;
    dc->xformVport2World = dcs->xformVport2World;
    dc->vport2WorldValid = dcs->vport2WorldValid;
    dc->BoundsRect       = dcs->BoundsRect;

    dc->wndOrgX          = dcs->wndOrgX;
    dc->wndOrgY          = dcs->wndOrgY;
    dc->wndExtX          = dcs->wndExtX;
    dc->wndExtY          = dcs->wndExtY;
    dc->vportOrgX        = dcs->vportOrgX;
    dc->vportOrgY        = dcs->vportOrgY;
    dc->vportExtX        = dcs->vportExtX;
    dc->vportExtY        = dcs->vportExtY;

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
    CLIPPING_UpdateGCRegion( dc );

    SelectObject( hdc, dcs->hBitmap );
    SelectObject( hdc, dcs->hBrush );
    SelectObject( hdc, dcs->hFont );
    SelectObject( hdc, dcs->hPen );
    SetBkColor( hdc, dcs->backgroundColor);
    SetTextColor( hdc, dcs->textColor);
    GDISelectPalette( hdc, dcs->hPalette, FALSE );
    GDI_ReleaseObj( hdcs );
    GDI_ReleaseObj( hdc );
}


/***********************************************************************
 *           GetDCState   (GDI.179)
 */
HDC16 WINAPI GetDCState16( HDC16 hdc )
{
    return HDC_16( GetDCState( HDC_32(hdc) ));
}


/***********************************************************************
 *           SetDCState   (GDI.180)
 */
void WINAPI SetDCState16( HDC16 hdc, HDC16 hdcs )
{
    SetDCState( HDC_32(hdc), HDC_32(hdcs) );
}


/***********************************************************************
 *           SaveDC    (GDI32.@)
 */
INT WINAPI SaveDC( HDC hdc )
{
    HDC hdcs;
    DC * dc, * dcs;
    INT ret;

    dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;

    if(dc->funcs->pSaveDC)
    {
        ret = dc->funcs->pSaveDC( dc->physDev );
        if(ret)
            ret = ++dc->saveLevel;
        GDI_ReleaseObj( hdc );
        return ret;
    }

    if (!(hdcs = GetDCState( hdc )))
    {
      GDI_ReleaseObj( hdc );
      return 0;
    }
    dcs = DC_GetDCPtr( hdcs );

    /* Copy path. The reason why path saving / restoring is in SaveDC/
     * RestoreDC and not in GetDCState/SetDCState is that the ...DCState
     * functions are only in Win16 (which doesn't have paths) and that
     * SetDCState doesn't allow us to signal an error (which can happen
     * when copying paths).
     */
    if (!PATH_AssignGdiPath( &dcs->path, &dc->path ))
    {
        GDI_ReleaseObj( hdc );
	GDI_ReleaseObj( hdcs );
	DeleteDC( hdcs );
	return 0;
    }

    dcs->saved_dc = dc->saved_dc;
    dc->saved_dc = hdcs;
    TRACE("(%p): returning %d\n", hdc, dc->saveLevel+1 );
    ret = ++dc->saveLevel;
    GDI_ReleaseObj( hdcs );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           RestoreDC    (GDI32.@)
 */
BOOL WINAPI RestoreDC( HDC hdc, INT level )
{
    DC * dc, * dcs;
    BOOL success;

    TRACE("%p %d\n", hdc, level );
    dc = DC_GetDCUpdate( hdc );
    if(!dc) return FALSE;

    if(abs(level) > dc->saveLevel || level == 0)
    {
        GDI_ReleaseObj( hdc );
        return FALSE;
    }
        
    if(dc->funcs->pRestoreDC)
    {
        success = dc->funcs->pRestoreDC( dc->physDev, level );
        if(level < 0) level = dc->saveLevel + level + 1;
        if(success)
            dc->saveLevel = level - 1;
        GDI_ReleaseObj( hdc );
        return success;
    }

    if (level < 0) level = dc->saveLevel + level + 1;
    success=TRUE;
    while (dc->saveLevel >= level)
    {
        HDC hdcs = dc->saved_dc;
	if (!(dcs = DC_GetDCPtr( hdcs )))
	{
	  GDI_ReleaseObj( hdc );
	  return FALSE;
	}
        dc->saved_dc = dcs->saved_dc;
        dcs->saved_dc = 0;
	if (--dc->saveLevel < level)
	{
	    SetDCState( hdc, hdcs );
            if (!PATH_AssignGdiPath( &dc->path, &dcs->path ))
		/* FIXME: This might not be quite right, since we're
		 * returning FALSE but still destroying the saved DC state */
	        success=FALSE;
	}
        GDI_ReleaseObj( hdcs );
        GDI_ReleaseObj( hdc );
	DeleteDC( hdcs );
        if (!(dc = DC_GetDCPtr( hdc ))) return FALSE;
    }
    GDI_ReleaseObj( hdc );
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
    const DC_FUNCTIONS *funcs;
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
    if (!(dc = DC_AllocDC( funcs, DC_MAGIC ))) goto error;
    hdc = dc->hSelf;

    dc->hBitmap = GetStockObject( DEFAULT_BITMAP );
    if (!(dc->hVisRgn = CreateRectRgn( 0, 0, 1, 1 ))) goto error;

    TRACE("(driver=%s, device=%s, output=%s): returning %p\n",
          debugstr_w(driver), debugstr_w(device), debugstr_w(output), dc->hSelf );

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( hdc, &dc->physDev, buf, device, output, initData ))
    {
        WARN("creation aborted by device\n" );
        goto error;
    }

    SetRectRgn( dc->hVisRgn, 0, 0,
                GetDeviceCaps( hdc, DESKTOPHORZRES ), GetDeviceCaps( hdc, DESKTOPVERTRES ) );

    DC_InitDC( dc );
    GDI_ReleaseObj( hdc );
    return hdc;

error:
    if (dc && dc->hVisRgn) DeleteObject( dc->hVisRgn );
    if (dc) GDI_FreeObject( dc->hSelf, dc );
    DRIVER_release_driver( funcs );
    return 0;
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
    const DC_FUNCTIONS *funcs;
    PHYSDEV physDev;

    GDI_CheckNotLock();

    if ((origDC = GDI_GetObjPtr( hdc, DC_MAGIC )))
    {
        funcs = origDC->funcs;
        physDev = origDC->physDev;
        GDI_ReleaseObj( hdc ); /* can't hold the lock while loading the driver */
        funcs = DRIVER_get_driver( funcs );
    }
    else
    {
        funcs = DRIVER_load_driver( displayW );
        physDev = NULL;
    }

    if (!funcs) return 0;

    if (!(dc = DC_AllocDC( funcs, MEMORY_DC_MAGIC ))) goto error;

    TRACE("(%p): returning %p\n", hdc, dc->hSelf );

    dc->hBitmap = GetStockObject( DEFAULT_BITMAP );
    if (!(dc->hVisRgn = CreateRectRgn( 0, 0, 1, 1 ))) goto error;   /* default bitmap is 1x1 */

    /* Copy the driver-specific physical device info into
     * the new DC. The driver may use this read-only info
     * while creating the compatible DC below. */
    dc->physDev = physDev;

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc->hSelf, &dc->physDev, NULL, NULL, NULL, NULL ))
    {
        WARN("creation aborted by device\n");
        goto error;
    }

    DC_InitDC( dc );
    GDI_ReleaseObj( dc->hSelf );
    return dc->hSelf;

error:
    if (dc && dc->hVisRgn) DeleteObject( dc->hVisRgn );
    if (dc) GDI_FreeObject( dc->hSelf, dc );
    DRIVER_release_driver( funcs );
    return 0;
}


/***********************************************************************
 *           DeleteDC    (GDI32.@)
 */
BOOL WINAPI DeleteDC( HDC hdc )
{
    const DC_FUNCTIONS *funcs = NULL;
    DC * dc;

    TRACE("%p\n", hdc );

    GDI_CheckNotLock();

    if (!(dc = DC_GetDCPtr( hdc ))) return FALSE;

    /* Call hook procedure to check whether is it OK to delete this DC */
    if (dc->hookThunk)
    {
        DCHOOKPROC proc = dc->hookThunk;
        DWORD data = dc->dwHookData;
        GDI_ReleaseObj( hdc );
        if (!proc( HDC_16(hdc), DCHC_DELETEDC, data, 0 )) return FALSE;
        if (!(dc = DC_GetDCPtr( hdc ))) return TRUE;  /* deleted by the hook */
    }

    while (dc->saveLevel)
    {
        DC * dcs;
        HDC hdcs = dc->saved_dc;
        if (!(dcs = DC_GetDCPtr( hdcs ))) break;
        dc->saved_dc = dcs->saved_dc;
        dc->saveLevel--;
        if (dcs->hClipRgn) DeleteObject( dcs->hClipRgn );
        if (dcs->hMetaRgn) DeleteObject( dcs->hMetaRgn );
        if (dcs->hMetaClipRgn) DeleteObject( dcs->hMetaClipRgn );
        if (dcs->hVisRgn) DeleteObject( dcs->hVisRgn );
        PATH_DestroyGdiPath(&dcs->path);
        GDI_FreeObject( hdcs, dcs );
    }

    if (!(dc->flags & DC_SAVED))
    {
	SelectObject( hdc, GetStockObject(BLACK_PEN) );
	SelectObject( hdc, GetStockObject(WHITE_BRUSH) );
	SelectObject( hdc, GetStockObject(SYSTEM_FONT) );
        SelectObject( hdc, GetStockObject(DEFAULT_BITMAP) );
        funcs = dc->funcs;
        if (dc->funcs->pDeleteDC) dc->funcs->pDeleteDC(dc->physDev);
        dc->physDev = NULL;
    }

    while (dc->saved_visrgn)
    {
        struct saved_visrgn *next = dc->saved_visrgn->next;
        DeleteObject( dc->saved_visrgn->hrgn );
        HeapFree( GetProcessHeap(), 0, dc->saved_visrgn );
        dc->saved_visrgn = next;
    }
    if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
    if (dc->hMetaRgn) DeleteObject( dc->hMetaRgn );
    if (dc->hMetaClipRgn) DeleteObject( dc->hMetaClipRgn );
    if (dc->hVisRgn) DeleteObject( dc->hVisRgn );
    PATH_DestroyGdiPath(&dc->path);

    GDI_FreeObject( hdc, dc );
    if (funcs) DRIVER_release_driver( funcs );  /* do that after releasing the GDI lock */
    return TRUE;
}


/***********************************************************************
 *           ResetDCW    (GDI32.@)
 */
HDC WINAPI ResetDCW( HDC hdc, const DEVMODEW *devmode )
{
    DC *dc;
    HDC ret = hdc;

    if ((dc = DC_GetDCPtr( hdc )))
    {
        if (dc->funcs->pResetDC) ret = dc->funcs->pResetDC( dc->physDev, devmode );
        GDI_ReleaseObj( hdc );
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

    if ((dc = DC_GetDCPtr( hdc )))
    {
        if (dc->funcs->pGetDeviceCaps) ret = dc->funcs->pGetDeviceCaps( dc->physDev, cap );
        else switch(cap)  /* return meaningful values for some entries */
        {
        case HORZRES:     ret = 640; break;
        case VERTRES:     ret = 480; break;
        case BITSPIXEL:   ret = 1; break;
        case PLANES:      ret = 1; break;
        case NUMCOLORS:   ret = 2; break;
        case ASPECTX:     ret = 36; break;
        case ASPECTY:     ret = 36; break;
        case ASPECTXY:    ret = 51; break;
        case LOGPIXELSX:  ret = 72; break;
        case LOGPIXELSY:  ret = 72; break;
        case SIZEPALETTE: ret = 2; break;
        }
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *		GetBkColor (GDI32.@)
 */
COLORREF WINAPI GetBkColor( HDC hdc )
{
    COLORREF ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->backgroundColor;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           SetBkColor    (GDI32.@)
 */
COLORREF WINAPI SetBkColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE("hdc=%p color=0x%08x\n", hdc, color);

    if (!dc) return CLR_INVALID;
    oldColor = dc->backgroundColor;
    if (dc->funcs->pSetBkColor)
    {
        color = dc->funcs->pSetBkColor(dc->physDev, color);
        if (color == CLR_INVALID)  /* don't change it */
        {
            color = oldColor;
            oldColor = CLR_INVALID;
        }
    }
    dc->backgroundColor = color;
    GDI_ReleaseObj( hdc );
    return oldColor;
}


/***********************************************************************
 *		GetTextColor (GDI32.@)
 */
COLORREF WINAPI GetTextColor( HDC hdc )
{
    COLORREF ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->textColor;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           SetTextColor    (GDI32.@)
 */
COLORREF WINAPI SetTextColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = DC_GetDCPtr( hdc );

    TRACE(" hdc=%p color=0x%08x\n", hdc, color);

    if (!dc) return CLR_INVALID;
    oldColor = dc->textColor;
    if (dc->funcs->pSetTextColor)
    {
        color = dc->funcs->pSetTextColor(dc->physDev, color);
        if (color == CLR_INVALID)  /* don't change it */
        {
            color = oldColor;
            oldColor = CLR_INVALID;
        }
    }
    dc->textColor = color;
    GDI_ReleaseObj( hdc );
    return oldColor;
}


/***********************************************************************
 *		GetTextAlign (GDI32.@)
 */
UINT WINAPI GetTextAlign( HDC hdc )
{
    UINT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->textAlign;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           SetTextAlign    (GDI32.@)
 */
UINT WINAPI SetTextAlign( HDC hdc, UINT align )
{
    UINT ret;
    DC *dc = DC_GetDCPtr( hdc );

    TRACE("hdc=%p align=%d\n", hdc, align);

    if (!dc) return 0x0;
    ret = dc->textAlign;
    if (dc->funcs->pSetTextAlign)
        if (!dc->funcs->pSetTextAlign(dc->physDev, align))
            ret = GDI_ERROR;
    if (ret != GDI_ERROR)
	dc->textAlign = align;
    GDI_ReleaseObj( hdc );
    return ret;
}

/***********************************************************************
 *           GetDCOrgEx  (GDI32.@)
 */
BOOL WINAPI GetDCOrgEx( HDC hDC, LPPOINT lpp )
{
    DC * dc;

    if (!lpp) return FALSE;
    if (!(dc = DC_GetDCPtr( hDC ))) return FALSE;

    lpp->x = lpp->y = 0;
    if (dc->funcs->pGetDCOrgEx) dc->funcs->pGetDCOrgEx( dc->physDev, lpp );
    GDI_ReleaseObj( hDC );
    return TRUE;
}


/***********************************************************************
 *           SetDCOrg   (GDI.117)
 */
DWORD WINAPI SetDCOrg16( HDC16 hdc16, INT16 x, INT16 y )
{
    DWORD prevOrg = 0;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetDCOrg) prevOrg = dc->funcs->pSetDCOrg( dc->physDev, x, y );
    GDI_ReleaseObj( hdc );
    return prevOrg;
}


/***********************************************************************
 *		GetGraphicsMode (GDI32.@)
 */
INT WINAPI GetGraphicsMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->GraphicsMode;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           SetGraphicsMode    (GDI32.@)
 */
INT WINAPI SetGraphicsMode( HDC hdc, INT mode )
{
    INT ret = 0;
    DC *dc = DC_GetDCPtr( hdc );

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
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *		GetArcDirection (GDI32.@)
 */
INT WINAPI GetArcDirection( HDC hdc )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->ArcDirection;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *           SetArcDirection    (GDI32.@)
 */
INT WINAPI SetArcDirection( HDC hdc, INT nDirection )
{
    DC * dc;
    INT nOldDirection = 0;

    if (nDirection!=AD_COUNTERCLOCKWISE && nDirection!=AD_CLOCKWISE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }

    if ((dc = DC_GetDCPtr( hdc )))
    {
        if (dc->funcs->pSetArcDirection)
        {
            dc->funcs->pSetArcDirection(dc->physDev, nDirection);
        }
        nOldDirection = dc->ArcDirection;
        dc->ArcDirection = nDirection;
        GDI_ReleaseObj( hdc );
    }
    return nOldDirection;
}


/***********************************************************************
 *           GetWorldTransform    (GDI32.@)
 */
BOOL WINAPI GetWorldTransform( HDC hdc, LPXFORM xform )
{
    DC * dc;
    if (!xform) return FALSE;
    if (!(dc = DC_GetDCPtr( hdc ))) return FALSE;
    *xform = dc->xformWorld2Wnd;
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *           GetTransform    (GDI32.@)
 */
BOOL WINAPI GetTransform( HDC hdc, DWORD unknown, LPXFORM xform )
{
    if (unknown == 0x0203) return GetWorldTransform( hdc, xform );
    FIXME("stub: don't know what to do for code %x\n", unknown );
    return FALSE;
}


/***********************************************************************
 *           SetWorldTransform    (GDI32.@)
 */
BOOL WINAPI SetWorldTransform( HDC hdc, const XFORM *xform )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return FALSE;
    if (!xform) goto done;

    /* Check that graphics mode is GM_ADVANCED */
    if (dc->GraphicsMode!=GM_ADVANCED) goto done;

    if (dc->funcs->pSetWorldTransform)
    {
        ret = dc->funcs->pSetWorldTransform(dc->physDev, xform);
        if (!ret) goto done;
    }

    dc->xformWorld2Wnd = *xform;
    DC_UpdateXforms( dc );
    ret = TRUE;
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/****************************************************************************
 * ModifyWorldTransform [GDI32.@]
 * Modifies the world transformation for a device context.
 *
 * PARAMS
 *    hdc   [I] Handle to device context
 *    xform [I] XFORM structure that will be used to modify the world
 *              transformation
 *    iMode [I] Specifies in what way to modify the world transformation
 *              Possible values:
 *              MWT_IDENTITY
 *                 Resets the world transformation to the identity matrix.
 *                 The parameter xform is ignored.
 *              MWT_LEFTMULTIPLY
 *                 Multiplies xform into the world transformation matrix from
 *                 the left.
 *              MWT_RIGHTMULTIPLY
 *                 Multiplies xform into the world transformation matrix from
 *                 the right.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI ModifyWorldTransform( HDC hdc, const XFORM *xform,
    DWORD iMode )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCPtr( hdc );

    /* Check for illegal parameters */
    if (!dc) return FALSE;
    if (!xform && iMode != MWT_IDENTITY) goto done;

    /* Check that graphics mode is GM_ADVANCED */
    if (dc->GraphicsMode!=GM_ADVANCED) goto done;

    if (dc->funcs->pModifyWorldTransform)
    {
        ret = dc->funcs->pModifyWorldTransform(dc->physDev, xform, iMode);
        if (!ret) goto done;
    }

    switch (iMode)
    {
        case MWT_IDENTITY:
	    dc->xformWorld2Wnd.eM11 = 1.0f;
	    dc->xformWorld2Wnd.eM12 = 0.0f;
	    dc->xformWorld2Wnd.eM21 = 0.0f;
	    dc->xformWorld2Wnd.eM22 = 1.0f;
	    dc->xformWorld2Wnd.eDx  = 0.0f;
	    dc->xformWorld2Wnd.eDy  = 0.0f;
	    break;
        case MWT_LEFTMULTIPLY:
	    CombineTransform( &dc->xformWorld2Wnd, xform,
	        &dc->xformWorld2Wnd );
	    break;
	case MWT_RIGHTMULTIPLY:
	    CombineTransform( &dc->xformWorld2Wnd, &dc->xformWorld2Wnd,
	        xform );
	    break;
        default:
            goto done;
    }

    DC_UpdateXforms( dc );
    ret = TRUE;
 done:
    GDI_ReleaseObj( hdc );
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
BOOL WINAPI SetDCHook( HDC hdc, DCHOOKPROC hookProc, DWORD dwHookData )
{
    DC *dc = GDI_GetObjPtr( hdc, DC_MAGIC );

    if (!dc) return FALSE;

    if (!(dc->flags & DC_SAVED))
    {
        dc->dwHookData = dwHookData;
        dc->hookThunk = hookProc;
    }
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/* relay function to call the 16-bit DC hook proc */
static BOOL16 WINAPI call_dc_hook16( HDC16 hdc16, WORD code, DWORD data, LPARAM lParam )
{
    WORD args[6];
    DWORD ret;
    FARPROC16 proc = NULL;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return FALSE;
    proc = dc->hookProc;
    GDI_ReleaseObj( hdc );
    if (!proc) return FALSE;
    args[5] = hdc16;
    args[4] = code;
    args[3] = HIWORD(data);
    args[2] = LOWORD(data);
    args[1] = HIWORD(lParam);
    args[0] = LOWORD(lParam);
    WOWCallback16Ex( (DWORD)proc, WCB16_PASCAL, sizeof(args), args, &ret );
    return LOWORD(ret);
}

/***********************************************************************
 *           SetDCHook   (GDI.190)
 */
BOOL16 WINAPI SetDCHook16( HDC16 hdc16, FARPROC16 hookProc, DWORD dwHookData )
{
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;

    dc->hookProc = hookProc;
    GDI_ReleaseObj( hdc );
    return SetDCHook( hdc, call_dc_hook16, dwHookData );
}


/***********************************************************************
 *           GetDCHook   (GDI.191)
 */
DWORD WINAPI GetDCHook16( HDC16 hdc16, FARPROC16 *phookProc )
{
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCPtr( hdc );
    DWORD ret;

    if (!dc) return 0;
    *phookProc = dc->hookProc;
    ret = dc->dwHookData;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetHookFlags   (GDI.192)
 */
WORD WINAPI SetHookFlags16(HDC16 hdc16, WORD flags)
{
    HDC hdc = HDC_32( hdc16 );
    DC *dc = DC_GetDCPtr( hdc );

    if( dc )
    {
        WORD wRet = dc->flags & DC_DIRTY;

        /* "Undocumented Windows" info is slightly confusing.
         */

        TRACE("hDC %p, flags %04x\n",hdc,flags);

        if( flags & DCHF_INVALIDATEVISRGN )
            dc->flags |= DC_DIRTY;
        else if( flags & DCHF_VALIDATEVISRGN || !flags )
            dc->flags &= ~DC_DIRTY;
        GDI_ReleaseObj( hdc );
        return wRet;
    }
    return 0;
}

/***********************************************************************
 *           SetICMMode    (GDI32.@)
 */
INT WINAPI SetICMMode(HDC hdc, INT iEnableICM)
{
/*FIXME  Asuming that ICM is always off, and cannot be turned on */
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
    DC *dc = DC_GetDCPtr( hDC );

    if( dc )
    {
	if (dc->funcs->pGetDeviceGammaRamp)
	    ret = dc->funcs->pGetDeviceGammaRamp(dc->physDev, ptr);
	GDI_ReleaseObj( hDC );
    }
    return ret;
}

/***********************************************************************
 *           SetDeviceGammaRamp    (GDI32.@)
 */
BOOL WINAPI SetDeviceGammaRamp(HDC hDC, LPVOID ptr)
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCPtr( hDC );

    if( dc )
    {
	if (dc->funcs->pSetDeviceGammaRamp)
	    ret = dc->funcs->pSetDeviceGammaRamp(dc->physDev, ptr);
	GDI_ReleaseObj( hDC );
    }
    return ret;
}

/***********************************************************************
 *           GetColorSpace    (GDI32.@)
 */
HCOLORSPACE WINAPI GetColorSpace(HDC hdc)
{
/*FIXME    Need to to whatever GetColorSpace actually does */
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
    UINT ret;
    DC *dc = DC_GetDCPtr( hdc );

    if ( !dc ) return 0;

    if (rect) *rect = dc->BoundsRect;

    ret = ((dc->flags & DC_BOUNDS_SET) ? DCB_SET : DCB_RESET);

    if (flags & DCB_RESET)
    {
        dc->BoundsRect.left   = 0;
        dc->BoundsRect.top    = 0;
        dc->BoundsRect.right  = 0;
        dc->BoundsRect.bottom = 0;
        dc->flags &= ~DC_BOUNDS_SET;
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetBoundsRect    (GDI32.@)
 */
UINT WINAPI SetBoundsRect(HDC hdc, const RECT* rect, UINT flags)
{
    UINT ret;
    DC *dc;

    if ((flags & DCB_ENABLE) && (flags & DCB_DISABLE)) return 0;
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;

    ret = ((dc->flags & DC_BOUNDS_ENABLE) ? DCB_ENABLE : DCB_DISABLE) |
          ((dc->flags & DC_BOUNDS_SET) ? DCB_SET : DCB_RESET);

    if (flags & DCB_RESET)
    {
        dc->BoundsRect.left   = 0;
        dc->BoundsRect.top    = 0;
        dc->BoundsRect.right  = 0;
        dc->BoundsRect.bottom = 0;
        dc->flags &= ~DC_BOUNDS_SET;
    }

    if ((flags & DCB_ACCUMULATE) && rect && rect->left < rect->right && rect->top < rect->bottom)
    {
        if (dc->flags & DC_BOUNDS_SET)
        {
            dc->BoundsRect.left   = min( dc->BoundsRect.left, rect->left );
            dc->BoundsRect.top    = min( dc->BoundsRect.top, rect->top );
            dc->BoundsRect.right  = max( dc->BoundsRect.right, rect->right );
            dc->BoundsRect.bottom = max( dc->BoundsRect.bottom, rect->bottom );
        }
        else
        {
            dc->BoundsRect = *rect;
            dc->flags |= DC_BOUNDS_SET;
        }
    }

    if (flags & DCB_ENABLE) dc->flags |= DC_BOUNDS_ENABLE;
    if (flags & DCB_DISABLE) dc->flags &= ~DC_BOUNDS_ENABLE;

    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *		GetRelAbs		(GDI32.@)
 */
INT WINAPI GetRelAbs( HDC hdc, DWORD dwIgnore )
{
    INT ret = 0;
    DC *dc = DC_GetDCPtr( hdc );
    if (dc) ret = dc->relAbsMode;
    GDI_ReleaseObj( hdc );
    return ret;
}




/***********************************************************************
 *		GetBkMode (GDI32.@)
 */
INT WINAPI GetBkMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->backgroundMode;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *		SetBkMode (GDI32.@)
 */
INT WINAPI SetBkMode( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode <= 0) || (mode > BKMODE_LAST))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;

    ret = dc->backgroundMode;
    if (dc->funcs->pSetBkMode)
        if (!dc->funcs->pSetBkMode( dc->physDev, mode ))
            ret = 0;
    if (ret)
        dc->backgroundMode = mode;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *		GetROP2 (GDI32.@)
 */
INT WINAPI GetROP2( HDC hdc )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->ROPmode;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *		SetROP2 (GDI32.@)
 */
INT WINAPI SetROP2( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode < R2_BLACK) || (mode > R2_WHITE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    ret = dc->ROPmode;
    if (dc->funcs->pSetROP2)
        if (!dc->funcs->pSetROP2( dc->physDev, mode ))
            ret = 0;
    if (ret)
        dc->ROPmode = mode;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *		SetRelAbs (GDI32.@)
 */
INT WINAPI SetRelAbs( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode != ABSOLUTE) && (mode != RELATIVE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    if (dc->funcs->pSetRelAbs)
        ret = dc->funcs->pSetRelAbs( dc->physDev, mode );
    else
    {
        ret = dc->relAbsMode;
        dc->relAbsMode = mode;
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *		GetPolyFillMode (GDI32.@)
 */
INT WINAPI GetPolyFillMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->polyFillMode;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *		SetPolyFillMode (GDI32.@)
 */
INT WINAPI SetPolyFillMode( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode <= 0) || (mode > POLYFILL_LAST))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    ret = dc->polyFillMode;
    if (dc->funcs->pSetPolyFillMode)
        if (!dc->funcs->pSetPolyFillMode( dc->physDev, mode ))
            ret = 0;
    if (ret)
        dc->polyFillMode = mode;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *		GetStretchBltMode (GDI32.@)
 */
INT WINAPI GetStretchBltMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->stretchBltMode;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *		SetStretchBltMode (GDI32.@)
 */
INT WINAPI SetStretchBltMode( HDC hdc, INT mode )
{
    INT ret;
    DC *dc;
    if ((mode <= 0) || (mode > MAXSTRETCHBLTMODE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    ret = dc->stretchBltMode;
    if (dc->funcs->pSetStretchBltMode)
        if (!dc->funcs->pSetStretchBltMode( dc->physDev, mode ))
            ret = 0;
    if (ret)
        dc->stretchBltMode = mode;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *		GetMapMode (GDI32.@)
 */
INT WINAPI GetMapMode( HDC hdc )
{
    INT ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->MapMode;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}


/***********************************************************************
 *		GetBrushOrgEx (GDI32.@)
 */
BOOL WINAPI GetBrushOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    pt->x = dc->brushOrgX;
    pt->y = dc->brushOrgY;
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *		GetCurrentPositionEx (GDI32.@)
 */
BOOL WINAPI GetCurrentPositionEx( HDC hdc, LPPOINT pt )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    pt->x = dc->CursPosX;
    pt->y = dc->CursPosY;
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *		GetViewportExtEx (GDI32.@)
 */
BOOL WINAPI GetViewportExtEx( HDC hdc, LPSIZE size )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    size->cx = dc->vportExtX;
    size->cy = dc->vportExtY;
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *		GetViewportOrgEx (GDI32.@)
 */
BOOL WINAPI GetViewportOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    pt->x = dc->vportOrgX;
    pt->y = dc->vportOrgY;
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *		GetWindowExtEx (GDI32.@)
 */
BOOL WINAPI GetWindowExtEx( HDC hdc, LPSIZE size )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    size->cx = dc->wndExtX;
    size->cy = dc->wndExtY;
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *		GetWindowOrgEx (GDI32.@)
 */
BOOL WINAPI GetWindowOrgEx( HDC hdc, LPPOINT pt )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;
    pt->x = dc->wndOrgX;
    pt->y = dc->wndOrgY;
    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *		InquireVisRgn   (GDI.131)
 */
HRGN16 WINAPI InquireVisRgn16( HDC16 hdc )
{
    HRGN16 ret = 0;
    DC * dc = DC_GetDCPtr( HDC_32(hdc) );
    if (dc)
    {
        ret = HRGN_16(dc->hVisRgn);
        GDI_ReleaseObj( HDC_32(hdc) );
    }
    return ret;
}


/***********************************************************************
 *		GetClipRgn (GDI.173)
 */
HRGN16 WINAPI GetClipRgn16( HDC16 hdc )
{
    HRGN16 ret = 0;
    DC * dc = DC_GetDCPtr( HDC_32(hdc) );
    if (dc)
    {
        ret = HRGN_16(dc->hClipRgn);
        GDI_ReleaseObj( HDC_32(hdc) );
    }
    return ret;
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

    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        layout = dc->layout;
        GDI_ReleaseObj( hdc );
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

    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        oldlayout = dc->layout;
        dc->layout = layout;
        GDI_ReleaseObj( hdc );
    }

    TRACE("hdc : %p, old layout : %08x, new layout : %08x\n", hdc, oldlayout, layout);

    return oldlayout;
}

/***********************************************************************
 *           GetDCBrushColor    (GDI32.@)
 *
 * Retrieves the current brush color for the specified device
 * context (DC).
 *
 */
COLORREF WINAPI GetDCBrushColor(HDC hdc)
{
    DC *dc;
    COLORREF dcBrushColor = CLR_INVALID;

    TRACE("hdc(%p)\n", hdc);

    dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        dcBrushColor = dc->dcBrushColor;
	GDI_ReleaseObj( hdc );
    }

    return dcBrushColor;
}

/***********************************************************************
 *           SetDCBrushColor    (GDI32.@)
 *
 * Sets the current device context (DC) brush color to the specified
 * color value. If the device cannot represent the specified color
 * value, the color is set to the nearest physical color.
 *
 */
COLORREF WINAPI SetDCBrushColor(HDC hdc, COLORREF crColor)
{
    DC *dc;
    COLORREF oldClr = CLR_INVALID;

    TRACE("hdc(%p) crColor(%08x)\n", hdc, crColor);

    dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        if (dc->funcs->pSetDCBrushColor)
            crColor = dc->funcs->pSetDCBrushColor( dc->physDev, crColor );
        else if (dc->hBrush == GetStockObject( DC_BRUSH ))
        {
            /* If DC_BRUSH is selected, update driver pen color */
            HBRUSH hBrush = CreateSolidBrush( crColor );
            dc->funcs->pSelectBrush( dc->physDev, hBrush );
	    DeleteObject( hBrush );
	}

        if (crColor != CLR_INVALID)
        {
            oldClr = dc->dcBrushColor;
            dc->dcBrushColor = crColor;
        }

        GDI_ReleaseObj( hdc );
    }

    return oldClr;
}

/***********************************************************************
 *           GetDCPenColor    (GDI32.@)
 *
 * Retrieves the current pen color for the specified device
 * context (DC).
 *
 */
COLORREF WINAPI GetDCPenColor(HDC hdc)
{
    DC *dc;
    COLORREF dcPenColor = CLR_INVALID;

    TRACE("hdc(%p)\n", hdc);

    dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        dcPenColor = dc->dcPenColor;
	GDI_ReleaseObj( hdc );
    }

    return dcPenColor;
}

/***********************************************************************
 *           SetDCPenColor    (GDI32.@)
 *
 * Sets the current device context (DC) pen color to the specified
 * color value. If the device cannot represent the specified color
 * value, the color is set to the nearest physical color.
 *
 */
COLORREF WINAPI SetDCPenColor(HDC hdc, COLORREF crColor)
{
    DC *dc;
    COLORREF oldClr = CLR_INVALID;

    TRACE("hdc(%p) crColor(%08x)\n", hdc, crColor);

    dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        if (dc->funcs->pSetDCPenColor)
            crColor = dc->funcs->pSetDCPenColor( dc->physDev, crColor );
        else if (dc->hPen == GetStockObject( DC_PEN ))
        {
            /* If DC_PEN is selected, update the driver pen color */
            LOGPEN logpen = { PS_SOLID, { 0, 0 }, crColor };
            HPEN hPen = CreatePenIndirect( &logpen );
            dc->funcs->pSelectPen( dc->physDev, hPen );
	    DeleteObject( hPen );
	}

        if (crColor != CLR_INVALID)
        {
            oldClr = dc->dcPenColor;
            dc->dcPenColor = crColor;
        }

        GDI_ReleaseObj( hdc );
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

/***********************************************************************
 *           SetVirtualResolution   (GDI32.@)
 *
 * Undocumented on msdn.  Called when PowerPoint XP saves a file.
 */
DWORD WINAPI SetVirtualResolution(HDC hdc, DWORD dw2, DWORD dw3, DWORD dw4, DWORD dw5)
{
    FIXME("(%p %08x %08x %08x %08x): stub!\n", hdc, dw2, dw3, dw4, dw5);
    return FALSE;
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

    dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        if (peLimit)
            *peLimit = dc->miterLimit;

        GDI_ReleaseObj( hdc );
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

    dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        if (peOldLimit)
            *peOldLimit = dc->miterLimit;
        dc->miterLimit = eNewLimit;
        GDI_ReleaseObj( hdc );
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
