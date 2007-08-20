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
#include "wine/list.h"

#define MAX_PIXELFORMATS 8
#define MAX_DASHLEN 16

struct tagCURSORICONINFO;
struct dce;

extern void wine_tsx11_lock(void);
extern void wine_tsx11_unlock(void);

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

  /* X physical bitmap */
typedef struct
{
    HBITMAP      hbitmap;
    Pixmap       pixmap;
    XID          glxpixmap;
    int          pixmap_depth;
    /* the following fields are only used for DIB section bitmaps */
    int          status, p_status;  /* mapping status */
    XImage      *image;             /* cached XImage */
    int         *colorMap;          /* color map info */
    int          nColorMap;
    CRITICAL_SECTION lock;          /* GDI access lock */
#ifdef HAVE_LIBXXSHM
    XShmSegmentInfo shminfo;        /* shared memory segment info */
#endif
    struct list   entry;            /* Entry in global DIB list */
    BYTE         *base;             /* Base address */
    SIZE_T        size;             /* Size in bytes */
} X_PHYSBITMAP;

  /* X physical font */
typedef UINT	 X_PHYSFONT;

typedef struct tagXRENDERINFO *XRENDERINFO;

  /* X physical device */
typedef struct
{
    HDC           hdc;
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
    int           exposures;   /* count of graphics exposures operations */
    struct dce   *dce;         /* opaque pointer to DCE */
    int           current_pf;
    XRENDERINFO   xrender;
} X11DRV_PDEVICE;


  /* GCs used for B&W and color bitmap operations */
extern GC BITMAP_monoGC, BITMAP_colorGC;
extern X_PHYSBITMAP BITMAP_stock_phys_bitmap;  /* phys bitmap for the default stock bitmap */

#define BITMAP_GC(physBitmap) (((physBitmap)->pixmap_depth == 1) ? BITMAP_monoGC : BITMAP_colorGC)

/* Wine driver X11 functions */

extern BOOL X11DRV_AlphaBlend( X11DRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                               INT widthDst, INT heightDst,
                               X11DRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc,
                               INT widthSrc, INT heightSrc, BLENDFUNCTION blendfn );
extern BOOL X11DRV_BitBlt( X11DRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                             INT width, INT height, X11DRV_PDEVICE *physDevSrc,
                             INT xSrc, INT ySrc, DWORD rop );
extern BOOL X11DRV_EnumDeviceFonts( X11DRV_PDEVICE *physDev, LPLOGFONTW plf,
                                    FONTENUMPROCW dfeproc, LPARAM lp );
extern LONG X11DRV_GetBitmapBits( HBITMAP hbitmap, void *bits, LONG count );
extern BOOL X11DRV_GetCharWidth( X11DRV_PDEVICE *physDev, UINT firstChar,
                                   UINT lastChar, LPINT buffer );
extern BOOL X11DRV_GetDCOrgEx( X11DRV_PDEVICE *physDev, LPPOINT lpp );
extern BOOL X11DRV_GetTextExtentExPoint( X11DRV_PDEVICE *physDev, LPCWSTR str, INT count,
                                  INT maxExt, LPINT lpnFit, LPINT alpDx, LPSIZE size );
extern BOOL X11DRV_GetTextMetrics(X11DRV_PDEVICE *physDev, TEXTMETRICW *metrics);
extern BOOL X11DRV_PatBlt( X11DRV_PDEVICE *physDev, INT left, INT top,
                             INT width, INT height, DWORD rop );
extern BOOL X11DRV_StretchBlt( X11DRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                                 INT widthDst, INT heightDst,
                                 X11DRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc,
                                 INT widthSrc, INT heightSrc, DWORD rop );
extern BOOL X11DRV_LineTo( X11DRV_PDEVICE *physDev, INT x, INT y);
extern BOOL X11DRV_Arc( X11DRV_PDEVICE *physDev, INT left, INT top, INT right,
			  INT bottom, INT xstart, INT ystart, INT xend,
			  INT yend );
