/*
 * GDI Device Context functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include "gdi.h"
#include "heap.h"
#include "debugtools.h"
#include "font.h"
#include "callback.h"
#include "winerror.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(dc);


/***********************************************************************
 *           DC_AllocDC
 */
DC *DC_AllocDC( const DC_FUNCTIONS *funcs )
{
    HDC hdc;
    DC *dc;

    if (!(dc = GDI_AllocObject( sizeof(*dc), DC_MAGIC, &hdc ))) return NULL;

    dc->hSelf               = hdc;
    dc->funcs               = funcs;
    dc->physDev             = NULL;
    dc->saveLevel           = 0;
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
    dc->flags               = 0;
    dc->devCaps             = NULL;
    dc->hClipRgn            = 0;
    dc->hVisRgn             = 0;
    dc->hGCClipRgn          = 0;
    dc->hPen                = GetStockObject( BLACK_PEN );
    dc->hBrush              = GetStockObject( WHITE_BRUSH );
    dc->hFont               = GetStockObject( SYSTEM_FONT );
    dc->hBitmap             = 0;
    dc->hDevice             = 0;
    dc->hPalette            = GetStockObject( DEFAULT_PALETTE );
    dc->ROPmode             = R2_COPYPEN;
    dc->polyFillMode        = ALTERNATE;
    dc->stretchBltMode      = BLACKONWHITE;
    dc->relAbsMode          = ABSOLUTE;
    dc->backgroundMode      = OPAQUE;
    dc->backgroundColor     = RGB( 255, 255, 255 );
    dc->textColor           = RGB( 0, 0, 0 );
    dc->brushOrgX           = 0;
    dc->brushOrgY           = 0;
    dc->textAlign           = TA_LEFT | TA_TOP | TA_NOUPDATECP;
    dc->charExtra           = 0;
    dc->breakTotalExtra     = 0;
    dc->breakCount          = 0;
    dc->breakExtra          = 0;
    dc->breakRem            = 0;
    dc->totalExtent.left    = 0;
    dc->totalExtent.top     = 0;
    dc->totalExtent.right   = 0;
    dc->totalExtent.bottom  = 0;
    dc->bitsPerPixel        = 1;
    dc->MapMode             = MM_TEXT;
    dc->GraphicsMode        = GM_COMPATIBLE;
    dc->DCOrgX              = 0;
    dc->DCOrgY              = 0;
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
        dc->flags &= ~DC_DIRTY;
        if (!(dc->flags & (DC_SAVED | DC_MEMORY)))
        {
            DCHOOKPROC proc = dc->hookThunk;
            if (proc)
            {
                DWORD data = dc->dwHookData;
                GDI_ReleaseObj( hdc );
                proc( hdc, DCHC_INVALIDVISRGN, data, 0 );
                if (!(dc = DC_GetDCPtr( hdc ))) break;
                /* otherwise restart the loop in case it became dirty again in the meantime */
            }
        }
    }
    return dc;
}

/***********************************************************************
 *           DC_InitDC
 *
 * Setup device-specific DC values for a newly created DC.
 */
void DC_InitDC( DC* dc )
{
    RealizeDefaultPalette16( dc->hSelf );
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
    XFORM xformWnd2Vport;
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

    /* Combine with the world transformation */
    CombineTransform( &dc->xformWorld2Vport, &dc->xformWorld2Wnd,
        &xformWnd2Vport );

    /* Create inverse of world-to-viewport transformation */
    dc->vport2WorldValid = DC_InvertXform( &dc->xformWorld2Vport,
        &dc->xformVport2World );
}


/***********************************************************************
 *           GetDCState    (GDI.179)
 */
