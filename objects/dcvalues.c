/*
 * DC device-independent Get/SetXXX functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include "config.h"

#include "winbase.h"
#include "winerror.h"

#include "gdi.h"


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
    if (dc->funcs->pSetBkMode)
        ret = dc->funcs->pSetBkMode( dc, mode );
    else
    {
        ret = dc->backgroundMode;
        dc->backgroundMode = mode;
    }
    GDI_ReleaseObj( hdc );
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
    if (dc->funcs->pSetROP2)
        ret = dc->funcs->pSetROP2( dc, mode );
    else
    {
        ret = dc->ROPmode;
        dc->ROPmode = mode;
    }
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
        ret = dc->funcs->pSetRelAbs( dc, mode );
    else
    {
        ret = dc->relAbsMode;
        dc->relAbsMode = mode;
    }
    GDI_ReleaseObj( hdc );
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
    if (dc->funcs->pSetPolyFillMode)
        ret = dc->funcs->pSetPolyFillMode( dc, mode );
    else
    {
        ret = dc->polyFillMode;
        dc->polyFillMode = mode;
    }
    GDI_ReleaseObj( hdc );
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
    if (dc->funcs->pSetStretchBltMode)
        ret = dc->funcs->pSetStretchBltMode( dc, mode );
    else
    {
        ret = dc->stretchBltMode;
        dc->stretchBltMode = mode;
    }
    GDI_ReleaseObj( hdc );
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


/**** 16-bit functions ****/


/***********************************************************************
 *		SetBkMode (GDI.2)
 */
INT16 WINAPI SetBkMode16( HDC16 hdc, INT16 mode )
{
    return SetBkMode( hdc, mode );
}

/***********************************************************************
 *		SetROP2	(GDI.4)
 */
INT16 WINAPI SetROP216( HDC16 hdc, INT16 mode )
{
    return SetROP2( hdc, mode );
}

/***********************************************************************
 *		SetRelAbs (GDI.5)
 */
INT16 WINAPI SetRelAbs16( HDC16 hdc, INT16 mode )
{
    return SetRelAbs( hdc, mode );
}

/***********************************************************************
 *		SetPolyFillMode	(GDI.6)
 */
INT16 WINAPI SetPolyFillMode16( HDC16 hdc, INT16 mode )
{
    return SetPolyFillMode( hdc, mode );
}

/***********************************************************************
 *		SetStretchBltMode (GDI.7)
 */
INT16 WINAPI SetStretchBltMode16( HDC16 hdc, INT16 mode )
{
    return SetStretchBltMode( hdc, mode );
}

/***********************************************************************
 *		GetBkColor (GDI.75)
 */
COLORREF WINAPI GetBkColor16( HDC16 hdc )
{
    return GetBkColor( hdc );
}

/***********************************************************************
 *		GetBkMode (GDI.76)
 */
INT16 WINAPI GetBkMode16( HDC16 hdc )
{
    return GetBkMode( hdc );
}

/***********************************************************************
 *		GetCurrentPosition (GDI.78)
 */
DWORD WINAPI GetCurrentPosition16( HDC16 hdc )
{
    POINT pt32;
    if (!GetCurrentPositionEx( hdc, &pt32 )) return 0;
    return MAKELONG( pt32.x, pt32.y );
}

/***********************************************************************
 *		GetMapMode (GDI.81)
 */
INT16 WINAPI GetMapMode16( HDC16 hdc )
{
    return GetMapMode( hdc );
}

/***********************************************************************
 *		GetPolyFillMode (GDI.84)
 */
INT16 WINAPI GetPolyFillMode16( HDC16 hdc )
{
    return GetPolyFillMode( hdc );
}

/***********************************************************************
 *		GetROP2 (GDI.85)
 */
INT16 WINAPI GetROP216( HDC16 hdc )
{
    return GetROP2( hdc );
}

/***********************************************************************
 *		GetRelAbs (GDI.86)
 */
INT16 WINAPI GetRelAbs16( HDC16 hdc )
{
    return GetRelAbs( hdc, 0 );
}

/***********************************************************************
 *		GetStretchBltMode (GDI.88)
 */
INT16 WINAPI GetStretchBltMode16( HDC16 hdc )
{
    return GetStretchBltMode( hdc );
}

/***********************************************************************
 *		GetTextColor (GDI.90)
 */
COLORREF WINAPI GetTextColor16( HDC16 hdc )
{
    return GetTextColor( hdc );
}

/***********************************************************************
 *		GetViewportExt (GDI.94)
 */
