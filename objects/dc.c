/*
 * GDI Device Context functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#ifndef X_DISPLAY_MISSING
#include "ts_xlib.h"
#include "x11drv.h"
#endif  /* !defined(X_DISPLAY_MISSING) */

#include <stdlib.h>
#include <string.h>
#include "dc.h"
#include "gdi.h"
#include "heap.h"
#include "debug.h"
#include "font.h"
#include "winerror.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(dc)

/***********************************************************************
 *           DC_Init_DC_INFO
 *
 * Fill the WIN_DC_INFO structure.
 */
static void DC_Init_DC_INFO( WIN_DC_INFO *win_dc_info )
{
    win_dc_info->flags               = 0;
    win_dc_info->devCaps             = NULL;
    win_dc_info->hClipRgn            = 0;
    win_dc_info->hVisRgn             = 0;
    win_dc_info->hGCClipRgn          = 0;
    win_dc_info->hPen                = STOCK_BLACK_PEN;
    win_dc_info->hBrush              = STOCK_WHITE_BRUSH;
    win_dc_info->hFont               = STOCK_SYSTEM_FONT;
    win_dc_info->hBitmap             = 0;
    win_dc_info->hFirstBitmap        = 0;
    win_dc_info->hDevice             = 0;
    win_dc_info->hPalette            = STOCK_DEFAULT_PALETTE;
    win_dc_info->ROPmode             = R2_COPYPEN;
    win_dc_info->polyFillMode        = ALTERNATE;
    win_dc_info->stretchBltMode      = BLACKONWHITE;
    win_dc_info->relAbsMode          = ABSOLUTE;
    win_dc_info->backgroundMode      = OPAQUE;
    win_dc_info->backgroundColor     = RGB( 255, 255, 255 );
    win_dc_info->textColor           = RGB( 0, 0, 0 );
    win_dc_info->brushOrgX           = 0;
    win_dc_info->brushOrgY           = 0;
    win_dc_info->textAlign           = TA_LEFT | TA_TOP | TA_NOUPDATECP;
    win_dc_info->charExtra           = 0;
    win_dc_info->breakTotalExtra     = 0;
    win_dc_info->breakCount          = 0;
    win_dc_info->breakExtra          = 0;
    win_dc_info->breakRem            = 0;
    win_dc_info->bitsPerPixel        = 1;
    win_dc_info->MapMode             = MM_TEXT;
    win_dc_info->GraphicsMode        = GM_COMPATIBLE;
    win_dc_info->DCOrgX              = 0;
    win_dc_info->DCOrgY              = 0;
    win_dc_info->lpfnPrint           = NULL;
    win_dc_info->CursPosX            = 0;
    win_dc_info->CursPosY            = 0;
    win_dc_info->ArcDirection        = AD_COUNTERCLOCKWISE;
    win_dc_info->xformWorld2Wnd.eM11 = 1.0f;
    win_dc_info->xformWorld2Wnd.eM12 = 0.0f;
    win_dc_info->xformWorld2Wnd.eM21 = 0.0f;
    win_dc_info->xformWorld2Wnd.eM22 = 1.0f;
    win_dc_info->xformWorld2Wnd.eDx  = 0.0f;
    win_dc_info->xformWorld2Wnd.eDy  = 0.0f;
    win_dc_info->xformWorld2Vport    = win_dc_info->xformWorld2Wnd;
    win_dc_info->xformVport2World    = win_dc_info->xformWorld2Wnd;
    win_dc_info->vport2WorldValid    = TRUE;

    PATH_InitGdiPath(&win_dc_info->path);
}


/***********************************************************************
 *           DC_AllocDC
 */
DC *DC_AllocDC( const DC_FUNCTIONS *funcs )
{
    HDC16 hdc;
    DC *dc;

    if (!(hdc = GDI_AllocObject( sizeof(DC), DC_MAGIC ))) return NULL;
    dc = (DC *) GDI_HEAP_LOCK( hdc );

    dc->hSelf      = hdc;
    dc->funcs      = funcs;
    dc->physDev    = NULL;
    dc->saveLevel  = 0;
    dc->dwHookData = 0L;
    dc->hookProc   = NULL;
    dc->wndOrgX    = 0;
    dc->wndOrgY    = 0;
    dc->wndExtX    = 1;
    dc->wndExtY    = 1;
    dc->vportOrgX  = 0;
    dc->vportOrgY  = 0;
    dc->vportExtX  = 1;
    dc->vportExtY  = 1;

    DC_Init_DC_INFO( &dc->w );

    return dc;
}



