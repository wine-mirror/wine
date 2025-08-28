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
#include "wine/debug.h"
#include "mwm.h"

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

extern BOOL X11DRV_Arc( PHYSDEV dev, INT left, INT top, INT right,
                        INT bottom, INT xstart, INT ystart, INT xend, INT yend );
extern BOOL X11DRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                          INT xstart, INT ystart, INT xend, INT yend );
extern BOOL X11DRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom );
extern BOOL X11DRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType );
extern BOOL X11DRV_FillPath( PHYSDEV dev );
extern BOOL X11DRV_GetDeviceGammaRamp( PHYSDEV dev, LPVOID ramp );
extern DWORD X11DRV_GetImage( PHYSDEV dev, BITMAPINFO *info,
                              struct gdi_image_bits *bits, struct bitblt_coords *src );
extern COLORREF X11DRV_GetNearestColor( PHYSDEV dev, COLORREF color );
extern UINT X11DRV_GetSystemPaletteEntries( PHYSDEV dev, UINT start, UINT count, LPPALETTEENTRY entries );
extern BOOL X11DRV_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                 void *grad_array, ULONG ngrad, ULONG mode );
extern BOOL X11DRV_LineTo( PHYSDEV dev, INT x, INT y);
extern BOOL X11DRV_PaintRgn( PHYSDEV dev, HRGN hrgn );
extern BOOL X11DRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop );
extern BOOL X11DRV_Pie( PHYSDEV dev, INT left, INT top, INT right,
                        INT bottom, INT xstart, INT ystart, INT xend, INT yend );
extern BOOL X11DRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polygons);
extern BOOL X11DRV_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polylines);
extern DWORD X11DRV_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                              const struct gdi_image_bits *bits, struct bitblt_coords *src,
                              struct bitblt_coords *dst, DWORD rop );
extern UINT X11DRV_RealizeDefaultPalette( PHYSDEV dev );
extern UINT X11DRV_RealizePalette( PHYSDEV dev, HPALETTE hpal, BOOL primary );
extern BOOL X11DRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom);
extern BOOL X11DRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                              INT ell_width, INT ell_height );
extern HBRUSH X11DRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern );
extern HPEN X11DRV_SelectPen( PHYSDEV dev, HPEN hpen, const struct brush_pattern *pattern );
extern COLORREF X11DRV_SetDCBrushColor( PHYSDEV dev, COLORREF crColor );
extern COLORREF X11DRV_SetDCPenColor( PHYSDEV dev, COLORREF crColor );
extern void X11DRV_SetDeviceClipping( PHYSDEV dev, HRGN rgn );
extern BOOL X11DRV_SetDeviceGammaRamp( PHYSDEV dev, LPVOID ramp );
extern COLORREF X11DRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color );
extern BOOL X11DRV_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                               PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop );
extern BOOL X11DRV_StrokeAndFillPath( PHYSDEV dev );
extern BOOL X11DRV_StrokePath( PHYSDEV dev );
extern BOOL X11DRV_UnrealizePalette( HPALETTE hpal );

