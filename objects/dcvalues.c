/*
 * DC device-independent Get/SetXXX functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include "gdi.h"
#include "dc.h"

#define DC_GET_VAL_16( func_type, func_name, dc_field ) \
func_type WINAPI func_name( HDC16 hdc ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    return dc->dc_field; \
}

#define DC_GET_VAL_32( func_type, func_name, dc_field ) \
func_type WINAPI func_name( HDC hdc ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    return dc->dc_field; \
}

#define DC_GET_X_Y( func_type, func_name, ret_x, ret_y ) \
func_type WINAPI func_name( HDC16 hdc ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    return MAKELONG( dc->ret_x, dc->ret_y ); \
}

/* DC_GET_VAL_EX is used to define functions returning a POINT or a SIZE. It is 
 * important that the function has the right signature, for the implementation 
 * we can do whatever we want.
 */
#define DC_GET_VAL_EX( func_name, ret_x, ret_y, type ) \
BOOL16 WINAPI func_name##16( HDC16 hdc, LP##type##16 pt ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return FALSE; \
    ((LPPOINT16)pt)->x = dc->ret_x; \
    ((LPPOINT16)pt)->y = dc->ret_y; \
    return TRUE; \
} \
 \
BOOL WINAPI func_name( HDC hdc, LP##type pt ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( (HDC16)hdc, DC_MAGIC ); \
    if (!dc) return FALSE; \
    ((LPPOINT)pt)->x = dc->ret_x; \
    ((LPPOINT)pt)->y = dc->ret_y; \
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
    DC *dc = DC_GetDCPtr( hdc ); \
    if(!dc) return 0; \
    if ((mode < min_val) || (mode > max_val)) return 0; \
    if (dc->funcs->p##func_name) { \
	prevMode = dc->funcs->p##func_name( dc, mode ); \
    } else { \
        prevMode = dc->dc_field; \
        dc->dc_field = mode; \
    } \
    GDI_HEAP_UNLOCK( hdc ); \
    return prevMode; \
}



/* GDI.2 GDI32.306 */
DC_SET_MODE( SetBkMode, w.backgroundMode, TRANSPARENT, OPAQUE ) 

/* GDI.4 GDI32.331 */
DC_SET_MODE( SetROP2, w.ROPmode, R2_BLACK, R2_WHITE )

/* GDI.5 GDI32.333 */
DC_SET_MODE( SetRelAbs, w.relAbsMode, ABSOLUTE, RELATIVE )

/* GDI.6 GDI32.330 */
DC_SET_MODE( SetPolyFillMode, w.polyFillMode, ALTERNATE, WINDING )

/* GDI.7 GDI32.334 */
DC_SET_MODE( SetStretchBltMode, w.stretchBltMode, BLACKONWHITE, HALFTONE )

DC_GET_VAL_16( COLORREF, GetBkColor16, w.backgroundColor )      /* GDI.75    */
DC_GET_VAL_32( COLORREF, GetBkColor, w.backgroundColor )      /* GDI32.145 */
DC_GET_VAL_16( INT16, GetBkMode16, w.backgroundMode )           /* GDI.76    */
DC_GET_VAL_32( INT, GetBkMode, w.backgroundMode )           /* GDI32.146 */
DC_GET_X_Y( DWORD, GetCurrentPosition16, w.CursPosX, w.CursPosY ) /* GDI.78    */
DC_GET_VAL_16( INT16, GetMapMode16, w.MapMode )                 /* GDI.81    */
DC_GET_VAL_32( INT, GetMapMode, w.MapMode )                 /* GDI32.196 */
DC_GET_VAL_16( INT16, GetPolyFillMode16, w.polyFillMode )       /* GDI.84    */
DC_GET_VAL_32( INT, GetPolyFillMode, w.polyFillMode )       /* GDI32.213 */
DC_GET_VAL_16( INT16, GetROP216, w.ROPmode )                    /* GDI.85    */
DC_GET_VAL_32( INT, GetROP2, w.ROPmode )                    /* GDI32.214 */
DC_GET_VAL_16( INT16, GetRelAbs16, w.relAbsMode )               /* GDI.86    */
DC_GET_VAL_32( INT, GetRelAbs, w.relAbsMode )               /* GDI32.218 */
DC_GET_VAL_16( INT16, GetStretchBltMode16, w.stretchBltMode )   /* GDI.88    */
DC_GET_VAL_32( INT, GetStretchBltMode, w.stretchBltMode )   /* GDI32.221 */
DC_GET_VAL_16( COLORREF, GetTextColor16, w.textColor )          /* GDI.90    */
DC_GET_VAL_32( COLORREF, GetTextColor, w.textColor )          /* GDI32.227 */
DC_GET_X_Y( DWORD, GetViewportExt16, vportExtX, vportExtY )       /* GDI.94    */
DC_GET_X_Y( DWORD, GetViewportOrg16, vportOrgX, vportOrgY )       /* GDI.95    */
DC_GET_X_Y( DWORD, GetWindowExt16, wndExtX, wndExtY )             /* GDI.96    */
DC_GET_X_Y( DWORD, GetWindowOrg16, wndOrgX, wndOrgY )             /* GDI.97    */
DC_GET_VAL_16( HRGN16, InquireVisRgn16, w.hVisRgn )               /* GDI.131   */
DC_GET_VAL_16( HRGN16, GetClipRgn16, w.hClipRgn )               /* GDI.173   */
DC_GET_X_Y( DWORD, GetBrushOrg16, w.brushOrgX, w.brushOrgY )      /* GDI.149   */
DC_GET_VAL_16( UINT16, GetTextAlign16, w.textAlign )            /* GDI.345   */
DC_GET_VAL_32( UINT, GetTextAlign, w.textAlign )            /* GDI32.224 */
DC_GET_VAL_16( HFONT16, GetCurLogFont16, w.hFont )                /* GDI.411   */
DC_GET_VAL_EX( GetBrushOrgEx, w.brushOrgX, w.brushOrgY, POINT ) /* GDI.469 GDI32.148 */
DC_GET_VAL_EX( GetCurrentPositionEx, w.CursPosX, w.CursPosY,    /* GDI.470 GDI32.167 */
               POINT )
DC_GET_VAL_EX( GetViewportExtEx, vportExtX, vportExtY, SIZE )   /* GDI.472 GDI32.239 */
DC_GET_VAL_EX( GetViewportOrgEx, vportOrgX, vportOrgY, POINT )  /* GDI.473 GDI32.240 */
DC_GET_VAL_EX( GetWindowExtEx, wndExtX, wndExtY, SIZE )         /* GDI.474 GDI32.242 */
DC_GET_VAL_EX( GetWindowOrgEx, wndOrgX, wndOrgY, POINT )        /* GDI.475 GDI32.243 */
