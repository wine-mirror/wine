/*
 * X11 display driver definitions
 */

#ifndef __WINE_X11DRV_H
#define __WINE_X11DRV_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "windows.h"

  /* X physical pen */
typedef struct
{
    int          style;
    int          pixel;
    int          width;
    char *       dashes;
    int          dash_len;
} X_PHYSPEN;

  /* X physical brush */
typedef struct
{
    int          style;
    int          fillStyle;
    int          pixel;
    Pixmap       pixmap;
} X_PHYSBRUSH;

  /* X physical font */
typedef struct
{
    XFontStruct * fstruct;
    TEXTMETRIC16  metrics;
} X_PHYSFONT;

  /* X physical device */
typedef struct
{
    GC            gc;          /* X Window GC */
    Drawable      drawable;
    X_PHYSFONT    font;
    X_PHYSPEN     pen;
    X_PHYSBRUSH   brush;
} X11DRV_PDEVICE;

/* Wine driver X11 functions */

struct tagDC;

extern BOOL32 X11DRV_BitBlt( struct tagDC *dcDst, INT32 xDst, INT32 yDst,
                             INT32 width, INT32 height, struct tagDC *dcSrc,
                             INT32 xSrc, INT32 ySrc, DWORD rop );
extern BOOL32 X11DRV_GetCharWidth( struct tagDC *dc, UINT32 firstChar,
                                   UINT32 lastChar, LPINT32 buffer );
extern BOOL32 X11DRV_GetTextExtentPoint( struct tagDC *dc, LPCSTR str,
                                         INT32 count, LPSIZE32 size );
extern BOOL32 X11DRV_GetTextMetrics(struct tagDC *dc, TEXTMETRIC32A *metrics);
extern BOOL32 X11DRV_PatBlt( struct tagDC *dc, INT32 left, INT32 top,
                             INT32 width, INT32 height, DWORD rop );
extern VOID X11DRV_SetDeviceClipping(struct tagDC *dc);
extern BOOL32 X11DRV_StretchBlt( struct tagDC *dcDst, INT32 xDst, INT32 yDst,
                                 INT32 widthDst, INT32 heightDst,
                                 struct tagDC *dcSrc, INT32 xSrc, INT32 ySrc,
                                 INT32 widthSrc, INT32 heightSrc, DWORD rop );
extern BOOL32 X11DRV_MoveToEx( struct tagDC *dc, INT32 x, INT32 y,LPPOINT32 pt);
extern BOOL32 X11DRV_LineTo( struct tagDC *dc, INT32 x, INT32 y);
extern BOOL32 X11DRV_Arc( struct tagDC *dc, INT32 left, INT32 top, INT32 right,
			  INT32 bottom, INT32 xstart, INT32 ystart, INT32 xend,
			  INT32 yend );
extern BOOL32 X11DRV_Pie( struct tagDC *dc, INT32 left, INT32 top, INT32 right,
			  INT32 bottom, INT32 xstart, INT32 ystart, INT32 xend,
			  INT32 yend );
extern BOOL32 X11DRV_Chord( struct tagDC *dc, INT32 left, INT32 top,
			    INT32 right, INT32 bottom, INT32 xstart,
			    INT32 ystart, INT32 xend, INT32 yend );
extern BOOL32 X11DRV_Ellipse( struct tagDC *dc, INT32 left, INT32 top,
			      INT32 right, INT32 bottom );
extern BOOL32 X11DRV_Rectangle(struct tagDC *dc, INT32 left, INT32 top,
			      INT32 right, INT32 bottom);
extern BOOL32 X11DRV_RoundRect( struct tagDC *dc, INT32 left, INT32 top,
				INT32 right, INT32 bottom, INT32 ell_width,
				INT32 ell_height );
extern COLORREF X11DRV_SetPixel( struct tagDC *dc, INT32 x, INT32 y,
				 COLORREF color );
extern COLORREF X11DRV_GetPixel( struct tagDC *dc, INT32 x, INT32 y);
extern BOOL32 X11DRV_PaintRgn( struct tagDC *dc, HRGN32 hrgn );
extern BOOL32 X11DRV_Polyline( struct tagDC *dc,const LPPOINT32 pt,INT32 count);
extern BOOL32 X11DRV_Polygon( struct tagDC *dc, LPPOINT32 pt, INT32 count );
extern BOOL32 X11DRV_PolyPolygon( struct tagDC *dc, LPPOINT32 pt,
				  LPINT32 counts, UINT32 polygons);

extern HGDIOBJ32 X11DRV_SelectObject( struct tagDC *dc, HGDIOBJ32 handle );

extern BOOL32 X11DRV_ExtFloodFill( struct tagDC *dc, INT32 x, INT32 y,
				   COLORREF color, UINT32 fillType );
extern BOOL32 X11DRV_ExtTextOut( struct tagDC *dc, INT32 x, INT32 y,
				 UINT32 flags, const RECT32 *lprect,
				 LPCSTR str, UINT32 count, const INT32 *lpDx );


/* X11 driver internal functions */

extern BOOL32 X11DRV_BITMAP_Init(void);
extern BOOL32 X11DRV_BRUSH_Init(void);
extern BOOL32 X11DRV_FONT_Init(void);

#endif  /* __WINE_X11DRV_H */
