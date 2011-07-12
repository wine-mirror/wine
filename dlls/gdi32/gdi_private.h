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

/* Device functions for the Wine driver interface */

typedef struct
{
    int bit_count, width, height;
    int stride; /* stride in bytes.  Will be -ve for bottom-up dibs (see bits). */
    void *bits; /* points to the top-left corner of the dib. */
    void *ptr_to_free;

    DWORD red_mask, green_mask, blue_mask;
    int red_shift, green_shift, blue_shift;
    int red_len, green_len, blue_len;

    RGBQUAD *color_table;
    DWORD color_table_size;

    const struct primitive_funcs *funcs;
} dib_info;

typedef struct
{
    DWORD count;
    DWORD dashes[16]; /* 16 is the maximium number for a PS_USERSTYLE pen */
    DWORD total_len;  /* total length of the dashes, should be multiplied by 2 if there are an odd number of dash lengths */
} dash_pattern;

typedef struct
{
    int left_in_dash;
    int cur_dash;
    BOOL mark;
} dash_pos;

typedef struct
{
    DWORD and;
    DWORD xor;
} rop_mask;

typedef struct
{
    void *and;
    void *xor;
} rop_mask_bits;

typedef struct dibdrv_physdev
{
    struct gdi_physdev dev;
    dib_info dib;

    HRGN clip;
    DWORD defer;

    /* pen */
    COLORREF pen_colorref;
    DWORD pen_color, pen_and, pen_xor;
    dash_pattern pen_pattern;
    dash_pos dash_pos;
    BOOL   (* pen_line)(struct dibdrv_physdev *pdev, POINT *start, POINT *end);

    /* brush */
    UINT brush_style;
    UINT brush_hatch;
    INT brush_rop;   /* PatBlt, for example, can override the DC's rop2 */
    COLORREF brush_colorref;
    DWORD brush_color, brush_and, brush_xor;
    dib_info brush_dib;
    void *brush_and_bits, *brush_xor_bits;
    BOOL   (* brush_rects)(struct dibdrv_physdev *pdev, int num, RECT *rects);

    /* background */
    DWORD bkgnd_color, bkgnd_and, bkgnd_xor;
} dibdrv_physdev;

#define DEFER_FORMAT     1
#define DEFER_PEN        2
#define DEFER_BRUSH      4

