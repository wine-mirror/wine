#ifndef __GRAPHICS_WINE_DDRAW_PRIVATE_H
#define __GRAPHICS_WINE_DDRAW_PRIVATE_H

#include "ddraw.h"

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectDrawPaletteImpl IDirectDrawPaletteImpl;
typedef struct IDirectDrawClipperImpl IDirectDrawClipperImpl;
typedef struct IDirectDrawImpl IDirectDrawImpl;
typedef struct IDirectDraw2Impl IDirectDraw2Impl;
typedef struct IDirectDraw4Impl IDirectDraw4Impl;
typedef struct IDirectDrawSurfaceImpl IDirectDrawSurfaceImpl;
typedef struct IDirectDrawSurface3Impl IDirectDrawSurface2Impl;
typedef struct IDirectDrawSurface4Impl IDirectDrawSurface3Impl;
typedef struct IDirectDrawSurface4Impl IDirectDrawSurface4Impl;
typedef struct IDirectDrawColorControlImpl IDirectDrawColorControlImpl;

#include "d3d_private.h"

/*****************************************************************************
 * IDirectDrawPalette implementation structure
 */
struct IDirectDrawPaletteImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDrawPalette)* lpvtbl;
    DWORD                            ref;
    /* IDirectDrawPalette fields */
    IDirectDrawImpl* ddraw;
    Colormap         cm;
    PALETTEENTRY     palents[256];
    int              installed;
    /* This is to store the palette in 'screen format' */
    int              screen_palents[256];
};

/*****************************************************************************
 * IDirectDrawClipper implementation structure
 */
struct IDirectDrawClipperImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDrawClipper)* lpvtbl;
    DWORD                            ref;
    /* IDirectDrawClipper fields */
    /* none */
};

/*****************************************************************************
 * IDirectDraw implementation structure
 */
struct _common_directdrawdata
{
    DDPIXELFORMAT directdraw_pixelformat;
    DDPIXELFORMAT screen_pixelformat;
    int           pixmap_depth;
    void (*pixel_convert)(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette);
    void (*palette_convert)(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count);
    DWORD         height,width;	/* SetDisplayMode */
    HWND          mainWindow;   /* SetCooperativeLevel */

    /* This is for Wine's fake mainWindow.
       We need it also in DGA mode to make some games (for example Monkey Island III work) */
    ATOM          winclass;
    HWND          window;
    Window        drawable;
    PAINTSTRUCT   ps;
    int           paintable;
};

struct _dga_directdrawdata
{
    DWORD        fb_width,fb_height,fb_banksize,fb_memsize;
    void*        fb_addr;
    unsigned int vpmask;
};

struct _xlib_directdrawdata
{
#ifdef HAVE_LIBXXSHM
    int xshm_active;
#endif /* defined(HAVE_LIBXXSHM) */
    
    /* are these needed for anything? (draw_surf is the active surface)
    IDirectDrawSurfaceImpl* surfs;
    DWORD		num_surfs, alloc_surfs, draw_surf; */
};

struct IDirectDrawImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDraw)* lpvtbl;
    DWORD                     ref;
    /* IDirectDraw fields */
    struct _common_directdrawdata   d;
    union {
        struct _xlib_directdrawdata xlib;
        struct _dga_directdrawdata  dga;
    } e;
};

/*****************************************************************************
 * IDirectDraw2 implementation structure
 */
struct IDirectDraw2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDraw2)* lpvtbl;
    DWORD                      ref;
    /* IDirectDraw2 fields */
    struct _common_directdrawdata   d;
    union {
        struct _xlib_directdrawdata xlib;
        struct _dga_directdrawdata  dga;
    } e;
};

/*****************************************************************************
 * IDirectDraw4 implementation structure
 */
struct IDirectDraw4Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDraw4)* lpvtbl;
    DWORD                      ref;
    /* IDirectDraw4 fields */
    struct _common_directdrawdata   d;
    union {
        struct _xlib_directdrawdata xlib;
        struct _dga_directdrawdata  dga;
    } e;
};

/*****************************************************************************
 * IDirectDrawSurface implementation structure
 */
struct _common_directdrawsurface
{
    IDirectDrawPaletteImpl*     palette;
    IDirectDraw2Impl*           ddraw;
    IDirectDrawSurface4Impl*    backbuffer;

    DDSURFACEDESC               surface_desc;

    /* Callback for loaded textures */
    IDirect3DTexture2Impl*      texture;
    HRESULT WINAPI            (*SetColorKey_cb)(IDirect3DTexture2Impl *texture, DWORD dwFlags, LPDDCOLORKEY ckey ) ;
};

struct _dga_directdrawsurface
{
    DWORD		fb_height;
};

struct _xlib_directdrawsurface
{
    XImage		*image;
#ifdef HAVE_LIBXXSHM
    XShmSegmentInfo	shminfo;
#endif
};

struct IDirectDrawSurfaceImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDrawSurface)* lpvtbl;
    DWORD                            ref;
    /* IDirectDrawSurface fields */
    struct _common_directdrawsurface	s;
    union {
	struct _dga_directdrawsurface	dga;
	struct _xlib_directdrawsurface	xlib;
    } t;
};

/*****************************************************************************
 * IDirectDrawSurface2 implementation structure
 */
struct IDirectDrawSurface2Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDrawSurface2)* lpvtbl;
    DWORD                             ref;
    /* IDirectDrawSurface2 fields */
    struct _common_directdrawsurface	s;
    union {
	struct _dga_directdrawsurface	dga;
	struct _xlib_directdrawsurface	xlib;
    } t;
};

/*****************************************************************************
 * IDirectDrawSurface3 implementation structure
 */
struct IDirectDrawSurface3Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDrawSurface3)* lpvtbl;
    DWORD                             ref;
    /* IDirectDrawSurface3 fields */
    struct _common_directdrawsurface	s;
    union {
	struct _dga_directdrawsurface	dga;
	struct _xlib_directdrawsurface	xlib;
    } t;
};

/*****************************************************************************
 * IDirectDrawSurface4 implementation structure
 */
struct IDirectDrawSurface4Impl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDrawSurface4)* lpvtbl;
    DWORD                             ref;
    /* IDirectDrawSurface4 fields */
    struct _common_directdrawsurface	s;
    union {
	struct _dga_directdrawsurface	dga;
	struct _xlib_directdrawsurface	xlib;
    } t;
} ;

/*****************************************************************************
 * IDirectDrawColorControl implementation structure
 */
struct IDirectDrawColorControlImpl
{
    /* IUnknown fields */
    ICOM_VTABLE(IDirectDrawColorControl)* lpvtbl;
    DWORD                                 ref;
    /* IDirectDrawColorControl fields */
    /* none */
};


#endif /* __GRAPHICS_WINE_DDRAW_PRIVATE_H */
