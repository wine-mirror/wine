/*
 * Enhanced MetaFile driver dc value functions
 *
 * Copyright 1999 Huw D M Davies
 *
 */
#include "enhmetafiledrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(enhmetafile)

INT EMFDRV_SaveDC( DC *dc )
{
    EMRSAVEDC emr;
    emr.emr.iType = EMR_SAVEDC;
    emr.emr.nSize = sizeof(emr);
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_RestoreDC( DC *dc, INT level )
{
    EMRRESTOREDC emr;
    emr.emr.iType = EMR_RESTOREDC;
    emr.emr.nSize = sizeof(emr);
    emr.iRelative = level;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

UINT EMFDRV_SetTextAlign( DC *dc, UINT align )
{
    EMRSETTEXTALIGN emr;
    emr.emr.iType = EMR_SETTEXTALIGN;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = align;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

INT EMFDRV_SetBkMode( DC *dc, INT mode )
{
    EMRSETBKMODE emr;
    emr.emr.iType = EMR_SETBKMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

INT EMFDRV_SetROP2( DC *dc, INT rop )
{
    EMRSETROP2 emr;
    emr.emr.iType = EMR_SETROP2;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = rop;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

INT EMFDRV_SetPolyFillMode( DC *dc, INT mode )
{
    EMRSETPOLYFILLMODE emr;
    emr.emr.iType = EMR_SETPOLYFILLMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

INT EMFDRV_SetStretchBltMode( DC *dc, INT mode )
{
    EMRSETSTRETCHBLTMODE emr;
    emr.emr.iType = EMR_SETSTRETCHBLTMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

INT EMFDRV_SetMapMode( DC *dc, INT mode )
{
    EMRSETMAPMODE emr;
    emr.emr.iType = EMR_SETMAPMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

INT EMFDRV_ExcludeClipRect( DC *dc, INT left, INT top, INT right, INT bottom )
{
    EMREXCLUDECLIPRECT emr;
    emr.emr.iType      = EMR_EXCLUDECLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

INT EMFDRV_IntersectClipRect( DC *dc, INT left, INT top, INT right, INT bottom)
{
    EMRINTERSECTCLIPRECT emr;
    emr.emr.iType      = EMR_INTERSECTCLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

INT EMFDRV_OffsetClipRgn( DC *dc, INT x, INT y )
{
    EMROFFSETCLIPRGN emr;
    emr.emr.iType   = EMR_OFFSETCLIPRGN;
    emr.emr.nSize   = sizeof(emr);
    emr.ptlOffset.x = x;
    emr.ptlOffset.y = y;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

DWORD EMFDRV_SetMapperFlags( DC *dc, DWORD flags )
{
    EMRSETMAPPERFLAGS emr;

    emr.emr.iType = EMR_SETMAPPERFLAGS;
    emr.emr.nSize = sizeof(emr);
    emr.dwFlags   = flags;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_AbortPath( DC *dc )
{
    EMRABORTPATH emr;

    emr.emr.iType = EMR_ABORTPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_BeginPath( DC *dc )
{
    EMRBEGINPATH emr;

    emr.emr.iType = EMR_BEGINPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_CloseFigure( DC *dc )
{
    EMRCLOSEFIGURE emr;

    emr.emr.iType = EMR_CLOSEFIGURE;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_EndPath( DC *dc )
{
    EMRENDPATH emr;

    emr.emr.iType = EMR_ENDPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_FillPath( DC *dc )
{
    EMRFILLPATH emr;

    emr.emr.iType = EMR_FILLPATH;
    emr.emr.nSize = sizeof(emr);
    FIXME("Bounds\n");
    emr.rclBounds.left = 0;
    emr.rclBounds.top = 0;
    emr.rclBounds.right = 0;
    emr.rclBounds.bottom = 0;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_FlattenPath( DC *dc )
{
    EMRFLATTENPATH emr;

    emr.emr.iType = EMR_FLATTENPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_SelectClipPath( DC *dc, INT iMode )
{
    EMRSELECTCLIPPATH emr;

    emr.emr.iType = EMR_SELECTCLIPPATH;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = iMode;

    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_StrokeAndFillPath( DC *dc )
{
    EMRSTROKEANDFILLPATH emr;

    emr.emr.iType = EMR_STROKEANDFILLPATH;
    emr.emr.nSize = sizeof(emr);
    FIXME("Bounds\n");
    emr.rclBounds.left = 0;
    emr.rclBounds.top = 0;
    emr.rclBounds.right = 0;
    emr.rclBounds.bottom = 0;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_StrokePath( DC *dc )
{
    EMRSTROKEPATH emr;

    emr.emr.iType = EMR_STROKEPATH;
    emr.emr.nSize = sizeof(emr);
    FIXME("Bounds\n");
    emr.rclBounds.left = 0;
    emr.rclBounds.top = 0;
    emr.rclBounds.right = 0;
    emr.rclBounds.bottom = 0;
    return EMFDRV_WriteRecord( dc, &emr.emr );
}

BOOL EMFDRV_WidenPath( DC *dc )
{
    EMRWIDENPATH emr;

    emr.emr.iType = EMR_WIDENPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dc, &emr.emr );
}
