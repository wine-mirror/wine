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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "metafiledrv.h"

INT MFDRV_SaveDC( DC *dc )
{
    return MFDRV_MetaParam0( dc, META_SAVEDC );
}

BOOL MFDRV_RestoreDC( DC *dc, INT level )
{
    if(level != -1) return FALSE;
    return MFDRV_MetaParam1( dc, META_RESTOREDC, level );
}

UINT MFDRV_SetTextAlign( DC *dc, UINT align )
{
    return MFDRV_MetaParam1( dc, META_SETTEXTALIGN, (WORD)align);
}

INT MFDRV_SetBkMode( DC *dc, INT mode )
{
    return MFDRV_MetaParam1( dc, META_SETBKMODE, (WORD)mode);
}

INT MFDRV_SetROP2( DC *dc, INT rop )
{
    return MFDRV_MetaParam1( dc, META_SETROP2, (WORD)rop);
}

INT MFDRV_SetRelAbs( DC *dc, INT mode )
{
    return MFDRV_MetaParam1( dc, META_SETRELABS, (WORD)mode);
}

INT MFDRV_SetPolyFillMode( DC *dc, INT mode )
{
    return MFDRV_MetaParam1( dc, META_SETPOLYFILLMODE, (WORD)mode);
}

INT MFDRV_SetStretchBltMode( DC *dc, INT mode )
{
    return MFDRV_MetaParam1( dc, META_SETSTRETCHBLTMODE, (WORD)mode);
}

INT MFDRV_IntersectClipRect( DC *dc, INT left, INT top, INT right, INT bottom )
{
    return MFDRV_MetaParam4( dc, META_INTERSECTCLIPRECT, left, top, right,
			     bottom );
}

INT MFDRV_ExcludeClipRect( DC *dc, INT left, INT top, INT right, INT bottom )
{
    return MFDRV_MetaParam4( dc, META_EXCLUDECLIPRECT, left, top, right,
			     bottom );
}

INT MFDRV_OffsetClipRgn( DC *dc, INT x, INT y )
{
    return MFDRV_MetaParam2( dc, META_OFFSETCLIPRGN, x, y );
}

INT MFDRV_SetTextJustification( DC *dc, INT extra, INT breaks )
{
    return MFDRV_MetaParam2( dc, META_SETTEXTJUSTIFICATION, extra, breaks );
}

INT MFDRV_SetTextCharacterExtra( DC *dc, INT extra )
{
    return MFDRV_MetaParam1( dc, META_SETTEXTCHAREXTRA, extra );
}

DWORD MFDRV_SetMapperFlags( DC *dc, DWORD flags )
{
    return MFDRV_MetaParam2( dc, META_SETMAPPERFLAGS, HIWORD(flags),
			     LOWORD(flags) );
}

BOOL MFDRV_AbortPath( DC *dc )
{
    return FALSE;
}

BOOL MFDRV_BeginPath( DC *dc )
{
    return FALSE;
}

BOOL MFDRV_CloseFigure( DC *dc )
{
    return FALSE;
}

BOOL MFDRV_EndPath( DC *dc )
{
    return FALSE;
}

BOOL MFDRV_FillPath( DC *dc )
{
    return FALSE;
}

BOOL MFDRV_FlattenPath( DC *dc )
{
    return FALSE;
}

BOOL MFDRV_SelectClipPath( DC *dc, INT iMode )
{
    return FALSE;
}

BOOL MFDRV_StrokeAndFillPath( DC *dc )
{
    return FALSE;
}

BOOL MFDRV_StrokePath( DC *dc )
{
    return FALSE;
}

BOOL MFDRV_WidenPath( DC *dc )
{
    return FALSE;
}