HDC16 WINAPI GetDCState16( HDC16 hdc )
{
    DC * newdc, * dc;
    HGDIOBJ handle;
    
    if (!(dc = DC_GetDCUpdate( hdc ))) return 0;
    if (!(newdc = GDI_AllocObject( sizeof(DC), DC_MAGIC, &handle )))
    {
      GDI_ReleaseObj( hdc );
      return 0;
    }
    TRACE("(%04x): returning %04x\n", hdc, handle );

    newdc->flags            = dc->flags | DC_SAVED;
    newdc->devCaps          = dc->devCaps;
    newdc->hPen             = dc->hPen;       
    newdc->hBrush           = dc->hBrush;     
    newdc->hFont            = dc->hFont;      
    newdc->hBitmap          = dc->hBitmap;    
    newdc->hDevice          = dc->hDevice;
    newdc->hPalette         = dc->hPalette;   
    newdc->totalExtent      = dc->totalExtent;
    newdc->bitsPerPixel     = dc->bitsPerPixel;
    newdc->ROPmode          = dc->ROPmode;
    newdc->polyFillMode     = dc->polyFillMode;
    newdc->stretchBltMode   = dc->stretchBltMode;
    newdc->relAbsMode       = dc->relAbsMode;
    newdc->backgroundMode   = dc->backgroundMode;
    newdc->backgroundColor  = dc->backgroundColor;
    newdc->textColor        = dc->textColor;
    newdc->brushOrgX        = dc->brushOrgX;
    newdc->brushOrgY        = dc->brushOrgY;
    newdc->textAlign        = dc->textAlign;
    newdc->charExtra        = dc->charExtra;
    newdc->breakTotalExtra  = dc->breakTotalExtra;
    newdc->breakCount       = dc->breakCount;
    newdc->breakExtra       = dc->breakExtra;
    newdc->breakRem         = dc->breakRem;
    newdc->MapMode          = dc->MapMode;
    newdc->GraphicsMode     = dc->GraphicsMode;
#if 0
    /* Apparently, the DC origin is not changed by [GS]etDCState */
    newdc->DCOrgX           = dc->DCOrgX;
    newdc->DCOrgY           = dc->DCOrgY;
#endif
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

    newdc->hSelf = (HDC)handle;
    newdc->saveLevel = 0;

    PATH_InitGdiPath( &newdc->path );
    
    newdc->pAbortProc = NULL;
    newdc->hookThunk  = NULL;
    newdc->hookProc   = 0;

    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

    newdc->hGCClipRgn = newdc->hVisRgn = 0;
    if (dc->hClipRgn)
    {
	newdc->hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
	CombineRgn( newdc->hClipRgn, dc->hClipRgn, 0, RGN_COPY );
    }
    else
	newdc->hClipRgn = 0;
    GDI_ReleaseObj( handle );
    GDI_ReleaseObj( hdc );
    return handle;
}


/***********************************************************************
 *           SetDCState    (GDI.180)
 */
void WINAPI SetDCState16( HDC16 hdc, HDC16 hdcs )
{
    DC *dc, *dcs;

    if (!(dc = GDI_GetObjPtr( hdc, DC_MAGIC ))) return;
    if (!(dcs = GDI_GetObjPtr( hdcs, DC_MAGIC )))
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
    TRACE("%04x %04x\n", hdc, hdcs );

    dc->flags            = dcs->flags & ~DC_SAVED;
    dc->devCaps          = dcs->devCaps;
    dc->hDevice          = dcs->hDevice;
    dc->totalExtent      = dcs->totalExtent;
    dc->ROPmode          = dcs->ROPmode;
    dc->polyFillMode     = dcs->polyFillMode;
    dc->stretchBltMode   = dcs->stretchBltMode;
    dc->relAbsMode       = dcs->relAbsMode;
    dc->backgroundMode   = dcs->backgroundMode;
    dc->backgroundColor  = dcs->backgroundColor;
    dc->textColor        = dcs->textColor;
    dc->brushOrgX        = dcs->brushOrgX;
    dc->brushOrgY        = dcs->brushOrgY;
    dc->textAlign        = dcs->textAlign;
    dc->charExtra        = dcs->charExtra;
    dc->breakTotalExtra  = dcs->breakTotalExtra;
    dc->breakCount       = dcs->breakCount;
    dc->breakExtra       = dcs->breakExtra;
    dc->breakRem         = dcs->breakRem;
    dc->MapMode          = dcs->MapMode;
    dc->GraphicsMode     = dcs->GraphicsMode;
#if 0
    /* Apparently, the DC origin is not changed by [GS]etDCState */
    dc->DCOrgX           = dcs->DCOrgX;
    dc->DCOrgY           = dcs->DCOrgY;
#endif
    dc->CursPosX         = dcs->CursPosX;
    dc->CursPosY         = dcs->CursPosY;
    dc->ArcDirection     = dcs->ArcDirection;
    dc->xformWorld2Wnd   = dcs->xformWorld2Wnd;
    dc->xformWorld2Vport = dcs->xformWorld2Vport;
    dc->xformVport2World = dcs->xformVport2World;
    dc->vport2WorldValid = dcs->vport2WorldValid;

    dc->wndOrgX          = dcs->wndOrgX;
    dc->wndOrgY          = dcs->wndOrgY;
    dc->wndExtX          = dcs->wndExtX;
    dc->wndExtY          = dcs->wndExtY;
    dc->vportOrgX        = dcs->vportOrgX;
    dc->vportOrgY        = dcs->vportOrgY;
    dc->vportExtX        = dcs->vportExtX;
    dc->vportExtY        = dcs->vportExtY;

    if (!(dc->flags & DC_MEMORY)) dc->bitsPerPixel = dcs->bitsPerPixel;

    if (dcs->hClipRgn)
    {
        if (!dc->hClipRgn) dc->hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( dc->hClipRgn, dcs->hClipRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->hClipRgn) DeleteObject16( dc->hClipRgn );
        dc->hClipRgn = 0;
    }
    CLIPPING_UpdateGCRegion( dc );

    SelectObject( hdc, dcs->hBitmap );
    SelectObject( hdc, dcs->hBrush );
    SelectObject( hdc, dcs->hFont );
    SelectObject( hdc, dcs->hPen );
    SetBkColor( hdc, dcs->backgroundColor);
    SetTextColor( hdc, dcs->textColor);
    GDISelectPalette16( hdc, dcs->hPalette, FALSE );
    GDI_ReleaseObj( hdcs );
    GDI_ReleaseObj( hdc );
}


