/*
 * DC device-independent Get/SetXXX functions
 *
 * Copyright 1993 Alexandre Julliard
 *
 */

#include "gdi.h"
#include "metafile.h"


#define DC_GET_VAL( func_type, func_name, dc_field ) \
func_type func_name( HDC hdc ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    return dc->w.dc_field; \
}

#define DC_GET_X_Y( func_type, func_name, ret_x, ret_y ) \
func_type func_name( HDC hdc ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    return MAKELONG( dc->w.ret_x, dc->w.ret_y ); \
}

#define DC_GET_VAL_EX( func_name, ret_x, ret_y ) \
BOOL16 func_name##16( HDC16 hdc, LPPOINT16 pt ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return FALSE; \
    pt->x = dc->w.ret_x; \
    pt->y = dc->w.ret_y; \
    return TRUE; \
} \
 \
BOOL32 func_name##32( HDC32 hdc, LPPOINT32 pt ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( (HDC16)hdc, DC_MAGIC ); \
    if (!dc) return FALSE; \
    pt->x = dc->w.ret_x; \
    pt->y = dc->w.ret_y; \
    return TRUE; \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val, meta_func ) \
WORD func_name( HDC hdc, WORD mode ) \
{ \
    WORD prevMode; \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if ((mode < min_val) || (mode > max_val)) return 0; \
    if (!dc) { \
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC); \
	if (!dc) return 0; \
	MF_MetaParam1(dc, meta_func, mode); \
	return 1; \
    } \
    prevMode = dc->w.dc_field; \
    dc->w.dc_field = mode; \
    return prevMode; \
}


DC_SET_MODE( SetBkMode, backgroundMode, TRANSPARENT, OPAQUE,
	     META_SETBKMODE )                                     /* GDI.2 */
DC_SET_MODE( SetROP2, ROPmode, R2_BLACK, R2_WHITE, META_SETROP2 ) /* GDI.4 */
DC_SET_MODE( SetRelAbs, relAbsMode, ABSOLUTE, RELATIVE,
	     META_SETRELABS )                                     /* GDI.5 */
DC_SET_MODE( SetPolyFillMode, polyFillMode, ALTERNATE, WINDING,
	     META_SETPOLYFILLMODE )                               /* GDI.6 */
DC_SET_MODE( SetStretchBltMode, stretchBltMode,
	     BLACKONWHITE, COLORONCOLOR, META_SETSTRETCHBLTMODE ) /* GDI.7 */
DC_GET_VAL( COLORREF, GetBkColor, backgroundColor )               /* GDI.75 */
DC_GET_VAL( WORD, GetBkMode, backgroundMode )                     /* GDI.76 */
DC_GET_X_Y( DWORD, GetCurrentPosition, CursPosX, CursPosY )       /* GDI.78 */
DC_GET_VAL( WORD, GetMapMode, MapMode )                           /* GDI.81 */
DC_GET_VAL( WORD, GetPolyFillMode, polyFillMode )                 /* GDI.84 */
DC_GET_VAL( WORD, GetROP2, ROPmode )                              /* GDI.85 */
DC_GET_VAL( WORD, GetRelAbs, relAbsMode )                         /* GDI.86 */
DC_GET_VAL( WORD, GetStretchBltMode, stretchBltMode )             /* GDI.88 */
DC_GET_VAL( COLORREF, GetTextColor, textColor )                   /* GDI.90 */
DC_GET_X_Y( DWORD, GetViewportExt, VportExtX, VportExtY )         /* GDI.94 */
DC_GET_X_Y( DWORD, GetViewportOrg, VportOrgX, VportOrgY )         /* GDI.95 */
DC_GET_X_Y( DWORD, GetWindowExt, WndExtX, WndExtY )               /* GDI.96 */
DC_GET_X_Y( DWORD, GetWindowOrg, WndOrgX, WndOrgY )               /* GDI.97 */
DC_GET_VAL( HRGN32, InquireVisRgn, hVisRgn )                      /* GDI.131 */
DC_GET_X_Y( DWORD, GetBrushOrg, brushOrgX, brushOrgY )            /* GDI.149 */
DC_GET_VAL( HRGN32, GetClipRgn, hClipRgn )                        /* GDI.173 */
DC_GET_VAL( WORD, GetTextAlign, textAlign )                       /* GDI.345 */
DC_GET_VAL( HFONT16, GetCurLogFont, hFont )                       /* GDI.411 */
DC_GET_VAL_EX( GetBrushOrgEx, brushOrgX, brushOrgY )              /* GDI.469 */
DC_GET_VAL_EX( GetCurrentPositionEx, CursPosX, CursPosY )         /* GDI.470 */
DC_GET_VAL_EX( GetViewportExtEx, VportExtX, VportExtY )           /* GDI.472 */
DC_GET_VAL_EX( GetViewportOrgEx, VportOrgX, VportOrgY )           /* GDI.473 */
DC_GET_VAL_EX( GetWindowExtEx, WndExtX, WndExtY )                 /* GDI.474 */
DC_GET_VAL_EX( GetWindowOrgEx, WndOrgX, WndOrgY )                 /* GDI.475 */
