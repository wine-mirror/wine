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
#define MEMORY_DC_MAGIC       0x4f54
#define LAST_MAGIC            0x4f54

#define MAGIC_DONTCARE	      0xffff

/* GDI constants for making objects private/system (naming undoc. !) */
#define OBJECT_PRIVATE        0x2000
#define OBJECT_NOSYSTEM       0x8000

#define GDIMAGIC(magic) ((magic) & ~(OBJECT_PRIVATE|OBJECT_NOSYSTEM))

struct gdi_obj_funcs
{
    HGDIOBJ (*pSelectObject)( HGDIOBJ handle, void *obj, HDC hdc );
    INT     (*pGetObject16)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    INT     (*pGetObjectA)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    INT     (*pGetObjectW)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    BOOL    (*pUnrealizeObject)( HGDIOBJ handle, void *obj );
    BOOL    (*pDeleteObject)( HGDIOBJ handle, void *obj );
};

typedef struct tagGDIOBJHDR
{
    HANDLE16    hNext;
    WORD        wMagic;
    DWORD       dwCount;
    const struct gdi_obj_funcs *funcs;
} GDIOBJHDR;


/* It should not be necessary to access the contents of the GdiPath
 * structure directly; if you find that the exported functions don't
 * allow you to do what you want, then please place a new exported
 * function that does this job in path.c.
 */
typedef enum tagGdiPathState
{
   PATH_Null,
   PATH_Open,
   PATH_Closed
} GdiPathState;

typedef struct tagGdiPath
{
   GdiPathState state;
   POINT      *pPoints;
   BYTE         *pFlags;
   int          numEntriesUsed, numEntriesAllocated;
   BOOL       newStroke;
} GdiPath;

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
    HRGN          hClipRgn;     /* Clip region (may be 0) */
    HRGN          hVisRgn;      /* Visible region (must never be 0) */
    HRGN          hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
    HPEN          hPen;
    HBRUSH        hBrush;
    HFONT         hFont;
    HBITMAP       hBitmap;
    HANDLE        hDevice;
    HPALETTE      hPalette;

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

