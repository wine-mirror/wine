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
#include "ntgdi_private.h"
#include "gdi_private.h"

/* Metafile driver physical DC */

typedef struct
{
    struct gdi_physdev dev;
    METAHEADER  *mh;           /* Pointer to metafile header */
    UINT       handles_size, cur_handles;
    HGDIOBJ   *handles;
    HANDLE     hFile;          /* Handle for disk based MetaFile */
    HPEN       pen;
    HBRUSH     brush;
    HFONT      font;
} METAFILEDRV_PDEVICE;

#define HANDLE_LIST_INC 20


extern BOOL MFDRV_MetaParam0(PHYSDEV dev, short func) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam1(PHYSDEV dev, short func, short param1) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam2(PHYSDEV dev, short func, short param1, short param2) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam4(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam6(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4, short param5,
                             short param6) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam8(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4, short param5,
                             short param6, short param7, short param8) DECLSPEC_HIDDEN;
extern BOOL MFDRV_WriteRecord(PHYSDEV dev, METARECORD *mr, DWORD rlen) DECLSPEC_HIDDEN;
extern UINT MFDRV_AddHandle( PHYSDEV dev, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_RemoveHandle( PHYSDEV dev, UINT index ) DECLSPEC_HIDDEN;
extern INT16 MFDRV_CreateBrushIndirect( PHYSDEV dev, HBRUSH hBrush ) DECLSPEC_HIDDEN;

extern METAFILEDRV_PDEVICE *get_metadc_ptr( HDC hdc ) DECLSPEC_HIDDEN;
extern BOOL metadc_param0( HDC hdc, short func ) DECLSPEC_HIDDEN;
extern BOOL metadc_param1( HDC hdc, short func, short param ) DECLSPEC_HIDDEN;
extern BOOL metadc_param2( HDC hdc, short func, short param1, short param2 ) DECLSPEC_HIDDEN;
extern BOOL metadc_param4( HDC hdc, short func, short param1, short param2,
                           short param3, short param4 ) DECLSPEC_HIDDEN;
extern BOOL metadc_param5( HDC hdc, short func, short param1, short param2, short param3,
                           short param4, short param5 ) DECLSPEC_HIDDEN;
extern BOOL metadc_param6( HDC hdc, short func, short param1, short param2, short param3,
                           short param4, short param5, short param6 ) DECLSPEC_HIDDEN;
extern BOOL metadc_param8( HDC hdc, short func, short param1, short param2,
                           short param3, short param4, short param5, short param6,
                           short param7, short param8 ) DECLSPEC_HIDDEN;
extern BOOL metadc_record( HDC hdc, METARECORD *mr, DWORD rlen ) DECLSPEC_HIDDEN;

/* Metafile driver functions */

extern BOOL CDECL MFDRV_AbortPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                               INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_BeginPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_CloseFigure( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_EndPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_PolyBezier( PHYSDEV dev, const POINT* pt, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_PolyBezierTo( PHYSDEV dev, const POINT* pt, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_RestoreDC( PHYSDEV dev, INT level ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_SelectClipPath( PHYSDEV dev, INT iMode ) DECLSPEC_HIDDEN;
extern COLORREF CDECL MFDRV_SetBkColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL MFDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL MFDRV_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF  CDECL MFDRV_SetTextColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;

#endif  /* __WINE_METAFILEDRV_H */
