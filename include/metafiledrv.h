/*
 * Metafile driver definitions
 */

#ifndef __WINE_METAFILEDRV_H
#define __WINE_METAFILEDRV_H

#include "windef.h"
#include "wingdi.h"
#include "gdi.h"

/* Metafile driver physical DC */

typedef struct
{
    METAHEADER  *mh;           /* Pointer to metafile header */
    UINT       nextHandle;     /* Next handle number */
    HFILE      hFile;          /* HFILE for disk based MetaFile */ 
} METAFILEDRV_PDEVICE;


extern BOOL MFDRV_MetaParam0(DC *dc, short func);
extern BOOL MFDRV_MetaParam1(DC *dc, short func, short param1);
extern BOOL MFDRV_MetaParam2(DC *dc, short func, short param1, short param2);
extern BOOL MFDRV_MetaParam4(DC *dc, short func, short param1, short param2, 
			     short param3, short param4);
extern BOOL MFDRV_MetaParam6(DC *dc, short func, short param1, short param2, 
			     short param3, short param4, short param5,
			     short param6);
extern BOOL MFDRV_MetaParam8(DC *dc, short func, short param1, short param2, 
			     short param3, short param4, short param5,
			     short param6, short param7, short param8);
extern BOOL MFDRV_WriteRecord(DC *dc, METARECORD *mr, DWORD rlen);
extern int MFDRV_AddHandleDC( DC *dc );
extern INT16 MFDRV_CreateBrushIndirect( DC *dc, HBRUSH hBrush );

/* Metafile driver functions */

extern BOOL MFDRV_AbortPath( DC *dc );
extern BOOL MFDRV_Arc( DC *dc, INT left, INT top, INT right, INT bottom,
		       INT xstart, INT ystart, INT xend, INT yend );
extern BOOL MFDRV_BeginPath( DC *dc );
extern BOOL MFDRV_BitBlt( DC *dcDst, INT xDst, INT yDst,  INT width,
			  INT height, DC *dcSrc, INT xSrc, INT ySrc,
			  DWORD rop );
extern BOOL MFDRV_Chord( DC *dc, INT left, INT top, INT right,
			 INT bottom, INT xstart, INT ystart, INT xend,
			 INT yend );
extern BOOL MFDRV_CloseFigure( DC *dc );
extern BOOL MFDRV_Ellipse( DC *dc, INT left, INT top,
			   INT right, INT bottom );
extern BOOL MFDRV_EndPath( DC *dc );
extern INT MFDRV_ExcludeClipRect( DC *dc, INT left, INT top, INT right, INT
				  bottom );
extern BOOL MFDRV_ExtFloodFill( DC *dc, INT x, INT y,
				COLORREF color, UINT fillType );
extern BOOL MFDRV_ExtTextOut( DC *dc, INT x, INT y,
			      UINT flags, const RECT *lprect, LPCWSTR str,
			      UINT count, const INT *lpDx );
extern BOOL MFDRV_FillPath( DC *dc );
extern BOOL MFDRV_FillRgn( DC *dc, HRGN hrgn, HBRUSH hbrush );
extern BOOL MFDRV_FlattenPath( DC *dc );
extern BOOL MFDRV_FrameRgn( DC *dc, HRGN hrgn, HBRUSH hbrush, INT x, INT y );
extern INT MFDRV_IntersectClipRect( DC *dc, INT left, INT top, INT right, INT
				    bottom );
extern BOOL MFDRV_InvertRgn( DC *dc, HRGN hrgn );
extern BOOL MFDRV_LineTo( DC *dc, INT x, INT y );
extern BOOL MFDRV_MoveToEx( DC *dc, INT x, INT y, LPPOINT pt );
extern INT MFDRV_OffsetClipRgn( DC *dc, INT x, INT y );
extern BOOL MFDRV_OffsetViewportOrg( DC *dc, INT x, INT y );
extern BOOL MFDRV_OffsetWindowOrg( DC *dc, INT x, INT y );
extern BOOL MFDRV_PaintRgn( DC *dc, HRGN hrgn );
extern BOOL MFDRV_PatBlt( DC *dc, INT left, INT top, INT width, INT height,
			  DWORD rop );
