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

#ifndef WINE_UNIX_LIB
#error The GDI driver can only be used on the Unix side
#endif

#include <stdarg.h>
#include <stddef.h>

#include <pthread.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntuser.h"
#include "immdev.h"
#include "shellapi.h"
#include "ddk/d3dkmthk.h"
#include "kbd.h"
#include "wine/list.h"
#include "wine/debug.h"

struct gdi_dc_funcs;
struct opengl_funcs;
struct vulkan_funcs;

struct window_rects
{
    RECT window;    /* window area, including non-client frame */
    RECT client;    /* client area, excluding non-client frame */
    RECT visible;   /* area currently visible on the host screen, backed with a surface */
};

static inline const char *debugstr_window_rects( const struct window_rects *rects )
{
    return wine_dbg_sprintf( "{ window %s, client %s, visible %s }", wine_dbgstr_rect( &rects->window ),
                             wine_dbgstr_rect( &rects->client ), wine_dbgstr_rect( &rects->visible ) );
}

/* convert a visible rect to the corresponding window rect, using the window_rects offsets */
static inline RECT window_rect_from_visible( const struct window_rects *rects, RECT visible_rect )
{
    RECT rect = visible_rect;

    rect.left += rects->window.left - rects->visible.left;
    rect.top += rects->window.top - rects->visible.top;
    rect.right += rects->window.right - rects->visible.right;
    rect.bottom += rects->window.bottom - rects->visible.bottom;

    return rect;
}

/* convert a window rect to the corresponding visible rect, using the window_rects offsets */
static inline RECT visible_rect_from_window( struct window_rects *rects, RECT window_rect )
{
    RECT rect = window_rect;

    rect.left += rects->visible.left - rects->window.left;
    rect.top += rects->visible.top - rects->window.top;
    rect.right += rects->visible.right - rects->window.right;
    rect.bottom += rects->visible.bottom - rects->window.bottom;

    return rect;
}

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

typedef int (*font_enum_proc)(const LOGFONTW *, const TEXTMETRICW *, DWORD, LPARAM);

