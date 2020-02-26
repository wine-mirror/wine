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

#include <limits.h>
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

typedef struct tagDC
{
    HDC          hSelf;            /* Handle to this DC */
    struct gdi_physdev nulldrv;    /* physdev for the null driver */
    PHYSDEV      physDev;          /* current top of the physdev stack */
    DWORD        thread;           /* thread owning the DC */
    LONG         refcount;         /* thread refcount */
    LONG         dirty;            /* dirty flag */
    LONG         disabled;         /* get_dc_ptr() will return NULL.  Controlled by DCHF_(DISABLE|ENABLE)DC */
    INT          saveLevel;
    struct tagDC *saved_dc;
    DWORD_PTR    dwHookData;
    DCHOOKPROC   hookProc;         /* DC hook */
    BOOL         bounds_enabled:1; /* bounds tracking is enabled */
    BOOL         path_open:1;      /* path is currently open (only for saved DCs) */

    POINT        wnd_org;          /* Window origin */
    SIZE         wnd_ext;          /* Window extent */
    POINT        vport_org;        /* Viewport origin */
    SIZE         vport_ext;        /* Viewport extent */
    SIZE         virtual_res;      /* Initially HORZRES,VERTRES. Changed by SetVirtualResolution */
    SIZE         virtual_size;     /* Initially HORZSIZE,VERTSIZE. Changed by SetVirtualResolution */
    RECT         vis_rect;         /* visible rectangle in screen coords */
    RECT         device_rect;      /* rectangle for the whole device */
    int          pixel_format;     /* pixel format (for memory DCs) */
    UINT         aa_flags;         /* anti-aliasing flags to pass to GetGlyphOutline for current font */
    FLOAT        miterLimit;

    int           flags;
    DWORD         layout;
    HRGN          hClipRgn;      /* Clip region */
    HRGN          hMetaRgn;      /* Meta region */
    HRGN          hVisRgn;       /* Visible region */
    HRGN          region;        /* Total DC region (intersection of clip and visible) */
    HPEN          hPen;
    HBRUSH        hBrush;
    HFONT         hFont;
    HBITMAP       hBitmap;
    HPALETTE      hPalette;

    struct gdi_path *path;

    struct font_gamma_ramp *font_gamma_ramp;

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
    POINT         brush_org;

    DWORD         mapperFlags;       /* Font mapper flags */
    WORD          textAlign;         /* Text alignment from SetTextAlign() */
    INT           charExtra;         /* Spacing from SetTextCharacterExtra() */
    INT           breakExtra;        /* breakTotalExtra / breakCount */
    INT           breakRem;          /* breakTotalExtra % breakCount */
    INT           MapMode;
    INT           GraphicsMode;      /* Graphics mode */
    ABORTPROC     pAbortProc;        /* AbortProc for Printing */
    POINT         cur_pos;           /* Current position */
    INT           ArcDirection;
    XFORM         xformWorld2Wnd;    /* World-to-window transformation */
    XFORM         xformWorld2Vport;  /* World-to-viewport transformation */
    XFORM         xformVport2World;  /* Inverse of the above transformation */
    BOOL          vport2WorldValid;  /* Is xformVport2World valid? */
    RECT          bounds;            /* Current bounding rect */
} DC;

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

static inline PHYSDEV pop_dc_driver( DC *dc, const struct gdi_dc_funcs *funcs )
{
    PHYSDEV dev, *pdev = &dc->physDev;
    while (*pdev && (*pdev)->funcs != funcs) pdev = &(*pdev)->next;
    if (!*pdev) return NULL;
    dev = *pdev;
    *pdev = dev->next;
    return dev;
}

static inline PHYSDEV find_dc_driver( DC *dc, const struct gdi_dc_funcs *funcs )
{
    PHYSDEV dev;

    for (dev = dc->physDev; dev; dev = dev->next) if (dev->funcs == funcs) return dev;
    return NULL;
}

/* bitmap object */

typedef struct tagBITMAPOBJ
{
    DIBSECTION          dib;
    SIZE                size;   /* For SetBitmapDimension() */
    RGBQUAD            *color_table;  /* DIB color table if <= 8bpp (always 1 << bpp in size) */
} BITMAPOBJ;

static inline BOOL is_bitmapobj_dib( const BITMAPOBJ *bmp )
{
    return bmp->dib.dsBmih.biSize != 0;
}

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