extern BOOL X11DRV_Pie( X11DRV_PDEVICE *physDev, INT left, INT top, INT right,
			  INT bottom, INT xstart, INT ystart, INT xend,
			  INT yend );
extern BOOL X11DRV_Chord( X11DRV_PDEVICE *physDev, INT left, INT top,
			    INT right, INT bottom, INT xstart,
			    INT ystart, INT xend, INT yend );
extern BOOL X11DRV_Ellipse( X11DRV_PDEVICE *physDev, INT left, INT top,
			      INT right, INT bottom );
extern BOOL X11DRV_Rectangle(X11DRV_PDEVICE *physDev, INT left, INT top,
			      INT right, INT bottom);
extern BOOL X11DRV_RoundRect( X11DRV_PDEVICE *physDev, INT left, INT top,
				INT right, INT bottom, INT ell_width,
				INT ell_height );
extern COLORREF X11DRV_SetPixel( X11DRV_PDEVICE *physDev, INT x, INT y,
				 COLORREF color );
extern COLORREF X11DRV_GetPixel( X11DRV_PDEVICE *physDev, INT x, INT y);
extern BOOL X11DRV_PaintRgn( X11DRV_PDEVICE *physDev, HRGN hrgn );
extern BOOL X11DRV_Polyline( X11DRV_PDEVICE *physDev,const POINT* pt,INT count);
extern BOOL X11DRV_Polygon( X11DRV_PDEVICE *physDev, const POINT* pt, INT count );
extern BOOL X11DRV_PolyPolygon( X11DRV_PDEVICE *physDev, const POINT* pt,
				  const INT* counts, UINT polygons);
extern BOOL X11DRV_PolyPolyline( X11DRV_PDEVICE *physDev, const POINT* pt,
				  const DWORD* counts, DWORD polylines);

extern COLORREF X11DRV_SetBkColor( X11DRV_PDEVICE *physDev, COLORREF color );
extern COLORREF X11DRV_SetTextColor( X11DRV_PDEVICE *physDev, COLORREF color );
extern BOOL X11DRV_ExtFloodFill( X11DRV_PDEVICE *physDev, INT x, INT y,
				   COLORREF color, UINT fillType );
extern BOOL X11DRV_ExtTextOut( X11DRV_PDEVICE *physDev, INT x, INT y,
				 UINT flags, const RECT *lprect,
				 LPCWSTR str, UINT count, const INT *lpDx );
extern LONG X11DRV_SetBitmapBits( HBITMAP hbitmap, const void *bits, LONG count );
extern void X11DRV_SetDeviceClipping( X11DRV_PDEVICE *physDev, HRGN vis_rgn, HRGN clip_rgn );
extern INT X11DRV_SetDIBitsToDevice( X11DRV_PDEVICE *physDev, INT xDest,
				       INT yDest, DWORD cx, DWORD cy,
				       INT xSrc, INT ySrc,
				       UINT startscan, UINT lines,
				       LPCVOID bits, const BITMAPINFO *info,
				       UINT coloruse );
extern BOOL X11DRV_GetDeviceGammaRamp( X11DRV_PDEVICE *physDev, LPVOID ramp );
extern BOOL X11DRV_SetDeviceGammaRamp( X11DRV_PDEVICE *physDev, LPVOID ramp );

/* OpenGL / X11 driver functions */
extern int X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev,
		                      const PIXELFORMATDESCRIPTOR *pppfd);
extern int X11DRV_DescribePixelFormat(X11DRV_PDEVICE *physDev,
		                        int iPixelFormat, UINT nBytes,
					PIXELFORMATDESCRIPTOR *ppfd);
extern int X11DRV_GetPixelFormat(X11DRV_PDEVICE *physDev);
extern BOOL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev);

/* X11 driver internal functions */

extern void X11DRV_Xcursor_Init(void);
extern void X11DRV_BITMAP_Init(void);
extern void X11DRV_FONT_Init( int log_pixels_x, int log_pixels_y );

