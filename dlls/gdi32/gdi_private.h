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
#include <stdlib.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/gdi_driver.h"

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

struct gdi_obj_funcs
{
    HGDIOBJ (*pSelectObject)( HGDIOBJ handle, HDC hdc );
    INT     (*pGetObjectA)( HGDIOBJ handle, INT count, LPVOID buffer );
    INT     (*pGetObjectW)( HGDIOBJ handle, INT count, LPVOID buffer );
    BOOL    (*pUnrealizeObject)( HGDIOBJ handle );
    BOOL    (*pDeleteObject)( HGDIOBJ handle );
};

struct hdc_list
{
    HDC hdc;
    struct hdc_list *next;
};

typedef struct tagGDIOBJHDR
{
    WORD        type;         /* object type (one of the OBJ_* constants) */
    WORD        system : 1;   /* system object flag */
    WORD        deleted : 1;  /* whether DeleteObject has been called on this object */
    DWORD       selcount;     /* number of times the object is selected in a DC */
    const struct gdi_obj_funcs *funcs;
    struct hdc_list *hdcs;
} GDIOBJHDR;

typedef struct gdi_dc_funcs DC_FUNCTIONS;

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

typedef struct tagDC
{
    GDIOBJHDR    header;
    HDC          hSelf;            /* Handle to this DC */
    struct gdi_physdev nulldrv;    /* physdev for the null driver */
    PHYSDEV      dibdrv;           /* physdev for the dib driver */
    PHYSDEV      physDev;          /* current top of the physdev stack */
    DWORD        thread;           /* thread owning the DC */
    LONG         refcount;         /* thread refcount */
    LONG         dirty;            /* dirty flag */
    INT          saveLevel;
    struct tagDC *saved_dc;
    DWORD_PTR    dwHookData;
    DCHOOKPROC   hookProc;         /* DC hook */

    INT          wndOrgX;          /* Window origin */
    INT          wndOrgY;
    INT          wndExtX;          /* Window extent */
    INT          wndExtY;
    INT          vportOrgX;        /* Viewport origin */
    INT          vportOrgY;
    INT          vportExtX;        /* Viewport extent */
    INT          vportExtY;
    SIZE         virtual_res;      /* Initially HORZRES,VERTRES. Changed by SetVirtualResolution */
    SIZE         virtual_size;     /* Initially HORZSIZE,VERTSIZE. Changed by SetVirtualResolution */
    RECT         vis_rect;         /* visible rectangle in screen coords */
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

    UINT          font_code_page;
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

    DWORD         mapperFlags;       /* Font mapper flags */
    WORD          textAlign;         /* Text alignment from SetTextAlign() */
    INT           charExtra;         /* Spacing from SetTextCharacterExtra() */
    INT           breakExtra;        /* breakTotalExtra / breakCount */
    INT           breakRem;          /* breakTotalExtra % breakCount */
    INT           MapMode;
    INT           GraphicsMode;      /* Graphics mode */
    ABORTPROC     pAbortProc;        /* AbortProc for Printing */
    INT           CursPosX;          /* Current position */
    INT           CursPosY;
    INT           ArcDirection;
    XFORM         xformWorld2Wnd;    /* World-to-window transformation */
    XFORM         xformWorld2Vport;  /* World-to-viewport transformation */
    XFORM         xformVport2World;  /* Inverse of the above transformation */
    BOOL          vport2WorldValid;  /* Is xformVport2World valid? */
    RECT          BoundsRect;        /* Current bounding rect */
} DC;

  /* DC flags */
#define DC_BOUNDS_ENABLE 0x0008   /* Bounding rectangle tracking is enabled */

/* Certain functions will do no further processing if the driver returns this.
   Used by mfdrv for example. */
#define GDI_NO_MORE_WORK 2

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static inline INT GDI_ROUND(double val)
{
   return (int)floor(val + 0.5);
}

#define GET_DC_PHYSDEV(dc,func) \
    get_physdev_entry_point( (dc)->physDev, FIELD_OFFSET(struct gdi_dc_funcs,func))