/* bitblt.c */
extern DWORD convert_bits( const BITMAPINFO *src_info, struct bitblt_coords *src,
                           BITMAPINFO *dst_info, struct gdi_image_bits *bits ) DECLSPEC_HIDDEN;
extern BOOL intersect_vis_rectangles( struct bitblt_coords *dst, struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern DWORD stretch_bits( const BITMAPINFO *src_info, struct bitblt_coords *src,
                           BITMAPINFO *dst_info, struct bitblt_coords *dst,
                           struct gdi_image_bits *bits, int mode ) DECLSPEC_HIDDEN;
extern void get_mono_dc_colors( DC *dc, int color_table_size, BITMAPINFO *info, int count ) DECLSPEC_HIDDEN;

/* brush.c */
extern BOOL store_brush_pattern( LOGBRUSH *brush, struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern void free_brush_pattern( struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern BOOL get_brush_bitmap_info( HBRUSH handle, BITMAPINFO *info, void **bits, UINT *usage ) DECLSPEC_HIDDEN;

/* clipping.c */
extern BOOL clip_device_rect( DC *dc, RECT *dst, const RECT *src ) DECLSPEC_HIDDEN;
extern BOOL clip_visrect( DC *dc, RECT *dst, const RECT *src ) DECLSPEC_HIDDEN;
extern void update_dc_clipping( DC * dc ) DECLSPEC_HIDDEN;

/* Return the total DC region (if any) */
static inline HRGN get_dc_region( DC *dc )
{
    if (dc->region) return dc->region;
    if (dc->hVisRgn) return dc->hVisRgn;
    if (dc->hClipRgn) return dc->hClipRgn;
    return dc->hMetaRgn;
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
extern BOOL fill_color_table_from_pal_colors( BITMAPINFO *info, HDC hdc ) DECLSPEC_HIDDEN;
extern const RGBQUAD *get_default_color_table( int bpp ) DECLSPEC_HIDDEN;
extern void fill_default_color_table( BITMAPINFO *info ) DECLSPEC_HIDDEN;
extern void get_ddb_bitmapinfo( BITMAPOBJ *bmp, BITMAPINFO *info ) DECLSPEC_HIDDEN;
extern BITMAPINFO *copy_packed_dib( const BITMAPINFO *src_info, UINT usage ) DECLSPEC_HIDDEN;
extern DWORD convert_bitmapinfo( const BITMAPINFO *src_info, void *src_bits, struct bitblt_coords *src,
                                 const BITMAPINFO *dst_info, void *dst_bits ) DECLSPEC_HIDDEN;

extern DWORD stretch_bitmapinfo( const BITMAPINFO *src_info, void *src_bits, struct bitblt_coords *src,
                                 const BITMAPINFO *dst_info, void *dst_bits, struct bitblt_coords *dst,
                                 INT mode ) DECLSPEC_HIDDEN;
extern DWORD blend_bitmapinfo( const BITMAPINFO *src_info, void *src_bits, struct bitblt_coords *src,
                               const BITMAPINFO *dst_info, void *dst_bits, struct bitblt_coords *dst,
                               BLENDFUNCTION blend ) DECLSPEC_HIDDEN;
extern DWORD gradient_bitmapinfo( const BITMAPINFO *info, void *bits, TRIVERTEX *vert_array, ULONG nvert,
                                  void *grad_array, ULONG ngrad, ULONG mode, const POINT *dev_pts, HRGN rgn ) DECLSPEC_HIDDEN;
extern COLORREF get_pixel_bitmapinfo( const BITMAPINFO *info, void *bits, struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern BOOL render_aa_text_bitmapinfo( DC *dc, BITMAPINFO *info, struct gdi_image_bits *bits,
                                       struct bitblt_coords *src, INT x, INT y, UINT flags,
                                       UINT aa_flags, LPCWSTR str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern DWORD get_image_from_bitmap( BITMAPOBJ *bmp, BITMAPINFO *info,
                                    struct gdi_image_bits *bits, struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern DWORD put_image_into_bitmap( BITMAPOBJ *bmp, HRGN clip, BITMAPINFO *info,
                                    const struct gdi_image_bits *bits, struct bitblt_coords *src,
                                    struct bitblt_coords *dst ) DECLSPEC_HIDDEN;
extern void dibdrv_set_window_surface( DC *dc, struct window_surface *surface ) DECLSPEC_HIDDEN;

/* driver.c */
extern const struct gdi_dc_funcs null_driver DECLSPEC_HIDDEN;
extern const struct gdi_dc_funcs dib_driver DECLSPEC_HIDDEN;
extern const struct gdi_dc_funcs path_driver DECLSPEC_HIDDEN;
extern const struct gdi_dc_funcs *font_driver DECLSPEC_HIDDEN;
extern const struct gdi_dc_funcs *DRIVER_load_driver( LPCWSTR name ) DECLSPEC_HIDDEN;
extern BOOL DRIVER_GetDriverName( LPCWSTR device, LPWSTR driver, DWORD size ) DECLSPEC_HIDDEN;

/* enhmetafile.c */
extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, DWORD filesize, BOOL on_disk ) DECLSPEC_HIDDEN;

/* font.c */
struct font_gamma_ramp
{
    DWORD gamma;
    BYTE  encode[256];
    BYTE  decode[256];
};

/* freetype.c */

/* Undocumented structure filled in by GetFontRealizationInfo */
struct font_realization_info
{
    DWORD size;        /* could be 16 or 24 */
    DWORD flags;       /* 1 for bitmap fonts, 3 for scalable fonts */
    DWORD cache_num;   /* keeps incrementing - num of fonts that have been created allowing for caching?? */
    DWORD instance_id; /* identifies a realized font instance */
    DWORD unk;         /* unknown */
    WORD  face_index;  /* face index in case of font collections */
    WORD  simulations; /* 0 bit - bold simulation, 1 bit - oblique simulation */
};

/* Undocumented structure filled in by GetCharWidthInfo */
struct char_width_info
{
    INT lsb;   /* minimum left side bearing */
    INT rsb;   /* minimum right side bearing */
    INT unk;   /* unknown */
};

extern INT WineEngAddFontResourceEx(LPCWSTR, DWORD, PVOID) DECLSPEC_HIDDEN;
extern HANDLE WineEngAddFontMemResourceEx(PVOID, DWORD, PVOID, LPDWORD) DECLSPEC_HIDDEN;
extern BOOL WineEngCreateScalableFontResource(DWORD, LPCWSTR, LPCWSTR, LPCWSTR) DECLSPEC_HIDDEN;
extern BOOL WineEngInit(void) DECLSPEC_HIDDEN;
extern BOOL WineEngRemoveFontResourceEx(LPCWSTR, DWORD, PVOID) DECLSPEC_HIDDEN;

/* gdiobj.c */
extern HGDIOBJ alloc_gdi_handle( void *obj, WORD type, const struct gdi_obj_funcs *funcs ) DECLSPEC_HIDDEN;
extern void *free_gdi_handle( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern HGDIOBJ get_full_gdi_handle( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern void *GDI_GetObjPtr( HGDIOBJ, WORD ) DECLSPEC_HIDDEN;
extern void *get_any_obj_ptr( HGDIOBJ, WORD * ) DECLSPEC_HIDDEN;
extern void GDI_ReleaseObj( HGDIOBJ ) DECLSPEC_HIDDEN;
extern void GDI_CheckNotLock(void) DECLSPEC_HIDDEN;
extern UINT GDI_get_ref_count( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern HGDIOBJ GDI_inc_ref_count( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern BOOL GDI_dec_ref_count( HGDIOBJ handle ) DECLSPEC_HIDDEN;
extern void GDI_hdc_using_object(HGDIOBJ obj, HDC hdc) DECLSPEC_HIDDEN;
extern void GDI_hdc_not_using_object(HGDIOBJ obj, HDC hdc) DECLSPEC_HIDDEN;
extern DWORD get_dpi(void) DECLSPEC_HIDDEN;
extern DWORD get_system_dpi(void) DECLSPEC_HIDDEN;

/* mapping.c */
extern BOOL dp_to_lp( DC *dc, POINT *points, INT count ) DECLSPEC_HIDDEN;
extern void lp_to_dp( DC *dc, POINT *points, INT count ) DECLSPEC_HIDDEN;

/* metafile.c */
extern HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh) DECLSPEC_HIDDEN;
extern METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mr, LPCVOID filename, BOOL unicode ) DECLSPEC_HIDDEN;

/* Format of comment record added by GetWinMetaFileBits */
#include <pshpack2.h>
typedef struct
{
    DWORD magic;   /* WMFC */
    WORD unk04;    /* 1 */
    WORD unk06;    /* 0 */
    WORD unk08;    /* 0 */
    WORD unk0a;    /* 1 */
    WORD checksum;
    DWORD unk0e;   /* 0 */
    DWORD num_chunks;
    DWORD chunk_size;
    DWORD remaining_size;
    DWORD emf_size;
    BYTE emf_data[1];
} emf_in_wmf_comment;
#include <poppack.h>

#define WMFC_MAGIC 0x43464d57

/* path.c */

extern void free_gdi_path( struct gdi_path *path ) DECLSPEC_HIDDEN;
extern struct gdi_path *get_gdi_flat_path( DC *dc, HRGN *rgn ) DECLSPEC_HIDDEN;
extern int get_gdi_path_data( struct gdi_path *path, POINT **points, BYTE **flags ) DECLSPEC_HIDDEN;
extern BOOL PATH_SavePath( DC *dst, DC *src ) DECLSPEC_HIDDEN;
extern BOOL PATH_RestorePath( DC *dst, DC *src ) DECLSPEC_HIDDEN;

/* painting.c */
extern POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut ) DECLSPEC_HIDDEN;

/* palette.c */
extern HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg) DECLSPEC_HIDDEN;
extern UINT WINAPI GDIRealizePalette( HDC hdc ) DECLSPEC_HIDDEN;
extern HPALETTE PALETTE_Init(void) DECLSPEC_HIDDEN;

/* region.c */
extern BOOL add_rect_to_region( HRGN rgn, const RECT *rect ) DECLSPEC_HIDDEN;
extern INT mirror_region( HRGN dst, HRGN src, INT width ) DECLSPEC_HIDDEN;
extern BOOL REGION_FrameRgn( HRGN dest, HRGN src, INT x, INT y ) DECLSPEC_HIDDEN;
extern HRGN create_polypolygon_region( const POINT *pts, const INT *count, INT nbpolygons,
                                       INT mode, const RECT *clip_rect ) DECLSPEC_HIDDEN;

#define RGN_DEFAULT_RECTS 4
typedef struct
{
    INT size;
    INT numRects;
    RECT *rects;
    RECT extents;
    RECT rects_buf[RGN_DEFAULT_RECTS];
} WINEREGION;

/* return the region data without making a copy */
static inline const WINEREGION *get_wine_region(HRGN rgn)
{
    return GDI_GetObjPtr( rgn, OBJ_REGION );
}
static inline void release_wine_region(HRGN rgn)
{
    GDI_ReleaseObj(rgn);
}

/**********************************************************
 *     region_find_pt
 *
 * Return either the index of the rectangle that contains (x,y) or the first
 * rectangle after it.  Sets *hit to TRUE if the region contains (x,y).
 * Note if (x,y) follows all rectangles, then the return value will be rgn->numRects.
 */
static inline int region_find_pt( const WINEREGION *rgn, int x, int y, BOOL *hit )
{
    int i, start = 0, end = rgn->numRects - 1;
    BOOL h = FALSE;

    while (start <= end)
    {
        i = (start + end) / 2;

        if (rgn->rects[i].bottom <= y ||
            (rgn->rects[i].top <= y && rgn->rects[i].right <= x))
            start = i + 1;
        else if (rgn->rects[i].top > y ||
                 (rgn->rects[i].bottom > y && rgn->rects[i].left > x))
            end = i - 1;
        else
        {
            h = TRUE;
            break;
        }
    }

    if (hit) *hit = h;
    return h ? i : start;
}

/* null driver entry points */
extern BOOL CDECL nulldrv_AbortPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_AlphaBlend( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                      PHYSDEV src_dev, struct bitblt_coords *src, BLENDFUNCTION func) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_BeginPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern DWORD CDECL nulldrv_BlendImage( PHYSDEV dev, BITMAPINFO *info, const struct gdi_image_bits *bits,
                                       struct bitblt_coords *src, struct bitblt_coords *dst, BLENDFUNCTION func ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_CloseFigure( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_EndPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_ExtSelectClipRgn( PHYSDEV dev, HRGN rgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect,
                                      LPCWSTR str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_FillRgn( PHYSDEV dev, HRGN rgn, HBRUSH brush ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_FrameRgn( PHYSDEV dev, HRGN rgn, HBRUSH brush, INT width, INT height ) DECLSPEC_HIDDEN;
extern LONG CDECL nulldrv_GetBitmapBits( HBITMAP bitmap, void *bits, LONG size ) DECLSPEC_HIDDEN;
extern COLORREF CDECL nulldrv_GetNearestColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF CDECL nulldrv_GetPixel( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern UINT CDECL nulldrv_GetSystemPaletteEntries( PHYSDEV dev, UINT start, UINT count, PALETTEENTRY *entries ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                        void * grad_array, ULONG ngrad, ULONG mode ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_InvertRgn( PHYSDEV dev, HRGN rgn ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, DWORD mode ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_OffsetClipRgn( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_PolyBezier( PHYSDEV dev, const POINT *points, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_PolyBezierTo( PHYSDEV dev, const POINT *points, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_PolyDraw( PHYSDEV dev, const POINT *points, const BYTE *types, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_PolylineTo( PHYSDEV dev, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_RestoreDC( PHYSDEV dev, INT level ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_SaveDC( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_ScaleViewportExtEx( PHYSDEV dev, INT x_num, INT x_denom, INT y_num, INT y_denom, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_ScaleWindowExtEx( PHYSDEV dev, INT x_num, INT x_denom, INT y_num, INT y_denom, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_SelectClipPath( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern HFONT CDECL nulldrv_SelectFont( PHYSDEV dev, HFONT font, UINT *ggo_flags ) DECLSPEC_HIDDEN;
extern INT CDECL nulldrv_SetDIBitsToDevice( PHYSDEV dev, INT x_dst, INT y_dst, DWORD width, DWORD height,
                                            INT x_src, INT y_src, UINT start, UINT lines,
                                            const void *bits, BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_SetMapMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_SetViewportExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_SetWindowExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_SetWorldTransform( PHYSDEV dev, const XFORM *xform ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                      PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                         INT xSrc, INT ySrc, INT widthSrc, INT heightSrc, const void *bits,
                                         BITMAPINFO *info, UINT coloruse, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;

static inline DC *get_nulldrv_dc( PHYSDEV dev )
{
    return CONTAINING_RECORD( dev, DC, nulldrv );
}

static inline DC *get_physdev_dc( PHYSDEV dev )
{
    while (dev->funcs != &null_driver)
        dev = dev->next;
    return get_nulldrv_dc( dev );
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

static inline void set_rect( RECT *rect, int left, int top, int right, int bottom )
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static inline void order_rect( RECT *rect )
{
    if (rect->left > rect->right)
    {
        int tmp = rect->left;
        rect->left = rect->right;
        rect->right = tmp;
    }
    if (rect->top > rect->bottom)
    {
        int tmp = rect->top;
        rect->top = rect->bottom;
        rect->bottom = tmp;
    }
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

static inline void reset_bounds( RECT *bounds )
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
}

static inline void add_bounds_rect( RECT *bounds, const RECT *rect )
{
    if (is_rect_empty( rect )) return;
    bounds->left   = min( bounds->left, rect->left );
    bounds->top    = min( bounds->top, rect->top );
    bounds->right  = max( bounds->right, rect->right );
    bounds->bottom = max( bounds->bottom, rect->bottom );
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

/* only for use on sanitized BITMAPINFO structures */
static inline int get_dib_info_size( const BITMAPINFO *info, UINT coloruse )
{
    if (info->bmiHeader.biCompression == BI_BITFIELDS)
        return sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD);
    if (coloruse == DIB_PAL_COLORS)
        return sizeof(BITMAPINFOHEADER) + info->bmiHeader.biClrUsed * sizeof(WORD);
    return FIELD_OFFSET( BITMAPINFO, bmiColors[info->bmiHeader.biClrUsed] );
}

/* only for use on sanitized BITMAPINFO structures */
static inline void copy_bitmapinfo( BITMAPINFO *dst, const BITMAPINFO *src )
{
    memcpy( dst, src, get_dib_info_size( src, DIB_RGB_COLORS ));
}

extern void free_heap_bits( struct gdi_image_bits *bits ) DECLSPEC_HIDDEN;

extern HMODULE gdi32_module DECLSPEC_HIDDEN;

#endif /* __WINE_GDI_PRIVATE_H */