extern BOOL X11DRV_ActivateKeyboardLayout( HKL hkl, UINT flags );
extern void X11DRV_Beep(void);
extern INT X11DRV_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size );
extern UINT X11DRV_MapVirtualKeyEx( UINT code, UINT map_type, HKL hkl );
extern INT X11DRV_ToUnicodeEx( UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                               LPWSTR bufW, int bufW_size, UINT flags, HKL hkl );
extern SHORT X11DRV_VkKeyScanEx( WCHAR wChar, HKL hkl );
extern void X11DRV_NotifyIMEStatus( HWND hwnd, UINT status );
extern BOOL X11DRV_SetIMECompositionRect( HWND hwnd, RECT rect );
extern void X11DRV_DestroyCursorIcon( HCURSOR handle );
extern void X11DRV_SetCursor( HWND hwnd, HCURSOR handle );
extern BOOL X11DRV_SetCursorPos( INT x, INT y );
extern BOOL X11DRV_GetCursorPos( LPPOINT pos );
extern BOOL X11DRV_ClipCursor( const RECT *clip, BOOL reset );
extern void X11DRV_SystrayDockInit( HWND systray );
extern BOOL X11DRV_SystrayDockInsert( HWND owner, UINT cx, UINT cy, void *icon );
extern void X11DRV_SystrayDockClear( HWND hwnd );
extern BOOL X11DRV_SystrayDockRemove( HWND hwnd );
extern LONG X11DRV_ChangeDisplaySettings( LPDEVMODEW displays, LPCWSTR primary_name, HWND hwnd, DWORD flags, LPVOID lpvoid );
extern UINT X11DRV_UpdateDisplayDevices( const struct gdi_device_manager *device_manager, void *param );
extern BOOL X11DRV_CreateDesktop( const WCHAR *name, UINT width, UINT height );
extern BOOL X11DRV_CreateWindow( HWND hwnd );
extern LRESULT X11DRV_DesktopWindowProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
extern void X11DRV_DestroyWindow( HWND hwnd );
extern void X11DRV_FlashWindowEx( PFLASHWINFO pfinfo );
extern void X11DRV_GetDC( HDC hdc, HWND hwnd, HWND top, const RECT *win_rect,
                          const RECT *top_rect, DWORD flags );
extern void X11DRV_ReleaseDC( HWND hwnd, HDC hdc );
extern BOOL X11DRV_ScrollDC( HDC hdc, INT dx, INT dy, HRGN update );
extern void X11DRV_SetCapture( HWND hwnd, UINT flags );
extern void X11DRV_SetDesktopWindow( HWND hwnd );
extern void X11DRV_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha,
                                               DWORD flags );
extern void X11DRV_SetParent( HWND hwnd, HWND parent, HWND old_parent );
extern void X11DRV_SetWindowIcon( HWND hwnd, UINT type, HICON icon );
extern void X11DRV_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw );
extern void X11DRV_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style );
extern void X11DRV_SetWindowText( HWND hwnd, LPCWSTR text );
extern UINT X11DRV_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp );
extern LRESULT X11DRV_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam, const POINT *pos );
extern LRESULT X11DRV_ClipboardWindowProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
extern void X11DRV_UpdateClipboard(void);
extern void X11DRV_UpdateLayeredWindow( HWND hwnd, BYTE alpha, UINT flags );
extern LRESULT X11DRV_WindowMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
extern BOOL X11DRV_WindowPosChanging( HWND hwnd, UINT swp_flags, BOOL shaped, const struct window_rects *rects );
extern BOOL X11DRV_GetWindowStyleMasks( HWND hwnd, UINT style, UINT ex_style, UINT *style_mask, UINT *ex_style_mask );
extern BOOL X11DRV_GetWindowStateUpdates( HWND hwnd, UINT *state_cmd, UINT *config_cmd, RECT *rect, HWND *foreground );
extern BOOL X11DRV_CreateWindowSurface( HWND hwnd, BOOL layered, const RECT *surface_rect, struct window_surface **surface );
extern void X11DRV_MoveWindowBits( HWND hwnd, const struct window_rects *old_rects,
                                   const struct window_rects *new_rects, const RECT *valid_rects );
extern void X11DRV_WindowPosChanged( HWND hwnd, HWND insert_after, HWND owner_hint, UINT swp_flags, BOOL fullscreen,
                                     const struct window_rects *new_rects, struct window_surface *surface );
extern BOOL X11DRV_SystemParametersInfo( UINT action, UINT int_param, void *ptr_param,
                                         UINT flags );
extern void X11DRV_ThreadDetach(void);

/* X11 driver internal functions */

extern void X11DRV_Xcursor_Init(void);

extern DWORD copy_image_bits( BITMAPINFO *info, BOOL is_r8g8b8, XImage *image,
                              const struct gdi_image_bits *src_bits, struct gdi_image_bits *dst_bits,
                              struct bitblt_coords *coords, const int *mapping, unsigned int zeropad_mask );
extern Pixmap create_pixmap_from_image( HDC hdc, const XVisualInfo *vis, const BITMAPINFO *info,
                                        const struct gdi_image_bits *bits, UINT coloruse );