/* bitmap object */

typedef struct tagBITMAPOBJ
{
    GDIOBJHDR           header;
    BITMAP              bitmap;
    SIZE                size;   /* For SetBitmapDimension() */
    const DC_FUNCTIONS *funcs; /* DC function table */
    /* For device-independent bitmaps: */
    DIBSECTION         *dib;
    RGBQUAD            *color_table;  /* DIB color table if <= 8bpp */
    UINT                nb_colors;    /* number of colors in table */
} BITMAPOBJ;

/* bidi.c */

/* Wine_GCPW Flags */
/* Directionality -
 * LOOSE means taking the directionality of the first strong character, if there is found one.
 * FORCE means the paragraph direction is forced. (RLE/LRE)
 */
#define WINE_GCPW_FORCE_LTR 0
#define WINE_GCPW_FORCE_RTL 1
#define WINE_GCPW_LOOSE_LTR 2
#define WINE_GCPW_LOOSE_RTL 3
#define WINE_GCPW_DIR_MASK 3
#define WINE_GCPW_LOOSE_MASK 2

extern BOOL BIDI_Reorder( HDC hDC, LPCWSTR lpString, INT uCount, DWORD dwFlags, DWORD dwWineGCP_Flags,
                          LPWSTR lpOutString, INT uCountOut, UINT *lpOrder, WORD **lpGlyphs, INT* cGlyphs ) DECLSPEC_HIDDEN;

/* bitmap.c */
extern void get_ddb_bitmapinfo( BITMAPOBJ *bmp, BITMAPINFO *info ) DECLSPEC_HIDDEN;
extern BOOL get_bitmap_image( HBITMAP hbitmap, BITMAPINFO *info, struct gdi_image_bits *bits ) DECLSPEC_HIDDEN;
extern HBITMAP BITMAP_CopyBitmap( HBITMAP hbitmap ) DECLSPEC_HIDDEN;
extern BOOL BITMAP_SetOwnerDC( HBITMAP hbitmap, PHYSDEV physdev ) DECLSPEC_HIDDEN;

/* clipping.c */
extern int get_clip_box( DC *dc, RECT *rect ) DECLSPEC_HIDDEN;
extern void CLIPPING_UpdateGCRegion( DC * dc ) DECLSPEC_HIDDEN;

/* Return the total clip region (if any) */
static inline HRGN get_clip_region( DC * dc )
{
    if (dc->hMetaClipRgn) return dc->hMetaClipRgn;
    if (dc->hMetaRgn) return dc->hMetaRgn;
    return dc->hClipRgn;
}

/* dc.c */
extern DC *alloc_dc_ptr( WORD magic ) DECLSPEC_HIDDEN;
extern void free_dc_ptr( DC *dc ) DECLSPEC_HIDDEN;
extern DC *get_dc_ptr( HDC hdc ) DECLSPEC_HIDDEN;
extern void release_dc_ptr( DC *dc ) DECLSPEC_HIDDEN;
extern void update_dc( DC *dc ) DECLSPEC_HIDDEN;
extern void DC_InitDC( DC * dc ) DECLSPEC_HIDDEN;
extern void DC_UpdateXforms( DC * dc ) DECLSPEC_HIDDEN;

/* dib.c */
extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse ) DECLSPEC_HIDDEN;
extern DWORD convert_bitmapinfo( const BITMAPINFO *src_info, void *src_bits, struct bitblt_coords *src,
                                 const BITMAPINFO *dst_info, void *dst_bits, BOOL add_alpha ) DECLSPEC_HIDDEN;

extern DWORD stretch_bitmapinfo( const BITMAPINFO *src_info, void *src_bits, struct bitblt_coords *src,
                                 const BITMAPINFO *dst_info, void *dst_bits, struct bitblt_coords *dst,
                                 INT mode ) DECLSPEC_HIDDEN;