/***********************************************************************
 *           DC_GetDCPtr
 */
DC *DC_GetDCPtr( HDC hdc )
{
    GDIOBJHDR *ptr = (GDIOBJHDR *)GDI_HEAP_LOCK( hdc );
    if (!ptr) return NULL;
    if ((ptr->wMagic == DC_MAGIC) || (ptr->wMagic == METAFILE_DC_MAGIC) ||
	(ptr->wMagic == ENHMETAFILE_DC_MAGIC))
        return (DC *)ptr;
    GDI_HEAP_UNLOCK( hdc );
    return NULL;
}


/***********************************************************************
 *           DC_InitDC
 *
 * Setup device-specific DC values for a newly created DC.
 */
void DC_InitDC( DC* dc )
{
    RealizeDefaultPalette16( dc->hSelf );
    SetTextColor( dc->hSelf, dc->w.textColor );
    SetBkColor( dc->hSelf, dc->w.backgroundColor );
    SelectObject( dc->hSelf, dc->w.hPen );
    SelectObject( dc->hSelf, dc->w.hBrush );
    SelectObject( dc->hSelf, dc->w.hFont );
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
    CombineTransform( &dc->w.xformWorld2Vport, &dc->w.xformWorld2Wnd,
        &xformWnd2Vport );

    /* Create inverse of world-to-viewport transformation */
    dc->w.vport2WorldValid = DC_InvertXform( &dc->w.xformWorld2Vport,
        &dc->w.xformVport2World );
}


/***********************************************************************
 *           GetDCState    (GDI.179)
 */
HDC16 WINAPI GetDCState16( HDC16 hdc )
{
    DC * newdc, * dc;
    HGDIOBJ16 handle;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!(handle = GDI_AllocObject( sizeof(DC), DC_MAGIC )))
    {
      GDI_HEAP_UNLOCK( hdc );
      return 0;
    }
    newdc = (DC *) GDI_HEAP_LOCK( handle );

    TRACE(dc, "(%04x): returning %04x\n", hdc, handle );

    newdc->w.flags            = dc->w.flags | DC_SAVED;
    newdc->w.devCaps          = dc->w.devCaps;
    newdc->w.hPen             = dc->w.hPen;       
    newdc->w.hBrush           = dc->w.hBrush;     
    newdc->w.hFont            = dc->w.hFont;      
    newdc->w.hBitmap          = dc->w.hBitmap;    
    newdc->w.hFirstBitmap     = dc->w.hFirstBitmap;
    newdc->w.hDevice          = dc->w.hDevice;
    newdc->w.hPalette         = dc->w.hPalette;   
    newdc->w.totalExtent      = dc->w.totalExtent;
    newdc->w.bitsPerPixel     = dc->w.bitsPerPixel;
    newdc->w.ROPmode          = dc->w.ROPmode;
    newdc->w.polyFillMode     = dc->w.polyFillMode;
    newdc->w.stretchBltMode   = dc->w.stretchBltMode;
    newdc->w.relAbsMode       = dc->w.relAbsMode;
    newdc->w.backgroundMode   = dc->w.backgroundMode;
    newdc->w.backgroundColor  = dc->w.backgroundColor;
    newdc->w.textColor        = dc->w.textColor;
    newdc->w.brushOrgX        = dc->w.brushOrgX;
    newdc->w.brushOrgY        = dc->w.brushOrgY;
    newdc->w.textAlign        = dc->w.textAlign;
    newdc->w.charExtra        = dc->w.charExtra;
    newdc->w.breakTotalExtra  = dc->w.breakTotalExtra;
    newdc->w.breakCount       = dc->w.breakCount;
    newdc->w.breakExtra       = dc->w.breakExtra;
    newdc->w.breakRem         = dc->w.breakRem;
    newdc->w.MapMode          = dc->w.MapMode;
    newdc->w.GraphicsMode     = dc->w.GraphicsMode;
#if 0
    /* Apparently, the DC origin is not changed by [GS]etDCState */
    newdc->w.DCOrgX           = dc->w.DCOrgX;
    newdc->w.DCOrgY           = dc->w.DCOrgY;