extern DWORD get_pixmap_image( Pixmap pixmap, int width, int height, const XVisualInfo *vis,
                               BITMAPINFO *info, struct gdi_image_bits *bits );

extern RGNDATA *X11DRV_GetRegionData( HRGN hrgn, HDC hdc_lptodp );
extern BOOL add_extra_clipping_region( X11DRV_PDEVICE *dev, HRGN rgn );
extern void restore_clipping_region( X11DRV_PDEVICE *dev );
extern void add_device_bounds( X11DRV_PDEVICE *dev, const RECT *rect );

extern void execute_rop( X11DRV_PDEVICE *physdev, Pixmap src_pixmap, GC gc, const RECT *visrect, DWORD rop );

extern BOOL X11DRV_SetupGCForPatBlt( X11DRV_PDEVICE *physDev, GC gc, BOOL fMapColors );
extern BOOL X11DRV_SetupGCForBrush( X11DRV_PDEVICE *physDev );
extern INT X11DRV_XWStoDS( HDC hdc, INT width );
extern INT X11DRV_YWStoDS( HDC hdc, INT height );

extern BOOL client_side_graphics;
extern BOOL client_side_with_render;
extern BOOL shape_layered_windows;
extern const struct gdi_dc_funcs *X11DRV_XRender_Init(void);

extern UINT X11DRV_OpenGLInit( UINT, const struct opengl_funcs *, const struct opengl_driver_funcs ** );
extern UINT X11DRV_VulkanInit( UINT, void *, const struct vulkan_driver_funcs ** );

extern struct format_entry *import_xdnd_selection( Display *display, Window win, Atom selection,
                                                   Atom *targets, UINT count,
                                                   size_t *size );

/**************************************************************************
 * X11 GDI driver
 */

extern Display *gdi_display;  /* display to use for all GDI functions */

/* X11 GDI palette driver */

#define X11DRV_PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define X11DRV_PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define X11DRV_PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */

extern UINT16 X11DRV_PALETTE_PaletteFlags;

extern int *X11DRV_PALETTE_PaletteToXPixel;
extern int *X11DRV_PALETTE_XPixelToPalette;
extern ColorShifts X11DRV_PALETTE_default_shifts;

extern int X11DRV_PALETTE_mapEGAPixel[16];

extern int X11DRV_PALETTE_Init(void);
extern BOOL X11DRV_IsSolidColor(COLORREF color);

extern COLORREF X11DRV_PALETTE_ToLogical(X11DRV_PDEVICE *physDev, int pixel);
extern int X11DRV_PALETTE_ToPhysical(X11DRV_PDEVICE *physDev, COLORREF color);
extern COLORREF X11DRV_PALETTE_GetColor( X11DRV_PDEVICE *physDev, COLORREF color );
extern int *get_window_surface_mapping( int bpp, int *mapping );

static inline const char *debugstr_color( COLORREF color )
{
    if (color & (1 << 24))  /* PALETTEINDEX */
        return wine_dbg_sprintf( "PALETTEINDEX(%u)", LOWORD(color) );
    if (color >> 16 == 0x10ff)  /* DIBINDEX */
        return wine_dbg_sprintf( "DIBINDEX(%u)", LOWORD(color) );
    return wine_dbg_sprintf( "RGB(%02x,%02x,%02x)", GetRValue(color), GetGValue(color), GetBValue(color) );
}

/* GDI escapes */

#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_SET_DRAWABLE,     /* set current drawable for a DC */
    X11DRV_GET_DRAWABLE,     /* get current drawable for a DC */
    X11DRV_START_EXPOSURES,  /* start graphics exposures */
    X11DRV_END_EXPOSURES,    /* end graphics exposures */
};

struct x11drv_escape_set_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_SET_DRAWABLE) */
    Drawable                 drawable;     /* X drawable */
    int                      mode;         /* ClipByChildren or IncludeInferiors */
    RECT                     dc_rect;      /* DC rectangle relative to drawable */
    XVisualInfo              visual;       /* X visual used by drawable, may be unspecified if no change is needed */
};

