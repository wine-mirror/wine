/*
 * Metafile driver definitions
 */

#ifndef __WINE_METAFILEDRV_H
#define __WINE_METAFILEDRV_H

#include "wingdi.h"
#include "gdi.h"

/* FIXME: SDK docs says these should be 1 and 2 */
/*  DR 980322: most wmf's have 1, so I think 0 and 1 is correct */
#define METAFILE_MEMORY 0
#define METAFILE_DISK   1

/* Metafile driver physical DC */

typedef struct
{
    METAHEADER  *mh;           /* Pointer to metafile header */
    UINT       nextHandle;   /* Next handle number */
} METAFILEDRV_PDEVICE;


/* Metafile driver functions */

extern BOOL MFDRV_BitBlt( struct tagDC *dcDst, INT xDst, INT yDst,
                            INT width, INT height, struct tagDC *dcSrc,
                            INT xSrc, INT ySrc, DWORD rop );
extern BOOL MFDRV_PatBlt( struct tagDC *dc, INT left, INT top,
                            INT width, INT height, DWORD rop );
extern BOOL MFDRV_StretchBlt( struct tagDC *dcDst, INT xDst, INT yDst,
                                INT widthDst, INT heightDst,
                                struct tagDC *dcSrc, INT xSrc, INT ySrc,
                                INT widthSrc, INT heightSrc, DWORD rop );
extern INT MFDRV_SetMapMode( struct tagDC *dc, INT mode );
extern BOOL MFDRV_SetViewportExt( struct tagDC *dc, INT x, INT y );
extern BOOL MFDRV_SetViewportOrg( struct tagDC *dc, INT x, INT y );
extern BOOL MFDRV_SetWindowExt( struct tagDC *dc, INT x, INT y );
extern BOOL MFDRV_SetWindowOrg( struct tagDC *dc, INT x, INT y );
extern BOOL MFDRV_OffsetViewportOrg( struct tagDC *dc, INT x, INT y );
extern BOOL MFDRV_OffsetWindowOrg( struct tagDC *dc, INT x, INT y );
extern BOOL MFDRV_ScaleViewportExt( struct tagDC *dc, INT xNum,
                                      INT xDenom, INT yNum, INT yDenom );
extern BOOL MFDRV_ScaleWindowExt( struct tagDC *dc, INT xNum, INT xDenom,
                                    INT yNum, INT yDenom );
extern BOOL MFDRV_MoveToEx(struct tagDC *dc, INT x, INT y, LPPOINT pt);
extern BOOL MFDRV_LineTo( struct tagDC *dc, INT x, INT y );
extern BOOL MFDRV_Arc( struct tagDC *dc, INT left, INT top, INT right,
			 INT bottom, INT xstart, INT ystart, INT xend,
			 INT yend );
extern BOOL MFDRV_Pie( struct tagDC *dc, INT left, INT top, INT right,
			 INT bottom, INT xstart, INT ystart, INT xend,
			 INT yend );
extern BOOL MFDRV_Chord( struct tagDC *dc, INT left, INT top, INT right,
			   INT bottom, INT xstart, INT ystart, INT xend,
			   INT yend );
extern BOOL MFDRV_Ellipse( struct tagDC *dc, INT left, INT top,
			     INT right, INT bottom );
extern BOOL MFDRV_Rectangle( struct tagDC *dc, INT left, INT top,
			       INT right, INT bottom);
extern BOOL MFDRV_RoundRect( struct tagDC *dc, INT left, INT top,
			       INT right, INT bottom, INT ell_width,
			       INT ell_height );
extern COLORREF MFDRV_SetPixel( struct tagDC *dc, INT x, INT y, COLORREF color );
extern BOOL MFDRV_Polyline( struct tagDC *dc, const POINT* pt,INT count);
extern BOOL MFDRV_Polygon( struct tagDC *dc, const POINT* pt, INT count );
extern BOOL MFDRV_PolyPolygon( struct tagDC *dc, const POINT* pt, const INT* counts,
				 UINT polygons);
extern HGDIOBJ MFDRV_SelectObject( DC *dc, HGDIOBJ handle );
extern COLORREF MFDRV_SetBkColor( DC *dc, COLORREF color );
extern COLORREF MFDRV_SetTextColor( DC *dc, COLORREF color );
extern BOOL MFDRV_ExtFloodFill( struct tagDC *dc, INT x, INT y,
				  COLORREF color, UINT fillType );
extern BOOL MFDRV_ExtTextOut( struct tagDC *dc, INT x, INT y,
				UINT flags, const RECT *lprect, LPCSTR str,
				UINT count, const INT *lpDx );
extern BOOL MFDRV_PaintRgn( DC *dc, HRGN hrgn );

#endif  /* __WINE_METAFILEDRV_H */