#endif
    newdc->w.CursPosX         = dc->w.CursPosX;
    newdc->w.CursPosY         = dc->w.CursPosY;
    newdc->w.ArcDirection     = dc->w.ArcDirection;
    newdc->w.xformWorld2Wnd   = dc->w.xformWorld2Wnd;
    newdc->w.xformWorld2Vport = dc->w.xformWorld2Vport;
    newdc->w.xformVport2World = dc->w.xformVport2World;
    newdc->w.vport2WorldValid = dc->w.vport2WorldValid;
    newdc->wndOrgX            = dc->wndOrgX;
    newdc->wndOrgY            = dc->wndOrgY;
    newdc->wndExtX            = dc->wndExtX;
    newdc->wndExtY            = dc->wndExtY;
    newdc->vportOrgX          = dc->vportOrgX;
    newdc->vportOrgY          = dc->vportOrgY;
    newdc->vportExtX          = dc->vportExtX;
    newdc->vportExtY          = dc->vportExtY;

    newdc->hSelf = (HDC)handle;
    newdc->saveLevel = 0;

    PATH_InitGdiPath( &newdc->w.path );
    
    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

    newdc->w.hGCClipRgn = newdc->w.hVisRgn = 0;
    if (dc->w.hClipRgn)
    {
	newdc->w.hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
	CombineRgn( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
    }
    else
	newdc->w.hClipRgn = 0;
    GDI_HEAP_UNLOCK( handle );
    GDI_HEAP_UNLOCK( hdc );
    return handle;
}


/***********************************************************************
 *           SetDCState    (GDI.180)
 */
void WINAPI SetDCState16( HDC16 hdc, HDC16 hdcs )
{
    DC *dc, *dcs;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return;
    if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC )))
    {
      GDI_HEAP_UNLOCK( hdc );
      return;
    }
    if (!dcs->w.flags & DC_SAVED)
    {
      GDI_HEAP_UNLOCK( hdc );
      GDI_HEAP_UNLOCK( hdcs );
      return;
    }
    TRACE(dc, "%04x %04x\n", hdc, hdcs );

    dc->w.flags            = dcs->w.flags & ~DC_SAVED;
    dc->w.devCaps          = dcs->w.devCaps;
    dc->w.hFirstBitmap     = dcs->w.hFirstBitmap;
    dc->w.hDevice          = dcs->w.hDevice;
    dc->w.totalExtent      = dcs->w.totalExtent;
    dc->w.ROPmode          = dcs->w.ROPmode;
    dc->w.polyFillMode     = dcs->w.polyFillMode;
    dc->w.stretchBltMode   = dcs->w.stretchBltMode;
    dc->w.relAbsMode       = dcs->w.relAbsMode;
    dc->w.backgroundMode   = dcs->w.backgroundMode;
    dc->w.backgroundColor  = dcs->w.backgroundColor;
    dc->w.textColor        = dcs->w.textColor;
    dc->w.brushOrgX        = dcs->w.brushOrgX;
    dc->w.brushOrgY        = dcs->w.brushOrgY;
    dc->w.textAlign        = dcs->w.textAlign;
    dc->w.charExtra        = dcs->w.charExtra;
    dc->w.breakTotalExtra  = dcs->w.breakTotalExtra;
    dc->w.breakCount       = dcs->w.breakCount;
    dc->w.breakExtra       = dcs->w.breakExtra;
    dc->w.breakRem         = dcs->w.breakRem;
    dc->w.MapMode          = dcs->w.MapMode;
    dc->w.GraphicsMode     = dcs->w.GraphicsMode;
#if 0
    /* Apparently, the DC origin is not changed by [GS]etDCState */
    dc->w.DCOrgX           = dcs->w.DCOrgX;
    dc->w.DCOrgY           = dcs->w.DCOrgY;
