/*
 * MetaFile driver DC value functions
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

#include "mfdrv/metafiledrv.h"

BOOL METADC_SaveDC( HDC hdc )
{
    return metadc_param0( hdc, META_SAVEDC );
}

BOOL CDECL MFDRV_RestoreDC( PHYSDEV dev, INT level )
{
    return MFDRV_MetaParam1( dev, META_RESTOREDC, level );
}

BOOL METADC_SetTextAlign( HDC hdc, UINT align )
{
    return metadc_param2( hdc, META_SETTEXTALIGN, HIWORD(align), LOWORD(align) );
}

BOOL METADC_SetBkMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETBKMODE, (WORD)mode );
}

COLORREF CDECL MFDRV_SetBkColor( PHYSDEV dev, COLORREF color )
{
    return MFDRV_MetaParam2(dev, META_SETBKCOLOR, HIWORD(color), LOWORD(color)) ? color : CLR_INVALID;
}

COLORREF CDECL MFDRV_SetTextColor( PHYSDEV dev, COLORREF color )
{
    return MFDRV_MetaParam2(dev, META_SETTEXTCOLOR, HIWORD(color), LOWORD(color)) ? color : CLR_INVALID;
}

BOOL METADC_SetROP2( HDC hdc, INT rop )
{
    return metadc_param1( hdc, META_SETROP2, (WORD)rop );
}

BOOL METADC_SetRelAbs( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETRELABS, (WORD)mode );
}

BOOL METADC_SetPolyFillMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETPOLYFILLMODE, mode );
}

BOOL METADC_SetStretchBltMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETSTRETCHBLTMODE, mode );
}

INT CDECL MFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return MFDRV_MetaParam4( dev, META_INTERSECTCLIPRECT, left, top, right, bottom );
}

INT CDECL MFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    return MFDRV_MetaParam4( dev, META_EXCLUDECLIPRECT, left, top, right, bottom );
}

INT CDECL MFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y )
{
    return MFDRV_MetaParam2( dev, META_OFFSETCLIPRGN, x, y );
}

DWORD CDECL MFDRV_SetLayout( PHYSDEV dev, DWORD layout )
{
    return MFDRV_MetaParam2( dev, META_SETLAYOUT, HIWORD(layout), LOWORD(layout) );
}

INT CDECL MFDRV_SetMapMode( PHYSDEV dev, INT mode )
{
    return MFDRV_MetaParam1( dev, META_SETMAPMODE, mode );
}

BOOL CDECL MFDRV_SetViewportExtEx( PHYSDEV dev, INT x, INT y, SIZE *size )
{
    return MFDRV_MetaParam2( dev, META_SETVIEWPORTEXT, x, y );
}

BOOL CDECL MFDRV_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_SETVIEWPORTORG, x, y );
}

BOOL CDECL MFDRV_SetWindowExtEx( PHYSDEV dev, INT x, INT y, SIZE *size )
{
    return MFDRV_MetaParam2( dev, META_SETWINDOWEXT, x, y );
}

BOOL CDECL MFDRV_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_SETWINDOWORG, x, y );
}

BOOL CDECL MFDRV_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_OFFSETVIEWPORTORG, x, y );
}

BOOL CDECL MFDRV_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    return MFDRV_MetaParam2( dev, META_OFFSETWINDOWORG, x, y );
}

BOOL CDECL MFDRV_ScaleViewportExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    return MFDRV_MetaParam4( dev, META_SCALEVIEWPORTEXT, xNum, xDenom, yNum, yDenom );
}

BOOL CDECL MFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    return MFDRV_MetaParam4( dev, META_SCALEWINDOWEXT, xNum, xDenom, yNum, yDenom );
}

BOOL CDECL MFDRV_SetTextJustification( PHYSDEV dev, INT extra, INT breaks )
{
    return MFDRV_MetaParam2( dev, META_SETTEXTJUSTIFICATION, extra, breaks );
}

INT CDECL MFDRV_SetTextCharacterExtra( PHYSDEV dev, INT extra )
{
    return MFDRV_MetaParam1( dev, META_SETTEXTCHAREXTRA, extra ) ? extra : 0x80000000;
}

DWORD CDECL MFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags )
{
    return MFDRV_MetaParam2( dev, META_SETMAPPERFLAGS, HIWORD(flags), LOWORD(flags) ) ? flags : GDI_ERROR;
}

BOOL CDECL MFDRV_AbortPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL CDECL MFDRV_BeginPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL CDECL MFDRV_CloseFigure( PHYSDEV dev )
{
    return FALSE;
}

BOOL CDECL MFDRV_EndPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL CDECL MFDRV_FillPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL CDECL MFDRV_FlattenPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL CDECL MFDRV_SelectClipPath( PHYSDEV dev, INT iMode )
{
    return FALSE;
}

BOOL CDECL MFDRV_StrokeAndFillPath( PHYSDEV dev )
{
    return FALSE;
}

BOOL CDECL MFDRV_StrokePath( PHYSDEV dev )
{
    return FALSE;
}

BOOL CDECL MFDRV_WidenPath( PHYSDEV dev )
{
    return FALSE;
}

COLORREF CDECL MFDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color )
{
    return CLR_INVALID;
}

COLORREF CDECL MFDRV_SetDCPenColor( PHYSDEV dev, COLORREF color )
{
    return CLR_INVALID;
}
