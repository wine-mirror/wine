/*
 * DC device-independent Get/SetXXX functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include "winbase.h"
#include "winerror.h"
#include "gdi.h"

#define COLORREF16 COLORREF  /*hack*/

#define DC_GET_VAL_16( func_type, func_name, dc_field ) \
func_type WINAPI func_name( HDC16 hdc ) \
{ \
    func_type ret = 0; \
    DC * dc = DC_GetDCPtr( hdc ); \
    if (dc) \
    { \
        ret = dc->dc_field; \
        GDI_ReleaseObj( hdc ); \
    } \
    return ret; \
}

#define DC_GET_VAL( func_type, func_name, dc_field ) \
func_type##16 WINAPI func_name##16( HDC16 hdc ) \
{ \
    return func_name( hdc ); \
} \
 \
func_type WINAPI func_name( HDC hdc ) \
{ \
    func_type ret = 0; \
    DC * dc = DC_GetDCPtr( hdc ); \
    if (dc) \
    { \
        ret = dc->dc_field; \
        GDI_ReleaseObj( hdc ); \
    } \
    return ret; \
}

#define DC_GET_X_Y( func_type, func_name, ret_x, ret_y ) \
func_type WINAPI func_name( HDC16 hdc ) \
{ \
    func_type ret = 0; \
    DC * dc = DC_GetDCPtr( hdc ); \
    if (dc) \
    { \
        ret = MAKELONG( dc->ret_x, dc->ret_y ); \
        GDI_ReleaseObj( hdc ); \
    } \
    return ret; \
}

/* DC_GET_VAL_EX is used to define functions returning a POINT or a SIZE. It is 
 * important that the function has the right signature, for the implementation 
 * we can do whatever we want.
 */
#define DC_GET_VAL_EX( func_name, ret_x, ret_y, type ) \
BOOL16 WINAPI func_name##16( HDC16 hdc, LP##type##16 pt ) \
{ \
    DC * dc = DC_GetDCPtr( hdc ); \
    if (!dc) return FALSE; \
    ((LPPOINT16)pt)->x = dc->ret_x; \
    ((LPPOINT16)pt)->y = dc->ret_y; \
    GDI_ReleaseObj( hdc ); \
    return TRUE; \
} \
 \
