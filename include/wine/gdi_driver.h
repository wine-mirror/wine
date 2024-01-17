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

#include "winternl.h"
#include "ntuser.h"
#include "immdev.h"
#include "shellapi.h"
#include "ddk/d3dkmthk.h"
#include "kbd.h"
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
    NTSTATUS (*pD3DKMTCheckVidPnExclusiveOwnership)(const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *);
    NTSTATUS (*pD3DKMTCloseAdapter)(const D3DKMT_CLOSEADAPTER *);
    NTSTATUS (*pD3DKMTOpenAdapterFromLuid)(D3DKMT_OPENADAPTERFROMLUID *);
    NTSTATUS (*pD3DKMTQueryVideoMemoryInfo)(D3DKMT_QUERYVIDEOMEMORYINFO *);
    NTSTATUS (*pD3DKMTSetVidPnSourceOwner)(const D3DKMT_SETVIDPNSOURCEOWNER *);

    /* priority order for the driver on the stack */
    UINT       priority;
};

/* increment this when you change the DC function table */
#define WINE_GDI_DRIVER_VERSION 83

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
    DWORD                              draw_start_ticks; /* start ticks of fresh draw */
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

/* display manager interface, used to initialize display device registry data */

struct gdi_gpu
{
    ULONG_PTR id;
    WCHAR name[128];      /* name */
    UINT vendor_id;       /* PCI ID */
    UINT device_id;
    UINT subsys_id;
    UINT revision_id;
    GUID vulkan_uuid;     /* Vulkan device UUID */
    ULONGLONG memory_size;
};

struct gdi_adapter
{
    ULONG_PTR id;
    DWORD state_flags;
};

struct gdi_monitor
{
    RECT rc_monitor;      /* RcMonitor in MONITORINFO struct */
    RECT rc_work;         /* RcWork in MONITORINFO struct */
    DWORD state_flags;    /* StateFlags in DISPLAY_DEVICE struct */
    unsigned char *edid;  /* Extended Device Identification Data */
    UINT edid_len;
};

struct gdi_device_manager
{
    void (*add_gpu)( const struct gdi_gpu *gpu, void *param );
    void (*add_adapter)( const struct gdi_adapter *adapter, void *param );
    void (*add_monitor)( const struct gdi_monitor *monitor, void *param );
    void (*add_mode)( const DEVMODEW *mode, BOOL current, void *param );
};

#define WINE_DM_UNSUPPORTED 0x80000000

struct tagUPDATELAYEREDWINDOWINFO;

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
    BOOL    (*pGetCurrentDisplaySettings)(LPCWSTR,BOOL,LPDEVMODEW);
    INT     (*pGetDisplayDepth)(LPCWSTR,BOOL);
    BOOL    (*pUpdateDisplayDevices)(const struct gdi_device_manager *,BOOL,void*);
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
    void    (*pSetFocus)(HWND);
    void    (*pSetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
    void    (*pSetParent)(HWND,HWND,HWND);
    void    (*pSetWindowRgn)(HWND,HRGN,BOOL);
    void    (*pSetWindowIcon)(HWND,UINT,HICON);
    void    (*pSetWindowStyle)(HWND,INT,STYLESTRUCT*);
    void    (*pSetWindowText)(HWND,LPCWSTR);
    UINT    (*pShowWindow)(HWND,INT,RECT*,UINT);
    LRESULT (*pSysCommand)(HWND,WPARAM,LPARAM);
    BOOL    (*pUpdateLayeredWindow)(HWND,const struct tagUPDATELAYEREDWINDOWINFO *,const RECT *);
    LRESULT (*pWindowMessage)(HWND,UINT,WPARAM,LPARAM);
    BOOL    (*pWindowPosChanging)(HWND,HWND,UINT,const RECT *,const RECT *,RECT *,
                                  struct window_surface**);
    void    (*pWindowPosChanged)(HWND,HWND,UINT,const RECT *,const RECT *,const RECT *,
                                 const RECT *,struct window_surface*);
    /* system parameters */
    BOOL    (*pSystemParametersInfo)(UINT,UINT,void*,UINT);
    /* vulkan support */
    const struct vulkan_funcs * (*pwine_get_vulkan_driver)(UINT);
    /* opengl support */
    struct opengl_funcs * (*pwine_get_wgl_driver)(UINT);
    /* thread management */
    void    (*pThreadDetach)(void);
};

W32KAPI void __wine_set_user_driver( const struct user_driver_funcs *funcs, UINT version );

W32KAPI BOOL win32u_set_window_pixel_format( HWND hwnd, int format, BOOL internal );
W32KAPI int win32u_get_window_pixel_format( HWND hwnd );

#endif /* __WINE_WINE_GDI_DRIVER_H */
