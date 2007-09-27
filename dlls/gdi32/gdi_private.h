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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_GDI_PRIVATE_H
#define __WINE_GDI_PRIVATE_H

#include <math.h>
#include "wine/wingdi16.h"

/* Metafile defines */
#define META_EOF 0x0000
/* values of mtType in METAHEADER.  Note however that the disk image of a disk
   based metafile has mtType == 1 */
#define METAFILE_MEMORY 1
#define METAFILE_DISK   2
#define MFHEADERSIZE (sizeof(METAHEADER))
#define MFVERSION 0x300

typedef struct {
    EMR   emr;
    INT   nBreakExtra;
    INT   nBreakCount;
} EMRSETTEXTJUSTIFICATION, *PEMRSETTEXTJUSTIFICATION;

/* extra stock object: default 1x1 bitmap for memory DCs */
#define DEFAULT_BITMAP (STOCK_LAST+1)

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
#define EXT_PEN_MAGIC         0x4f55
#define LAST_MAGIC            0x4f55

#define MAGIC_DONTCARE	      0xffff

/* GDI constants for making objects private/system (naming undoc. !) */
#define OBJECT_PRIVATE        0x2000
#define OBJECT_NOSYSTEM       0x8000

#define GDIMAGIC(magic) ((magic) & ~(OBJECT_PRIVATE|OBJECT_NOSYSTEM))

struct gdi_obj_funcs
{
    HGDIOBJ (*pSelectObject)( HGDIOBJ handle, HDC hdc );
    INT     (*pGetObject16)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    INT     (*pGetObjectA)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    INT     (*pGetObjectW)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    BOOL    (*pUnrealizeObject)( HGDIOBJ handle, void *obj );
    BOOL    (*pDeleteObject)( HGDIOBJ handle, void *obj );
};

struct hdc_list
{
    HDC hdc;
    struct hdc_list *next;
};

typedef struct tagGDIOBJHDR
{
    WORD        wMagic;
    DWORD       dwCount;
    const struct gdi_obj_funcs *funcs;
    struct hdc_list *hdcs;
} GDIOBJHDR;

/* Device functions for the Wine driver interface */

typedef struct { int opaque; } *PHYSDEV;  /* PHYSDEV is an opaque pointer */