#endif
    dc->w.CursPosX         = dcs->w.CursPosX;
    dc->w.CursPosY         = dcs->w.CursPosY;
    dc->w.ArcDirection     = dcs->w.ArcDirection;
    dc->w.xformWorld2Wnd   = dcs->w.xformWorld2Wnd;
    dc->w.xformWorld2Vport = dcs->w.xformWorld2Vport;
    dc->w.xformVport2World = dcs->w.xformVport2World;
    dc->w.vport2WorldValid = dcs->w.vport2WorldValid;

    dc->wndOrgX            = dcs->wndOrgX;
    dc->wndOrgY            = dcs->wndOrgY;
    dc->wndExtX            = dcs->wndExtX;
    dc->wndExtY            = dcs->wndExtY;
    dc->vportOrgX          = dcs->vportOrgX;
    dc->vportOrgY          = dcs->vportOrgY;
    dc->vportExtX          = dcs->vportExtX;
    dc->vportExtY          = dcs->vportExtY;

    if (!(dc->w.flags & DC_MEMORY)) dc->w.bitsPerPixel = dcs->w.bitsPerPixel;

    if (dcs->w.hClipRgn)
    {
        if (!dc->w.hClipRgn) dc->w.hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( dc->w.hClipRgn, dcs->w.hClipRgn, 0, RGN_COPY );
    }
    else
    {
        if (dc->w.hClipRgn) DeleteObject16( dc->w.hClipRgn );
        dc->w.hClipRgn = 0;
    }
    CLIPPING_UpdateGCRegion( dc );

    SelectObject( hdc, dcs->w.hBitmap );
    SelectObject( hdc, dcs->w.hBrush );
    SelectObject( hdc, dcs->w.hFont );
    SelectObject( hdc, dcs->w.hPen );
    SetBkColor( hdc, dcs->w.backgroundColor);
    SetTextColor( hdc, dcs->w.textColor);
    GDISelectPalette16( hdc, dcs->w.hPalette, FALSE );
    GDI_HEAP_UNLOCK( hdc );
    GDI_HEAP_UNLOCK( hdcs );
}


/***********************************************************************
 *           SaveDC16    (GDI.30)
 */
INT16 WINAPI SaveDC16( HDC16 hdc )
{
    return (INT16)SaveDC( hdc );
}


/***********************************************************************
 *           SaveDC32    (GDI32.292)
 */
INT WINAPI SaveDC( HDC hdc )
{
    HDC hdcs;
    DC * dc, * dcs;
    INT ret;

    dc = DC_GetDCPtr( hdc );
    if (!dc) return 0;

    if(dc->funcs->pSaveDC)
        return dc->funcs->pSaveDC( dc );

    if (!(hdcs = GetDCState16( hdc )))
    {
      GDI_HEAP_UNLOCK( hdc );
      return 0;
    }
    dcs = (DC *) GDI_HEAP_LOCK( hdcs );

    /* Copy path. The reason why path saving / restoring is in SaveDC/
     * RestoreDC and not in GetDCState/SetDCState is that the ...DCState
     * functions are only in Win16 (which doesn't have paths) and that
     * SetDCState doesn't allow us to signal an error (which can happen
     * when copying paths).
     */
    if (!PATH_AssignGdiPath( &dcs->w.path, &dc->w.path ))
    {
        GDI_HEAP_UNLOCK( hdc );
	GDI_HEAP_UNLOCK( hdcs );
	DeleteDC( hdcs );
	return 0;
    }
    
    dcs->header.hNext = dc->header.hNext;
    dc->header.hNext = hdcs;
    TRACE(dc, "(%04x): returning %d\n", hdc, dc->saveLevel+1 );
    ret = ++dc->saveLevel;
    GDI_HEAP_UNLOCK( hdcs );
    GDI_HEAP_UNLOCK( hdc );
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
 *           RestoreDC32    (GDI32.290)
 */
BOOL WINAPI RestoreDC( HDC hdc, INT level )
{
    DC * dc, * dcs;
    BOOL success;

    TRACE(dc, "%04x %d\n", hdc, level );
    dc = DC_GetDCPtr( hdc );
    if(!dc) return FALSE;
    if(dc->funcs->pRestoreDC)
        return dc->funcs->pRestoreDC( dc, level );

    if (level == -1) level = dc->saveLevel;
    if ((level < 1) || (level > dc->saveLevel))
    {
      GDI_HEAP_UNLOCK( hdc );
      return FALSE;
    }
    
    success=TRUE;
    while (dc->saveLevel >= level)
    {
	HDC16 hdcs = dc->header.hNext;
	if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC )))
	{
	  GDI_HEAP_UNLOCK( hdc );
	  return FALSE;
	}	
	dc->header.hNext = dcs->header.hNext;
	if (--dc->saveLevel < level)
	{
	    SetDCState16( hdc, hdcs );
            if (!PATH_AssignGdiPath( &dc->w.path, &dcs->w.path ))
		/* FIXME: This might not be quite right, since we're
		 * returning FALSE but still destroying the saved DC state */
	        success=FALSE;
	}
	DeleteDC( hdcs );
    }
    GDI_HEAP_UNLOCK( hdc );
    return success;
}