BOOL WINAPI func_name( HDC hdc, LP##type pt ) \
{ \
    DC * dc = DC_GetDCPtr( hdc ); \
    if (!dc) return FALSE; \
    ((LPPOINT)pt)->x = dc->ret_x; \
    ((LPPOINT)pt)->y = dc->ret_y; \
    GDI_ReleaseObj( hdc ); \
    return TRUE; \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT16 WINAPI func_name##16( HDC16 hdc, INT16 mode ) \
{ \
    return func_name( hdc, mode ); \
} \
 \
INT WINAPI func_name( HDC hdc, INT mode ) \
{ \
    INT prevMode; \
    DC *dc; \
    if ((mode < min_val) || (mode > max_val)) { \
        SetLastError(ERROR_INVALID_PARAMETER); \
        return 0; \
    } \
    if (!(dc = DC_GetDCPtr( hdc ))) return 0; \
    if (dc->funcs->p##func_name) { \
	prevMode = dc->funcs->p##func_name( dc, mode ); \
    } else { \
        prevMode = dc->dc_field; \
        dc->dc_field = mode; \
    } \
    GDI_ReleaseObj( hdc ); \
    return prevMode; \
}

/***********************************************************************
 *		SetBkMode		(GDI.2) (GDI32.@)
 *
 */
DC_SET_MODE( SetBkMode, backgroundMode, TRANSPARENT, OPAQUE ) 

/***********************************************************************
 *		SetROP2			(GDI.4) (GDI32.@)
 */
DC_SET_MODE( SetROP2, ROPmode, R2_BLACK, R2_WHITE )

/***********************************************************************
 *		SetRelAbs		(GDI.5) (GDI32.@)
 */
DC_SET_MODE( SetRelAbs, relAbsMode, ABSOLUTE, RELATIVE )

/***********************************************************************
 *		SetPolyFillMode		(GDI.6) (GDI32.@)
 */
DC_SET_MODE( SetPolyFillMode, polyFillMode, ALTERNATE, WINDING )

/***********************************************************************
 *		SetStretchBltMode	(GDI.7) (GDI32.@)
 */
DC_SET_MODE( SetStretchBltMode, stretchBltMode, BLACKONWHITE, HALFTONE )

/***********************************************************************
 *		GetBkColor		(GDI.75) (GDI32.@)
 */
DC_GET_VAL( COLORREF, GetBkColor, backgroundColor )

/***********************************************************************
 *		GetBkMode		(GDI.76) (GDI32.@)
 */
DC_GET_VAL( INT, GetBkMode, backgroundMode )

/***********************************************************************
 *		GetCurrentPosition16	(GDI.78)
 */
DC_GET_X_Y( DWORD, GetCurrentPosition16, CursPosX, CursPosY )

/***********************************************************************
 *		GetMapMode		(GDI.81) (GDI32.@)
 */
DC_GET_VAL( INT, GetMapMode, MapMode )

/***********************************************************************
 *		GetPolyFillMode		(GDI.84) (GDI32.@)
 */
DC_GET_VAL( INT, GetPolyFillMode, polyFillMode )

/***********************************************************************
 *		GetROP2			(GDI.85) (GDI32.@)
 */
DC_GET_VAL( INT, GetROP2, ROPmode )

/***********************************************************************
 *		GetRelAbs16		(GDI.86)
 */
DC_GET_VAL_16( INT16, GetRelAbs16, relAbsMode )

/***********************************************************************
 *		GetStretchBltMode	(GDI.88) (GDI32.@)
 */
DC_GET_VAL( INT, GetStretchBltMode, stretchBltMode )

/***********************************************************************
 *		GetTextColor		(GDI.90) (GDI32.@)
 */
DC_GET_VAL( COLORREF, GetTextColor, textColor )

/***********************************************************************
 *		GetViewportExt16	(GDI.94)
 */
DC_GET_X_Y( DWORD, GetViewportExt16, vportExtX, vportExtY )

/***********************************************************************
 *		GetViewportOrg16	(GDI.95)
 */
DC_GET_X_Y( DWORD, GetViewportOrg16, vportOrgX, vportOrgY )

/***********************************************************************
 *		GetWindowExt16		(GDI.96)
 */
DC_GET_X_Y( DWORD, GetWindowExt16, wndExtX, wndExtY )

/***********************************************************************
 *		GetWindowOrg16	(GDI.97)
 */
DC_GET_X_Y( DWORD, GetWindowOrg16, wndOrgX, wndOrgY )

/***********************************************************************
 *		InquireVisRgn16	(GDI.131)
 */
DC_GET_VAL_16( HRGN16, InquireVisRgn16, hVisRgn )

/***********************************************************************
 *		GetClipRgn16	(GDI.173)
 */
DC_GET_VAL_16( HRGN16, GetClipRgn16, hClipRgn )

/***********************************************************************
 *		GetBrushOrg16	(GDI.149)
 */
DC_GET_X_Y( DWORD, GetBrushOrg16, brushOrgX, brushOrgY )

/***********************************************************************
 *		GetTextAlign	(GDI.345) (GDI32.@)
 */
DC_GET_VAL( UINT, GetTextAlign, textAlign )

/***********************************************************************
 *		GetCurLogFont16	(GDI.411)
 */
DC_GET_VAL_16( HFONT16, GetCurLogFont16, hFont )

/***********************************************************************
 *		GetArcDirection	(GDI.524) (GDI32.@)
 */
DC_GET_VAL( INT, GetArcDirection, ArcDirection )

/***********************************************************************
 *		GetGraphicsMode (GDI32.@)
 */
DC_GET_VAL( INT, GetGraphicsMode, GraphicsMode)

/***********************************************************************
 *		GetBrushOrgEx	(GDI.469) (GDI32.@)
 */
DC_GET_VAL_EX( GetBrushOrgEx, brushOrgX, brushOrgY, POINT ) /*  */

/***********************************************************************
 *		GetCurrentPositionEx	(GDI.470) (GDI32.@)
 */
DC_GET_VAL_EX( GetCurrentPositionEx, CursPosX, CursPosY, POINT )

/***********************************************************************
 *		GetViewportExtEx	(GDI.472 GDI32.@)
 */
DC_GET_VAL_EX( GetViewportExtEx, vportExtX, vportExtY, SIZE )

/***********************************************************************
 *		GetViewportOrgEx	(GDI.473) (GDI32.@)
 */
DC_GET_VAL_EX( GetViewportOrgEx, vportOrgX, vportOrgY, POINT )

/***********************************************************************
 *		GetWindowExtEx	(GDI.474) (GDI32.@)
 */
DC_GET_VAL_EX( GetWindowExtEx, wndExtX, wndExtY, SIZE )

/***********************************************************************
 *		GetWindowOrgEx	(GDI.475) (GDI32.@)
 */
DC_GET_VAL_EX( GetWindowOrgEx, wndOrgX, wndOrgY, POINT )