extern int X11DRV_DIB_BitmapInfoSize( const BITMAPINFO * info, WORD coloruse );
extern XImage *X11DRV_DIB_CreateXImage( int width, int height, int depth );
extern HGLOBAL X11DRV_DIB_CreateDIBFromBitmap(HDC hdc, HBITMAP hBmp);
extern HGLOBAL X11DRV_DIB_CreateDIBFromPixmap(Pixmap pixmap, HDC hdc);
extern Pixmap X11DRV_DIB_CreatePixmapFromDIB( HGLOBAL hPackedDIB, HDC hdc );
extern X_PHYSBITMAP *X11DRV_get_phys_bitmap( HBITMAP hbitmap );
extern X_PHYSBITMAP *X11DRV_init_phys_bitmap( HBITMAP hbitmap );
extern Pixmap X11DRV_get_pixmap( HBITMAP hbitmap );

extern RGNDATA *X11DRV_GetRegionData( HRGN hrgn, HDC hdc_lptodp );

extern BOOL X11DRV_SetupGCForPatBlt( X11DRV_PDEVICE *physDev, GC gc, BOOL fMapColors );
extern BOOL X11DRV_SetupGCForBrush( X11DRV_PDEVICE *physDev );
extern BOOL X11DRV_SetupGCForPen( X11DRV_PDEVICE *physDev );
extern BOOL X11DRV_SetupGCForText( X11DRV_PDEVICE *physDev );
extern INT X11DRV_XWStoDS( X11DRV_PDEVICE *physDev, INT width );
extern INT X11DRV_YWStoDS( X11DRV_PDEVICE *physDev, INT height );

extern const int X11DRV_XROPfunction[];

extern void _XInitImageFuncPtrs(XImage *);

extern int client_side_with_core;
extern int client_side_with_render;
extern int client_side_antialias_with_core;
extern int client_side_antialias_with_render;
extern int using_client_side_fonts;
extern void X11DRV_XRender_Init(void);
extern void X11DRV_XRender_Finalize(void);
extern BOOL X11DRV_XRender_SelectFont(X11DRV_PDEVICE*, HFONT);
extern void X11DRV_XRender_DeleteDC(X11DRV_PDEVICE*);
extern BOOL X11DRV_XRender_ExtTextOut(X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
				      const RECT *lprect, LPCWSTR wstr,
				      UINT count, const INT *lpDx);
extern void X11DRV_XRender_UpdateDrawable(X11DRV_PDEVICE *physDev);

extern XVisualInfo *X11DRV_setup_opengl_visual(Display *display);
extern Drawable get_glxdrawable(X11DRV_PDEVICE *physDev);
extern BOOL destroy_glxpixmap(XID glxpixmap);

/* XIM support */
extern XIC X11DRV_CreateIC(XIM xim, Display *display, Window win);
extern XIM X11DRV_SetupXIM(Display *display, const char *input_style);
extern void X11DRV_XIMLookupChars( const char *str, DWORD count );

extern void X11DRV_XDND_EnterEvent( HWND hWnd, XClientMessageEvent *event );
extern void X11DRV_XDND_PositionEvent( HWND hWnd, XClientMessageEvent *event );
extern void X11DRV_XDND_DropEvent( HWND hWnd, XClientMessageEvent *event );
extern void X11DRV_XDND_LeaveEvent( HWND hWnd, XClientMessageEvent *event );

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

extern const dib_conversions dib_normal, dib_src_byteswap, dib_dst_byteswap;

extern INT X11DRV_DIB_MaskToShift(DWORD mask);
extern INT X11DRV_CoerceDIBSection(X11DRV_PDEVICE *physDev,INT,BOOL);
extern INT X11DRV_LockDIBSection(X11DRV_PDEVICE *physDev,INT,BOOL);
extern void X11DRV_UnlockDIBSection(X11DRV_PDEVICE *physDev,BOOL);

