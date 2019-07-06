/*
 * Definitions for Wine GDI drivers
 *
 * Copyright 2011 Alexandre Julliard
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

#ifndef __WINE_WINE_GDI_DRIVER_H
#define __WINE_WINE_GDI_DRIVER_H

#include "wine/list.h"

struct gdi_dc_funcs;
struct opengl_funcs;
struct vulkan_funcs;

typedef struct gdi_physdev
{
    const struct gdi_dc_funcs *funcs;
    struct gdi_physdev        *next;
    HDC                        hdc;
} *PHYSDEV;

struct bitblt_coords
{
    int  log_x;     /* original position and size, in logical coords */
    int  log_y;
    int  log_width;
    int  log_height;
    int  x;         /* mapped position and size, in device coords */
    int  y;
    int  width;
    int  height;
    RECT visrect;   /* rectangle clipped to the visible part, in device coords */
    DWORD layout;   /* DC layout */
};

struct gdi_image_bits
{
    void   *ptr;       /* pointer to the bits */
    BOOL    is_copy;   /* whether this is a copy of the bits that can be modified */
    void  (*free)(struct gdi_image_bits *);  /* callback for freeing the bits */
    void   *param;     /* extra parameter for callback private use */
};

struct brush_pattern
{
    BITMAPINFO           *info;     /* DIB info */
    struct gdi_image_bits bits;     /* DIB bits */
    UINT                  usage;    /* color usage for DIB info */
};