extern BOOL MFDRV_Pie( DC *dc, INT left, INT top, INT right,
		       INT bottom, INT xstart, INT ystart, INT xend,
		       INT yend );
extern BOOL MFDRV_PolyBezier( DC *dc, const POINT* pt, DWORD count );
extern BOOL MFDRV_PolyBezierTo( DC *dc, const POINT* pt, DWORD count );
extern BOOL MFDRV_PolyPolygon( DC *dc, const POINT* pt, const INT* counts,
			       UINT polygons);
extern BOOL MFDRV_Polygon( DC *dc, const POINT* pt, INT count );
extern BOOL MFDRV_Polyline( DC *dc, const POINT* pt,INT count);
extern BOOL MFDRV_Rectangle( DC *dc, INT left, INT top,
			     INT right, INT bottom);
extern BOOL MFDRV_RestoreDC( DC *dc, INT level );
extern BOOL MFDRV_RoundRect( DC *dc, INT left, INT top,
			     INT right, INT bottom, INT ell_width,
			     INT ell_height );
extern INT MFDRV_SaveDC( DC *dc );
extern BOOL MFDRV_ScaleViewportExt( DC *dc, INT xNum, INT xDenom, INT yNum,
				    INT yDenom );
extern BOOL MFDRV_ScaleWindowExt( DC *dc, INT xNum, INT xDenom, INT yNum,
				  INT yDenom );
extern BOOL MFDRV_SelectClipPath( DC *dc, INT iMode );
extern HGDIOBJ MFDRV_SelectObject( DC *dc, HGDIOBJ handle );
extern COLORREF MFDRV_SetBkColor( DC *dc, COLORREF color );
extern INT MFDRV_SetBkMode( DC *dc, INT mode );
extern INT MFDRV_SetMapMode( DC *dc, INT mode );
extern DWORD MFDRV_SetMapperFlags( DC *dc, DWORD flags );
extern COLORREF MFDRV_SetPixel( DC *dc, INT x, INT y, COLORREF color );
extern INT MFDRV_SetPolyFillMode( DC *dc, INT mode );
extern INT MFDRV_SetROP2( DC *dc, INT rop );
extern INT MFDRV_SetRelAbs( DC *dc, INT mode );
extern INT MFDRV_SetStretchBltMode( DC *dc, INT mode );
extern UINT MFDRV_SetTextAlign( DC *dc, UINT align );
extern INT MFDRV_SetTextCharacterExtra( DC *dc, INT extra );
extern COLORREF MFDRV_SetTextColor( DC *dc, COLORREF color );
extern INT MFDRV_SetTextJustification( DC *dc, INT extra, INT breaks );
extern BOOL MFDRV_SetViewportExt( DC *dc, INT x, INT y );
extern BOOL MFDRV_SetViewportOrg( DC *dc, INT x, INT y );
extern BOOL MFDRV_SetWindowExt( DC *dc, INT x, INT y );
extern BOOL MFDRV_SetWindowOrg( DC *dc, INT x, INT y );
extern BOOL MFDRV_StretchBlt( DC *dcDst, INT xDst, INT yDst, INT widthDst,
			      INT heightDst, DC *dcSrc, INT xSrc, INT ySrc,
			      INT widthSrc, INT heightSrc, DWORD rop );
extern BOOL MFDRV_PaintRgn( DC *dc, HRGN hrgn );
extern INT MFDRV_SetDIBitsToDevice( DC *dc, INT xDest, INT yDest, DWORD cx,
				    DWORD cy, INT xSrc, INT ySrc,
				    UINT startscan, UINT lines, LPCVOID bits,
				    const BITMAPINFO *info, UINT coloruse );
extern INT MFDRV_StretchDIBits( DC *dc, INT xDst, INT yDst, INT widthDst,
				INT heightDst, INT xSrc, INT ySrc,
				INT widthSrc, INT heightSrc, const void *bits,
				const BITMAPINFO *info, UINT wUsage,
				DWORD dwRop );
extern BOOL MFDRV_StrokeAndFillPath( DC *dc );
extern BOOL MFDRV_StrokePath( DC *dc );
extern BOOL MFDRV_WidenPath( DC *dc );

#endif  /* __WINE_METAFILEDRV_H */

