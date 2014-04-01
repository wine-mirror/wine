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
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

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
#include "wine/gdi_driver.h"
#include "wine/list.h"

#define MAX_DASHLEN 16

#define WINE_XDND_VERSION 4

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

struct x11drv_gamma_ramp
{
    WORD red[256];
    WORD green[256];
    WORD blue[256];
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
                        INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                          INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern INT X11DRV_EnumICMProfiles( PHYSDEV dev, ICMENUMPROCW proc, LPARAM lparam ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetDeviceGammaRamp( PHYSDEV dev, LPVOID ramp ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GetICMProfile( PHYSDEV dev, LPDWORD size, LPWSTR filename ) DECLSPEC_HIDDEN;
extern DWORD X11DRV_GetImage( PHYSDEV dev, BITMAPINFO *info,
                              struct gdi_image_bits *bits, struct bitblt_coords *src ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_GetNearestColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern UINT X11DRV_GetSystemPaletteEntries( PHYSDEV dev, UINT start, UINT count, LPPALETTEENTRY entries ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                 void *grad_array, ULONG ngrad, ULONG mode ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_LineTo( PHYSDEV dev, INT x, INT y) DECLSPEC_HIDDEN;
extern BOOL X11DRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Pie( PHYSDEV dev, INT left, INT top, INT right,
                        INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Polygon( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polygons) DECLSPEC_HIDDEN;
extern BOOL X11DRV_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polylines) DECLSPEC_HIDDEN;
extern DWORD X11DRV_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                              const struct gdi_image_bits *bits, struct bitblt_coords *src,
                              struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern UINT X11DRV_RealizeDefaultPalette( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern UINT X11DRV_RealizePalette( PHYSDEV dev, HPALETTE hpal, BOOL primary ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL X11DRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                              INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern HBRUSH X11DRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern HPEN X11DRV_SelectPen( PHYSDEV dev, HPEN hpen, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_SetDCBrushColor( PHYSDEV dev, COLORREF crColor ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_SetDCPenColor( PHYSDEV dev, COLORREF crColor ) DECLSPEC_HIDDEN;
extern void X11DRV_SetDeviceClipping( PHYSDEV dev, HRGN rgn ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_SetDeviceGammaRamp( PHYSDEV dev, LPVOID ramp ) DECLSPEC_HIDDEN;
extern COLORREF X11DRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                               PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL X11DRV_UnrealizePalette( HPALETTE hpal ) DECLSPEC_HIDDEN;

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

extern const int X11DRV_XROPfunction[];

extern BOOL client_side_graphics DECLSPEC_HIDDEN;
extern BOOL client_side_with_render DECLSPEC_HIDDEN;
extern BOOL shape_layered_windows DECLSPEC_HIDDEN;
extern const struct gdi_dc_funcs *X11DRV_XRender_Init(void) DECLSPEC_HIDDEN;

extern struct opengl_funcs *get_glx_driver(UINT) DECLSPEC_HIDDEN;

/* IME support */
extern void IME_SetOpenStatus(BOOL fOpen) DECLSPEC_HIDDEN;
extern void IME_SetCompositionStatus(BOOL fOpen) DECLSPEC_HIDDEN;
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
extern void X11DRV_PALETTE_ComputeColorShifts(ColorShifts *shifts, unsigned long redMask, unsigned long greenMask, unsigned long blueMask) DECLSPEC_HIDDEN;

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
    XFontSet font_set;             /* international text drawing font set */
    Window   selection_wnd;        /* window used for selection interactions */
    unsigned long warp_serial;     /* serial number of last pointer warp request */
    Window   clip_window;          /* window used for cursor clipping */
    HWND     clip_hwnd;            /* message window stored in desktop while clipping is active */
    DWORD    clip_reset;           /* time when clipping was last reset */
    HKL      kbd_layout;           /* active keyboard layout */
    enum { xi_unavailable = -1, xi_unknown, xi_disabled, xi_enabled } xi2_state; /* XInput2 state */
    void    *xi2_devices;          /* list of XInput2 devices (valid when state is enabled) */
    int      xi2_device_count;
    int      xi2_core_pointer;     /* XInput2 core pointer id */
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

extern XVisualInfo default_visual DECLSPEC_HIDDEN;
extern XVisualInfo argb_visual DECLSPEC_HIDDEN;
extern Colormap default_colormap DECLSPEC_HIDDEN;
extern XPixmapFormatValues **pixmap_formats DECLSPEC_HIDDEN;
extern Window root_window DECLSPEC_HIDDEN;
extern BOOL clipping_cursor DECLSPEC_HIDDEN;
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
    Display    *display;        /* display connection for the thread owning the window */
    XVisualInfo vis;            /* X visual used by this window */
    Colormap    colormap;       /* colormap if non-default visual */
    HWND        hwnd;           /* hwnd that this private data belongs to */
    Window      whole_window;   /* X window for the complete window */
    Window      client_window;  /* X window for the client area */
    RECT        window_rect;    /* USER window rectangle relative to parent */
    RECT        whole_rect;     /* X window rectangle for the whole window relative to parent */
    RECT        client_rect;    /* client area relative to parent */
    XIC         xic;            /* X input context */
    BOOL        managed : 1;    /* is window managed? */
    BOOL        mapped : 1;     /* is window mapped? (in either normal or iconic state) */
    BOOL        iconic : 1;     /* is window in iconic state? */
    BOOL        embedded : 1;   /* is window an XEMBED client? */
    BOOL        shaped : 1;     /* is window using a custom region shape? */
    BOOL        layered : 1;    /* is window layered and with valid attributes? */
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

extern void sync_gl_drawable( HWND hwnd, const RECT *visible_rect, const RECT *client_rect ) DECLSPEC_HIDDEN;
extern void set_gl_drawable_parent( HWND hwnd, HWND parent ) DECLSPEC_HIDDEN;
extern void destroy_gl_drawable( HWND hwnd ) DECLSPEC_HIDDEN;

extern void wait_for_withdrawn_state( HWND hwnd, BOOL set ) DECLSPEC_HIDDEN;
extern Window init_clip_window(void) DECLSPEC_HIDDEN;
extern void update_user_time( Time time ) DECLSPEC_HIDDEN;
extern void update_net_wm_states( struct x11drv_win_data *data ) DECLSPEC_HIDDEN;
extern void make_window_embedded( struct x11drv_win_data *data ) DECLSPEC_HIDDEN;
extern Window create_client_window( struct x11drv_win_data *data, const XVisualInfo *visual ) DECLSPEC_HIDDEN;
extern void set_window_visual( struct x11drv_win_data *data, const XVisualInfo *vis ) DECLSPEC_HIDDEN;
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

/* X context to associate a hwnd to an X window */
extern XContext winContext DECLSPEC_HIDDEN;
/* X context to associate a struct x11drv_win_data to an hwnd */
extern XContext win_data_context DECLSPEC_HIDDEN;
/* X context to associate an X cursor to a Win32 cursor handle */
extern XContext cursor_context DECLSPEC_HIDDEN;

extern void X11DRV_InitClipboard(void) DECLSPEC_HIDDEN;
extern int CDECL X11DRV_AcquireClipboard(HWND hWndClipWindow) DECLSPEC_HIDDEN;
extern void X11DRV_ResetSelectionOwner(void) DECLSPEC_HIDDEN;
extern void CDECL X11DRV_SetFocus( HWND hwnd ) DECLSPEC_HIDDEN;
extern void set_window_cursor( Window window, HCURSOR handle ) DECLSPEC_HIDDEN;
extern void sync_window_cursor( Window window ) DECLSPEC_HIDDEN;
extern LRESULT clip_cursor_notify( HWND hwnd, HWND new_clip_hwnd ) DECLSPEC_HIDDEN;
extern void ungrab_clipping_window(void) DECLSPEC_HIDDEN;
extern void reset_clipping_window(void) DECLSPEC_HIDDEN;
extern BOOL clip_fullscreen_window( HWND hwnd, BOOL reset ) DECLSPEC_HIDDEN;
extern void move_resize_window( HWND hwnd, int dir ) DECLSPEC_HIDDEN;
extern void X11DRV_InitKeyboard( Display *display ) DECLSPEC_HIDDEN;
extern DWORD CDECL X11DRV_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                       DWORD mask, DWORD flags ) DECLSPEC_HIDDEN;

typedef int (*x11drv_error_callback)( Display *display, XErrorEvent *event, void *arg );

extern void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg ) DECLSPEC_HIDDEN;
extern int X11DRV_check_error(void) DECLSPEC_HIDDEN;
extern void X11DRV_X_to_window_rect( struct x11drv_win_data *data, RECT *rect ) DECLSPEC_HIDDEN;
extern POINT virtual_screen_to_root( INT x, INT y ) DECLSPEC_HIDDEN;
extern POINT root_to_virtual_screen( INT x, INT y ) DECLSPEC_HIDDEN;
extern RECT get_virtual_screen_rect(void) DECLSPEC_HIDDEN;
extern RECT get_primary_monitor_rect(void) DECLSPEC_HIDDEN;
extern void xinerama_init( unsigned int width, unsigned int height ) DECLSPEC_HIDDEN;

struct x11drv_mode_info
{
    unsigned int width;
    unsigned int height;
    unsigned int bpp;
    unsigned int refresh_rate;
};

extern void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height ) DECLSPEC_HIDDEN;
extern void X11DRV_resize_desktop(unsigned int width, unsigned int height) DECLSPEC_HIDDEN;
extern BOOL is_desktop_fullscreen(void) DECLSPEC_HIDDEN;
extern BOOL create_desktop_win_data( Window win ) DECLSPEC_HIDDEN;
extern void X11DRV_Settings_AddDepthModes(void) DECLSPEC_HIDDEN;
extern void X11DRV_Settings_AddOneMode(unsigned int width, unsigned int height, unsigned int bpp, unsigned int freq) DECLSPEC_HIDDEN;
unsigned int X11DRV_Settings_GetModeCount(void) DECLSPEC_HIDDEN;
void X11DRV_Settings_Init(void) DECLSPEC_HIDDEN;
struct x11drv_mode_info *X11DRV_Settings_SetHandlers(const char *name,
                                                     int (*pNewGCM)(void),
                                                     LONG (*pNewSCM)(int),
                                                     unsigned int nmodes,
                                                     int reserve_depths) DECLSPEC_HIDDEN;

void X11DRV_XF86VM_Init(void) DECLSPEC_HIDDEN;
void X11DRV_XRandR_Init(void) DECLSPEC_HIDDEN;

/* XIM support */
extern BOOL X11DRV_InitXIM( const char *input_style ) DECLSPEC_HIDDEN;
extern XIC X11DRV_CreateIC(XIM xim, struct x11drv_win_data *data) DECLSPEC_HIDDEN;
extern void X11DRV_SetupXIM(void) DECLSPEC_HIDDEN;
extern void X11DRV_XIMLookupChars( const char *str, DWORD count ) DECLSPEC_HIDDEN;
extern void X11DRV_ForceXIMReset(HWND hwnd) DECLSPEC_HIDDEN;
extern void X11DRV_SetPreeditState(HWND hwnd, BOOL fOpen) DECLSPEC_HIDDEN;

#define XEMBED_MAPPED  (1 << 0)

static inline BOOL is_window_rect_mapped( const RECT *rect )
{
    RECT virtual_rect = get_virtual_screen_rect();
    return (rect->left < virtual_rect.right &&
            rect->top < virtual_rect.bottom &&
            max( rect->right, rect->left + 1 ) > virtual_rect.left &&
            max( rect->bottom, rect->top + 1 ) > virtual_rect.top);
}

static inline BOOL is_window_rect_fullscreen( const RECT *rect )
{
    RECT primary_rect = get_primary_monitor_rect();
    return (rect->left <= primary_rect.left && rect->right >= primary_rect.right &&
            rect->top <= primary_rect.top && rect->bottom >= primary_rect.bottom);
}

#endif  /* __WINE_X11DRV_H */
