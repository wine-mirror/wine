/*
 * GDI definitions
 *
 * Copyright 1993 Alexandre Julliard
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

#ifndef __WINE_GDI_H
#define __WINE_GDI_H

#include "windef.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "path.h"
#include <math.h>

  /* GDI objects magic numbers */
#define FIRST_MAGIC           0x4f47
#define PEN_MAGIC             0x4f47
#define BRUSH_MAGIC           0x4f48
#define FONT_MAGIC            0x4f49
#define PALETTE_MAGIC         0x4f4a
#define BITMAP_MAGIC          0x4f4b
#define REGION_MAGIC          0x4f4c
#define DC_MAGIC              0x4f4d
#define DISABLED_DC_MAGIC     0x4f4e
#define META_DC_MAGIC         0x4f4f
#define METAFILE_MAGIC        0x4f50
#define METAFILE_DC_MAGIC     0x4f51
#define ENHMETAFILE_MAGIC     0x4f52
#define ENHMETAFILE_DC_MAGIC  0x4f53
#define LAST_MAGIC            0x4f53

#define MAGIC_DONTCARE	      0xffff

/* GDI constants for making objects private/system (naming undoc. !) */
#define OBJECT_PRIVATE        0x2000
#define OBJECT_NOSYSTEM       0x8000

#define GDIMAGIC(magic) ((magic) & ~(OBJECT_PRIVATE|OBJECT_NOSYSTEM))

typedef struct tagGDIOBJHDR
{
    HANDLE16    hNext;
    WORD        wMagic;
    DWORD       dwCount;
} GDIOBJHDR;


typedef struct tagGdiFont *GdiFont;

typedef struct { int opaque; } *PHYSDEV;  /* PHYSDEV is an opaque pointer */

typedef struct tagDC
{
    GDIOBJHDR    header;
    HDC          hSelf;            /* Handle to this DC */
    const struct tagDC_FUNCS *funcs; /* DC function table */
    PHYSDEV      physDev;         /* Physical device (driver-specific) */
    INT          saveLevel;
    DWORD        dwHookData;
    FARPROC16    hookProc;         /* the original SEGPTR ... */
    DCHOOKPROC   hookThunk;        /* ... and the thunk to call it */

    INT          wndOrgX;          /* Window origin */
    INT          wndOrgY;
    INT          wndExtX;          /* Window extent */
    INT          wndExtY;
    INT          vportOrgX;        /* Viewport origin */
    INT          vportOrgY;
    INT          vportExtX;        /* Viewport extent */
    INT          vportExtY;

    int           flags;
    HRGN16        hClipRgn;     /* Clip region (may be 0) */
    HRGN16        hVisRgn;      /* Visible region (must never be 0) */
    HRGN16        hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
    HPEN16        hPen;
    HBRUSH16      hBrush;
    HFONT16       hFont;
    HBITMAP16     hBitmap;
    HANDLE16      hDevice;
    HPALETTE16    hPalette;

    GdiFont       gdiFont;
    GdiPath       path;

    WORD          ROPmode;
    WORD          polyFillMode;
    WORD          stretchBltMode;
    WORD          relAbsMode;
    WORD          backgroundMode;
    COLORREF      backgroundColor;
    COLORREF      textColor;
    short         brushOrgX;
    short         brushOrgY;

    WORD          textAlign;         /* Text alignment from SetTextAlign() */
    short         charExtra;         /* Spacing from SetTextCharacterExtra() */
    short         breakTotalExtra;   /* Total extra space for justification */
    short         breakCount;        /* Break char. count */
    short         breakExtra;        /* breakTotalExtra / breakCount */
    short         breakRem;          /* breakTotalExtra % breakCount */

    RECT          totalExtent;
    BYTE          bitsPerPixel;

    INT           MapMode;
    INT           GraphicsMode;      /* Graphics mode */
    INT           DCOrgX;            /* DC origin */
    INT           DCOrgY;
    ABORTPROC     pAbortProc;        /* AbortProc for Printing */
    ABORTPROC16   pAbortProc16;
    INT           CursPosX;          /* Current position */
    INT           CursPosY;
    INT           ArcDirection;
    XFORM         xformWorld2Wnd;    /* World-to-window transformation */
    XFORM         xformWorld2Vport;  /* World-to-viewport transformation */
    XFORM         xformVport2World;  /* Inverse of the above transformation */
    BOOL          vport2WorldValid;  /* Is xformVport2World valid? */
} DC;