/***********************************************************************
 *           SaveDC16    (GDI.30)
 */
INT16 WINAPI SaveDC16( HDC16 hdc )
{
    return (INT16)SaveDC( hdc );
}


/***********************************************************************
 *           SaveDC    (GDI32.292)
 */
INT WINAPI SaveDC( HDC hdc )
{
    HDC hdcs;
    DC * dc, * dcs;
    INT ret;

    dc = DC_GetDCUpdate( hdc );
    if (!dc) return 0;

    if(dc->funcs->pSaveDC)
    {
        ret = dc->funcs->pSaveDC( dc );
        GDI_ReleaseObj( hdc );
        return ret;
    }

    if (!(hdcs = GetDCState16( hdc )))
    {
      GDI_ReleaseObj( hdc );
      return 0;
    }
    dcs = GDI_GetObjPtr( hdcs, DC_MAGIC );

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

    dcs->header.hNext = dc->header.hNext;
    dc->header.hNext = hdcs;
    TRACE("(%04x): returning %d\n", hdc, dc->saveLevel+1 );
    ret = ++dc->saveLevel;
    GDI_ReleaseObj( hdcs );
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           RestoreDC16    (GDI.39)
 */
BOOL16 WINAPI RestoreDC16( HDC16 hdc, INT16 level )
{
    return RestoreDC( hdc, level );
}


/***********************************************************************
 *           RestoreDC    (GDI32.290)
 */
BOOL WINAPI RestoreDC( HDC hdc, INT level )
{
    DC * dc, * dcs;
    BOOL success;

    TRACE("%04x %d\n", hdc, level );
    dc = DC_GetDCPtr( hdc );
    if(!dc) return FALSE;
    if(dc->funcs->pRestoreDC)
    {
        success = dc->funcs->pRestoreDC( dc, level );
        GDI_ReleaseObj( hdc );
        return success;
    }

    if (level == -1) level = dc->saveLevel;
    if ((level < 1) 
            /* This pair of checks disagrees with MSDN "Platform SDK:
               Windows GDI" July 2000 which says all negative values
               for level will be interpreted as an instance relative
               to the current state.  Restricting it to just -1 does
               not satisfy this */
	|| (level > dc->saveLevel))
    {
        GDI_ReleaseObj( hdc );
        return FALSE;
    }
    
    success=TRUE;
    while (dc->saveLevel >= level)
    {
	HDC16 hdcs = dc->header.hNext;
	if (!(dcs = GDI_GetObjPtr( hdcs, DC_MAGIC )))
	{
	  GDI_ReleaseObj( hdc );
	  return FALSE;
	}	
	dc->header.hNext = dcs->header.hNext;
	if (--dc->saveLevel < level)
	{
	    SetDCState16( hdc, hdcs );
            if (!PATH_AssignGdiPath( &dc->path, &dcs->path ))
		/* FIXME: This might not be quite right, since we're
		 * returning FALSE but still destroying the saved DC state */
	        success=FALSE;
	}
        GDI_ReleaseObj( hdcs );
	DeleteDC( hdcs );
    }
    GDI_ReleaseObj( hdc );
    return success;
}


/***********************************************************************
 *           CreateDC16    (GDI.53)
 */
HDC16 WINAPI CreateDC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                         const DEVMODEA *initData )
{
    HDC hdc;
    DC * dc;
    const DC_FUNCTIONS *funcs;
    char buf[300];

    if (!device || !DRIVER_GetDriverName( device, buf, sizeof(buf) ))
        strcpy(buf, driver);

    if (!(funcs = DRIVER_FindDriver( buf ))) return 0;
    if (!(dc = DC_AllocDC( funcs ))) return 0;
    dc->flags = 0;

    TRACE("(driver=%s, device=%s, output=%s): returning %04x\n",
               debugstr_a(driver), debugstr_a(device), debugstr_a(output), dc->hSelf );

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc, buf, device, output, initData ))
    {
        WARN("creation aborted by device\n" );
        GDI_FreeObject( dc->hSelf, dc );
        return 0;
    }

    DC_InitDC( dc );
    hdc = dc->hSelf;
    GDI_ReleaseObj( hdc );
    return hdc;
}