extern DWORD blend_bitmapinfo( const BITMAPINFO *src_info, void *src_bits, struct bitblt_coords *src,
                               const BITMAPINFO *dst_info, void *dst_bits, struct bitblt_coords *dst,
                               BLENDFUNCTION blend ) DECLSPEC_HIDDEN;

/* driver.c */
extern const DC_FUNCTIONS null_driver DECLSPEC_HIDDEN;
extern const DC_FUNCTIONS dib_driver DECLSPEC_HIDDEN;
extern const DC_FUNCTIONS *DRIVER_load_driver( LPCWSTR name ) DECLSPEC_HIDDEN;
extern BOOL DRIVER_GetDriverName( LPCWSTR device, LPWSTR driver, DWORD size ) DECLSPEC_HIDDEN;

/* enhmetafile.c */
extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, BOOL on_disk ) DECLSPEC_HIDDEN;

/* freetype.c */

/* Undocumented structure filled in by GdiRealizationInfo */
typedef struct
{
    DWORD flags;       /* 1 for bitmap fonts, 3 for scalable fonts */
    DWORD cache_num;   /* keeps incrementing - num of fonts that have been created allowing for caching?? */
    DWORD unknown2;    /* fixed for a given font - looks like it could be the order of the face in the font list or the order
                          in which the face was first rendered. */
} realization_info_t;


extern INT WineEngAddFontResourceEx(LPCWSTR, DWORD, PVOID) DECLSPEC_HIDDEN;
extern HANDLE WineEngAddFontMemResourceEx(PVOID, DWORD, PVOID, LPDWORD) DECLSPEC_HIDDEN;
extern GdiFont* WineEngCreateFontInstance(DC*, HFONT) DECLSPEC_HIDDEN;
extern BOOL WineEngDestroyFontInstance(HFONT handle) DECLSPEC_HIDDEN;
extern DWORD WineEngEnumFonts(LPLOGFONTW, FONTENUMPROCW, LPARAM) DECLSPEC_HIDDEN;
extern BOOL WineEngGetCharABCWidths(GdiFont *font, UINT firstChar,
                                    UINT lastChar, LPABC buffer) DECLSPEC_HIDDEN;
extern BOOL WineEngGetCharABCWidthsFloat(GdiFont *font, UINT firstChar,
                                         UINT lastChar, LPABCFLOAT buffer) DECLSPEC_HIDDEN;
extern BOOL WineEngGetCharABCWidthsI(GdiFont *font, UINT firstChar,
                                    UINT count, LPWORD pgi, LPABC buffer) DECLSPEC_HIDDEN;
extern BOOL WineEngGetCharWidth(GdiFont*, UINT, UINT, LPINT) DECLSPEC_HIDDEN;
extern DWORD WineEngGetFontData(GdiFont*, DWORD, DWORD, LPVOID, DWORD) DECLSPEC_HIDDEN;
extern DWORD WineEngGetFontUnicodeRanges(GdiFont *, LPGLYPHSET) DECLSPEC_HIDDEN;
extern DWORD WineEngGetGlyphIndices(GdiFont *font, LPCWSTR lpstr, INT count,
                                    LPWORD pgi, DWORD flags) DECLSPEC_HIDDEN;
extern DWORD WineEngGetGlyphOutline(GdiFont*, UINT glyph, UINT format,
                                    LPGLYPHMETRICS, DWORD buflen, LPVOID buf,
                                    const MAT2*) DECLSPEC_HIDDEN;
