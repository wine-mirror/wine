/*
 * Metafile driver definitions
 */

#ifndef __WINE_METAFILEDRV_H
#define __WINE_METAFILEDRV_H

#include "windows.h"
#include "gdi.h"

/* FIXME: SDK docs says these should be 1 and 2 */
/*  DR 980322: most wmf's have 1, so I think 0 and 1 is correct */
#define METAFILE_MEMORY 0
#define METAFILE_DISK   1

/* Metafile driver physical DC */

typedef struct
{
    METAHEADER  *mh;           /* Pointer to metafile header */
    UINT32       nextHandle;   /* Next handle number */
} METAFILEDRV_PDEVICE;


/* Metafile driver functions */

extern BOOL32 MFDRV_BitBlt( struct tagDC *dcDst, INT32 xDst, INT32 yDst,
                            INT32 width, INT32 height, struct tagDC *dcSrc,
                            INT32 xSrc, INT32 ySrc, DWORD rop );
extern BOOL32 MFDRV_PatBlt( struct tagDC *dc, INT32 left, INT32 top,
                            INT32 width, INT32 height, DWORD rop );
extern BOOL32 MFDRV_StretchBlt( struct tagDC *dcDst, INT32 xDst, INT32 yDst,
                                INT32 widthDst, INT32 heightDst,
                                struct tagDC *dcSrc, INT32 xSrc, INT32 ySrc,
                                INT32 widthSrc, INT32 heightSrc, DWORD rop );
extern INT32 MFDRV_SetMapMode( struct tagDC *dc, INT32 mode );
extern BOOL32 MFDRV_SetViewportExt( struct tagDC *dc, INT32 x, INT32 y );
extern BOOL32 MFDRV_SetViewportOrg( struct tagDC *dc, INT32 x, INT32 y );
extern BOOL32 MFDRV_SetWindowExt( struct tagDC *dc, INT32 x, INT32 y );
extern BOOL32 MFDRV_SetWindowOrg( struct tagDC *dc, INT32 x, INT32 y );
extern BOOL32 MFDRV_OffsetViewportOrg( struct tagDC *dc, INT32 x, INT32 y );
extern BOOL32 MFDRV_OffsetWindowOrg( struct tagDC *dc, INT32 x, INT32 y );
extern BOOL32 MFDRV_ScaleViewportExt( struct tagDC *dc, INT32 xNum,
                                      INT32 xDenom, INT32 yNum, INT32 yDenom );
extern BOOL32 MFDRV_ScaleWindowExt( struct tagDC *dc, INT32 xNum, INT32 xDenom,
                                    INT32 yNum, INT32 yDenom );
extern BOOL32 MFDRV_MoveToEx(struct tagDC *dc, INT32 x, INT32 y, LPPOINT32 pt);
extern BOOL32 MFDRV_LineTo( struct tagDC *dc, INT32 x, INT32 y );
extern BOOL32 MFDRV_Arc( struct tagDC *dc, INT32 left, INT32 top, INT32 right,
			 INT32 bottom, INT32 xstart, INT32 ystart, INT32 xend,
			 INT32 yend );
extern BOOL32 MFDRV_Pie( struct tagDC *dc, INT32 left, INT32 top, INT32 right,
			 INT32 bottom, INT32 xstart, INT32 ystart, INT32 xend,
			 INT32 yend );
extern BOOL32 MFDRV_Chord( struct tagDC *dc, INT32 left, INT32 top, INT32 right,
			   INT32 bottom, INT32 xstart, INT32 ystart, INT32 xend,
			   INT32 yend );
extern BOOL32 MFDRV_Ellipse( struct tagDC *dc, INT32 left, INT32 top,
			     INT32 right, INT32 bottom );
extern BOOL32 MFDRV_Rectangle( struct tagDC *dc, INT32 left, INT32 top,
			       INT32 right, INT32 bottom);
extern BOOL32 MFDRV_RoundRect( struct tagDC *dc, INT32 left, INT32 top,
			       INT32 right, INT32 bottom, INT32 ell_width,
			       INT32 ell_height );
extern COLORREF MFDRV_SetPixel( struct tagDC *dc, INT32 x, INT32 y, COLORREF color );
extern BOOL32 MFDRV_Polyline( struct tagDC *dc, const POINT32* pt,INT32 count);
extern BOOL32 MFDRV_Polygon( struct tagDC *dc, const POINT32* pt, INT32 count );
extern BOOL32 MFDRV_PolyPolygon( struct tagDC *dc, const POINT32* pt, const INT32* counts,
				 UINT32 polygons);
extern HGDIOBJ32 MFDRV_SelectObject( DC *dc, HGDIOBJ32 handle );
extern COLORREF MFDRV_SetBkColor( DC *dc, COLORREF color );
extern COLORREF MFDRV_SetTextColor( DC *dc, COLORREF color );
extern BOOL32 MFDRV_ExtFloodFill( struct tagDC *dc, INT32 x, INT32 y,
				  COLORREF color, UINT32 fillType );
extern BOOL32 MFDRV_ExtTextOut( struct tagDC *dc, INT32 x, INT32 y,
				UINT32 flags, const RECT32 *lprect, LPCSTR str,
				UINT32 count, const INT32 *lpDx );
extern BOOL32 MFDRV_PaintRgn( DC *dc, HRGN32 hrgn );

#endif  /* __WINE_METAFILEDRV_H */
