/*
 * Enhanced MetaFile driver definitions
 */

#ifndef __WINE_ENHMETAFILEDRV_H
#define __WINE_ENHMETAFILEDRV_H

#include "windef.h"
#include "wingdi.h"
#include "gdi.h"

/* Enhanced Metafile driver physical DC */

typedef struct
{
    ENHMETAHEADER  *emh;           /* Pointer to enhanced metafile header */
    UINT       nextHandle;         /* Next handle number */
    HFILE      hFile;              /* HFILE for disk based MetaFile */ 
} EMFDRV_PDEVICE;


extern BOOL EMFDRV_WriteRecord( DC *dc, EMR *emr );
extern int EMFDRV_AddHandleDC( DC *dc );
extern void EMFDRV_UpdateBBox( DC *dc, RECTL *rect );
extern DWORD EMFDRV_CreateBrushIndirect( DC *dc, HBRUSH hBrush );

/* Metafile driver functions */
extern BOOL     EMFDRV_AbortPath( DC *dc );
extern BOOL     EMFDRV_Arc( DC *dc, INT left, INT top, INT right,
			    INT bottom, INT xstart, INT ystart, INT xend,
			    INT yend );
extern BOOL     EMFDRV_BeginPath( DC *dc );
extern BOOL     EMFDRV_BitBlt( DC *dcDst, INT xDst, INT yDst,
			       INT width, INT height, DC *dcSrc,
			       INT xSrc, INT ySrc, DWORD rop );
extern BOOL     EMFDRV_Chord( DC *dc, INT left, INT top, INT right,
			      INT bottom, INT xstart, INT ystart, INT xend,
			      INT yend );
extern BOOL     EMFDRV_CloseFigure( DC *dc );
extern BOOL     EMFDRV_Ellipse( DC *dc, INT left, INT top,
				INT right, INT bottom );
extern BOOL     EMFDRV_EndPath( DC *dc );
extern INT      EMFDRV_ExcludeClipRect( DC *dc, INT left, INT top, INT right,
					INT bottom );
extern BOOL     EMFDRV_ExtFloodFill( DC *dc, INT x, INT y,
				     COLORREF color, UINT fillType );
extern BOOL     EMFDRV_ExtTextOut( DC *dc, INT x, INT y,
				   UINT flags, const RECT *lprect, LPCSTR str,
				   UINT count, const INT *lpDx );
extern BOOL     EMFDRV_FillPath( DC *dc );
extern BOOL     EMFDRV_FillRgn( DC *dc, HRGN hrgn, HBRUSH hbrush );
extern BOOL     EMFDRV_FlattenPath( DC *dc );
extern BOOL     EMFDRV_FrameRgn( DC *dc, HRGN hrgn, HBRUSH hbrush, INT width,
				 INT height );
extern INT      EMFDRV_IntersectClipRect( DC *dc, INT left, INT top, INT right,
					  INT bottom );
extern BOOL     EMFDRV_InvertRgn( DC *dc, HRGN hrgn );
extern BOOL     EMFDRV_LineTo( DC *dc, INT x, INT y );
extern BOOL     EMFDRV_MoveToEx( DC *dc, INT x, INT y, LPPOINT pt);
extern INT      EMFDRV_OffsetClipRgn( DC *dc, INT x, INT y );
extern BOOL     EMFDRV_OffsetViewportOrg( DC *dc, INT x, INT y );
extern BOOL     EMFDRV_OffsetWindowOrg( DC *dc, INT x, INT y );
extern BOOL     EMFDRV_PaintRgn( DC *dc, HRGN hrgn );
extern BOOL     EMFDRV_PatBlt( DC *dc, INT left, INT top,
			       INT width, INT height, DWORD rop );
extern BOOL     EMFDRV_Pie( DC *dc, INT left, INT top, INT right,
			    INT bottom, INT xstart, INT ystart, INT xend,
			    INT yend );
extern BOOL     EMFDRV_PolyPolygon( DC *dc, const POINT* pt,
				    const INT* counts, UINT polys);
extern BOOL     EMFDRV_PolyPolyline( DC *dc, const POINT* pt,
				     const DWORD* counts, DWORD polys);
extern BOOL     EMFDRV_Polygon( DC *dc, const POINT* pt, INT count );
extern BOOL     EMFDRV_Polyline( DC *dc, const POINT* pt,INT count);
extern BOOL     EMFDRV_Rectangle( DC *dc, INT left, INT top,
				  INT right, INT bottom);
extern BOOL     EMFDRV_RestoreDC( DC *dc, INT level );
extern BOOL     EMFDRV_RoundRect( DC *dc, INT left, INT top,
				  INT right, INT bottom, INT ell_width,
				  INT ell_height );
extern INT      EMFDRV_SaveDC( DC *dc );
extern BOOL     EMFDRV_ScaleViewportExt( DC *dc, INT xNum,
					 INT xDenom, INT yNum, INT yDenom );
extern BOOL     EMFDRV_ScaleWindowExt( DC *dc, INT xNum, INT xDenom,
				       INT yNum, INT yDenom );
extern BOOL     EMFDRV_SelectClipPath( DC *dc, INT iMode );
extern HGDIOBJ  EMFDRV_SelectObject( DC *dc, HGDIOBJ handle );
extern COLORREF EMFDRV_SetBkColor( DC *dc, COLORREF color );
extern INT      EMFDRV_SetBkMode( DC *dc, INT mode );
extern INT      EMFDRV_SetDIBitsToDevice( DC *dc, INT xDest, INT yDest,
					  DWORD cx, DWORD cy, INT xSrc,
					  INT ySrc, UINT startscan, UINT lines,
					  LPCVOID bits, const BITMAPINFO *info,
					  UINT coloruse );
extern INT      EMFDRV_SetMapMode( DC *dc, INT mode );
extern DWORD    EMFDRV_SetMapperFlags( DC *dc, DWORD flags );
extern COLORREF EMFDRV_SetPixel( DC *dc, INT x, INT y, COLORREF color );
extern INT      EMFDRV_SetPolyFillMode( DC *dc, INT mode );
extern INT      EMFDRV_SetROP2( DC *dc, INT rop );
extern INT      EMFDRV_SetStretchBltMode( DC *dc, INT mode );
extern UINT     EMFDRV_SetTextAlign( DC *dc, UINT align );
extern COLORREF EMFDRV_SetTextColor( DC *dc, COLORREF color );
extern BOOL     EMFDRV_SetViewportExt( DC *dc, INT x, INT y );
extern BOOL     EMFDRV_SetViewportOrg( DC *dc, INT x, INT y );
extern BOOL     EMFDRV_SetWindowExt( DC *dc, INT x, INT y );
extern BOOL     EMFDRV_SetWindowOrg( DC *dc, INT x, INT y );
extern BOOL     EMFDRV_StretchBlt( DC *dcDst, INT xDst, INT yDst,
				   INT widthDst, INT heightDst,
				   DC *dcSrc, INT xSrc, INT ySrc,
				   INT widthSrc, INT heightSrc, DWORD rop );
extern INT      EMFDRV_StretchDIBits( DC *dc, INT xDst, INT yDst, INT widthDst,
				      INT heightDst, INT xSrc, INT ySrc,
				      INT widthSrc, INT heightSrc,
				      const void *bits, const BITMAPINFO *info,
				      UINT wUsage, DWORD dwRop );
extern BOOL     EMFDRV_StrokeAndFillPath( DC *dc );
extern BOOL     EMFDRV_StrokePath( DC *dc );
extern BOOL     EMFDRV_WidenPath( DC *dc );


#endif  /* __WINE_METAFILEDRV_H */

