/*
 * Metafile driver definitions
 *
 * Copyright 1996 Alexandre Julliard
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

#ifndef __WINE_METAFILEDRV_H
#define __WINE_METAFILEDRV_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"

/* Metafile driver physical DC */

typedef struct
{
    HDC          hdc;
    METAHEADER  *mh;           /* Pointer to metafile header */
    UINT       handles_size, cur_handles;
    HGDIOBJ   *handles;
    HANDLE     hFile;          /* Handle for disk based MetaFile */
} METAFILEDRV_PDEVICE;

#define HANDLE_LIST_INC 20


extern BOOL MFDRV_MetaParam0(PHYSDEV dev, short func);
extern BOOL MFDRV_MetaParam1(PHYSDEV dev, short func, short param1);
extern BOOL MFDRV_MetaParam2(PHYSDEV dev, short func, short param1, short param2);
extern BOOL MFDRV_MetaParam4(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4);
extern BOOL MFDRV_MetaParam6(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4, short param5,
                             short param6);
extern BOOL MFDRV_MetaParam8(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4, short param5,
                             short param6, short param7, short param8);
extern BOOL MFDRV_WriteRecord(PHYSDEV dev, METARECORD *mr, DWORD rlen);
extern UINT MFDRV_AddHandle( PHYSDEV dev, HGDIOBJ obj );
extern BOOL MFDRV_RemoveHandle( PHYSDEV dev, UINT index );
extern INT16 MFDRV_CreateBrushIndirect( PHYSDEV dev, HBRUSH hBrush );

/* Metafile driver functions */

extern BOOL MFDRV_AbortPath( PHYSDEV dev );
extern BOOL MFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                       INT xstart, INT ystart, INT xend, INT yend );
extern BOOL MFDRV_BeginPath( PHYSDEV dev );
extern BOOL MFDRV_BitBlt( PHYSDEV devDst, INT xDst, INT yDst,  INT width,
                          INT height, PHYSDEV devSrc, INT xSrc, INT ySrc,
                          DWORD rop );
extern BOOL MFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right,
                         INT bottom, INT xstart, INT ystart, INT xend,
                         INT yend );
extern BOOL MFDRV_CloseFigure( PHYSDEV dev );
extern BOOL MFDRV_DeleteObject( PHYSDEV dev, HGDIOBJ obj );
extern BOOL MFDRV_Ellipse( PHYSDEV dev, INT left, INT top,
                           INT right, INT bottom );
extern BOOL MFDRV_EndPath( PHYSDEV dev );
extern INT MFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT
                                  bottom );
extern INT MFDRV_ExtEscape( PHYSDEV dev, INT nEscape, INT cbInput, LPCVOID in_data,
                            INT cbOutput, LPVOID out_data );
extern BOOL MFDRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType );
extern INT  MFDRV_ExtSelectClipRgn( PHYSDEV dev, HRGN hrgn, INT mode );
extern BOOL MFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y,
                              UINT flags, const RECT *lprect, LPCWSTR str,
                              UINT count, const INT *lpDx );
extern BOOL MFDRV_FillPath( PHYSDEV dev );
extern BOOL MFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush );
extern BOOL MFDRV_FlattenPath( PHYSDEV dev );
extern BOOL MFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT x, INT y );
extern INT MFDRV_GetDeviceCaps( PHYSDEV dev , INT cap );
extern INT MFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT
                                    bottom );
extern BOOL MFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn );
extern BOOL MFDRV_LineTo( PHYSDEV dev, INT x, INT y );
extern BOOL MFDRV_MoveTo( PHYSDEV dev, INT x, INT y );
extern INT  MFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y );
extern INT  MFDRV_OffsetViewportOrg( PHYSDEV dev, INT x, INT y );
extern INT  MFDRV_OffsetWindowOrg( PHYSDEV dev, INT x, INT y );
extern BOOL MFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn );
extern BOOL MFDRV_PatBlt( PHYSDEV dev, INT left, INT top, INT width, INT height,
                          DWORD rop );
extern BOOL MFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right,
                       INT bottom, INT xstart, INT ystart, INT xend,
                       INT yend );
extern BOOL MFDRV_PolyBezier( PHYSDEV dev, const POINT* pt, DWORD count );
extern BOOL MFDRV_PolyBezierTo( PHYSDEV dev, const POINT* pt, DWORD count );
extern BOOL MFDRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts,
                               UINT polygons);