typedef INT (*DEVICEFONTENUMPROC)(LPENUMLOGFONTEXW,NEWTEXTMETRICEXW *,DWORD,
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
    INT      (*pChoosePixelFormat)(PHYSDEV,const PIXELFORMATDESCRIPTOR *);
    BOOL     (*pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pCloseFigure)(PHYSDEV);
    BOOL     (*pCreateBitmap)(PHYSDEV,HBITMAP);
    BOOL     (*pCreateDC)(DC *,PHYSDEV *,LPCSTR,LPCSTR,LPCSTR,const DEVMODEA*);
    HBITMAP  (*pCreateDIBSection)(PHYSDEV,BITMAPINFO *,UINT,LPVOID *,HANDLE,DWORD,DWORD);
    BOOL     (*pDeleteBitmap)(HBITMAP);
    BOOL     (*pDeleteDC)(PHYSDEV);
    INT      (*pDescribePixelFormat)(PHYSDEV,INT,UINT,PIXELFORMATDESCRIPTOR *);
    DWORD    (*pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    BOOL     (*pEllipse)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pEndDoc)(PHYSDEV);
    INT      (*pEndPage)(PHYSDEV);
    BOOL     (*pEndPath)(PHYSDEV);
    BOOL     (*pEnumDeviceFonts)(PHYSDEV,LPLOGFONTW,DEVICEFONTENUMPROC,LPARAM);
    INT      (*pExcludeClipRect)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);
    INT      (*pExtEscape)(PHYSDEV,INT,INT,LPCVOID,INT,LPVOID);
    BOOL     (*pExtFloodFill)(PHYSDEV,INT,INT,COLORREF,UINT);
    INT      (*pExtSelectClipRgn)(PHYSDEV,HRGN,INT);
    BOOL     (*pExtTextOut)(PHYSDEV,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
    BOOL     (*pFillPath)(PHYSDEV);
    BOOL     (*pFillRgn)(PHYSDEV,HRGN,HBRUSH);
    BOOL     (*pFlattenPath)(PHYSDEV);
    BOOL     (*pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    BOOL     (*pGdiComment)(PHYSDEV,UINT,CONST BYTE*);
    LONG     (*pGetBitmapBits)(HBITMAP,void*,LONG);
    BOOL     (*pGetCharWidth)(PHYSDEV,UINT,UINT,LPINT);
    BOOL     (*pGetDCOrgEx)(PHYSDEV,LPPOINT);
    UINT     (*pGetDIBColorTable)(PHYSDEV,UINT,UINT,RGBQUAD*);
    INT      (*pGetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPVOID,BITMAPINFO*,UINT);
    INT      (*pGetDeviceCaps)(PHYSDEV,INT);
    BOOL     (*pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    COLORREF (*pGetNearestColor)(PHYSDEV,COLORREF);
    COLORREF (*pGetPixel)(PHYSDEV,INT,INT);
    INT      (*pGetPixelFormat)(PHYSDEV);
    UINT     (*pGetSystemPaletteEntries)(PHYSDEV,UINT,UINT,LPPALETTEENTRY);
    BOOL     (*pGetTextExtentPoint)(PHYSDEV,LPCWSTR,INT,LPSIZE);
    BOOL     (*pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    INT      (*pIntersectClipRect)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (*pInvertRgn)(PHYSDEV,HRGN);
    BOOL     (*pLineTo)(PHYSDEV,INT,INT);
    BOOL     (*pModifyWorldTransform)(PHYSDEV,const XFORM*,INT);
    BOOL     (*pMoveTo)(PHYSDEV,INT,INT);
    INT      (*pOffsetClipRgn)(PHYSDEV,INT,INT);
    INT      (*pOffsetViewportOrg)(PHYSDEV,INT,INT);
    INT      (*pOffsetWindowOrg)(PHYSDEV,INT,INT);
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
    UINT     (*pRealizeDefaultPalette)(PHYSDEV);
    UINT     (*pRealizePalette)(PHYSDEV,HPALETTE,BOOL);
    BOOL     (*pRectangle)(PHYSDEV,INT,INT,INT,INT);
    HDC      (*pResetDC)(PHYSDEV,const DEVMODEA*);
    BOOL     (*pRestoreDC)(PHYSDEV,INT);
    BOOL     (*pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    INT      (*pSaveDC)(PHYSDEV);
    INT      (*pScaleViewportExt)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pScaleWindowExt)(PHYSDEV,INT,INT,INT,INT);
    HBITMAP  (*pSelectBitmap)(PHYSDEV,HBITMAP);
    HBRUSH   (*pSelectBrush)(PHYSDEV,HBRUSH);
    BOOL     (*pSelectClipPath)(PHYSDEV,INT);
    HFONT    (*pSelectFont)(PHYSDEV,HFONT);
    HPALETTE (*pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    HPEN     (*pSelectPen)(PHYSDEV,HPEN);
    LONG     (*pSetBitmapBits)(HBITMAP,const void*,LONG);
    COLORREF (*pSetBkColor)(PHYSDEV,COLORREF);
    INT      (*pSetBkMode)(PHYSDEV,INT);
    DWORD    (*pSetDCOrg)(PHYSDEV,INT,INT);
    UINT     (*pSetDIBColorTable)(PHYSDEV,UINT,UINT,const RGBQUAD*);
    INT      (*pSetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    INT      (*pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,
                                   const BITMAPINFO*,UINT);
    VOID     (*pSetDeviceClipping)(PHYSDEV,HRGN);
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
    INT      (*pSetViewportExt)(PHYSDEV,INT,INT);
    INT      (*pSetViewportOrg)(PHYSDEV,INT,INT);
    INT      (*pSetWindowExt)(PHYSDEV,INT,INT);
    INT      (*pSetWindowOrg)(PHYSDEV,INT,INT);
    BOOL     (*pSetWorldTransform)(PHYSDEV,const XFORM*);
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

/* Certain functions will do no further processing if the driver returns this.
   Used by mfdrv for example. */
#define GDI_NO_MORE_WORK 2

  /* DC hook codes */
#define DCHC_INVALIDVISRGN      0x0001
#define DCHC_DELETEDC           0x0002

#define DCHF_INVALIDATEVISRGN   0x0001
#define DCHF_VALIDATEVISRGN     0x0002

  /* DC flags */
#define DC_SAVED      0x0002   /* It is a saved DC */
#define DC_DIRTY      0x0004   /* hVisRgn has to be updated */
#define DC_THUNKHOOK  0x0008   /* DC hook is in the 16-bit code */

#define GDI_HEAP_SIZE 0xffe0

/* extra stock object: default 1x1 bitmap for memory DCs */
#define DEFAULT_BITMAP (STOCK_LAST+1)

/* Metafile defines */

#define META_EOF 0x0000
/* values of mtType in METAHEADER.  Note however that the disk image of a disk
   based metafile has mtType == 1 */
#define METAFILE_MEMORY 1
#define METAFILE_DISK   2

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static inline INT WINE_UNUSED GDI_ROUND(FLOAT val)
{
   return (int)floor(val + 0.5);
}

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
extern void *GDI_AllocObject( WORD, WORD, HGDIOBJ *, const struct gdi_obj_funcs *funcs );
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

extern DC * DC_AllocDC( const DC_FUNCTIONS *funcs, WORD magic );
extern DC * DC_GetDCPtr( HDC hdc );
extern DC * DC_GetDCUpdate( HDC hdc );
extern void DC_InitDC( DC * dc );
extern void DC_UpdateXforms( DC * dc );

/* clipping.c */
extern void CLIPPING_UpdateGCRegion( DC * dc );

/* enhmetafile.c */
extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, BOOL on_disk );

/* freetype.c */
extern INT WineEngAddFontResourceEx(LPCWSTR, DWORD, PVOID);
extern GdiFont WineEngCreateFontInstance(DC*, HFONT);
extern BOOL WineEngDestroyFontInstance(HFONT handle);
extern DWORD WineEngEnumFonts(LPLOGFONTW, DEVICEFONTENUMPROC, LPARAM);
extern BOOL WineEngGetCharWidth(GdiFont, UINT, UINT, LPINT);
extern DWORD WineEngGetFontData(GdiFont, DWORD, DWORD, LPVOID, DWORD);
extern DWORD WineEngGetGlyphIndices(GdiFont font, LPCWSTR lpstr, INT count,
				    LPWORD pgi, DWORD flags);
extern DWORD WineEngGetGlyphOutline(GdiFont, UINT glyph, UINT format,
				    LPGLYPHMETRICS, DWORD buflen, LPVOID buf,
				    const MAT2*);
extern UINT WineEngGetOutlineTextMetrics(GdiFont, UINT, LPOUTLINETEXTMETRICW);
extern UINT WineEngGetTextCharsetInfo(GdiFont font, LPFONTSIGNATURE fs, DWORD flags);
extern BOOL WineEngGetTextExtentPoint(GdiFont, LPCWSTR, INT, LPSIZE);
extern BOOL WineEngGetTextExtentPointI(GdiFont, const WORD *, INT, LPSIZE);
extern INT  WineEngGetTextFace(GdiFont, INT, LPWSTR);
extern BOOL WineEngGetTextMetrics(GdiFont, LPTEXTMETRICW);
extern BOOL WineEngInit(void);
extern BOOL WineEngRemoveFontResourceEx(LPCWSTR, DWORD, PVOID);

/* metafile.c */
extern HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh);
extern HMETAFILE16 MF_Create_HMETAFILE16(METAHEADER *mh);
extern METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mr, LPCSTR filename);

/* region.c */
extern BOOL REGION_FrameRgn( HRGN dest, HRGN src, INT x, INT y );

/* palette.c */
extern HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg);
extern UINT WINAPI GDIRealizePalette( HDC hdc );

/* path.c */

#define PATH_IsPathOpen(path) ((path).state==PATH_Open)
/* Returns TRUE if the specified path is in the open state, i.e. in the
 * state where points will be added to the path, or FALSE otherwise. This
 * function is implemented as a macro for performance reasons.
 */

extern void PATH_InitGdiPath(GdiPath *pPath);
extern void PATH_DestroyGdiPath(GdiPath *pPath);
extern BOOL PATH_AssignGdiPath(GdiPath *pPathDest, const GdiPath *pPathSrc);

extern BOOL PATH_MoveTo(DC *dc);
extern BOOL PATH_LineTo(DC *dc, INT x, INT y);
extern BOOL PATH_Rectangle(DC *dc, INT x1, INT y1, INT x2, INT y2);
extern BOOL PATH_Ellipse(DC *dc, INT x1, INT y1, INT x2, INT y2);
extern BOOL PATH_Arc(DC *dc, INT x1, INT y1, INT x2, INT y2,
                     INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines);
extern BOOL PATH_PolyBezierTo(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyBezier(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolylineTo(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_Polyline(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_Polygon(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyPolyline(DC *dc, const POINT *pt, const DWORD *counts, DWORD polylines);
extern BOOL PATH_PolyPolygon(DC *dc, const POINT *pt, const INT *counts, UINT polygons);
extern BOOL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height);
extern BOOL PATH_AddEntry(GdiPath *pPath, const POINT *pPoint, BYTE flags);

/* text.c */
extern LPWSTR FONT_mbtowc(HDC, LPCSTR, INT, INT*, UINT*);

#define WINE_GGO_GRAY16_BITMAP 0x7f

#endif  /* __WINE_GDI_H */
