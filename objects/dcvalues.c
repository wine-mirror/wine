/*
 * DC device-independent Get/SetXXX functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wownt32.h"

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
        ret = dc->funcs->pSetBkMode( dc->physDev, mode );
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
        ret = dc->funcs->pSetROP2( dc->physDev, mode );
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
        ret = dc->funcs->pSetPolyFillMode( dc->physDev, mode );
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
        ret = dc->funcs->pSetStretchBltMode( dc->physDev, mode );
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


/**** 16-bit functions ***/

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
