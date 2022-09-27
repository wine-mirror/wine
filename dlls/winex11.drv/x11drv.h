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

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#ifdef HAVE_X11_EXTENSIONS_XINPUT2_H
#include <X11/extensions/XInput2.h>
#endif

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

/* avoid conflict with processthreadsapi.h */
#undef ControlMask

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"
#include "unixlib.h"
#include "wine/list.h"

#define MAX_DASHLEN 16

#define WINE_XDND_VERSION 5

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

typedef struct {
    int shift;
    int scale;
    int max;
} ChannelShift;

typedef struct
{
    ChannelShift physicalRed, physicalGreen, physicalBlue;
    ChannelShift logicalRed, logicalGreen, logicalBlue;
} ColorShifts;

  /* X physical device */
typedef struct
{
    struct gdi_physdev dev;
    GC            gc;          /* X Window GC */
    Drawable      drawable;
    RECT          dc_rect;       /* DC rectangle relative to drawable */
    RECT         *bounds;        /* Graphics bounds */
    HRGN          region;        /* Device region (visible region & clip region) */
    X_PHYSPEN     pen;
    X_PHYSBRUSH   brush;
    int           depth;       /* bit depth of the DC */
    ColorShifts  *color_shifts; /* color shifts of the DC */
    int           exposures;   /* count of graphics exposures operations */
} X11DRV_PDEVICE;

#define GAMMA_RAMP_SIZE 256

struct x11drv_gamma_ramp
{
    WORD red[GAMMA_RAMP_SIZE];
    WORD green[GAMMA_RAMP_SIZE];
    WORD blue[GAMMA_RAMP_SIZE];
};

static inline X11DRV_PDEVICE *get_x11drv_dev( PHYSDEV dev )
{
    return (X11DRV_PDEVICE *)dev;
}

static inline void reset_bounds( RECT *bounds )
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
}

static inline void add_bounds_rect( RECT *bounds, const RECT *rect )
{
    if (rect->left >= rect->right || rect->top >= rect->bottom) return;
    bounds->left   = min( bounds->left, rect->left );
    bounds->top    = min( bounds->top, rect->top );
    bounds->right  = max( bounds->right, rect->right );
    bounds->bottom = max( bounds->bottom, rect->bottom );
}

/* Wine driver X11 functions */