/***********************************************************************
 *           CreateDCA    (GDI32.)
 */
HDC WINAPI CreateDCA( LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODEA *initData )
{
    return CreateDC16( driver, device, output, (const DEVMODEA *)initData );
}


/***********************************************************************
 *           CreateDCW    (GDI32.)
 */
HDC WINAPI CreateDCW( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                          const DEVMODEW *initData )
{ 
    LPSTR driverA = HEAP_strdupWtoA( GetProcessHeap(), 0, driver );
    LPSTR deviceA = HEAP_strdupWtoA( GetProcessHeap(), 0, device );
    LPSTR outputA = HEAP_strdupWtoA( GetProcessHeap(), 0, output );
    HDC res = CreateDC16( driverA, deviceA, outputA,
                            (const DEVMODEA *)initData /*FIXME*/ );
    HeapFree( GetProcessHeap(), 0, driverA );
    HeapFree( GetProcessHeap(), 0, deviceA );
    HeapFree( GetProcessHeap(), 0, outputA );
    return res;
}


/***********************************************************************
 *           CreateIC16    (GDI.153)
 */
HDC16 WINAPI CreateIC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                         const DEVMODEA* initData )
{
      /* Nothing special yet for ICs */
    return CreateDC16( driver, device, output, initData );
}


/***********************************************************************
 *           CreateICA    (GDI32.49)
 */
HDC WINAPI CreateICA( LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODEA* initData )
{
      /* Nothing special yet for ICs */
    return CreateDCA( driver, device, output, initData );
}


/***********************************************************************
 *           CreateICW    (GDI32.50)
 */
HDC WINAPI CreateICW( LPCWSTR driver, LPCWSTR device, LPCWSTR output,
                          const DEVMODEW* initData )
{
      /* Nothing special yet for ICs */
    return CreateDCW( driver, device, output, initData );
}


/***********************************************************************
 *           CreateCompatibleDC16    (GDI.52)
 */
HDC16 WINAPI CreateCompatibleDC16( HDC16 hdc )
{
    return (HDC16)CreateCompatibleDC( hdc );
}


/***********************************************************************
 *           CreateCompatibleDC   (GDI32.31)
 */
HDC WINAPI CreateCompatibleDC( HDC hdc )
{
    DC *dc, *origDC;
    const DC_FUNCTIONS *funcs;

    if ((origDC = GDI_GetObjPtr( hdc, DC_MAGIC ))) funcs = origDC->funcs;
    else funcs = DRIVER_FindDriver( "DISPLAY" );

    if (!funcs || !(dc = DC_AllocDC( funcs )))
    {
        if (origDC) GDI_ReleaseObj( hdc );
        return 0;
    }

    TRACE("(%04x): returning %04x\n",
               hdc, dc->hSelf );

    dc->flags        = DC_MEMORY;
    dc->bitsPerPixel = 1;
    dc->hBitmap      = hPseudoStockBitmap;

    /* Copy the driver-specific physical device info into
     * the new DC. The driver may use this read-only info
     * while creating the compatible DC below. */
    if (origDC)
        dc->physDev = origDC->physDev;

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc, NULL, NULL, NULL, NULL ))
    {
        WARN("creation aborted by device\n");
        GDI_FreeObject( dc->hSelf, dc );
	if (origDC) GDI_ReleaseObj( hdc );
        return 0;
    }

    DC_InitDC( dc );
    GDI_ReleaseObj( dc->hSelf );
    if (origDC) GDI_ReleaseObj( hdc );
    return dc->hSelf;
}