typedef struct tagDC_FUNCS
{
    INT      (*pAbortDoc)(PHYSDEV);
    BOOL     (*pAbortPath)(PHYSDEV);
    BOOL     (*pAlphaBlend)(PHYSDEV,INT,INT,INT,INT,PHYSDEV,INT,INT,INT,INT,BLENDFUNCTION);
    BOOL     (*pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    BOOL     (*pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pBeginPath)(PHYSDEV);
    BOOL     (*pBitBlt)(PHYSDEV,INT,INT,INT,INT,PHYSDEV,INT,INT,DWORD);
    INT      (*pChoosePixelFormat)(PHYSDEV,const PIXELFORMATDESCRIPTOR *);
    BOOL     (*pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pCloseFigure)(PHYSDEV);
    BOOL     (*pCreateBitmap)(PHYSDEV,HBITMAP,LPVOID);
    BOOL     (*pCreateDC)(HDC,PHYSDEV *,LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
    HBITMAP  (*pCreateDIBSection)(PHYSDEV,HBITMAP,const BITMAPINFO *,UINT);
    BOOL     (*pDeleteBitmap)(HBITMAP);
    BOOL     (*pDeleteDC)(PHYSDEV);
    BOOL     (*pDeleteObject)(PHYSDEV,HGDIOBJ);
    INT      (*pDescribePixelFormat)(PHYSDEV,INT,UINT,PIXELFORMATDESCRIPTOR *);
    DWORD    (*pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    BOOL     (*pEllipse)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pEndDoc)(PHYSDEV);
    INT      (*pEndPage)(PHYSDEV);
    BOOL     (*pEndPath)(PHYSDEV);
    BOOL     (*pEnumDeviceFonts)(PHYSDEV,LPLOGFONTW,FONTENUMPROCW,LPARAM);
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
    BOOL     (*pGetTextExtentExPoint)(PHYSDEV,LPCWSTR,INT,INT,LPINT,LPINT,LPSIZE);
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
    HDC      (*pResetDC)(PHYSDEV,const DEVMODEW*);
    BOOL     (*pRestoreDC)(PHYSDEV,INT);
    BOOL     (*pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    INT      (*pSaveDC)(PHYSDEV);
    INT      (*pScaleViewportExt)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pScaleWindowExt)(PHYSDEV,INT,INT,INT,INT);
    HBITMAP  (*pSelectBitmap)(PHYSDEV,HBITMAP);
    HBRUSH   (*pSelectBrush)(PHYSDEV,HBRUSH);
    BOOL     (*pSelectClipPath)(PHYSDEV,INT);
    HFONT    (*pSelectFont)(PHYSDEV,HFONT,HANDLE);
    HPALETTE (*pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    HPEN     (*pSelectPen)(PHYSDEV,HPEN);
    INT      (*pSetArcDirection)(PHYSDEV,INT);
    LONG     (*pSetBitmapBits)(HBITMAP,const void*,LONG);
    COLORREF (*pSetBkColor)(PHYSDEV,COLORREF);
    INT      (*pSetBkMode)(PHYSDEV,INT);
    COLORREF (*pSetDCBrushColor)(PHYSDEV, COLORREF);
    DWORD    (*pSetDCOrg)(PHYSDEV,INT,INT);
    COLORREF (*pSetDCPenColor)(PHYSDEV, COLORREF);
    UINT     (*pSetDIBColorTable)(PHYSDEV,UINT,UINT,const RGBQUAD*);
    INT      (*pSetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    INT      (*pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,
                                   const BITMAPINFO*,UINT);
    VOID     (*pSetDeviceClipping)(PHYSDEV,HRGN,HRGN);
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
    INT      (*pStartDoc)(PHYSDEV,const DOCINFOW*);
    INT      (*pStartPage)(PHYSDEV);
    BOOL     (*pStretchBlt)(PHYSDEV,INT,INT,INT,INT,PHYSDEV,INT,INT,INT,INT,DWORD);
    INT      (*pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void *,
                               const BITMAPINFO*,UINT,DWORD);
    BOOL     (*pStrokeAndFillPath)(PHYSDEV);
    BOOL     (*pStrokePath)(PHYSDEV);
    BOOL     (*pSwapBuffers)(PHYSDEV);
    BOOL     (*pUnrealizePalette)(HPALETTE);
    BOOL     (*pWidenPath)(PHYSDEV);

    /* OpenGL32 */
    HGLRC    (*pwglCreateContext)(PHYSDEV);
    BOOL     (*pwglDeleteContext)(HGLRC);
    PROC     (*pwglGetProcAddress)(LPCSTR);
    HDC      (*pwglGetPbufferDCARB)(PHYSDEV, void*);
    BOOL     (*pwglMakeCurrent)(PHYSDEV, HGLRC);
    BOOL     (*pwglMakeContextCurrentARB)(PHYSDEV, PHYSDEV, HGLRC);
    BOOL     (*pwglShareLists)(HGLRC hglrc1, HGLRC hglrc2);
    BOOL     (*pwglUseFontBitmapsA)(PHYSDEV, DWORD, DWORD, DWORD);
    BOOL     (*pwglUseFontBitmapsW)(PHYSDEV, DWORD, DWORD, DWORD);
} DC_FUNCTIONS;

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

typedef struct tagGdiFont GdiFont;

struct saved_visrgn
{
    struct saved_visrgn *next;
    HRGN                 hrgn;
};

typedef struct tagDC
{
    GDIOBJHDR    header;
    HDC          hSelf;            /* Handle to this DC */
    const struct tagDC_FUNCS *funcs; /* DC function table */
    PHYSDEV      physDev;         /* Physical device (driver-specific) */
    DWORD        thread;          /* thread owning the DC */
    LONG         refcount;        /* thread refcount */
    LONG         dirty;           /* dirty flag */
    INT          saveLevel;
    HDC          saved_dc;
    DWORD_PTR    dwHookData;
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
    FLOAT        miterLimit;

    int           flags;
    DWORD         layout;
    HRGN          hClipRgn;      /* Clip region (may be 0) */
    HRGN          hMetaRgn;      /* Meta region (may be 0) */
    HRGN          hMetaClipRgn;  /* Intersection of meta and clip regions (may be 0) */
    HRGN          hVisRgn;       /* Visible region (must never be 0) */
    HPEN          hPen;
    HBRUSH        hBrush;
    HFONT         hFont;
    HBITMAP       hBitmap;
    HANDLE        hDevice;
    HPALETTE      hPalette;

    GdiFont      *gdiFont;
    GdiPath       path;

    WORD          ROPmode;
    WORD          polyFillMode;
    WORD          stretchBltMode;
    WORD          relAbsMode;
    WORD          backgroundMode;
    COLORREF      backgroundColor;
    COLORREF      textColor;
    COLORREF      dcBrushColor;
    COLORREF      dcPenColor;
    short         brushOrgX;
    short         brushOrgY;

    WORD          textAlign;         /* Text alignment from SetTextAlign() */
    INT           charExtra;         /* Spacing from SetTextCharacterExtra() */
    INT           breakExtra;        /* breakTotalExtra / breakCount */
    INT           breakRem;          /* breakTotalExtra % breakCount */
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
    RECT          BoundsRect;        /* Current bounding rect */

    struct saved_visrgn *saved_visrgn;
} DC;

  /* DC flags */
#define DC_SAVED         0x0002   /* It is a saved DC */
#define DC_BOUNDS_ENABLE 0x0008   /* Bounding rectangle tracking is enabled */
#define DC_BOUNDS_SET    0x0010   /* Bounding rectangle has been set */

/* Certain functions will do no further processing if the driver returns this.
   Used by mfdrv for example. */
#define GDI_NO_MORE_WORK 2

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static inline INT GDI_ROUND(FLOAT val)
{
   return (int)floor(val + 0.5);
}

/* bitmap object */

typedef struct tagBITMAPOBJ
{
    GDIOBJHDR           header;
    BITMAP              bitmap;
    SIZE                size;   /* For SetBitmapDimension() */
    const DC_FUNCTIONS *funcs; /* DC function table */
    /* For device-independent bitmaps: */
    DIBSECTION         *dib;
    SEGPTR              segptr_bits;  /* segptr to DIB bits */
    RGBQUAD            *color_table;  /* DIB color table if <= 8bpp */
    UINT                nb_colors;    /* number of colors in table */
} BITMAPOBJ;

/* bidi.c */

/* Wine_GCPW Flags */
/* Directionality -
 * LOOSE means that the paragraph dir is only set if there is no strong character.
 * FORCE means override the characters in the paragraph.
 */
#define WINE_GCPW_FORCE_LTR 0
#define WINE_GCPW_FORCE_RTL 1
#define WINE_GCPW_LOOSE_LTR 2
#define WINE_GCPW_LOOSE_RTL 3
#define WINE_GCPW_DIR_MASK 3
extern BOOL BIDI_Reorder( LPCWSTR lpString, INT uCount, DWORD dwFlags, DWORD dwWineGCP_Flags,
                          LPWSTR lpOutString, INT uCountOut, UINT *lpOrder );

/* bitmap.c */
extern HBITMAP BITMAP_CopyBitmap( HBITMAP hbitmap );
extern BOOL BITMAP_SetOwnerDC( HBITMAP hbitmap, DC *dc );
extern INT BITMAP_GetWidthBytes( INT bmWidth, INT bpp );

/* clipping.c */
extern void CLIPPING_UpdateGCRegion( DC * dc );

/* dc.c */
extern DC *alloc_dc_ptr( const DC_FUNCTIONS *funcs, WORD magic );
extern DC * DC_GetDCUpdate( HDC hdc );
extern DC * DC_GetDCPtr( HDC hdc );
extern void DC_ReleaseDCPtr( DC *dc );
extern BOOL free_dc_ptr( DC *dc );
extern DC *get_dc_ptr( HDC hdc );
extern void release_dc_ptr( DC *dc );
extern void update_dc( DC *dc );
extern void DC_InitDC( DC * dc );
extern void DC_UpdateXforms( DC * dc );

/* dib.c */
extern int DIB_GetDIBWidthBytes( int width, int depth );
extern int DIB_GetDIBImageBytes( int width, int height, int depth );
extern int DIB_BitmapInfoSize( const BITMAPINFO * info, WORD coloruse );

/* driver.c */
extern const DC_FUNCTIONS *DRIVER_load_driver( LPCWSTR name );
extern const DC_FUNCTIONS *DRIVER_get_driver( const DC_FUNCTIONS *funcs );
extern void DRIVER_release_driver( const DC_FUNCTIONS *funcs );
extern BOOL DRIVER_GetDriverName( LPCWSTR device, LPWSTR driver, DWORD size );

/* enhmetafile.c */
extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, BOOL on_disk );

/* freetype.c */
extern INT WineEngAddFontResourceEx(LPCWSTR, DWORD, PVOID);
extern HANDLE WineEngAddFontMemResourceEx(PVOID, DWORD, PVOID, LPDWORD);
extern GdiFont* WineEngCreateFontInstance(DC*, HFONT);
extern BOOL WineEngDestroyFontInstance(HFONT handle);
extern DWORD WineEngEnumFonts(LPLOGFONTW, FONTENUMPROCW, LPARAM);
extern BOOL WineEngGetCharABCWidths(GdiFont *font, UINT firstChar,
                                    UINT lastChar, LPABC buffer);
extern BOOL WineEngGetCharABCWidthsI(GdiFont *font, UINT firstChar,
                                    UINT count, LPWORD pgi, LPABC buffer);
extern BOOL WineEngGetCharWidth(GdiFont*, UINT, UINT, LPINT);
extern DWORD WineEngGetFontData(GdiFont*, DWORD, DWORD, LPVOID, DWORD);
extern DWORD WineEngGetFontUnicodeRanges(GdiFont *, LPGLYPHSET);
extern DWORD WineEngGetGlyphIndices(GdiFont *font, LPCWSTR lpstr, INT count,
                                    LPWORD pgi, DWORD flags);
extern DWORD WineEngGetGlyphOutline(GdiFont*, UINT glyph, UINT format,
                                    LPGLYPHMETRICS, DWORD buflen, LPVOID buf,
                                    const MAT2*);
extern DWORD WineEngGetKerningPairs(GdiFont*, DWORD, KERNINGPAIR *);
extern BOOL WineEngGetLinkedHFont(DC *dc, WCHAR c, HFONT *new_hfont, UINT *glyph);
extern UINT WineEngGetOutlineTextMetrics(GdiFont*, UINT, LPOUTLINETEXTMETRICW);
extern UINT WineEngGetTextCharsetInfo(GdiFont *font, LPFONTSIGNATURE fs, DWORD flags);
extern BOOL WineEngGetTextExtentExPoint(GdiFont*, LPCWSTR, INT, INT, LPINT, LPINT, LPSIZE);
extern BOOL WineEngGetTextExtentPointI(GdiFont*, const WORD *, INT, LPSIZE);
extern INT  WineEngGetTextFace(GdiFont*, INT, LPWSTR);
extern BOOL WineEngGetTextMetrics(GdiFont*, LPTEXTMETRICW);
extern BOOL WineEngFontIsLinked(GdiFont*);
extern BOOL WineEngInit(void);
extern BOOL WineEngRemoveFontResourceEx(LPCWSTR, DWORD, PVOID);

/* gdiobj.c */
extern BOOL GDI_Init(void);
extern void *GDI_AllocObject( WORD, WORD, HGDIOBJ *, const struct gdi_obj_funcs *funcs );
extern void *GDI_ReallocObject( WORD, HGDIOBJ, void *obj );
extern BOOL GDI_FreeObject( HGDIOBJ, void *obj );
extern void *GDI_GetObjPtr( HGDIOBJ, WORD );
extern void GDI_ReleaseObj( HGDIOBJ );
extern void GDI_CheckNotLock(void);
extern BOOL GDI_inc_ref_count( HGDIOBJ handle );
extern BOOL GDI_dec_ref_count( HGDIOBJ handle );
extern BOOL GDI_hdc_using_object(HGDIOBJ obj, HDC hdc);
extern BOOL GDI_hdc_not_using_object(HGDIOBJ obj, HDC hdc);

/* metafile.c */
extern HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh);
extern HMETAFILE16 MF_Create_HMETAFILE16(METAHEADER *mh);
extern METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mr, LPCVOID filename, BOOL unicode );
extern METAHEADER *MF_ReadMetaFile(HANDLE hfile);
extern METAHEADER *MF_LoadDiskBasedMetaFile(METAHEADER *mh);
extern BOOL MF_PlayMetaFile( HDC hdc, METAHEADER *mh);

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
extern BOOL PATH_ExtTextOut(DC *dc, INT x, INT y, UINT flags, const RECT *lprc,
                            LPCWSTR str, UINT count, const INT *dx);
extern BOOL PATH_Ellipse(DC *dc, INT x1, INT y1, INT x2, INT y2);
extern BOOL PATH_Arc(DC *dc, INT x1, INT y1, INT x2, INT y2,
                     INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines);
extern BOOL PATH_PolyBezierTo(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyBezier(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyDraw(DC *dc, const POINT *pts, const BYTE *types, DWORD cbCount);
extern BOOL PATH_PolylineTo(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_Polyline(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_Polygon(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyPolyline(DC *dc, const POINT *pt, const DWORD *counts, DWORD polylines);
extern BOOL PATH_PolyPolygon(DC *dc, const POINT *pt, const INT *counts, UINT polygons);
extern BOOL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height);
extern BOOL PATH_AddEntry(GdiPath *pPath, const POINT *pPoint, BYTE flags);

/* painting.c */
extern POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut );

/* palette.c */
extern HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg);
extern UINT WINAPI GDIRealizePalette( HDC hdc );
extern HPALETTE PALETTE_Init(void);

/* region.c */
extern BOOL REGION_FrameRgn( HRGN dest, HRGN src, INT x, INT y );

/* Undocumented value for DIB's iUsage: Indicates a mono DIB w/o pal entries */
#define DIB_PAL_MONO 2
#endif /* __WINE_GDI_PRIVATE_H */

BOOL WINAPI FontIsLinked(HDC);