extern DWORD WineEngGetKerningPairs(GdiFont*, DWORD, KERNINGPAIR *) DECLSPEC_HIDDEN;
extern BOOL WineEngGetLinkedHFont(DC *dc, WCHAR c, HFONT *new_hfont, UINT *glyph) DECLSPEC_HIDDEN;
extern UINT WineEngGetOutlineTextMetrics(GdiFont*, UINT, LPOUTLINETEXTMETRICW) DECLSPEC_HIDDEN;
extern UINT WineEngGetTextCharsetInfo(GdiFont *font, LPFONTSIGNATURE fs, DWORD flags) DECLSPEC_HIDDEN;
extern BOOL WineEngGetTextExtentExPoint(GdiFont*, LPCWSTR, INT, INT, LPINT, LPINT, LPSIZE) DECLSPEC_HIDDEN;
extern BOOL WineEngGetTextExtentExPointI(GdiFont*, const WORD *, INT, INT, LPINT, LPINT, LPSIZE) DECLSPEC_HIDDEN;
extern INT  WineEngGetTextFace(GdiFont*, INT, LPWSTR) DECLSPEC_HIDDEN;
extern BOOL WineEngGetTextMetrics(GdiFont*, LPTEXTMETRICW) DECLSPEC_HIDDEN;
extern BOOL WineEngFontIsLinked(GdiFont*) DECLSPEC_HIDDEN;
extern BOOL WineEngInit(void) DECLSPEC_HIDDEN;
extern BOOL WineEngRealizationInfo(GdiFont*, realization_info_t*) DECLSPEC_HIDDEN;
extern BOOL WineEngRemoveFontResourceEx(LPCWSTR, DWORD, PVOID) DECLSPEC_HIDDEN;

/* gdiobj.c */
extern HGDIOBJ alloc_gdi_handle( GDIOBJHDR *obj, WORD type, const struct gdi_obj_funcs *funcs ) DECLSPEC_HIDDEN;
extern void *free_gdi_handle( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern void *GDI_GetObjPtr( HGDIOBJ, WORD ) DECLSPEC_HIDDEN;
extern void GDI_ReleaseObj( HGDIOBJ ) DECLSPEC_HIDDEN;
extern void GDI_CheckNotLock(void) DECLSPEC_HIDDEN;
extern HGDIOBJ GDI_inc_ref_count( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern BOOL GDI_dec_ref_count( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern BOOL GDI_hdc_using_object(HGDIOBJ obj, HDC hdc) DECLSPEC_HIDDEN;
extern BOOL GDI_hdc_not_using_object(HGDIOBJ obj, HDC hdc) DECLSPEC_HIDDEN;

/* metafile.c */
extern HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh) DECLSPEC_HIDDEN;
extern METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mr, LPCVOID filename, BOOL unicode ) DECLSPEC_HIDDEN;

/* path.c */

#define PATH_IsPathOpen(path) ((path).state==PATH_Open)
/* Returns TRUE if the specified path is in the open state, i.e. in the
 * state where points will be added to the path, or FALSE otherwise. This
 * function is implemented as a macro for performance reasons.
 */

extern void PATH_InitGdiPath(GdiPath *pPath) DECLSPEC_HIDDEN;
extern void PATH_DestroyGdiPath(GdiPath *pPath) DECLSPEC_HIDDEN;
extern BOOL PATH_AssignGdiPath(GdiPath *pPathDest, const GdiPath *pPathSrc) DECLSPEC_HIDDEN;

extern BOOL PATH_MoveTo(DC *dc) DECLSPEC_HIDDEN;
extern BOOL PATH_LineTo(DC *dc, INT x, INT y) DECLSPEC_HIDDEN;
extern BOOL PATH_Rectangle(DC *dc, INT x1, INT y1, INT x2, INT y2) DECLSPEC_HIDDEN;
extern BOOL PATH_ExtTextOut(DC *dc, INT x, INT y, UINT flags, const RECT *lprc,
                            LPCWSTR str, UINT count, const INT *dx) DECLSPEC_HIDDEN;
extern BOOL PATH_Ellipse(DC *dc, INT x1, INT y1, INT x2, INT y2) DECLSPEC_HIDDEN;
extern BOOL PATH_Arc(DC *dc, INT x1, INT y1, INT x2, INT y2,
                     INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines) DECLSPEC_HIDDEN;
extern BOOL PATH_PolyBezierTo(DC *dc, const POINT *pt, DWORD cbCount) DECLSPEC_HIDDEN;
extern BOOL PATH_PolyBezier(DC *dc, const POINT *pt, DWORD cbCount) DECLSPEC_HIDDEN;
extern BOOL PATH_PolyDraw(DC *dc, const POINT *pts, const BYTE *types, DWORD cbCount) DECLSPEC_HIDDEN;
extern BOOL PATH_PolylineTo(DC *dc, const POINT *pt, DWORD cbCount) DECLSPEC_HIDDEN;
extern BOOL PATH_Polyline(DC *dc, const POINT *pt, DWORD cbCount) DECLSPEC_HIDDEN;
extern BOOL PATH_Polygon(DC *dc, const POINT *pt, DWORD cbCount) DECLSPEC_HIDDEN;
extern BOOL PATH_PolyPolyline(DC *dc, const POINT *pt, const DWORD *counts, DWORD polylines) DECLSPEC_HIDDEN;
extern BOOL PATH_PolyPolygon(DC *dc, const POINT *pt, const INT *counts, UINT polygons) DECLSPEC_HIDDEN;
extern BOOL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height) DECLSPEC_HIDDEN;