/***********************************************************************
 *           DeleteDC16    (GDI.68)
 */
BOOL16 WINAPI DeleteDC16( HDC16 hdc )
{
    return DeleteDC( hdc );
}


/***********************************************************************
 *           DeleteDC    (GDI32.67)
 */
BOOL WINAPI DeleteDC( HDC hdc )
{
    DC * dc = GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    TRACE("%04x\n", hdc );

    /* Call hook procedure to check whether is it OK to delete this DC */
    if (dc->hookThunk && !(dc->flags & (DC_SAVED | DC_MEMORY)))
    {
        DCHOOKPROC proc = dc->hookThunk;
        if (proc)
        {
            DWORD data = dc->dwHookData;
            GDI_ReleaseObj( hdc );
            if (!proc( hdc, DCHC_DELETEDC, data, 0 )) return FALSE;
            if (!(dc = DC_GetDCPtr( hdc ))) return FALSE;
        }
    }

    while (dc->saveLevel)
    {
	DC * dcs;
	HDC16 hdcs = dc->header.hNext;
	if (!(dcs = GDI_GetObjPtr( hdcs, DC_MAGIC ))) break;
	dc->header.hNext = dcs->header.hNext;
	dc->saveLevel--;
        GDI_ReleaseObj( hdcs );
	DeleteDC( hdcs );
    }
    
    if (!(dc->flags & DC_SAVED))
    {
	SelectObject( hdc, GetStockObject(BLACK_PEN) );
	SelectObject( hdc, GetStockObject(WHITE_BRUSH) );
	SelectObject( hdc, GetStockObject(SYSTEM_FONT) );
        if (dc->funcs->pDeleteDC) dc->funcs->pDeleteDC(dc);
    }

    if (dc->hClipRgn) DeleteObject( dc->hClipRgn );
    if (dc->hVisRgn) DeleteObject( dc->hVisRgn );
    if (dc->hGCClipRgn) DeleteObject( dc->hGCClipRgn );
    if (dc->pAbortProc) THUNK_Free( (FARPROC)dc->pAbortProc );
    if (dc->hookThunk) THUNK_Free( (FARPROC)dc->hookThunk );
    PATH_DestroyGdiPath(&dc->path);
    
    return GDI_FreeObject( hdc, dc );
}


/***********************************************************************
 *           ResetDC16    (GDI.376)
 */
HDC16 WINAPI ResetDC16( HDC16 hdc, const DEVMODEA *devmode )
{
    FIXME("stub\n" );
    return hdc;
}


/***********************************************************************
 *           ResetDCA    (GDI32.287)
 */
HDC WINAPI ResetDCA( HDC hdc, const DEVMODEA *devmode )
{
    FIXME("stub\n" );
    return hdc;
}


/***********************************************************************
 *           ResetDCW    (GDI32.288)
 */
HDC WINAPI ResetDCW( HDC hdc, const DEVMODEW *devmode )
{
    FIXME("stub\n" );
    return hdc;
}


/***********************************************************************
 *           GetDeviceCaps16    (GDI.80)
 */
INT16 WINAPI GetDeviceCaps16( HDC16 hdc, INT16 cap )
{
    INT16 ret = GetDeviceCaps( hdc, cap );
    /* some apps don't expect -1 and think it's a B&W screen */
    if ((cap == NUMCOLORS) && (ret == -1)) ret = 2048;
    return ret;
}


/***********************************************************************
 *           GetDeviceCaps    (GDI32.171)
 */