struct gdi_dc_funcs
{
    INT      (*pAbortDoc)(PHYSDEV);
    BOOL     (*pAbortPath)(PHYSDEV);
    BOOL     (*pAlphaBlend)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,BLENDFUNCTION);
    BOOL     (*pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    BOOL     (*pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pBeginPath)(PHYSDEV);
    DWORD    (*pBlendImage)(PHYSDEV,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,BLENDFUNCTION);
    BOOL     (*pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pCloseFigure)(PHYSDEV);
    BOOL     (*pCreateCompatibleDC)(PHYSDEV,PHYSDEV*);
    BOOL     (*pCreateDC)(PHYSDEV*,LPCWSTR,LPCWSTR,const DEVMODEW*);
    BOOL     (*pDeleteDC)(PHYSDEV);
    BOOL     (*pDeleteObject)(PHYSDEV,HGDIOBJ);
    BOOL     (*pEllipse)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pEndDoc)(PHYSDEV);
    INT      (*pEndPage)(PHYSDEV);
    BOOL     (*pEndPath)(PHYSDEV);
    BOOL     (*pEnumFonts)(PHYSDEV,LPLOGFONTW,font_enum_proc,LPARAM);
    INT      (*pExtEscape)(PHYSDEV,INT,INT,LPCVOID,INT,LPVOID);
    BOOL     (*pExtFloodFill)(PHYSDEV,INT,INT,COLORREF,UINT);
    BOOL     (*pExtTextOut)(PHYSDEV,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
    BOOL     (*pFillPath)(PHYSDEV);
    BOOL     (*pFillRgn)(PHYSDEV,HRGN,HBRUSH);
    BOOL     (*pFontIsLinked)(PHYSDEV);
    BOOL     (*pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    UINT     (*pGetBoundsRect)(PHYSDEV,RECT*,UINT);
    BOOL     (*pGetCharABCWidths)(PHYSDEV,UINT,UINT,WCHAR*,LPABC);
    BOOL     (*pGetCharABCWidthsI)(PHYSDEV,UINT,UINT,WORD*,LPABC);
    BOOL     (*pGetCharWidth)(PHYSDEV,UINT,UINT,const WCHAR*,LPINT);
    BOOL     (*pGetCharWidthInfo)(PHYSDEV,void*);
    INT      (*pGetDeviceCaps)(PHYSDEV,INT);
    BOOL     (*pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    DWORD    (*pGetFontData)(PHYSDEV,DWORD,DWORD,LPVOID,DWORD);
    BOOL     (*pGetFontRealizationInfo)(PHYSDEV,void*);
    DWORD    (*pGetFontUnicodeRanges)(PHYSDEV,LPGLYPHSET);
    DWORD    (*pGetGlyphIndices)(PHYSDEV,LPCWSTR,INT,LPWORD,DWORD);
    DWORD    (*pGetGlyphOutline)(PHYSDEV,UINT,UINT,LPGLYPHMETRICS,DWORD,LPVOID,const MAT2*);
    BOOL     (*pGetICMProfile)(PHYSDEV,BOOL,LPDWORD,LPWSTR);
    DWORD    (*pGetImage)(PHYSDEV,BITMAPINFO*,struct gdi_image_bits*,struct bitblt_coords*);
    DWORD    (*pGetKerningPairs)(PHYSDEV,DWORD,LPKERNINGPAIR);
    COLORREF (*pGetNearestColor)(PHYSDEV,COLORREF);
    UINT     (*pGetOutlineTextMetrics)(PHYSDEV,UINT,LPOUTLINETEXTMETRICW);
    COLORREF (*pGetPixel)(PHYSDEV,INT,INT);
    UINT     (*pGetSystemPaletteEntries)(PHYSDEV,UINT,UINT,LPPALETTEENTRY);
    UINT     (*pGetTextCharsetInfo)(PHYSDEV,LPFONTSIGNATURE,DWORD);
    BOOL     (*pGetTextExtentExPoint)(PHYSDEV,LPCWSTR,INT,LPINT);
    BOOL     (*pGetTextExtentExPointI)(PHYSDEV,const WORD*,INT,LPINT);
    INT      (*pGetTextFace)(PHYSDEV,INT,LPWSTR);
    BOOL     (*pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    BOOL     (*pGradientFill)(PHYSDEV,TRIVERTEX*,ULONG,void*,ULONG,ULONG);
    BOOL     (*pInvertRgn)(PHYSDEV,HRGN);
    BOOL     (*pLineTo)(PHYSDEV,INT,INT);
    BOOL     (*pMoveTo)(PHYSDEV,INT,INT);
    BOOL     (*pPaintRgn)(PHYSDEV,HRGN);
    BOOL     (*pPatBlt)(PHYSDEV,struct bitblt_coords*,DWORD);
    BOOL     (*pPie)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pPolyBezier)(PHYSDEV,const POINT*,DWORD);
    BOOL     (*pPolyBezierTo)(PHYSDEV,const POINT*,DWORD);
    BOOL     (*pPolyDraw)(PHYSDEV,const POINT*,const BYTE *,DWORD);
    BOOL     (*pPolyPolygon)(PHYSDEV,const POINT*,const INT*,UINT);
    BOOL     (*pPolyPolyline)(PHYSDEV,const POINT*,const DWORD*,DWORD);
    BOOL     (*pPolylineTo)(PHYSDEV,const POINT*,INT);
    DWORD    (*pPutImage)(PHYSDEV,HRGN,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,DWORD);
    UINT     (*pRealizeDefaultPalette)(PHYSDEV);
    UINT     (*pRealizePalette)(PHYSDEV,HPALETTE,BOOL);
    BOOL     (*pRectangle)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (*pResetDC)(PHYSDEV,const DEVMODEW*);
    BOOL     (*pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    HBITMAP  (*pSelectBitmap)(PHYSDEV,HBITMAP);
    HBRUSH   (*pSelectBrush)(PHYSDEV,HBRUSH,const struct brush_pattern*);
    HFONT    (*pSelectFont)(PHYSDEV,HFONT,UINT*);
    HPEN     (*pSelectPen)(PHYSDEV,HPEN,const struct brush_pattern*);
    COLORREF (*pSetBkColor)(PHYSDEV,COLORREF);
    UINT     (*pSetBoundsRect)(PHYSDEV,RECT*,UINT);
    COLORREF (*pSetDCBrushColor)(PHYSDEV, COLORREF);
    COLORREF (*pSetDCPenColor)(PHYSDEV, COLORREF);
    INT      (*pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,BITMAPINFO*,UINT);
    VOID     (*pSetDeviceClipping)(PHYSDEV,HRGN);
    BOOL     (*pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    COLORREF (*pSetPixel)(PHYSDEV,INT,INT,COLORREF);
    COLORREF (*pSetTextColor)(PHYSDEV,COLORREF);
    INT      (*pStartDoc)(PHYSDEV,const DOCINFOW*);
    INT      (*pStartPage)(PHYSDEV);
    BOOL     (*pStretchBlt)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,DWORD);
    INT      (*pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void*,BITMAPINFO*,UINT,DWORD);
    BOOL     (*pStrokeAndFillPath)(PHYSDEV);
    BOOL     (*pStrokePath)(PHYSDEV);
    BOOL     (*pUnrealizePalette)(HPALETTE);

    /* priority order for the driver on the stack */
    UINT       priority;
};

/* increment this when you change the DC function table */
#define WINE_GDI_DRIVER_VERSION 105

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

/* support for client surfaces */

struct client_surface;
struct client_surface_funcs
{
    void (*destroy)( struct client_surface *surface );
};

struct client_surface
{
    const struct client_surface_funcs *funcs;
    LONG                               ref;            /* reference count */
    HWND                               hwnd;           /* window the surface was created for */
};

W32KAPI void *client_surface_create( UINT size, const struct client_surface_funcs *funcs, HWND hwnd );
W32KAPI void client_surface_add_ref( struct client_surface *surface );
W32KAPI void client_surface_release( struct client_surface *surface );

static inline const char *debugstr_client_surface( struct client_surface *surface )
{
    if (!surface) return "(null)";
    return wine_dbg_sprintf( "%p/%p", surface->hwnd, surface );
}

/* support for window surfaces */

struct window_surface;

struct window_surface_funcs
{
    void  (*set_clip)( struct window_surface *surface, const RECT *rects, UINT count );
    BOOL  (*flush)( struct window_surface *surface, const RECT *rect, const RECT *dirty,
                    const BITMAPINFO *color_info, const void *color_bits, BOOL shape_changed,
                    const BITMAPINFO *shape_info, const void *shape_bits );
    void  (*destroy)( struct window_surface *surface );
};

struct window_surface
{
    const struct window_surface_funcs *funcs; /* driver-specific implementations  */
    struct list                        entry; /* entry in global list managed by user32 */
    LONG                               ref;   /* reference count */
    HWND                               hwnd;  /* window the surface was created for */
    RECT                               rect;  /* constant, no locking needed */

    pthread_mutex_t                    mutex;        /* mutex needed for any field below */
    RECT                               bounds;       /* dirty area rectangle */
    HRGN                               clip_region;  /* visible region of the surface, fully visible if 0 */
    DWORD                              draw_start_ticks; /* start ticks of fresh draw */
    COLORREF                           color_key;    /* layered window surface color key, invalid if CLR_INVALID */
    UINT                               alpha_bits;   /* layered window global alpha bits, invalid if -1 */
    UINT                               alpha_mask;   /* layered window per-pixel alpha mask, invalid if 0 */
    HRGN                               shape_region; /* shape of the window surface, unshaped if 0 */
    HBITMAP                            shape_bitmap; /* bitmap for the surface shape (1bpp) */
    HBITMAP                            color_bitmap; /* bitmap for the surface colors */
    /* driver-specific fields here */
};

W32KAPI struct window_surface *window_surface_create( UINT size, const struct window_surface_funcs *funcs, HWND hwnd,
                                                      const RECT *rect, BITMAPINFO *info, HBITMAP bitmap );
W32KAPI void window_surface_add_ref( struct window_surface *surface );
W32KAPI void window_surface_release( struct window_surface *surface );
W32KAPI void window_surface_lock( struct window_surface *surface );
W32KAPI void window_surface_unlock( struct window_surface *surface );
W32KAPI void window_surface_set_layered( struct window_surface *surface, COLORREF color_key, UINT alpha_bits, UINT alpha_mask );
W32KAPI void window_surface_flush( struct window_surface *surface );
W32KAPI void window_surface_set_clip( struct window_surface *surface, HRGN clip_region );
W32KAPI void window_surface_set_shape( struct window_surface *surface, HRGN shape_region );
W32KAPI void window_surface_set_layered( struct window_surface *surface, COLORREF color_key, UINT alpha_bits, UINT alpha_mask );

/* display manager interface, used to initialize display device registry data */

struct pci_id
{
    UINT16 vendor;
    UINT16 device;
    UINT16 subsystem;
    UINT16 revision;
};

struct gdi_monitor
{
    RECT rc_monitor;      /* RcMonitor in MONITORINFO struct */
    RECT rc_work;         /* RcWork in MONITORINFO struct */
    unsigned char *edid;  /* Extended Device Identification Data */
    UINT edid_len;
};

struct gdi_device_manager
{
    void (*add_gpu)( const char *name, const struct pci_id *pci_id, const GUID *vulkan_uuid, void *param );
    void (*add_source)( const char *name, UINT state_flags, UINT dpi, void *param );
    void (*add_monitor)( const struct gdi_monitor *monitor, void *param );
    void (*add_modes)( const DEVMODEW *current, UINT modes_count, const DEVMODEW *modes, void *param );
};

#define WINE_DM_UNSUPPORTED 0x80000000

struct vulkan_driver_funcs;
struct opengl_driver_funcs;

struct user_driver_funcs
{
    struct gdi_dc_funcs dc_funcs;

    /* keyboard functions */
    BOOL    (*pActivateKeyboardLayout)(HKL, UINT);
    void    (*pBeep)(void);
    INT     (*pGetKeyNameText)(LONG,LPWSTR,INT);
    UINT    (*pGetKeyboardLayoutList)(INT, HKL *);
    UINT    (*pMapVirtualKeyEx)(UINT,UINT,HKL);
    BOOL    (*pRegisterHotKey)(HWND,UINT,UINT);
    INT     (*pToUnicodeEx)(UINT,UINT,const BYTE *,LPWSTR,int,UINT,HKL);
    void    (*pUnregisterHotKey)(HWND, UINT, UINT);
    SHORT   (*pVkKeyScanEx)(WCHAR, HKL);
    const KBDTABLES *(*pKbdLayerDescriptor)(HKL);
    void    (*pReleaseKbdTables)(const KBDTABLES *);
    /* IME functions */
    UINT    (*pImeProcessKey)(HIMC,UINT,UINT,const BYTE*);
    void    (*pNotifyIMEStatus)(HWND,UINT);
    BOOL    (*pSetIMECompositionRect)(HWND,RECT);
    /* cursor/icon functions */
    void    (*pDestroyCursorIcon)(HCURSOR);
    void    (*pSetCursor)(HWND,HCURSOR);
    BOOL    (*pGetCursorPos)(LPPOINT);
    BOOL    (*pSetCursorPos)(INT,INT);
    BOOL    (*pClipCursor)(const RECT*,BOOL);
    /* notify icon functions */
    LRESULT (*pNotifyIcon)(HWND,UINT,NOTIFYICONDATAW *);
    void    (*pCleanupIcons)(HWND);
    void    (*pSystrayDockInit)(HWND);
    BOOL    (*pSystrayDockInsert)(HWND,UINT,UINT,void *);
    void    (*pSystrayDockClear)(HWND);
    BOOL    (*pSystrayDockRemove)(HWND);
    /* clipboard functions */
    LRESULT (*pClipboardWindowProc)(HWND,UINT,WPARAM,LPARAM);
    void    (*pUpdateClipboard)(void);
    /* display modes */
    LONG    (*pChangeDisplaySettings)(LPDEVMODEW,LPCWSTR,HWND,DWORD,LPVOID);
    UINT    (*pUpdateDisplayDevices)(const struct gdi_device_manager *,void*);
    /* windowing functions */
    BOOL    (*pCreateDesktop)(const WCHAR *,UINT,UINT);
    BOOL    (*pCreateWindow)(HWND);
    LRESULT (*pDesktopWindowProc)(HWND,UINT,WPARAM,LPARAM);
    void    (*pDestroyWindow)(HWND);
    void    (*pFlashWindowEx)(FLASHWINFO*);
    void    (*pGetDC)(HDC,HWND,HWND,const RECT *,const RECT *,DWORD);
    BOOL    (*pProcessEvents)(DWORD);
    void    (*pReleaseDC)(HWND,HDC);
    BOOL    (*pScrollDC)(HDC,INT,INT,HRGN);
    void    (*pSetCapture)(HWND,UINT);
    void    (*pSetDesktopWindow)(HWND);
    void    (*pActivateWindow)(HWND,HWND);
    void    (*pSetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
    void    (*pSetParent)(HWND,HWND,HWND);
    void    (*pSetWindowRgn)(HWND,HRGN,BOOL);
    void    (*pSetWindowIcon)(HWND,UINT,HICON);
    void    (*pSetWindowStyle)(HWND,INT,STYLESTRUCT*);
    void    (*pSetWindowText)(HWND,LPCWSTR);
    UINT    (*pShowWindow)(HWND,INT,RECT*,UINT);
    LRESULT (*pSysCommand)(HWND,WPARAM,LPARAM,const POINT*);
    void    (*pUpdateLayeredWindow)(HWND,BYTE,UINT);
    LRESULT (*pWindowMessage)(HWND,UINT,WPARAM,LPARAM);
    BOOL    (*pWindowPosChanging)(HWND,UINT,BOOL,const struct window_rects *);
    BOOL    (*pGetWindowStyleMasks)(HWND,UINT,UINT,UINT*,UINT*);
    BOOL    (*pGetWindowStateUpdates)(HWND,UINT*,UINT*,RECT*,HWND*);
    BOOL    (*pCreateWindowSurface)(HWND,BOOL,const RECT *,struct window_surface**);
    void    (*pMoveWindowBits)(HWND,const struct window_rects *,const struct window_rects *,const RECT *);
    void    (*pWindowPosChanged)(HWND,HWND,HWND,UINT,BOOL,const struct window_rects*,struct window_surface*);
    /* system parameters */
    BOOL    (*pSystemParametersInfo)(UINT,UINT,void*,UINT);
    /* vulkan support */
    UINT    (*pVulkanInit)(UINT,void *,const struct vulkan_driver_funcs **);
    /* opengl support */
    UINT    (*pOpenGLInit)(UINT,const struct opengl_funcs *,const struct opengl_driver_funcs **);
    /* thread management */
    void    (*pThreadDetach)(void);
};

W32KAPI void __wine_set_user_driver( const struct user_driver_funcs *funcs, UINT version );

#endif /* __WINE_WINE_GDI_DRIVER_H */