/***********************************************************************
 *           CreateDC16    (GDI.53)
 */
HDC16 WINAPI CreateDC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                         const DEVMODEA *initData )
{
    DC * dc;
    const DC_FUNCTIONS *funcs;
    char buf[300];

    if (device) {
	if(!DRIVER_GetDriverName( device, buf, sizeof(buf) )) return 0;
    } else
        strcpy(buf, driver);

    if (!(funcs = DRIVER_FindDriver( buf ))) return 0;
    if (!(dc = DC_AllocDC( funcs ))) return 0;
    dc->w.flags = 0;

    TRACE(dc, "(driver=%s, device=%s, output=%s): returning %04x\n",
               debugstr_a(driver), debugstr_a(device), debugstr_a(output), dc->hSelf );

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc, driver, device, output, initData ))
    {
        WARN(dc, "creation aborted by device\n" );
        GDI_HEAP_FREE( dc->hSelf );
        return 0;
    }

    DC_InitDC( dc );
    GDI_HEAP_UNLOCK( dc->hSelf );
    return dc->hSelf;
}


/***********************************************************************
 *           CreateDC32A    (GDI32.)
 */
HDC WINAPI CreateDCA( LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODEA *initData )
{
    return CreateDC16( driver, device, output, (const DEVMODEA *)initData );
}


/***********************************************************************
 *           CreateDC32W    (GDI32.)
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
 *           CreateIC32A    (GDI32.49)
 */
HDC WINAPI CreateICA( LPCSTR driver, LPCSTR device, LPCSTR output,
                          const DEVMODEA* initData )
{
      /* Nothing special yet for ICs */
    return CreateDCA( driver, device, output, initData );
}


/***********************************************************************
 *           CreateIC32W    (GDI32.50)
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
 *           CreateCompatibleDC32   (GDI32.31)
 */
HDC WINAPI CreateCompatibleDC( HDC hdc )
{
    DC *dc, *origDC;
    HBITMAP hbitmap;
    const DC_FUNCTIONS *funcs;

    if ((origDC = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC ))) funcs = origDC->funcs;
    else funcs = DRIVER_FindDriver( "DISPLAY" );
    if (!funcs) return 0;

    if (!(dc = DC_AllocDC( funcs ))) return 0;

    TRACE(dc, "(%04x): returning %04x\n",
               hdc, dc->hSelf );

      /* Create default bitmap */
    if (!(hbitmap = CreateBitmap( 1, 1, 1, 1, NULL )))
    {
	GDI_HEAP_FREE( dc->hSelf );
	return 0;
    }
    dc->w.flags        = DC_MEMORY;
    dc->w.bitsPerPixel = 1;
    dc->w.hBitmap      = hbitmap;
    dc->w.hFirstBitmap = hbitmap;

    if (dc->funcs->pCreateDC &&
        !dc->funcs->pCreateDC( dc, NULL, NULL, NULL, NULL ))
    {
        WARN(dc, "creation aborted by device\n");
        DeleteObject( hbitmap );
        GDI_HEAP_FREE( dc->hSelf );
        return 0;
    }

    DC_InitDC( dc );
    GDI_HEAP_UNLOCK( dc->hSelf );
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
 *           DeleteDC32    (GDI32.67)
 */
BOOL WINAPI DeleteDC( HDC hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;

    TRACE(dc, "%04x\n", hdc );

    /* Call hook procedure to check whether is it OK to delete this DC */
    if (    dc->hookProc && !(dc->w.flags & (DC_SAVED | DC_MEMORY))
         && dc->hookProc( hdc, DCHC_DELETEDC, dc->dwHookData, 0 ) == FALSE )
    {
        GDI_HEAP_UNLOCK( hdc );
        return FALSE;
    }

    while (dc->saveLevel)
    {
	DC * dcs;
	HDC16 hdcs = dc->header.hNext;
	if (!(dcs = (DC *) GDI_GetObjPtr( hdcs, DC_MAGIC ))) break;
	dc->header.hNext = dcs->header.hNext;
	dc->saveLevel--;
	DeleteDC( hdcs );
    }
    
    if (!(dc->w.flags & DC_SAVED))
    {
	SelectObject( hdc, STOCK_BLACK_PEN );
	SelectObject( hdc, STOCK_WHITE_BRUSH );
	SelectObject( hdc, STOCK_SYSTEM_FONT );
        if (dc->w.flags & DC_MEMORY) DeleteObject( dc->w.hFirstBitmap );
        if (dc->funcs->pDeleteDC) dc->funcs->pDeleteDC(dc);
    }

    if (dc->w.hClipRgn) DeleteObject( dc->w.hClipRgn );
    if (dc->w.hVisRgn) DeleteObject( dc->w.hVisRgn );
    if (dc->w.hGCClipRgn) DeleteObject( dc->w.hGCClipRgn );
    
    PATH_DestroyGdiPath(&dc->w.path);
    
    return GDI_FreeObject( hdc );
}


