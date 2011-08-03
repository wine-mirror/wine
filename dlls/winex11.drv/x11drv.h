/*
 * X11 driver definitions
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 1999 Patrik Stridvall
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

#ifndef __WINE_X11DRV_H
#define __WINE_X11DRV_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <stdarg.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#ifdef HAVE_LIBXXSHM
# include <X11/extensions/XShm.h>
#endif /* defined(HAVE_LIBXXSHM) */

#define BOOL X_BOOL
#define BYTE X_BYTE
#define INT8 X_INT8
#define INT16 X_INT16
#define INT32 X_INT32
#define INT64 X_INT64
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#undef BOOL
#undef BYTE
#undef INT8
#undef INT16
#undef INT32
#undef INT64
#undef LONG64

#undef Status  /* avoid conflict with wintrnl.h */
typedef int Status;

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ddrawi.h"
#include "wine/gdi_driver.h"
#include "wine/list.h"

#define MAX_PIXELFORMATS 8
#define MAX_DASHLEN 16

#define WINE_XDND_VERSION 4

extern void CDECL wine_tsx11_lock(void);
extern void CDECL wine_tsx11_unlock(void);

  /* X physical pen */
typedef struct
{
    int          style;
    int          endcap;
    int          linejoin;
    int          pixel;
    int          width;
    char         dashes[MAX_DASHLEN];
    int          dash_len;
    int          type;          /* GEOMETRIC || COSMETIC */
    int          ext;           /* extended pen - 1, otherwise - 0 */
} X_PHYSPEN;

  /* X physical brush */
typedef struct
{
    int          style;
    int          fillStyle;
    int          pixel;
    Pixmap       pixmap;
} X_PHYSBRUSH;

enum x11drv_shm_mode
{
    X11DRV_SHM_NONE = 0,
    X11DRV_SHM_PIXMAP,
    X11DRV_SHM_IMAGE,
};

typedef struct {
    int shift;
    int scale;
    int max;
} ChannelShift;

typedef struct
{
    ChannelShift physicalRed, physicalBlue, physicalGreen;
    ChannelShift logicalRed, logicalBlue, logicalGreen;
} ColorShifts;

  /* X physical bitmap */
typedef struct
{
    HBITMAP      hbitmap;
    Pixmap       pixmap;
    XID          glxpixmap;
    int          pixmap_depth;
    ColorShifts  pixmap_color_shifts;
    /* the following fields are only used for DIB section bitmaps */
    int          status, p_status;  /* mapping status */
    XImage      *image;             /* cached XImage */
    int         *colorMap;          /* color map info */
    int          nColorMap;
    BOOL         trueColor;
    BOOL         topdown;
    CRITICAL_SECTION lock;          /* GDI access lock */
    enum x11drv_shm_mode shm_mode;
#ifdef HAVE_LIBXXSHM
    XShmSegmentInfo shminfo;        /* shared memory segment info */
#endif
    struct list   entry;            /* Entry in global DIB list */
    BYTE         *base;             /* Base address */
    SIZE_T        size;             /* Size in bytes */
} X_PHYSBITMAP;

  /* X physical font */
typedef UINT	 X_PHYSFONT;

struct xrender_info;

  /* X physical device */
typedef struct
{
    struct gdi_physdev dev;
    GC            gc;          /* X Window GC */
    Drawable      drawable;
    RECT          dc_rect;       /* DC rectangle relative to drawable */
    RECT          drawable_rect; /* Drawable rectangle relative to screen */
    HRGN          region;        /* Device region (visible region & clip region) */
    X_PHYSFONT    font;
    X_PHYSPEN     pen;
    X_PHYSBRUSH   brush;
    X_PHYSBITMAP *bitmap;       /* currently selected bitmap for memory DCs */
    BOOL          has_gdi_font; /* is current font a GDI font? */
    int           backgroundPixel;
    int           textPixel;
    int           depth;       /* bit depth of the DC */
    ColorShifts  *color_shifts; /* color shifts of the DC */
    int           exposures;   /* count of graphics exposures operations */
    int           current_pf;
    Drawable      gl_drawable;
    Pixmap        pixmap;      /* Pixmap for a GLXPixmap gl_drawable */
    int           gl_copy;
    struct xrender_info *xrender;
} X11DRV_PDEVICE;

static inline X11DRV_PDEVICE *get_x11drv_dev( PHYSDEV dev )
{
    return (X11DRV_PDEVICE *)dev;
}

extern X_PHYSBITMAP BITMAP_stock_phys_bitmap DECLSPEC_HIDDEN;  /* phys bitmap for the default stock bitmap */

/* Retrieve the GC used for bitmap operations */
extern GC get_bitmap_gc(int depth) DECLSPEC_HIDDEN;

/* Wine driver X11 functions */