extern BOOL MFDRV_Polygon( PHYSDEV dev, const POINT* pt, INT count );
extern BOOL MFDRV_Polyline( PHYSDEV dev, const POINT* pt,INT count);
extern BOOL MFDRV_Rectangle( PHYSDEV dev, INT left, INT top,
                             INT right, INT bottom);
extern BOOL MFDRV_RestoreDC( PHYSDEV dev, INT level );
extern BOOL MFDRV_RoundRect( PHYSDEV dev, INT left, INT top,
                             INT right, INT bottom, INT ell_width,
                             INT ell_height );
extern INT MFDRV_SaveDC( PHYSDEV dev );
extern INT MFDRV_ScaleViewportExt( PHYSDEV dev, INT xNum, INT xDenom, INT yNum,
				   INT yDenom );
extern INT MFDRV_ScaleWindowExt( PHYSDEV dev, INT xNum, INT xDenom, INT yNum,
				 INT yDenom );
extern HBITMAP MFDRV_SelectBitmap( PHYSDEV dev, HBITMAP handle );
extern HBRUSH MFDRV_SelectBrush( PHYSDEV dev, HBRUSH handle );
extern BOOL MFDRV_SelectClipPath( PHYSDEV dev, INT iMode );
extern HFONT MFDRV_SelectFont( PHYSDEV dev, HFONT handle, HANDLE gdiFont );
extern HPEN MFDRV_SelectPen( PHYSDEV dev, HPEN handle );
extern HPALETTE MFDRV_SelectPalette( PHYSDEV dev, HPALETTE hPalette, BOOL bForceBackground);
extern UINT MFDRV_RealizePalette(PHYSDEV dev, HPALETTE hPalette, BOOL primary);
extern COLORREF MFDRV_SetBkColor( PHYSDEV dev, COLORREF color );
extern INT MFDRV_SetBkMode( PHYSDEV dev, INT mode );
extern INT MFDRV_SetMapMode( PHYSDEV dev, INT mode );
extern DWORD MFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags );
extern COLORREF MFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color );
extern INT MFDRV_SetPolyFillMode( PHYSDEV dev, INT mode );
extern INT MFDRV_SetROP2( PHYSDEV dev, INT rop );
extern INT MFDRV_SetRelAbs( PHYSDEV dev, INT mode );
extern INT MFDRV_SetStretchBltMode( PHYSDEV dev, INT mode );
extern UINT MFDRV_SetTextAlign( PHYSDEV dev, UINT align );
extern INT MFDRV_SetTextCharacterExtra( PHYSDEV dev, INT extra );
extern COLORREF MFDRV_SetTextColor( PHYSDEV dev, COLORREF color );
extern INT MFDRV_SetTextJustification( PHYSDEV dev, INT extra, INT breaks );
extern INT MFDRV_SetViewportExt( PHYSDEV dev, INT x, INT y );
extern INT MFDRV_SetViewportOrg( PHYSDEV dev, INT x, INT y );
extern INT MFDRV_SetWindowExt( PHYSDEV dev, INT x, INT y );
extern INT MFDRV_SetWindowOrg( PHYSDEV dev, INT x, INT y );
extern BOOL MFDRV_StretchBlt( PHYSDEV devDst, INT xDst, INT yDst, INT widthDst,
                              INT heightDst, PHYSDEV devSrc, INT xSrc, INT ySrc,
                              INT widthSrc, INT heightSrc, DWORD rop );
extern BOOL MFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn );
extern INT MFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDest, INT yDest, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc,
                                    UINT startscan, UINT lines, LPCVOID bits,
                                    const BITMAPINFO *info, UINT coloruse );
extern INT MFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                                INT heightDst, INT xSrc, INT ySrc,
                                INT widthSrc, INT heightSrc, const void *bits,
                                const BITMAPINFO *info, UINT wUsage,
                                DWORD dwRop );
extern BOOL MFDRV_StrokeAndFillPath( PHYSDEV dev );
extern BOOL MFDRV_StrokePath( PHYSDEV dev );
extern BOOL MFDRV_WidenPath( PHYSDEV dev );

#endif  /* __WINE_METAFILEDRV_H */
