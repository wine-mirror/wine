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

/* Define pseudo types */
#define COLORREF16 COLORREF
#define DWORD16    DWORD

#define DC_GET_VAL_16( func_type, func_name, dc_field ) \
func_type##16 WINAPI func_name##16( HDC16 hdc ) \
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

#define DC_GET_VAL_32( func_type, func_name, dc_field ) \
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

#define DC_GET_X_Y_16( func_type, func_name, ret_x, ret_y ) \
func_type##16 WINAPI func_name##16( HDC16 hdc ) \
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
#define DC_GET_VAL_EX_16( func_name, ret_x, ret_y, type ) \
BOOL16 WINAPI func_name##16( HDC16 hdc, LP##type##16 pt ) \
{ \
    DC * dc = DC_GetDCPtr( hdc ); \
    if (!dc) return FALSE; \
    ((LPPOINT16)pt)->x = dc->ret_x; \
    ((LPPOINT16)pt)->y = dc->ret_y; \
    GDI_ReleaseObj( hdc ); \
    return TRUE; \
}

#define DC_GET_VAL_EX_32( func_name, ret_x, ret_y, type ) \
BOOL WINAPI func_name( HDC hdc, LP##type pt ) \
{ \
    DC * dc = DC_GetDCPtr( hdc ); \
    if (!dc) return FALSE; \
    ((LPPOINT)pt)->x = dc->ret_x; \
    ((LPPOINT)pt)->y = dc->ret_y; \
    GDI_ReleaseObj( hdc ); \
    return TRUE; \
}

#define DC_SET_MODE_16( func_name, dc_field, min_val, max_val ) \
INT16 WINAPI func_name##16( HDC16 hdc, INT16 mode ) \
{ \
    return func_name( hdc, mode ); \
} \

#define DC_SET_MODE_32( func_name, dc_field, min_val, max_val ) \
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
 *		SetBkMode (GDI.2)
 */
DC_SET_MODE_16( SetBkMode, backgroundMode, TRANSPARENT, OPAQUE ) 

/***********************************************************************
 *		SetBkMode (GDI32.@)
 */
DC_SET_MODE_32( SetBkMode, backgroundMode, TRANSPARENT, OPAQUE ) 

/***********************************************************************
 *		SetROP2	(GDI.4)
 */
DC_SET_MODE_16( SetROP2, ROPmode, R2_BLACK, R2_WHITE )

/***********************************************************************
 *		SetROP2 (GDI32.@)
 */
DC_SET_MODE_32( SetROP2, ROPmode, R2_BLACK, R2_WHITE )

/***********************************************************************
 *		SetRelAbs (GDI.5)
 */
DC_SET_MODE_16( SetRelAbs, relAbsMode, ABSOLUTE, RELATIVE )

/***********************************************************************
 *		SetRelAbs (GDI32.@)
 */
DC_SET_MODE_32( SetRelAbs, relAbsMode, ABSOLUTE, RELATIVE )

/***********************************************************************
 *		SetPolyFillMode	(GDI.6)
 */
DC_SET_MODE_16( SetPolyFillMode, polyFillMode, ALTERNATE, WINDING )

/***********************************************************************
 *		SetPolyFillMode (GDI32.@)
 */
DC_SET_MODE_32( SetPolyFillMode, polyFillMode, ALTERNATE, WINDING )

/***********************************************************************
 *		SetStretchBltMode (GDI.7)
 */
DC_SET_MODE_16( SetStretchBltMode, stretchBltMode, BLACKONWHITE, HALFTONE )

/***********************************************************************
 *		SetStretchBltMode (GDI32.@)
 */
DC_SET_MODE_32( SetStretchBltMode, stretchBltMode, BLACKONWHITE, HALFTONE )

/***********************************************************************
 *		GetBkColor (GDI.75)
 */
DC_GET_VAL_16( COLORREF, GetBkColor, backgroundColor )

/***********************************************************************
 *		GetBkColor (GDI32.@)
 */
DC_GET_VAL_32( COLORREF, GetBkColor, backgroundColor )

/***********************************************************************
 *		GetBkMode (GDI.76)
 */
DC_GET_VAL_16( INT, GetBkMode, backgroundMode )

/***********************************************************************
 *		GetBkMode (GDI32.@)
 */
DC_GET_VAL_32( INT, GetBkMode, backgroundMode )

/***********************************************************************
 *		GetCurrentPosition (GDI.78)
 */
DC_GET_X_Y_16( DWORD, GetCurrentPosition, CursPosX, CursPosY )

/***********************************************************************
 *		GetMapMode (GDI.81)
 */
DC_GET_VAL_16( INT, GetMapMode, MapMode )

/***********************************************************************
 *		GetMapMode (GDI32.@)
 */
DC_GET_VAL_32( INT, GetMapMode, MapMode )

/***********************************************************************
 *		GetPolyFillMode (GDI.84)
 */
DC_GET_VAL_16( INT, GetPolyFillMode, polyFillMode )

/***********************************************************************
 *		GetPolyFillMode (GDI32.@)
 */
DC_GET_VAL_32( INT, GetPolyFillMode, polyFillMode )

/***********************************************************************
 *		GetROP2 (GDI.85)
 */
DC_GET_VAL_16( INT, GetROP2, ROPmode )

/***********************************************************************
 *		GetROP2 (GDI32.@)
 */
DC_GET_VAL_32( INT, GetROP2, ROPmode )

/***********************************************************************
 *		GetRelAbs  (GDI.86)
 */
DC_GET_VAL_16( INT, GetRelAbs, relAbsMode )

/***********************************************************************
 *		GetStretchBltMode (GDI.88)
 */

DC_GET_VAL_16( INT, GetStretchBltMode, stretchBltMode )

/***********************************************************************
 *		GetStretchBltMode (GDI32.@)
 */
DC_GET_VAL_32( INT, GetStretchBltMode, stretchBltMode )

/***********************************************************************
 *		GetTextColor (GDI.90)
 */
DC_GET_VAL_16( COLORREF, GetTextColor, textColor )

/***********************************************************************
 *		GetTextColor (GDI32.@)
 */
DC_GET_VAL_32( COLORREF, GetTextColor, textColor )

/***********************************************************************
 *		GetViewportExt (GDI.94)
 */
DC_GET_X_Y_16( DWORD, GetViewportExt, vportExtX, vportExtY )

/***********************************************************************
 *		GetViewportOrg (GDI.95)
 */
DC_GET_X_Y_16( DWORD, GetViewportOrg, vportOrgX, vportOrgY )

/***********************************************************************
 *		GetWindowExt (GDI.96)
 */
DC_GET_X_Y_16( DWORD, GetWindowExt, wndExtX, wndExtY )

/***********************************************************************
 *		GetWindowOrg (GDI.97)
 */
DC_GET_X_Y_16( DWORD, GetWindowOrg, wndOrgX, wndOrgY )

/***********************************************************************
 *		InquireVisRgn (GDI.131)
 */
DC_GET_VAL_16( HRGN, InquireVisRgn, hVisRgn )

/***********************************************************************
 *		GetClipRgn (GDI.173)
 */
DC_GET_VAL_16( HRGN, GetClipRgn, hClipRgn )

/***********************************************************************
 *		GetBrushOrg (GDI.149)
 */
DC_GET_X_Y_16( DWORD, GetBrushOrg, brushOrgX, brushOrgY )

/***********************************************************************
 *		GetTextAlign (GDI.345)
 */
DC_GET_VAL_16( UINT, GetTextAlign, textAlign )

/***********************************************************************
 *		GetTextAlign (GDI32.@)
 */
DC_GET_VAL_32( UINT, GetTextAlign, textAlign )

/***********************************************************************
 *		GetCurLogFont (GDI.411)
 */
DC_GET_VAL_16( HFONT, GetCurLogFont, hFont )

/***********************************************************************
 *		GetArcDirection (GDI.524)
 */
DC_GET_VAL_16( INT, GetArcDirection, ArcDirection )

/***********************************************************************
 *		GetArcDirection (GDI32.@)
 */
DC_GET_VAL_32( INT, GetArcDirection, ArcDirection )

/***********************************************************************
 *		GetGraphicsMode (GDI32.@)
 */
DC_GET_VAL_16( INT, GetGraphicsMode, GraphicsMode)

/***********************************************************************
 *		GetGraphicsMode (GDI32.@)
 */
DC_GET_VAL_32( INT, GetGraphicsMode, GraphicsMode)

/***********************************************************************
 *		GetBrushOrgEx (GDI.469)
 */
DC_GET_VAL_EX_16( GetBrushOrgEx, brushOrgX, brushOrgY, POINT ) /*  */

/***********************************************************************
 *		GetBrushOrgEx (GDI32.@)
 */
DC_GET_VAL_EX_32( GetBrushOrgEx, brushOrgX, brushOrgY, POINT ) /*  */

/***********************************************************************
 *		GetCurrentPositionEx (GDI.470)
 */
DC_GET_VAL_EX_16( GetCurrentPositionEx, CursPosX, CursPosY, POINT )

/***********************************************************************
 *		GetCurrentPositionEx (GDI32.@)
 */
DC_GET_VAL_EX_32( GetCurrentPositionEx, CursPosX, CursPosY, POINT )

/***********************************************************************
 *		GetViewportExtEx (GDI.472)
 */
DC_GET_VAL_EX_16( GetViewportExtEx, vportExtX, vportExtY, SIZE )

/***********************************************************************
 *		GetViewportExtEx (GDI32.@)
 */
DC_GET_VAL_EX_32( GetViewportExtEx, vportExtX, vportExtY, SIZE )

/***********************************************************************
 *		GetViewportOrgEx (GDI.473)
 */
DC_GET_VAL_EX_16( GetViewportOrgEx, vportOrgX, vportOrgY, POINT )

/***********************************************************************
 *		GetViewportOrgEx (GDI32.@)
 */
DC_GET_VAL_EX_32( GetViewportOrgEx, vportOrgX, vportOrgY, POINT )

/***********************************************************************
 *		GetWindowExtEx (GDI.474)
 */
DC_GET_VAL_EX_16( GetWindowExtEx, wndExtX, wndExtY, SIZE )

/***********************************************************************
 *		GetWindowExtEx (GDI32.@)
 */
DC_GET_VAL_EX_32( GetWindowExtEx, wndExtX, wndExtY, SIZE )

/***********************************************************************
 *		GetWindowOrgEx (GDI.475)
 */
DC_GET_VAL_EX_16( GetWindowOrgEx, wndOrgX, wndOrgY, POINT )

/***********************************************************************
 *		GetWindowOrgEx (GDI32.@)
 */
DC_GET_VAL_EX_32( GetWindowOrgEx, wndOrgX, wndOrgY, POINT )