extern void X11DRV_DIB_DeleteDIBSection(X_PHYSBITMAP *physBitmap, DIBSECTION *dib);
extern void X11DRV_DIB_CopyDIBSection(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                                      DWORD xSrc, DWORD ySrc, DWORD xDest, DWORD yDest,
                                      DWORD width, DWORD height);
struct _DCICMD;
extern INT X11DRV_DCICommand(INT cbInput, const struct _DCICMD *lpCmd, LPVOID lpOutData);

/**************************************************************************
 * X11 GDI driver
 */

extern void X11DRV_GDI_Finalize(void);

extern Display *gdi_display;  /* display to use for all GDI functions */

/* X11 GDI palette driver */

#define X11DRV_PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define X11DRV_PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define X11DRV_PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */
#define X11DRV_PALETTE_WHITESET 0x2000

extern Colormap X11DRV_PALETTE_PaletteXColormap;
extern UINT16 X11DRV_PALETTE_PaletteFlags;

extern int *X11DRV_PALETTE_PaletteToXPixel;
extern int *X11DRV_PALETTE_XPixelToPalette;

extern int X11DRV_PALETTE_mapEGAPixel[16];

extern int X11DRV_PALETTE_Init(void);
extern void X11DRV_PALETTE_Cleanup(void);
extern BOOL X11DRV_IsSolidColor(COLORREF color);

extern COLORREF X11DRV_PALETTE_ToLogical(int pixel);
extern int X11DRV_PALETTE_ToPhysical(X11DRV_PDEVICE *physDev, COLORREF color);

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
    X11DRV_GET_DCE,          /* get the DCE pointer */
    X11DRV_SET_DCE,          /* set the DCE pointer */
    X11DRV_GET_GLX_DRAWABLE, /* get current glx drawable for a DC */
    X11DRV_SYNC_PIXMAP       /* sync the dibsection to its pixmap */
};

struct x11drv_escape_set_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_SET_DRAWABLE) */
    Drawable                 drawable;     /* X drawable */
    int                      mode;         /* ClipByChildren or IncludeInferiors */
    RECT                     dc_rect;      /* DC rectangle relative to drawable */
    RECT                     drawable_rect;/* Drawable rectangle relative to screen */
};

struct x11drv_escape_set_dce
{
    enum x11drv_escape_codes code;            /* escape code (X11DRV_SET_DRAWABLE) */
    struct dce              *dce;             /* pointer to DCE (opaque ptr for GDI) */
};

/**************************************************************************
 * X11 USER driver
 */

struct x11drv_thread_data
{
    Display *display;
    int      process_event_count;  /* recursion count for event processing */
    Cursor   cursor;               /* current cursor */
    Window   cursor_window;        /* current window that contains the cursor */
    Window   grab_window;          /* window that currently grabs the mouse */
    HWND     last_focus;           /* last window that had focus */
    XIM      xim;                  /* input method */
    Window   selection_wnd;        /* window used for selection interactions */
};

extern struct x11drv_thread_data *x11drv_init_thread_data(void);
extern DWORD thread_data_tls_index;

static inline struct x11drv_thread_data *x11drv_thread_data(void)
{
    struct x11drv_thread_data *data = TlsGetValue( thread_data_tls_index );
    if (!data) data = x11drv_init_thread_data();
    return data;
}

static inline Display *thread_display(void) { return x11drv_thread_data()->display; }

extern Visual *visual;
extern Window root_window;
extern unsigned int screen_width;
extern unsigned int screen_height;
extern unsigned int screen_depth;
extern RECT virtual_screen_rect;
extern unsigned int text_caps;
extern int dxgrab;
extern int use_xkb;
extern int use_take_focus;
extern int use_primary_selection;
extern int managed_mode;
extern int private_color_map;
extern int primary_monitor;
extern int copy_default_colors;
extern int alloc_system_colors;
extern int xrender_error_base;