/***********************************************************************
 *           ResetDC16    (GDI.376)
 */
HDC16 WINAPI ResetDC16( HDC16 hdc, const DEVMODEA *devmode )
{
    FIXME(dc, "stub\n" );
    return hdc;
}


/***********************************************************************
 *           ResetDC32A    (GDI32.287)
 */
HDC WINAPI ResetDCA( HDC hdc, const DEVMODEA *devmode )
{
    FIXME(dc, "stub\n" );
    return hdc;
}


/***********************************************************************
 *           ResetDC32W    (GDI32.288)
 */
HDC WINAPI ResetDCW( HDC hdc, const DEVMODEW *devmode )
{
    FIXME(dc, "stub\n" );
    return hdc;
}


/***********************************************************************
 *           GetDeviceCaps16    (GDI.80)
 */
INT16 WINAPI GetDeviceCaps16( HDC16 hdc, INT16 cap )
{
    return GetDeviceCaps( hdc, cap );
}


/***********************************************************************
 *           GetDeviceCaps32    (GDI32.171)
 */
INT WINAPI GetDeviceCaps( HDC hdc, INT cap )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    INT ret;
    POINT pt;

    if (!dc) return 0;

    /* Device capabilities for the printer */
    switch (cap)
    {
    case PHYSICALWIDTH:
        if(Escape(hdc, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
	    return pt.x;
    case PHYSICALHEIGHT:
	if(Escape(hdc, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
	    return pt.y;
    case PHYSICALOFFSETX:
	if(Escape(hdc, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
	    return pt.x;
    case PHYSICALOFFSETY:
	if(Escape(hdc, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
	    return pt.y;
    case SCALINGFACTORX:
	if(Escape(hdc, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
	    return pt.x;
    case SCALINGFACTORY:
	if(Escape(hdc, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
	    return pt.y;
    }

    if ((cap < 0) || (cap > sizeof(DeviceCaps)-sizeof(WORD)))
    {
      GDI_HEAP_UNLOCK( hdc );
      return 0;
    }
    
    TRACE(dc, "(%04x,%d): returning %d\n",
	    hdc, cap, *(WORD *)(((char *)dc->w.devCaps) + cap) );
    ret = *(WORD *)(((char *)dc->w.devCaps) + cap);
    GDI_HEAP_UNLOCK( hdc );
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
 *           SetBkColor32    (GDI32.305)
 */
COLORREF WINAPI SetBkColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc) return 0x80000000;
    if (dc->funcs->pSetBkColor)
        oldColor = dc->funcs->pSetBkColor(dc, color);
    else {
	oldColor = dc->w.backgroundColor;
	dc->w.backgroundColor = color;
    }
    GDI_HEAP_UNLOCK( hdc );
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
 *           SetTextColor32    (GDI32.338)
 */
COLORREF WINAPI SetTextColor( HDC hdc, COLORREF color )
{
    COLORREF oldColor;
    DC * dc = DC_GetDCPtr( hdc );
  
    if (!dc) return 0x80000000;
    if (dc->funcs->pSetTextColor)
        oldColor = dc->funcs->pSetTextColor(dc, color);
    else {
	oldColor = dc->w.textColor;
	dc->w.textColor = color;
    }
    GDI_HEAP_UNLOCK( hdc );
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
	prevAlign = dc->w.textAlign;
	dc->w.textAlign = align;
    }
    GDI_HEAP_UNLOCK( hdc );
    return prevAlign;
}

/***********************************************************************
 *           GetDCOrgEx  (GDI32.168)
 */
BOOL WINAPI GetDCOrgEx( HDC hDC, LPPOINT lpp )
{
    DC * dc;

    if (!lpp) return FALSE;
    if (!(dc = (DC *) GDI_GetObjPtr( hDC, DC_MAGIC ))) return FALSE;

#ifndef X_DISPLAY_MISSING
    if (!(dc->w.flags & DC_MEMORY))
    {
       X11DRV_PDEVICE *physDev;
       Window root;
       int w, h, border, depth;

       physDev = (X11DRV_PDEVICE *) dc->physDev;

       /* FIXME: this is not correct for managed windows */
       TSXGetGeometry( display, physDev->drawable, &root,
                    (int*)&lpp->x, (int*)&lpp->y, &w, &h, &border, &depth );
    }
    else 
#endif  /* !defined(X_DISPLAY_MISSING) */
      lpp->x = lpp->y = 0;

    lpp->x += dc->w.DCOrgX; lpp->y += dc->w.DCOrgY;
    GDI_HEAP_UNLOCK( hDC );
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
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    prevOrg = dc->w.DCOrgX | (dc->w.DCOrgY << 16);
    dc->w.DCOrgX = x;
    dc->w.DCOrgY = y;
    GDI_HEAP_UNLOCK( hdc );
    return prevOrg;
}


/***********************************************************************
 *           GetGraphicsMode    (GDI32.188)
 */
INT WINAPI GetGraphicsMode( HDC hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    return dc->w.GraphicsMode;
}


/***********************************************************************
 *           SetGraphicsMode    (GDI32.317)
 */
INT WINAPI SetGraphicsMode( HDC hdc, INT mode )
{
    INT ret;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    /* One would think that setting the graphics mode to GM_COMPATIBLE
     * would also reset the world transformation matrix to the unity
     * matrix. However, in Windows, this is not the case. This doesn't
     * make a lot of sense to me, but that's the way it is.
     */
    
    if (!dc) return 0;
    if ((mode <= 0) || (mode > GM_LAST)) return 0;
    ret = dc->w.GraphicsMode;
    dc->w.GraphicsMode = mode;
    return ret;
}


/***********************************************************************
 *           GetArcDirection16    (GDI.524)
 */
INT16 WINAPI GetArcDirection16( HDC16 hdc )
{
    return GetArcDirection( (HDC)hdc );
}


/***********************************************************************
 *           GetArcDirection32    (GDI32.141)
 */
INT WINAPI GetArcDirection( HDC hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    
    if (!dc)
        return 0;

    return dc->w.ArcDirection;
}


/***********************************************************************
 *           SetArcDirection16    (GDI.525)
 */
INT16 WINAPI SetArcDirection16( HDC16 hdc, INT16 nDirection )
{
    return SetArcDirection( (HDC)hdc, (INT)nDirection );
}


/***********************************************************************
 *           SetArcDirection32    (GDI32.302)
 */
INT WINAPI SetArcDirection( HDC hdc, INT nDirection )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    INT nOldDirection;
    
    if (!dc)
        return 0;

    if (nDirection!=AD_COUNTERCLOCKWISE && nDirection!=AD_CLOCKWISE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }

    nOldDirection = dc->w.ArcDirection;
    dc->w.ArcDirection = nDirection;

    return nOldDirection;
}


/***********************************************************************
 *           GetWorldTransform    (GDI32.244)
 */
BOOL WINAPI GetWorldTransform( HDC hdc, LPXFORM xform )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    
    if (!dc)
        return FALSE;
    if (!xform)
	return FALSE;

    *xform = dc->w.xformWorld2Wnd;
    
    return TRUE;
}


/***********************************************************************
 *           SetWorldTransform    (GDI32.346)
 */
BOOL WINAPI SetWorldTransform( HDC hdc, const XFORM *xform )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    
    if (!dc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!xform)
	return FALSE;
    
    /* Check that graphics mode is GM_ADVANCED */
    if (dc->w.GraphicsMode!=GM_ADVANCED)
       return FALSE;

    dc->w.xformWorld2Wnd = *xform;
    
    DC_UpdateXforms( dc );

    return TRUE;
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
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    
    /* Check for illegal parameters */
    if (!dc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (!xform)
	return FALSE;
    
    /* Check that graphics mode is GM_ADVANCED */
    if (dc->w.GraphicsMode!=GM_ADVANCED)
       return FALSE;
       
    switch (iMode)
    {
        case MWT_IDENTITY:
	    dc->w.xformWorld2Wnd.eM11 = 1.0f;
	    dc->w.xformWorld2Wnd.eM12 = 0.0f;
	    dc->w.xformWorld2Wnd.eM21 = 0.0f;
	    dc->w.xformWorld2Wnd.eM22 = 1.0f;
	    dc->w.xformWorld2Wnd.eDx  = 0.0f;
	    dc->w.xformWorld2Wnd.eDy  = 0.0f;
	    break;
        case MWT_LEFTMULTIPLY:
	    CombineTransform( &dc->w.xformWorld2Wnd, xform,
	        &dc->w.xformWorld2Wnd );
	    break;
	case MWT_RIGHTMULTIPLY:
	    CombineTransform( &dc->w.xformWorld2Wnd, &dc->w.xformWorld2Wnd,
	        xform );
	    break;
        default:
	    return FALSE;
    }

    DC_UpdateXforms( dc );

    return TRUE;
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
BOOL16 WINAPI SetDCHook( HDC16 hdc, FARPROC16 hookProc, DWORD dwHookData )
{
    DC *dc = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC );

    TRACE(dc, "hookProc %08x, default is %08x\n",
                (UINT)hookProc, (UINT)DCHook16 );

    if (!dc) return FALSE;
    dc->hookProc = hookProc;
    dc->dwHookData = dwHookData;
    GDI_HEAP_UNLOCK( hdc );
    return TRUE;
}


/***********************************************************************
 *           GetDCHook   (GDI.191)
 */
DWORD WINAPI GetDCHook( HDC16 hdc, FARPROC16 *phookProc )
{
    DC *dc = (DC *)GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    *phookProc = dc->hookProc;
    GDI_HEAP_UNLOCK( hdc );
    return dc->dwHookData;
}


/***********************************************************************
 *           SetHookFlags       (GDI.192)
 */
WORD WINAPI SetHookFlags16(HDC16 hDC, WORD flags)
{
    DC* dc = (DC*)GDI_GetObjPtr( hDC, DC_MAGIC );

    if( dc )
    {
        WORD wRet = dc->w.flags & DC_DIRTY;

        /* "Undocumented Windows" info is slightly confusing.
         */

        TRACE(dc,"hDC %04x, flags %04x\n",hDC,flags);

        if( flags & DCHF_INVALIDATEVISRGN )
            dc->w.flags |= DC_DIRTY;
        else if( flags & DCHF_VALIDATEVISRGN || !flags )
            dc->w.flags &= ~DC_DIRTY;
	GDI_HEAP_UNLOCK( hDC );
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
 *           GetColorSpace    (GDI32.165)
 */
HCOLORSPACE WINAPI GetColorSpace(HDC hdc)
{
/*FIXME    Need to to whatever GetColorSpace actually does */
    return 0;
}

/***********************************************************************
 *           GetBoundsRect16    (GDI.194)
 */
UINT16 WINAPI GetBoundsRect16(HDC16 hdc, LPRECT16 rect, UINT16 flags)
{
    return DCB_RESET | DCB_DISABLE; /* bounding rectangle always empty and disabled*/
}

/***********************************************************************
 *           GetBoundsRect32    (GDI32.147)
 */
UINT WINAPI GetBoundsRect(HDC hdc, LPRECT rect, UINT flags)
{
    FIXME(dc, "(): stub\n");
    return DCB_RESET;   /* bounding rectangle always empty */
}
 
/***********************************************************************
 *           SetBoundsRect16    (GDI.193)
 */
UINT16 WINAPI SetBoundsRect16(HDC16 hdc, const RECT16* rect, UINT16 flags)
{
    if ( (flags & DCB_ACCUMULATE) || (flags & DCB_ENABLE) )
        FIXME( dc, "(%04x, %p, %04x): stub\n", hdc, rect, flags );

    return DCB_RESET | DCB_DISABLE; /* bounding rectangle always empty and disabled*/
}

/***********************************************************************
 *           SetBoundsRect32    (GDI32.307)
 */
UINT WINAPI SetBoundsRect(HDC hdc, const RECT* rect, UINT flags)
{
    FIXME(dc, "(): stub\n");
    return DCB_DISABLE;   /* bounding rectangle always empty */
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
    MSG("Death(%04x) called. Application enters text mode...\n", hDC);
}

/***********************************************************************
 *           Resurrection    (GDI.122)
 *
 * Restores GDI functionality
 */
void WINAPI Resurrection16(HDC16 hDC,
                           WORD w1, WORD w2, WORD w3, WORD w4, WORD w5, WORD w6)
{
    MSG("Resurrection(%04x, %04x, %04x, %04x, %04x, %04x, %04x) called. Application left text mode.\n", hDC, w1, w2, w3, w4, w5, w6);
}
