/*
 * Enhanced MetaFile driver definitions
 *
 * Copyright 1999 Huw D M Davies
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_ENHMETAFILEDRV_H
#define __WINE_ENHMETAFILEDRV_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi_private.h"
#include "gdi_private.h"

/* Enhanced Metafile driver physical DC */

typedef struct
{
    struct gdi_physdev dev;
    ENHMETAHEADER  *emh;           /* Pointer to enhanced metafile header */
    UINT       handles_size, cur_handles;
    HGDIOBJ   *handles;
    HANDLE     hFile;              /* Handle for disk based MetaFile */
    HBRUSH     dc_brush;
    HPEN       dc_pen;
    INT        restoring;          /* RestoreDC counter */
    BOOL       path;
    INT        dev_caps[COLORMGMTCAPS + 1];
} EMFDRV_PDEVICE;

static inline EMFDRV_PDEVICE *get_emf_physdev( PHYSDEV dev )
{
    return CONTAINING_RECORD( dev, EMFDRV_PDEVICE, dev );
}

extern BOOL EMFDRV_WriteRecord( PHYSDEV dev, EMR *emr ) DECLSPEC_HIDDEN;
extern void EMFDRV_UpdateBBox( PHYSDEV dev, RECTL *rect ) DECLSPEC_HIDDEN;
extern DWORD EMFDRV_CreateBrushIndirect( PHYSDEV dev, HBRUSH hBrush ) DECLSPEC_HIDDEN;

#define HANDLE_LIST_INC 20

/* Metafile driver functions */
extern BOOL     CDECL EMFDRV_AlphaBlend( PHYSDEV dev_dst, struct bitblt_coords *dst,
                                         PHYSDEV dev_src, struct bitblt_coords *src, BLENDFUNCTION func ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right,
                                  INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_ArcTo( PHYSDEV dev, INT left, INT top, INT right,
                                    INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_BitBlt( PHYSDEV devDst, INT xDst, INT yDst, INT width, INT height,
                                     PHYSDEV devSrc, INT xSrc, INT ySrc, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                    INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprect, LPCWSTR str,
                                         UINT count, const INT *lpDx ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT width, INT height ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_GdiComment( PHYSDEV dev, UINT bytes, const BYTE *buffer ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_GetDeviceCaps( PHYSDEV dev, INT cap ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                           void *grad_array, ULONG ngrad, ULONG mode ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                  INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PolyDraw( PHYSDEV dev, const POINT *pts, const BYTE *types, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polys) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polys) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PolylineTo( PHYSDEV dev, const POINT* pt,INT count) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_RestoreDC( PHYSDEV dev, INT level ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                        INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom,
                                               INT yNum, INT yDenom, SIZE *size ) DECLSPEC_HIDDEN;
extern HBITMAP  CDECL EMFDRV_SelectBitmap( PHYSDEV dev, HBITMAP handle ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_SelectClipPath( PHYSDEV dev, INT iMode ) DECLSPEC_HIDDEN;
extern HFONT    CDECL EMFDRV_SelectFont( PHYSDEV dev, HFONT handle, UINT *aa_flags ) DECLSPEC_HIDDEN;
extern COLORREF CDECL EMFDRV_SetBkColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL EMFDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL EMFDRV_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDest, INT yDest, DWORD cx, DWORD cy, INT xSrc,
                                                INT ySrc, UINT startscan, UINT lines, LPCVOID bits,
                                                BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
extern COLORREF CDECL EMFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL EMFDRV_SetTextColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                            INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                                            const void *bits, BITMAPINFO *info, UINT wUsage, DWORD dwRop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;


#endif  /* __WINE_METAFILEDRV_H */