extern BYTE key_state_table[256];
extern POINT cursor_pos;

/* atoms */

enum x11drv_atoms
{
    FIRST_XATOM = XA_LAST_PREDEFINED + 1,
    XATOM_CLIPBOARD = FIRST_XATOM,
    XATOM_COMPOUND_TEXT,
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
    XATOM_WM_TAKE_FOCUS,
    XATOM_KWM_DOCKWINDOW,
    XATOM_DndProtocol,
    XATOM_DndSelection,
    XATOM__MOTIF_WM_HINTS,
    XATOM__KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR,
    XATOM__NET_SYSTEM_TRAY_OPCODE,
    XATOM__NET_SYSTEM_TRAY_S0,
    XATOM__NET_WM_MOVERESIZE,
    XATOM__NET_WM_NAME,
    XATOM__NET_WM_PID,
    XATOM__NET_WM_PING,
    XATOM__NET_WM_STATE,
    XATOM__NET_WM_STATE_FULLSCREEN,
    XATOM__NET_WM_WINDOW_TYPE,
    XATOM__NET_WM_WINDOW_TYPE_DIALOG,
    XATOM__NET_WM_WINDOW_TYPE_NORMAL,
    XATOM__NET_WM_WINDOW_TYPE_UTILITY,
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
    XATOM_WCF_DIB,
    XATOM_image_gif,
    XATOM_text_html,
    XATOM_text_plain,
    XATOM_text_rtf,
    XATOM_text_richtext,
    XATOM_text_uri_list,
    NB_XATOMS
};

extern Atom X11DRV_Atoms[NB_XATOMS - FIRST_XATOM];

#define x11drv_atom(name) (X11DRV_Atoms[XATOM_##name - FIRST_XATOM])

/* X11 event driver */

typedef void (*x11drv_event_handler)( HWND hwnd, XEvent *event );

extern void X11DRV_register_event_handler( int type, x11drv_event_handler handler );

extern void X11DRV_ButtonPress( HWND hwnd, XEvent *event );
extern void X11DRV_ButtonRelease( HWND hwnd, XEvent *event );
extern void X11DRV_MotionNotify( HWND hwnd, XEvent *event );
extern void X11DRV_EnterNotify( HWND hwnd, XEvent *event );
extern void X11DRV_KeyEvent( HWND hwnd, XEvent *event );
extern void X11DRV_KeymapNotify( HWND hwnd, XEvent *event );
extern void X11DRV_Expose( HWND hwnd, XEvent *event );
extern void X11DRV_MapNotify( HWND hwnd, XEvent *event );
extern void X11DRV_UnmapNotify( HWND hwnd, XEvent *event );
extern void X11DRV_ConfigureNotify( HWND hwnd, XEvent *event );
extern void X11DRV_SelectionRequest( HWND hWnd, XEvent *event );
extern void X11DRV_SelectionClear( HWND hWnd, XEvent *event );
extern void X11DRV_MappingNotify( HWND hWnd, XEvent *event );

extern DWORD EVENT_x11_time_to_win32_time(Time time);

/* X11 driver private messages, must be in the range 0x80001000..0x80001fff */
enum x11drv_window_messages
{
    WM_X11DRV_ACQUIRE_SELECTION = 0x80001000,
    WM_X11DRV_DELETE_WINDOW
};

/* x11drv private window data */
struct x11drv_win_data
{
    HWND        hwnd;           /* hwnd that this private data belongs to */
    Window      whole_window;   /* X window for the complete window */
    Window      icon_window;    /* X window for the icon */
    RECT        window_rect;    /* USER window rectangle relative to parent */
    RECT        whole_rect;     /* X window rectangle for the whole window relative to parent */
    RECT        client_rect;    /* client area relative to whole window */
    XIC         xic;            /* X input context */
    XWMHints   *wm_hints;       /* window manager hints */
    BOOL        managed;        /* is window managed? */
    struct dce *dce;            /* DCE for CS_OWNDC or CS_CLASSDC windows */
    unsigned int lock_changes;  /* lock count for X11 change requests */
    HBITMAP     hWMIconBitmap;
    HBITMAP     hWMIconMask;
};