struct x11drv_escape_get_drawable
{
    enum x11drv_escape_codes code;         /* escape code (X11DRV_GET_DRAWABLE) */
    Drawable                 drawable;     /* X drawable */
    RECT                     dc_rect;      /* DC rectangle relative to drawable */
};

extern BOOL needs_offscreen_rendering( HWND hwnd );
extern void set_dc_drawable( HDC hdc, Drawable drawable, const RECT *rect, int mode );
extern Drawable get_dc_drawable( HDC hdc, RECT *rect );
extern HRGN get_dc_monitor_region( HWND hwnd, HDC hdc );
extern Window x11drv_client_surface_create( HWND hwnd, const XVisualInfo *visual, Colormap colormap, struct client_surface **client );

/**************************************************************************
 * X11 USER driver
 */

/* thread-local host-only window, for X11 relative position tracking */
struct host_window
{
    LONG refcount;
    Window window;
    BOOL destroyed; /* host window has already been destroyed */
    RECT rect; /* host window rect, relative to parent */
    struct host_window *parent;
    unsigned int children_count;
    struct { Window window; RECT rect; } *children;
};

extern void host_window_destroy( struct host_window *win );
extern void host_window_set_parent( struct host_window *win, Window parent );
extern RECT host_window_configure_child( struct host_window *win, Window window, RECT rect, BOOL root_coords );
extern POINT host_window_map_point( struct host_window *win, int x, int y );
extern struct host_window *get_host_window( Window window, BOOL create );

struct display_state
{
    Window net_active_window;
};

struct x11drv_thread_data
{
    Display *display;
    XEvent  *current_event;        /* event currently being processed */
    HWND     grab_hwnd;            /* window that currently grabs the mouse */
    HWND     last_focus;           /* last window that had focus */
    HWND     keymapnotify_hwnd;    /* window that should receive modifier release events */
    XIM      xim;                  /* input method */
    HWND     last_xic_hwnd;        /* last xic window */
    XFontSet font_set;             /* international text drawing font set */
    Window   selection_wnd;        /* window used for selection interactions */
    unsigned long warp_serial;     /* serial number of last pointer warp request */
    Window   clip_window;          /* window used for cursor clipping */
    BOOL     clipping_cursor;      /* whether thread is currently clipping the cursor */
    Atom    *net_supported;        /* list of _NET_SUPPORTED atoms */
    int      net_supported_count;  /* number of _NET_SUPPORTED atoms */
    UINT     net_wm_state_mask;    /* mask of supported _NET_WM_STATE *bits */
#ifdef HAVE_X11_EXTENSIONS_XINPUT2_H
    XIValuatorClassInfo x_valuator;
    XIValuatorClassInfo y_valuator;
    int      xinput2_pointer;      /* XInput2 master pointer device id */
#endif /* HAVE_X11_EXTENSIONS_XINPUT2_H */

    struct display_state desired_state;       /* display state tracking the desired / win32 state */
    struct display_state pending_state;       /* display state tracking the pending / requested state */
    struct display_state current_state;       /* display state tracking the current X11 state */
    unsigned long net_active_window_serial;   /* serial of last pending _NET_ACTIVE_WINDOW request */
};

extern struct x11drv_thread_data *x11drv_init_thread_data(void);

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