/* painting.c */
extern POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut ) DECLSPEC_HIDDEN;

/* palette.c */
extern HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg) DECLSPEC_HIDDEN;
extern UINT WINAPI GDIRealizePalette( HDC hdc ) DECLSPEC_HIDDEN;
extern HPALETTE PALETTE_Init(void) DECLSPEC_HIDDEN;

/* region.c */
extern INT mirror_region( HRGN dst, HRGN src, INT width ) DECLSPEC_HIDDEN;
extern BOOL REGION_FrameRgn( HRGN dest, HRGN src, INT x, INT y ) DECLSPEC_HIDDEN;

typedef struct
{
    INT size;
    INT numRects;
    RECT *rects;
    RECT extents;
} WINEREGION;
extern const WINEREGION *get_wine_region(HRGN rgn) DECLSPEC_HIDDEN;
static inline void release_wine_region(HRGN rgn)
{
    GDI_ReleaseObj(rgn);
}

/* null driver entry points */
extern BOOL nulldrv_AbortPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_AlphaBlend( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                PHYSDEV src_dev, struct bitblt_coords *src, BLENDFUNCTION func) DECLSPEC_HIDDEN;
extern BOOL nulldrv_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_BeginPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern DWORD nulldrv_BlendImage( PHYSDEV dev, BITMAPINFO *info, const struct gdi_image_bits *bits,
                                 struct bitblt_coords *src, struct bitblt_coords *dst, BLENDFUNCTION func ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_CloseFigure( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_EndPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT  nulldrv_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern INT  nulldrv_ExtSelectClipRgn( PHYSDEV dev, HRGN rgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_FillRgn( PHYSDEV dev, HRGN rgn, HBRUSH brush ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_FrameRgn( PHYSDEV dev, HRGN rgn, HBRUSH brush, INT width, INT height ) DECLSPEC_HIDDEN;
extern LONG nulldrv_GetBitmapBits( HBITMAP bitmap, void *bits, LONG size ) DECLSPEC_HIDDEN;
extern DWORD nulldrv_GetImage( PHYSDEV dev, HBITMAP hbitmap, BITMAPINFO *info, struct gdi_image_bits *bits, struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern COLORREF nulldrv_GetNearestColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT  nulldrv_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_InvertRgn( PHYSDEV dev, HRGN rgn ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, DWORD mode ) DECLSPEC_HIDDEN;
extern INT  nulldrv_OffsetClipRgn( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_PolyBezier( PHYSDEV dev, const POINT *points, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_PolyBezierTo( PHYSDEV dev, const POINT *points, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_PolyDraw( PHYSDEV dev, const POINT *points, const BYTE *types, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_PolylineTo( PHYSDEV dev, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern DWORD nulldrv_PutImage( PHYSDEV dev, HBITMAP hbitmap, HRGN clip, BITMAPINFO *info,
                               const struct gdi_image_bits *bits, struct bitblt_coords *src,
                               struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_RestoreDC( PHYSDEV dev, INT level ) DECLSPEC_HIDDEN;
extern INT  nulldrv_SaveDC( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_ScaleViewportExtEx( PHYSDEV dev, INT x_num, INT x_denom, INT y_num, INT y_denom, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_ScaleWindowExtEx( PHYSDEV dev, INT x_num, INT x_denom, INT y_num, INT y_denom, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_SelectClipPath( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT nulldrv_SetDIBitsToDevice( PHYSDEV dev, INT x_dst, INT y_dst, DWORD width, DWORD height,
                                      INT x_src, INT y_src, UINT start, UINT lines,
                                      const void *bits, BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
extern INT  nulldrv_SetMapMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_SetViewportExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_SetWindowExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_SetWorldTransform( PHYSDEV dev, const XFORM *xform ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;
extern INT  nulldrv_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                   INT xSrc, INT ySrc, INT widthSrc, INT heightSrc, const void *bits,
                                   BITMAPINFO *info, UINT coloruse, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL nulldrv_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;

static inline DC *get_nulldrv_dc( PHYSDEV dev )
{
    return CONTAINING_RECORD( dev, DC, nulldrv );
}

/* Undocumented value for DIB's iUsage: Indicates a mono DIB w/o pal entries */
#define DIB_PAL_MONO 2

BOOL WINAPI FontIsLinked(HDC);

BOOL WINAPI SetVirtualResolution(HDC hdc, DWORD horz_res, DWORD vert_res, DWORD horz_size, DWORD vert_size);

static inline BOOL is_rect_empty( const RECT *rect )
{
    return (rect->left >= rect->right || rect->top >= rect->bottom);
}

static inline BOOL intersect_rect( RECT *dst, const RECT *src1, const RECT *src2 )
{
    dst->left   = max( src1->left, src2->left );
    dst->top    = max( src1->top, src2->top );
    dst->right  = min( src1->right, src2->right );
    dst->bottom = min( src1->bottom, src2->bottom );
    return !is_rect_empty( dst );
}

static inline void offset_rect( RECT *rect, int offset_x, int offset_y )
{
    rect->left   += offset_x;
    rect->top    += offset_y;
    rect->right  += offset_x;
    rect->bottom += offset_y;
}

static inline void get_bounding_rect( RECT *rect, int x, int y, int width, int height )
{
    rect->left   = x;
    rect->right  = x + width;
    rect->top    = y;
    rect->bottom = y + height;
    if (rect->left > rect->right)
    {
        int tmp = rect->left;
        rect->left = rect->right + 1;
        rect->right = tmp + 1;
    }
    if (rect->top > rect->bottom)
    {
        int tmp = rect->top;
        rect->top = rect->bottom + 1;
        rect->bottom = tmp + 1;
    }
}

static inline int get_bitmap_stride( int width, int bpp )
{
    return ((width * bpp + 15) >> 3) & ~1;
}

static inline int get_dib_stride( int width, int bpp )
{
    return ((width * bpp + 31) >> 3) & ~3;
}

static inline int get_dib_image_size( const BITMAPINFO *info )
{
    return get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount )
        * abs( info->bmiHeader.biHeight );
}

static inline int get_dib_num_of_colors( const BITMAPINFO *info )
{
    if (info->bmiHeader.biClrUsed) return min( info->bmiHeader.biClrUsed, 256 );
    return info->bmiHeader.biBitCount > 8 ? 0 : 1 << info->bmiHeader.biBitCount;
}

static inline const struct gdi_dc_funcs *get_bitmap_funcs( const BITMAPOBJ *bitmap )
{
    if( bitmap->dib ) return &dib_driver;
    return bitmap->funcs;
}

extern void free_heap_bits( struct gdi_image_bits *bits ) DECLSPEC_HIDDEN;

#endif /* __WINE_GDI_PRIVATE_H */