extern struct x11drv_win_data *X11DRV_get_win_data( HWND hwnd );
extern Window X11DRV_get_whole_window( HWND hwnd );
extern BOOL X11DRV_is_window_rect_mapped( const RECT *rect );
extern XIC X11DRV_get_ic( HWND hwnd );

extern void alloc_window_dce( struct x11drv_win_data *data );
extern void free_window_dce( struct x11drv_win_data *data );
extern void invalidate_dce( HWND hwnd, const RECT *rect );

/* X context to associate a hwnd to an X window */
extern XContext winContext;

extern void X11DRV_InitClipboard(void);
extern int X11DRV_AcquireClipboard(HWND hWndClipWindow);
extern void X11DRV_ResetSelectionOwner(void);
extern void X11DRV_SetFocus( HWND hwnd );
extern Cursor X11DRV_GetCursor( Display *display, struct tagCURSORICONINFO *ptr );
extern BOOL X11DRV_ClipCursor( LPCRECT clip );
extern void X11DRV_InitKeyboard(void);
extern void X11DRV_send_keyboard_input( WORD wVk, WORD wScan, DWORD dwFlags, DWORD time,
                                        DWORD dwExtraInfo, UINT injected_flags );
extern void X11DRV_send_mouse_input( HWND hwnd, DWORD flags, DWORD x, DWORD y,
                                     DWORD data, DWORD time, DWORD extra_info, UINT injected_flags );
extern DWORD X11DRV_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                 DWORD mask, DWORD flags );

typedef int (*x11drv_error_callback)( Display *display, XErrorEvent *event, void *arg );

extern void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg );
extern int X11DRV_check_error(void);
extern BOOL is_window_managed( HWND hwnd, const RECT *window_rect );
extern void X11DRV_set_iconic_state( HWND hwnd );
extern void X11DRV_window_to_X_rect( struct x11drv_win_data *data, RECT *rect );
extern void X11DRV_X_to_window_rect( struct x11drv_win_data *data, RECT *rect );
extern void X11DRV_sync_window_style( Display *display, struct x11drv_win_data *data );
extern void X11DRV_sync_window_position( Display *display, struct x11drv_win_data *data,
                                         UINT swp_flags, const RECT *new_client_rect,
                                         const RECT *new_whole_rect );
extern BOOL X11DRV_SetWindowPos( HWND hwnd, HWND insert_after, const RECT *rectWindow,
                                   const RECT *rectClient, UINT swp_flags, const RECT *validRects );
extern void X11DRV_set_wm_hints( Display *display, struct x11drv_win_data *data );
extern void xinerama_init(void);

extern void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height );
extern void X11DRV_handle_desktop_resize(unsigned int width, unsigned int height);
extern void X11DRV_Settings_AddDepthModes(void);
extern void X11DRV_Settings_AddOneMode(unsigned int width, unsigned int height, unsigned int bpp, unsigned int freq);
extern int X11DRV_Settings_CreateDriver(LPDDHALINFO info);
extern LPDDHALMODEINFO X11DRV_Settings_CreateModes(unsigned int max_modes, int reserve_depths);
unsigned int X11DRV_Settings_GetModeCount(void);
void X11DRV_Settings_Init(void);
extern void X11DRV_Settings_SetDefaultMode(int mode);
LPDDHALMODEINFO X11DRV_Settings_SetHandlers(const char *name,
                                            int (*pNewGCM)(void),
                                            LONG (*pNewSCM)(int),
                                            unsigned int nmodes,
                                            int reserve_depths);

extern void X11DRV_DDHAL_SwitchMode(DWORD dwModeIndex, LPVOID fb_addr, LPVIDMEM fb_mem);

#endif  /* __WINE_X11DRV_H */