extern XVisualInfo default_visual;
extern XVisualInfo argb_visual;
extern Colormap default_colormap;
extern XPixmapFormatValues **pixmap_formats;
extern Window root_window;
extern BOOL clipping_cursor;
extern BOOL keyboard_grabbed;
extern unsigned int screen_bpp;
extern BOOL usexrandr;
extern BOOL usexvidmode;
extern BOOL use_egl;
extern BOOL use_take_focus;
extern BOOL use_primary_selection;
extern BOOL use_system_cursors;
extern BOOL grab_fullscreen;
extern BOOL usexcomposite;
extern BOOL managed_mode;
extern BOOL private_color_map;
extern int primary_monitor;
extern int copy_default_colors;
extern int alloc_system_colors;
extern int xrender_error_base;
extern char *process_name;
extern Display *clipboard_display;

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
    XATOM_WM_PROTOCOLS,
    XATOM_WM_DELETE_WINDOW,
    XATOM_WM_STATE,
    XATOM_WM_TAKE_FOCUS,
    XATOM_DndProtocol,
    XATOM_DndSelection,
    XATOM__ICC_PROFILE,
    XATOM__KDE_NET_WM_STATE_SKIP_SWITCHER,
    XATOM__MOTIF_WM_HINTS,
    XATOM__NET_ACTIVE_WINDOW,
    XATOM__NET_STARTUP_INFO_BEGIN,
    XATOM__NET_STARTUP_INFO,
    XATOM__NET_SUPPORTED,
    XATOM__NET_SYSTEM_TRAY_OPCODE,
    XATOM__NET_SYSTEM_TRAY_S0,
    XATOM__NET_SYSTEM_TRAY_VISUAL,
    XATOM__NET_WM_FULLSCREEN_MONITORS,
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

extern Atom X11DRV_Atoms[NB_XATOMS - FIRST_XATOM];
extern Atom systray_atom;
extern HWND systray_hwnd;

#define x11drv_atom(name) (X11DRV_Atoms[XATOM_##name - FIRST_XATOM])

/* X11 event driver */

typedef BOOL (*x11drv_event_handler)( HWND hwnd, XEvent *event );

extern void X11DRV_register_event_handler( int type, x11drv_event_handler handler, const char *name );

extern BOOL X11DRV_ButtonPress( HWND hwnd, XEvent *event );
extern BOOL X11DRV_ButtonRelease( HWND hwnd, XEvent *event );
extern BOOL X11DRV_MotionNotify( HWND hwnd, XEvent *event );
extern BOOL X11DRV_EnterNotify( HWND hwnd, XEvent *event );
extern BOOL X11DRV_KeyEvent( HWND hwnd, XEvent *event );
extern BOOL X11DRV_KeymapNotify( HWND hwnd, XEvent *event );
extern BOOL X11DRV_DestroyNotify( HWND hwnd, XEvent *event );
extern BOOL X11DRV_SelectionRequest( HWND hWnd, XEvent *event );
extern BOOL X11DRV_SelectionClear( HWND hWnd, XEvent *event );
extern BOOL X11DRV_MappingNotify( HWND hWnd, XEvent *event );
extern BOOL X11DRV_GenericEvent( HWND hwnd, XEvent *event );

extern int xinput2_opcode;
extern void x11drv_xinput2_load(void);
extern void x11drv_xinput2_init( struct x11drv_thread_data *data );
extern void x11drv_xinput2_enable( Display *display, Window window );
extern void x11drv_xinput2_disable( Display *display, Window window );

extern Bool (*pXGetEventData)( Display *display, XEvent /*XGenericEventCookie*/ *event );
extern void (*pXFreeEventData)( Display *display, XEvent /*XGenericEventCookie*/ *event );

extern DWORD EVENT_x11_time_to_win32_time(Time time);

/* X11 driver private messages */
enum x11drv_window_messages
{
    WM_X11DRV_UPDATE_CLIPBOARD = WM_WINE_FIRST_DRIVER_MSG,
    WM_X11DRV_SET_WIN_REGION,
    WM_X11DRV_DELETE_TAB,
    WM_X11DRV_ADD_TAB
};

/* _NET_WM_STATE properties that we keep track of */
enum x11drv_net_wm_state
{
    KDE_NET_WM_STATE_SKIP_SWITCHER,
    NET_WM_STATE_FULLSCREEN,
    NET_WM_STATE_ABOVE,
    NET_WM_STATE_MAXIMIZED,
    NET_WM_STATE_SKIP_PAGER,
    NET_WM_STATE_SKIP_TASKBAR,
    NB_NET_WM_STATES
};

struct monitor_indices
{
    unsigned int generation;
    long indices[4];
};

struct window_state
{
    UINT wm_state;
    BOOL activate;
    UINT net_wm_state;
    MwmHints mwm_hints;
    struct monitor_indices monitors;
    RECT rect;
    BOOL above;
};

