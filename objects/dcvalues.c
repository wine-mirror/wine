/*
 * DC device-independent Get/SetXXX functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include "gdi.h"
#include "metafile.h"


#define DC_GET_VAL_16( func_type, func_name, dc_field ) \
func_type func_name( HDC16 hdc ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    return dc->dc_field; \
}

#define DC_GET_VAL_32( func_type, func_name, dc_field ) \
func_type func_name( HDC32 hdc ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    return dc->dc_field; \
}

#define DC_GET_X_Y( func_type, func_name, ret_x, ret_y ) \
func_type func_name( HDC16 hdc ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    return MAKELONG( dc->ret_x, dc->ret_y ); \
}

#define DC_GET_VAL_EX( func_name, ret_x, ret_y ) \
BOOL16 func_name##16( HDC16 hdc, LPPOINT16 pt ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return FALSE; \
    pt->x = dc->ret_x; \
    pt->y = dc->ret_y; \
    return TRUE; \
} \
 \
BOOL32 func_name##32( HDC32 hdc, LPPOINT32 pt ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( (HDC16)hdc, DC_MAGIC ); \
    if (!dc) return FALSE; \
    pt->x = dc->ret_x; \
    pt->y = dc->ret_y; \
    return TRUE; \
}

#define DC_SET_MODE_16( func_name, dc_field, min_val, max_val, meta_func ) \
INT16 func_name( HDC16 hdc, INT16 mode ) \
{ \
    INT16 prevMode; \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if ((mode < min_val) || (mode > max_val)) return 0; \
    if (!dc) { \
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC); \
	if (!dc) return 0; \
	MF_MetaParam1(dc, meta_func, mode); \
	return 1; \
    } \
    prevMode = dc->dc_field; \
    dc->dc_field = mode; \
    return prevMode; \
}

#define DC_SET_MODE_32( func_name, dc_field, min_val, max_val, meta_func ) \
INT32 func_name( HDC32 hdc, INT32 mode ) \
{ \
    INT32 prevMode; \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if ((mode < min_val) || (mode > max_val)) return 0; \
    if (!dc) { \
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC); \
	if (!dc) return 0; \
	MF_MetaParam1(dc, meta_func, mode); \
	return 1; \
    } \
    prevMode = dc->dc_field; \
    dc->dc_field = mode; \
    return prevMode; \
}


DC_SET_MODE_16( SetBkMode16, w.backgroundMode, TRANSPARENT,     /* GDI.2     */
                OPAQUE, META_SETBKMODE )
DC_SET_MODE_32( SetBkMode32, w.backgroundMode, TRANSPARENT,     /* GDI32.306 */
                OPAQUE, META_SETBKMODE )
DC_SET_MODE_16( SetROP216, w.ROPmode, R2_BLACK, R2_WHITE,       /* GDI.4     */
                META_SETROP2 )
DC_SET_MODE_32( SetROP232, w.ROPmode, R2_BLACK, R2_WHITE,       /* GDI32.331 */
                META_SETROP2 )
DC_SET_MODE_16( SetRelAbs16, w.relAbsMode, ABSOLUTE, RELATIVE,  /* GDI.5     */
                META_SETRELABS )
DC_SET_MODE_32( SetRelAbs32, w.relAbsMode, ABSOLUTE, RELATIVE,  /* GDI32.333 */
                META_SETRELABS )
DC_SET_MODE_16( SetPolyFillMode16, w.polyFillMode,              /* GDI.6     */
                ALTERNATE, WINDING, META_SETPOLYFILLMODE )
DC_SET_MODE_32( SetPolyFillMode32, w.polyFillMode,              /* GDI32.330 */
                ALTERNATE, WINDING, META_SETPOLYFILLMODE )
DC_SET_MODE_16( SetStretchBltMode16, w.stretchBltMode,          /* GDI.7     */
                BLACKONWHITE, COLORONCOLOR, META_SETSTRETCHBLTMODE )
DC_SET_MODE_32( SetStretchBltMode32, w.stretchBltMode,          /* GDI32.334 */
                BLACKONWHITE, COLORONCOLOR, META_SETSTRETCHBLTMODE )