DWORD WINAPI GetViewportExt16( HDC16 hdc )
{
    SIZE size;
    if (!GetViewportExtEx( hdc, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}

/***********************************************************************
 *		GetViewportOrg (GDI.95)
 */
DWORD WINAPI GetViewportOrg16( HDC16 hdc )
{
    POINT pt;
    if (!GetViewportOrgEx( hdc, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *		GetWindowExt (GDI.96)
 */
DWORD WINAPI GetWindowExt16( HDC16 hdc )
{
    SIZE size;
    if (!GetWindowExtEx( hdc, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}

/***********************************************************************
 *		GetWindowOrg (GDI.97)
 */
DWORD WINAPI GetWindowOrg16( HDC16 hdc )
{
    POINT pt;
    if (!GetWindowOrgEx( hdc, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}

/***********************************************************************
 *		InquireVisRgn (GDI.131)
 */
HRGN16 WINAPI InquireVisRgn16( HDC16 hdc )
{
    HRGN16 ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->hVisRgn;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *		GetBrushOrg (GDI.149)
 */
DWORD WINAPI GetBrushOrg16( HDC16 hdc )
{
    POINT pt;
    if (!GetBrushOrgEx( hdc, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}

/***********************************************************************
 *		GetClipRgn (GDI.173)
 */
HRGN16 WINAPI GetClipRgn16( HDC16 hdc )
{
    HRGN16 ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->hClipRgn;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *		GetTextAlign (GDI.345)
 */
UINT16 WINAPI GetTextAlign16( HDC16 hdc )
{
    return GetTextAlign( hdc );
}

/***********************************************************************
 *		GetCurLogFont (GDI.411)
 */
HFONT16 WINAPI GetCurLogFont16( HDC16 hdc )
{
    HFONT16 ret = 0;
    DC * dc = DC_GetDCPtr( hdc );
    if (dc)
    {
        ret = dc->hFont;
        GDI_ReleaseObj( hdc );
    }
    return ret;
}

/***********************************************************************
 *		GetBrushOrgEx (GDI.469)
 */
BOOL16 WINAPI GetBrushOrgEx16( HDC16 hdc, LPPOINT16 pt )
{
    POINT pt32;
    if (!GetBrushOrgEx( hdc, &pt32 )) return FALSE;
    pt->x = pt32.x;
    pt->y = pt32.y;
    return TRUE;
}

/***********************************************************************
 *		GetCurrentPositionEx (GDI.470)
 */
BOOL16 WINAPI GetCurrentPositionEx16( HDC16 hdc, LPPOINT16 pt )
{
    POINT pt32;
    if (!GetCurrentPositionEx( hdc, &pt32 )) return FALSE;
    pt->x = pt32.x;
    pt->y = pt32.y;
    return TRUE;
}

/***********************************************************************
 *		GetViewportExtEx (GDI.472)
 */
BOOL16 WINAPI GetViewportExtEx16( HDC16 hdc, LPSIZE16 size )
{
    SIZE size32;
    if (!GetViewportExtEx( hdc, &size32 )) return FALSE;
    size->cx = size32.cx;
    size->cy = size32.cy;
    return TRUE;
}

/***********************************************************************
 *		GetViewportOrgEx (GDI.473)
 */
BOOL16 WINAPI GetViewportOrgEx16( HDC16 hdc, LPPOINT16 pt )
{
    POINT pt32;
    if (!GetViewportOrgEx( hdc, &pt32 )) return FALSE;
    pt->x = pt32.x;
    pt->y = pt32.y;
    return TRUE;
}

/***********************************************************************
 *		GetWindowExtEx (GDI.474)
 */
BOOL16 WINAPI GetWindowExtEx16( HDC16 hdc, LPSIZE16 size )
{
    SIZE size32;
    if (!GetWindowExtEx( hdc, &size32 )) return FALSE;
    size->cx = size32.cx;
    size->cy = size32.cy;
    return TRUE;
}

/***********************************************************************
 *		GetWindowOrgEx (GDI.475)
 */
BOOL16 WINAPI GetWindowOrgEx16( HDC16 hdc, LPPOINT16 pt )
{
    POINT pt32;
    if (!GetWindowOrgEx( hdc, &pt32 )) return FALSE;
    pt->x = pt32.x;
    pt->y = pt32.y;
    return TRUE;
}

/***********************************************************************
 *		GetArcDirection (GDI.524)
 */
INT16 WINAPI GetArcDirection16( HDC16 hdc )
{
    return GetArcDirection( hdc );
}