extern BOOL X11DRV_AlphaBlend( PHYSDEV dst_dev, struct bitblt_coords *dst,
                               PHYSDEV src_dev, struct bitblt_coords *src,
                               BLENDFUNCTION blendfn ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Arc( PHYSDEV dev, INT left, INT top, INT right,
                        INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                          INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_CreateBitmap( PHYSDEV dev, HBITMAP hbitmap ) DECLSPEC_HIDDEN;
extern HBITMAP X11DRV_CreateDIBSection( PHYSDEV dev, HBITMAP hbitmap,
                                        const BITMAPINFO *bmi, UINT usage ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_DeleteBitmap( HBITMAP hbitmap ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_EnumDeviceFonts( PHYSDEV dev, LPLOGFONTW plf,
                                    FONTENUMPROCW dfeproc, LPARAM lp ) DECLSPEC_HIDDEN;
extern INT X11DRV_EnumICMProfiles( PHYSDEV dev, ICMENUMPROCW proc, LPARAM lparam ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprect,
                               LPCWSTR str, UINT count, const INT *lpDx ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetCharWidth( PHYSDEV dev, UINT firstChar, UINT lastChar, LPINT buffer ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetDeviceGammaRamp( PHYSDEV dev, LPVOID ramp ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetICMProfile( PHYSDEV dev, LPDWORD size, LPWSTR filename ) DECLSPEC_HIDDEN;
extern DWORD X11DRV_GetImage( PHYSDEV dev, HBITMAP hbitmap, BITMAPINFO *info,
                              struct gdi_image_bits *bits, struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_GetNearestColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_GetPixel( PHYSDEV dev, INT x, INT y) DECLSPEC_HIDDEN;
extern UINT X11DRV_GetSystemPaletteEntries( PHYSDEV dev, UINT start, UINT count, LPPALETTEENTRY entries ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetTextExtentExPoint( PHYSDEV dev, LPCWSTR str, INT count, INT maxExt,
                                         LPINT lpnFit, LPINT alpDx, LPSIZE size ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetTextMetrics(PHYSDEV dev, TEXTMETRICW *metrics) DECLSPEC_HIDDEN;
extern BOOL X11DRV_LineTo( PHYSDEV dev, INT x, INT y) DECLSPEC_HIDDEN;
extern BOOL X11DRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Pie( PHYSDEV dev, INT left, INT top, INT right,
                        INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Polyline( PHYSDEV dev,const POINT* pt,INT count) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Polygon( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polygons) DECLSPEC_HIDDEN;
extern BOOL X11DRV_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polylines) DECLSPEC_HIDDEN;
extern DWORD X11DRV_PutImage( PHYSDEV dev, HBITMAP hbitmap, HRGN clip, BITMAPINFO *info,
                              const struct gdi_image_bits *bits, struct bitblt_coords *src,
                              struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern UINT X11DRV_RealizeDefaultPalette( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern UINT X11DRV_RealizePalette( PHYSDEV dev, HPALETTE hpal, BOOL primary ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL X11DRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                              INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern HBITMAP X11DRV_SelectBitmap( PHYSDEV dev, HBITMAP hbitmap ) DECLSPEC_HIDDEN;
extern HBRUSH X11DRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern HFONT X11DRV_SelectFont( PHYSDEV dev, HFONT hfont, HANDLE gdiFont ) DECLSPEC_HIDDEN;
extern HPEN X11DRV_SelectPen( PHYSDEV dev, HPEN hpen ) DECLSPEC_HIDDEN;
extern LONG X11DRV_SetBitmapBits( HBITMAP hbitmap, const void *bits, LONG count ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_SetBkColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_SetDCBrushColor( PHYSDEV dev, COLORREF crColor ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_SetDCPenColor( PHYSDEV dev, COLORREF crColor ) DECLSPEC_HIDDEN;
extern void X11DRV_SetDeviceClipping( PHYSDEV dev, HRGN vis_rgn, HRGN clip_rgn ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetDeviceGammaRamp( PHYSDEV dev, LPVOID ramp ) DECLSPEC_HIDDEN;
extern UINT X11DRV_SetDIBColorTable( PHYSDEV dev, UINT start, UINT count, const RGBQUAD *colors ) DECLSPEC_HIDDEN;
extern INT X11DRV_SetDIBitsToDevice( PHYSDEV dev, INT xDest, INT yDest, DWORD cx, DWORD cy, INT xSrc, INT ySrc,
                                     UINT startscan, UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetPixelFormat(PHYSDEV dev, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_SetTextColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                               PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_UnrealizePalette( HPALETTE hpal ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_wglCopyContext( HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask ) DECLSPEC_HIDDEN;
extern HGLRC X11DRV_wglCreateContext( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern HGLRC X11DRV_wglCreateContextAttribsARB( PHYSDEV dev, HGLRC hShareContext, const int* attribList ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_wglDeleteContext( HGLRC hglrc ) DECLSPEC_HIDDEN;
extern PROC X11DRV_wglGetProcAddress( LPCSTR proc ) DECLSPEC_HIDDEN;
extern HDC X11DRV_wglGetPbufferDCARB( PHYSDEV dev, void *pbuffer ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_wglMakeContextCurrentARB( PHYSDEV draw_dev, PHYSDEV read_dev, HGLRC hglrc ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_wglMakeCurrent( PHYSDEV dev, HGLRC hglrc ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_wglSetPixelFormatWINE( PHYSDEV dev, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_wglShareLists( HGLRC hglrc1, HGLRC hglrc2 ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_wglUseFontBitmapsA( PHYSDEV dev, DWORD first, DWORD count, DWORD listBase ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_wglUseFontBitmapsW( PHYSDEV dev, DWORD first, DWORD count, DWORD listBase ) DECLSPEC_HIDDEN;

/* OpenGL / X11 driver functions */
extern int X11DRV_ChoosePixelFormat(PHYSDEV dev,
		                      const PIXELFORMATDESCRIPTOR *pppfd) DECLSPEC_HIDDEN;
extern int X11DRV_DescribePixelFormat(PHYSDEV dev,
		                        int iPixelFormat, UINT nBytes,
					PIXELFORMATDESCRIPTOR *ppfd) DECLSPEC_HIDDEN;
extern int X11DRV_GetPixelFormat(PHYSDEV dev) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SwapBuffers(PHYSDEV dev) DECLSPEC_HIDDEN;
extern void X11DRV_OpenGL_Cleanup(void) DECLSPEC_HIDDEN;

/* X11 driver internal functions */

extern void X11DRV_Xcursor_Init(void) DECLSPEC_HIDDEN;
extern void X11DRV_BITMAP_Init(void) DECLSPEC_HIDDEN;
extern void X11DRV_FONT_Init( int log_pixels_x, int log_pixels_y ) DECLSPEC_HIDDEN;
extern void X11DRV_XInput2_Init(void) DECLSPEC_HIDDEN;

extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse ) DECLSPEC_HIDDEN;
extern XImage *X11DRV_DIB_CreateXImage( int width, int height, int depth ) DECLSPEC_HIDDEN;
extern void X11DRV_DIB_DestroyXImage( XImage *image ) DECLSPEC_HIDDEN;
extern HGLOBAL X11DRV_DIB_CreateDIBFromBitmap(HDC hdc, HBITMAP hBmp) DECLSPEC_HIDDEN;
extern HGLOBAL X11DRV_DIB_CreateDIBFromPixmap(Pixmap pixmap, HDC hdc) DECLSPEC_HIDDEN;
extern Pixmap X11DRV_DIB_CreatePixmapFromDIB( HGLOBAL hPackedDIB, HDC hdc ) DECLSPEC_HIDDEN;
extern X_PHYSBITMAP *X11DRV_get_phys_bitmap( HBITMAP hbitmap ) DECLSPEC_HIDDEN;
extern X_PHYSBITMAP *X11DRV_init_phys_bitmap( HBITMAP hbitmap ) DECLSPEC_HIDDEN;
extern Pixmap X11DRV_get_pixmap( HBITMAP hbitmap ) DECLSPEC_HIDDEN;

extern RGNDATA *X11DRV_GetRegionData( HRGN hrgn, HDC hdc_lptodp ) DECLSPEC_HIDDEN;
extern HRGN add_extra_clipping_region( X11DRV_PDEVICE *dev, HRGN rgn ) DECLSPEC_HIDDEN;
extern void restore_clipping_region( X11DRV_PDEVICE *dev, HRGN rgn ) DECLSPEC_HIDDEN;

extern BOOL X11DRV_SetupGCForPatBlt( X11DRV_PDEVICE *physDev, GC gc, BOOL fMapColors ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetupGCForBrush( X11DRV_PDEVICE *physDev ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetupGCForText( X11DRV_PDEVICE *physDev ) DECLSPEC_HIDDEN;
extern INT X11DRV_XWStoDS( X11DRV_PDEVICE *physDev, INT width ) DECLSPEC_HIDDEN;
extern INT X11DRV_YWStoDS( X11DRV_PDEVICE *physDev, INT height ) DECLSPEC_HIDDEN;

extern const int X11DRV_XROPfunction[];

extern void _XInitImageFuncPtrs(XImage *) DECLSPEC_HIDDEN;

extern int client_side_with_core DECLSPEC_HIDDEN;
extern int client_side_with_render DECLSPEC_HIDDEN;
extern int client_side_antialias_with_core DECLSPEC_HIDDEN;
extern int client_side_antialias_with_render DECLSPEC_HIDDEN;
extern int using_client_side_fonts DECLSPEC_HIDDEN;
extern void X11DRV_XRender_Init(void) DECLSPEC_HIDDEN;
extern void X11DRV_XRender_Finalize(void) DECLSPEC_HIDDEN;
extern BOOL X11DRV_XRender_SelectFont(X11DRV_PDEVICE*, HFONT) DECLSPEC_HIDDEN;
extern void X11DRV_XRender_SetDeviceClipping(X11DRV_PDEVICE *physDev, const RGNDATA *data) DECLSPEC_HIDDEN;
extern void X11DRV_XRender_DeleteDC(X11DRV_PDEVICE*) DECLSPEC_HIDDEN;
extern void X11DRV_XRender_CopyBrush(X11DRV_PDEVICE *physDev, X_PHYSBITMAP *physBitmap, int width, int height) DECLSPEC_HIDDEN;
extern BOOL X11DRV_XRender_ExtTextOut(X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
				      const RECT *lprect, LPCWSTR wstr,
				      UINT count, const INT *lpDx) DECLSPEC_HIDDEN;
extern BOOL X11DRV_XRender_SetPhysBitmapDepth(X_PHYSBITMAP *physBitmap, int bits_pixel, const DIBSECTION *dib) DECLSPEC_HIDDEN;
BOOL X11DRV_XRender_GetSrcAreaStretch(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                                      Pixmap pixmap, GC gc,
                                      const struct bitblt_coords *src, const struct bitblt_coords *dst ) DECLSPEC_HIDDEN;
extern void X11DRV_XRender_UpdateDrawable(X11DRV_PDEVICE *physDev) DECLSPEC_HIDDEN;
extern BOOL XRender_AlphaBlend( X11DRV_PDEVICE *devDst, struct bitblt_coords *dst,
                                X11DRV_PDEVICE *devSrc, struct bitblt_coords *src,
                                BLENDFUNCTION blendfn ) DECLSPEC_HIDDEN;

extern Drawable get_glxdrawable(X11DRV_PDEVICE *physDev) DECLSPEC_HIDDEN;
extern BOOL destroy_glxpixmap(Display *display, XID glxpixmap) DECLSPEC_HIDDEN;

/* IME support */
extern void IME_UnregisterClasses(void) DECLSPEC_HIDDEN;
extern void IME_SetOpenStatus(BOOL fOpen, BOOL force) DECLSPEC_HIDDEN;
extern INT IME_GetCursorPos(void) DECLSPEC_HIDDEN;
extern void IME_SetCursorPos(DWORD pos) DECLSPEC_HIDDEN;
extern void IME_UpdateAssociation(HWND focus) DECLSPEC_HIDDEN;
extern BOOL IME_SetCompositionString(DWORD dwIndex, LPCVOID lpComp,
                                     DWORD dwCompLen, LPCVOID lpRead,
                                     DWORD dwReadLen) DECLSPEC_HIDDEN;
extern void IME_SetResultString(LPWSTR lpResult, DWORD dwResultlen) DECLSPEC_HIDDEN;

extern void X11DRV_XDND_EnterEvent( HWND hWnd, XClientMessageEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_XDND_PositionEvent( HWND hWnd, XClientMessageEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_XDND_DropEvent( HWND hWnd, XClientMessageEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_XDND_LeaveEvent( HWND hWnd, XClientMessageEvent *event ) DECLSPEC_HIDDEN;

/* exported dib functions for now */

/* DIB Section sync state */
enum { DIB_Status_None, DIB_Status_InSync, DIB_Status_GdiMod, DIB_Status_AppMod };

typedef struct {
    void (*Convert_5x5_asis)(int width, int height,
                             const void* srcbits, int srclinebytes,
                             void* dstbits, int dstlinebytes);
    void (*Convert_555_reverse)(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes);
    void (*Convert_555_to_565_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_555_to_565_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_555_to_888_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_555_to_888_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_555_to_0888_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_555_to_0888_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_5x5_to_any0888)(int width, int height,
                                   const void* srcbits, int srclinebytes,
                                   WORD rsrc, WORD gsrc, WORD bsrc,
                                   void* dstbits, int dstlinebytes,
                                   DWORD rdst, DWORD gdst, DWORD bdst);
    void (*Convert_565_reverse)(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes);
    void (*Convert_565_to_555_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_565_to_555_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_565_to_888_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_565_to_888_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_565_to_0888_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_565_to_0888_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_888_asis)(int width, int height,
                             const void* srcbits, int srclinebytes,
                             void* dstbits, int dstlinebytes);
    void (*Convert_888_reverse)(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes);
    void (*Convert_888_to_555_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_888_to_555_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_888_to_565_asis)(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes);
    void (*Convert_888_to_565_reverse)(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes);
    void (*Convert_888_to_0888_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_888_to_0888_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_rgb888_to_any0888)(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      void* dstbits, int dstlinebytes,
                                      DWORD rdst, DWORD gdst, DWORD bdst);
    void (*Convert_bgr888_to_any0888)(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      void* dstbits, int dstlinebytes,
                                      DWORD rdst, DWORD gdst, DWORD bdst);
    void (*Convert_0888_asis)(int width, int height,
                              const void* srcbits, int srclinebytes,
                              void* dstbits, int dstlinebytes);
    void (*Convert_0888_reverse)(int width, int height,
                                 const void* srcbits, int srclinebytes,
                                 void* dstbits, int dstlinebytes);
    void (*Convert_0888_any)(int width, int height,
                             const void* srcbits, int srclinebytes,
                             DWORD rsrc, DWORD gsrc, DWORD bsrc,
                             void* dstbits, int dstlinebytes,
                             DWORD rdst, DWORD gdst, DWORD bdst);
    void (*Convert_0888_to_555_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_0888_to_555_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_0888_to_565_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_0888_to_565_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_any0888_to_5x5)(int width, int height,
                                   const void* srcbits, int srclinebytes,
                                   DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                   void* dstbits, int dstlinebytes,
                                   WORD rdst, WORD gdst, WORD bdst);
    void (*Convert_0888_to_888_asis)(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes);
    void (*Convert_0888_to_888_reverse)(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes);
    void (*Convert_any0888_to_rgb888)(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                      void* dstbits, int dstlinebytes);
    void (*Convert_any0888_to_bgr888)(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                      void* dstbits, int dstlinebytes);
} dib_conversions;

extern const dib_conversions dib_normal DECLSPEC_HIDDEN, dib_src_byteswap DECLSPEC_HIDDEN, dib_dst_byteswap DECLSPEC_HIDDEN;

extern INT X11DRV_DIB_MaskToShift(DWORD mask) DECLSPEC_HIDDEN;
extern INT X11DRV_CoerceDIBSection(X11DRV_PDEVICE *physDev,INT) DECLSPEC_HIDDEN;
extern INT X11DRV_LockDIBSection(X11DRV_PDEVICE *physDev,INT) DECLSPEC_HIDDEN;
extern void X11DRV_UnlockDIBSection(X11DRV_PDEVICE *physDev,BOOL) DECLSPEC_HIDDEN;
extern INT X11DRV_DIB_Lock(X_PHYSBITMAP *,INT) DECLSPEC_HIDDEN;
extern void X11DRV_DIB_Unlock(X_PHYSBITMAP *,BOOL) DECLSPEC_HIDDEN;

extern void X11DRV_DIB_DeleteDIBSection(X_PHYSBITMAP *physBitmap, DIBSECTION *dib) DECLSPEC_HIDDEN;
extern void X11DRV_DIB_CopyDIBSection(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                                      DWORD xSrc, DWORD ySrc, DWORD xDest, DWORD yDest,
                                      DWORD width, DWORD height) DECLSPEC_HIDDEN;

/**************************************************************************
 * X11 GDI driver
 */

extern void X11DRV_GDI_Finalize(void) DECLSPEC_HIDDEN;

extern Display *gdi_display DECLSPEC_HIDDEN;  /* display to use for all GDI functions */

/* X11 GDI palette driver */

#define X11DRV_PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define X11DRV_PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define X11DRV_PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */
#define X11DRV_PALETTE_WHITESET 0x2000

extern Colormap X11DRV_PALETTE_PaletteXColormap DECLSPEC_HIDDEN;
extern UINT16 X11DRV_PALETTE_PaletteFlags DECLSPEC_HIDDEN;

extern int *X11DRV_PALETTE_PaletteToXPixel DECLSPEC_HIDDEN;
extern int *X11DRV_PALETTE_XPixelToPalette DECLSPEC_HIDDEN;
extern ColorShifts X11DRV_PALETTE_default_shifts DECLSPEC_HIDDEN;

extern int X11DRV_PALETTE_mapEGAPixel[16] DECLSPEC_HIDDEN;

extern int X11DRV_PALETTE_Init(void) DECLSPEC_HIDDEN;
extern void X11DRV_PALETTE_Cleanup(void) DECLSPEC_HIDDEN;
extern BOOL X11DRV_IsSolidColor(COLORREF color) DECLSPEC_HIDDEN;

extern COLORREF X11DRV_PALETTE_ToLogical(X11DRV_PDEVICE *physDev, int pixel) DECLSPEC_HIDDEN;
extern int X11DRV_PALETTE_ToPhysical(X11DRV_PDEVICE *physDev, COLORREF color) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_PALETTE_GetColor( X11DRV_PDEVICE *physDev, COLORREF color ) DECLSPEC_HIDDEN;
extern int X11DRV_PALETTE_LookupPixel(ColorShifts *shifts, COLORREF color) DECLSPEC_HIDDEN;
extern void X11DRV_PALETTE_ComputeColorShifts(ColorShifts *shifts, unsigned long redMask, unsigned long greenMask, unsigned long blueMask) DECLSPEC_HIDDEN;

/* GDI escapes */

#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,      /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,     /* get current drawable for a DC */
    X11DRV_GET_FONT,         /* get current X font for a DC */
    X11DRV_SET_DRAWABLE,     /* set current drawable for a DC */
    X11DRV_START_EXPOSURES,  /* start graphics exposures */
    X11DRV_END_EXPOSURES,    /* end graphics exposures */
    X11DRV_GET_DCE,          /* no longer used */
    X11DRV_SET_DCE,          /* no longer used */
    X11DRV_GET_GLX_DRAWABLE, /* get current glx drawable for a DC */
    X11DRV_SYNC_PIXMAP,      /* sync the dibsection to its pixmap */
    X11DRV_FLUSH_GL_DRAWABLE /* flush changes made to the gl drawable */
};

struct x11drv_escape_set_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_SET_DRAWABLE) */
    Drawable                 drawable;     /* X drawable */
    int                      mode;         /* ClipByChildren or IncludeInferiors */
    RECT                     dc_rect;      /* DC rectangle relative to drawable */
    RECT                     drawable_rect;/* Drawable rectangle relative to screen */
    XID                      fbconfig_id;  /* fbconfig id used by the GL drawable */
    Drawable                 gl_drawable;  /* GL drawable */
    Pixmap                   pixmap;       /* Pixmap for a GLXPixmap gl_drawable */
    int                      gl_copy;      /* whether the GL contents need explicit copying */
};

/**************************************************************************
 * X11 USER driver
 */

struct x11drv_thread_data
{
    Display *display;
    XEvent  *current_event;        /* event currently being processed */
    Window   grab_window;          /* window that currently grabs the mouse */
    HWND     last_focus;           /* last window that had focus */
    XIM      xim;                  /* input method */
    HWND     last_xic_hwnd;        /* last xic window */
    Time     last_motion_notify;   /* time of last mouse motion */
    XFontSet font_set;             /* international text drawing font set */
    Window   selection_wnd;        /* window used for selection interactions */
    unsigned long warp_serial;     /* serial number of last pointer warp request */
    Window   clip_window;          /* window used for cursor clipping */
    HWND     clip_hwnd;            /* message window stored in desktop while clipping is active */
    DWORD    clip_reset;           /* time when clipping was last reset */
    HKL      kbd_layout;           /* active keyboard layout */
    enum { xi_unavailable = -1, xi_unknown, xi_disabled, xi_enabled } xi2_state; /* XInput2 state */
};

extern struct x11drv_thread_data *x11drv_init_thread_data(void) DECLSPEC_HIDDEN;
extern DWORD thread_data_tls_index DECLSPEC_HIDDEN;

static inline struct x11drv_thread_data *x11drv_thread_data(void)
{
    return TlsGetValue( thread_data_tls_index );
}

/* retrieve the thread display, or NULL if not created yet */
static inline Display *thread_display(void)
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    if (!data) return NULL;
    return data->display;
}

/* retrieve the thread display, creating it if needed */
static inline Display *thread_init_display(void)
{
    return x11drv_init_thread_data()->display;
}

static inline size_t get_property_size( int format, unsigned long count )
{
    /* format==32 means long, which can be 64 bits... */
    if (format == 32) return count * sizeof(long);
    return count * (format / 8);
}

extern Visual *visual DECLSPEC_HIDDEN;
extern XPixmapFormatValues **pixmap_formats DECLSPEC_HIDDEN;
extern Window root_window DECLSPEC_HIDDEN;
extern int clipping_cursor DECLSPEC_HIDDEN;
extern unsigned int screen_width DECLSPEC_HIDDEN;
extern unsigned int screen_height DECLSPEC_HIDDEN;
extern unsigned int screen_bpp DECLSPEC_HIDDEN;
extern unsigned int screen_depth DECLSPEC_HIDDEN;
extern RECT virtual_screen_rect DECLSPEC_HIDDEN;
extern unsigned int text_caps DECLSPEC_HIDDEN;
extern int use_xkb DECLSPEC_HIDDEN;
extern int usexrandr DECLSPEC_HIDDEN;
extern int usexvidmode DECLSPEC_HIDDEN;
extern BOOL ximInComposeMode DECLSPEC_HIDDEN;
extern int use_take_focus DECLSPEC_HIDDEN;
extern int use_primary_selection DECLSPEC_HIDDEN;
extern int use_system_cursors DECLSPEC_HIDDEN;
extern int show_systray DECLSPEC_HIDDEN;
extern int grab_pointer DECLSPEC_HIDDEN;
extern int grab_fullscreen DECLSPEC_HIDDEN;
extern int usexcomposite DECLSPEC_HIDDEN;
extern int managed_mode DECLSPEC_HIDDEN;
extern int decorated_mode DECLSPEC_HIDDEN;
extern int private_color_map DECLSPEC_HIDDEN;
extern int primary_monitor DECLSPEC_HIDDEN;
extern int copy_default_colors DECLSPEC_HIDDEN;
extern int alloc_system_colors DECLSPEC_HIDDEN;
extern int xrender_error_base DECLSPEC_HIDDEN;
extern HMODULE x11drv_module DECLSPEC_HIDDEN;

/* atoms */

enum x11drv_atoms
{
    FIRST_XATOM = XA_LAST_PREDEFINED + 1,
    XATOM_CLIPBOARD = FIRST_XATOM,
    XATOM_COMPOUND_TEXT,
    XATOM_INCR,
    XATOM_MANAGER,
    XATOM_MULTIPLE,
    XATOM_SELECTION_DATA,
    XATOM_TARGETS,
    XATOM_TEXT,
    XATOM_UTF8_STRING,
    XATOM_RAW_ASCENT,
    XATOM_RAW_DESCENT,
    XATOM_RAW_CAP_HEIGHT,
    XATOM_WM_PROTOCOLS,
    XATOM_WM_DELETE_WINDOW,
    XATOM_WM_STATE,
    XATOM_WM_TAKE_FOCUS,
    XATOM_DndProtocol,
    XATOM_DndSelection,
    XATOM__ICC_PROFILE,
    XATOM__MOTIF_WM_HINTS,
    XATOM__NET_STARTUP_INFO_BEGIN,
    XATOM__NET_STARTUP_INFO,
    XATOM__NET_SUPPORTED,
    XATOM__NET_SYSTEM_TRAY_OPCODE,
    XATOM__NET_SYSTEM_TRAY_S0,
    XATOM__NET_WM_ICON,
    XATOM__NET_WM_MOVERESIZE,
    XATOM__NET_WM_NAME,
    XATOM__NET_WM_PID,
    XATOM__NET_WM_PING,
    XATOM__NET_WM_STATE,
    XATOM__NET_WM_STATE_ABOVE,
    XATOM__NET_WM_STATE_FULLSCREEN,
    XATOM__NET_WM_STATE_MAXIMIZED_HORZ,
    XATOM__NET_WM_STATE_MAXIMIZED_VERT,
    XATOM__NET_WM_STATE_SKIP_PAGER,
    XATOM__NET_WM_STATE_SKIP_TASKBAR,
    XATOM__NET_WM_USER_TIME,
    XATOM__NET_WM_USER_TIME_WINDOW,
    XATOM__NET_WM_WINDOW_OPACITY,
    XATOM__NET_WM_WINDOW_TYPE,
    XATOM__NET_WM_WINDOW_TYPE_DIALOG,
    XATOM__NET_WM_WINDOW_TYPE_NORMAL,
    XATOM__NET_WM_WINDOW_TYPE_UTILITY,
    XATOM__NET_WORKAREA,
    XATOM__XEMBED,
    XATOM__XEMBED_INFO,
    XATOM_XdndAware,
    XATOM_XdndEnter,
    XATOM_XdndPosition,
    XATOM_XdndStatus,
    XATOM_XdndLeave,
    XATOM_XdndFinished,
    XATOM_XdndDrop,
    XATOM_XdndActionCopy,
    XATOM_XdndActionMove,
    XATOM_XdndActionLink,
    XATOM_XdndActionAsk,
    XATOM_XdndActionPrivate,
    XATOM_XdndSelection,
    XATOM_XdndTarget,
    XATOM_XdndTypeList,
    XATOM_HTML_Format,
    XATOM_WCF_BITMAP,
    XATOM_WCF_DIB,
    XATOM_WCF_DIBV5,
    XATOM_WCF_DIF,
    XATOM_WCF_DSPBITMAP,
    XATOM_WCF_DSPENHMETAFILE,
    XATOM_WCF_DSPMETAFILEPICT,
    XATOM_WCF_DSPTEXT,
    XATOM_WCF_ENHMETAFILE,
    XATOM_WCF_HDROP,
    XATOM_WCF_LOCALE,
    XATOM_WCF_METAFILEPICT,
    XATOM_WCF_OEMTEXT,
    XATOM_WCF_OWNERDISPLAY,
    XATOM_WCF_PALETTE,
    XATOM_WCF_PENDATA,
    XATOM_WCF_RIFF,
    XATOM_WCF_SYLK,
    XATOM_WCF_TIFF,
    XATOM_WCF_WAVE,
    XATOM_image_bmp,
    XATOM_image_gif,
    XATOM_image_jpeg,
    XATOM_image_png,
    XATOM_text_html,
    XATOM_text_plain,
    XATOM_text_rtf,
    XATOM_text_richtext,
    XATOM_text_uri_list,
    NB_XATOMS
};

extern Atom X11DRV_Atoms[NB_XATOMS - FIRST_XATOM] DECLSPEC_HIDDEN;
extern Atom systray_atom DECLSPEC_HIDDEN;

#define x11drv_atom(name) (X11DRV_Atoms[XATOM_##name - FIRST_XATOM])

/* X11 event driver */

typedef void (*x11drv_event_handler)( HWND hwnd, XEvent *event );

extern void X11DRV_register_event_handler( int type, x11drv_event_handler handler, const char *name ) DECLSPEC_HIDDEN;

extern void X11DRV_ButtonPress( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_ButtonRelease( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_MotionNotify( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_EnterNotify( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_KeyEvent( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_KeymapNotify( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_DestroyNotify( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_SelectionRequest( HWND hWnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_SelectionClear( HWND hWnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_MappingNotify( HWND hWnd, XEvent *event ) DECLSPEC_HIDDEN;
extern void X11DRV_GenericEvent( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;

extern int xinput2_opcode DECLSPEC_HIDDEN;
extern Bool (*pXGetEventData)( Display *display, XEvent /*XGenericEventCookie*/ *event ) DECLSPEC_HIDDEN;
extern void (*pXFreeEventData)( Display *display, XEvent /*XGenericEventCookie*/ *event ) DECLSPEC_HIDDEN;

extern DWORD EVENT_x11_time_to_win32_time(Time time) DECLSPEC_HIDDEN;

/* X11 driver private messages, must be in the range 0x80001000..0x80001fff */
enum x11drv_window_messages
{
    WM_X11DRV_ACQUIRE_SELECTION = 0x80001000,
    WM_X11DRV_SET_WIN_FORMAT,
    WM_X11DRV_SET_WIN_REGION,
    WM_X11DRV_RESIZE_DESKTOP,
    WM_X11DRV_SET_CURSOR,
    WM_X11DRV_CLIP_CURSOR
};

/* _NET_WM_STATE properties that we keep track of */
enum x11drv_net_wm_state
{
    NET_WM_STATE_FULLSCREEN,
    NET_WM_STATE_ABOVE,
    NET_WM_STATE_MAXIMIZED,
    NET_WM_STATE_SKIP_PAGER,
    NET_WM_STATE_SKIP_TASKBAR,
    NB_NET_WM_STATES
};

/* x11drv private window data */
struct x11drv_win_data
{
    HWND        hwnd;           /* hwnd that this private data belongs to */
    Window      whole_window;   /* X window for the complete window */
    Window      client_window;  /* X window for the client area */
    Window      icon_window;    /* X window for the icon */
    Colormap    colormap;       /* Colormap for this window */
    VisualID    visualid;       /* visual id of the client window */
    XID         fbconfig_id;    /* fbconfig id for the GL drawable this hwnd uses */
    Drawable    gl_drawable;    /* Optional GL drawable for rendering the client area */
    Pixmap      pixmap;         /* Base pixmap for if gl_drawable is a GLXPixmap */
    RECT        window_rect;    /* USER window rectangle relative to parent */
    RECT        whole_rect;     /* X window rectangle for the whole window relative to parent */
    RECT        client_rect;    /* client area relative to parent */
    XIC         xic;            /* X input context */
    XWMHints   *wm_hints;       /* window manager hints */
    BOOL        managed : 1;    /* is window managed? */
    BOOL        mapped : 1;     /* is window mapped? (in either normal or iconic state) */
    BOOL        iconic : 1;     /* is window in iconic state? */
    BOOL        embedded : 1;   /* is window an XEMBED client? */
    BOOL        shaped : 1;     /* is window using a custom region shape? */
    int         wm_state;       /* current value of the WM_STATE property */
    DWORD       net_wm_state;   /* bit mask of active x11drv_net_wm_state values */
    Window      embedder;       /* window id of embedder */
    unsigned long configure_serial; /* serial number of last configure request */
    HBITMAP     hWMIconBitmap;
    HBITMAP     hWMIconMask;
};

extern struct x11drv_win_data *X11DRV_get_win_data( HWND hwnd ) DECLSPEC_HIDDEN;
extern struct x11drv_win_data *X11DRV_create_win_data( HWND hwnd ) DECLSPEC_HIDDEN;
extern Window X11DRV_get_whole_window( HWND hwnd ) DECLSPEC_HIDDEN;
extern XIC X11DRV_get_ic( HWND hwnd ) DECLSPEC_HIDDEN;

extern int pixelformat_from_fbconfig_id( XID fbconfig_id ) DECLSPEC_HIDDEN;
extern XVisualInfo *visual_from_fbconfig_id( XID fbconfig_id ) DECLSPEC_HIDDEN;
extern void mark_drawable_dirty( Drawable old, Drawable new ) DECLSPEC_HIDDEN;
extern Drawable create_glxpixmap( Display *display, XVisualInfo *vis, Pixmap parent ) DECLSPEC_HIDDEN;
extern void flush_gl_drawable( X11DRV_PDEVICE *physDev ) DECLSPEC_HIDDEN;

extern void wait_for_withdrawn_state( Display *display, struct x11drv_win_data *data, BOOL set ) DECLSPEC_HIDDEN;
extern Window init_clip_window(void) DECLSPEC_HIDDEN;
extern void update_user_time( Time time ) DECLSPEC_HIDDEN;
extern void update_net_wm_states( Display *display, struct x11drv_win_data *data ) DECLSPEC_HIDDEN;
extern void make_window_embedded( Display *display, struct x11drv_win_data *data ) DECLSPEC_HIDDEN;
extern void change_systray_owner( Display *display, Window systray_window ) DECLSPEC_HIDDEN;
extern void update_systray_balloon_position(void) DECLSPEC_HIDDEN;
extern HWND create_foreign_window( Display *display, Window window ) DECLSPEC_HIDDEN;

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

static inline BOOL is_window_rect_mapped( const RECT *rect )
{
    return (rect->left < virtual_screen_rect.right && rect->top < virtual_screen_rect.bottom &&
            rect->right > virtual_screen_rect.left && rect->bottom > virtual_screen_rect.top);
}

static inline BOOL is_window_rect_fullscreen( const RECT *rect )
{
    return (rect->left <= 0 && rect->right >= screen_width &&
            rect->top <= 0 && rect->bottom >= screen_height);
}

/* X context to associate a hwnd to an X window */
extern XContext winContext DECLSPEC_HIDDEN;

extern void X11DRV_InitClipboard(void) DECLSPEC_HIDDEN;
extern int CDECL X11DRV_AcquireClipboard(HWND hWndClipWindow) DECLSPEC_HIDDEN;
extern void X11DRV_Clipboard_Cleanup(void) DECLSPEC_HIDDEN;
extern void X11DRV_ResetSelectionOwner(void) DECLSPEC_HIDDEN;
extern void CDECL X11DRV_SetFocus( HWND hwnd ) DECLSPEC_HIDDEN;
extern void set_window_cursor( Window window, HCURSOR handle ) DECLSPEC_HIDDEN;
extern void sync_window_cursor( Window window ) DECLSPEC_HIDDEN;
extern LRESULT clip_cursor_notify( HWND hwnd, HWND new_clip_hwnd ) DECLSPEC_HIDDEN;
extern void ungrab_clipping_window(void) DECLSPEC_HIDDEN;
extern void reset_clipping_window(void) DECLSPEC_HIDDEN;
extern BOOL clip_fullscreen_window( HWND hwnd, BOOL reset ) DECLSPEC_HIDDEN;
extern void X11DRV_InitKeyboard( Display *display ) DECLSPEC_HIDDEN;
extern DWORD CDECL X11DRV_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                       DWORD mask, DWORD flags ) DECLSPEC_HIDDEN;

typedef int (*x11drv_error_callback)( Display *display, XErrorEvent *event, void *arg );

extern void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg ) DECLSPEC_HIDDEN;
extern int X11DRV_check_error(void) DECLSPEC_HIDDEN;
extern void X11DRV_X_to_window_rect( struct x11drv_win_data *data, RECT *rect ) DECLSPEC_HIDDEN;
extern void xinerama_init( unsigned int width, unsigned int height ) DECLSPEC_HIDDEN;

extern void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height ) DECLSPEC_HIDDEN;
extern void X11DRV_resize_desktop(unsigned int width, unsigned int height) DECLSPEC_HIDDEN;
extern void X11DRV_Settings_AddDepthModes(void) DECLSPEC_HIDDEN;
extern void X11DRV_Settings_AddOneMode(unsigned int width, unsigned int height, unsigned int bpp, unsigned int freq) DECLSPEC_HIDDEN;
extern int X11DRV_Settings_CreateDriver(LPDDHALINFO info) DECLSPEC_HIDDEN;
extern LPDDHALMODEINFO X11DRV_Settings_CreateModes(unsigned int max_modes, int reserve_depths) DECLSPEC_HIDDEN;
unsigned int X11DRV_Settings_GetModeCount(void) DECLSPEC_HIDDEN;
void X11DRV_Settings_Init(void) DECLSPEC_HIDDEN;
LPDDHALMODEINFO X11DRV_Settings_SetHandlers(const char *name,
                                            int (*pNewGCM)(void),
                                            LONG (*pNewSCM)(int),
                                            unsigned int nmodes,
                                            int reserve_depths) DECLSPEC_HIDDEN;

/* XIM support */
extern BOOL X11DRV_InitXIM( const char *input_style ) DECLSPEC_HIDDEN;
extern XIC X11DRV_CreateIC(XIM xim, struct x11drv_win_data *data) DECLSPEC_HIDDEN;
extern void X11DRV_SetupXIM(void) DECLSPEC_HIDDEN;
extern void X11DRV_XIMLookupChars( const char *str, DWORD count ) DECLSPEC_HIDDEN;
extern void X11DRV_ForceXIMReset(HWND hwnd) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetPreeditState(HWND hwnd, BOOL fOpen) DECLSPEC_HIDDEN;

#define XEMBED_MAPPED  (1 << 0)

#endif  /* __WINE_X11DRV_H */