typedef struct gdi_dc_funcs
{
    INT      (CDECL *pAbortDoc)(PHYSDEV);
    BOOL     (CDECL *pAbortPath)(PHYSDEV);
    BOOL     (CDECL *pAlphaBlend)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,BLENDFUNCTION);
    BOOL     (CDECL *pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    BOOL     (CDECL *pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pBeginPath)(PHYSDEV);
    INT      (CDECL *pChoosePixelFormat)(PHYSDEV,const PIXELFORMATDESCRIPTOR *);
    BOOL     (CDECL *pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pCloseFigure)(PHYSDEV);
    BOOL     (CDECL *pCreateBitmap)(PHYSDEV,HBITMAP,LPVOID);
    BOOL     (CDECL *pCreateDC)(HDC,PHYSDEV *,LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
    HBITMAP  (CDECL *pCreateDIBSection)(PHYSDEV,HBITMAP,const BITMAPINFO *,UINT);
    BOOL     (CDECL *pDeleteBitmap)(HBITMAP);
    BOOL     (CDECL *pDeleteDC)(PHYSDEV);
    BOOL     (CDECL *pDeleteObject)(PHYSDEV,HGDIOBJ);
    INT      (CDECL *pDescribePixelFormat)(PHYSDEV,INT,UINT,PIXELFORMATDESCRIPTOR *);
    DWORD    (CDECL *pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    BOOL     (CDECL *pEllipse)(PHYSDEV,INT,INT,INT,INT);
    INT      (CDECL *pEndDoc)(PHYSDEV);
    INT      (CDECL *pEndPage)(PHYSDEV);
    BOOL     (CDECL *pEndPath)(PHYSDEV);
    BOOL     (CDECL *pEnumDeviceFonts)(PHYSDEV,LPLOGFONTW,FONTENUMPROCW,LPARAM);
    INT      (CDECL *pEnumICMProfiles)(PHYSDEV,ICMENUMPROCW,LPARAM);
    INT      (CDECL *pExcludeClipRect)(PHYSDEV,INT,INT,INT,INT);
    INT      (CDECL *pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);
    INT      (CDECL *pExtEscape)(PHYSDEV,INT,INT,LPCVOID,INT,LPVOID);
    BOOL     (CDECL *pExtFloodFill)(PHYSDEV,INT,INT,COLORREF,UINT);
    INT      (CDECL *pExtSelectClipRgn)(PHYSDEV,HRGN,INT);
    BOOL     (CDECL *pExtTextOut)(PHYSDEV,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
    BOOL     (CDECL *pFillPath)(PHYSDEV);
    BOOL     (CDECL *pFillRgn)(PHYSDEV,HRGN,HBRUSH);
    BOOL     (CDECL *pFlattenPath)(PHYSDEV);
    BOOL     (CDECL *pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    BOOL     (CDECL *pGdiComment)(PHYSDEV,UINT,CONST BYTE*);
    LONG     (CDECL *pGetBitmapBits)(HBITMAP,void*,LONG);
    BOOL     (CDECL *pGetCharWidth)(PHYSDEV,UINT,UINT,LPINT);
    INT      (CDECL *pGetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPVOID,BITMAPINFO*,UINT);
    INT      (CDECL *pGetDeviceCaps)(PHYSDEV,INT);
    BOOL     (CDECL *pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    BOOL     (CDECL *pGetICMProfile)(PHYSDEV,LPDWORD,LPWSTR);
    COLORREF (CDECL *pGetNearestColor)(PHYSDEV,COLORREF);
    COLORREF (CDECL *pGetPixel)(PHYSDEV,INT,INT);
    INT      (CDECL *pGetPixelFormat)(PHYSDEV);
    UINT     (CDECL *pGetSystemPaletteEntries)(PHYSDEV,UINT,UINT,LPPALETTEENTRY);
    BOOL     (CDECL *pGetTextExtentExPoint)(PHYSDEV,LPCWSTR,INT,INT,LPINT,LPINT,LPSIZE);
    BOOL     (CDECL *pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    INT      (CDECL *pIntersectClipRect)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (CDECL *pInvertRgn)(PHYSDEV,HRGN);
    BOOL     (CDECL *pLineTo)(PHYSDEV,INT,INT);
    BOOL     (CDECL *pModifyWorldTransform)(PHYSDEV,const XFORM*,DWORD);
    BOOL     (CDECL *pMoveTo)(PHYSDEV,INT,INT);
    INT      (CDECL *pOffsetClipRgn)(PHYSDEV,INT,INT);
    BOOL     (CDECL *pOffsetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (CDECL *pOffsetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (CDECL *pPaintRgn)(PHYSDEV,HRGN);
    BOOL     (CDECL *pPatBlt)(PHYSDEV,struct bitblt_coords*,DWORD);
    BOOL     (CDECL *pPie)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pPolyBezier)(PHYSDEV,const POINT*,DWORD);
    BOOL     (CDECL *pPolyBezierTo)(PHYSDEV,const POINT*,DWORD);
    BOOL     (CDECL *pPolyDraw)(PHYSDEV,const POINT*,const BYTE *,DWORD);
    BOOL     (CDECL *pPolyPolygon)(PHYSDEV,const POINT*,const INT*,UINT);
    BOOL     (CDECL *pPolyPolyline)(PHYSDEV,const POINT*,const DWORD*,DWORD);
    BOOL     (CDECL *pPolygon)(PHYSDEV,const POINT*,INT);
    BOOL     (CDECL *pPolyline)(PHYSDEV,const POINT*,INT);
    BOOL     (CDECL *pPolylineTo)(PHYSDEV,const POINT*,INT);
    UINT     (CDECL *pRealizeDefaultPalette)(PHYSDEV);
    UINT     (CDECL *pRealizePalette)(PHYSDEV,HPALETTE,BOOL);
    BOOL     (CDECL *pRectangle)(PHYSDEV,INT,INT,INT,INT);
    HDC      (CDECL *pResetDC)(PHYSDEV,const DEVMODEW*);
    BOOL     (CDECL *pRestoreDC)(PHYSDEV,INT);
    BOOL     (CDECL *pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    INT      (CDECL *pSaveDC)(PHYSDEV);
    BOOL     (CDECL *pScaleViewportExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    BOOL     (CDECL *pScaleWindowExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    HBITMAP  (CDECL *pSelectBitmap)(PHYSDEV,HBITMAP);
    HBRUSH   (CDECL *pSelectBrush)(PHYSDEV,HBRUSH);
    BOOL     (CDECL *pSelectClipPath)(PHYSDEV,INT);
    HFONT    (CDECL *pSelectFont)(PHYSDEV,HFONT,HANDLE);
    HPALETTE (CDECL *pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    HPEN     (CDECL *pSelectPen)(PHYSDEV,HPEN);
    INT      (CDECL *pSetArcDirection)(PHYSDEV,INT);
    LONG     (CDECL *pSetBitmapBits)(HBITMAP,const void*,LONG);
    COLORREF (CDECL *pSetBkColor)(PHYSDEV,COLORREF);
    INT      (CDECL *pSetBkMode)(PHYSDEV,INT);
    COLORREF (CDECL *pSetDCBrushColor)(PHYSDEV, COLORREF);
    COLORREF (CDECL *pSetDCPenColor)(PHYSDEV, COLORREF);
    UINT     (CDECL *pSetDIBColorTable)(PHYSDEV,UINT,UINT,const RGBQUAD*);
    INT      (CDECL *pSetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    INT      (CDECL *pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,
                                           const BITMAPINFO*,UINT);
    VOID     (CDECL *pSetDeviceClipping)(PHYSDEV,HRGN,HRGN);
    BOOL     (CDECL *pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    DWORD    (CDECL *pSetLayout)(PHYSDEV,DWORD);
    INT      (CDECL *pSetMapMode)(PHYSDEV,INT);
    DWORD    (CDECL *pSetMapperFlags)(PHYSDEV,DWORD);
    COLORREF (CDECL *pSetPixel)(PHYSDEV,INT,INT,COLORREF);
    BOOL     (CDECL *pSetPixelFormat)(PHYSDEV,INT,const PIXELFORMATDESCRIPTOR *);
    INT      (CDECL *pSetPolyFillMode)(PHYSDEV,INT);
    INT      (CDECL *pSetROP2)(PHYSDEV,INT);
    INT      (CDECL *pSetRelAbs)(PHYSDEV,INT);
    INT      (CDECL *pSetStretchBltMode)(PHYSDEV,INT);
    UINT     (CDECL *pSetTextAlign)(PHYSDEV,UINT);
    INT      (CDECL *pSetTextCharacterExtra)(PHYSDEV,INT);
    COLORREF (CDECL *pSetTextColor)(PHYSDEV,COLORREF);
    BOOL     (CDECL *pSetTextJustification)(PHYSDEV,INT,INT);
    BOOL     (CDECL *pSetViewportExtEx)(PHYSDEV,INT,INT,SIZE*);
    BOOL     (CDECL *pSetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (CDECL *pSetWindowExtEx)(PHYSDEV,INT,INT,SIZE*);
    BOOL     (CDECL *pSetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (CDECL *pSetWorldTransform)(PHYSDEV,const XFORM*);
    INT      (CDECL *pStartDoc)(PHYSDEV,const DOCINFOW*);
    INT      (CDECL *pStartPage)(PHYSDEV);
    BOOL     (CDECL *pStretchBlt)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,DWORD);
    INT      (CDECL *pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void *,
                                       const BITMAPINFO*,UINT,DWORD);
    BOOL     (CDECL *pStrokeAndFillPath)(PHYSDEV);
    BOOL     (CDECL *pStrokePath)(PHYSDEV);
    BOOL     (CDECL *pSwapBuffers)(PHYSDEV);
    BOOL     (CDECL *pUnrealizePalette)(HPALETTE);
    BOOL     (CDECL *pWidenPath)(PHYSDEV);

    /* OpenGL32 */
    BOOL     (CDECL *pwglCopyContext)(HGLRC, HGLRC, UINT);
    HGLRC    (CDECL *pwglCreateContext)(PHYSDEV);
    HGLRC    (CDECL *pwglCreateContextAttribsARB)(PHYSDEV, HGLRC, const int*);
    BOOL     (CDECL *pwglDeleteContext)(HGLRC);
    PROC     (CDECL *pwglGetProcAddress)(LPCSTR);
    HDC      (CDECL *pwglGetPbufferDCARB)(PHYSDEV, void*);
    BOOL     (CDECL *pwglMakeCurrent)(PHYSDEV, HGLRC);
    BOOL     (CDECL *pwglMakeContextCurrentARB)(PHYSDEV, PHYSDEV, HGLRC);
    BOOL     (CDECL *pwglSetPixelFormatWINE)(PHYSDEV,INT,const PIXELFORMATDESCRIPTOR *);
    BOOL     (CDECL *pwglShareLists)(HGLRC hglrc1, HGLRC hglrc2);
    BOOL     (CDECL *pwglUseFontBitmapsA)(PHYSDEV, DWORD, DWORD, DWORD);
    BOOL     (CDECL *pwglUseFontBitmapsW)(PHYSDEV, DWORD, DWORD, DWORD);
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

typedef struct tagDC
{
    GDIOBJHDR    header;
    HDC          hSelf;            /* Handle to this DC */
    struct gdi_physdev nulldrv;    /* physdev for the null driver */
    struct dibdrv_physdev dibdrv;  /* physdev for the dib driver */
    PHYSDEV      physDev;         /* Physical device (driver-specific) */
    DWORD        thread;          /* thread owning the DC */
    LONG         refcount;        /* thread refcount */
    LONG         dirty;           /* dirty flag */
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
#define DC_BOUNDS_SET    0x0010   /* Bounding rectangle has been set */

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
extern HBITMAP BITMAP_CopyBitmap( HBITMAP hbitmap ) DECLSPEC_HIDDEN;
extern BOOL BITMAP_SetOwnerDC( HBITMAP hbitmap, PHYSDEV physdev ) DECLSPEC_HIDDEN;
extern INT BITMAP_GetWidthBytes( INT bmWidth, INT bpp ) DECLSPEC_HIDDEN;

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
extern void push_dc_driver( DC * dc, PHYSDEV physdev, const DC_FUNCTIONS *funcs ) DECLSPEC_HIDDEN;
extern void pop_dc_driver( DC * dc, PHYSDEV physdev ) DECLSPEC_HIDDEN;
extern void DC_InitDC( DC * dc ) DECLSPEC_HIDDEN;
extern void DC_UpdateXforms( DC * dc ) DECLSPEC_HIDDEN;

/* dib.c */
extern int DIB_GetDIBWidthBytes( int width, int depth ) DECLSPEC_HIDDEN;
extern int DIB_GetDIBImageBytes( int width, int height, int depth ) DECLSPEC_HIDDEN;
extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse ) DECLSPEC_HIDDEN;
extern int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                              LONG *height, WORD *planes, WORD *bpp, DWORD *compr, DWORD *size ) DECLSPEC_HIDDEN;

/* driver.c */
extern const DC_FUNCTIONS null_driver DECLSPEC_HIDDEN;
extern const DC_FUNCTIONS dib_driver DECLSPEC_HIDDEN;
extern const DC_FUNCTIONS *DRIVER_get_display_driver(void) DECLSPEC_HIDDEN;
extern const DC_FUNCTIONS *DRIVER_load_driver( LPCWSTR name ) DECLSPEC_HIDDEN;
extern BOOL DRIVER_GetDriverName( LPCWSTR device, LPWSTR driver, DWORD size ) DECLSPEC_HIDDEN;

static inline PHYSDEV get_physdev_entry_point( PHYSDEV dev, size_t offset )
{
    while (!((void **)dev->funcs)[offset / sizeof(void *)]) dev = dev->next;
    return dev;
}

#define GET_DC_PHYSDEV(dc,func) get_physdev_entry_point( (dc)->physDev, FIELD_OFFSET(DC_FUNCTIONS,func))
#define GET_NEXT_PHYSDEV(dev,func) get_physdev_entry_point( (dev)->next, FIELD_OFFSET(DC_FUNCTIONS,func))

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
extern BOOL CDECL nulldrv_AbortPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_BeginPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_CloseFigure( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_EndPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_ExtSelectClipRgn( PHYSDEV dev, HRGN rgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_FillRgn( PHYSDEV dev, HRGN rgn, HBRUSH brush ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_FrameRgn( PHYSDEV dev, HRGN rgn, HBRUSH brush, INT width, INT height ) DECLSPEC_HIDDEN;
extern LONG CDECL nulldrv_GetBitmapBits( HBITMAP bitmap, void *bits, LONG size ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_GetDIBits( PHYSDEV dev, HBITMAP bitmap, UINT start, UINT lines, LPVOID bits, BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
extern COLORREF CDECL nulldrv_GetNearestColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
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
extern LONG CDECL nulldrv_SetBitmapBits( HBITMAP bitmap, const void *bits, LONG size ) DECLSPEC_HIDDEN;
extern INT  CDECL nulldrv_SetDIBits( PHYSDEV dev, HBITMAP bitmap, UINT start, UINT lines, const void *bits, const BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
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
                                         const BITMAPINFO *info, UINT coloruse, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;

static inline DC *get_nulldrv_dc( PHYSDEV dev )
{
    return CONTAINING_RECORD( dev, DC, nulldrv );
}

/* Undocumented value for DIB's iUsage: Indicates a mono DIB w/o pal entries */
#define DIB_PAL_MONO 2

BOOL WINAPI FontIsLinked(HDC);

BOOL WINAPI SetVirtualResolution(HDC hdc, DWORD horz_res, DWORD vert_res, DWORD horz_size, DWORD vert_size);

#endif /* __WINE_GDI_PRIVATE_H */