/* x11drv private window data */
struct x11drv_win_data
{
    Display    *display;        /* display connection for the thread owning the window */
    XVisualInfo vis;            /* X visual used by this window */
    Colormap    whole_colormap; /* colormap if non-default visual */
    HWND        hwnd;           /* hwnd that this private data belongs to */
    Window      whole_window;   /* X window for the complete window */
    Window      client_window;  /* X window for the client area */
    struct window_rects rects;  /* window rects in monitor DPI, relative to parent client area */
    struct host_window *parent; /* the host window parent, frame or embedder, NULL if root_window */
    XIC         xic;            /* X input context */
    UINT        managed : 1;    /* is window managed? */
    UINT        embedded : 1;   /* is window an XEMBED client? */
    UINT        shaped : 1;     /* is window using a custom region shape? */
    UINT        layered : 1;    /* is window layered and with valid attributes? */
    UINT        use_alpha : 1;  /* does window use an alpha channel? */
    UINT        skip_taskbar : 1; /* does window should be deleted from taskbar */
    UINT        add_taskbar : 1; /* does window should be added to taskbar regardless of style */
    UINT        is_fullscreen : 1; /* is the window visible rect fullscreen */
    UINT        is_offscreen : 1; /* has been moved offscreen by the window manager */
    UINT        parent_invalid : 1; /* is the parent host window possibly invalid */
    UINT        reparenting : 1; /* window is being reparented, likely from a decoration change */
    Window      embedder;       /* window id of embedder */
    Pixmap         icon_pixmap;
    Pixmap         icon_mask;
    unsigned long *icon_bits;
    unsigned int   icon_size;
    Time           user_time;

    struct window_state desired_state; /* window state tracking the desired / win32 state */
    struct window_state pending_state; /* window state tracking the pending / requested state */
    struct window_state current_state; /* window state tracking the current X11 state */
    unsigned long wm_state_serial;     /* serial of last pending WM_STATE request */
    unsigned long net_wm_state_serial; /* serial of last pending _NET_WM_STATE request */
    unsigned long mwm_hints_serial;    /* serial of last pending _MOTIF_WM_HINTS request */
    unsigned long configure_serial;    /* serial of last pending configure request */
};

extern struct x11drv_win_data *get_win_data( HWND hwnd );
extern void release_win_data( struct x11drv_win_data *data );
extern void set_window_parent( struct x11drv_win_data *data, Window parent );
extern Window X11DRV_get_whole_window( HWND hwnd );
extern Window get_dummy_parent(void);

extern BOOL window_is_reparenting( HWND hwnd );
extern BOOL window_should_take_focus( HWND hwnd, Time time );
extern BOOL window_has_pending_wm_state( HWND hwnd, UINT state );
extern void window_wm_state_notify( struct x11drv_win_data *data, unsigned long serial, UINT value, Time time );
extern void window_net_wm_state_notify( struct x11drv_win_data *data, unsigned long serial, UINT value );
extern void window_mwm_hints_notify( struct x11drv_win_data *data, unsigned long serial, const MwmHints *hints );
extern void window_configure_notify( struct x11drv_win_data *data, unsigned long serial, const RECT *rect );

extern void set_net_active_window( HWND hwnd, HWND previous );
extern Window get_net_active_window( Display *display );
extern void net_active_window_notify( unsigned long serial, Window window, Time time );
extern void net_active_window_init( struct x11drv_thread_data *data );
extern void net_supported_init( struct x11drv_thread_data *data );
extern BOOL is_net_supported( Atom atom );