/* Device functions for the Wine driver interface */

typedef INT (*DEVICEFONTENUMPROC)(LPENUMLOGFONTEXW,LPNEWTEXTMETRICEXW,DWORD,
				  LPARAM);

typedef struct tagDC_FUNCS
{
    INT      (*pAbortDoc)(PHYSDEV);
    BOOL     (*pAbortPath)(PHYSDEV);
    BOOL     (*pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    BOOL     (*pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pBeginPath)(PHYSDEV);
    BOOL     (*pBitBlt)(PHYSDEV,INT,INT,INT,INT,PHYSDEV,INT,INT,DWORD);
    LONG     (*pBitmapBits)(HBITMAP,void*,LONG,WORD);
    INT      (*pChoosePixelFormat)(PHYSDEV,const PIXELFORMATDESCRIPTOR *);
    BOOL     (*pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pCloseFigure)(PHYSDEV);
    BOOL     (*pCreateBitmap)(HBITMAP);
    BOOL     (*pCreateDC)(DC *,LPCSTR,LPCSTR,LPCSTR,const DEVMODEA*);
    HBITMAP  (*pCreateDIBSection)(PHYSDEV,BITMAPINFO *,UINT,LPVOID *,HANDLE,DWORD,DWORD);
    BOOL     (*pDeleteDC)(PHYSDEV);
    BOOL     (*pDeleteObject)(HGDIOBJ);
    INT      (*pDescribePixelFormat)(PHYSDEV,INT,UINT,PIXELFORMATDESCRIPTOR *);
    DWORD    (*pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    BOOL     (*pEllipse)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pEndDoc)(PHYSDEV);
    INT      (*pEndPage)(PHYSDEV);
    BOOL     (*pEndPath)(PHYSDEV);
    BOOL     (*pEnumDeviceFonts)(HDC,LPLOGFONTW,DEVICEFONTENUMPROC,LPARAM);
    INT      (*pExcludeClipRect)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);
    INT      (*pExtEscape)(PHYSDEV,INT,INT,LPCVOID,INT,LPVOID);
    BOOL     (*pExtFloodFill)(PHYSDEV,INT,INT,COLORREF,UINT);
    BOOL     (*pExtTextOut)(PHYSDEV,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
    BOOL     (*pFillPath)(PHYSDEV);
    BOOL     (*pFillRgn)(PHYSDEV,HRGN,HBRUSH);
    BOOL     (*pFlattenPath)(PHYSDEV);
    BOOL     (*pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    BOOL     (*pGetCharWidth)(PHYSDEV,UINT,UINT,LPINT);
    BOOL     (*pGetDCOrgEx)(PHYSDEV,LPPOINT);
    UINT     (*pGetDIBColorTable)(PHYSDEV,UINT,UINT,RGBQUAD*);
    INT      (*pGetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPVOID,BITMAPINFO*,UINT);
    INT      (*pGetDeviceCaps)(PHYSDEV,INT);
    BOOL     (*pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    COLORREF (*pGetPixel)(PHYSDEV,INT,INT);
    INT      (*pGetPixelFormat)(PHYSDEV);
    BOOL     (*pGetTextExtentPoint)(PHYSDEV,LPCWSTR,INT,LPSIZE);
    BOOL     (*pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    INT      (*pIntersectClipRect)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (*pInvertRgn)(PHYSDEV,HRGN);
    BOOL     (*pLineTo)(PHYSDEV,INT,INT);
    BOOL     (*pMoveTo)(PHYSDEV,INT,INT);
    INT      (*pOffsetClipRgn)(PHYSDEV,INT,INT);
    BOOL     (*pOffsetViewportOrg)(PHYSDEV,INT,INT);
    BOOL     (*pOffsetWindowOrg)(PHYSDEV,INT,INT);
    BOOL     (*pPaintRgn)(PHYSDEV,HRGN);
    BOOL     (*pPatBlt)(PHYSDEV,INT,INT,INT,INT,DWORD);
    BOOL     (*pPie)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pPolyBezier)(PHYSDEV,const POINT*,DWORD);
    BOOL     (*pPolyBezierTo)(PHYSDEV,const POINT*,DWORD);
    BOOL     (*pPolyDraw)(PHYSDEV,const POINT*,const BYTE *,DWORD);
    BOOL     (*pPolyPolygon)(PHYSDEV,const POINT*,const INT*,UINT);
    BOOL     (*pPolyPolyline)(PHYSDEV,const POINT*,const DWORD*,DWORD);
    BOOL     (*pPolygon)(PHYSDEV,const POINT*,INT);
    BOOL     (*pPolyline)(PHYSDEV,const POINT*,INT);
    BOOL     (*pPolylineTo)(PHYSDEV,const POINT*,INT);
    UINT     (*pRealizePalette)(PHYSDEV);
    BOOL     (*pRectangle)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (*pRestoreDC)(PHYSDEV,INT);
    BOOL     (*pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    INT      (*pSaveDC)(PHYSDEV);
    BOOL     (*pScaleViewportExt)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (*pScaleWindowExt)(PHYSDEV,INT,INT,INT,INT);
    HBITMAP  (*pSelectBitmap)(PHYSDEV,HBITMAP);
    HBRUSH   (*pSelectBrush)(PHYSDEV,HBRUSH);
    BOOL     (*pSelectClipPath)(PHYSDEV,INT);
    INT      (*pSelectClipRgn)(PHYSDEV,HRGN);
    HFONT    (*pSelectFont)(PHYSDEV,HFONT);
    HPALETTE (*pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    HPEN     (*pSelectPen)(PHYSDEV,HPEN);
    COLORREF (*pSetBkColor)(PHYSDEV,COLORREF);
    INT      (*pSetBkMode)(PHYSDEV,INT);
    UINT     (*pSetDIBColorTable)(PHYSDEV,UINT,UINT,const RGBQUAD*);
    INT      (*pSetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    INT      (*pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,
                                   const BITMAPINFO*,UINT);
    VOID     (*pSetDeviceClipping)(PHYSDEV);
    BOOL     (*pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    INT      (*pSetMapMode)(PHYSDEV,INT);
    DWORD    (*pSetMapperFlags)(PHYSDEV,DWORD);
    COLORREF (*pSetPixel)(PHYSDEV,INT,INT,COLORREF);
    BOOL     (*pSetPixelFormat)(PHYSDEV,INT,const PIXELFORMATDESCRIPTOR *);
    INT      (*pSetPolyFillMode)(PHYSDEV,INT);
    INT      (*pSetROP2)(PHYSDEV,INT);
    INT      (*pSetRelAbs)(PHYSDEV,INT);
    INT      (*pSetStretchBltMode)(PHYSDEV,INT);
    UINT     (*pSetTextAlign)(PHYSDEV,UINT);
    INT      (*pSetTextCharacterExtra)(PHYSDEV,INT);
    DWORD    (*pSetTextColor)(PHYSDEV,DWORD);
    INT      (*pSetTextJustification)(PHYSDEV,INT,INT);
    BOOL     (*pSetViewportExt)(PHYSDEV,INT,INT);
    BOOL     (*pSetViewportOrg)(PHYSDEV,INT,INT);
    BOOL     (*pSetWindowExt)(PHYSDEV,INT,INT);
    BOOL     (*pSetWindowOrg)(PHYSDEV,INT,INT);
    INT      (*pStartDoc)(PHYSDEV,const DOCINFOA*);
    INT      (*pStartPage)(PHYSDEV);
    BOOL     (*pStretchBlt)(PHYSDEV,INT,INT,INT,INT,PHYSDEV,INT,INT,INT,INT,DWORD);
    INT      (*pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void *,
                               const BITMAPINFO*,UINT,DWORD);
    BOOL     (*pStrokeAndFillPath)(PHYSDEV);
    BOOL     (*pStrokePath)(PHYSDEV);
    BOOL     (*pSwapBuffers)(PHYSDEV);
    BOOL     (*pWidenPath)(PHYSDEV);
} DC_FUNCTIONS;

  /* DC hook codes */
#define DCHC_INVALIDVISRGN      0x0001
#define DCHC_DELETEDC           0x0002

#define DCHF_INVALIDATEVISRGN   0x0001
#define DCHF_VALIDATEVISRGN     0x0002

  /* DC flags */
#define DC_MEMORY     0x0001   /* It is a memory DC */
#define DC_SAVED      0x0002   /* It is a saved DC */
#define DC_DIRTY      0x0004   /* hVisRgn has to be updated */
#define DC_THUNKHOOK  0x0008   /* DC hook is in the 16-bit code */ 

#define GDI_HEAP_SIZE 0xffe0

/* extra stock object: default 1x1 bitmap for memory DCs */
#define DEFAULT_BITMAP (STOCK_LAST+1)

  /* Device <-> logical coords conversion */

/* A floating point version of the POINT structure */
typedef struct tagFLOAT_POINT
{
   FLOAT x, y;
} FLOAT_POINT;

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static inline INT WINE_UNUSED GDI_ROUND(FLOAT val)
{
   return (int)floor(val + 0.5);
}

/* Performs a viewport-to-world transformation on the specified point (which
 * is in floating point format). Returns TRUE if successful, else FALSE.
 */
static inline BOOL WINE_UNUSED INTERNAL_DPTOLP_FLOAT(DC *dc, FLOAT_POINT *point)
{
    FLOAT x, y;
    
    /* Check that the viewport-to-world transformation is valid */
    if (!dc->vport2WorldValid)
        return FALSE;

    /* Perform the transformation */
    x = point->x;
    y = point->y;
    point->x = x * dc->xformVport2World.eM11 +
               y * dc->xformVport2World.eM21 +
	       dc->xformVport2World.eDx;
    point->y = x * dc->xformVport2World.eM12 +
               y * dc->xformVport2World.eM22 +
	       dc->xformVport2World.eDy;

    return TRUE;
}

/* Performs a viewport-to-world transformation on the specified point (which
 * is in integer format). Returns TRUE if successful, else FALSE.
 */
static inline BOOL WINE_UNUSED INTERNAL_DPTOLP(DC *dc, LPPOINT point)
{
    FLOAT_POINT floatPoint;
    
    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)point->x;
    floatPoint.y=(FLOAT)point->y;
    if (!INTERNAL_DPTOLP_FLOAT(dc, &floatPoint))
        return FALSE;
    
    /* Round to integers */
    point->x = GDI_ROUND(floatPoint.x);
    point->y = GDI_ROUND(floatPoint.y);

    return TRUE;
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in floating point format).
 */
static inline void WINE_UNUSED INTERNAL_LPTODP_FLOAT(DC *dc, FLOAT_POINT *point)
{
    FLOAT x, y;
    
    /* Perform the transformation */
    x = point->x;
    y = point->y;
    point->x = x * dc->xformWorld2Vport.eM11 +
               y * dc->xformWorld2Vport.eM21 +
	       dc->xformWorld2Vport.eDx;
    point->y = x * dc->xformWorld2Vport.eM12 +
               y * dc->xformWorld2Vport.eM22 +
	       dc->xformWorld2Vport.eDy;
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in integer format).
 */
static inline void WINE_UNUSED INTERNAL_LPTODP(DC *dc, LPPOINT point)
{
    FLOAT_POINT floatPoint;
    
    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)point->x;
    floatPoint.y=(FLOAT)point->y;
    INTERNAL_LPTODP_FLOAT(dc, &floatPoint);
    
    /* Round to integers */
    point->x = GDI_ROUND(floatPoint.x);
    point->y = GDI_ROUND(floatPoint.y);
}


/* Performs a world-to-viewport transformation on the specified point (which
 * is in integer format).
 */
static inline INT WINE_UNUSED INTERNAL_XWPTODP(DC *dc, INT x, INT y)
{
    FLOAT_POINT floatPoint;

    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)x;
    floatPoint.y=(FLOAT)y;
    INTERNAL_LPTODP_FLOAT(dc, &floatPoint);

    /* Round to integers */
    return GDI_ROUND(floatPoint.x);
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in integer format).
 */
static inline INT WINE_UNUSED INTERNAL_YWPTODP(DC *dc, INT x, INT y)
{
    FLOAT_POINT floatPoint;

    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)x;
    floatPoint.y=(FLOAT)y;
    INTERNAL_LPTODP_FLOAT(dc, &floatPoint);

    /* Round to integers */
    return GDI_ROUND(floatPoint.y);
}


/* Performs a viewport-to-world transformation on the specified point (which
 * is in integer format).
 */
static inline INT WINE_UNUSED INTERNAL_XDPTOWP(DC *dc, INT x, INT y)
{
    FLOAT_POINT floatPoint;

    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)x;
    floatPoint.y=(FLOAT)y;
    INTERNAL_DPTOLP_FLOAT(dc, &floatPoint);

    /* Round to integers */
    return GDI_ROUND(floatPoint.x);
}

/* Performs a viewport-to-world transformation on the specified point (which
 * is in integer format).
 */
static inline INT WINE_UNUSED INTERNAL_YDPTOWP(DC *dc, INT x, INT y)
{
    FLOAT_POINT floatPoint;

    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)x;
    floatPoint.y=(FLOAT)y;
    INTERNAL_DPTOLP_FLOAT(dc, &floatPoint);

    /* Round to integers */
    return GDI_ROUND(floatPoint.y);
}


#define XDPTOLP(dc,x) \
    (MulDiv(((x)-(dc)->vportOrgX), (dc)->wndExtX, (dc)->vportExtX) + (dc)->wndOrgX)
#define YDPTOLP(dc,y) \
    (MulDiv(((y)-(dc)->vportOrgY), (dc)->wndExtY, (dc)->vportExtY) + (dc)->wndOrgY)
#define XLPTODP(dc,x) \
    (MulDiv(((x)-(dc)->wndOrgX), (dc)->vportExtX, (dc)->wndExtX) + (dc)->vportOrgX)
#define YLPTODP(dc,y) \
    (MulDiv(((y)-(dc)->wndOrgY), (dc)->vportExtY, (dc)->wndExtY) + (dc)->vportOrgY)



  /* World -> Device size conversion */

/* Performs a world-to-viewport transformation on the specified width (which
 * is in floating point format).
 */
static inline void WINE_UNUSED INTERNAL_XWSTODS_FLOAT(DC *dc, FLOAT *width)
{
    /* Perform the transformation */
    *width = *width * dc->xformWorld2Vport.eM11;
}

/* Performs a world-to-viewport transformation on the specified width (which
 * is in integer format).
 */
static inline INT WINE_UNUSED INTERNAL_XWSTODS(DC *dc, INT width)
{
    FLOAT floatWidth;

    /* Perform operation with floating point */
    floatWidth = (FLOAT)width;
    INTERNAL_XWSTODS_FLOAT(dc, &floatWidth);

    /* Round to integers */
    return GDI_ROUND(floatWidth);
}


/* Performs a world-to-viewport transformation on the specified size (which
 * is in floating point format).
 */
static inline void WINE_UNUSED INTERNAL_YWSTODS_FLOAT(DC *dc, FLOAT *height)
{
    /* Perform the transformation */
    *height = *height * dc->xformWorld2Vport.eM22;
}

/* Performs a world-to-viewport transformation on the specified size (which
 * is in integer format).
 */
static inline INT WINE_UNUSED INTERNAL_YWSTODS(DC *dc, INT height)
{
    FLOAT floatHeight;

    /* Perform operation with floating point */
    floatHeight = (FLOAT)height;
    INTERNAL_YWSTODS_FLOAT(dc, &floatHeight);

    /* Round to integers */
    return GDI_ROUND(floatHeight);
}


  /* Device -> World size conversion */

/* Performs a device to world transformation on the specified width (which
 * is in floating point format).
 */
static inline void INTERNAL_XDSTOWS_FLOAT(DC *dc, FLOAT *width)
{
    /* Perform the transformation */
    *width = *width * dc->xformVport2World.eM11;
}

/* Performs a device to world transformation on the specified width (which
 * is in integer format).
 */
static inline INT INTERNAL_XDSTOWS(DC *dc, INT width)
{
    FLOAT floatWidth;

    /* Perform operation with floating point */
    floatWidth = (FLOAT)width;
    INTERNAL_XDSTOWS_FLOAT(dc, &floatWidth);

    /* Round to integers */
    return GDI_ROUND(floatWidth);
}


/* Performs a device to world transformation on the specified size (which
 * is in floating point format).
 */
static inline void INTERNAL_YDSTOWS_FLOAT(DC *dc, FLOAT *height)
{
    /* Perform the transformation */
    *height = *height * dc->xformVport2World.eM22;
}

/* Performs a device to world transformation on the specified size (which
 * is in integer format).
 */
static inline INT INTERNAL_YDSTOWS(DC *dc, INT height)
{
    FLOAT floatHeight;

    /* Perform operation with floating point */
    floatHeight = (FLOAT)height;
    INTERNAL_YDSTOWS_FLOAT(dc, &floatHeight);

    /* Round to integers */
    return GDI_ROUND(floatHeight);
}


  /* Device <-> logical size conversion */

#define XDSTOLS(dc,x) \
    MulDiv((x), (dc)->wndExtX, (dc)->vportExtX)
#define YDSTOLS(dc,y) \
    MulDiv((y), (dc)->wndExtY, (dc)->vportExtY)
#define XLSTODS(dc,x) \
    MulDiv((x), (dc)->vportExtX, (dc)->wndExtX)
#define YLSTODS(dc,y) \
    MulDiv((y), (dc)->vportExtY, (dc)->wndExtY)

  /* GDI local heap */

extern BOOL GDI_Init(void);
extern void *GDI_AllocObject( WORD, WORD, HGDIOBJ * );
extern void *GDI_ReallocObject( WORD, HGDIOBJ, void *obj );
extern BOOL GDI_FreeObject( HGDIOBJ, void *obj );
extern void *GDI_GetObjPtr( HGDIOBJ, WORD );
extern void GDI_ReleaseObj( HGDIOBJ );
extern void GDI_CheckNotLock(void);

extern const DC_FUNCTIONS *DRIVER_load_driver( LPCSTR name );
extern const DC_FUNCTIONS *DRIVER_get_driver( const DC_FUNCTIONS *funcs );
extern void DRIVER_release_driver( const DC_FUNCTIONS *funcs );
extern BOOL DRIVER_GetDriverName( LPCSTR device, LPSTR driver, DWORD size );

extern POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut );

extern DC * DC_AllocDC( const DC_FUNCTIONS *funcs );
extern DC * DC_GetDCPtr( HDC hdc );
extern DC * DC_GetDCUpdate( HDC hdc );
extern void DC_InitDC( DC * dc );
extern void DC_UpdateXforms( DC * dc );


#define CLIP_INTERSECT 0x0001
#define CLIP_EXCLUDE   0x0002
#define CLIP_KEEPRGN   0x0004

/* objects/clipping.c */
INT CLIPPING_IntersectClipRect( DC * dc, INT left, INT top,
                                INT right, INT bottom, UINT flags );
INT CLIPPING_IntersectVisRect( DC * dc, INT left, INT top,
                               INT right, INT bottom, BOOL exclude );
extern void CLIPPING_UpdateGCRegion( DC * dc );

/* objects/enhmetafile.c */
extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, BOOL on_disk );

#define WINE_GGO_GRAY16_BITMAP 0x7f

#endif  /* __WINE_GDI_H */