DC_GET_VAL_16( COLORREF, GetBkColor16, w.backgroundColor )      /* GDI.75    */
DC_GET_VAL_32( COLORREF, GetBkColor32, w.backgroundColor )      /* GDI32.145 */
DC_GET_VAL_16( INT16, GetBkMode16, w.backgroundMode )           /* GDI.76    */
DC_GET_VAL_32( INT32, GetBkMode32, w.backgroundMode )           /* GDI32.146 */
DC_GET_X_Y( DWORD, GetCurrentPosition, w.CursPosX, w.CursPosY ) /* GDI.78    */
DC_GET_VAL_16( INT16, GetMapMode16, w.MapMode )                 /* GDI.81    */
DC_GET_VAL_32( INT32, GetMapMode32, w.MapMode )                 /* GDI32.196 */
DC_GET_VAL_16( INT16, GetPolyFillMode16, w.polyFillMode )       /* GDI.84    */
DC_GET_VAL_32( INT32, GetPolyFillMode32, w.polyFillMode )       /* GDI32.213 */
DC_GET_VAL_16( INT16, GetROP216, w.ROPmode )                    /* GDI.85    */
DC_GET_VAL_32( INT32, GetROP232, w.ROPmode )                    /* GDI32.214 */
DC_GET_VAL_16( INT16, GetRelAbs16, w.relAbsMode )               /* GDI.86    */
DC_GET_VAL_32( INT32, GetRelAbs32, w.relAbsMode )               /* GDI32.218 */
DC_GET_VAL_16( INT16, GetStretchBltMode16, w.stretchBltMode )   /* GDI.88    */
DC_GET_VAL_32( INT32, GetStretchBltMode32, w.stretchBltMode )   /* GDI32.221 */
DC_GET_VAL_16( COLORREF, GetTextColor16, w.textColor )          /* GDI.90    */
DC_GET_VAL_32( COLORREF, GetTextColor32, w.textColor )          /* GDI32.227 */
DC_GET_X_Y( DWORD, GetViewportExt, vportExtX, vportExtY )       /* GDI.94    */
DC_GET_X_Y( DWORD, GetViewportOrg, vportOrgX, vportOrgY )       /* GDI.95    */
DC_GET_X_Y( DWORD, GetWindowExt, wndExtX, wndExtY )             /* GDI.96    */
DC_GET_X_Y( DWORD, GetWindowOrg, wndOrgX, wndOrgY )             /* GDI.97    */
DC_GET_VAL_16( HRGN16, InquireVisRgn, w.hVisRgn )               /* GDI.131   */
DC_GET_X_Y( DWORD, GetBrushOrg, w.brushOrgX, w.brushOrgY )      /* GDI.149   */
DC_GET_VAL_16( UINT16, GetTextAlign16, w.textAlign )            /* GDI.345   */
DC_GET_VAL_32( UINT32, GetTextAlign32, w.textAlign )            /* GDI32.224 */
DC_GET_VAL_16( HFONT16, GetCurLogFont, w.hFont )                /* GDI.411   */
DC_GET_VAL_EX( GetBrushOrgEx, w.brushOrgX, w.brushOrgY )/* GDI.469 GDI32.148 */
DC_GET_VAL_EX( GetCurrentPositionEx, w.CursPosX,        /* GDI.470 GDI32.167 */
               w.CursPosY )
DC_GET_VAL_EX( GetViewportExtEx, vportExtX, vportExtY ) /* GDI.472 GDI32.239 */
DC_GET_VAL_EX( GetViewportOrgEx, vportOrgX, vportOrgY ) /* GDI.473 GDI32.240 */
DC_GET_VAL_EX( GetWindowExtEx, wndExtX, wndExtY )       /* GDI.474 GDI32.242 */
DC_GET_VAL_EX( GetWindowOrgEx, wndOrgX, wndOrgY )       /* GDI.475 GDI32.243 */

/* this one is wrong - Windows returns region that
   is relative to the device and not to the DC origin */

DC_GET_VAL_16( HRGN16, GetClipRgn16, w.hClipRgn )              /* GDI.173    */