extern Window init_clip_window(void);
extern void window_set_user_time( struct x11drv_win_data *data, Time time, BOOL init );
extern UINT get_window_net_wm_state( Display *display, Window window );
extern void make_window_embedded( struct x11drv_win_data *data );
extern Window create_client_window( HWND hwnd, const XVisualInfo *visual, Colormap colormap );
extern void detach_client_window( struct x11drv_win_data *data, Window client_window );
extern void attach_client_window( struct x11drv_win_data *data, Window client_window );
extern void destroy_client_window( HWND hwnd, Window client_window );
extern void set_window_visual( struct x11drv_win_data *data, const XVisualInfo *vis, BOOL use_alpha );
extern void change_systray_owner( Display *display, Window systray_window );
extern BOOL update_clipboard( HWND hwnd );
extern void init_win_context(void);
extern DROPFILES *file_list_to_drop_files( const void *data, size_t size, size_t *ret_size );
extern DROPFILES *uri_list_to_drop_files( const void *data, size_t size, size_t *ret_size );

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

/* X context to associate a hwnd to an X window */
extern XContext winContext;
/* X context to associate an X cursor to a Win32 cursor handle */
extern XContext cursor_context;

extern BOOL is_current_process_focused(void);
extern void X11DRV_ActivateWindow( HWND hwnd, HWND previous );
extern void set_window_cursor( Window window, HCURSOR handle );
extern void reapply_cursor_clipping(void);
extern void ungrab_clipping_window(void);
extern void move_resize_window( HWND hwnd, int dir, POINT pos );
extern void X11DRV_InitKeyboard( Display *display );
extern BOOL X11DRV_ProcessEvents( DWORD mask );

typedef int (*x11drv_error_callback)( Display *display, XErrorEvent *event, void *arg );

extern void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg );
extern int X11DRV_check_error(void);
extern POINT virtual_screen_to_root( INT x, INT y );
extern POINT root_to_virtual_screen( INT x, INT y );
extern RECT get_host_primary_monitor_rect(void);
extern RECT get_work_area( const RECT *monitor_rect );
extern void xinerama_get_fullscreen_monitors( const RECT *rect, unsigned int *generation, long *indices );
extern void xinerama_init( unsigned int width, unsigned int height );
extern void init_recursive_mutex( pthread_mutex_t *mutex );
extern void init_icm_profile(void);

#define DEPTH_COUNT 3
extern const unsigned int *depths;

/* Use a distinct type for the settings id, to avoid mixups other types of ids */
typedef struct { ULONG_PTR id; } x11drv_settings_id;

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
    BOOL (*get_id)(const WCHAR *device_name, BOOL is_primary, x11drv_settings_id *id);

    /* get_modes() will be called to get a list of supported modes of the device of id in modes
     * with respect to flags, which could be 0, EDS_RAWMODE or EDS_ROTATEDMODE. If the implementation
     * uses dmDriverExtra then every DEVMODEW in the list must have the same dmDriverExtra value
     *
     * Following fields in DEVMODE must be valid:
     * dmSize, dmDriverExtra, dmFields, dmDisplayOrientation, dmBitsPerPel, dmPelsWidth, dmPelsHeight,
     * dmDisplayFlags and dmDisplayFrequency
     *
     * Return FALSE on failure with parameters unchanged and error code set. Return TRUE on success */
    BOOL (*get_modes)(x11drv_settings_id id, DWORD flags, DEVMODEW **modes, UINT *mode_count, BOOL full);

    /* free_modes() will be called to free the mode list returned from get_modes() */
    void (*free_modes)(DEVMODEW *modes);

    /* get_current_mode() will be called to get the current display mode of the device of id
     *
     * Following fields in DEVMODE must be valid:
     * dmFields, dmDisplayOrientation, dmBitsPerPel, dmPelsWidth, dmPelsHeight, dmDisplayFlags,
     * dmDisplayFrequency and dmPosition
     *
     * Return FALSE on failure with parameters unchanged and error code set. Return TRUE on success */
    BOOL (*get_current_mode)(x11drv_settings_id id, DEVMODEW *mode);

    /* set_current_mode() will be called to change the display mode of the display device of id.
     * mode must be a valid mode from get_modes() with optional fields, such as dmPosition set.
     *
     * Return DISP_CHANGE_*, same as ChangeDisplaySettingsExW() return values */
    LONG (*set_current_mode)(x11drv_settings_id id, const DEVMODEW *mode);
};

#define NEXT_DEVMODEW(mode) ((DEVMODEW *)((char *)((mode) + 1) + (mode)->dmDriverExtra))

