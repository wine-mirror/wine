/*
 * DC device-independent Get/SetXXX functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "gdi.h"
#include "metafile.h"

  /* Default DC values */
const WIN_DC_INFO DCVAL_defaultValues =
{
    0,                      /* flags */
    NULL,                   /* devCaps */
    0,                      /* hMetaFile */
    0,                      /* hClipRgn */
    0,                      /* hVisRgn */
    0,                      /* hGCClipRgn */
    STOCK_BLACK_PEN,        /* hPen */
    STOCK_WHITE_BRUSH,      /* hBrush */
    STOCK_SYSTEM_FONT,      /* hFont */
    0,                      /* hBitmap */
    0,                      /* hDevice */
    STOCK_DEFAULT_PALETTE,  /* hPalette */
    R2_COPYPEN,             /* ROPmode */
    ALTERNATE,              /* polyFillMode */
    BLACKONWHITE,           /* stretchBltMode */
    ABSOLUTE,               /* relAbsMode */
    OPAQUE,                 /* backgroundMode */
    RGB( 255, 255, 255 ),   /* backgroundColor */
    RGB( 0, 0, 0 ),         /* textColor */
    0,                      /* backgroundPixel */
    0,                      /* textPixel */
    0,                      /* brushOrgX */
    0,                      /* brushOrgY */
    TA_LEFT | TA_TOP | TA_NOUPDATECP,  /* textAlign */
    0,                      /* charExtra */
    0,                      /* breakTotalExtra */
    0,                      /* breakCount */
    0,                      /* breakExtra */
    0,                      /* breakRem */
    1,                      /* bitsPerPixel */
    MM_TEXT,                /* MapMode */
    0,                      /* DCOrgX */
    0,                      /* DCOrgY */
    0,                      /* DCSizeX */
    0,                      /* DCSizeY */
    0,                      /* CursPosX */
    0,                      /* CursPosY */
    0,                      /* WndOrgX */
    0,                      /* WndOrgY */
    1,                      /* WndExtX */
    1,                      /* WndExtY */
    0,                      /* VportOrgX */
    0,                      /* VportOrgY */
    1,                      /* VportExtX */
    1                       /* VportExtY */
};


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
    return MAKELONG( dc->w.ret_x, dc->w.ret_y << 16 ); \
}

#define DC_GET_VAL_EX( func_name, ret_x, ret_y ) \
BOOL func_name( HDC hdc, LPPOINT pt ) \
{ \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return FALSE; \
    pt->x = dc->w.ret_x; \
    pt->y = dc->w.ret_y; \
    return TRUE; \
}

#define DC_SET_VAL( func_type, func_name, dc_field ) \
func_type func_name( HDC hdc, func_type val ) \
{ \
    func_type prevVal; \
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ); \
    if (!dc) return 0; \
    prevVal = dc->w.dc_field; \
    dc->w.dc_field = val; \
    return prevVal; \
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
DC_GET_VAL( HRGN, InquireVisRgn, hVisRgn )                        /* GDI.131 */
DC_GET_X_Y( DWORD, GetBrushOrg, brushOrgX, brushOrgY )            /* GDI.149 */
DC_GET_VAL( HRGN, GetClipRgn, hClipRgn )                          /* GDI.173 */
DC_GET_VAL( WORD, GetTextAlign, textAlign )                       /* GDI.345 */
DC_SET_VAL( WORD, SetTextAlign, textAlign )                       /* GDI.346 */
DC_GET_VAL( HFONT, GetCurLogFont, hFont )                         /* GDI.411 */
DC_GET_VAL_EX( GetBrushOrgEx, brushOrgX, brushOrgY )              /* GDI.469 */
DC_GET_VAL_EX( GetCurrentPositionEx, CursPosX, CursPosY )         /* GDI.470 */
DC_GET_VAL_EX( GetViewportExtEx, VportExtX, VportExtY )           /* GDI.472 */
DC_GET_VAL_EX( GetViewportOrgEx, VportOrgX, VportOrgY )           /* GDI.473 */
DC_GET_VAL_EX( GetWindowExtEx, WndExtX, WndExtY )                 /* GDI.474 */
DC_GET_VAL_EX( GetWindowOrgEx, WndOrgX, WndOrgY )                 /* GDI.475 */