INT WINAPI GetDeviceCaps( HDC hdc, INT cap )
{
    DC *dc;
    INT ret = 0;
    POINT pt;

    /* Device capabilities for the printer */
    switch (cap)
    {
    case PHYSICALWIDTH:
        if(Escape(hdc, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0) ret = pt.x;
        break;
    case PHYSICALHEIGHT:
        if(Escape(hdc, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0) ret = pt.y;
        break;
    case PHYSICALOFFSETX:
        if(Escape(hdc, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0) ret = pt.x;
        break;
    case PHYSICALOFFSETY:
        if(Escape(hdc, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0) ret = pt.y;
        break;
    case SCALINGFACTORX:
        if(Escape(hdc, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0) ret = pt.x;
        break;
    case SCALINGFACTORY:
        if(Escape(hdc, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0) ret = pt.y;
        break;
    case CAPS1:
        FIXME("(%04x,%d): DeviceCaps param CAPS1 is UNIMPLEMENTED, will yield 0!\n",
                  hdc,cap );

        /* please see wingdi.h for the possible bit-flag values that need
           to be returned.

           also, see 
           http://msdn.microsoft.com/library/ddkdoc/win95ddk/graphcnt_1m0p.htm

           the fall-through to 'default' is on purpose */


    default:
        if ((cap < 0) || (cap > sizeof(DeviceCaps)-sizeof(WORD))) break;

        if (((cap>=46) && (cap<88)) || ((cap>=92) && (cap<104)))
            FIXME("(%04x,%d): unsupported DeviceCaps capability, will yield 0!\n",
                  hdc,cap );
        if ((dc = DC_GetDCPtr( hdc )))
        {
            ret = *(WORD *)(((char *)dc->devCaps) + cap);
            GDI_ReleaseObj( hdc );
            if ((cap == NUMCOLORS) && (ret == 0xffff)) ret = -1;
        }
        break;
    }

    TRACE("(%04x,%d): returning %d\n", hdc, cap, ret );
    return ret;
}


/***********************************************************************
 *           SetBkColor16    (GDI.1)
 */
COLORREF WINAPI SetBkColor16( HDC16 hdc, COLORREF color )
{
    return SetBkColor( hdc, color );
}


/***********************************************************************
 *           SetBkColor    (GDI32.305)
 */
COLORREF WINAPI SetBkColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc) return 0x80000000;
    if (dc->funcs->pSetBkColor)
        oldColor = dc->funcs->pSetBkColor(dc, color);
    else {
	oldColor = dc->backgroundColor;
	dc->backgroundColor = color;
    }
    GDI_ReleaseObj( hdc );
    return oldColor;
}


/***********************************************************************
 *           SetTextColor16    (GDI.9)
 */
COLORREF WINAPI SetTextColor16( HDC16 hdc, COLORREF color )
{
    return SetTextColor( hdc, color );
}


/***********************************************************************
 *           SetTextColor    (GDI32.338)
 */
COLORREF WINAPI SetTextColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc) return 0x80000000;
    if (dc->funcs->pSetTextColor)
        oldColor = dc->funcs->pSetTextColor(dc, color);
    else {
	oldColor = dc->textColor;
	dc->textColor = color;
    }
    GDI_ReleaseObj( hdc );
    return oldColor;
}

/***********************************************************************
 *           SetTextAlign16    (GDI.346)
 */
UINT16 WINAPI SetTextAlign16( HDC16 hdc, UINT16 align )
{
    return SetTextAlign( hdc, align );
}


/***********************************************************************
 *           SetTextAlign    (GDI32.336)
 */
UINT WINAPI SetTextAlign( HDC hdc, UINT align )
{
    UINT prevAlign;
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return 0x0;
    if (dc->funcs->pSetTextAlign)
        prevAlign = dc->funcs->pSetTextAlign(dc, align);
    else {
	prevAlign = dc->textAlign;
	dc->textAlign = align;
    }
    GDI_ReleaseObj( hdc );
    return prevAlign;
}

/***********************************************************************
 *           GetDCOrgEx  (GDI32.168)
 */
BOOL WINAPI GetDCOrgEx( HDC hDC, LPPOINT lpp )
{
    DC * dc;

    if (!lpp) return FALSE;
    if (!(dc = DC_GetDCPtr( hDC ))) return FALSE;

    lpp->x = lpp->y = 0;
    if (dc->funcs->pGetDCOrgEx) dc->funcs->pGetDCOrgEx( dc, lpp );
    lpp->x += dc->DCOrgX;
    lpp->y += dc->DCOrgY;
    GDI_ReleaseObj( hDC );
    return TRUE;
}


/***********************************************************************
 *           GetDCOrg    (GDI.79)
 */
DWORD WINAPI GetDCOrg16( HDC16 hdc )
{
    POINT	pt;
    if( GetDCOrgEx( hdc, &pt) )
  	return MAKELONG( (WORD)pt.x, (WORD)pt.y );    
    return 0;
}


/***********************************************************************
 *           SetDCOrg    (GDI.117)
 */
DWORD WINAPI SetDCOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    DWORD prevOrg;
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    prevOrg = dc->DCOrgX | (dc->DCOrgY << 16);
    dc->DCOrgX = x;
    dc->DCOrgY = y;
    GDI_ReleaseObj( hdc );
    return prevOrg;
}


/***********************************************************************
 *           SetGraphicsMode    (GDI32.317)
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
    if ((mode > 0) || (mode <= GM_LAST))
    {
        ret = dc->GraphicsMode;
        dc->GraphicsMode = mode;
    }
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           SetArcDirection16    (GDI.525)
 */
INT16 WINAPI SetArcDirection16( HDC16 hdc, INT16 nDirection )
{
    return SetArcDirection( (HDC)hdc, (INT)nDirection );
}


/***********************************************************************
 *           SetArcDirection    (GDI32.302)
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
        nOldDirection = dc->ArcDirection;
        dc->ArcDirection = nDirection;
        GDI_ReleaseObj( hdc );
    }
    return nOldDirection;
}


/***********************************************************************
 *           GetWorldTransform    (GDI32.244)
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
 *           SetWorldTransform    (GDI32.346)
 */
BOOL WINAPI SetWorldTransform( HDC hdc, const XFORM *xform )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return FALSE;
    if (!xform) goto done;

    /* Check that graphics mode is GM_ADVANCED */
    if (dc->GraphicsMode!=GM_ADVANCED) goto done;

    dc->xformWorld2Wnd = *xform;
    DC_UpdateXforms( dc );
    ret = TRUE;
 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/****************************************************************************
 * ModifyWorldTransform [GDI32.253]
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
 * RETURNS STD
 */
BOOL WINAPI ModifyWorldTransform( HDC hdc, const XFORM *xform,
    DWORD iMode )
{
    BOOL ret = FALSE;
    DC *dc = DC_GetDCPtr( hdc );

    /* Check for illegal parameters */
    if (!dc) return FALSE;
    if (!xform) goto done;

    /* Check that graphics mode is GM_ADVANCED */
    if (dc->GraphicsMode!=GM_ADVANCED) goto done;

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
 * CombineTransform [GDI32.20]
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
 * RETURNS STD
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
 *           SetDCHook   (GDI.190)
 */
/* ### start build ### */
extern WORD CALLBACK GDI_CallTo16_word_wwll(FARPROC16,WORD,WORD,LONG,LONG);
/* ### stop build ### */
BOOL16 WINAPI SetDCHook( HDC16 hdc, FARPROC16 hookProc, DWORD dwHookData )
{
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return FALSE;

    /*
     * Note: We store the original SEGPTR 'hookProc' as we need to be
     *       able to return it verbatim in GetDCHook,
     *
     *       On the other hand, we still call THUNK_Alloc and store the
     *       32-bit thunk into another DC member, because THUNK_Alloc
     *       recognizes the (typical) case that the 'hookProc' is indeed
     *       the 16-bit API entry point of a built-in routine (e.g. DCHook16)
     *
     *       We could perform that test every time the hook is about to
     *       be called (or else we could live with the 32->16->32 detour),
     *       but this way is the most efficient ...
     */

    dc->hookProc = hookProc;
    dc->dwHookData = dwHookData;

    THUNK_Free( (FARPROC)dc->hookThunk );
    dc->hookThunk = (DCHOOKPROC)
                    THUNK_Alloc( hookProc, (RELAY)GDI_CallTo16_word_wwll );

    GDI_ReleaseObj( hdc );
    return TRUE;
}


/***********************************************************************
 *           GetDCHook   (GDI.191)
 */
DWORD WINAPI GetDCHook( HDC16 hdc, FARPROC16 *phookProc )
{
    DC *dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;
    *phookProc = dc->hookProc;
    GDI_ReleaseObj( hdc );
    return dc->dwHookData;
}


/***********************************************************************
 *           SetHookFlags       (GDI.192)
 */
WORD WINAPI SetHookFlags16(HDC16 hDC, WORD flags)
{
    DC *dc = DC_GetDCPtr( hDC );

    if( dc )
    {
        WORD wRet = dc->flags & DC_DIRTY;

        /* "Undocumented Windows" info is slightly confusing.
         */

        TRACE("hDC %04x, flags %04x\n",hDC,flags);

        if( flags & DCHF_INVALIDATEVISRGN )
            dc->flags |= DC_DIRTY;
        else if( flags & DCHF_VALIDATEVISRGN || !flags )
            dc->flags &= ~DC_DIRTY;
	GDI_ReleaseObj( hDC );
        return wRet;
    }
    return 0;
}

/***********************************************************************
 *           SetICMMode    (GDI32.318)
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
 *           GetDeviceGammaRamp    (GDI32.*)
 */
BOOL WINAPI GetDeviceGammaRamp(HDC hdc,LPVOID ptr) {
    FIXME("(%x,%p), empty stub!\n",hdc,ptr);
    return FALSE;
}

/***********************************************************************
 *           GetColorSpace    (GDI32.165)
 */
HCOLORSPACE WINAPI GetColorSpace(HDC hdc)
{
/*FIXME    Need to to whatever GetColorSpace actually does */
    return 0;
}

/***********************************************************************
 *           CreateColorSpaceA    (GDI32.???)
 */
HCOLORSPACE WINAPI CreateColorSpaceA( LPLOGCOLORSPACEA lpLogColorSpace )
{
  FIXME( "stub\n" );
  return 0; 
}

/***********************************************************************
 *           CreateColorSpaceW    (GDI32.???)
 */
HCOLORSPACE WINAPI CreateColorSpaceW( LPLOGCOLORSPACEW lpLogColorSpace )
{
  FIXME( "stub\n" );
  return 0;
}

/***********************************************************************
 *           DeleteColorSpace     (GDI32.???)
 */
BOOL WINAPI DeleteColorSpace( HCOLORSPACE hColorSpace )
{
  FIXME( "stub\n" );
  
  return TRUE;
}

/***********************************************************************
 *           SetColorSpace     (GDI32.???)
 */
HCOLORSPACE WINAPI SetColorSpace( HDC hDC, HCOLORSPACE hColorSpace )
{
  FIXME( "stub\n" );

  return hColorSpace;
}

/***********************************************************************
 *           GetBoundsRect16    (GDI.194)
 */
UINT16 WINAPI GetBoundsRect16(HDC16 hdc, LPRECT16 rect, UINT16 flags)
{
    return DCB_RESET | DCB_DISABLE; /* bounding rectangle always empty and disabled*/
}

/***********************************************************************
 *           GetBoundsRect    (GDI32.147)
 */
UINT WINAPI GetBoundsRect(HDC hdc, LPRECT rect, UINT flags)
{
    FIXME("(): stub\n");
    return DCB_RESET;   /* bounding rectangle always empty */
}
 
/***********************************************************************
 *           SetBoundsRect16    (GDI.193)
 */
UINT16 WINAPI SetBoundsRect16(HDC16 hdc, const RECT16* rect, UINT16 flags)
{
    if ( (flags & DCB_ACCUMULATE) || (flags & DCB_ENABLE) )
        FIXME("(%04x, %p, %04x): stub\n", hdc, rect, flags );

    return DCB_RESET | DCB_DISABLE; /* bounding rectangle always empty and disabled*/
}

/***********************************************************************
 *           SetBoundsRect    (GDI32.307)
 */
UINT WINAPI SetBoundsRect(HDC hdc, const RECT* rect, UINT flags)
{
    FIXME("(): stub\n");
    return DCB_DISABLE;   /* bounding rectangle always empty */
}


/***********************************************************************
 *		GetRelAbs		(GDI32.218)
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
 *           Death    (GDI.121)
 *
 * Disables GDI, switches back to text mode.
 * We don't have to do anything here,
 * just let console support handle everything
 */
void WINAPI Death16(HDC16 hDC)
{
    MESSAGE("Death(%04x) called. Application enters text mode...\n", hDC);
}

/***********************************************************************
 *           Resurrection    (GDI.122)
 *
 * Restores GDI functionality
 */
void WINAPI Resurrection16(HDC16 hDC,
                           WORD w1, WORD w2, WORD w3, WORD w4, WORD w5, WORD w6)
{
    MESSAGE("Resurrection(%04x, %04x, %04x, %04x, %04x, %04x, %04x) called. Application left text mode.\n", hDC, w1, w2, w3, w4, w5, w6);
}

/***********************************************************************
 *           GetLayout    (GDI32.321)
 *
 * Gets left->right or right->left text layout flags of a dc.
 * win98 just returns 0 and sets ERROR_CALL_NOT_IMPLEMENTED so we do the same
 *
 */
DWORD WINAPI GetLayout(HDC hdc)
{
    FIXME("(%08x): stub\n", hdc);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *           SetLayout    (GDI32.450)
 *
 * Sets left->right or right->left text layout flags of a dc.
 * win98 just returns 0 and sets ERROR_CALL_NOT_IMPLEMENTED so we do the same
 *
 */
DWORD WINAPI SetLayout(HDC hdc, DWORD layout)
{
    FIXME("(%08x,%08lx): stub\n", hdc, layout);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