extern void X11DRV_Settings_SetHandler(const struct x11drv_settings_handler *handler);

extern void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height );
extern BOOL is_virtual_desktop(void);
extern BOOL is_desktop_fullscreen(void);
extern BOOL is_detached_mode(const DEVMODEW *);
void X11DRV_Settings_Init(void);

void X11DRV_XF86VM_Init(void);
void X11DRV_XRandR_Init(void);
void init_user_driver(void);

/* X11 display device handler. Used to initialize display device registry data */

struct x11drv_gpu
{
    ULONG_PTR id;
    char *name;
    struct pci_id pci_id;
    GUID vulkan_uuid;
};

struct x11drv_adapter
{
    ULONG_PTR id;
    DWORD state_flags;
};

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
    BOOL (*get_gpus)(struct x11drv_gpu **gpus, int *count, BOOL get_properties);

    /* get_adapters will be called to get a list of adapters in EnumDisplayDevices context under a GPU.
     * The first adapter has to be primary if GPU is primary.
     *
     * Return FALSE on failure with parameters unchanged */
    BOOL (*get_adapters)(ULONG_PTR gpu_id, struct x11drv_adapter **adapters, int *count);

    /* get_monitors will be called to get a list of monitors in EnumDisplayDevices context under an adapter.
     * The first monitor has to be primary if adapter is primary.
     *
     * Return FALSE on failure with parameters unchanged */
    BOOL (*get_monitors)(ULONG_PTR adapter_id, struct gdi_monitor **monitors, int *count);

    /* free_gpus will be called to free a GPU list from get_gpus */
    void (*free_gpus)(struct x11drv_gpu *gpus, int count);

    /* free_adapters will be called to free an adapter list from get_adapters */
    void (*free_adapters)(struct x11drv_adapter *adapters);

    /* free_monitors will be called to free a monitor list from get_monitors */
    void (*free_monitors)(struct gdi_monitor *monitors, int count);

    /* register_event_handlers will be called to register event handlers.
     * This function pointer is optional and can be NULL when driver doesn't support it */
    void (*register_event_handlers)(void);
};

extern void X11DRV_DisplayDevices_SetHandler(const struct x11drv_display_device_handler *handler);
extern void X11DRV_DisplayDevices_RegisterEventHandlers(void);
extern BOOL X11DRV_DisplayDevices_SupportEventHandlers(void);
/* Display device handler used in virtual desktop mode */
extern struct x11drv_display_device_handler desktop_handler;

/* XIM support */
extern BOOL xim_init( const WCHAR *input_style );
extern void xim_thread_attach( struct x11drv_thread_data *data );
extern BOOL xim_in_compose_mode(void);
extern void xim_set_result_string( HWND hwnd, const char *str, UINT count );
extern XIC X11DRV_get_ic( HWND hwnd );
extern void xim_set_focus( HWND hwnd, BOOL focus );

#define XEMBED_MAPPED  (1 << 0)

static inline BOOL is_window_rect_mapped( const RECT *rect )
{
    RECT virtual_rect = NtUserGetVirtualScreenRect( MDT_RAW_DPI );
    return (rect->left < virtual_rect.right &&
            rect->top < virtual_rect.bottom &&
            max( rect->right, rect->left + 1 ) > virtual_rect.left &&
            max( rect->bottom, rect->top + 1 ) > virtual_rect.top);
}

/* unixlib interface */

extern NTSTATUS x11drv_tablet_attach_queue( void *arg );
extern NTSTATUS x11drv_tablet_get_packet( void *arg );
extern NTSTATUS x11drv_tablet_load_info( void *arg );
extern NTSTATUS x11drv_tablet_info( void *arg );

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
    return NtUserMessageCall( hwnd, msg, wparam, lparam, NULL, NtUserSendMessage, FALSE );
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

extern HKEY open_hkcu_key( const char *name );
extern ULONG query_reg_value( HKEY hkey, const WCHAR *name,
                              KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size );
extern HKEY reg_open_key( HKEY root, const WCHAR *name, ULONG name_len );

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