struct gdi_dc_funcs
{
    INT      (CDECL *pAbortDoc)(PHYSDEV);
    BOOL     (CDECL *pAbortPath)(PHYSDEV);
    BOOL     (CDECL *pAlphaBlend)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,BLENDFUNCTION);
    BOOL     (CDECL *pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    BOOL     (CDECL *pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pBeginPath)(PHYSDEV);
    DWORD    (CDECL *pBlendImage)(PHYSDEV,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,BLENDFUNCTION);
    BOOL     (CDECL *pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pCloseFigure)(PHYSDEV);
    BOOL     (CDECL *pCreateCompatibleDC)(PHYSDEV,PHYSDEV*);
    BOOL     (CDECL *pCreateDC)(PHYSDEV*,LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
    BOOL     (CDECL *pDeleteDC)(PHYSDEV);
    BOOL     (CDECL *pDeleteObject)(PHYSDEV,HGDIOBJ);
    DWORD    (CDECL *pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    BOOL     (CDECL *pEllipse)(PHYSDEV,INT,INT,INT,INT);
    INT      (CDECL *pEndDoc)(PHYSDEV);
    INT      (CDECL *pEndPage)(PHYSDEV);
    BOOL     (CDECL *pEndPath)(PHYSDEV);
    BOOL     (CDECL *pEnumFonts)(PHYSDEV,LPLOGFONTW,FONTENUMPROCW,LPARAM);
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
    BOOL     (CDECL *pFontIsLinked)(PHYSDEV);
    BOOL     (CDECL *pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    BOOL     (CDECL *pGdiComment)(PHYSDEV,UINT,const BYTE*);
    UINT     (CDECL *pGetBoundsRect)(PHYSDEV,RECT*,UINT);
    BOOL     (CDECL *pGetCharABCWidths)(PHYSDEV,UINT,UINT,LPABC);
    BOOL     (CDECL *pGetCharABCWidthsI)(PHYSDEV,UINT,UINT,WORD*,LPABC);
    BOOL     (CDECL *pGetCharWidth)(PHYSDEV,UINT,UINT,LPINT);
    BOOL     (CDECL *pGetCharWidthInfo)(PHYSDEV,void*);
    INT      (CDECL *pGetDeviceCaps)(PHYSDEV,INT);
    BOOL     (CDECL *pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    DWORD    (CDECL *pGetFontData)(PHYSDEV,DWORD,DWORD,LPVOID,DWORD);
    BOOL     (CDECL *pGetFontRealizationInfo)(PHYSDEV,void*);
    DWORD    (CDECL *pGetFontUnicodeRanges)(PHYSDEV,LPGLYPHSET);
    DWORD    (CDECL *pGetGlyphIndices)(PHYSDEV,LPCWSTR,INT,LPWORD,DWORD);
    DWORD    (CDECL *pGetGlyphOutline)(PHYSDEV,UINT,UINT,LPGLYPHMETRICS,DWORD,LPVOID,const MAT2*);
    BOOL     (CDECL *pGetICMProfile)(PHYSDEV,LPDWORD,LPWSTR);
    DWORD    (CDECL *pGetImage)(PHYSDEV,BITMAPINFO*,struct gdi_image_bits*,struct bitblt_coords*);
    DWORD    (CDECL *pGetKerningPairs)(PHYSDEV,DWORD,LPKERNINGPAIR);
    COLORREF (CDECL *pGetNearestColor)(PHYSDEV,COLORREF);
    UINT     (CDECL *pGetOutlineTextMetrics)(PHYSDEV,UINT,LPOUTLINETEXTMETRICW);
    COLORREF (CDECL *pGetPixel)(PHYSDEV,INT,INT);
    UINT     (CDECL *pGetSystemPaletteEntries)(PHYSDEV,UINT,UINT,LPPALETTEENTRY);
    UINT     (CDECL *pGetTextCharsetInfo)(PHYSDEV,LPFONTSIGNATURE,DWORD);
    BOOL     (CDECL *pGetTextExtentExPoint)(PHYSDEV,LPCWSTR,INT,LPINT);
    BOOL     (CDECL *pGetTextExtentExPointI)(PHYSDEV,const WORD*,INT,LPINT);
    INT      (CDECL *pGetTextFace)(PHYSDEV,INT,LPWSTR);
    BOOL     (CDECL *pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    BOOL     (CDECL *pGradientFill)(PHYSDEV,TRIVERTEX*,ULONG,void*,ULONG,ULONG);
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
    DWORD    (CDECL *pPutImage)(PHYSDEV,HRGN,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,DWORD);
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
    HBRUSH   (CDECL *pSelectBrush)(PHYSDEV,HBRUSH,const struct brush_pattern*);
    BOOL     (CDECL *pSelectClipPath)(PHYSDEV,INT);
    HFONT    (CDECL *pSelectFont)(PHYSDEV,HFONT,UINT*);
    HPALETTE (CDECL *pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    HPEN     (CDECL *pSelectPen)(PHYSDEV,HPEN,const struct brush_pattern*);
    INT      (CDECL *pSetArcDirection)(PHYSDEV,INT);
    COLORREF (CDECL *pSetBkColor)(PHYSDEV,COLORREF);
    INT      (CDECL *pSetBkMode)(PHYSDEV,INT);
    UINT     (CDECL *pSetBoundsRect)(PHYSDEV,RECT*,UINT);
    COLORREF (CDECL *pSetDCBrushColor)(PHYSDEV, COLORREF);
    COLORREF (CDECL *pSetDCPenColor)(PHYSDEV, COLORREF);
    INT      (CDECL *pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,BITMAPINFO*,UINT);
    VOID     (CDECL *pSetDeviceClipping)(PHYSDEV,HRGN);
    BOOL     (CDECL *pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    DWORD    (CDECL *pSetLayout)(PHYSDEV,DWORD);
    INT      (CDECL *pSetMapMode)(PHYSDEV,INT);
    DWORD    (CDECL *pSetMapperFlags)(PHYSDEV,DWORD);
    COLORREF (CDECL *pSetPixel)(PHYSDEV,INT,INT,COLORREF);
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
    INT      (CDECL *pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void*,BITMAPINFO*,UINT,DWORD);
    BOOL     (CDECL *pStrokeAndFillPath)(PHYSDEV);
    BOOL     (CDECL *pStrokePath)(PHYSDEV);
    BOOL     (CDECL *pUnrealizePalette)(HPALETTE);
    BOOL     (CDECL *pWidenPath)(PHYSDEV);
    struct opengl_funcs * (CDECL *wine_get_wgl_driver)(PHYSDEV,UINT);
    const struct vulkan_funcs * (CDECL *wine_get_vulkan_driver)(PHYSDEV,UINT);

    /* priority order for the driver on the stack */
    UINT       priority;
};

/* increment this when you change the DC function table */
#define WINE_GDI_DRIVER_VERSION 50

#define GDI_PRIORITY_NULL_DRV        0  /* null driver */
#define GDI_PRIORITY_FONT_DRV      100  /* any font driver */
#define GDI_PRIORITY_GRAPHICS_DRV  200  /* any graphics driver */
#define GDI_PRIORITY_DIB_DRV       300  /* the DIB driver */
#define GDI_PRIORITY_PATH_DRV      400  /* the path driver */

static inline PHYSDEV get_physdev_entry_point( PHYSDEV dev, size_t offset )
{
    while (!((void **)dev->funcs)[offset / sizeof(void *)]) dev = dev->next;
    return dev;
}

#define GET_NEXT_PHYSDEV(dev,func) \
    get_physdev_entry_point( (dev)->next, FIELD_OFFSET(struct gdi_dc_funcs,func))

static inline void push_dc_driver( PHYSDEV *dev, PHYSDEV physdev, const struct gdi_dc_funcs *funcs )
{
    while ((*dev)->funcs->priority > funcs->priority) dev = &(*dev)->next;
    physdev->funcs = funcs;
    physdev->next = *dev;
    physdev->hdc = (*dev)->hdc;
    *dev = physdev;
}

/* support for window surfaces */

struct window_surface;

struct window_surface_funcs
{
    void  (*lock)( struct window_surface *surface );
    void  (*unlock)( struct window_surface *surface );
    void* (*get_info)( struct window_surface *surface, BITMAPINFO *info );
    RECT* (*get_bounds)( struct window_surface *surface );
    void  (*set_region)( struct window_surface *surface, HRGN region );
    void  (*flush)( struct window_surface *surface );
    void  (*destroy)( struct window_surface *surface );
};

struct window_surface
{
    const struct window_surface_funcs *funcs; /* driver-specific implementations  */
    struct list                        entry; /* entry in global list managed by user32 */
    LONG                               ref;   /* reference count */
    RECT                               rect;  /* constant, no locking needed */
    /* driver-specific fields here */
};

static inline ULONG window_surface_add_ref( struct window_surface *surface )
{
    return InterlockedIncrement( &surface->ref );
}

static inline ULONG window_surface_release( struct window_surface *surface )
{
    ULONG ret = InterlockedDecrement( &surface->ref );
    if (!ret) surface->funcs->destroy( surface );
    return ret;
}

/* the DC hook support is only exported on Win16, the 32-bit version is a Wine extension */

#define DCHC_INVALIDVISRGN      0x0001
#define DCHC_DELETEDC           0x0002
#define DCHF_INVALIDATEVISRGN   0x0001
#define DCHF_VALIDATEVISRGN     0x0002
#define DCHF_RESETDC            0x0004  /* Wine extension */
#define DCHF_DISABLEDC          0x0008  /* Wine extension */
#define DCHF_ENABLEDC           0x0010  /* Wine extension */

typedef BOOL (CALLBACK *DCHOOKPROC)(HDC,WORD,DWORD_PTR,LPARAM);

WINGDIAPI DWORD_PTR WINAPI GetDCHook(HDC,DCHOOKPROC*);
WINGDIAPI BOOL      WINAPI SetDCHook(HDC,DCHOOKPROC,DWORD_PTR);
WINGDIAPI WORD      WINAPI SetHookFlags(HDC,WORD);

extern void CDECL __wine_make_gdi_object_system( HGDIOBJ handle, BOOL set );
extern void CDECL __wine_set_visible_region( HDC hdc, HRGN hrgn, const RECT *vis_rect,
                                             const RECT *device_rect, struct window_surface *surface );
extern void CDECL __wine_set_display_driver( HMODULE module );
extern struct opengl_funcs * CDECL __wine_get_wgl_driver( HDC hdc, UINT version );
extern const struct vulkan_funcs * CDECL __wine_get_vulkan_driver( HDC hdc, UINT version );

#endif /* __WINE_WINE_GDI_DRIVER_H */