extern BOOL CDECL X11DRV_Arc( PHYSDEV dev, INT left, INT top, INT right,
                              INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL X11DRV_D3DKMTCheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL X11DRV_D3DKMTCloseAdapter( const D3DKMT_CLOSEADAPTER *desc ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL X11DRV_D3DKMTOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID *desc ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL X11DRV_D3DKMTQueryVideoMemoryInfo( D3DKMT_QUERYVIDEOMEMORYINFO *desc ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL X11DRV_D3DKMTSetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_GetDeviceGammaRamp( PHYSDEV dev, LPVOID ramp ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_GetICMProfile( PHYSDEV dev, BOOL allow_default, LPDWORD size, LPWSTR filename ) DECLSPEC_HIDDEN;
extern DWORD CDECL X11DRV_GetImage( PHYSDEV dev, BITMAPINFO *info,
                                    struct gdi_image_bits *bits, struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern COLORREF CDECL X11DRV_GetNearestColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern UINT CDECL X11DRV_GetSystemPaletteEntries( PHYSDEV dev, UINT start, UINT count, LPPALETTEENTRY entries ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                       void *grad_array, ULONG ngrad, ULONG mode ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_LineTo( PHYSDEV dev, INT x, INT y) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_Pie( PHYSDEV dev, INT left, INT top, INT right,
                              INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polygons) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polylines) DECLSPEC_HIDDEN;
extern DWORD CDECL X11DRV_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                                    const struct gdi_image_bits *bits, struct bitblt_coords *src,
                                    struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern UINT CDECL X11DRV_RealizeDefaultPalette( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern UINT CDECL X11DRV_RealizePalette( PHYSDEV dev, HPALETTE hpal, BOOL primary ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                    INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern HBRUSH CDECL X11DRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern HPEN CDECL X11DRV_SelectPen( PHYSDEV dev, HPEN hpen, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern COLORREF CDECL X11DRV_SetDCBrushColor( PHYSDEV dev, COLORREF crColor ) DECLSPEC_HIDDEN;
extern COLORREF CDECL X11DRV_SetDCPenColor( PHYSDEV dev, COLORREF crColor ) DECLSPEC_HIDDEN;
extern void CDECL X11DRV_SetDeviceClipping( PHYSDEV dev, HRGN rgn ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_SetDeviceGammaRamp( PHYSDEV dev, LPVOID ramp ) DECLSPEC_HIDDEN;
extern COLORREF CDECL X11DRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                     PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL X11DRV_UnrealizePalette( HPALETTE hpal ) DECLSPEC_HIDDEN;

extern BOOL X11DRV_ActivateKeyboardLayout( HKL hkl, UINT flags ) DECLSPEC_HIDDEN;
extern void X11DRV_Beep(void) DECLSPEC_HIDDEN;
extern INT X11DRV_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size ) DECLSPEC_HIDDEN;
extern UINT X11DRV_MapVirtualKeyEx( UINT code, UINT map_type, HKL hkl ) DECLSPEC_HIDDEN;
extern INT X11DRV_ToUnicodeEx( UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                               LPWSTR bufW, int bufW_size, UINT flags, HKL hkl ) DECLSPEC_HIDDEN;
extern SHORT X11DRV_VkKeyScanEx( WCHAR wChar, HKL hkl ) DECLSPEC_HIDDEN;
extern void X11DRV_DestroyCursorIcon( HCURSOR handle ) DECLSPEC_HIDDEN;
extern void X11DRV_SetCursor( HCURSOR handle ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetCursorPos( INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetCursorPos( LPPOINT pos ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_ClipCursor( LPCRECT clip ) DECLSPEC_HIDDEN;
extern LONG X11DRV_ChangeDisplaySettings( LPDEVMODEW displays, HWND hwnd, DWORD flags, LPVOID lpvoid ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetCurrentDisplaySettings( LPCWSTR name, LPDEVMODEW devmode ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_UpdateDisplayDevices( const struct gdi_device_manager *device_manager,
                                         BOOL force, void *param ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_CreateDesktopWindow( HWND hwnd ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_CreateWindow( HWND hwnd ) DECLSPEC_HIDDEN;
extern LRESULT X11DRV_DesktopWindowProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) DECLSPEC_HIDDEN;
extern void X11DRV_DestroyWindow( HWND hwnd ) DECLSPEC_HIDDEN;
extern void X11DRV_FlashWindowEx( PFLASHWINFO pfinfo ) DECLSPEC_HIDDEN;
extern void X11DRV_GetDC( HDC hdc, HWND hwnd, HWND top, const RECT *win_rect,
                          const RECT *top_rect, DWORD flags ) DECLSPEC_HIDDEN;
extern void X11DRV_ReleaseDC( HWND hwnd, HDC hdc ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_ScrollDC( HDC hdc, INT dx, INT dy, HRGN update ) DECLSPEC_HIDDEN;
extern void X11DRV_SetCapture( HWND hwnd, UINT flags ) DECLSPEC_HIDDEN;
extern void X11DRV_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha,
                                               DWORD flags ) DECLSPEC_HIDDEN;
extern void X11DRV_SetParent( HWND hwnd, HWND parent, HWND old_parent ) DECLSPEC_HIDDEN;
extern void X11DRV_SetWindowIcon( HWND hwnd, UINT type, HICON icon ) DECLSPEC_HIDDEN;
extern void X11DRV_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw ) DECLSPEC_HIDDEN;
extern void X11DRV_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style ) DECLSPEC_HIDDEN;
extern void X11DRV_SetWindowText( HWND hwnd, LPCWSTR text ) DECLSPEC_HIDDEN;
extern UINT X11DRV_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp ) DECLSPEC_HIDDEN;
extern LRESULT X11DRV_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam ) DECLSPEC_HIDDEN;
extern LRESULT X11DRV_ClipboardWindowProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) DECLSPEC_HIDDEN;
extern void X11DRV_UpdateClipboard(void) DECLSPEC_HIDDEN;
extern BOOL X11DRV_UpdateLayeredWindow( HWND hwnd, const UPDATELAYEREDWINDOWINFO *info,
                                        const RECT *window_rect ) DECLSPEC_HIDDEN;
extern LRESULT X11DRV_WindowMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                      const RECT *window_rect, const RECT *client_rect, RECT *visible_rect,
                                      struct window_surface **surface ) DECLSPEC_HIDDEN;
extern void X11DRV_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                     const RECT *rectWindow, const RECT *rectClient,
                                     const RECT *visible_rect, const RECT *valid_rects,
                                     struct window_surface *surface ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SystemParametersInfo( UINT action, UINT int_param, void *ptr_param,
                                         UINT flags ) DECLSPEC_HIDDEN;
extern void X11DRV_ThreadDetach(void) DECLSPEC_HIDDEN;

/* X11 driver internal functions */

extern void X11DRV_Xcursor_Init(void) DECLSPEC_HIDDEN;
extern void X11DRV_XInput2_Init(void) DECLSPEC_HIDDEN;

extern DWORD copy_image_bits( BITMAPINFO *info, BOOL is_r8g8b8, XImage *image,
                              const struct gdi_image_bits *src_bits, struct gdi_image_bits *dst_bits,
                              struct bitblt_coords *coords, const int *mapping, unsigned int zeropad_mask ) DECLSPEC_HIDDEN;
extern Pixmap create_pixmap_from_image( HDC hdc, const XVisualInfo *vis, const BITMAPINFO *info,
                                        const struct gdi_image_bits *bits, UINT coloruse ) DECLSPEC_HIDDEN;
extern DWORD get_pixmap_image( Pixmap pixmap, int width, int height, const XVisualInfo *vis,
                               BITMAPINFO *info, struct gdi_image_bits *bits ) DECLSPEC_HIDDEN;
extern struct window_surface *create_surface( Window window, const XVisualInfo *vis, const RECT *rect,
                                              COLORREF color_key, BOOL use_alpha ) DECLSPEC_HIDDEN;
extern void set_surface_color_key( struct window_surface *window_surface, COLORREF color_key ) DECLSPEC_HIDDEN;
extern HRGN expose_surface( struct window_surface *window_surface, const RECT *rect ) DECLSPEC_HIDDEN;

extern RGNDATA *X11DRV_GetRegionData( HRGN hrgn, HDC hdc_lptodp ) DECLSPEC_HIDDEN;
extern BOOL add_extra_clipping_region( X11DRV_PDEVICE *dev, HRGN rgn ) DECLSPEC_HIDDEN;
extern void restore_clipping_region( X11DRV_PDEVICE *dev ) DECLSPEC_HIDDEN;
extern void add_device_bounds( X11DRV_PDEVICE *dev, const RECT *rect ) DECLSPEC_HIDDEN;

extern void execute_rop( X11DRV_PDEVICE *physdev, Pixmap src_pixmap, GC gc, const RECT *visrect, DWORD rop ) DECLSPEC_HIDDEN;

extern BOOL X11DRV_SetupGCForPatBlt( X11DRV_PDEVICE *physDev, GC gc, BOOL fMapColors ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetupGCForBrush( X11DRV_PDEVICE *physDev ) DECLSPEC_HIDDEN;
extern INT X11DRV_XWStoDS( HDC hdc, INT width ) DECLSPEC_HIDDEN;
extern INT X11DRV_YWStoDS( HDC hdc, INT height ) DECLSPEC_HIDDEN;

extern BOOL client_side_graphics DECLSPEC_HIDDEN;
extern BOOL client_side_with_render DECLSPEC_HIDDEN;
extern BOOL shape_layered_windows DECLSPEC_HIDDEN;
extern const struct gdi_dc_funcs *X11DRV_XRender_Init(void) DECLSPEC_HIDDEN;

extern struct opengl_funcs *get_glx_driver(UINT) DECLSPEC_HIDDEN;
extern const struct vulkan_funcs *get_vulkan_driver(UINT) DECLSPEC_HIDDEN;

extern struct format_entry *import_xdnd_selection( Display *display, Window win, Atom selection,
                                                   Atom *targets, UINT count,
                                                   size_t *size ) DECLSPEC_HIDDEN;

/**************************************************************************
 * X11 GDI driver
 */

extern Display *gdi_display DECLSPEC_HIDDEN;  /* display to use for all GDI functions */

/* X11 GDI palette driver */

#define X11DRV_PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define X11DRV_PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define X11DRV_PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */

extern UINT16 X11DRV_PALETTE_PaletteFlags DECLSPEC_HIDDEN;

extern int *X11DRV_PALETTE_PaletteToXPixel DECLSPEC_HIDDEN;
extern int *X11DRV_PALETTE_XPixelToPalette DECLSPEC_HIDDEN;
extern ColorShifts X11DRV_PALETTE_default_shifts DECLSPEC_HIDDEN;

extern int X11DRV_PALETTE_mapEGAPixel[16] DECLSPEC_HIDDEN;

extern int X11DRV_PALETTE_Init(void) DECLSPEC_HIDDEN;
extern BOOL X11DRV_IsSolidColor(COLORREF color) DECLSPEC_HIDDEN;

extern COLORREF X11DRV_PALETTE_ToLogical(X11DRV_PDEVICE *physDev, int pixel) DECLSPEC_HIDDEN;
extern int X11DRV_PALETTE_ToPhysical(X11DRV_PDEVICE *physDev, COLORREF color) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_PALETTE_GetColor( X11DRV_PDEVICE *physDev, COLORREF color ) DECLSPEC_HIDDEN;
extern int *get_window_surface_mapping( int bpp, int *mapping ) DECLSPEC_HIDDEN;

/* GDI escapes */

#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_SET_DRAWABLE,     /* set current drawable for a DC */
    X11DRV_GET_DRAWABLE,     /* get current drawable for a DC */
    X11DRV_START_EXPOSURES,  /* start graphics exposures */
    X11DRV_END_EXPOSURES,    /* end graphics exposures */
    X11DRV_FLUSH_GL_DRAWABLE /* flush changes made to the gl drawable */
};

struct x11drv_escape_set_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_SET_DRAWABLE) */
    Drawable                 drawable;     /* X drawable */
    int                      mode;         /* ClipByChildren or IncludeInferiors */
    RECT                     dc_rect;      /* DC rectangle relative to drawable */
};

struct x11drv_escape_get_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_GET_DRAWABLE) */
    Drawable                 drawable;     /* X drawable */
    Drawable                 gl_drawable;  /* GL drawable */
    int                      pixel_format; /* internal GL pixel format */
};

struct x11drv_escape_flush_gl_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_FLUSH_GL_DRAWABLE) */
    Drawable                 gl_drawable;  /* GL drawable */
    BOOL                     flush;        /* flush X11 before copying */
};

/**************************************************************************
 * X11 USER driver
 */

struct x11drv_thread_data
{
    Display *display;
    XEvent  *current_event;        /* event currently being processed */
    HWND     grab_hwnd;            /* window that currently grabs the mouse */
    HWND     last_focus;           /* last window that had focus */
    XIM      xim;                  /* input method */
    HWND     last_xic_hwnd;        /* last xic window */
    XFontSet font_set;             /* international text drawing font set */
    Window   selection_wnd;        /* window used for selection interactions */
    unsigned long warp_serial;     /* serial number of last pointer warp request */
    Window   clip_window;          /* window used for cursor clipping */
    HWND     clip_hwnd;            /* message window stored in desktop while clipping is active */
    DWORD    clip_reset;           /* time when clipping was last reset */
#ifdef HAVE_X11_EXTENSIONS_XINPUT2_H
    enum { xi_unavailable = -1, xi_unknown, xi_disabled, xi_enabled } xi2_state; /* XInput2 state */
    void    *xi2_devices;          /* list of XInput2 devices (valid when state is enabled) */
    int      xi2_device_count;
    XIValuatorClassInfo x_valuator;
    XIValuatorClassInfo y_valuator;
    int      xi2_core_pointer;     /* XInput2 core pointer id */
    int      xi2_current_slave;    /* Current slave driving the Core pointer */
#endif /* HAVE_X11_EXTENSIONS_XINPUT2_H */
};

extern struct x11drv_thread_data *x11drv_init_thread_data(void) DECLSPEC_HIDDEN;

static inline struct x11drv_thread_data *x11drv_thread_data(void)
{
    return (struct x11drv_thread_data *)(UINT_PTR)NtUserGetThreadInfo()->driver_data;
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

extern XVisualInfo default_visual DECLSPEC_HIDDEN;
extern XVisualInfo argb_visual DECLSPEC_HIDDEN;
extern Colormap default_colormap DECLSPEC_HIDDEN;
extern XPixmapFormatValues **pixmap_formats DECLSPEC_HIDDEN;
extern Window root_window DECLSPEC_HIDDEN;
extern BOOL clipping_cursor DECLSPEC_HIDDEN;
extern BOOL keyboard_grabbed DECLSPEC_HIDDEN;
extern unsigned int screen_bpp DECLSPEC_HIDDEN;
extern BOOL use_xkb DECLSPEC_HIDDEN;
extern BOOL usexrandr DECLSPEC_HIDDEN;
extern BOOL usexvidmode DECLSPEC_HIDDEN;
extern BOOL ximInComposeMode DECLSPEC_HIDDEN;
extern BOOL use_take_focus DECLSPEC_HIDDEN;
extern BOOL use_primary_selection DECLSPEC_HIDDEN;
extern BOOL use_system_cursors DECLSPEC_HIDDEN;
extern BOOL show_systray DECLSPEC_HIDDEN;
extern BOOL grab_pointer DECLSPEC_HIDDEN;
extern BOOL grab_fullscreen DECLSPEC_HIDDEN;
extern BOOL usexcomposite DECLSPEC_HIDDEN;
extern BOOL managed_mode DECLSPEC_HIDDEN;
extern BOOL decorated_mode DECLSPEC_HIDDEN;
extern BOOL private_color_map DECLSPEC_HIDDEN;
extern int primary_monitor DECLSPEC_HIDDEN;
extern int copy_default_colors DECLSPEC_HIDDEN;
extern int alloc_system_colors DECLSPEC_HIDDEN;
extern int xrender_error_base DECLSPEC_HIDDEN;
extern char *process_name DECLSPEC_HIDDEN;
extern Display *clipboard_display DECLSPEC_HIDDEN;
extern WNDPROC client_foreign_window_proc;

/* atoms */

enum x11drv_atoms
{
    FIRST_XATOM = XA_LAST_PREDEFINED + 1,
    XATOM_CLIPBOARD = FIRST_XATOM,
    XATOM_COMPOUND_TEXT,
    XATOM_EDID,
    XATOM_INCR,
    XATOM_MANAGER,
    XATOM_MULTIPLE,
    XATOM_SELECTION_DATA,
    XATOM_TARGETS,
    XATOM_TEXT,
    XATOM_TIMESTAMP,
    XATOM_UTF8_STRING,
    XATOM_RAW_ASCENT,
    XATOM_RAW_DESCENT,
    XATOM_RAW_CAP_HEIGHT,
    XATOM_Rel_X,
    XATOM_Rel_Y,
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
    XATOM__NET_SYSTEM_TRAY_VISUAL,
    XATOM__NET_WM_ICON,
    XATOM__NET_WM_MOVERESIZE,
    XATOM__NET_WM_NAME,
    XATOM__NET_WM_PID,
    XATOM__NET_WM_PING,
    XATOM__NET_WM_STATE,
    XATOM__NET_WM_STATE_ABOVE,
    XATOM__NET_WM_STATE_DEMANDS_ATTENTION,
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
    XATOM__GTK_WORKAREAS_D0,
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
    XATOM_XdndTypeList,
    XATOM_HTML_Format,
    XATOM_WCF_DIF,
    XATOM_WCF_ENHMETAFILE,
    XATOM_WCF_HDROP,
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

typedef BOOL (*x11drv_event_handler)( HWND hwnd, XEvent *event );

extern void X11DRV_register_event_handler( int type, x11drv_event_handler handler, const char *name ) DECLSPEC_HIDDEN;

extern BOOL X11DRV_ButtonPress( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_ButtonRelease( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_MotionNotify( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_EnterNotify( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_KeyEvent( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_KeymapNotify( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_DestroyNotify( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SelectionRequest( HWND hWnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SelectionClear( HWND hWnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_MappingNotify( HWND hWnd, XEvent *event ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GenericEvent( HWND hwnd, XEvent *event ) DECLSPEC_HIDDEN;

extern int xinput2_opcode DECLSPEC_HIDDEN;
extern Bool (*pXGetEventData)( Display *display, XEvent /*XGenericEventCookie*/ *event ) DECLSPEC_HIDDEN;
extern void (*pXFreeEventData)( Display *display, XEvent /*XGenericEventCookie*/ *event ) DECLSPEC_HIDDEN;

extern DWORD EVENT_x11_time_to_win32_time(Time time) DECLSPEC_HIDDEN;

/* X11 driver private messages, must be in the range 0x80001000..0x80001fff */
enum x11drv_window_messages
{
    WM_X11DRV_UPDATE_CLIPBOARD = 0x80001000,
    WM_X11DRV_SET_WIN_REGION,
    WM_X11DRV_DESKTOP_RESIZED,
    WM_X11DRV_SET_CURSOR,
    WM_X11DRV_CLIP_CURSOR_NOTIFY,
    WM_X11DRV_CLIP_CURSOR_REQUEST,
    WM_X11DRV_DELETE_TAB,
    WM_X11DRV_ADD_TAB
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
    Display    *display;        /* display connection for the thread owning the window */
    XVisualInfo vis;            /* X visual used by this window */
    Colormap    whole_colormap; /* colormap if non-default visual */
    Colormap    client_colormap; /* colormap for the client window */
    HWND        hwnd;           /* hwnd that this private data belongs to */
    Window      whole_window;   /* X window for the complete window */
    Window      client_window;  /* X window for the client area */
    RECT        window_rect;    /* USER window rectangle relative to win32 parent window client area */
    RECT        whole_rect;     /* X window rectangle for the whole window relative to win32 parent window client area */
    RECT        client_rect;    /* client area relative to win32 parent window client area */
    XIC         xic;            /* X input context */
    BOOL        managed : 1;    /* is window managed? */
    BOOL        mapped : 1;     /* is window mapped? (in either normal or iconic state) */
    BOOL        iconic : 1;     /* is window in iconic state? */
    BOOL        embedded : 1;   /* is window an XEMBED client? */
    BOOL        shaped : 1;     /* is window using a custom region shape? */
    BOOL        layered : 1;    /* is window layered and with valid attributes? */
    BOOL        use_alpha : 1;  /* does window use an alpha channel? */
    BOOL        skip_taskbar : 1; /* does window should be deleted from taskbar */
    BOOL        add_taskbar : 1; /* does window should be added to taskbar regardless of style */
    int         wm_state;       /* current value of the WM_STATE property */
    DWORD       net_wm_state;   /* bit mask of active x11drv_net_wm_state values */
    Window      embedder;       /* window id of embedder */
    unsigned long configure_serial; /* serial number of last configure request */
    struct window_surface *surface;
    Pixmap         icon_pixmap;
    Pixmap         icon_mask;
    unsigned long *icon_bits;
    unsigned int   icon_size;
};

extern struct x11drv_win_data *get_win_data( HWND hwnd ) DECLSPEC_HIDDEN;
extern void release_win_data( struct x11drv_win_data *data ) DECLSPEC_HIDDEN;
extern Window X11DRV_get_whole_window( HWND hwnd ) DECLSPEC_HIDDEN;
extern XIC X11DRV_get_ic( HWND hwnd ) DECLSPEC_HIDDEN;
extern Window get_dummy_parent(void) DECLSPEC_HIDDEN;

extern void sync_gl_drawable( HWND hwnd, BOOL known_child ) DECLSPEC_HIDDEN;
extern void set_gl_drawable_parent( HWND hwnd, HWND parent ) DECLSPEC_HIDDEN;
extern void destroy_gl_drawable( HWND hwnd ) DECLSPEC_HIDDEN;
extern void wine_vk_surface_destroy( HWND hwnd ) DECLSPEC_HIDDEN;
extern void vulkan_thread_detach(void) DECLSPEC_HIDDEN;

extern void wait_for_withdrawn_state( HWND hwnd, BOOL set ) DECLSPEC_HIDDEN;
extern Window init_clip_window(void) DECLSPEC_HIDDEN;
extern void update_user_time( Time time ) DECLSPEC_HIDDEN;
extern void read_net_wm_states( Display *display, struct x11drv_win_data *data ) DECLSPEC_HIDDEN;
extern void update_net_wm_states( struct x11drv_win_data *data ) DECLSPEC_HIDDEN;
extern void make_window_embedded( struct x11drv_win_data *data ) DECLSPEC_HIDDEN;
extern Window create_dummy_client_window(void) DECLSPEC_HIDDEN;
extern Window create_client_window( HWND hwnd, const XVisualInfo *visual ) DECLSPEC_HIDDEN;
extern void set_window_visual( struct x11drv_win_data *data, const XVisualInfo *vis, BOOL use_alpha ) DECLSPEC_HIDDEN;
extern void change_systray_owner( Display *display, Window systray_window ) DECLSPEC_HIDDEN;
extern HWND create_foreign_window( Display *display, Window window ) DECLSPEC_HIDDEN;
extern BOOL update_clipboard( HWND hwnd ) DECLSPEC_HIDDEN;
extern void init_win_context(void) DECLSPEC_HIDDEN;
extern void *file_list_to_drop_files( const void *data, size_t size, size_t *ret_size ) DECLSPEC_HIDDEN;
extern void *uri_list_to_drop_files( const void *data, size_t size, size_t *ret_size ) DECLSPEC_HIDDEN;

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

/* X context to associate a hwnd to an X window */
extern XContext winContext DECLSPEC_HIDDEN;
/* X context to associate an X cursor to a Win32 cursor handle */
extern XContext cursor_context DECLSPEC_HIDDEN;

extern void X11DRV_SetFocus( HWND hwnd ) DECLSPEC_HIDDEN;
extern void set_window_cursor( Window window, HCURSOR handle ) DECLSPEC_HIDDEN;
extern void sync_window_cursor( Window window ) DECLSPEC_HIDDEN;
extern LRESULT clip_cursor_notify( HWND hwnd, HWND prev_clip_hwnd, HWND new_clip_hwnd ) DECLSPEC_HIDDEN;
extern LRESULT clip_cursor_request( HWND hwnd, BOOL fullscreen, BOOL reset ) DECLSPEC_HIDDEN;
extern void ungrab_clipping_window(void) DECLSPEC_HIDDEN;
extern void reset_clipping_window(void) DECLSPEC_HIDDEN;
extern void retry_grab_clipping_window(void) DECLSPEC_HIDDEN;
extern BOOL clip_fullscreen_window( HWND hwnd, BOOL reset ) DECLSPEC_HIDDEN;
extern void move_resize_window( HWND hwnd, int dir ) DECLSPEC_HIDDEN;
extern void X11DRV_InitKeyboard( Display *display ) DECLSPEC_HIDDEN;
extern NTSTATUS X11DRV_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                                    const LARGE_INTEGER *timeout,
                                                    DWORD mask, DWORD flags ) DECLSPEC_HIDDEN;
extern HWND *build_hwnd_list(void) DECLSPEC_HIDDEN;

typedef int (*x11drv_error_callback)( Display *display, XErrorEvent *event, void *arg );

extern void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg ) DECLSPEC_HIDDEN;
extern int X11DRV_check_error(void) DECLSPEC_HIDDEN;
extern void X11DRV_X_to_window_rect( struct x11drv_win_data *data, RECT *rect, int x, int y, int cx, int cy ) DECLSPEC_HIDDEN;
extern POINT virtual_screen_to_root( INT x, INT y ) DECLSPEC_HIDDEN;
extern POINT root_to_virtual_screen( INT x, INT y ) DECLSPEC_HIDDEN;
extern RECT get_host_primary_monitor_rect(void) DECLSPEC_HIDDEN;
extern RECT get_work_area( const RECT *monitor_rect ) DECLSPEC_HIDDEN;
extern void xinerama_init( unsigned int width, unsigned int height ) DECLSPEC_HIDDEN;
extern void init_recursive_mutex( pthread_mutex_t *mutex ) DECLSPEC_HIDDEN;

#define DEPTH_COUNT 3
extern const unsigned int *depths DECLSPEC_HIDDEN;

/* Required functions for changing and enumerating display settings */
struct x11drv_settings_handler
{
    /* A name to tell what host driver is used */
    const char *name;

    /* Higher priority can override handlers with a lower priority */
    UINT priority;

    /* get_id() will be called to map a device name, e.g., \\.\DISPLAY1 to a driver specific id.
     * Following functions use this id to identify the device.
     *
     * Return FALSE if the device cannot be found and TRUE on success */
    BOOL (*get_id)(const WCHAR *device_name, ULONG_PTR *id);

    /* get_modes() will be called to get a list of supported modes of the device of id in modes
     * with respect to flags, which could be 0, EDS_RAWMODE or EDS_ROTATEDMODE. If the implementation
     * uses dmDriverExtra then every DEVMODEW in the list must have the same dmDriverExtra value
     *
     * Following fields in DEVMODE must be valid:
     * dmSize, dmDriverExtra, dmFields, dmDisplayOrientation, dmBitsPerPel, dmPelsWidth, dmPelsHeight,
     * dmDisplayFlags and dmDisplayFrequency
     *
     * Return FALSE on failure with parameters unchanged and error code set. Return TRUE on success */
    BOOL (*get_modes)(ULONG_PTR id, DWORD flags, DEVMODEW **modes, UINT *mode_count);

    /* free_modes() will be called to free the mode list returned from get_modes() */
    void (*free_modes)(DEVMODEW *modes);

    /* get_current_mode() will be called to get the current display mode of the device of id
     *
     * Following fields in DEVMODE must be valid:
     * dmFields, dmDisplayOrientation, dmBitsPerPel, dmPelsWidth, dmPelsHeight, dmDisplayFlags,
     * dmDisplayFrequency and dmPosition
     *
     * Return FALSE on failure with parameters unchanged and error code set. Return TRUE on success */
    BOOL (*get_current_mode)(ULONG_PTR id, DEVMODEW *mode);

    /* set_current_mode() will be called to change the display mode of the display device of id.
     * mode must be a valid mode from get_modes() with optional fields, such as dmPosition set.
     *
     * Return DISP_CHANGE_*, same as ChangeDisplaySettingsExW() return values */
    LONG (*set_current_mode)(ULONG_PTR id, const DEVMODEW *mode);
};

extern void X11DRV_Settings_SetHandler(const struct x11drv_settings_handler *handler) DECLSPEC_HIDDEN;

extern void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height ) DECLSPEC_HIDDEN;
extern void X11DRV_resize_desktop(void) DECLSPEC_HIDDEN;
extern void init_registry_display_settings(void) DECLSPEC_HIDDEN;
extern BOOL is_virtual_desktop(void) DECLSPEC_HIDDEN;
extern BOOL is_desktop_fullscreen(void) DECLSPEC_HIDDEN;
extern BOOL is_detached_mode(const DEVMODEW *) DECLSPEC_HIDDEN;
extern BOOL create_desktop_win_data( Window win ) DECLSPEC_HIDDEN;
extern BOOL get_primary_adapter(WCHAR *) DECLSPEC_HIDDEN;
void X11DRV_Settings_Init(void) DECLSPEC_HIDDEN;

void X11DRV_XF86VM_Init(void) DECLSPEC_HIDDEN;
void X11DRV_XRandR_Init(void) DECLSPEC_HIDDEN;
void init_user_driver(void) DECLSPEC_HIDDEN;

/* X11 display device handler. Used to initialize display device registry data */

/* Required functions for display device registry initialization */
struct x11drv_display_device_handler
{
    /* A name to tell what host driver is used */
    const char *name;

    /* Higher priority can override handlers with lower priority */
    INT priority;

    /* get_gpus will be called to get a list of GPUs. First GPU has to be where the primary adapter is.
     *
     * Return FALSE on failure with parameters unchanged */
    BOOL (*get_gpus)(struct gdi_gpu **gpus, int *count);

    /* get_adapters will be called to get a list of adapters in EnumDisplayDevices context under a GPU.
     * The first adapter has to be primary if GPU is primary.
     *
     * Return FALSE on failure with parameters unchanged */
    BOOL (*get_adapters)(ULONG_PTR gpu_id, struct gdi_adapter **adapters, int *count);

    /* get_monitors will be called to get a list of monitors in EnumDisplayDevices context under an adapter.
     * The first monitor has to be primary if adapter is primary.
     *
     * Return FALSE on failure with parameters unchanged */
    BOOL (*get_monitors)(ULONG_PTR adapter_id, struct gdi_monitor **monitors, int *count);

    /* free_gpus will be called to free a GPU list from get_gpus */
    void (*free_gpus)(struct gdi_gpu *gpus);

    /* free_adapters will be called to free an adapter list from get_adapters */
    void (*free_adapters)(struct gdi_adapter *adapters);

    /* free_monitors will be called to free a monitor list from get_monitors */
    void (*free_monitors)(struct gdi_monitor *monitors, int count);

    /* register_event_handlers will be called to register event handlers.
     * This function pointer is optional and can be NULL when driver doesn't support it */
    void (*register_event_handlers)(void);
};

extern BOOL get_host_primary_gpu(struct gdi_gpu *gpu) DECLSPEC_HIDDEN;
extern void X11DRV_DisplayDevices_SetHandler(const struct x11drv_display_device_handler *handler) DECLSPEC_HIDDEN;
extern void X11DRV_DisplayDevices_Init(BOOL force) DECLSPEC_HIDDEN;
extern void X11DRV_DisplayDevices_RegisterEventHandlers(void) DECLSPEC_HIDDEN;
/* Display device handler used in virtual desktop mode */
extern struct x11drv_display_device_handler desktop_handler DECLSPEC_HIDDEN;

/* XIM support */
extern BOOL X11DRV_InitXIM( const WCHAR *input_style ) DECLSPEC_HIDDEN;
extern XIC X11DRV_CreateIC(XIM xim, struct x11drv_win_data *data) DECLSPEC_HIDDEN;
extern void X11DRV_SetupXIM(void) DECLSPEC_HIDDEN;
extern void X11DRV_XIMLookupChars( const char *str, DWORD count ) DECLSPEC_HIDDEN;

#define XEMBED_MAPPED  (1 << 0)

static inline BOOL is_window_rect_mapped( const RECT *rect )
{
    RECT virtual_rect = NtUserGetVirtualScreenRect();
    return (rect->left < virtual_rect.right &&
            rect->top < virtual_rect.bottom &&
            max( rect->right, rect->left + 1 ) > virtual_rect.left &&
            max( rect->bottom, rect->top + 1 ) > virtual_rect.top);
}

/* unixlib interface */

extern NTSTATUS x11drv_create_desktop( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_systray_clear( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_systray_dock( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_systray_hide( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_systray_init( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_tablet_attach_queue( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_tablet_get_packet( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_tablet_load_info( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_tablet_info( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_xim_preedit_state( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_xim_reset( void *arg ) DECLSPEC_HIDDEN;

extern NTSTATUS x11drv_client_func( enum x11drv_client_funcs func, const void *params,
                                    ULONG size ) DECLSPEC_HIDDEN;
extern NTSTATUS x11drv_client_call( enum client_callback func, UINT arg ) DECLSPEC_HIDDEN;

/* GDI helpers */

static inline BOOL lp_to_dp( HDC hdc, POINT *points, INT count )
{
    return NtGdiTransformPoints( hdc, points, points, count, NtGdiLPtoDP );
}

static inline UINT get_palette_entries( HPALETTE palette, UINT start, UINT count, PALETTEENTRY *entries )
{
    return NtGdiDoPalette( palette, start, count, entries, NtGdiGetPaletteEntries, TRUE );
}

/* user helpers */

static inline LRESULT send_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return NtUserMessageCall( hwnd, msg, wparam, lparam, NULL, NtUserSendDriverMessage, FALSE );
}

static inline LRESULT send_message_timeout( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                            UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_timeout_params params = { .flags = flags, .timeout = timeout };
    LRESULT res = NtUserMessageCall( hwnd, msg, wparam, lparam, &params,
                                     NtUserSendMessageTimeout, FALSE );
    if (res_ptr) *res_ptr = params.result;
    return res;
}

static inline BOOL send_notify_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return NtUserMessageCall( hwnd, msg, wparam, lparam, 0, NtUserSendNotifyMessage, FALSE );
}

static inline HWND get_focus(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndFocus : 0;
}

static inline HWND get_active_window(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndActive : 0;
}

static inline BOOL intersect_rect( RECT *dst, const RECT *src1, const RECT *src2 )
{
    dst->left   = max( src1->left, src2->left );
    dst->top    = max( src1->top, src2->top );
    dst->right  = min( src1->right, src2->right );
    dst->bottom = min( src1->bottom, src2->bottom );
    return !IsRectEmpty( dst );
}

/* registry helpers */

extern HKEY open_hkcu_key( const char *name ) DECLSPEC_HIDDEN;
extern ULONG query_reg_value( HKEY hkey, const WCHAR *name,
                              KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size ) DECLSPEC_HIDDEN;
extern HKEY reg_open_key( HKEY root, const WCHAR *name, ULONG name_len ) DECLSPEC_HIDDEN;

/* string helpers */

static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static inline UINT asciiz_to_unicode( WCHAR *dst, const char *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return (p - dst) * sizeof(WCHAR);
}

#endif  /* __WINE_X11DRV_H */
